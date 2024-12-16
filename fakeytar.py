import serial

port = '/dev/tty.usbserial-A904MHYD'
port = '/dev/ttyUSB0'

# for this to work with the pts example, i ran socat like this
# socat -d -d pty,rawer,echo=0 pty,rawer,echo=0
# which shows two pseudo-tty-s that it's camping on
# i then ran ./midi.exs with one of those pty's
# and set port here to the other

port = '/dev/pts/5'

ser = serial.Serial(port=port, baudrate=31250)

def keyon(note,vel):
  global ser
  a = [144,note,vel]
  ser.write(bytes(a))

def keyoff(note,vel):
  global ser
  a = [128,note,vel]
  ser.write(bytes(a))

'''
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
voice_map = {}
voice_start = 40
voice_max = 48
voice_next = voice_start
voice_delta = 4

active_sense = 0

wire("[34")

while True:
    out = ser.read(1)
    n = int.from_bytes(out, 'little')
    if n == 144:
      out = ser.read(1)
      key = int.from_bytes(out, 'little')
      out = ser.read(1)
      v = int.from_bytes(out, 'little')
      vel = v/127.0 + 5
      print(" on", key, v, voice_next, voice_next+1, voice_next+2)
      key_uses_note[key] = voice_next
      voice_map[voice_next] = key
      wire(f"v{voice_next}n{key}l5v{voice_next+1}n{key+.1}l{vel}v{voice_next+2}n{key/2}l{vel}")
      voice_next += voice_delta
      if voice_next >= voice_max:
        voice_next = voice_start
    elif n == 128:
      out = ser.read(1)
      key = int.from_bytes(out, 'little')
      out = ser.read(1)
      vel = int.from_bytes(out, 'little')
      voff = key_uses_note.get(key, -1)
      if voff != -1:
        print("off", key, vel, voff, voff+1, voff+2)
        wire(f"v{voff}l0v{voff+1}l0v{voff+1}l0v{voff+2}l0")
        del key_uses_note[key]
    elif n == 254:
      active_sense += 1
    elif n == 251:
      print("keyicon")
    elif n == 252:
      print("minus")
    elif n == 250:
      print("plus")
    else:
      print(n)
'''

