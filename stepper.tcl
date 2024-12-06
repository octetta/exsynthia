package require udp

proc sleep {time} {
    after $time set end 1
    vwait end
}

set sock [udp_open]
fconfigure $sock -buffering none -translation binary
fconfigure $sock -remote [list 127.0.0.1 60440]

proc wire {msg} {
    puts -nonewline $::sock $msg
}

set rate 250

while {1} {
    # puts clock
    # puts -nonewline $sock .
    wire .
    sleep $rate
}
