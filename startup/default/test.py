import socket
import struct

sd = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, 0)

b = struct.pack("!i", 6900)
sd.sendto(b, ("127.0.0.1", 8081))
