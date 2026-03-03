#ifndef SMILES_HASH_H
#define SMILES_HASH_H

#include <stdint.h>

// On a microcontroller, a true Morgan Fingerprint requires full Graph Isomorphism.
// Here we use an extremely fast character n-gram hashing algorithm to approximate
// 1024-bit fingerprints locally directly from SMILES strings!
void smiles_to_fingerprint(const char* smiles, float* fp);

#endif // SMILES_HASH_H
