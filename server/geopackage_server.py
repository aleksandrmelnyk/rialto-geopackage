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

import sys
import time
import BaseHTTPServer
import sqlite3
import os.path
import glob
import json
from os import sep
from struct import pack


# we always keep the last connection open
currentConn = None
currentName = ""


def simpleName(s):
    ss = os.path.basename(s)
    sss = os.path.splitext(ss)[0]
    return sss


def getDatabases(dbpath):
    fullnames = glob.glob(dbpath + sep + "*" + ".gpkg")

    # return just the basename of the file
    l = [simpleName(i) for i in fullnames]
    return l


def databaseExists(dbname):
    return os.path.isfile(dbname + ".gpkg")


def openDatabase(dbname):
    global currentName
    global currentConn
    
    fullname = dbname + ".gpkg"
    if currentName == fullname and currentConn != None:
        return currentConn

    try:
        currentConn = sqlite3.connect(fullname)
        currentName = fullname
        #print("*** opened %s" % currentName)
    except sqlite.Error, e:        
        print "Error %s:" % e.args[0]
        if currentConn:
            currentConn.close()
            currentConn = None
            currentName = ""
        return None
        
    return currentConn


def listTables(dbname):
    global currentConn

    con = None
    resp = list()
    
    try:
        con = openDatabase(dbname)
        
        cur = con.cursor()    
        cur.execute('SELECT table_name from gpkg_contents')
        while True:
            row = cur.fetchone()
            if row == None:
                break
            resp.append(row[0])
        
    except sqlite3.Error, e:
        
        print "Error %s:" % e.args[0]
        if con: con.close()
        currentConn = None
        return None
    
    return resp


def getInfo(dbname, tablename):
    global currentConn

    con = None
    resp = dict()
    
    try:
        con = openDatabase(dbname)
        
        resp["database"] = simpleName(dbname)
        resp["table"] = tablename
        resp["version"] = 4

        cur = con.cursor()
        cur.execute("SELECT min_x,min_y,max_x,max_y,description,last_change,srs_id FROM gpkg_contents WHERE data_type='pctiles' AND table_name='%s'" % tablename)
        while True:
            row = cur.fetchone()
            if row == None:
                break
            resp["databbox"] = [row[0], row[1], row[2], row[3]]
            resp["description"] = row[4]
            resp["last_change"] = row[5]
            resp["srs_id"] = row[6]
        
        cur = con.cursor()
        cur.execute("SELECT min_x,min_y,max_x,max_y FROM gpkg_pctile_matrix_set WHERE table_name='%s'" % tablename)
        while True:
            row = cur.fetchone()
            if row == None:
                break
            resp["tilebbox"] = [row[0], row[1], row[2], row[3]]
        
        cur = con.cursor()
        cur.execute("SELECT matrix_width,matrix_height FROM gpkg_pctile_matrix WHERE table_name='%s' AND zoom_level=0" % tablename)
        while True:
            row = cur.fetchone()
            if row == None:
                break
            resp["numTilesX"] = row[0]
            resp["numTilesY"] = row[1]

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
            dims["min"] = row[4]
            dims["mean"] = row[5]
            dims["max"] = row[6]
            resp["dimensions"].append(dims)

        # TODO: metadata
        
    except sqlite3.Error, e:
        
        print "Error %s:" % e.args[0]
        if con: con.close()
        currentConn = None
        return None
        
    return resp


def getBlob(dbname, table, level, x, y):
    global currentConn

    con = None
    resp = None
    mask = None
    numPoints = None
    
    #print("Blob query: %s,%s,%s" % (level, x, y))
    
    try:
        con = openDatabase(dbname)
        
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
        if con: con.close()
        currentConn = None
        return None
    
    if resp == None: return None
    
    #print("done: %s,%s,%s" % (len(resp), numPoints, mask))
    return (resp, numPoints, mask)
    

