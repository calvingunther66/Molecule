#ifndef MCTS_H
#define MCTS_H

#include <stdint.h>
#include <stdbool.h>

#define MAX_NODES 350
#define MAX_CHILDREN 50 // Equal to OUTPUT_SIZE of NN

typedef struct {
    int parent;
    int children[MAX_CHILDREN];
    int num_children;
    int visits;
    float value;       // Q-value
    float prior_prob;  // P-value from NN
    
    // The embedded "State" is just the predicted molecular fingerprint
    // To save RAM, we might binarize this to 1024 bits = 128 bytes
    uint8_t fingerprint_bits[128];
    
    bool is_terminal;
} MCTSNode;

void mcts_init();
int mcts_search(const float* initial_fp, int num_simulations);
int mcts_get_best_action(int node_idx);

#endif // MCTS_H
