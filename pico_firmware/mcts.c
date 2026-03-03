#include "mcts.h"
#include "nn_inference.h"
#include <string.h>
#include <math.h>
#include <stdlib.h>

#define C_PUCT 1.0f

static MCTSNode tree[MAX_NODES];
static int num_nodes = 0;

void mcts_init() {
    num_nodes = 0;
    memset(tree, 0, sizeof(tree));
}

// Pseudo-Environmental Transition 
// In a full system, this would be a second NN predicting reactant fingerprints.
// Here we apply a bitwise hash based on the template action applied to the parent FP.
static void apply_transition(const uint8_t* parent_fp, int action, uint8_t* child_fp) {
    for (int i = 0; i < 128; ++i) {
        // Deterministic but pseudo-random transition to avoid cycles 
        child_fp[i] = parent_fp[i] ^ (action * 17 + i);
    }
}

// Converts a bit array (uint8_t[128]) to float array (float[1024])
static void bits_to_floats(const uint8_t* bits, float* fp) {
    for (int i = 0; i < 128; ++i) {
        for (int b = 0; b < 8; ++b) {
            fp[i * 8 + b] = (bits[i] & (1 << b)) ? 1.0f : 0.0f;
        }
    }
}

static int create_node(int parent, const uint8_t* fp, float prior, bool is_terminal) {
    if (num_nodes >= MAX_NODES) return -1; // Out of memory
    int idx = num_nodes++;
    tree[idx].parent = parent;
    tree[idx].num_children = 0;
    tree[idx].visits = 0;
    tree[idx].value = 0.0f;
    tree[idx].prior_prob = prior;
    tree[idx].is_terminal = is_terminal;
    memcpy(tree[idx].fingerprint_bits, fp, 128);
    return idx;
}

static void backpropagate(int node_idx, float value) {
    while (node_idx != -1 && node_idx < MAX_NODES) {
        tree[node_idx].visits++;
        tree[node_idx].value += (value - tree[node_idx].value) / tree[node_idx].visits;
        node_idx = tree[node_idx].parent;
    }
}

static int select_child(int node_idx) {
    MCTSNode* node = &tree[node_idx];
    int best_child = -1;
    float best_ucb = -10000.0f;
    
    for (int i = 0; i < node->num_children; ++i) {
        int child_idx = node->children[i];
        MCTSNode* child = &tree[child_idx];
        
        float q = child->visits > 0 ? child->value : 0.0f;
        float u = C_PUCT * child->prior_prob * sqrtf((float)node->visits) / (1.0f + child->visits);
        float ucb = q + u;
        
        if (ucb > best_ucb) {
            best_ucb = ucb;
            best_child = child_idx;
        }
    }
    return best_child;
}

int mcts_search(const float* initial_fp, int num_simulations) {
    mcts_init();
    
    uint8_t root_fp_bits[128] = {0};
    for(int i=0; i<1024; i++) {
        if(initial_fp[i] > 0.0f) root_fp_bits[i/8] |= (1 << (i%8));
    }
    
    int root = create_node(-1, root_fp_bits, 1.0f, false);
    
    for (int sim = 0; sim < num_simulations; ++sim) {
        int curr = root;
        // Selection
        while (tree[curr].num_children > 0 && !tree[curr].is_terminal) {
            curr = select_child(curr);
        }
        
        if (tree[curr].is_terminal || num_nodes >= MAX_NODES) {
            backpropagate(curr, tree[curr].value);
            continue;
        }
        
        // Expansion
        float fp_floats[1024];
        bits_to_floats(tree[curr].fingerprint_bits, fp_floats);
        
        float logits[OUTPUT_SIZE];
        nn_inference(fp_floats, logits);
        
        // Softmax
        float max_logit = logits[0];
        for(int i=1; i<OUTPUT_SIZE; i++) if(logits[i] > max_logit) max_logit = logits[i];
        float sum_exp = 0.0f;
        for(int i=0; i<OUTPUT_SIZE; i++) {
            logits[i] = expf(logits[i] - max_logit);
            sum_exp += logits[i];
        }
        for(int i=0; i<OUTPUT_SIZE; i++) logits[i] /= sum_exp;
        
        // Value heuristic (for testing, we use probability of class 0)
        float value = logits[0]; 
        
        // Expand
        for (int i = 0; i < OUTPUT_SIZE; ++i) {
            uint8_t next_fp[128];
            apply_transition(tree[curr].fingerprint_bits, i, next_fp);
            int child = create_node(curr, next_fp, logits[i], false);
            if (child != -1) {
                tree[curr].children[tree[curr].num_children++] = child;
            }
        }
        
        backpropagate(curr, value);
    }
    
    return root;
}

int mcts_get_best_action(int node_idx) {
    MCTSNode* node = &tree[node_idx];
    int best_action = 0;
    int max_visits = -1;
    
    for (int i = 0; i < node->num_children; ++i) {
        int child_idx = node->children[i];
        if (tree[child_idx].visits > max_visits) {
            max_visits = tree[child_idx].visits;
            best_action = i;
        }
    }
    return best_action;
}
