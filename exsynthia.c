#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>

#include "audio.h"

#include "korg.h"

#define _USE_MATH_DEFINES
#include <math.h>
#undef M_PI
#define M_PI (double)(355.0/113.0)

#include <pthread.h>

#include "minietf.h"
#include "miniwav.h"

#include <sys/time.h>

static int ms = 5;

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

#define CYCLE_1HZ (SAMPLE_RATE * 2)
#define BUFFER_SIZE (512)  // Number of samples per ALSA period

#define VOICES (52)

#define PWAVEMAX (12)

void make_sine(int16_t *table, int size);
void make_cosine(int16_t *table, int size);
void make_sqr(int16_t *table, int size);
void make_tri(int16_t *table, int size);
void make_sawup(int16_t *table, int size);
void make_sawdown(int16_t *table, int size);
void make_noise(int16_t *table, int size);
void make_none(int16_t *table, int size);

int16_t *pwave[PWAVEMAX];
char *pwave_name[PWAVEMAX];
int pwave_size[PWAVEMAX];
double pwave_freq[PWAVEMAX];

int16_t pwave_sin[CYCLE_1HZ];
int16_t pwave_sqr[CYCLE_1HZ];
int16_t pwave_sawd[CYCLE_1HZ];
int16_t pwave_sawu[CYCLE_1HZ];
int16_t pwave_tri[CYCLE_1HZ];
int16_t pwave_noise[CYCLE_1HZ * 256];
int16_t pwave_none[1];
int16_t pwave_cos[CYCLE_1HZ];

#define PWAVE_SIZE(x) (sizeof(x)/sizeof(int16_t))

void pwave_init(void) {
    make_none(pwave_none, sizeof(pwave_none)/sizeof(int16_t));
    for (int i=0; i<PWAVEMAX; i++) {
      pwave[i] = pwave_none;
      pwave_size[i] = PWAVE_SIZE(pwave_none);
      pwave_name[i] = "none";
    }
    pwave_name[EXWAVEPCM] = "pcm";
    pwave_name[EXWAVEUSR3] = "capture";

    make_cosine(pwave_cos, CYCLE_1HZ);

    pwave[EXWAVESINE] = pwave_sin;
    pwave_size[EXWAVESINE] = PWAVE_SIZE(pwave_sin);
    make_sine(pwave_sin, pwave_size[EXWAVESINE]);
    pwave_name[EXWAVESINE] = "sine";

    pwave[EXWAVEUSR1] = pwave_sin;
    pwave_size[EXWAVEUSR1] = PWAVE_SIZE(pwave_cos);
    make_sine(pwave_sin, pwave_size[EXWAVEUSR1]);
    pwave_name[EXWAVEUSR1] = "cosine";

    pwave[EXWAVESQR] = pwave_sqr;
    pwave_size[EXWAVESQR] = PWAVE_SIZE(pwave_sqr);
    make_sqr(pwave_sqr, pwave_size[EXWAVESQR]);
    pwave_name[EXWAVESQR] = "square";

    pwave[EXWAVESAWDN] = pwave_sawd;
    pwave_size[EXWAVESAWDN] = PWAVE_SIZE(pwave_sawd);
    make_sawdown(pwave_sawd, pwave_size[EXWAVESAWDN]);
    pwave_name[EXWAVESAWDN] = "saw-down";
    
    pwave[EXWAVESAWUP] = pwave_sawu;
    pwave_size[EXWAVESAWUP] = PWAVE_SIZE(pwave_sawu);
    make_sawup(pwave_sawu, pwave_size[EXWAVESAWUP]);
    pwave_name[EXWAVESAWUP] = "saw-up";
    
    pwave[EXWAVETRI] = pwave_tri;
    pwave_size[EXWAVETRI] = PWAVE_SIZE(pwave_tri);
    make_tri(pwave_tri, pwave_size[EXWAVETRI]);
    pwave_name[EXWAVETRI] = "tri";
    
    pwave[EXWAVENOISE] = pwave_noise;
    pwave_size[EXWAVENOISE] = PWAVE_SIZE(pwave_noise);
    make_noise(pwave_noise, pwave_size[EXWAVENOISE]);
    pwave_name[EXWAVENOISE] = "noise";
}

#define USRWAVMAX (100)

int16_t *uwave[USRWAVMAX];
char uwave_name[USRWAVMAX][32];
int uwave_size[USRWAVMAX];
char uwave_one[USRWAVMAX];
double uwave_freq[USRWAVMAX];

#define MAX_VALUE (32767)
#define MIN_VALUE (-32767)

void make_sine(int16_t *table, int size) {
    for (int i = 0; i < size; i++) {
        table[i] = (int16_t)(MAX_VALUE * sinf(2.0f * M_PI * i / size));
    }
}

void make_cosine(int16_t *table, int size) {
    for (int i = 0; i < size; i++) {
        table[i] = (int16_t)(MAX_VALUE * cosf(2.0f * M_PI * i / size));
    }
}

void make_sqr(int16_t *table, int size) {
    for (int i = 0; i < size; i++) {
        if (i < (size/2)) {
            table[i] = MAX_VALUE;
        } else {
            table[i] = MIN_VALUE;
        }
    }
}

