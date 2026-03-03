import torch
import numpy as np
import os

from train_pico_model import RetrosynthPolicyNet

def quantize_tensor(tensor):
    t = tensor.detach().numpy()
    max_val = np.max(np.abs(t))
    scale = max_val / 127.0
    quantized = np.round(t / scale).astype(np.int8)
    return quantized, scale

def write_c_array(f, array, name, dtype="int8_t"):
    flat = array.flatten()
    f.write(f"const {dtype} {name}[{len(flat)}] = {{\n")
    # Write 16 values per line
    for i in range(0, len(flat), 16):
        chunk = flat[i:i+16]
        f.write("    " + ", ".join(str(x) for x in chunk) + ",\n")
    f.write("};\n\n")

def export_model(model_path="models/policy_net.pth", output_path="../pico_firmware/nn_weights.h"):
    model = RetrosynthPolicyNet()
    model.load_state_dict(torch.load(model_path))
    model.eval()
    
    os.makedirs(os.path.dirname(output_path), exist_ok=True)
    
    fc1_w, fc1_scale = quantize_tensor(model.fc1.weight)
    fc1_b = model.fc1.bias.detach().numpy() # Keep bias as float, it's small!
    
    fc2_w, fc2_scale = quantize_tensor(model.fc2.weight)
    fc2_b = model.fc2.bias.detach().numpy()

    with open(output_path, "w") as f:
        f.write("#ifndef NN_WEIGHTS_H\n")
        f.write("#define NN_WEIGHTS_H\n\n")
        f.write("#include <stdint.h>\n\n")
        
        f.write(f"// Quantization scales\n")
        f.write(f"const float FC1_SCALE = {fc1_scale}f;\n")
        f.write(f"const float FC2_SCALE = {fc2_scale}f;\n\n")
        
        write_c_array(f, fc1_w, "fc1_weight", "int8_t")
        write_c_array(f, fc1_b, "fc1_bias", "float")
        
        write_c_array(f, fc2_w, "fc2_weight", "int8_t")
        write_c_array(f, fc2_b, "fc2_bias", "float")
        
        f.write("#endif // NN_WEIGHTS_H\n")
        
    print(f"Exported C headers to {output_path}")

if __name__ == "__main__":
    export_model()
