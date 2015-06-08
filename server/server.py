import sys
import time
import BaseHTTPServer
import sqlite3
import os.path
import glob

HOST_NAME = 'localhost'
PORT_NUMBER = 1234

connection = None
currentDb = None


def getFiles():
    return glob.glob("*.gpkg")
    
def exists(dbname):
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
    resp = list()
    
    try:
        con = sqlite3.connect(dbname + ".gpkg")
        
        cur = con.cursor()    
        cur.execute("SELECT min_x,min_y,max_x,max_y FROM gpkg_contents WHERE table_name='%s'" % tablename)
        while True:
            row = cur.fetchone()
            if row == None:
                break
            resp.append(row[0])
            resp.append(row[1])
            resp.append(row[2])
            resp.append(row[3])
        
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


class MyHandler(BaseHTTPServer.BaseHTTPRequestHandler):
    def do_GET(s):
        """Respond to a GET request."""
        
        if (s.path == "/favicon.ico"):
            s.send_response(404)
            s.send_header("Content-type", "text/html")
            s.end_headers()
            s.wfile.write("Not found.")
            return
            
        if (s.path == "/"):
            s.send_response(200)
            s.send_header("Content-type", "text/html")
            s.end_headers()
            s.wfile.write("list all DBs!")
            f = getFiles()
            s.wfile.write(f)    
            return

        parts = s.path.split("/")[1:]
        
        if (len(parts) == 1):
            db = parts[0]
            if not exists(db):
                s.send_response(404)
                s.send_header("Content-type", "text/html")
                s.end_headers()
                s.wfile.write("Database not found")
                return
            
            tables = listTables(db)
            if (tables == None):
                s.send_response(404)
                s.send_header("Content-type", "text/html")
                s.end_headers()
                s.wfile.write("Database query failed")
                return
            
            s.send_response(200)
            s.send_header("Content-type", "text/html")
            s.end_headers()
            s.wfile.write("list tables of DB %s" % db)
            s.wfile.write(tables)
            return
            
        if (len(parts) == 2):
            db = parts[0]
            table = parts[1]

            if not exists(db):
                s.send_response(404)
                s.send_header("Content-type", "text/html")
                s.end_headers()
                s.wfile.write("Database not found")
                return

            info = getInfo(db, table)
            if (info == None):
                s.send_response(404)
                s.send_header("Content-type", "text/html")
                s.end_headers()
                s.wfile.write("Database query failed")
                return

            s.send_response(200)
            s.send_header("Content-type", "text/html")
            s.end_headers()
            s.wfile.write("info on table %s of DB %s" % (db, table))
            s.wfile.write(info)
            return

        elif (len(parts) == 5):
            db = parts[0]
            table = parts[1]
            level = parts[2]
            x = parts[3]
            y = parts[4]

            if not exists(db):
                s.send_response(404)
                s.send_header("Content-type", "text/html")
                s.end_headers()
                s.wfile.write("Database not found")
                return

            blob = getBlob(db, table, level, x, y)
            if (blob == None):
                s.send_response(404)
                s.send_header("Content-type", "text/html")
                s.end_headers()
                s.wfile.write("Database query failed")
                return

            s.send_response(200)
            s.send_header("Content-type", "application/octet-stream")
            s.end_headers()
            s.wfile.write(blob)
            return

        s.send_response(404)
        s.send_header("Content-type", "text/html")
        s.end_headers()
        s.wfile.write("Not found: %s" % self.path)


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
