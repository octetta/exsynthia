

```pikchr
boxwid = .75
boxht = .2
boxrad = 0.05

down

ovalht=0
ovalwid=0
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

left
line 1.2 from A0.e invisible
P0: circle "wire""protocol"

arrow from W0.e to V0 chop
arrow from A0.e to V0 chop
arrow from F0.e to V0 chop

arrow from P0 to W0.w chop
arrow from P0 to A0.w chop
arrow from P0 to F0.w chop

arrow dashed from M0 to F0 chop
```
