#include <stdlib.h>
#include <sys/time.h>


unsigned long long frames_sent = 0;

static int audio_is_running = 0;

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

static char _audio_state_string[128] = "not-running";

char *audio_state(void) {
  return _audio_state_string;
}

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
  return 0;
}

static char *MA_playbackname(int i) {
    if (!MA_started) return "?";
    if (i >= 0) {
        return pPlaybackInfos[i].name;
    }
    return "?";
}

static char *MA_capturename(int i) {
    if (!MA_started) return "?";
    if (i >= 0) {
        return pCaptureInfos[i].name;
    }
    return "?";
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
  char name[1024];
  char lfilter[1024];
  if (!output) {
    strcpy(lfilter, filter);
    strlower(lfilter);
  }
  if (what && what[0] == 'p') {
    if (output) puts("# playback devices");
    if (filter && filter[0] >= '0' && filter[0] <= '9') {
      return atoi(filter);
    }
    for (i=0; i<playbackCount; i++) {
        strcpy(name, pPlaybackInfos[i].name);
        strlower(name);
        if (output) printf("-p%d # \"%s\" (%s)\n", i, name, pPlaybackInfos[i].id.alsa);
        if (!output) {
            char *needle = lfilter;
            char *haystack = name;
            if (strstr(haystack, needle)) {
                printf("# <%s> found in <%s> -> %d\n", needle, haystack, i);
                return i;
            }
        }
    }
  }

  if (what && what[0] == 'c') {
    if (output) puts("# capture devices");
    for (i=0; i<captureCount; i++) {
        if (output) printf("-c%d # \"%s\" (%s)\n", i, pCaptureInfos[i].name, pCaptureInfos[i].id.alsa);
    }
  }
  return 1000;
}

static void (*audio_fn)(int16_t*,int16_t*,int) = NULL;

static void audio_data_cb(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frame_count) {
    if (audio_fn && audio_is_running) {
        audio_fn((int16_t *)pOutput, (int16_t *)pInput, frame_count);
        frames_sent+=frame_count;
    }
}

static int MA_audio_start(void (*fn)(int16_t*,int16_t*,int)) {
    audio_fn = fn;
    return 0;
}

static void audio_notification_cb(const ma_device_notification* pNotification) {
  return;
}

static int MA_audio_open(char *outdev, char *indev, int sample_rate, int buffer_len, int ms, int period) {
    MA_init();
    if (outdev == NULL) return -1;
    if (outdev[0] == '\0') return -1;
    int playback = atoi(outdev);
    if (playback < 0) return -1;
    if (playback == 0) {
        //
    } else if (playback >= pbmax) return 0;

    capture = -1; // disable capture for now
    if (indev) {
      strcpy(_audio_state_string, "running-duplex");
      config = ma_device_config_init(ma_device_type_duplex);
    } else {
      strcpy(_audio_state_string, "running-playback-only");
      config = ma_device_config_init(ma_device_type_playback);
    }
  if (capture >= 0) {
    //config.capture.pDeviceID = &pCaptureInfos[capture].id;
    //config.capture.pDeviceID = NULL;
  }
  config.playback.format   = ma_format_s16;
  config.playback.channels = 2;
  config.sampleRate        = sample_rate;
  config.dataCallback      = audio_data_cb;
  config.notificationCallback = audio_notification_cb;
  config.pUserData         = custom_data;

  if (capture >= 0) {
    config.capture.format = ma_format_s16;
    config.capture.channels = 2;
    config.capture.shareMode = ma_share_mode_shared;
  }

  //config.periodSizeInFrames = buffer_len;
  config.periodSizeInMilliseconds = ms;
  config.periods = period;

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


// -----------

int audio_running(void) {
    return audio_is_running;
}

int audio_open(char *outdev, char *indev, int sample_rate, int buffer_len, int ms, int period) {
  printf("# using miniaudio v%s from https://miniaud.io\n", MA_VERSION_STRING);
  return MA_audio_open(outdev, indev, sample_rate, buffer_len, ms, period);
}

int audio_start(void (*fn)(int16_t*,int16_t*,int)) {
  audio_is_running = 1;
  return MA_audio_start(fn);
}

int audio_stop(void) {
    audio_is_running = 0;
    return audio_is_running;
}

int audio_list(char *what, char *filter) {
  return MA_audio_list(what, filter);
}

void audio_close(void) {
  MA_audio_close();
}

char *audio_playbackname(int i) {
  return MA_playbackname(i);
}

char *audio_capturename(int i) {
  return MA_capturename(i);
}
