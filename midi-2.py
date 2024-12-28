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
  sock.sendto(bytes(msg, "utf-8"), (ip,port))

the_mode = 0 # off 1 = on

key_uses_note = {}
voice_map = {}
voice_start = 0
voice_max = 15
voice_next = voice_start
voice_delta = 3

active_sense = 0

wire("[36")
# wire(":1")

while True:
    out = ser.read(1)
    n = int.from_bytes(out, 'little')
    if n == NOTE_ON:
      out = ser.read(1)
      key = int.from_bytes(out, 'little')
      out = ser.read(1)
      v = int.from_bytes(out, 'little')
      vel = v/127.0 + 5
      print(" on", key, v, voice_next, voice_next+1, voice_next+2)
      key_uses_note[key] = voice_next
      voice_map[voice_next] = key
      wire(f"v{voice_next}n{key}l{vel} v{voice_next+1}n{key}l{vel} v{voice_next+2}n{key}l{vel}")
      voice_next += voice_delta
      if voice_next >= voice_max:
        voice_next = voice_start
    elif n == NOTE_OFF:
      out = ser.read(1)
      key = int.from_bytes(out, 'little')
      out = ser.read(1)
      vel = int.from_bytes(out, 'little')
      voff = key_uses_note.get(key, -1)
      if voff != -1:
        print("off", key, vel, voff, voff+1, voff+2)
        wire(f"v{voff}l0 v{voff+1}l0 v{voff+2}l0")
        del key_uses_note[key]
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
