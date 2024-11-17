TARGETS = \
exsynthia \
#

all: $(TARGETS)

MYLIBS = \
linenoise.o \
audio.o \
miniwav.o \
minietf.o \
#

LIBS = \
-lm \
-lpthread \
#

ifeq ($(shell uname), Linux)
LIBS += -lasound
endif

minietf.o : minietf.c minietf.h
	gcc -c minietf.c

miniwav.o : miniwav.c miniwav.h
	gcc -c miniwav.c

linenoise.o : linenoise.c linenoise.h
	gcc -c linenoise.c

audio.o : audio.c audio.h
	gcc -c audio.c

exsynthia: exsynthia.c $(MYLIBS)
	gcc -g exsynthia.c $(MYLIBS) -o $@ $(LIBS)

clean:
	rm -f $(TARGETS) $(MYLIBS)
