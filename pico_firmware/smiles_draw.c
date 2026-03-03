#include "smiles_draw.h"
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// A stack for handling SMILES branches '()'
#define MAX_STACK 32

void parse_smiles_to_graph(const char *smiles, MolGraph *graph) {
  graph->num_nodes = 0;
  graph->num_edges = 0;

  int branch_stack[MAX_STACK];
  int stack_idx = 0;

  // For handling ring closures (digits 1-9)
  // Map of digit -> node_index that started it
  int ring_starts[10] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1};

  int prev_node = -1;

  size_t len = strlen(smiles);
  for (size_t i = 0; i < len; ++i) {
    char c = smiles[i];

    if (isalpha(c)) {
      // Found an atom
      if (graph->num_nodes >= MAX_DRAW_NODES)
        break;

      // To handle two-letter atoms like 'Cl', 'Br', just take first letter for
      // simplicity right now
      if (i + 1 < len && islower(smiles[i + 1]) && c != 'c' && c != 'n' &&
          c != 'o') {
        i++; // Skip the lowercase letter
      }

      int curr_node = graph->num_nodes++;
      graph->atom_type[curr_node] = toupper(c);

      // Random initial placement in a 64x64 box
      graph->x[curr_node] = (rand() % 64);
      graph->y[curr_node] = (rand() % 64);

      // Connect to previous if not first in sequence
      if (prev_node != -1 && graph->num_edges < MAX_DRAW_EDGES) {
        graph->edge_u[graph->num_edges] = prev_node;
        graph->edge_v[graph->num_edges] = curr_node;
        graph->num_edges++;
      }

      prev_node = curr_node;

    } else if (c == '(') {
      // Start branch: push current node to stack
      if (stack_idx < MAX_STACK && prev_node != -1) {
        branch_stack[stack_idx++] = prev_node;
      }
    } else if (c == ')') {
      // End branch: pop back to the node that started this branch
      if (stack_idx > 0) {
        prev_node = branch_stack[--stack_idx];
      }
    } else if (isdigit(c)) {
      // Ring closure
      int ring_id = c - '0';
      if (ring_starts[ring_id] == -1) {
        // First time seeing this digit, record the current node
        ring_starts[ring_id] = prev_node;
      } else {
        // Second time seeing this digit, close the ring
        if (graph->num_edges < MAX_DRAW_EDGES && prev_node != -1) {
          graph->edge_u[graph->num_edges] = prev_node;
          graph->edge_v[graph->num_edges] = ring_starts[ring_id];
          graph->num_edges++;
        }
        // Reset
        ring_starts[ring_id] = -1;
      }
    }
    // Ignore -, =, #, for basic topology we just need the edges regardless of
    // bond order.
  }
}

// ---------------------------------------------------------
// Fruchterman-Reingold Force-Directed Layout
// ---------------------------------------------------------

#define MAX_ITERATIONS 100
#define AREA_WIDTH 128.0f
#define AREA_HEIGHT 64.0f
#define INITIAL_TEMP 8.0f

static float calc_dist(float dx, float dy) {
  float d = sqrtf(dx * dx + dy * dy);
  return (d < 0.1f) ? 0.1f : d; // Prevent div by 0
}

