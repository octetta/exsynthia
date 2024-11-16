#include <stdlib.h>
#include <sys/time.h>

#ifdef USE_ALSA
#include <alsa/asoundlib.h>
#endif

struct timeval rtns0;
struct timeval rtns1;

unsigned long long sent = 0;
long int rtms = 0;
long int btms = 0;
long int diff = 0;
#define LATENCY_HACK_MS (100)
int latency_hack_ms = LATENCY_HACK_MS;

static int audio_buffer_len = 1024;
static int16_t *audio_buffer = NULL;
static int audio_sample_rate = 44100;

// ---------------

// miniaudio stuff

#define MA_NO_FLAC
#define MA_NO_MP3
#define MA_NO_RESOURCE_MANAGER
#define MA_NO_NODE_GRAPH
#define MA_NO_ENGINE
#define MA_NO_GENERATION
//#define MA_DEBUG_OUTPUT
#define MINIAUDIO_IMPLEMENTATION

#define MA_NO_JACK
#define MA_NO_PULSEAUDIO

#include "miniaudio.h"

static ma_device_config config;
static ma_device device;

static unsigned char custom_data[2048];

static ma_context context;

static ma_device_info* pPlaybackInfos;
static ma_uint32 playbackCount;

static ma_device_info* pCaptureInfos;
static ma_uint32 captureCount;

static int playback = -1;
static int pbmax = -1;
static int capture = -1;
static int capmax = -1;

static char MA_started = 0;

int MA_init(void) {
    if (MA_started) return 0;
    MA_started = 1;
    if (ma_context_init(NULL, 0, NULL, &context) != MA_SUCCESS) {
    printf("failed to get MA context\n");
    return -1;
  }

  if (ma_context_get_devices(&context,
    &pPlaybackInfos,
    &playbackCount,
    &pCaptureInfos,
    &captureCount) != MA_SUCCESS) {
    return -1;
  }
  int i;

  for (i=0; i<playbackCount; i++) {
    pbmax = i;
  }
  if (pbmax > 0) {
    playback = 0;
  }

  for (i=0; i<captureCount; i++) {
    capmax = i;
  }
}

#include <ctype.h>

static void strlower(char *s) {
    while (s && *s != '\0') {
        if (*s >= 'A' && *s <= 'Z') *s = tolower(*s);
        s++;
    }
}

static int MA_audio_list(char *what, char *filter) {
  MA_init();
  int i;
  char output = 0;
  if (filter == NULL) output = 1;
  if (filter && filter[0] == '\0') output = 1;
  if (output) puts("playback devices");
  char name[1024];
  char lfilter[1024];
  if (!output) {
    strcpy(lfilter, filter);
    strlower(lfilter);
  }
  for (i=0; i<playbackCount; i++) {
    strcpy(name, pPlaybackInfos[i].name);
    strlower(name);
    if (output) printf("%d <%s>\n", i, name);
    if (!output) {
        char *needle = lfilter;
        char *haystack = name;
        if (strstr(haystack, needle)) {
            // printf("<%s> found in <%s> -> %d\n", needle, haystack, i);
            return i;
        }
    }
  }

  if (output) puts("capture devices");
  for (i=0; i<captureCount; i++) {
    if (output) printf("%d : %s\n", i, pCaptureInfos[i].name);
  }
  return 1000;
}

static void (*audio_fn)(int16_t*,int) = NULL;

static void audio_data_cb(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frame_count) {
    if (audio_fn) {
        audio_fn(audio_buffer, frame_count);
        int16_t *poke = (int16_t *)pOutput;
        int ptr = 0;
        for (int i=0; i<frame_count; i++) {
            poke[ptr++] = audio_buffer[i];
            poke[ptr++] = audio_buffer[i];
        }
    }
}

static int MA_audio_main(int *flag, void (*fn)(int16_t*,int)) {
    audio_fn = fn;
}

