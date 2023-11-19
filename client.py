#/bin/python3
import struct
import socket
import select
import sys
# this is to be run on a linux machine
IP = "192.168.100.6"
PORT = 1234
BUFFSIZE = 256

if __name__ == "__main__":
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect((IP,PORT))
    print("Connetion successful")
    inputs = [sock, sys.stdin]
    while True:
        readable, _, _ = select.select(inputs,[],[])
        closed = False
        for source in readable:
            if source == sock:
                data = source.recv(BUFFSIZE)
                if not data:
                    print("Server closed connection")
                    closed = True
                else:
                    print(data.decode())
            else:
                # source == sys.stdin
                message = sys.stdin.readline()
                chars = bytes(message, 'ascii')
                sock.send(chars)
        if closed:
            break
    sock.close()
    exit(0)