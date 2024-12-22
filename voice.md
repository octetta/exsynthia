
```pikchr
ovalht=0
ovalwid=.75

oval "MIDI""keytar"
arrow "opto" "isolator"

oval "UART""to USB"
arrow
oval "custom""conversion"
arrow "wire""protocol"
oval "socket"
arrow
circle "exsynthia"
```

```pikchr
ovalht=0
ovalwid=.5
C0: oval "console"
down
L0: line .2 invisible

U0: oval "socket"
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


arrow dashed from C0.e to E0 chop
arrow dashed from U0.e to E0 chop
```

```pikchr
boxwid = .75
boxht = .2
boxrad = 0.05

down

ovalht=0
ovalwid=.7
W0: oval "amplitude"
line .25 invisible

A0: oval "waveform"
L0: line .25 invisible

F0: oval "frequency"
line .25 invisible

M0: circle dashed "voice[y]"
right
line .25 from A0.e invisible
arrow .25 invisible

V0: circle "voice[x]"

arrow from W0.e to V0 chop
arrow from A0.e to V0 chop
arrow from F0.e to V0 chop

arrow dashed from M0 to F0 chop
```