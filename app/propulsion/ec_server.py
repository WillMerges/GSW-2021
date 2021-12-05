#!/usr/bin/python
#
# File:     ec_server.py
#
# Purpose:  listens for HTTP PUSH requests with text/plain content.
#           Content is plaintext arguments to execute "ec_cmd/ec_cmd" with.
#           If process returns an error sends an HTTP 418 (I'm a teapot),
#           otherwise a 200 (OK). Will also respond to OPTIONS requests, but
#           no other HTTP functions are supported.
#
#
# Usage:    ./ec_server.py <port>
#           'port' is optional port to listen to HTTP requests over,
#           uses 8080 by default
#
# Base code taken from:
#   https://stackoverflow.com/questions/31371166/reading-json-from-simplehttpserver-post-data/31393963#31393963
#
from BaseHTTPServer import BaseHTTPRequestHandler, HTTPServer
import SocketServer
import os

class S(BaseHTTPRequestHandler):
    def _set_headers(self):
        self.send_response(200)
        self.send_header('Content-type', 'text/plain')
        self.end_headers()

    def do_OPTIONS(self):
        self.send_response(200, "ok")
        self.send_header('Access-Control-Allow-Origin', '*')
        self.send_header('Access-Control-Allow-Methods', 'OPTIONS, POST')
        self.send_header("Access-Control-Allow-Headers", "Content-Type")

    def do_POST(self):
        self._set_headers()

        # we only parse plaintext
        if self.headers["Content-Type"] != "text/plain":
            self.send_response(418) # I'm a teapot
            self.end_headers()
            return

        self.data_string = self.rfile.read(int(self.headers["Content-Length"]))

        tokens = self.data_string.split()
        if len(tokens) == 2:
            cmd = "ec_cmd/ec_cmd " + self.data_string
        elif len(tokens) == 1:
            cmd = "cat ec_program/programs/" + self.data_string + " | ec_program/ec_program"
        else:
            self.send_response(418) # I'm a teapot
            self.end_headers()
            return

        resp = os.system(cmd)
        if resp == 0:
            self.send_response(200) # OK
        else:
            self.send_response(418) # I'm a teapot

        self.end_headers()


def run(server_class=HTTPServer, handler_class=S, port=8080):
    server_address = ('', port)
    httpd = server_class(server_address, handler_class)
    print("Starting httpd...")
    httpd.serve_forever()

if __name__ == "__main__":
    from sys import argv

if len(argv) == 2:
    run(port=int(argv[1]))
else:
    run()