void make_tri(int16_t *table, int size) {
    int quarter = size / 4;
    for (int i = 0; i < size; i++) {
        if (i < quarter) { // 0 -> 1/4
            table[i] = (4 * MAX_VALUE * i) / size;
        } else if (i < 2 * quarter) { // 1/4 -> 1/2
            table[i] = MAX_VALUE - (4 * MAX_VALUE * (i - quarter)) / size;
        } else if (i < 3 * quarter) { // 1/2 -> 3/4
            table[i] = 0 - (4 * MAX_VALUE * (i - 2 * quarter)) / size;
        } else { // 3/4 -> 4/4
            table[i] = MIN_VALUE + (4 * MAX_VALUE * (i - 3 * quarter)) / size;
        }
    }
}

void make_sawup(int16_t *table, int size) {
    double acc = MIN_VALUE;
    double rate = (double)MAX_VALUE*2.0/(double)size*2;
    for (int i = 0; i < size/2; i++) {
        table[i] = (int16_t)acc;
        acc += rate;
    }
    acc = MIN_VALUE;
    for (int i = size/2; i < size; i++) {
        table[i] = (int16_t)acc;
        acc += rate;
    }
}

void make_sawdown(int16_t *table, int size) {
    double acc = MAX_VALUE;
    double rate = (double)MAX_VALUE*2.0/(double)size*2;
    for (int i = 0; i < size/2; i++) {
        table[i] = (int16_t)acc;
        acc -= rate;
    }
    acc = MAX_VALUE;
    for (int i = size/2; i < size; i++) {
        table[i] = (int16_t)acc;
        acc -= rate;
    }
}

void make_noise(int16_t *table, int size) {
    for (int i = 0; i < size; i++) {
        table[i] = ((double)rand()/(double)RAND_MAX - 0.5) * MAX_VALUE * 2;
    }
}

void make_none(int16_t *table, int size) {
    for (int i = 0; i < size; i++) {
        table[i] = 0;
    }
}

void signal_handler(int sig) {
    if (sig == SIGABRT) {
        puts("SIGABRT");
    } else {
        puts("something else");
        exit(EXIT_FAILURE);
    }
}

#if 1
int Note_active[VOICES];
double Env_level[VOICES];

double envelope(
    int *note_active, int gate, double *env_level, double t,
    double attack, double decay, double sustain, double release) {
    if (gate)  {
        if (t > attack + decay) return(*env_level = sustain);
        if (t > attack) return(*env_level = 1.0 - (1.0 - sustain) * (t - attack) / decay);
        return *env_level = t / attack;
    } else {
        if (t > release) {
            if (note_active) *note_active = 0;
            return *env_level = 0;
        }
        return *env_level * (1.0 - t / release);
    }
}

int32_t env_next(int v) {
  return 1;
}
  
void env_init(int v,
    uint32_t attack_ms,
    uint32_t decay_ms,
    uint32_t release_ms,
    uint32_t attack_level,
    uint32_t sustain_level) {
}

void env_on(int v) {
}

void env_off(int v) {
}

#endif

//double oft[VOICES];
//int ofg[VOICES];
//double ofgd[VOICES];

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

