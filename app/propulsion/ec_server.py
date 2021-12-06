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
#   and doing multiple connections on one port
#   https://stackoverflow.com/questions/46210672/python-2-7-streaming-http-server-supporting-multiple-connections-on-one-port
#
from BaseHTTPServer import BaseHTTPRequestHandler, HTTPServer
import SocketServer
import socket
import os
import sys
import time

# how many separate requests can we handle at once
NUM_SERVERS = 5

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

        print(cmd)

        resp = os.system(cmd)
        if resp == 0:
            self.send_response(200) # OK
        else:
            self.send_response(418) # I'm a teapot

        self.end_headers()


def run(port, server_class=HTTPServer, handler_class=S):
    server_address = ('', port)

    # to enable multiple server processes
    # https://stackoverflow.com/questions/46210672/python-2-7-streaming-http-server-supporting-multiple-connections-on-one-port
    sock = socket.socket (socket.AF_INET, socket.SOCK_STREAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEPORT, 1)
    sock.bind(server_address)
    sock.listen(5)

    httpd = server_class(server_address, handler_class, False)
    httpd.socket = sock

    print("Starting http server on port " + str(port))
    httpd.serve_forever()

if __name__ == "__main__":
    if len(sys.argv) == 2:
        port = int(sys.argv[1])
    else:
        port = 8080

    # start a bunch of server processes
    # since recv can handle multiple processes asking for packets, they can
    # all create separate sockets using the same address and only one process
    # will handle each incoming packet
    i = 0
    while i < NUM_SERVERS:
        pid = os.fork()

        if pid == 0:
            # child proc
            run(port)

        # else parent proc
        i = i + 1

# sleep forever
time.sleep(9e9)
