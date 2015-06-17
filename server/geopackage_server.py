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


from BaseHTTPServer import HTTPServer
import glob
import json
import os.path
from os import sep
import Queue
import SimpleHTTPServer
from SimpleHTTPServer import SimpleHTTPRequestHandler
from SocketServer import ThreadingMixIn
import sqlite3
from struct import pack
import sys
import threading, socket
import time    
    

def simpleName(s):
    ss = os.path.basename(s)
    sss = os.path.splitext(ss)[0]
    return sss


def databaseExists(dbname):
    return os.path.isfile(dbname + ".gpkg")


##
##
##
class MyThread(threading.Thread):
    currConn = None
    currConnName = ""


    def __init__(self, *args, **kwargs):
        threading.Thread.__init__(self, *args, **kwargs)
        self.currConn = None
        self.currConnName = ""

    
    def openDatabase(self, dbname):
        
        fullname = dbname + ".gpkg"
        if self.currConnName == fullname and self.currConn != None:
            return self.currConn

        if not databaseExists(dbname):
            return None

        try:
            self.currConn = sqlite3.connect(fullname)
            self.currConnName = fullname
            #print("*** opened %s" % currentName)
        except sqlite.Error, e:        
            print "Error %s:" % e.args[0]
            if self.currConn:
                self.currConn.close()
                self.currConn = None
            self.currConnName = ""
            return None
            
        return self.currConn


    def closeDatabase(self):
        
        if self.currConn:
            self.currConn.close()
        self.currConn = None
        self.currConnName = ""


    def getDatabases(self, dbpath):
        fullnames = glob.glob(dbpath + sep + "*" + ".gpkg")

        # return just the basename of the file
        l = [simpleName(i) for i in fullnames]
        return l
    
    
    def listTables(self, dbname):
        con = None
        resp = list()
        
        try:
            con = self.openDatabase(dbname)
            
            cur = con.cursor()    
            cur.execute('SELECT table_name from gpkg_contents')
            while True:
                row = cur.fetchone()
                if row == None:
                    break
                resp.append(row[0])
            
        except sqlite3.Error, e:
            
            print "Error %s:" % e.args[0]
            self.closeDatabase()
            return None
        
        return resp


    def getInfo(self, dbname, tablename):
        con = None
        resp = dict()
        
        try:
            con = self.openDatabase(dbname)
            
            resp["database"] = simpleName(dbname)
            resp["table"] = tablename
            resp["version"] = 4

            cur = con.cursor()
            cur.execute("SELECT min_x,min_y,max_x,max_y,description,last_change,srs_id FROM gpkg_contents WHERE data_type='pctiles' AND table_name='%s'" % tablename)
            while True:
                row = cur.fetchone()
                if row == None:
                    break
                resp["data_bbox"] = [row[0], row[1], row[2], row[3]]
                resp["description"] = row[4]
                resp["last_change"] = row[5]
            
            cur = con.cursor()
            cur.execute("SELECT min_x,min_y,max_x,max_y FROM gpkg_pctile_matrix_set WHERE table_name='%s'" % tablename)
            while True:
                row = cur.fetchone()
                if row == None:
                    break
                resp["tile_bbox"] = [row[0], row[1], row[2], row[3]]
            
            cur = con.cursor()
            cur.execute("SELECT matrix_width,matrix_height FROM gpkg_pctile_matrix WHERE table_name='%s' AND zoom_level=0" % tablename)
            while True:
                row = cur.fetchone()
                if row == None:
                    break
                resp["num_cols_L0"] = row[0]
                resp["num_rows_L0"] = row[1]

            resp["dimensions"] = list()
            
            cur = con.cursor()
            cur.execute("SELECT ordinal_position,dimension_name,data_type,description,minimum,mean,maximum FROM gpkg_pctile_dimension_set WHERE table_name='%s'" % tablename)
            while True:
                row = cur.fetchone()
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

            # TODO: metadata
            
        except sqlite3.Error, e:
            
            print "Error %s:" % e.args[0]
            self.closeDatabase()
            return None
            
        return resp


    def getBlob(self, dbname, table, level, x, y):

        con = None
        resp = None
        mask = None
        numPoints = None
        
        #print("Blob query: %s,%s,%s" % (level, x, y))
        
        try:
            con = self.openDatabase(dbname)
            
            cur = con.cursor()
            sql = "SELECT tile_data,num_points,child_mask FROM %s WHERE zoom_level=%s AND tile_column=%s AND tile_row=%s" % (table, level, x, y)    
            cur.execute(sql)
            
            while True:
                row = cur.fetchone()
                if row == None:
                    break
                resp = row[0]
                numPoints = row[1]
                mask = row[2]
                
        except sqlite3.Error, e:
            
            print "Error %s:" % e.args[0]
            self.closeDatabase()
            return None
        
        if resp == None: return None
        
        #print("done: %s,%s,%s" % (len(resp), numPoints, mask))
        return (resp, numPoints, mask)


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
    def __init__(self, addr, handler):
        ThreadPoolMixIn.__init__(self, 10)
        HTTPServer.__init__(self, addr, handler)


