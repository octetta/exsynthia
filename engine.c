#include <stdint.h>
#include <stdio.h>

///

#define VOICES (64)

enum {
    EXWAVE,  // waveform index
    EXISMOD, // voice is a modulator (no direct sound)
    EXNOTE,  // midi note number double
    EXPATCH, // user wave patch # int
    EXDETUNE, // detune # signed, float
    //
    EXLASTSAMPLE, // last wave sample
    EXINTERP,     // interpolated wave sample
    //
    //
    EXSH,  // sample and hold amount
    EXSHI, // 
    EXSHS, //
    //
    EXFREQ,       // wave frequency for human
    EXFREQBASE,   // double
    EXFREQACC,    // wave freq fixedpoint phase accumulator u64 (from DDS)
    EXFREQINC,    // wave freq fixedpoint phase inc i32 (from DDS)
    EXFREQDIV,    // i32 (from DDS)
    EXFREQSIZE,   // wave size u32 (from DDS)
    EXFREQWPTR,   // raw pointer to waveform *i16
    EXFREQONE,    // oneshot boolean
    EXFREQACTIVE, // active boolean
    EXFREQMOD,    // freq modulator voice index
    //
    // EXFREQMODAMT, // amount of frequency modulation
    //
    // EXFREQGLIS,  // glissando
    // EXFREQGLISD, // glissando delta
    // EXFREQGLIST, // glissando target
    //
    // EXFREQEG,    // eg source
    // EXFREQEGAMT, // amount of eg
    //
    EXAMP,    // amplitude for human
    EXAMPTOP, // amplitude top ratio
    EXAMPBOT, // amplitude bottom ratio
    //
    // EXAMPMOD,
    // EXAMPMODAMT,
    //
    // EXAMPEG,
    // EXAMPEGAMT,
    //
    EXPAN,     // double for human 0=left, .5=center, 1=right
    //EXPANLEFT,  // i16 calculated from EXPAN
    //EXPANRIGHT, // i16 calculated from EXPAN
    //
    // EXPANMOD,
    // EXPANMODAMT,
    //
    // EXPANEG,
    // EXPANEGAMT,
    //
    // EXFILT, // mode 0=off, 1=LPF, 2=BPF
    // EXFILTCUT,
    // EXFILTRES,

    // EXFILTSTATE0,
    // EXFILTSTATE1,
    // EXFILTSTATE2,
    // EXFILTSTATE3,
    //
    // EXFILTMODCUT,
    // EXFILTMODCUTAMT,
    //
    // EXFILTEGCUT,
    // EXFILTEGCUTAMT,
    //
    EXGATE,
    //
    EXTRIGGER,
    EXTRIGGER0,
    EXTRIGGER1,
    EXTRIGGERF0,
    EXTRIGGERF1,
    //
    EXMAXCOLS,
};

typedef int32_t q248_t;
typedef int32_t q1715_t;
typedef int32_t q1616_t;

#include <sys/time.h>

union ExVoice {
    char b;
    int i;
    uint64_t u64;
    uint32_t u32;
    int64_t i64;
    int32_t i32;
    double f;
    int16_t s;
    q248_t q248;
    q1715_t q1715_t;
    q1616_t q1616_t;
    int16_t *pi16;
    struct timeval tv;
};

union ExVoice exvoice[VOICES][EXMAXCOLS];

#define EXS_GATE(voice)       exvoice[voice][EXGATE].b
#define EXS_WAVE(voice)       exvoice[voice][EXWAVE].i
#define EXS_ISMOD(voice)      exvoice[voice][EXISMOD].b
#define EXS_FREQ(voice)       exvoice[voice][EXFREQ].f
#define EXS_FREQMOD(voice)    exvoice[voice][EXFREQMOD].i
#define EXS_LASTSAMPLE(voice) exvoice[voice][EXLASTSAMPLE].s
#define EXS_AMP(voice)        exvoice[voice][EXAMP].f
#define EXS_AMPTOP(voice)     exvoice[voice][EXAMPTOP].i
#define EXS_AMPBOT(voice)     exvoice[voice][EXAMPBOT].i
#define EXS_SH(voice)         exvoice[voice][EXSH].i
#define EXS_SHI(voice)        exvoice[voice][EXSHI].i
#define EXS_SHS(voice)        exvoice[voice][EXSHS].s
#define EXS_FREQBASE(voice)   exvoice[voice][EXFREQBASE].f
#define EXS_FREQACC(voice)    exvoice[voice][EXFREQACC].u64
#define EXS_FREQINC(voice)    exvoice[voice][EXFREQINC].i32
#define EXS_FREQDIV(voice)    exvoice[voice][EXFREQDIV].i32
#define EXS_FREQSIZE(voice)   exvoice[voice][EXFREQSIZE].u32
#define EXS_FREQWPTR(voice)   exvoice[voice][EXFREQWPTR].pi16
#define EXS_FREQONE(voice)    exvoice[voice][EXFREQONE].b
#define EXS_FREQACTIVE(voice) exvoice[voice][EXFREQACTIVE].b
#define EXS_INTERP(voice)     exvoice[voice][EXINTERP].b
#define EXS_NOTE(voice)       exvoice[voice][EXNOTE].f
#define EXS_PATCH(voice)      exvoice[voice][EXPATCH].i
#define EXS_PAN(voice)        exvoice[voice][EXPAN].f
//
#define EXS_TRIGGER(voice)    exvoice[voice][EXTRIGGER].b
#define EXS_TRIGGER0(voice)    exvoice[voice][EXTRIGGER0].tv
#define EXS_TRIGGER1(voice)    exvoice[voice][EXTRIGGER1].tv
#define EXS_TRIGGERF0(voice)    exvoice[voice][EXTRIGGERF0].u64
#define EXS_TRIGGERF1(voice)    exvoice[voice][EXTRIGGERF1].u64