enum {
    EXWAVE,  // waveform index
    EXISMOD, // voice is a modulator (no direct sound)
    EXNOTE,  // midi note number double
    EXPATCH, // user wave patch # int
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
union ExVoice *exvoice_xyz[EXMAXCOLS];

// Q17.15
#define DDS_FRAC_BITS (15)
#define DDS_SCALE (1 << DDS_FRAC_BITS)

#define DDS_MAGIC (0.32874)

void wave_freq(int voice, double f) {
    if (EXS_WAVE(voice) == EXWAVEPCM) {
        EXS_FREQINC(voice) = (int32_t)((f / 440.0) * DDS_SCALE);
    } else {
        EXS_FREQINC(voice) = (int32_t)((f * EXS_FREQSIZE(voice)) / SAMPLE_RATE * DDS_SCALE);
    }
    // if (EXS_FREQBASE(voice) > 0) {
    //     // how to adjust for the real base...
    //     f = (f / EXS_FREQBASE(voice)) * DDS_MAGIC; // the magic number is magic... i found it via experimentation but need to learn what it means
    // }
    // EXS_FREQINC(voice) = (int32_t)((f * EXS_FREQSIZE(voice)) / SAMPLE_RATE * DDS_SCALE);
}

void wave_init(int voice, uint32_t size, double f, int16_t *w, int n) {
    EXS_FREQACC(voice) = 0;
    EXS_FREQSIZE(voice) = size;
    EXS_FREQDIV(voice) = SAMPLE_RATE * DDS_SCALE;
    EXS_FREQWPTR(voice) = w;
    EXS_FREQBASE(voice) = 0;
    wave_freq(voice, f);
}

int16_t wave_next(int voice) {
    if (EXS_FREQSIZE(voice) == 0) return 0;
    if (EXS_FREQACTIVE(voice) == 0) {
        EXS_FREQACC(voice) = 0;
        return 0;
    }
    uint32_t index = EXS_FREQACC(voice) >> DDS_FRAC_BITS;
    if (EXS_FREQONE(voice)) {
        if (index > EXS_FREQSIZE(voice)) {
            EXS_FREQACC(voice) = 0;
            EXS_FREQACTIVE(voice) = 0;
            return 0;
        }
    }
    int16_t sample = EXS_FREQWPTR(voice)[index % EXS_FREQSIZE(voice)];
    EXS_FREQACC(voice) += EXS_FREQINC(voice);
    return sample;
}

void wave_extra(int voice, int16_t *ptr, int len, char active, double base) {
    if (!ptr) return;
    if (!len) return;
    EXS_FREQWPTR(voice) = ptr;
    EXS_FREQSIZE(voice) = len;
    if (active) {
        EXS_FREQACTIVE(voice) = 1;
    }
    int wave = EXS_WAVE(voice);
    if (wave == EXWAVEPCM) {
        int patch = EXS_PATCH(voice);
        EXS_FREQONE(voice) = uwave_one[patch];
#if 0
        printf("# v%d w%d p%d # ptr:%p len:%d oneshot:%d active:%d base:%f\n",
            voice,
            wave,
            patch,
            uwave[patch],
            uwave_size[patch],
            uwave_one[patch],
            active,
            uwave_freq[patch]);
#endif
    } else {
      EXS_FREQONE(voice) = 0;
    }
    if (base != EXS_FREQBASE(voice)) {
        EXS_FREQBASE(voice) = base;
        wave_freq(voice, EXS_FREQ(voice));
    }
}

// LFO-ey stuff
// TODO

// int oam[VOICES]; // choose which oscillator is a amplitude modulator
// int opm[VOICES]; // choose which oscillator is a panning modulator

#include "linenoise.h"

int agcd(int a, int b) {
    while (b != 0) {
        int temp = a % b;
        a = b;
        b = temp;
    }
    return a;
}

void calc_ratio(int index) {
    int precision = 10000;
    int ip = EXS_AMP(index) * precision;
    int gcd = agcd(abs(ip), precision);
    EXS_AMPTOP(index) = ip / gcd;
    EXS_AMPBOT(index) = precision / gcd;
}

long mytol(char *str, int *valid, int *next) {
    long val;
    char *endptr;
    val = strtol(str, &endptr, 10);
    if (endptr == str) {
        if (valid) *valid = 0;
        if (next) *next = 0;
        return 0;
    }
    if (valid) *valid = 1;
    if (next) *next = endptr - str + 1;
    return val;
}

double mytod(char *str, int *valid, int *next) {
    double val;
    char *endptr;
    val = strtod(str, &endptr);
    if (endptr == str) {
        if (valid) *valid = 0;
        if (next) *next = 0;
        return 0;
    }
    if (valid) *valid = 1;
    if (next) *next = endptr - str + 1;
    return val;
}

char *mytok(char *str, char tok, int *next) {
    int n = 0;
    while (1) {
        if (str[n] == tok) {
            str[n] = '\0';
            if (next) *next = n+1;
            return str;
        }
    }
}

#ifdef __APPLE__
#define DEFAULT_DEVICE "built-in output"
#endif

#ifdef __linux__
#define DEFAULT_DEVICE "default"
#endif

char theplayback[1024] = DEFAULT_DEVICE;
char thecapture[1024] = DEFAULT_DEVICE;

static int _user_running = 1;
int user_running(void) { return _user_running; }
void user_start(void) { _user_running = 1; }
void user_stop(void) { _user_running = 0; }

static int _udp_running = 1;
int udp_running(void) { return _udp_running; }
void udp_start(void) { _udp_running = 1; }
void udp_stop(void) { _udp_running = 0; }

int wire(char *line, int *thisvoice, char *output);

int etf_fdin = -1;
int etf_fdout = -1;

#define RUNNING (user_running() && udp_running())

void *etf(void *arg) {
    int voice = 0;
    struct etf_tuple tuple;
    char output = 0;
    while (RUNNING) {
        if (etf_fdin < 0 || etf_fdout < 0) {
            // puts("NO");
            sleep(1);
            continue;
        }
        int n = etf_parse(etf_fdin, &tuple);
        if (n == etf_read_okay) {
            if (tuple.count == 0) {
                puts("{}");
            } else if (strcmp(tuple.key, "wire") == 0) {
                //int r = wire(line, &voice, &output);
                if (tuple.type == etf_list) {
                    printf("LIST[%d]\n", tuple.len);
                }
            }
        }
        sleep(1);
    }
    return NULL;
}

#define PORT 60440

#include <arpa/inet.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <ifaddrs.h>

struct sockaddr_in serve;

int udp_open(int port) {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    //global_sock = sock;
    int opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int));
    bzero(&serve, sizeof(serve));
    serve.sin_family = AF_INET;
    serve.sin_addr.s_addr = htonl(INADDR_ANY);
    serve.sin_port = htons(port);
    if (bind(sock, (struct sockaddr *)&serve, sizeof(serve)) >= 0) {
        return sock;
    }
    return -1;  
}

