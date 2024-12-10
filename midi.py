import serial
port = '/dev/tty.usbserial-A904MHYD'
port = '/dev/ttyUSB0'
ser = serial.Serial(port=port, baudrate=31250)

ACTIVE_SENSE = 254

NOTE_OFF = 0b1000_0000 >> 4
NOTE_ON  = 0b1001_0000 >> 4
POLY     = 0b1010_0000 >> 4
CCHANGE  = 0b1011_0000 >> 4
PCHANGE  = 0b1100_0000 >> 4
MONO     = 0b1101_0000 >> 4
BEND     = 0b1110_0000 >> 4
SYS      = 0b1111_0000 >> 4

SYS_START = 0b1010
SYS_CONT  = 0b1011
SYS_STOP  = 0b1100

STATE_NONE       = 0

STATE_NOTE_KEY   = 1
STATE_NOTE_VEL   = 2

STATE_CC_CONTROL = 100
STATE_CC_VALUE   = 101

STATE_PC_VALUE   = 200

STATE_POLY_KEY   = 300
STATE_POLY_PRESS = 301

STATE_BEND_LSB   = 400
STATE_BEND_MSB   = 401

c = 0
state = STATE_NONE

import socket

ip = '127.0.0.1'
port = 60440

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

def wire(msg):
  sock.sendto(bytes(msg, "utf-8"), (ip,port))

the_mode = 0 # off 1 = on

key_uses_note = {}
voice_max = 32
voice_next = 0
voice_delta = 4

while True:
    out = ser.read(1)
    n = int.from_bytes(out, 'little')
    if n == 144:
      out = ser.read(1)
      key = int.from_bytes(out, 'little')
      out = ser.read(1)
      v = int.from_bytes(out, 'little')
      vel = v/127.0 + 5
      print("on", key, vel, voice_next, voice_next+1, voice_next+2)
      key_uses_note[key] = voice_next
      wire(f"v{voice_next}n{key}l5v{voice_next+1}n{key+.1}l{vel}")
      voice_next += voice_delta
      if voice_next >= voice_max:
        voice_next = 0
    elif n == 128:
      out = ser.read(1)
      key = int.from_bytes(out, 'little')
      out = ser.read(1)
      vel = int.from_bytes(out, 'little')
      me = key_uses_note[key]
      print("off", key, vel, me, me+1, me+2)
      wire(f"v{me}l0v{me+1}l0v{me+1}l0")
      del key_uses_note[key]
