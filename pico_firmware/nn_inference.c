#include "nn_inference.h"
#include "nn_weights.h"

// Basic Rectified Linear Unit activation
static inline float relu(float x) {
    return x > 0.0f ? x : 0.0f;
}

void nn_inference(const float* fp, float* out_logits) {
    float hidden[HIDDEN_SIZE] = {0};

    // Layer 1: fc1
    for (int j = 0; j < HIDDEN_SIZE; ++j) {
        float sum = fc1_bias[j];
        for (int i = 0; i < INPUT_SIZE; ++i) {
            if (fp[i] > 0.0f) {
                // Dequantize weight on the fly for simple accumulation
                // w_f = w_int8 * scale
                sum += (float)(fc1_weight[j * INPUT_SIZE + i]) * FC1_SCALE;
            }
        }
        hidden[j] = relu(sum);
    }

    // Layer 2: fc2
    for (int k = 0; k < OUTPUT_SIZE; ++k) {
        float sum = fc2_bias[k];
        for (int j = 0; j < HIDDEN_SIZE; ++j) {
            sum += hidden[j] * (float)(fc2_weight[k * HIDDEN_SIZE + j]) * FC2_SCALE;
        }
        out_logits[k] = sum;
    }
}
