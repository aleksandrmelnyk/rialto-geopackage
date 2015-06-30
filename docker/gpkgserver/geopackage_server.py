#!/usr/bin/env python
#******************************************************************************
# Copyright (c) 2015, RadiantBlue Technologies, Inc.
#
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following
# conditions are met:
#
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in
#       the documentation and/or other materials provided
#       with the distribution.
#     * Neither the name of Hobu, Inc. or Flaxen Geo Consulting nor the
#       names of its contributors may be used to endorse or promote
#       products derived from this software without specific prior
#       written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
# FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
# COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
# OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
# AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
# OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
# OF SUCH DAMAGE.
#***************************************************************************/


#
# GET /             - returns JSON of all the databases available
# GET /file         - returns JSON of all the tables in the GeoPkg database named "file.gpkg"
# GET /file/tab     - returns JSON of info about table "tab" in "file.gpkg"
# GET /file/tab/L/X/Y   - returns the tile at level L, column X, row Y (as binary data)
#


import glob
import json
import os.path
import Queue
import SimpleHTTPServer
import socket
import sqlite3
import sys
import threading
import time    

from BaseHTTPServer import HTTPServer
from os import sep
from SocketServer import ThreadingMixIn
from struct import pack


##
##
##
class MyDatabase:
    connection = None
    connection_filename = ""

    def __init__(self):
        self.connection = None
        self.connection_filename = ""
    
    def _error(self, e):
        print "Error %s:" % e.args[0]
        self.close()
        return None

    # static creator, so we can return None
    @staticmethod
    def open(filename):
        if not os.path.isfile(filename):
            return None
        db = MyDatabase()
        ok = db._open(filename)
        if not ok:
            return None
        return db

    def _open(self, filename):
        if self.connection_filename == filename and self.connection != None:
            return True
        self.close() # just in case
        try:
            self.connection = sqlite3.connect("file:" + filename + "?mode=ro")
        except sqlite3.Error, e:
            return self._error(e)
        self.connection_filename = filename
        return True

    def close(self):
        if self.connection:
            self.connection.close()
            self.connection = None
        self.connection_filename = ""
        
    def list_tables(self):
        resp = list()

        try:
            cursor = self.connection.cursor()
            cursor.execute('SELECT table_name from gpkg_contents')
            while True:
                row = cursor.fetchone()
                if row == None:
                    break
                resp.append(row[0])
        except sqlite3.Error, e:
            return self._error(e)

        return resp

    def get_info(self, dbname, tablename):
        resp = dict()
        resp["database"] = dbname
        resp["table"] = tablename
        resp["version"] = 4
            
        try:            
            cursor = self.connection.cursor()
            cursor.execute("SELECT min_x,min_y,max_x,max_y,description,last_change,srs_id FROM gpkg_contents WHERE data_type='pctiles' AND table_name='%s'" % tablename)
            row = cursor.fetchone()
            if not row: return None
            resp["data_bbox"] = [row[0], row[1], row[2], row[3]]
            resp["description"] = row[4]
            resp["last_change"] = row[5]
        except sqlite3.Error, e:
            return self._error(e)

        try:
            cursor = self.connection.cursor()
            cursor.execute("SELECT min_x,min_y,max_x,max_y FROM gpkg_pctile_matrix_set WHERE table_name='%s'" % tablename)
            row = cursor.fetchone()
            if not row: return None
            resp["tile_bbox"] = [row[0], row[1], row[2], row[3]]
        except sqlite3.Error, e:
            return self._error(e)

        try:
            cursor = self.connection.cursor()
            cursor.execute("SELECT matrix_width,matrix_height FROM gpkg_pctile_matrix WHERE table_name='%s' AND zoom_level=0" % tablename)
            row = cursor.fetchone()
            if not row: return None
            resp["num_cols_L0"] = row[0]
            resp["num_rows_L0"] = row[1]
        except sqlite3.Error, e:
            return self._error(e)

        resp["dimensions"] = list()

        try:
            cursor = self.connection.cursor()
            cursor.execute("SELECT ordinal_position,dimension_name,data_type,description,minimum,mean,maximum FROM gpkg_pctile_dimension_set WHERE table_name='%s'" % tablename)
            while True:
                row = cursor.fetchone()
                if row == None:
                    break
                dims = dict()
                dims["ordinal_position"] = row[0]
                dims["name"] = row[1]
                dims["datatype"] = row[2]
                dims["description"] = row[3]
                dims["minimum"] = row[4]
                dims["mean"] = row[5]
                dims["maximum"] = row[6]
                resp["dimensions"].append(dims)            
        except sqlite3.Error, e:
            return self._error(e)
            
        return resp
        
    def get_blob(self, table, level, x, y):
        resp = None
        mask = None
        numPoints = None
                
        #print("Blob query: %s,%s,%s" % (level, x, y))
                
        try:                    
            cursor = self.connection.cursor()
            sql = "SELECT tile_data,num_points,child_mask FROM %s WHERE zoom_level=%s AND tile_column=%s AND tile_row=%s" % (table, level, x, y)    
            cursor.execute(sql)
                    
            while True:
                row = cursor.fetchone()
                if row == None:
                    break
                resp = row[0]
                numPoints = row[1]
                mask = row[2]
                        
        except sqlite3.Error, e:
            return self._error(e)
                
        if resp == None:
            return None
                
        #print("done: %s,%s,%s" % (len(resp), numPoints, mask))
        return (resp, numPoints, mask)