##
##
##
class MyHandler(SimpleHTTPServer.SimpleHTTPRequestHandler):


    def __init__(self, a, b, c):
        SimpleHTTPServer.SimpleHTTPRequestHandler.__init__(self, a, b, c)

    
    def send404(self, mssg=None):
        self.send_error(404, mssg)
        self.send_header("Access-Control-Allow-Origin", "*")
        self.end_headers()


    def do_GET_databases(self, dbpath):
        me = threading.current_thread()
        
        self.send_response(200)
        self.send_header("Content-type", "application/json")
        self.send_header("Access-Control-Allow-Origin", "*")
        self.end_headers()
        files = me.getDatabases(dbpath)
        if (files == None):
            self.send404(request, "database query failed")
            return
        self.wfile.write(json.dumps(files, sort_keys=True, indent=4))


    def do_GET_tables(self, dbname):
        me = threading.current_thread()

        tables = me.listTables(dbname)
        if (tables == None):
            self.send404(request, "table query failed")
            return

        self.send_response(200)
        self.send_header("Content-type", "application/json")
        self.send_header("Access-Control-Allow-Origin", "*")
        self.end_headers()
        self.wfile.write(json.dumps(tables, sort_keys=True, indent=4))
        

    def do_GET_info(self, dbname, tablename):
        me = threading.current_thread()

        info = me.getInfo(dbname, tablename)
        if (info == None):
            self.send404(s, "info query failed")
            return

        self.send_response(200)
        self.send_header("Content-type", "application/json")
        self.send_header("Access-Control-Allow-Origin", "*")
        self.end_headers()
        self.wfile.write(json.dumps(info, sort_keys=True, indent=4))


    def do_GET_blob(self, dbname, tablename, level, col, row):
        me = threading.current_thread()

        results = me.getBlob(dbname, tablename, level, col, row)
        if (results == None):
            # tile does not exist (and therefore has no point data)
            self.send_response(200)
            self.send_header("Content-type", "application/octet-stream")
            self.send_header("Access-Control-Allow-Origin", "*")
            self.end_headers()
            self.wfile.write(pack('<II', 0, 0))
            return

        (blob, numPoints, mask) = results
        self.send_response(200)
        self.send_header("Content-type", "application/octet-stream")
        self.send_header("Access-Control-Allow-Origin", "*")
        self.end_headers()
        self.wfile.write(pack('<II', numPoints, mask))
        self.wfile.write(blob)
        #print("sending blob on " + threading.current_thread().name)


    def do_GET(self):
        """Respond to a GET request."""
        
        parts = [i for i in self.path.split(sep) if i != '']
        
        if (len(parts) == 0):
            dbpath = rootdir
            self.do_GET_databases(dbpath)
            return
        
        if (len(parts) == 1):
            dbname = rootdir + sep + parts[0]
            self.do_GET_tables(dbname)
            return
            
        if (len(parts) == 2):
            dbname = rootdir + sep + parts[0]
            tablename = parts[1]
            self.do_GET_info(dbname, tablename)
            return

        elif (len(parts) == 5):
            dbname = rootdir + sep + parts[0]
            tablename = parts[1]
            level = parts[2]
            col = parts[3]
            row = parts[4]
            self.do_GET_blob(dbname, tablename, level, col, row)
            return

        self.send404(s, "not found: %s" % s.path)


if __name__ == '__main__':

    if len(sys.argv) != 4:
        print "Usage: $ server.py hostname portnumber rootdir"
        exit(1)
    
    hostname = sys.argv[1]
    portnumber = int(sys.argv[2])
    rootdir = sys.argv[3]
    
    rootdir = os.path.abspath(rootdir)

    httpd = MyServer((hostname, portnumber), MyHandler)
    print time.asctime(), "Server started: %s:%s" % (hostname, portnumber)
    print time.asctime(), "Serving from: %s" % (rootdir)
    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        pass
    httpd.server_close()
    print time.asctime(), "Server stopped: %s:%s" % (hostname, portnumber)
