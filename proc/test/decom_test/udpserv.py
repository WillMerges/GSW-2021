#!/usr/bin/python3

# trys to send a UDP message to udpclient.py using the OS options the decom proc uses

import socket
import time

ip = "127.0.0.1"
port = 8080

listeningAddress = (ip, port)

datagramSocket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
datagramSocket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
datagramSocket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEPORT, 1)
datagramSocket.bind(listeningAddress)

# using REUSEPORT and REUSEADDR is for simulation mostly, and it seems kinda wack
# have to start things in a weird order

data = "abcd"
while(True):
    datagramSocket.sendto(data.encode(), ("127.0.0.1", 8080))
    print("sent")
    time.sleep(3)
