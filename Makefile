TARGETS = \
exsynthia \
mf0 \
#

all: $(TARGETS)

MYLIBS = \
linenoise.o \
audio.o \
miniwav.o \
minietf.o \
#

MYINC = korg

LIBS = \
-lm \
-lpthread \
#

ifeq ($(shell uname), Linux)
LIBS += -lasound -latomic
endif

DEBUG = -ggdb

minietf.o : minietf.c minietf.h
	gcc $(DEBUG) -c minietf.c

miniwav.o : miniwav.c miniwav.h
	gcc $(DEBUG) -c miniwav.c

linenoise.o : linenoise.c linenoise.h
	gcc $(DEBUG) -c linenoise.c

audio.o : audio.c audio.h
	gcc $(DEBUG) -c audio.c

exsynthia: exsynthia.c $(MYLIBS) korg/korg.h plot.c
	gcc $(DEBUG) -I$(MYINC) exsynthia.c $(MYLIBS) -o $@ $(LIBS) plot.c

mf0: mf0.c
	gcc mf0.c -o mf0
clean:
	rm -f $(TARGETS) $(MYLIBS)