def send404(s, mssg=None):
    s.send_error(404, mssg)
    s.send_header("Access-Control-Allow-Origin", "*")
    s.end_headers()
    return


def do_GET_databases(s, dbpath):
    s.send_response(200)
    s.send_header("Content-type", "application/json")
    s.send_header("Access-Control-Allow-Origin", "*")
    s.end_headers()
    files = getDatabases(dbpath)
    if (files == None):
        send404(s, "database query failed")
        return
    s.wfile.write(json.dumps(files, sort_keys=True, indent=4))
    return


def do_GET_tables(s, dbname):
    if not databaseExists(dbname):
        send404(s, "database not found")
        return

    tables = listTables(dbname)
    if (tables == None):
        send404(s, "table query failed")
        return

    s.send_response(200)
    s.send_header("Content-type", "application/json")
    s.send_header("Access-Control-Allow-Origin", "*")
    s.end_headers()
    s.wfile.write(json.dumps(tables, sort_keys=True, indent=4))
    return
    

def do_GET_info(s, dbname, tablename):

    if not databaseExists(dbname):
        send404(s, "database not found")
        return

    info = getInfo(dbname, tablename)
    if (info == None):
        send404(s, "info query failed")
        return

    s.send_response(200)
    s.send_header("Content-type", "application/json")
    s.send_header("Access-Control-Allow-Origin", "*")
    s.end_headers()
    s.wfile.write(json.dumps(info, sort_keys=True, indent=4))
    return


def do_GET_blob(s, dbname, tablename, level, col, row):

    #if not databaseExists(dbname):
    #    send404(s, "database not found")
    #    return

    t = getBlob(dbname, tablename, level, col, row)
    if (t == None):
        # tile does not exist (and therefore has no point data)
        s.send_response(200)
        s.send_header("Content-type", "application/octet-stream")
        s.send_header("Access-Control-Allow-Origin", "*")
        s.end_headers()
        s.wfile.write(pack('<II', 0, 0))
        return

    (blob, numPoints, mask) = t
    s.send_response(200)
    s.send_header("Content-type", "application/octet-stream")
    s.send_header("Access-Control-Allow-Origin", "*")
    s.end_headers()
    s.wfile.write(pack('<II', numPoints, mask))
    s.wfile.write(blob)
    return


class MyHandler(BaseHTTPServer.BaseHTTPRequestHandler):
    def do_GET(s):
        """Respond to a GET request."""
        
        if (s.path == "/favicon.ico"):
            send404(s, "file not found")
            return
            
        parts = [i for i in s.path.split(sep) if i != '']
        
        if (len(parts) == 0):
            dbpath = rootdir
            do_GET_databases(s, dbpath)
            return
        
        if (len(parts) == 1):
            dbname = rootdir + sep + parts[0]
            do_GET_tables(s, dbname)
            return
            
        if (len(parts) == 2):
            dbname = rootdir + sep + parts[0]
            tablename = parts[1]
            do_GET_info(s, dbname, tablename)
            return

        elif (len(parts) == 5):
            dbname = rootdir + sep + parts[0]
            tablename = parts[1]
            level = parts[2]
            col = parts[3]
            row = parts[4]
            do_GET_blob(s, dbname, tablename, level, col, row)
            return

        send404(s, "not found: %s" % s.path)
        return


if __name__ == '__main__':

    if len(sys.argv) != 4:
        print "Usage: $ server.py hostname portnumber rootdir"
        exit(1)
    
    hostname = sys.argv[1]
    portnumber = int(sys.argv[2])
    rootdir = sys.argv[3]
    
    rootdir = os.path.abspath(rootdir)

    server_class = BaseHTTPServer.HTTPServer
    httpd = server_class((hostname, portnumber), MyHandler)
    print time.asctime(), "Server started: %s:%s" % (hostname, portnumber)
    print time.asctime(), "Serving from: %s" % (rootdir)
    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        pass
    httpd.server_close()
    print time.asctime(), "Server stopped: %s:%s" % (hostname, portnumber)
