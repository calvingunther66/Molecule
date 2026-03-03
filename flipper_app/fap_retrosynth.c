#include <furi.h>
#include <furi_hal.h>
#include <furi_hal_serial.h>
#include <furi_hal_serial_control.h>
#include <gui/gui.h>
#include <gui/view_dispatcher.h>
#include <gui/modules/text_input.h>
#include <gui/modules/widget.h>
#include <string.h>
#include <stdlib.h>

#define BAUD_RATE 115200
#define MAX_DRAW_LINES 120

typedef enum {
    MolRetroViewTextInput,
    MolRetroViewCanvas,
    MolRetroViewWidget,
} MolRetroView;

typedef struct {
    int x1, y1, x2, y2;
} DrawLine;

typedef struct {
    Gui* gui;
    ViewDispatcher* view_dispatcher;
    TextInput* text_input;
    View* canvas_view;
    Widget* widget;
    
    FuriHalSerialHandle* serial_handle;
    
    char smiles_buffer[256];
    char result_buffer[128];
    char rx_line_buffer[64];
    int rx_line_idx;
    
    DrawLine lines[MAX_DRAW_LINES];
    int num_lines;
    
    bool is_drawing;
    bool is_analyzing;
} MolRetroApp;

static void parse_uart_line(MolRetroApp* app, const char* line) {
    if (app->is_drawing) {
        if (strncmp(line, "L:", 2) == 0) {
            if (app->num_lines < MAX_DRAW_LINES) {
                int x1, y1, x2, y2;
                if (sscanf(line + 2, "%d,%d,%d,%d", &x1, &y1, &x2, &y2) == 4) {
                    app->lines[app->num_lines].x1 = x1;
                    app->lines[app->num_lines].y1 = y1;
                    app->lines[app->num_lines].x2 = x2;
                    app->lines[app->num_lines].y2 = y2;
                    app->num_lines++;
                }
            }
        } else if (strncmp(line, "DONE", 4) == 0) {
            app->is_drawing = false;
        } else if (strncmp(line, "CLR", 3) == 0) {
            app->num_lines = 0;
        }
    } else if (app->is_analyzing) {
        if (strncmp(line, "RES:", 4) == 0) {
            strncpy(app->result_buffer, line + 4, sizeof(app->result_buffer) - 1);
            app->is_analyzing = false;
        }
    }
}

static void uart_on_irq_cb(FuriHalSerialHandle* handle, FuriHalSerialRxEvent event, void* context) {
    if(event == FuriHalSerialRxEventData) {
        MolRetroApp* app = context;
        while(furi_hal_serial_async_rx_available(handle)) {
            uint8_t data = furi_hal_serial_async_rx(handle);
            if (data == '\n') {
                app->rx_line_buffer[app->rx_line_idx] = '\0';
                parse_uart_line(app, app->rx_line_buffer);
                app->rx_line_idx = 0;
            } else if (data != '\r' && app->rx_line_idx < (int)sizeof(app->rx_line_buffer) - 1) {
                app->rx_line_buffer[app->rx_line_idx++] = data;
            }
        }
    }
}

static void canvas_draw_cb(Canvas* canvas, void* context) {
    MolRetroApp* app = context;
    canvas_clear(canvas);
    
    if (app->is_drawing) {
        canvas_set_font(canvas, FontSecondary);
        canvas_draw_str_aligned(canvas, 64, 32, AlignCenter, AlignCenter, "Generating 2D Layout...");
    } else {
        // Draw the molecule lines
        for (int i = 0; i < app->num_lines; ++i) {
            canvas_draw_line(
                canvas, 
                app->lines[i].x1, app->lines[i].y1, 
                app->lines[i].x2, app->lines[i].y2
            );
        }
        
        // Instruction Box
        canvas_set_color(canvas, ColorWhite);
        canvas_draw_box(canvas, 0, 0, 128, 12);
        canvas_set_color(canvas, ColorBlack);
        canvas_set_font(canvas, FontSecondary);
        canvas_draw_str(canvas, 2, 10, "Press OK for Retro ML");
    }
}

static bool canvas_input_cb(InputEvent* event, void* context) {
    MolRetroApp* app = context;
    if(event->type == InputTypeShort && event->key == InputKeyOk) {
        if (!app->is_drawing && app->num_lines > 0) {
            // Trigger Retrosynthesis
            app->is_analyzing = true;
            memset(app->result_buffer, 0, sizeof(app->result_buffer));
            
            // Send RETRO: command
            char cmd[300];
            snprintf(cmd, sizeof(cmd), "RETRO:%s\n", app->smiles_buffer);
            furi_hal_serial_tx(app->serial_handle, (uint8_t*)cmd, strlen(cmd));
            
            widget_reset(app->widget);
            widget_add_string_element(app->widget, 2, 10, AlignLeft, AlignTop, FontSecondary, "Analyzing SMILES:");
            widget_add_string_element(app->widget, 2, 25, AlignLeft, AlignTop, FontSecondary, app->smiles_buffer);
            widget_add_string_multiline_element(app->widget, 2, 45, AlignLeft, AlignTop, FontPrimary, "WAITING FOR PICO...");

            view_dispatcher_switch_to_view(app->view_dispatcher, MolRetroViewWidget);
            return true;
        }
    }
    return false;
}