#define EXS_DETUNE(voice) exvoice[voice][EXDETUNE].f


///


// inspired by AMY :)
enum {
    EXWAVESINE,     // 0
    EXWAVESQR,      // 1
    EXWAVESAWDN,    // 2
    EXWAVESAWUP,    // 3
    EXWAVETRI,      // 4
    EXWAVENOISE,    // 5
    EXWAVEUSR0,     // 6
    EXWAVEPCM,      // 7
    EXWAVEUSR1,     // 8
    EXWAVEUSR2,     // 9
    EXWAVEUSR3,     // 10
    EXWAVENONE,     // 11
    
    EXWAVEUSR4,     // 12
    EXWAVEUSR5,     // 13
    EXWAVEUSR6,     // 14
    EXWAVEUSR7,     // 15
    
    EXWAVEKRG1,     // 16
    EXWAVEKRG2,     // 17
    EXWAVEKRG3,     // 18
    EXWAVEKRG4,     // 19
    EXWAVEKRG5,     // 10
    EXWAVEKRG6,     // 21
    EXWAVEKRG7,     // 22
    EXWAVEKRG8,     // 23
    EXWAVEKRG9,     // 24
    EXWAVEKRG10,    // 25
    EXWAVEKRG11,    // 26
    EXWAVEKRG12,    // 27
    EXWAVEKRG13,    // 28
    EXWAVEKRG14,    // 29
    EXWAVEKRG15,    // 30
    EXWAVEKRG16,    // 31

    EXWAVEKRG17,     // 32
    EXWAVEKRG18,     // 33
    EXWAVEKRG19,     // 34
    EXWAVEKRG20,     // 35
    EXWAVEKRG21,     // 36
    EXWAVEKRG22,     // 37
    EXWAVEKRG23,     // 38
    EXWAVEKRG24,     // 39
    EXWAVEKRG25,     // 40
    EXWAVEKRG26,    // 41
    EXWAVEKRG27,    // 42
    EXWAVEKRG28,    // 43
    EXWAVEKRG29,    // 44
    EXWAVEKRG30,    // 45
    EXWAVEKRG31,    // 46
    EXWAVEKRG32,    // 47

    EXWAVMAX
};

///

#include "audio.h"

void engine(int16_t *playback, int16_t *capture, int frame_count) {
    int32_t b = 0;
    // TODO process EGs here
    int16_t *outgoing = playback;
    int16_t *incoming = capture;
    for (int n = 0; n < frame_count; n++) {
        outgoing[n] = 0;
        outgoing[n+1] = 0;
        int32_t incoming_avg = (incoming[n] + incoming[n+1]) / 2;
        for (int i=0; i<VOICES; i++) {
            EXS_LASTSAMPLE(i) = 0;
            if (EXS_WAVE(i) == EXWAVENONE) continue;
            if (EXS_AMP(i) == 0.0) continue;
            if (EXS_AMPTOP(i) == 0 || EXS_AMPBOT(i) == 0) continue;
            if (EXS_TRIGGER(i)) {
              EXS_TRIGGERF1(i) = frames_sent;
              gettimeofday(&EXS_TRIGGER1(i), NULL);
              EXS_TRIGGER(i) = 0;
            }
            if (capture && EXS_WAVE(i) == EXWAVEUSR3) {
              b = incoming_avg;
            } else {
              b = wave_next(i);
            }
            b = b * EXS_AMPTOP(i) / EXS_AMPBOT(i);
            if (EXS_FREQMOD(i) >= 0) {
                wave_freq(i, EXS_FREQ(i) + (double)EXS_LASTSAMPLE(EXS_FREQMOD(i)));
            }
            // TODO... the eg next should have happened earlier. this should apply that value if enabled
            if (EXS_GATE(i)) {
                int32_t envelope_value = env_next(i);
                //int32_t sample = (b * envelope_value) >> ENV_FRAC_BITS;
                int32_t sample = (b * envelope_value);
                b = sample;
            }
            if (EXS_SH(i)) {
                if (EXS_SHI(i) > EXS_SH(i)) {
                    // get next sample
                    EXS_SHI(i) = 0;
                    EXS_SHS(i) = b;
                }
                EXS_SHI(i)++;
                b = EXS_SHS(i);
            }
            EXS_LASTSAMPLE(i) = b;
            if (!EXS_ISMOD(i)) {
              *outgoing += b;
              *(outgoing+1) += b;
            }
        }
        outgoing++;
        outgoing++;
        incoming++;
        incoming++;
    }
}