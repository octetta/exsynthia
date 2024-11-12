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
#

synth: synth.c $(EXTRA)
	gcc -g synth.c $(EXTRA) -o $@ $(LIBS)

clean:
	rm -f $(TARGETS)
