import socket

ip = '127.0.0.1'
port = 60440

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

def wire(msg):
  sock.sendto(bytes(msg, "utf-8"), (ip,port))

import time

wire("[32")

while True:
  wire("v32 l15 v33 l15")
  time.sleep(.4)
