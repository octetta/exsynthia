#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define Q 32768 // Fixed-point scaling factor

typedef struct {
    int32_t state[4]; // State of the 4 filter stages
} MoogFilter;

void init_filter(MoogFilter *filter) {
    for (int i = 0; i < 4; i++) {
        filter->state[i] = 0;
    }
}

int32_t process_sample(MoogFilter *filter, int32_t input, int32_t cutoff, int32_t resonance) {
    int32_t cutoff_scaled = (cutoff * Q) / 32768;
    int32_t resonance_scaled = (resonance * Q) / 32768;

    // Feedback calculation
    int32_t input_scaled = (input * Q) / 32768;
    int32_t fb = (resonance_scaled * filter->state[3]) / Q;
    int32_t input_with_feedback = input_scaled - fb;

    // Four cascaded 1-pole filters
    filter->state[0] += ((input_with_feedback - filter->state[0]) * cutoff_scaled) / Q;
    filter->state[1] += ((filter->state[0] - filter->state[1]) * cutoff_scaled) / Q;
    filter->state[2] += ((filter->state[1] - filter->state[2]) * cutoff_scaled) / Q;
    filter->state[3] += ((filter->state[2] - filter->state[3]) * cutoff_scaled) / Q;

    // Adjust output scaling
    int32_t output = (filter->state[3] * 32768) / Q;
    if (output > 32767) output = 32767;
    if (output < -32768) output = -32768;

    return output;
}

int main(int argc, char *argv[]) {
    MoogFilter filter;
    init_filter(&filter);

    // Parameters
    int32_t input_sample = 32767; // Larger input for testing
    int32_t cutoff = 30000;       // Higher cutoff frequency
    int32_t resonance = 0;        // No resonance for debugging
    int num_samples = 1000;         // Number of samples to process
    if (argc > 1) {
        input_sample = atoi(argv[1]);
    }
    if (argc > 2) {
        cutoff = atoi(argv[2]);
    }
    // Process multiple samples
    int32_t last = -1;
    int32_t repeat = 0;
    for (int i = 0; i < num_samples; i++) {
        int32_t output = process_sample(&filter, input_sample, cutoff, resonance);
        printf("[%d] %d -> %d\n", i, input_sample, output);
        if (last == output) {
            repeat++;
            if (repeat > 2) break;
        } else {
            repeat = 0;
        }
        last = output;
    }

    return 0;
}
