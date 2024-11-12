#include <stdio.h>
#include <stdint.h>
#include <string.h>

#if 0
//
#include "clonk.h"
#include "dator.h"
#include "whizo.h"
#include "zoe/zoe.h"

#include "tools.h"

#include "amyplus/amy.h"
#include "amyplus/amy_config.h"
#include "amyplus/pcm.h"


#define NEWLEN (256)

#define ESTACK_LEN (64)
struct event _stacke[ESTACK_LEN+1];
int event_sp = -1;

#define POSC 64
static int _wave[64] = { [0 ... POSC-1] = -1 };
static int _patch[64] = { [0 ... POSC-1] = -1 };
static float _velocity[64] = { [0 ... POSC-1] = 0 };

int guess_patch(int osc) {
    if (osc >= 0 && osc <= POSC) {
        return _patch[osc];
    }
    return -1;
}

void show_used(int *a, int alen) {
    char used[POSC*32] = "";
    for (int i=0; i<POSC; i++) {
        char sep = '/';
        if (_wave[i] != synth[i].wave) continue; // address S65/S* result mis-match
        if (_wave[i] >= 0 && _wave[i] != 11) {
            char this[8];
            sprintf(this, "v%dw%d", i, _wave[i]);
            if (a && i < alen) a[i] = 1;
            strcat(used, this);
            if ((_wave[i] == PCM || _wave[i] == ALGO || _wave[i] == PARTIAL)  && _patch[i] >= 0) {
                sprintf(this, "p%d", _patch[i]);
                strcat(used, this);
            }
            if (synth[i].mod_source >= 0) {
                sprintf(this, "%cv%d", sep, synth[i].mod_source);
                strcat(used, this);
                if (_wave[i] == ALGO) {
                    // need to ask brian/dan if this is really true
                    // but observing ALGO, it seems there's an
                    // oscillator being used but not evident in
                    // synth[] fields where I've seen.
                    sprintf(this, "%cv%d", sep, synth[i].mod_source-1);
                    strcat(used, this);
                }
            }
            if (_wave[i] == ALGO) {
                for (int j=0; j<6; j++) {
                    if (j>0) sep = ',';
                    int s = synth[i].algo_source[j];
                    if (s >= 0) {
                        sprintf(this, "%cv%d", sep, synth[i].algo_source[j]);
                        strcat(used, this);
                    }
                }
            }
            strcat(used, "\n");
        }
    }
    if (a == NULL) printf("%s", used);
}

void show_free(void) {
    int mark[POSC] = { [0 ... POSC-1] = -1 };
    char notused[POSC*8] = "";
    for (int i=0; i<POSC; i++) {
        if (_wave[i] != synth[i].wave) continue; // address S65/S* result mis-match
        if (_wave[i] >= 0 && _wave[i] != 11) {
            mark[i] = 1;
            if (synth[i].mod_source >= 0) {
                mark[synth[i].mod_source] = 1;
                if (_wave[i] == ALGO) {
                    // need to ask brian/dan if this is really true
                    // but observing ALGO, it seems there's an
                    // oscillator being used but not evident in
                    // synth[] fields where I've seen.
                    mark[synth[i].mod_source-1] = 1;
                }
            }
            if (_wave[i] == ALGO) {
                for (int j=0; j<6; j++) {
                    int s = synth[i].algo_source[j];
                    if (s >= 0) mark[s] = 1;
                }
            }
        }
    }
    char this[8];
    int state = 0;
    int start = -1;
    int stop = -1;
    int i=0;
    for (i=0; i<POSC; i++) {
        switch (state) {
            case 0: // looking for empty
                if (mark[i] < 0) {
                    start = i;
                    state = 1;
                }
                break;
            case 1: // looking for used
                if (mark[i] >= 0) {
                    stop = i-1;
                    if (start == stop) {
                        printf("v%d\n", start);
                    } else {
                        printf("v%d-%d\n", start, stop);
                    }
                    state = 0;
                }
                
        }
    }
    //printf("i:%d start:%d stop:%d state:%d\n", i, start, stop, state);
    if (state == 1) {
        printf("v%d-%d\n", start, i-1);
    }
}

