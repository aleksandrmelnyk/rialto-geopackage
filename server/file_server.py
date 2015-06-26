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
# Just serves up local files
#

import sys
import time
import BaseHTTPServer
import sqlite3
import os.path
import glob
import json

from os import sep

mimemap = {
    ".css": 'text/css',
    ".dart": 'application/dart',
    ".html": 'text/html',
    ".js": 'application/js',
    ".json": 'application/json',
    ".woff": 'application/font-woff',
    ".woff2": 'application/font-woff',
    ".yaml": 'text/plain'
    }
    
def send404(s, mssg=None):
    s.send_response(404)
    s.send_header("Content-type", "text/html")
    s.send_header("Access-Control-Allow-Origin", "*")
    s.end_headers()
    if mssg is not None:
        s.wfile.write(mssg)
    return


class MyHandler(BaseHTTPServer.BaseHTTPRequestHandler):
    def do_GET(s):
        """Respond to a GET request."""
        
        fullname = rootdir + sep + s.path
        
        if not os.path.isfile(fullname):
            send404(s, "not found: %s" % s.path)
            return

        (root,ext) = os.path.splitext(s.path)
        if ext in mimemap:
            f = open(fullname)
            s.send_response(200)
            s.send_header('Content-type', mimemap[ext])
            s.send_header("Access-Control-Allow-Origin", "*")
            s.end_headers()
            s.wfile.write(f.read())
            f.close()
            return

        # just treat it as a binary blob
        f = open(fullname)
        s.send_response(200)
        s.send_header('Content-type', 'application/octet-stream')
        s.send_header("Access-Control-Allow-Origin", "*")
        s.end_headers()
        s.wfile.write(f.read())
        f.close()

        send404(s, "not found: %s" % s.path)
        return


if __name__ == '__main__':

    if len(sys.argv) != 3:
        print "Usage: $ server.py portnumber rootdir"
        exit(1)
    
    hostname = ''
    portnumber = int(sys.argv[1])
    rootdir = sys.argv[2]
    
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
