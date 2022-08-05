#!/usr/bin/python3

import socket
import time

ip = "localhost"
port = 8089

servaddr = (ip, port)

datagramSocket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

data = "test,device=rocket alt=100,lat=74,long=78"

datagramSocket.sendto(data.encode(), servaddr)
