#!/usr/bin/env python

# http://effbot.org/librarybook/simplehttpserver/simplehttpserver-example-2.py

# curl  -G http://localhost:1242?http://www.intel.com/index.html

import SocketServer
import SimpleHTTPServer
import urllib
from urlparse import urlparse
import sys
import time

class Proxy(SimpleHTTPServer.SimpleHTTPRequestHandler):
    def do_GET(self):
        url = urlparse(self.path).query
        self.copyfile(urllib.urlopen(url), self.wfile)


if __name__ == '__main__':

    if len(sys.argv) != 3:
        print "Usage: $ server.py hostname portnumber"
        exit(1)
    
    hostname = sys.argv[1]
    portnumber = int(sys.argv[2])

    httpd = SocketServer.ForkingTCPServer((hostname, portnumber), Proxy)

    print time.asctime(), "Server started: %s:%s" % (hostname, portnumber)

    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        pass
    httpd.server_close()
    print time.asctime(), "Server stopped: %s:%s" % (hostname, portnumber)