##
##
##
class MyThread(threading.Thread):
    my_db = None
    
    def __init__(self, *args, **kwargs):
        threading.Thread.__init__(self, *args, **kwargs)
        self.my_db = None

    def _open_database(self, rootdir, dbname):        
        self.my_db = MyDatabase.open(rootdir + sep + dbname + ".gpkg")
        if not self.my_db:
            return None
        return True

    def get_databases(self, rootdir):
        fullnames = glob.glob(rootdir + sep + "*" + ".gpkg")

        # return just the basename of the file
        basenames = [os.path.basename(f) for f in fullnames]
        names = [os.path.splitext(f)[0] for f in basenames]
        return names    
    
    def list_tables(self, rootdir, dbname):
        ok = self._open_database(rootdir, dbname)
        if not ok: return None
        return self.my_db.list_tables()

    def get_info(self, rootdir, dbname, tablename):
        ok = self._open_database(rootdir, dbname)
        if not ok: return None
        return self.my_db.get_info(dbname, tablename)

    def get_blob(self, rootdir, dbname, tablename, level, x, y):
        ok = self._open_database(rootdir, dbname)
        if not ok: return None
        return self.my_db.get_blob(tablename, level, x, y)


##
##
##
class ThreadPoolMixIn(ThreadingMixIn):
    numThreads = None

    def __init__(self, numThreads):
        self.numThreads = numThreads
        self.requests = Queue.Queue(self.numThreads)
        for n in range(self.numThreads):
            t = MyThread(target = self.process_request_thread)
            t.setDaemon(1)
            t.start()

    def serve_forever(self):
        self.requests = Queue.Queue(self.numThreads)
        for x in range(self.numThreads):
            t = MyThread(target = self.process_request_thread)
            t.setDaemon(1)
            t.start()
        while True:
            self.handle_request()
        self.server_close()

    def process_request_thread(self):
        while True:
            ThreadingMixIn.process_request_thread(self, *self.requests.get())
    
    def handle_request(self):
        try:
            request, client_address = self.get_request()
        except socket.error:
            return
        if self.verify_request(request, client_address):
            self.requests.put((request, client_address))


##
##
##
class MyServer(ThreadPoolMixIn,HTTPServer):
    def __init__(self, addr, handler, numThreads):
        ThreadPoolMixIn.__init__(self, numThreads)
        HTTPServer.__init__(self, addr, handler)


