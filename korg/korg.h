/*
Wave ROM organization:

Each ROM contains 4 waves, 8bit unsigned format, with the following data:
2048 samples octave 0
2048 samples octave 1
1024 samples octave 2
1024 samples octave 3
512 samples octave 4
512 samples octave 5
512 samples octave 6
512 samples octave 7

Standard waves:
HN613256P-T70	1-4
HN613256P-T71	5-8
HN613256P-CB4	9-12
HN613256P-CB5	13-16

Expansion waves ("Version E"):
EXP-1		1-4
EXP-3		5-8
EXP-2		9-12
EXP-4		13-16

*/

int16_t kw0[] = {
#include "HN613256P_T70.w0" //	1-4
};

int16_t kw1[] = {
#include "HN613256P_T70.w1" //	1-4
};

int16_t kw2[] = {
#include "HN613256P_T70.w2" //	1-4
};

int16_t kw3[] = {
#include "HN613256P_T70.w3" //	1-4
};

int16_t kw4[] = {
#include "HN613256P_T71.w0" //	5-8
};

int16_t kw5[] = {
#include "HN613256P_T71.w1" //	5-8
};

int16_t kw6[] = {
#include "HN613256P_T71.w2" //	5-8
};

int16_t kw7[] = {
#include "HN613256P_T71.w3" //	5-8
};

int16_t kw8[] = {
#include "HN613256P_CB4.w0" //	9-12
};

int16_t kw9[] = {
#include "HN613256P_CB4.w1" //	9-12
};

int16_t kw10[] = {
#include "HN613256P_CB4.w2" //	9-12
};

int16_t kw11[] = {
#include "HN613256P_CB4.w3" //	9-12
};

int16_t kw12[] = {
#include "HN613256P_CB5.w0" //	13-16
};

int16_t kw13[] = {
#include "HN613256P_CB5.w1" //	13-16
};

int16_t kw14[] = {
#include "HN613256P_CB5.w2" //	13-16
};

int16_t kw15[] = {
#include "HN613256P_CB5.w3" //	13-16
};

#define KWAVEMAX (16)

int16_t *kwave[KWAVEMAX];
int kwave_size[KWAVEMAX];
double kwave_freq[KWAVEMAX];

void korg_init(void) {
  kwave[0] = kw0;
  kwave[1] = kw1;
  kwave[2] = kw2;
  kwave[3] = kw3;
  kwave[4] = kw4;
  kwave[5] = kw5;
  kwave[6] = kw6;
  kwave[7] = kw7;
  kwave[8] = kw8;
  kwave[9] = kw9;
  kwave[10] = kw10;
  kwave[11] = kw11;
  kwave[12] = kw12;
  kwave[13] = kw13;
  kwave[14] = kw14;
  kwave[15] = kw15;
  for (int i=0; i<KWAVEMAX; i++) {
    kwave_size[i] = 2048;
    kwave_freq[i] = 1;
  }
}
