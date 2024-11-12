#ifndef _MINIWAV_H_
#define _MINIWAV_H_
#include <stdint.h>
// int amy_parse(char *, int);
// int capture_frames(void);
// int capture_to_patch(int, int, int);
// int patch_to_capture(int);
// void capture_reverse(void);
// void capture_dynexp(void);
// void capture_trim(int);
// void capture_to_ms(int ms);
// void capture_rotate_left(int n);
// void capture_rotate_right(int n);
// void capture_stop(void);
// void capture_start(int n);
// void capture_to_wav(char *name);
// void capture_to_json(char *name);
// int wav_to_capture(char *name);
// //void show_used(void);
// void show_used(int *a, int alen);
// void show_free(void);
// int guess_patch(int osc);

int mw_put(char *name, int16_t *capture, int frames);
int mw_frames(char *name);
int mw_get(char *name, int16_t *dest, int copy_max);

#endif