##
##
##
class MyHandler(SimpleHTTPServer.SimpleHTTPRequestHandler):

    def __init__(self, a, b, c):
        SimpleHTTPServer.SimpleHTTPRequestHandler.__init__(self, a, b, c)

    def _send404(self, mssg=None):
        self.send_error(404, mssg)
        self.send_header("Access-Control-Allow-Origin", "*")
        self.end_headers()

    def _get_databases(self, rootdir):
        me = threading.current_thread()
        files = me.get_databases(rootdir)
        if not files:
            self._send404("databases query failed")
        self.send_response(200)
        self.send_header("Content-type", "application/json")
        self.send_header("Access-Control-Allow-Origin", "*")
        self.end_headers()        
        self.wfile.write(json.dumps(files, sort_keys=True, indent=4))

    def _get_tables(self, rootdir, dbname):
        me = threading.current_thread()
        tables = me.list_tables(rootdir, dbname)
        if not tables:
            self._send404("table query failed")
            return
        self.send_response(200)
        self.send_header("Content-type", "application/json")
        self.send_header("Access-Control-Allow-Origin", "*")
        self.end_headers()
        self.wfile.write(json.dumps(tables, sort_keys=True, indent=4))

    def _get_info(self, rootdir, dbname, tablename):
        me = threading.current_thread()
        info = me.get_info(rootdir, dbname, tablename)
        if not info:
            self._send404("info query failed")
            return
        self.send_response(200)
        self.send_header("Content-type", "application/json")
        self.send_header("Access-Control-Allow-Origin", "*")
        self.end_headers()
        self.wfile.write(json.dumps(info, sort_keys=True, indent=4))

    def _get_blob(self, rootdir, dbname, tablename, level, col, row):
        me = threading.current_thread()
        results = me.get_blob(rootdir, dbname, tablename, level, col, row)
        if not results:
            # tile does not exist (and therefore has no point data)
            self.send_response(200)
            self.send_header("Content-type", "application/octet-stream")
            self.send_header("Access-Control-Allow-Origin", "*")
            self.end_headers()
            self.wfile.write(pack('<II', 0, 0))
            return

        #print("sending blob on " + threading.current_thread().name)
        (blob, numPoints, mask) = results
        self.send_response(200)
        self.send_header("Content-type", "application/octet-stream")
        self.send_header("Access-Control-Allow-Origin", "*")
        self.end_headers()
        self.wfile.write(pack('<II', numPoints, mask))
        self.wfile.write(blob)

    def do_GET(self):
        """Respond to a GET request."""
        
        parts = [i for i in self.path.split(sep) if i != '']
        
        if (len(parts) == 0):
            self._get_databases(rootdir)
            return

        if (len(parts) == 1):
            dbname = parts[0]
            self._get_tables(rootdir, dbname)
            return
            
        if (len(parts) == 2):
            dbname = parts[0]
            tablename = parts[1]
            self._get_info(rootdir, dbname, tablename)
            return

        elif (len(parts) == 5):
            dbname = parts[0]
            tablename = parts[1]
            level = parts[2]
            col = parts[3]
            row = parts[4]
            self._get_blob(rootdir, dbname, tablename, level, col, row)
            return

        self._send404("not found: %s" % self.path)


if __name__ == '__main__':

    if len(sys.argv) != 3:
        print "Usage: $ server.py portnumber rootdir"
        exit(1)

    portnumber = int(sys.argv[1])
    rootdir = sys.argv[2]
    numThreads = 10
    
    rootdir = os.path.abspath(rootdir)

    httpd = MyServer(('', portnumber), MyHandler, 10)
    print time.asctime(), "Server started on port %s" % (portnumber)
    print time.asctime(), "Serving from: %s" % (rootdir)
    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        pass
    httpd.server_close()
    print time.asctime(), "Server stopped"
