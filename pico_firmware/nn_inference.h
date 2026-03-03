#ifndef NN_INFERENCE_H
#define NN_INFERENCE_H

#include <stdint.h>
#include <stddef.h>

#define INPUT_SIZE 1024
#define HIDDEN_SIZE 256
#define OUTPUT_SIZE 50

// Evaluates the given Morgan Fingerprint (1 or 0 values represented as floats for simplicity, or int8)
// against the quantized policy network.
//
// fp: input fingerprint of length 1024 (values 0.0f or 1.0f)
// out_logits: output buffer of length 10
void nn_inference(const float* fp, float* out_logits);

#endif // NN_INFERENCE_H