int amy_parse(char *s, int len) {
    //INFO printf("! AMY {%.*s}\n", len, s);
    char new[NEWLEN+1];
    char *out = new;
    new[0] = '\0';
    // expand +INT and $[a-z] (USRVAR) and $? top-of-stack
    int l = 0;
    //int64_t mark = amy_sysclock();
    int64_t mark = now();
    int voice_count = 0;
    int last_was_time = 0;
    if (!(s[0] == 't' || s[1] == '+')) {
        l = sprintf(new, "t%d", mark);
        out += l;
    }
    while (*s) {
        if (*s == 'v') {
            if (voice_count > 0) {
                if (out > new) {
                    if (!last_was_time) {
                        push_amy(new);
                        out = new;
                        last_was_time = 0;
                    }
                }
                voice_count = 0;
            } else {
                voice_count++;
            }
        }
        switch (*s) {
            case ' ':
                s++;
                break;
            case '_':
                // _ means generate "t" from now + the time from the meter table
                //   indexed from the next two integers
                if (out > new) {
                    push_amy(new);
                    out = new;
                }
                last_was_time = 1;
                s++;
                if (between(s[0], '1', '8') && between(s[1], '1', '8')) {
                    int64_t n = get_meter(s[0]-'1', s[1]-'1');
                    //INFO printf("REL METER got %d + now %d = %d\n", n, mark, n+mark);
                    n += mark;
                    if (n > 0) {
                        l = sprintf(out, "t%d", n);
                        out += l;
                    }
                    s+=2;
                }
                break;
#if 1 // maybe amy can do this internally?
            case '+':
                // + means generate "t" of now + the integer # of ms following
                if (out > new) {
                    push_amy(new);
                    out = new;
                }
                {
                    last_was_time = 1;
                    s++;
                    int64_t n = 0;
                    while (1) {
                        char c = *s++;
                        if (c == '\0') break;
                        else if (c >= '0' && c <= '9') {
                            n *= 10;
                            n += c -'0';
                        } else {
                            //INFO printf("REL got %d + now %d = %d\n", n, mark, n+mark);
                            n += mark;
                            if (n > 0) {
                                l = sprintf(out, "t%d", n);
                                out += l;
                            }
                            if (c != ' ') {
                                *out++ = c;
                                *out = '\0';
                            }
                            break;
                        }
                    }
                }
                break;
#endif
            case '$':
                {
                    char *v = NULL;
                    s++;
                    if (*s == '?') {
                        // this is completely untested...
                        v = top();
                    } else {
                        v = get(s);
                        // printf("$%c={%s}\n", *s, v);
                    }
                    if (v && strlen(v) > 0) {
                        while (*v) {
                            *out++ = *v;
                            *out = '\0';
                            v++;
                        }
                        s++;
                    }
                }
                break;
            default:
                *out++ = *s;
                *out = '\0';
                s++;
                break;
        }
        if ((out-new) >= NEWLEN) break;
    }
    if (out > new) {
        push_amy(new);
        out = new;
    }
    for (int i=0; i<depth_amy(); i++) {
        char *this = at_amy(i);
        INFO printf("= {%s} (%d)\n", this, strlen(this));
        if (strlen(this) > 0) {
            //amy_play_message(this);
            struct event e = amy_parse_message(this);
            // catch wave, patch, and velocity to help avoid sigfault in capture -> patch code
            //printf("osc:%d wave:%d patch:%d velocity:%g\n", e.osc, e.wave, e.patch, e.velocity);
            if (e.wave >= 0) _wave[e.osc] = e.wave;
            if (e.patch >= 0) _patch[e.osc] = e.patch;
            if (e.velocity >= 0) _velocity[e.osc] = e.velocity;
            if (e.status == SCHEDULED) {
                amy_add_event(e);
            }
        }
    }
    clear_amy();
    return 0;
}

#define RING_LEN (64)

static void *ring[RING_LEN] = { [0 ... RING_LEN-1] = NULL };
static int ring_index = 0;

#define GUARD (16)
 
int capture_frames(void) {
    return amy_frames();
}

int patch_in_use(int patch) {
}

