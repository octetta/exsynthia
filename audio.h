#ifndef _AUDIO_H_
#define _AUDIO_H_

#include <sys/time.h>

#define SAMPLE_RATE (44100)

#define AUDIO_NO_MATCH (1000)

int audio_list(char *what, char *filter);
int audio_open(char *outdev, char *indev, int sample_rate,  int buffer_len);
int audio_start(void (*fn)(int16_t*,int));
int audio_running(void);
int audio_stop(void);
void audio_close(void);

extern struct timeval rtns0;
extern struct timeval rtns1;

extern long long sent;
extern long int rtms;
extern long int btms;
extern long int diff;
extern int latency_hack_ms;

#endif
