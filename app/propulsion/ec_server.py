#!/usr/bin/python

# https://stackoverflow.com/questions/31371166/reading-json-from-simplehttpserver-post-data/31393963#31393963

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
        self.send_header('Access-Control-Allow-Methods', 'HEAD, GET, OPTIONS, POST')
        self.send_header("Access-Control-Allow-Headers", "Content-Type")

    def do_POST(self):
        self._set_headers()

        print("received POST")
        self.data_string = self.rfile.read(int(self.headers['Content-Length']))

        cmd = "ec_cmd/ec_cmd " + self.data_string
        print(cmd)
        resp = os.system(cmd)
        if resp == 0:
            self.send_response(200)
        else:
            self.send_response(418)

        self.end_headers()


def run(server_class=HTTPServer, handler_class=S, port=80):
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
