# Sends lines from stdin to InfluxDB database over UDP line protocol
# Sends in the 'logs' series

import socket
import sys

ADDR = "influx.local"
PORT = 8089

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

head = "logs log="

for line in sys.stdin:
    data = head + "\"" + line.rstrip('\n') + "\""

    sock.sendto(bytes(data, "utf-8"), (ADDR, PORT))