static void audio_notification_cb(const ma_device_notification* pNotification) {
}

static int MA_audio_open(char *outdev, char *indev, int sample_rate, int buffer_len) {
    MA_init();
    if (outdev == NULL) return -1;
    if (outdev[0] == '\0') return -1;
    int playback = atoi(outdev);
    if (playback < 0) return -1;
    if (playback == 0) {
        //
    } else if (playback >= pbmax) return 0;

    config = ma_device_config_init(ma_device_type_playback);
    // TODO when using a capture device:
    // config = ma_device_config_init(ma_device_type_duplex);
      config.playback.pDeviceID = &pPlaybackInfos[playback].id;
  if (capture >= 0) {
    config.capture.pDeviceID = &pCaptureInfos[capture].id;
  }
  config.playback.format   = ma_format_s16;
  //config.playback.format   = ma_format_f32;
  config.playback.channels = 2;
  config.sampleRate        = sample_rate;
  config.dataCallback      = audio_data_cb;
  config.notificationCallback = audio_notification_cb;
  config.pUserData         = custom_data;

  if (capture >= 0) {
    config.capture.format = ma_format_s16;
    config.capture.channels = 2;
  }

  config.periodSizeInFrames = buffer_len;

  if (ma_device_init(&context, &config, &device) != MA_SUCCESS) {
    // printf("failed to open I/O devices\n");
    ma_device_uninit(&device);
    ma_context_uninit(&context);
    return -1;
  }
 
  // start audio processing
  if (ma_device_start(&device) != MA_SUCCESS) {
    // printf("Failed to start playback device.\n");
    ma_device_uninit(&device);
    return -1;
  }
  return 0;
}

void MA_audio_close(void) {
}

// ----------

#ifdef USE_ALSA

// ALSA stuff
static snd_pcm_t *pcm_handle = NULL;
static snd_pcm_hw_params_t *hw_params = NULL;

// ALSA error handler
void check_alsa_error(int err, const char *msg) {
    if (err < 0) {
        fprintf(stderr, "%s: %s\n", msg, snd_strerror(err));
        fflush(stderr);
        fflush(stdout);
        // exit(EXIT_FAILURE);
    }
}