int capture_to_patch(int patch, int note, int loop) {
    int length = amy_frames();
    if (length <= 0) return 0;
    // writing to a patch while it's in use somewhere else might segfault :(
    // TODO track what OSCs might be using this patch and bail to prevent
    char inuse[POSC*4+1] = "";
    for (int i=0; i<POSC; i++) {
        if ((_wave[i] == PCM) && (_patch[i] == patch) && (_velocity[i] > 0)) {
            // see if the synth entry is still PCM
            if (synth[i].wave == PCM) {
                char osc[5];
                sprintf(osc, "v%d ", i);
                strcat(inuse, osc);
            } else {
                _wave[i] == synth[i].wave;
            }
        }
    }
    if (strlen(inuse)) {
        printf("! FAIL : >p%d may corrupt waveform used in %s\n", patch, inuse);
        return -1;
    }
    INFO printf("! >p %d %d %d\n", patch, note, loop);
    if (note < 0) note = 0;
    if (note > 127) note = 127;
    // loop is unused for now
    int16_t *capture = amy_captured();
    if ((patch < pcm_rom_patches()) || (patch > (pcm_rom_patches() + pcm_usr_patches()))) {
        printf("# cannot crunch into rom patch %d\n", patch);
    } else {
        pcm_map_t *map = pcm_patch_to_map(patch);
        if (map->external != NULL) {
            INFO printf("# %p here... tuck it into the ring[%d]\n", map->external, ring_index);
            if (ring[ring_index]) {
                free(ring[ring_index]);
                ring[ring_index] = map->external;
            }
            ring_index++;
            if (ring_index >= RING_LEN) ring_index = 0;
            map->external = NULL;
        }
        INFO printf("! length = %d\n", length);
        int16_t *sample = malloc((length+GUARD) * sizeof(int16_t));
        map->external = sample;
        // this is bare 44100
        for (int i=0; i<length; i++) sample[i] = capture[i];
        map->rate = 44100;
        map->offset = -1; // BE SURE???
        map->length = length;
        // curiously, @ 44100 Hz, having loopstart = 0 and loopend = length-1 breaks looping in pcm.c
        map->loopstart = 1;
        map->loopend = length-2;
        map->midinote = note;
    }
}

int patch_to_capture(int patch) {
    int length = 0;
    int offset = 0;
    if (patch < 0 || patch > 99) return 0;
    int16_t *capture = amy_captured();
    pcm_map_t *map = pcm_patch_to_map(patch);
    offset = map->offset;
    length = map->length;
    int i;
    int j = 0;
    int16_t *sample;
    if (map->offset < 0) {
        sample = map->external;
    } else {
        sample = &pcm[offset];
    }
    if (map->rate == 22050) {
        INFO printf("# stretch %d -> %d frames from patch %d\n",
            length, length*2, patch);
        for (i=0; i<length; i++) {
            capture[j++] = sample[i];
            capture[j++] = sample[i];
        }
        length *= 2;
    } else {
        INFO printf("# copy %d frames from patch %d\n", length, patch);
        for (i=0; i<length; i++) capture[i] = sample[i];
    }
    amy_set_frames(length);
    return 0;
}

void capture_reverse(void) {
    // reverse capture buffer
    int16_t *capture = amy_captured();
    int16_t a, b;
    int length = amy_frames();
    INFO printf("# reverse %d frames\n", length);
    int i;
    int j = length-1;
    for (i=0; i<(length/2); i++) {
        a = capture[i];
        b = capture[j];
        capture[j] = a;
        capture[i] = b;
        j--;
    }
}

void capture_dynexp(char *input) {
    double a0_max = (double)INT16_MIN;
    double s16_max = (double)(INT16_MAX);
    int16_t *capture = amy_captured();
    int length = amy_frames();
    int i;
    // maximize capture buffer dynamic range
    for (i=0; i<length; i++) {
        double n = (double)capture[i];
        a0_max = MAX(a0_max, n);
    }
    double factor = s16_max / a0_max;
    INFO printf("# a0_max=%g, s16_max=%g, factor=%g\n",
        a0_max, s16_max, factor);
    // multiply each value by the scaling factor
    for (i=0; i<length; i++) {
        double sample = (double)capture[i];
        double n = (sample * factor);
        if (n > (double)INT16_MAX) {
            n = (double)INT16_MAX;
        } else if (n < (double)(INT16_MIN+1)) {
            n = (double)INT16_MIN+1;
        }
        capture[i] = (int16_t)(n);
    }
}

