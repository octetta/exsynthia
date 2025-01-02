#include <stdio.h>
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

void measure_backend_latency(ma_backend backend) {
    ma_context context;
    ma_result result = ma_context_init(&backend, 1, NULL, &context);
    if (result != MA_SUCCESS) {
        printf("Failed to initialize backend %d\n", backend);
        return;
    }

    ma_device_info* playbackDevices;
    ma_uint32 playbackDeviceCount;
    result = ma_context_get_devices(&context, &playbackDevices, &playbackDeviceCount, NULL, NULL);
    if (result != MA_SUCCESS) {
        printf("Failed to enumerate devices for backend %d\n", backend);
        ma_context_uninit(&context);
        return;
    }

    printf("Backend %d:\n", backend);
    for (ma_uint32 i = 0; i < playbackDeviceCount; ++i) {
        printf("  Device: %s\n", playbackDevices[i].name);

        // Configure a playback device to query latency
        ma_device_config deviceConfig = ma_device_config_init(ma_device_type_playback);
        deviceConfig.playback.pDeviceID = &playbackDevices[i].id;
        deviceConfig.sampleRate = 48000;              // Example sample rate
        deviceConfig.playback.format = ma_format_f32; // Example format
        deviceConfig.playback.channels = 2;          // Stereo
        deviceConfig.periodSizeInFrames = 512;       // Example buffer size (frames)
        deviceConfig.periods = 3;                    // Default to triple buffering

        ma_device device;
        result = ma_device_init(&context, &deviceConfig, &device);
        if (result == MA_SUCCESS) {
            ma_uint32 bufferSizeInFrames = deviceConfig.periodSizeInFrames * deviceConfig.periods;
            double latencyMs = (double)bufferSizeInFrames / (double)device.sampleRate * 1000.0;
            printf("    Latency: %.2f ms (%d frames at %d Hz)\n", latencyMs, bufferSizeInFrames, device.sampleRate);
            ma_device_uninit(&device);
        } else {
            printf("    Failed to initialize device for latency measurement.\n");
        }
    }

    ma_context_uninit(&context);
}

int main() {
    ma_backend backends[] = {
        ma_backend_wasapi,
        ma_backend_dsound,
        ma_backend_winmm,
        ma_backend_alsa,
        ma_backend_pulseaudio,
        ma_backend_jack,
        ma_backend_coreaudio,
        ma_backend_opensl,
        ma_backend_webaudio,
        ma_backend_null
    };
    ma_uint32 backendCount = sizeof(backends) / sizeof(backends[0]);

    printf("Testing %d backends.\n", backendCount);
    for (ma_uint32 i = 0; i < backendCount; ++i) {
        measure_backend_latency(backends[i]);
    }

    return 0;
}