void *udp(void *arg) {
    int sock = udp_open(PORT);
    if (sock < 0) {
      puts("udp thread cannot run");
      return NULL;
    }
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (struct timeval *)&tv, sizeof(struct timeval));
    int voice = 0;
    struct sockaddr_in client;
    unsigned int client_len = sizeof(client);
    char line[1024];
    char output = 0;
    while (RUNNING) {
        int n = recvfrom(sock, line, sizeof(line), 0, (struct sockaddr *)&client, &client_len);
        if (n > 0) {
          line[n] = '\0';
          int r = wire(line, &voice, &output);
        } else {
          if (errno = EAGAIN) continue;
          printf("recvfrom = %d ; errno = %d\n", n, errno);
          perror("recvfrom");
        }
    }
    udp_stop();
    user_stop();
    return NULL;
}

void dump(int16_t *wave, int len) {
    int c = 0;
    char template[] = "_waveXXXXXX";
    int fd = mkstemp(template);
    if (fd < 0) {
        puts("FAIL");
        perror("mkstemp");
        return;
    }
    printf("created %s\n", template);
    char buf[80];
    for (int i=0; i<len; i++) {
        sprintf(buf, "%d\n", wave[i]);
        write(fd, buf, strlen(buf));
    }
    close(fd);
}

// Fixed point configuration
// Q17.15
#define ENV_FRAC_BITS 14
#define ENV_SCALE (1 << ENV_FRAC_BITS)
#define ENV_MAX INT32_MAX

#define AFACTOR (0.025) // scaling amplitude to match what i hear from AMY

void show_voice(char flag, int voice, char forceshow) {
    if (forceshow == 0) {
      if (EXS_AMP(voice) == 0) return;
    //   if (EXS_AMPTOP(voice) == 0) return;
      if (EXS_FREQ(voice) == 0) return;
    }
    printf("%c v%d w%d f%.4f a%.4f", flag, voice, EXS_WAVE(voice), EXS_FREQ(voice), EXS_AMP(voice) * (1.0 / AFACTOR));
    if (EXS_INTERP(voice)) printf(" Z1");
    if (EXS_ISMOD(voice)) printf(" M%d", EXS_ISMOD(voice));
    if (EXS_FREQMOD(voice) >= 0) printf(" F%d", EXS_FREQMOD(voice));
#if 0
    if (EXS_GATE(voice)) printf(" e%d B%d,%d,%d,%d,%d", EXS_GATE(voice),
        env[voice].attack_ms,
        env[voice].decay_ms,
        env[voice].release_ms,
        env[voice].attack_level,
        env[voice].sustain_level);
#endif
    if (EXS_SH(voice)) printf(" d%d", EXS_SH(voice));
    //if (ofg[voice]) printf(" G%d (%f/%f)", ofg[voice], ofgd[voice], oft[voice]);
    printf(" b%d", EXS_FREQONE(voice)==0);
    if (EXS_WAVE(voice) == EXWAVEPCM) printf(" p%d", EXS_PATCH(voice));
    printf(" #");
    printf(" Q%g", EXS_PAN(voice));
    printf(" acc:%"PRIu64" inc:%f size:%d freq:%f",
        (EXS_FREQACC(voice) >> DDS_FRAC_BITS) % EXS_FREQSIZE(voice),
        (double)EXS_FREQINC(voice)/ (double)DDS_SCALE,
        EXS_FREQSIZE(voice),
        EXS_FREQBASE(voice));
    if (1 || EXS_TRIGGER(voice) == 0) {
      struct timeval diff;
      int fdiff = EXS_TRIGGERF1(voice) - EXS_TRIGGERF0(voice);
      timersub(&EXS_TRIGGER1(voice), &EXS_TRIGGER0(voice), &diff);
      printf(" trigger:%gms, %d frames", (double)(diff.tv_usec)/1000.0, fdiff);
    }

    puts("");
}

int iswavinuse(int i) {
    int r = 0;
    for (int v=0; v<VOICES; v++) {
        if (EXS_PATCH(v) == i) {
            r = 1;
            break;
        }
    }
    return r;
}

int getwav(int i, char output) {
    if (iswavinuse(i)) {
        if (output) printf("P%d is in use, cannot free\n", i);
        return 0;
    }
    char name[64];
    sprintf(name, "%03d.wav", i);
    int frames = mw_frames(name);
    if (frames > 0) {
        if (output) printf("%s has %d frames\n", name, frames);
        if (uwave[i]) {
            if (output) printf("free W%d\n", i);
            free(uwave[i]);
        }
        int16_t *dest = malloc(frames * sizeof(int16_t));
        uwave[i] = dest;
        uwave_size[i] = frames;
        uwave_one[i] = 1;
        uwave_freq[i] = 440.0;
        int n = mw_get(name, dest, frames);
        strcpy(uwave_name[i], name);
    }
    return 0;
}


void trigger_active(char *output) {
    for (int voice=0; voice<VOICES; voice++) {
        double velocity = EXS_AMP(voice);
        double freq = EXS_FREQ(voice);
        int wave = EXS_WAVE(voice);
        int top = EXS_AMPTOP(voice);
        int bot = EXS_AMPBOT(voice);
        #if 0
        int copyvoice = voice;
        char wstr[64];

        char fstr[64];
        sprintf(fstr, "%.5f", freq);

        char *fptr = fstr;
        if (freq < 1) {
            fptr++;
        }
        
        char lstr[64];
        sprintf(lstr, "%.5f", velocity);

        char *lptr = lstr;
        if (velocity < 1) {
            lptr++;
        }

        sprintf(wstr, "v%dw%df%sl%s", voice, wave, fptr, lptr);
        if (output) printf("# -> %s\n", wstr);

        wire(wstr, &copyvoice, &output);
        #else
        if (velocity > 0.0) {
            // if (output) printf("# voice:%d wave:%d freq:%g velocity:%g top:%d bot:%d\n", voice, wave, freq, velocity, top, bot);
            EXS_FREQACC(voice) = 0;
            EXS_FREQACTIVE(voice) = 1;
            calc_ratio(voice);
            env_on(voice);
        }
        if (freq > 0.0) {
            wave_freq(voice, freq);
        }
        #endif
    } 
}

