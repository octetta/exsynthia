import serial
port = '/dev/tty.usbserial-A904MHYD'
port = '/dev/ttyUSB0'
ser = serial.Serial(port=port, baudrate=31250)

ACTIVE_SENSE = 254

NOTE_OFF = 0b1000_0000
NOTE_ON  = 0b1001_0000

SYS_START = 0xFA
SYS_CONT  = 0xFB
SYS_STOP  = 0xFC

import socket

ip = '127.0.0.1'
port = 60440

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

def wire(msg):
  print(msg)
  sock.sendto(bytes(msg, "utf-8"), (ip,port))

the_mode = 0 # off 1 = on

voice_next = 0
last_key_on = -1

active_sense = 0

while True:
    out = ser.read(1)
    n = int.from_bytes(out, 'little')
    if n == NOTE_ON:
      out = ser.read(1)
      key = int.from_bytes(out, 'little')
      out = ser.read(1)
      v = int.from_bytes(out, 'little')
      vel = v/127.0 + 5
      wire(f"v{voice_next}n{key}l{vel}")
      last_key_on = key
    elif n == NOTE_OFF:
      out = ser.read(1)
      key = int.from_bytes(out, 'little')
      out = ser.read(1)
      vel = int.from_bytes(out, 'little')
      if key == last_key_on:
        wire(f"v{voice_next}l0")
    elif n == ACTIVE_SENSE:
      active_sense += 1
    elif n == SYS_CONT:
      print("keyicon/CONT")
    elif n == SYS_STOP:
      print("minus/STOP")
    elif n == SYS_START:
      print("plus/START")
    else:
      print(n)