void calculate_2d_layout(MolGraph *graph) {
  if (graph->num_nodes == 0)
    return;

  // The "ideal" distance between nodes
  float area = AREA_WIDTH * AREA_HEIGHT;
  float k = sqrtf(area / (float)graph->num_nodes) * 0.6f;

  float disp_x[MAX_DRAW_NODES];
  float disp_y[MAX_DRAW_NODES];

  float temp = INITIAL_TEMP;

  for (int iter = 0; iter < MAX_ITERATIONS; ++iter) {
    memset(disp_x, 0, sizeof(disp_x));
    memset(disp_y, 0, sizeof(disp_y));

    // 1. Calculate repulsive forces between ALL nodes
    for (int v = 0; v < graph->num_nodes; ++v) {
      for (int u = 0; u < graph->num_nodes; ++u) {
        if (u != v) {
          float dx = graph->x[v] - graph->x[u];
          float dy = graph->y[v] - graph->y[u];
          float d = calc_dist(dx, dy);

          float rep = (k * k) / d;
          disp_x[v] += (dx / d) * rep;
          disp_y[v] += (dy / d) * rep;
        }
      }
    }

    // 2. Calculate attractive forces between edges (bonds)
    for (int e = 0; e < graph->num_edges; ++e) {
      int u = graph->edge_u[e];
      int v = graph->edge_v[e];

      float dx = graph->x[v] - graph->x[u];
      float dy = graph->y[v] - graph->y[u];
      float d = calc_dist(dx, dy);

      float attr = (d * d) / k;

      float d_x_attr = (dx / d) * attr;
      float d_y_attr = (dy / d) * attr;

      disp_x[v] -= d_x_attr;
      disp_y[v] -= d_y_attr;
      disp_x[u] += d_x_attr;
      disp_y[u] += d_y_attr;
    }

    // 3. Limit displacement by temperature and apply to coordinates
    for (int v = 0; v < graph->num_nodes; ++v) {
      float d = calc_dist(disp_x[v], disp_y[v]);
      float max_disp = (d < temp) ? d : temp;

      graph->x[v] += (disp_x[v] / d) * max_disp;
      graph->y[v] += (disp_y[v] / d) * max_disp;

      // Constrain to positive bounds, slightly padded
      if (graph->x[v] < 5.0f)
        graph->x[v] = 5.0f;
      if (graph->x[v] > AREA_WIDTH - 5.0f)
        graph->x[v] = AREA_WIDTH - 5.0f;
      if (graph->y[v] < 5.0f)
        graph->y[v] = 5.0f;
      if (graph->y[v] > AREA_HEIGHT - 5.0f)
        graph->y[v] = AREA_HEIGHT - 5.0f;
    }

    // 4. Cool temperature
    temp *= 0.95f;
  }

  // Final pass: Re-center coordinates in the 128x64 display
  float min_x = AREA_WIDTH, max_x = 0;
  float min_y = AREA_HEIGHT, max_y = 0;
  for (int v = 0; v < graph->num_nodes; ++v) {
    if (graph->x[v] < min_x)
      min_x = graph->x[v];
    if (graph->x[v] > max_x)
      max_x = graph->x[v];
    if (graph->y[v] < min_y)
      min_y = graph->y[v];
    if (graph->y[v] > max_y)
      max_y = graph->y[v];
  }

  float span_x = max_x - min_x;
  float span_y = max_y - min_y;
  if (span_x < 1.0f)
    span_x = 1.0f;
  if (span_y < 1.0f)
    span_y = 1.0f;

  // Scale up to use most of the screen, leaving a 10px margin
  float scale_x = (AREA_WIDTH - 20) / span_x;
  float scale_y = (AREA_HEIGHT - 20) / span_y;
  float scale = (scale_x < scale_y) ? scale_x : scale_y;

  // Re-center around (64, 32)
  float cx = (max_x + min_x) / 2.0f;
  float cy = (max_y + min_y) / 2.0f;

  for (int v = 0; v < graph->num_nodes; ++v) {
    graph->x[v] = 64.0f + (graph->x[v] - cx) * scale;
    graph->y[v] = 32.0f + (graph->y[v] - cy) * scale;
  }
}

// ---------------------------------------------------------
// Helper for UART transmission state
// ---------------------------------------------------------

static int current_edge_idx = 0;

bool format_next_draw_command(MolGraph *graph, char *out_buf, size_t buf_size) {
  if (current_edge_idx == 0) {
    // First call sends a clear command
    snprintf(out_buf, buf_size, "CLR\n");
    current_edge_idx++;
    return true;
  }

  int edge = current_edge_idx - 1;
  if (edge < graph->num_edges) {
    int u = graph->edge_u[edge];
    int v = graph->edge_v[edge];

    int x1 = (int)(graph->x[u] + 0.5f);
    int y1 = (int)(graph->y[u] + 0.5f);
    int x2 = (int)(graph->x[v] + 0.5f);
    int y2 = (int)(graph->y[v] + 0.5f);

    snprintf(out_buf, buf_size, "L:%d,%d,%d,%d\n", x1, y1, x2, y2);
    current_edge_idx++;
    return true;
  } else if (edge == graph->num_edges) {
    // Done
    snprintf(out_buf, buf_size, "DONE\n");
    current_edge_idx = 0; // Reset for next time
    return false;
  }

  return false;
}