// ALSA setup
static int ALSA_open(char *device, char *indev, int sample_rate, int buffer_size) {
    // printf("setup_alsa : <%s>\n", device);
    int err;

    // Open ALSA device for playback
    err = snd_pcm_open(&pcm_handle, device, SND_PCM_STREAM_PLAYBACK, 0);
    if (err < 0) {
        check_alsa_error(err, "Cannot open PCM device");
        return -1;
    }

    // Allocate hardware parameters
    err = snd_pcm_hw_params_malloc(&hw_params);
    if (err < 0) {
        check_alsa_error(err, "cannot snd_pcm_hw_params_malloc");
        return -1;
    }
    
    err = snd_pcm_hw_params_any(pcm_handle, hw_params);
    if (err < 0) {
        check_alsa_error(err, "cannot snd_pcm_hw_params_any");
        return -1;
    }

    // Set hardware parameters
    err = snd_pcm_hw_params_set_access(pcm_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
    if (err < 0) {
        check_alsa_error(err, "cannot snd_pcm_hw_params_set_access");
        return -1;
    }

    err = snd_pcm_hw_params_set_format(pcm_handle, hw_params, SND_PCM_FORMAT_S16_LE);
    if (err < 0) {
        check_alsa_error(err, "cannot snd_pcm_hw_params_set_format");
        return -1;
    }

    err = snd_pcm_hw_params_set_channels(pcm_handle, hw_params, 1);  // Mono output
    if (err < 0) {
        check_alsa_error(err, "cannot snd_pcm_hw_params_set_channels");
        return -1;
    }

    err = snd_pcm_hw_params_set_rate(pcm_handle, hw_params, sample_rate, 0);
    if (err < 0) {
        check_alsa_error(err, "cannot snd_pcm_hw_params_set_rate");
        return -1;
    }

    err = snd_pcm_hw_params_set_period_size(pcm_handle, hw_params, buffer_size, 0);
    if (err < 0) {
        check_alsa_error(err, "cannot snd_pcm_hw_params_set_period_size");
        return -1;
    }

    // Apply hardware parameters
    err = snd_pcm_hw_params(pcm_handle, hw_params);
    if (err < 0) {
        check_alsa_error(err, "Cannot set hardware parameters");
        return -1;
    }

    // Free hardware parameters structure
    snd_pcm_hw_params_free(hw_params);

    // Prepare PCM for playback
    err = snd_pcm_prepare(pcm_handle);
    if (err < 0) {
        check_alsa_error(err, "cannot snd_pcm_prepare");
        return -1;
    }

    return 0;
}


void ALSA_audio_close(void) {
  snd_pcm_close(pcm_handle);
}

static int ALSA_audio_main(int *flag, void (*fn)(int16_t*,int)) {
    int16_t *buffer = audio_buffer;
    int err;
    while (*flag) {
        fn(buffer, audio_buffer_len);

        if ((err = snd_pcm_wait(pcm_handle, 1000)) < 0) {
            check_alsa_error(err, "PCM wait failed");
        }
        if ((err = snd_pcm_writei(pcm_handle, buffer, audio_buffer_len)) < 0) {
            if (err == -EPIPE) {
                // Recover from buffer underrun
                snd_pcm_prepare(pcm_handle);
            } else {
                check_alsa_error(err, "Failed to write to PCM device");
            }
        } else {
            // try to not get too far ahead of realtime...
            // without this, we get about 24 seconds ahead of realtime!
            #define TV2MS(t) ((t.tv_sec*1000)+(t.tv_usec/1000))
            sent+=audio_buffer_len;
            gettimeofday(&rtns1, NULL);
            rtms = TV2MS(rtns1)-TV2MS(rtns0);
            btms = sent * 1000 / audio_sample_rate;
            diff = btms - rtms;
            if (diff > latency_hack_ms) {
                diff -= latency_hack_ms;
                usleep(diff * 1000);
            }
        }
    }
    return 0;
}

static int ALSA_audio_list(char *what, char *filter) {
    int status;
    char *kind = strdup(what);
    char **hints;
    status = snd_device_name_hint(-1, kind, (void ***)&hints);
    if (status < 0) {
        puts("NOPE");
    } else {
        for (char **n = hints; *n != NULL; n++) {
            char *name = snd_device_name_get_hint(*n, "NAME");
            if (name != NULL) {
                puts(name);
                free(name);
            }
        }
        snd_device_name_free_hint((void **)hints);
    }
    return 0;
}

#endif

// -----------

int audio_open(char *outdev, char *indev, int sample_rate, int buffer_len) {
  audio_buffer_len = buffer_len;
  audio_buffer = (int16_t *)malloc(buffer_len * sizeof(int16_t));
  audio_sample_rate = sample_rate;
  #ifdef USE_ALSA
  puts("using raw ALSA");
  return ALSA_open(outdev, indev, audio_sample_rate, audio_buffer_len);
  #else
  puts("using MiniAudio");
  return MA_audio_open(outdev, indev, audio_sample_rate, audio_buffer_len);
  #endif
}

int audio_main(int *flag, void (*fn)(int16_t*,int)) {
  #ifdef USE_ALSA
  return ALSA_audio_main(flag, fn);
  #else
  return MA_audio_main(flag, fn);
  #endif
}

int audio_list(char *what, char *filter) {
  #ifdef USE_ALSA
  return ALSA_audio_list(what, filter);
  #else
  return MA_audio_list(what, filter);
  #endif
}

void audio_close(void) {
  #ifdef USE_ALSA
  ALSA_audio_close();
  #else
  MA_audio_close();
  #endif
}


