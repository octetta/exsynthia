#include <stdlib.h>
#include <sys/time.h>

#include <alsa/asoundlib.h>

struct timeval rtns0;
struct timeval rtns1;

unsigned long long sent = 0;
long int rtms = 0;
long int btms = 0;
long int diff = 0;
#define LATENCY_HACK_MS (100)
int latency_hack_ms = LATENCY_HACK_MS;

// ALSA variables
snd_pcm_t *pcm_handle = NULL;
snd_pcm_hw_params_t *hw_params = NULL;

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
int setup_alsa(char *device, int sample_rate, int buffer_size) {
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

static int audio_buffer_len = 1024;
int16_t *audio_buffer = NULL;
int audio_sample_rate = 44100;

int exsynth_open(char *device, int sample_rate, int buffer_len) {
  audio_buffer_len = buffer_len;
  audio_buffer = (int16_t *)malloc(buffer_len * sizeof(int16_t));
  audio_sample_rate = sample_rate;
  return setup_alsa(device, audio_sample_rate, audio_buffer_len);
}

void exsynth_close(void) {
  snd_pcm_close(pcm_handle);
}

int exsynth_main(int *flag, void (*fn)(int16_t*,int)) {
    int16_t *buffer = audio_buffer;;
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
}

void listalsa(char *what) {
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
}

