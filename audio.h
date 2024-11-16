#ifndef _AUDIO_H_
#define _AUDIO_H_

#include <sys/time.h>

#define SAMPLE_RATE (44100)

void listalsa(char *what);
int exsynth_open(char *device, int sample_rate,  int buffer_len);
int exsynth_main(int *flag, void (*fn)(int16_t*,int));
void exsynth_close(void);

extern struct timeval rtns0;
extern struct timeval rtns1;

extern long long sent;
extern long int rtms;
extern long int btms;
extern long int diff;
extern int latency_hack_ms;

#endif
