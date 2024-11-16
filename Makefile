TARGETS = \
synth \
#

all: $(TARGETS)

MYLIBS = \
linenoise.o \
audio.o \
miniwav.o

LIBS = \
-lasound \
-lm \
-lpthread \
#

miniwav.o : miniwav.c miniwav.h
	gcc -c miniwav.c

linenoise.o : linenoise.c linenoise.h
	gcc -c linenoise.c

audio.o : audio.c audio.h
	gcc -c audio.c

synth: synth.c $(MYLIBS)
	gcc -g synth.c $(MYLIBS) -o $@ $(LIBS)

clean:
	rm -f $(TARGETS) $(MYLIBS)
