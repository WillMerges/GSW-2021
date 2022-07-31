import socket

ip = "localhost"
port = 5000
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind((ip, port))

while True:
    data, addr = sock.recvfrom(1024)
    print("received message: %s" % data)
