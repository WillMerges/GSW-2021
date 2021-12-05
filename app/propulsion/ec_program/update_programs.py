#!/usr/bin/python3
# Update InfluxDB with program names in the 'programs' directory
import socket
import os

# TODO make these configurable
ip = "localhost"
port = 8089

influxaddr = (ip, port)

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

for file in os.listdir("programs"):
    if os.path.isfile(os.path.abspath("programs") + "/" + file):
        data = "ec_programs program=\"" + file + "\""
        sock.sendto(data.encode(), influxaddr)