void reset_voice(int v) {
  EXS_WAVE(v) = EXWAVESINE;
  EXS_AMPTOP(v) = 0;
  EXS_AMPBOT(v) = 0;
  EXS_FREQMOD(v) = -1;
  EXS_FREQ(v) = 440;
  EXS_SH(v) = 0;
  EXS_SHI(v) = 0;
  EXS_ISMOD(v) = 0;
  EXS_AMP(v) = 0;
  calc_ratio(v);
  wave_init(v, sizeof(pwave_sin)/sizeof(int16_t), EXS_FREQ(v), pwave_sin, v);
  EXS_FREQACTIVE(v) = 0;
}

int valid_wave(int w) {
  if (w >= 0 && w < PWAVEMAX) return 1;
  if (w >= EXWAVEKRG1 && w <= EXWAVEKRG32) return 1;
  return 0;
}

int wire(char *line, int *thisvoice, char *output) {
    int p = 0;
    int valid;
    int voice = 0;
    if (thisvoice) {
        voice = *thisvoice;
    }
    while (line[p] != '\0') {
        valid = 1;
        char c = line[p++];
        int next;
        if (c == ' ' || c == '\t' || c == '\r' || c == ';') continue;
        if (c == '#') break;
        if (c == '*') {
            // nop
        } else if (c == '<') {
            char peek = line[p];
            switch (peek) {
              case '0': case '1': case '2': case '3': case '4':
              case '5': case '6': case '7': case '8': case '9':
                {
                  int ms = mytol(&line[p], &valid, &next);
                  if (!valid) goto errexit;
                  p += next-1;
                  if (*output) printf("# capture for %dms\n", ms);
                }
                break;
              case 'p':
                p++;
                // get ###.wav
                int n = mytol(&line[p], &valid, &next);
                if (!valid) goto errexit;
                p += next-1;
                getwav(n, *output);
                break;
              default:
                valid = 0;
                goto errexit;
            }
        } else if (c == ':') {
            char peek = line[p];
            switch (peek) {
                case '1':
                    p++;
                    *output = 1;
                    break;
                case 'c':
                    p++;
                    if (*output) printf("%c[2J%c[H\n", 27, 27);
                    break;
                case 'q':
                    p++;
                    puts("");
                    //user_stop();
                    //audio_stop();
                    //udp_stop();
                    return -1;
                    break;
                case 'l':
                    audio_list("pcm", "");
                    p++;
                    break;
                case 'r':
                    puts("RUN");
                    p++;
                    break;
                case 's':
                    puts("STOP");
                    p++;
                    break;
            }
        } else if (c == '~') {
            // sleep n ms
            int ms = mytol(&line[p], &valid, &next);
            if (!valid) break; else p += next-1;
            usleep(ms * 1000);
        } else if (c == '?') {
            char peek = line[p];
            if (peek == '?') {
                p++;
                for (int i=0; i<VOICES; i++) {
                    char flag = ' ';
                    if (i == voice) flag = '*';
                    if (*output) show_voice(flag, i, 0);
                }
                // printf("# rtms %ldms\n", rtms);
                // printf("# btms %ldms\n", btms);
                // printf("# diff %ldms\n", btms-rtms);
                // printf("# L%d\n", latency_hack_ms);
                if (*output) printf("# -m%d\n", ms);
                if (*output) printf("# -p%s\n", theplayback);
                if (*output) printf("# -c%s\n", thecapture);
                if (*output) printf("# frames sent %lld\n", frames_sent);
            } else {
                int i = voice;
                char flag = ' ';
                if (i == voice) flag = '*';
                if (*output) show_voice(flag, i, 1);
            }
            continue;
        } else if (c == 'Z') {
            int z = mytol(&line[p], &valid, &next);
            if (!valid) break; else p += next-1;
            EXS_INTERP(voice) = z;
        } else if (c == 'd') {
            int d = mytol(&line[p], &valid, &next);
            if (!valid) break; else p += next-1;
            EXS_SH(voice) = d;
        } else if (c == 'M') {
            int m = mytol(&line[p], &valid, &next);
            if (!valid) break; else p += next-1;
            EXS_ISMOD(voice) = m;
        //} else if (c == 'G') {
        //    int g = mytol(&line[p], &valid, &next);
        //    if (!valid) break; else p += next-1;
        //    ofg[voice] = g;
        } else if (c == 'S') {
            int v = mytol(&line[p], &valid, &next);
            if (!valid) break; else p += next-1;
            if (v >= VOICES) {
              for (int i=0; i<VOICES; i++) {
                reset_voice(i);
              }
            } else {
                reset_voice(v);
            }
        } else if (c == 'F') {
            int f = mytol(&line[p], &valid, &next);
            if (!valid) break; else p += next-1;
            if (f < VOICES) {
                EXS_FREQMOD(voice) = f;
                EXS_ISMOD(f) = 1;
            }
        } else if (c == 'B') {
            // breakpoint aka ADR ... poor copy of AMY's
            // b#,#,#
            // attack-ms, decay-ms, release-ms

            int a = mytol(&line[p], &valid, &next);
            if (!valid) break; else p += next-1;
            if (line[p] == ',') p++; else break;

            int d = mytol(&line[p], &valid, &next);
            if (!valid) break; else p += next-1;
            if (line[p] == ',') p++; else break;

            int r = mytol(&line[p], &valid, &next);
            if (!valid) break; else p += next-1;
            if (line[p] == ',') p++; else break;

            int al = mytol(&line[p], &valid, &next);
            if (!valid) break; else p += next-1;
            if (line[p] == ',') p++; else break;

            int sl = mytol(&line[p], &valid, &next);
            if (!valid) break; else p += next-1;

            // use the values
            env_init(voice,a,d,r, al, sl);
        } else if (c == 'e') {
            char peek = line[p];
            if (peek == '0') {
                p++;
                EXS_GATE(voice) = 0;
            } else if (peek == '1') {
                p++;
                EXS_GATE(voice) = 1;
            } else {
                continue;
            }
        } else if (c == 'f') {
            double f = mytod(&line[p], &valid, &next);
            if (!valid) break; else p += next-1;
            if (f >= 0.0) {
                // if (ofg[voice] > 0) {
                //     double d = f - EXS_FREQ(voice);
                //     //ofgd[voice] = d / (double)ofg[voice];
                //     //oft[voice] = f;
                //     f += d;
                //     EXS_FREQ(voice) = f;
                //     wave_freq(voice, f);
                // } else {
                //     EXS_FREQ(voice) = f;
                //     wave_freq(voice, f);
                // }
                EXS_FREQ(voice) = f;
                wave_freq(voice, f);
            }
        } else if (c == 'v') {
            int n = mytol(&line[p], &valid, &next);
            if (!valid) break; else p += next-1;
            if (n >= 0 && n < VOICES) voice = n;
        } else if (c == 'a') {
            double a = mytod(&line[p], &valid, &next);
            if (!valid) break; else p += next-1;
            a *= AFACTOR;
            if (a >= 0.0) {
                EXS_AMP(voice) = a;
                calc_ratio(voice);
            }
        } else if (c == 'b') {
            int loop = mytol(&line[p], &valid, &next);
            if (!valid) break; else p += next-1;
            EXS_FREQONE(voice) = (loop == 0);
        } else if (c == 'p') {
            int patch = mytol(&line[p], &valid, &next);
            if (!valid) break; else p += next-1;
            if (patch != EXS_PATCH(voice) && patch >= 0 && patch <= 99) {
                if (uwave[patch] && uwave_size[patch]) {
                    EXS_PATCH(voice) = patch;
                    int active = 0;
                    wave_extra(
                      voice,
                      uwave[patch],
                      uwave_size[patch], active, uwave_freq[patch]);
                }
            }
        } else if (c == 'P') {
            for (int patch=0; patch<100; patch++) {
                if (uwave[patch] && uwave_size[patch]) {
                    if (*output) printf("# p%d # %s %d one:%d\n", patch,
                        uwave_name[patch], uwave_size[patch], uwave_one[patch]);
                }
            }
        } else if (c == 'w') {
            int w = mytol(&line[p], &valid, &next);
            if (!valid) break; else p += next-1;
            if (w != EXS_WAVE(voice) && valid_wave(w)) {
                EXS_WAVE(voice) = w;
                int16_t *ptr = pwave_none;
                int len = 0;
                double base = 0;
                char forceactive = 0;
                switch (w) {
                    case EXWAVESINE:
                    case EXWAVESQR:
                    case EXWAVESAWDN:
                    case EXWAVESAWUP:
                    case EXWAVETRI:
                    case EXWAVENOISE:
                        ptr = pwave[w-EXWAVESINE];
                        len = pwave_size[w-EXWAVESINE];
                        base = 0;
                        forceactive = 1;
                        break;
                    case EXWAVEPCM: // PCM (sample)
                        ptr = uwave[EXS_PATCH(voice)];
                        len = uwave_size[EXS_PATCH(voice)];
                        base = 440.0;
                        break;
                    case EXWAVEUSR0: // KS
                    case EXWAVEUSR1: // algo
                    case EXWAVEUSR2: // part
                    case EXWAVEUSR3: // parts
                        break;
                    case EXWAVEKRG1: case EXWAVEKRG2: case EXWAVEKRG3: case EXWAVEKRG4:
                    case EXWAVEKRG5: case EXWAVEKRG6: case EXWAVEKRG7: case EXWAVEKRG8:
                    case EXWAVEKRG9: case EXWAVEKRG10: case EXWAVEKRG11: case EXWAVEKRG12:
                    case EXWAVEKRG13: case EXWAVEKRG14: case EXWAVEKRG15: case EXWAVEKRG16:
                    //
                    case EXWAVEKRG17: case EXWAVEKRG18: case EXWAVEKRG19:
                    case EXWAVEKRG20: case EXWAVEKRG21: case EXWAVEKRG22: case EXWAVEKRG23:
                    case EXWAVEKRG24: case EXWAVEKRG25: case EXWAVEKRG26: case EXWAVEKRG27:
                    case EXWAVEKRG28: case EXWAVEKRG29: case EXWAVEKRG30: case EXWAVEKRG31:
                    case EXWAVEKRG32:
                        ptr = kwave[w-EXWAVEKRG1];
                        len = kwave_size[w-EXWAVEKRG1];
                        forceactive = 1;
                        base = kwave_freq[w-EXWAVEKRG1];
                        break;
                    default:
                        puts("UNEXPECTED");
                        break;
                }
                wave_extra(voice, ptr, len, forceactive, base);
            }
        
        } else if (c == 'Q') {
            double pan = mytod(&line[p], &valid, &next);
            if (!valid) break; else p += next-1;
            EXS_PAN(voice) = pan;
        } else if (c == 'n') {
            double note = mytod(&line[p], &valid, &next);
            if (!valid) break; else p += next-1;
            if (note >= 0.0 && note <= 127.0) {
                EXS_NOTE(voice) = note;
                EXS_FREQ(voice) = 440.0 * pow(2.0, (note - 69.0) / 12.0);
                wave_freq(voice, EXS_FREQ(voice));
            }
        // } else if (c == 'L') {
        //     int n = mytol(&line[p], &valid, &next);
        //     // printf("LAT :: p:%d :: n:%d valid:%d next:%d\n", p, n, valid, next);
        //     if (!valid) break; else p += next-1;
        //     if (n > 0) {
        //         latency_hack_ms = n;
        //     }
        } else if (c == 'T') {
          timerclear(&EXS_TRIGGER0(voice));
          timerclear(&EXS_TRIGGER1(voice));
          EXS_TRIGGERF0(voice) = frames_sent;
          gettimeofday(&EXS_TRIGGER0(voice), NULL);
          EXS_TRIGGER(voice) = 1;
        } else if (c == 'W') {
            char peek = line[p];
            if (peek >= '0' && peek <= '9') {
                int n = mytol(&line[p], &valid, &next);
                switch (n) {
                    case EXWAVESINE:
                    case EXWAVESQR:
                    case EXWAVESAWDN:
                    case EXWAVESAWUP:
                    case EXWAVETRI:
                    case EXWAVEUSR1:
                    case EXWAVENOISE:
                        dump(pwave[n-EXWAVESINE], pwave_size[n-EXWAVESINE]);
                        break;
                    case EXWAVEKRG1: case EXWAVEKRG2: case EXWAVEKRG3: case EXWAVEKRG4:
                    case EXWAVEKRG5: case EXWAVEKRG6: case EXWAVEKRG7: case EXWAVEKRG8:
                    case EXWAVEKRG9: case EXWAVEKRG10: case EXWAVEKRG11: case EXWAVEKRG12:
                    case EXWAVEKRG13: case EXWAVEKRG14: case EXWAVEKRG15: case EXWAVEKRG16:
                        dump(kwave[n-EXWAVEKRG1], kwave_size[n-EXWAVEKRG1]);
                        break;
                }
            } else {
                if (*output) {
                  for (int i=0; i<PWAVEMAX; i++) {
                    printf("%d %s\n", i, pwave_name[i]);
                  }
                  for (int i=0; i<KWAVEMAX; i++) {
                    printf("%d %s\n", i+15, kwave_name[i]);
                  }
                }
            }
        } else if (c == 'L') {
            trigger_active(output);
        } else if (c == 'l') {
            double velocity = mytod(&line[p], &valid, &next);
            if (!valid) break; else p += next-1;
            velocity *= 0.025;
            if (velocity <= 0.0) {
                if (EXS_FREQONE(voice)) EXS_FREQACTIVE(voice) = 0;
                if (EXS_GATE(voice)) {
                    env_off(voice);
                } else {
                    EXS_AMP(voice) = 0.0;
                    calc_ratio(voice);
                }
            } else if (velocity > 0.0) {
                EXS_FREQACC(voice) = 0;
                EXS_AMP(voice) = velocity;
                calc_ratio(voice);
                env_on(voice);
                EXS_FREQACTIVE(voice) = 1;
            }
        } else if (c == '[') {
            int x = mytol(&line[p], &valid, &next);
            if (!valid) break; else p += next-1;
            char filename[1024];
            int localvoice = voice;
            sprintf(filename, "patch.%03d", x);
            if (*output) printf("# try to load %s\n", filename);
            FILE *file = fopen(filename, "r");
            if (file != NULL) {
                char look[1000];
                char foutput = 0;
                while (fgets(look, sizeof(look), file) != NULL) {
                    int n = strlen(look);
                    if (n > 0) {
                        look[n-1] = '\0';
                        if (*output) printf("# %s\n", look);
                        int r = wire(look, &localvoice, &foutput);
                        // printf("result = %d\n", r);
                    }
                }
                fclose(file);
            }
        } else {
            valid = 0;
            break;
        }
    }
    errexit: // if something bad happened, i hope you set valid == 0
    if (!valid) {
        if (*output) printf("trouble -> %s\n", &line[p-1]);
    }
    if (thisvoice) {
        *thisvoice = voice;
    }
  return 0;
}