static void text_input_callback(void* context) {
    MolRetroApp* app = context;
    
    // Switch to Canvas View
    app->is_drawing = true;
    app->num_lines = 0;
    view_dispatcher_switch_to_view(app->view_dispatcher, MolRetroViewCanvas);
    
    // Request Pico to draw the SMILES 
    char cmd[300];
    snprintf(cmd, sizeof(cmd), "DRAW:%s\n", app->smiles_buffer);
    furi_hal_serial_tx(app->serial_handle, (uint8_t*)cmd, strlen(cmd));
}

static uint32_t text_input_prev_callback(void* context) {
    UNUSED(context);
    return VIEW_NONE;
}

static uint32_t canvas_prev_callback(void* context) {
    UNUSED(context);
    return MolRetroViewTextInput;
}

static uint32_t widget_prev_callback(void* context) {
    UNUSED(context);
    return MolRetroViewCanvas;
}

static void app_tick(void* context) {
    MolRetroApp* app = context;
    if (app->is_analyzing == false && strlen(app->result_buffer) > 0) {
        widget_reset(app->widget);
        widget_add_string_element(app->widget, 2, 10, AlignLeft, AlignTop, FontSecondary, "Neural Network Results:");
        widget_add_string_element(app->widget, 2, 25, AlignLeft, AlignTop, FontSecondary, "Template Class Predicted:");
        widget_add_string_multiline_element(app->widget, 2, 45, AlignLeft, AlignTop, FontPrimary, app->result_buffer);
        // Clear buffer so we only update the widget once
        memset(app->result_buffer, 0, sizeof(app->result_buffer));
    }
}

int32_t mol_retro_app(void* p) {
    UNUSED(p);
    MolRetroApp* app = malloc(sizeof(MolRetroApp));
    memset(app, 0, sizeof(MolRetroApp));

    app->gui = furi_record_open(RECORD_GUI);
    app->view_dispatcher = view_dispatcher_alloc();
    view_dispatcher_attach_to_gui(app->view_dispatcher, app->gui, ViewDispatcherTypeFullscreen);
    
    view_dispatcher_set_tick_event_callback(app->view_dispatcher, app_tick, 500);
    view_dispatcher_set_event_callback_context(app->view_dispatcher, app);
    
    // 1. Text Input View
    app->text_input = text_input_alloc();
    text_input_set_header_text(app->text_input, "Enter SMILES String:");
    text_input_set_result_callback(
        app->text_input, text_input_callback, app, app->smiles_buffer, sizeof(app->smiles_buffer), true);
    view_set_previous_callback(text_input_get_view(app->text_input), text_input_prev_callback);
    view_dispatcher_add_view(app->view_dispatcher, MolRetroViewTextInput, text_input_get_view(app->text_input));

    // 2. 2D Molecule Canvas View
    app->canvas_view = view_alloc();
    view_set_context(app->canvas_view, app);
    view_set_draw_callback(app->canvas_view, canvas_draw_cb);
    view_set_input_callback(app->canvas_view, canvas_input_cb);
    view_set_previous_callback(app->canvas_view, canvas_prev_callback);
    view_dispatcher_add_view(app->view_dispatcher, MolRetroViewCanvas, app->canvas_view);

    // 3. Results Widget View
    app->widget = widget_alloc();
    view_set_previous_callback(widget_get_view(app->widget), widget_prev_callback);
    view_dispatcher_add_view(app->view_dispatcher, MolRetroViewWidget, widget_get_view(app->widget));

    // Modern FuriHalSerial initialization
    app->serial_handle = furi_hal_serial_control_acquire(FuriHalSerialIdUsart);
    furi_hal_serial_init(app->serial_handle, BAUD_RATE);
    furi_hal_serial_async_rx_start(app->serial_handle, uart_on_irq_cb, app, false);

    // Initial View
    view_dispatcher_switch_to_view(app->view_dispatcher, MolRetroViewTextInput);
    view_dispatcher_run(app->view_dispatcher);

    // Cleanup
    furi_hal_serial_async_rx_stop(app->serial_handle);
    furi_hal_serial_deinit(app->serial_handle);
    furi_hal_serial_control_release(app->serial_handle);

    view_dispatcher_remove_view(app->view_dispatcher, MolRetroViewTextInput);
    view_dispatcher_remove_view(app->view_dispatcher, MolRetroViewCanvas);
    view_dispatcher_remove_view(app->view_dispatcher, MolRetroViewWidget);
    text_input_free(app->text_input);
    view_free(app->canvas_view);
    widget_free(app->widget);
    view_dispatcher_free(app->view_dispatcher);
    furi_record_close(RECORD_GUI);
    free(app);

    return 0;
}
