import socket

msgFromClient       = "Hello, World!"
bytesToSend         = str.encode(msgFromClient)
serverAddressPort   = ("127.0.0.1", 5000)
bufferSize          = 1024

UDPClientSocket = socket.socket(family=socket.AF_INET, type=socket.SOCK_DGRAM)



while True:
    UDPClientSocket.sendto(bytesToSend, serverAddressPort)

    msgFromServer = UDPClientSocket.recvfrom(bufferSize)
    msg = "Message from Server {}".format(msgFromServer[0])

    print(msg)
