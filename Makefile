TARGETS = \
synth \
#

all: $(TARGETS)

LIBS = \
-lasound \
-lm \
-lpthread
#

EXTRA = \
linenoise.c \
miniwav.c \
audio.c \
#

synth: synth.c $(EXTRA)
	gcc -g synth.c $(EXTRA) -o $@ $(LIBS)

clean:
	rm -f $(TARGETS)