void capture_rotate_left(int len) {
    int frames = amy_frames();
    int16_t *capture = amy_captured();
    printf("len:%d frames:%d\n", len, frames);
    if (len < frames) {
        for (int i=0; i<(frames-len); i++) {
            int16_t t = capture[i+len];
            capture[i+len] = capture[i];
            capture[i] = t;
        }
    } else {
        puts("NOPE");
    }
}

void capture_rotate_right(int len) {
    int frames = amy_frames();
    int16_t *capture = amy_captured();
    printf("len:%d frames:%d\n", len, frames);
    if (len < frames) {
        for (int i=frames-1; i>len; i--) {
            int16_t t = capture[i];
            capture[i] = capture[i-len];
            capture[i-len] = t;
        }
    } else {
        puts("NOPE");
    }
}

void capture_to_ms(int ms) {
    int frames = amy_frames();
    int ms_in_frames = ms * AMY_SAMPLE_RATE / 1000;
    if (ms_in_frames < frames) {
        amy_trim(ms_in_frames);
    }
}

void capture_trim(int n) {
    int16_t *capture = amy_captured();
    int i;
    int top = 0;
    int end = amy_frames();
    INFO printf("n:%d end:%d\n", n, end);
    if (n > 0) {
        // trim by # samples
        if (n < end) amy_trim(end - n);
    } else if (n < 0) {
        n = -n;
        int ms = n * AMY_SAMPLE_RATE / 1000;
        INFO printf("! ms = %d\n", ms);
        if (ms < end) amy_trim(end - ms);
        // TODO trim by milliseconds
    } else if (n == 0) {
        // trim 0's at end
        int len = 0;
        int max = end;
        int flag = 0;
        for (i=0; i<end; i++) {
            if (capture[i] != 0) {
                top = i-1;
                if (top < 0) top = 0;
                flag = 1;
                break;
            }
        }
        if (flag) {
            for (i=end-1; i>=top; i--) {
                if (capture[i] != 0) {
                    end = i+1;
                    break;
                }
            }
            len = end - top + 1;
        } else {
            end = 0;
            len = 0;
        }
        amy_trim(len);
    }
}

void capture_start(int n) {
    int max = n;
    double ms_per_block = 1000.0 / AMY_SAMPLE_RATE * AMY_BLOCK_SIZE;
    double y = ceil((double)max/ms_per_block);
    int z = (int)y;
    if (z <= 0) z = 0;
    if (z >= BLOCK_COUNT) {
        z = BLOCK_COUNT;
        INFO printf("# limiting capture to %d\n", z);
    }
    INFO printf("# capturing %d blocks -> %d frames -> %g ms\n", z, z*AMY_BLOCK_SIZE, z*ms_per_block);
    amy_enable_capture(1, z * AMY_BLOCK_SIZE);
}

void capture_stop(void) {
    amy_enable_capture(0, 0);
    INFO printf("# capture stopped\n");
}
//
#endif

typedef struct {
    char      RIFFChunkID[4];
    uint32_t  RIFFChunkSize;
    char      Format[4];
    char      FormatSubchunkID[4];
    uint32_t  FormatSubchunkSize;
    uint16_t  AudioFormat;
    uint16_t  Channels;
    uint32_t  SamplesRate;
    uint32_t  ByteRate;
    uint16_t  BlockAlign;
    uint16_t  BitsPerSample;
    char      DataSubchunkID[4];
    uint32_t  DataSubchunkSize;
    // double* Data; 
} wav_t;

// typedef struct {
//     char      RIFFChunkID[4];
//     uint32_t  RIFFChunkSize;
//     uint32_t  Manufacturer;
//     uint32_t  Product;
//     uint32_t  SamplePeriod;
//     uint32_t  MIDIUnityNote;
//     uint32_t  MIDIPitchFraction;
//     uint32_t  SMPTEFormat;
//     uint32_t  SMPTEOffset;
//     uint32_t  SampleLoops;
//     uint32_t  SamplerData;
//     // hardcoded for one loop but SampleLoops by
//     // the standard supports more than one
//     // typedef struct {
//     uint32_t Identifier;
//     uint32_t Type;
//     uint32_t Start;
//     uint32_t End;
//     uint32_t Fraction;
//     uint32_t PlayCount;
//     // } SampleLoop;
// } sampler_t;