#define HISTORY_FILE ".exsynthia_history"

void *user(void *arg) {
    int voice = 0;
    linenoiseHistoryLoad(HISTORY_FILE);
    usleep(5 * 100 * 1000);
    char output = 1;
    while (RUNNING) {
        char *line = linenoise("> ");
        if (line == NULL) break;
        if (strlen(line) == 0) continue;
        linenoiseHistoryAdd(line);
        int n = wire(line, &voice, &output);
        linenoiseFree(line);
        if (n < 0) break;
    }
    linenoiseHistorySave(HISTORY_FILE);
    user_stop();
    udp_stop();
    return NULL;
}

int16_t *waves[PWAVEMAX] = {
    pwave_sin,
    pwave_sqr,
    pwave_sawd,
    pwave_sawu,
    pwave_tri,
    pwave_noise,
    pwave_none,
    //
    pwave_none,
    pwave_none,
    pwave_none,
    pwave_none,
    pwave_none,
};

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


int main(int argc, char *argv[]) {
    char playbackname[1024] = DEFAULT_DEVICE;
    char capturename[1024] = DEFAULT_DEVICE;
    if (argc > 1) {
        for (int i=1; i<argc; i++) {
            if (argv[i][0] == '-') {
              switch(argv[i][1]) {
              case 'l':
                  audio_list("playback", "");
                  audio_list("capture", "");
                  return 0;
              // case 'm':
              //     audio_list("rawmidi", "");
              //     return 0;
              case 'p':
                  strcpy(playbackname, &argv[i][2]);
                  break;
              case 'c':
                  strcpy(capturename, &argv[i][2]);
                  break;
              case 'm':
                  ms = atoi(&argv[i][2]);
                  break;
              }
            }
        }
    }

    int playbackindex = 0;
    printf("# <%s>\n", playbackname);
    if (playbackname[0] >= '0' && playbackname[0] <= '9') {
        playbackindex = atoi(playbackname);
        printf("# use playback device index %d\n", playbackindex);
    } else {
        printf("# search for playback device with string \"%s\"\n", playbackname);
        playbackindex = audio_list("pcm", playbackname);
    }
    sprintf(theplayback, "%d", playbackindex);
    if (playbackindex == AUDIO_NO_MATCH) {
        printf("# no playback device <%s> found\n", playbackname);
        return 0;
    }

    int captureindex = 0;
    if (capturename[0] >= '0' && capturename[0] <= '9') {
        captureindex = atoi(capturename);
        printf("# use capture device index %d\n", captureindex);
    } else {
        printf("# search for capture device with string \"%s\"\n", capturename);
        captureindex = audio_list("pcm", capturename);
    }
    sprintf(thecapture, "%d", captureindex);
    if (captureindex == AUDIO_NO_MATCH) {
        printf("# no capture device <%s> found\n", capturename);
    }

    printf("# DDS Q%d.%d\n", 32-DDS_FRAC_BITS, DDS_FRAC_BITS);
    printf("# ENV Q%d.%d\n", 32-ENV_FRAC_BITS, ENV_FRAC_BITS);

    printf("# PCM patches %d\n", USRWAVMAX);
    for (int i=0; i<USRWAVMAX; i++) {
        uwave[i] = NULL;
        uwave_size[i] = 0;
        uwave_name[i][0] = '\0';
        uwave_one[i] = 1;
    }

    pwave_init();

    korg_init();

    printf("# voices %d\n", VOICES);
    for (int i=0; i<VOICES; i++) {
        reset_voice(i);
    }

    for (int i=0; i<VOICES; i+=1) {
        // simple
        env_init(i, 
            2000,    // 2 second attack
            3000,    // 3 second decay
            4000,    // 4 second release
            ENV_SCALE, (ENV_SCALE * 7) / 10);
    }

    user_start();

    pthread_t user_thread;
    pthread_create(&user_thread, NULL, user, NULL);
    pthread_detach(user_thread);

    //pthread_t etf_thread;
    //pthread_create(&etf_thread, NULL, etf, NULL);
    //pthread_detach(etf_thread);

    pthread_t udp_thread;
    pthread_create(&udp_thread, NULL, udp, NULL);
    pthread_detach(udp_thread);

    fflush(stdout);

    signal(SIGABRT, signal_handler);

    printf("# using playback device %d -> \"%s\"\n", playbackindex, audio_playbackname(playbackindex));
    printf("# using capture device %d -> \"%s\"\n", captureindex, audio_capturename(captureindex));
    if (audio_open(theplayback, thecapture, SAMPLE_RATE, 0, ms, 1) != 0) {
        puts("WTF?");
    } else {
      printf("# audio state %s\n", audio_state());
      audio_start(engine);
      while (user_running()) sleep(1);
      audio_stop();
      while (audio_running()) sleep(1);
      audio_close();
    }
    while (udp_running()) sleep(1);

    //pthread_cancel(user_thread);
    //pthread_cancel(etf_thread);
    //pthread_cancel(udp_thread);

    pthread_join(user_thread, NULL);
    //pthread_join(etf_thread, NULL);
    pthread_join(udp_thread, NULL);

    return 0;
}
