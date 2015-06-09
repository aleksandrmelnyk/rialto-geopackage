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

import sys
import time
import BaseHTTPServer
import sqlite3
import os.path
import glob
import json


HOST_NAME = 'localhost'
PORT_NUMBER = 1234

connection = None
currentDb = None


def getFiles():
    return glob.glob("*.gpkg")


def fileExists(dbname):
    return os.path.isfile(dbname + ".gpkg")


def listTables(dbname):
    con = None
    resp = list()
    
    try:
        con = sqlite3.connect(dbname + ".gpkg")
        
        cur = con.cursor()    
        cur.execute('SELECT table_name from gpkg_contents')
        while True:
            row = cur.fetchone()
            if row == None:
                break
            resp.append(row[0])
        
    except sqlite3.Error, e:
        
        print "Error %s:" % e.args[0]
        return None
        
    finally:
        
        if con:
            con.close()
    
    return resp


def getInfo(dbname, tablename):
    con = None
    resp = dict()
    
    try:
        con = sqlite3.connect(dbname + ".gpkg")
        
        resp["database"] = dbname
        resp["table"] = tablename

        cur = con.cursor()
        cur.execute("SELECT min_x,min_y,max_x,max_y,description,last_change,srs_id FROM gpkg_contents WHERE data_type='pctiles' AND table_name='%s'" % tablename)
        while True:
            row = cur.fetchone()
            if row == None:
                break
            resp["data_min_x"] = row[0]
            resp["data_min_y"] = row[1]
            resp["data_max_x"] = row[2]
            resp["data_max_y"] = row[3]
            resp["description"] = row[4]
            resp["last_change"] = row[5]
            resp["srs_id"] = row[6]
        
        cur = con.cursor()
        cur.execute("SELECT min_x,min_y,max_x,max_y FROM gpkg_pctile_matrix_set WHERE table_name='%s'" % tablename)
        while True:
            row = cur.fetchone()
            if row == None:
                break
            resp["tms_min_x"] = row[0]
            resp["tms_min_y"] = row[1]
            resp["tms_max_x"] = row[2]
            resp["tms_max_y"] = row[3]
        
        cur = con.cursor()
        cur.execute("SELECT matrix_width,matrix_height FROM gpkg_pctile_matrix WHERE table_name='%s' AND zoom_level=0" % tablename)
        while True:
            row = cur.fetchone()
            if row == None:
                break
            resp["matrix_width_at_L0"] = row[0]
            resp["matrix_height_at_L0"] = row[1]

        resp["dimensions"] = list()
        
        cur = con.cursor()
        cur.execute("SELECT ordinal_position,dimension_name,data_type,description,minimum,mean,maximum FROM gpkg_pctile_dimension_set WHERE table_name='%s'" % tablename)
        while True:
            row = cur.fetchone()
            if row == None:
                break
            dims = dict()
            dims["ordinal_position"] = row[0]
            dims["dimension_name"] = row[1]
            dims["data_type"] = row[2]
            dims["description"] = row[3]
            dims["minimum"] = row[4]
            dims["mean"] = row[5]
            dims["maximum"] = row[6]
            resp["dimensions"].append(dims)

    except sqlite3.Error, e:
        
        print "Error %s:" % e.args[0]
        return None
        
    finally:
        
        if con:
            con.close()
    
    return resp


def getBlob(dbname, table, level, x, y):
    con = None
    resp = list()
    
    try:
        con = sqlite3.connect(dbname + ".gpkg")
        
        cur = con.cursor()
        sql = "SELECT tile_data FROM %s WHERE zoom_level=%s AND tile_column=%s AND tile_row=%s" % (table, level, x, y)    
        cur.execute(sql)
        
        while True:
            row = cur.fetchone()
            if row == None:
                break
            resp.append(row[0])
        
    except sqlite3.Error, e:
        
        print "Error %s:" % e.args[0]
        return None
        
    finally:
        
        if con:
            con.close()
    
    return resp
    

def send404(s, mssg=None):
    s.send_response(404)
    s.send_header("Content-type", "text/html")
    s.end_headers()
    if mssg is not None:
        s.wfile.write(mssg)
    return


def do_GET_databases(s):
    s.send_response(200)
    s.send_header("Content-type", "application/json")
    s.end_headers()
    files = getFiles()
    if (tables == None):
        send404("database query failed")
        return
    s.wfile.write(json.dumps(files))
    return


def do_GET_tables(s, parts):
    db = parts[0]
    if not fileExists(db):
        send404(s, "database not found")
        return

    tables = listTables(db)
    if (tables == None):
        send404(s, "table query failed")
        return

    s.send_response(200)
    s.send_header("Content-type", "application/json")
    s.end_headers()
    s.wfile.write(json.dumps(tables))
    return
    

def do_GET_info(s, parts):
    db = parts[0]
    table = parts[1]

    if not fileExists(db):
        send404(s, "database not found")
        return

    info = getInfo(db, table)
    if (info == None):
        send404(s, "info query failed")
        return

    s.send_response(200)
    s.send_header("Content-type", "application/json")
    s.end_headers()
    s.wfile.write(json.dumps(info))
    return


def do_GET_blob(s, parts):
    db = parts[0]
    table = parts[1]
    level = parts[2]
    x = parts[3]
    y = parts[4]

    if not exists(db):
        send404(s, "database not found")
        return

    blob = getBlob(db, table, level, x, y)
    if (blob == None):
        send404(s, "blob query failed")
        return

    s.send_response(200)
    s.send_header("Content-type", "application/octet-stream")
    s.end_headers()
    s.wfile.write(blob)
    return


class MyHandler(BaseHTTPServer.BaseHTTPRequestHandler):
    def do_GET(s):
        """Respond to a GET request."""
        
        if (s.path == "/favicon.ico"):
            send404(s, "file not found")
            return
            
        if (s.path == "/"):
            do_GET_databases(s)
            return

        parts = s.path.split("/")[1:]
        
        if (len(parts) == 1):
            do_GET_tables(s, parts)
            return
            
        if (len(parts) == 2):
            do_GET_info(s, parts)
            return

        elif (len(parts) == 5):
            do_GET_blob(s, parts)
            return

        send404("not found: %s" % self.path)
        return


if __name__ == '__main__':

    server_class = BaseHTTPServer.HTTPServer
    httpd = server_class((HOST_NAME, PORT_NUMBER), MyHandler)
    print time.asctime(), "Server Starts - %s:%s" % (HOST_NAME, PORT_NUMBER)
    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        pass
    httpd.server_close()
    print time.asctime(), "Server Stops - %s:%s" % (HOST_NAME, PORT_NUMBER)
