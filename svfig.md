
```pikchr
ovalht=0
ovalwid=0

circle "MIDI""keytar"
arrow .3 dotted
circle "opto""isolator"
arrow .3 dotted

circle "UART""to USB"
arrow "raw""MIDI"
circle "a miracle""occurs"
arrow "wire""protocol"
oval "port"
arrow dashed
circle "exsynthia"
```

```pikchr
ovalht=0
ovalwid=0
C0: oval "console"
down
L0: line .2 invisible

U0: oval "port"
right

line .6 from L0 invisible

right 
E0: circle "wire" "protocol"
arrow .3

circle "voice" "0-63"
arrow .3 dashed

circle dashed "audio" "callback"
arrow .4 dashed 

circle "speaker"
line invisible up .5 from E0.n


arrow from C0.e to E0 chop
arrow dashed from U0.e to E0 chop
```

