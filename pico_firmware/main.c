#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"

#include "smiles_hash.h"
#include "mcts.h"
#include "nn_inference.h"
#include "smiles_draw.h"

// Flipper UART usually operates at 115200 
#define UART_ID uart0
#define BAUD_RATE 115200

#define UART_TX_PIN 0
#define UART_RX_PIN 1

char input_buffer[256];
int buf_idx = 0;

void process_retro(const char* smiles) {
    printf("RX RETRO: %s\n", smiles);
    
    float fp[1024];
    smiles_to_fingerprint(smiles, fp);
    
    printf("Running MCTS (500 iterations)...\n");
    int root = mcts_search(fp, 500);
    int best_template = mcts_get_best_action(root);
    
    char response[128];
    snprintf(response, sizeof(response), "RES:%d\n", best_template);
    uart_puts(UART_ID, response);
    printf("TX: %s", response);
}

void process_draw(const char* smiles) {
    printf("RX DRAW: %s\n", smiles);
    
    MolGraph graph;
    parse_smiles_to_graph(smiles, &graph);
    calculate_2d_layout(&graph);
    
    char out_buf[128];
    while(format_next_draw_command(&graph, out_buf, sizeof(out_buf))) {
        uart_puts(UART_ID, out_buf);
        sleep_ms(20); // Give Flipper time to process each incoming UART line
    }
}

int main() {
    stdio_init_all();
    
    uart_init(UART_ID, BAUD_RATE);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
    
    printf("Video Game Module Retrosynthesis Coprocessor Active.\n");
    
    while (1) {
        if (uart_is_readable(UART_ID)) {
            char ch = uart_getc(UART_ID);
            if (ch == '\n' || ch == '\r') {
                if (buf_idx > 0) {
                    input_buffer[buf_idx] = '\0';
                    
                    if (strncmp(input_buffer, "DRAW:", 5) == 0) {
                        process_draw(input_buffer + 5);
                    } else if (strncmp(input_buffer, "RETRO:", 6) == 0) {
                        process_retro(input_buffer + 6);
                    }
                    
                    buf_idx = 0;
                }
            } else if (buf_idx < 255) {
                input_buffer[buf_idx++] = ch;
            }
        }
    }
    return 0;
}