//void capture_to_wav(char *name) {
int mw_put(char *name, int16_t *capture, int frames) {
    // int16_t *capture = amy_captured();
    // int frames = amy_frames();
    wav_t wave = {
        .RIFFChunkID = {'R', 'I', 'F', 'F'},
        .RIFFChunkSize = frames + 36,
        .Format = {'W', 'A', 'V', 'E'},
        .FormatSubchunkID = {'f', 'm', 't', ' '},
        .FormatSubchunkSize = 16,
        .AudioFormat = 1,
        .Channels = 1,
        .SamplesRate = 44100,
        .ByteRate = 44100,
        .BlockAlign = 2,
        .BitsPerSample = 16,
        .DataSubchunkID = {'d', 'a', 't', 'a'},
        .DataSubchunkSize = frames,
    };
    FILE *out = NULL;
    out = fopen(name, "wb+");
    if (out) {
        int n;
        n = fwrite(&wave, 1, sizeof(wave), out);
        n = fwrite(capture, 1, frames * sizeof(int16_t), out);
        fclose(out);
    } else {
        perror("! fopen");
        return -1;
    }
    return frames;
}

int mw_frames(char *name) {
    FILE *in = fopen(name, "rb");
    int length = 0;

    if (in) {
        wav_t wav;
        int frames = -1;
        int n = fread(&wav, sizeof(wav_t), 1, in);
        if (n > 0) {
            while (1) {
                if (strncmp(wav.RIFFChunkID, "RIFF", 4) != 0) break;
                if (strncmp(wav.Format, "WAVE", 4) != 0) break;
                if (strncmp(wav.FormatSubchunkID, "fmt ", 4) != 0) break;
                if (wav.Channels > 2) break;
                if (wav.SamplesRate != 44100) break;
                if (wav.BitsPerSample != 16) break;
                if (strncmp(wav.DataSubchunkID, "data", 4) != 0) break;
                frames = wav.DataSubchunkSize / wav.Channels / (wav.BitsPerSample / 8);
                break;
            }
        }
        fclose(in);
        return frames;
    }
    return -1;    
}

int mw_get(char *name, int16_t *dest, int copy_max) {
    // printf("mw_get(%s, %p, %d)\n", name, dest, copy_max);
    FILE *in = fopen(name, "rb");
    int length = 0;

    if (in) {
        wav_t wav;
        int n = fread(&wav, sizeof(wav_t), 1, in);
        if (n > 0) {
            while (1) {
                if (strncmp(wav.RIFFChunkID, "RIFF", 4) != 0) break;
                if (strncmp(wav.Format, "WAVE", 4) != 0) break;
                if (strncmp(wav.FormatSubchunkID, "fmt ", 4) != 0) break;
                if (wav.Channels > 2) break;
                if (wav.SamplesRate != 44100) break;
                if (wav.BitsPerSample != 16) break;
                if (strncmp(wav.DataSubchunkID, "data", 4) != 0) break;
                int frames = wav.DataSubchunkSize / wav.Channels / (wav.BitsPerSample / 8);
                // double rate = 1000.0 / (double)wav.SamplesRate;
                // double msec = rate * (double)frames;
                // int16_t *dest;
                int16_t frameBlock[2];
                while (length < copy_max) {
                    // printf("length = %d\n", length);
                    n = fread(frameBlock, sizeof(int16_t) * wav.Channels, 1, in);
                    if (n < 0) break;
                    if (n == 0) break;
                    int32_t sample = frameBlock[0];
                    if (wav.Channels == 2) {
                        // average stereo channels together
                        sample += frameBlock[1];
                        sample /= 2;
                    }
                    if (sample > 32767) sample = 32767;
                    if (sample < -32767) sample = 32767;
                    dest[length] = sample;
                    length++;
                }
                break;
            }
            fclose(in);
        }
    } else {
        perror("fopen");
    }
    return length;
}
