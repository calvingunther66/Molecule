# Flipper Zero Retrosynthesis Coprocessor

This project implements a lightweight machine learning retrosynthesis engine designed to run entirely offline on a Flipper Zero paired with a Video Game Module (Raspberry Pi Pico).

By offloading the heavy neural network matrix operations and Monte Carlo Tree Search (MCTS) to the Pico's ARM processor, the Flipper Zero serves as an interactive GUI—allowing users to draw chemical structures from SMILES codes and predict chemical reaction templates locally without an internet connection.

## Architecture

The system is divided into three main software components:

### 1. Model Training Pipeline (`training/`)
A PyTorch-based pipeline that trains a minimal Multilayer Perceptron (MLP) on a dataset of 50,000 synthetic retrosynthesis examples mapping to 50 common reaction templates. 
- The model is aggressively quantized down to 8-bit integers (`int8`).
- The trained weights and biases are automatically exported into C header files (`nn_weights.h`) for embedded deployment.
- **Usage:** Run `./run_pipeline.sh` to automatically construct the python environment, generate the dummy dataset, train the AI model, and export the C source arrays.

### 2. Video Game Module Backend (`pico_firmware/`)
The native C core running on the Raspberry Pi Pico (Video Game Module). It manages both visual layout physics and the AI inference engine.
- **SMILES Parser & 2D Physics:** Parses a given SMILES string into an internal molecular graph and uses a custom Fruchterman-Reingold Force-Directed Layout algorithm to mechanically "untangle" the molecule into a flat 2D coordinate plane purely within the C environment.
- **Neural Network Inference:** Performs 8-bit quantized matrix multiplications evaluating molecular fingerprints against the exported `nn_weights.h`.
- **Memory-Constrained MCTS:** Executes 500 simulations of Monte Carlo Tree Search, tightly packed into arrays designed specifically to fit within the hard 264KB SRAM limit of the Pico hardware.
- **Usage:** Follows standard `pico-sdk` workflows. Compile via `mkdir build && cd build && cmake .. && make`. Flash the resulting `.uf2` file.

### 3. Flipper Zero App Frontend (`flipper_app/`)
The physical GUI interface running on the Flipper Zero screen.
- Provides an on-screen keyboard (`TextInput`) allowing you to type a SMILES string.
- Communicates continuously over Serial UART (`furi_hal_serial`).
- Streams coordinates back from the Pico to dynamically construct the localized molecule shape line-by-line inside a Flipper `<Canvas>`.
- Issues the analysis trigger to the MCTS logic and presents the optimal predicted route.
- **Usage:** Follows the standard Flipper SDK toolchain framework. Compile using `ufbt`. Move the completed `molecule_retrosynth.fap` file into your Flipper directory.

## Getting Started

1. **Hardware:** Flipper Zero + Flipper Video Game Module.
2. **Flash the Pico:** Copy `pico_firmware/build/retrosynth_coprocessor.uf2` to your Video Game Module while holding down the BOOTSEL button.
3. **Install the Plugin:** Copy `flipper_app/dist/molecule_retrosynth.fap` to your Flipper's `apps/External` folder.
4. **Run:** Connect the module, launch the app from the Flipper GPIO menu, type a SMILES code, and analyze offline chemistry!
