#include "smiles_hash.h"
#include <string.h>

// A simple DJB2-inspired rolling n-gram hasher to map strings to 1024 bits.
// We hash pairs of characters (bigrams) and set the corresponding bit in the 1024 array.
void smiles_to_fingerprint(const char* smiles, float* fp) {
    for (int i = 0; i < 1024; i++) {
        fp[i] = 0.0f;
    }

    int len = strlen(smiles);
    if (len == 0) return;

    for (int i = 0; i < len - 1; i++) {
        unsigned int hash = 5381;
        hash = ((hash << 5) + hash) + smiles[i];     // hash * 33 + c1
        hash = ((hash << 5) + hash) + smiles[i + 1]; // hash * 33 + c2
        
        int bit_idx = hash % 1024;
        fp[bit_idx] = 1.0f;
    }
    
    // Also hash single chars
    for (int i = 0; i < len; i++) {
        unsigned int hash = 5381;
        hash = ((hash << 5) + hash) + smiles[i];
        int bit_idx = hash % 1024;
        fp[bit_idx] = 1.0f;
    }
}
