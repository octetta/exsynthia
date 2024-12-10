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

the_use = 0
the_key = 69
the_vel = 0
the_mode = 0 # off 1 = on

while True:
  extra = ""
  if ser.inWaiting() > 0:
    out = ser.read(1)
    n = int.from_bytes(out, 'little')
    if n == ACTIVE_SENSE:
      c += 1
      continue
    if c > 1:
      print(f'active-sense x {c}')
      c = 0
    if state != STATE_NONE:
      if state == STATE_NOTE_KEY: # get key
        extra = f"key {n}"
        the_key = n
        state = STATE_NOTE_VEL
      elif state == STATE_NOTE_VEL: # get velocity
        extra = f"vel {n}"
        state = STATE_NONE
        the_vel = n
      elif state == STATE_CC_CONTROL: # get CC control#
        extra = f"CC control {n}"
        state = STATE_CC_VALUE
      elif state == STATE_CC_VALUE: # get CC value
        extra = f"CC value {n}"
        state = STATE_NONE
      elif state == STATE_PC_VALUE: # get PC value
        extra = f"PC value {n}"
        state = STATE_NONE
      else:
        extra = "?help?"
        state = STATE_NONE
      print(f'{n:02x}/{n:03d} {extra}')
      continue
    top4 = n >> 4
    bot4 = n & 0xf
    if top4 == NOTE_OFF: # note-off
      the_use = 1
      extra = f"NOTE OFF / ch{bot4:d}"
      state = STATE_NOTE_KEY
      the_vel = 0
    elif top4 == NOTE_ON: # note-on
      the_use = 1
      extra = f"NOTE ON / ch{bot4:d}"
      state = STATE_NOTE_KEY
    elif top4 == POLY: # poly key pressure
      extra = "PP"
    elif top4 == CCHANGE: # control change
      extra = "CC"
      state = STATE_CC_CONTROL
    elif top4 == PCHANGE: # program change
      extra = "PC"
      state = STATE_PC_VALUE
    elif top4 == MONO: # mono aftertouch
      extra = "MA"
    elif top4 == BEND: # pitchbend
      extra = "bend"
    elif top4 == SYS: # system message
      if bot4 == SYS_START:
        extra = "START"
      elif bot4 == SYS_STOP:
        extra = "STOP"
      elif bot4 == SYS_CONT:
        extra = "CONT"
      else:
        extra = "SYS?"
    else:
      extra = "????"
    print(f'{n:02x}/{n:03d} {extra} {the_key} {the_vel} {the_use}')
    if the_use == 1:
      if the_vel == 0:
        wire("v0l0v1l0")
      else:
        wire(f"v0n{the_key}l5v1n{the_key+.1}l5")
      the_use = 0
      
