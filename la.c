#include "miniaudio.h"
#include <stdio.h>

int main() {
    ma_context context;
    ma_result result;
    ma_device_info* playback_devices;
    ma_device_info* capture_devices;
    ma_uint32 playback_device_count;
    ma_uint32 capture_device_count;

    // Initialize the Miniaudio context
    result = ma_context_init(NULL, 0, NULL, &context);
    if (result != MA_SUCCESS) {
        fprintf(stderr, "Failed to initialize Miniaudio context: %s\n", ma_result_to_string(result));
        return 1;
    }

    // Get the available playback and capture devices
    result = ma_context_get_devices(&context, &playback_devices, &playback_device_count, &capture_devices, &capture_device_count);
    if (result != MA_SUCCESS) {
        fprintf(stderr, "Failed to enumerate devices: %s\n", ma_result_to_string(result));
        ma_context_uninit(&context);
        return 1;
    }

    // List ALSA playback devices
    printf("ALSA Playback Devices:\n");
    for (ma_uint32 i = 0; i < playback_device_count; i++) {
        printf("  %d: %s (ID: %s)\n", i, playback_devices[i].name, playback_devices[i].id);
    }

    // List ALSA capture devices
    printf("\nALSA Capture Devices:\n");
    for (ma_uint32 i = 0; i < capture_device_count; i++) {
        printf("  %d: %s (ID: %s)\n", i, capture_devices[i].name, capture_devices[i].id);
    }

    // Uninitialize the Miniaudio context
    ma_context_uninit(&context);

    return 0;
}
