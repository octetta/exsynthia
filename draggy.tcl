#!/bin/sh
# \
exec tclsh "$0" "$@" && exit
package require Tk
package require udp

set addr 127.0.0.1
set port 60440

set sock [udp_open]
fconfigure $sock -buffering none -translation binary

proc dest {addr port} {
    # puts "-> $addr $port"
    fconfigure $::sock -remote [list $addr $port]
}

dest $addr $port

proc wire {msg} {
    puts $msg
    puts -nonewline $::sock $msg
}

set voice 36

# wire v${::voice}w0f110G1F110R7l1

set FO 0
pack [scale .fo -orient hor -variable FO] -fill x
set FM 100
pack [scale .fm -orient hor -variable FM] -fill x

canvas .cvs -width 800 -height 600
pack .cvs

set RO 0
pack [scale .ro -orient hor -variable RO] -fill x
set RM 100
pack [scale .rm -orient hor -variable RM] -fill x

# add canvas items
.cvs create text 400 300 -text "?" -tag ct0


proc doit {x y} {
    set x [expr { $x / 10.0 }]
    set y [expr { $y / 50.0 }]
    wire v${::voice}f${x}a${y}
    return "$x $y"
}

# this one is for subsequent moves
bind .cvs <B1-Motion> {
  set l [doit "%x" "%y"]
  set x [lindex $l 0]
  set y [lindex $l 1]
  .cvs itemconfig ct0 -text "v${::voice} f${x} a${y}"
}

# this one is for the initial press
bind .cvs <Button-1> {
  set l [doit "%x" "%y"]
  set x [lindex $l 0]
  set y [lindex $l 1]
  .cvs itemconfig ct0 -text "v${::voice} f${x} a${y}"
}
