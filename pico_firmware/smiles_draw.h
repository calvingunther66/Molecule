#ifndef SMILES_DRAW_H
#define SMILES_DRAW_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define MAX_DRAW_NODES 100
#define MAX_DRAW_EDGES 120

// Representation of a 2D molecule graph
typedef struct {
    float x[MAX_DRAW_NODES];
    float y[MAX_DRAW_NODES];
    char atom_type[MAX_DRAW_NODES]; // 'C', 'O', 'N', etc. (or 0 if invalid)
    int num_nodes;
    
    int edge_u[MAX_DRAW_EDGES];
    int edge_v[MAX_DRAW_EDGES];
    int num_edges;
} MolGraph;

// Parses a simplified SMILES string into a node/edge graph.
// Handles sequences, branches '()', and basic ring closures '1'-'9'
void parse_smiles_to_graph(const char* smiles, MolGraph* graph);

// Runs a force-directed layout (Fruchterman-Reingold) on the graph
// to untangle the nodes into a 2D plane
void calculate_2d_layout(MolGraph* graph);

// Formats the lines into Flipper-ready commands over the provided buffer
// Example: L:x1,y1,x2,y2\n
// Caller repeatedly calls this until it returns false, filling out_buf each time.
bool format_next_draw_command(MolGraph* graph, char* out_buf, size_t buf_size);

#endif // SMILES_DRAW_H
