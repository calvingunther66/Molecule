import pandas as pd
import numpy as np
from rdkit import Chem
from rdkit.Chem import AllChem
import json
import os

NUM_TEMPLATES = 50
NUM_SAMPLES = 50000

def generate_fingerprint(smiles, radius=2, nBits=1024):
    mol = Chem.MolFromSmiles(smiles)
    if mol is None:
        return np.zeros((nBits,))
    fp = AllChem.GetMorganFingerprintAsBitVect(mol, radius, nBits=nBits)
    arr = np.zeros((0,), dtype=np.int8)
    Chem.DataStructs.ConvertToNumpyArray(fp, arr)
    return arr

def prepare_dataset(output_dir="../data"):
    os.makedirs(output_dir, exist_ok=True)
    
    # We define 50 fake templates 
    templates = {i: f"TEMPLATE_{i}" for i in range(NUM_TEMPLATES)}
    with open(os.path.join(output_dir, "templates.json"), "w") as f:
        json.dump(templates, f, indent=2)
        
    print(f"Saved {len(templates)} templates to {output_dir}/templates.json")
    
    # Generate 50 unique base SMILES (using basic alkyl chains + functional groups)
    # The reality of USPTO-50k is it's 50k reactions mapping to ~50k templates,
    # but practically models predict the top N templates. Aizynthfinder uses top ~50 to 500.
    base_smiles = []
    for i in range(NUM_TEMPLATES):
        # Just create simple variants mapping to a template
        carbon_chain = "C" * ( (i % 8) + 1 )
        if i % 4 == 0: smiles = carbon_chain + "(=O)O"
        elif i % 4 == 1: smiles = carbon_chain + "c1ccccc1"
        elif i % 4 == 2: smiles = carbon_chain + "N(C)C"
        else: smiles = carbon_chain + "O"
        base_smiles.append(smiles)
        
    # Pre-compute target fingerprints for base smiles to save time
    base_fps = [generate_fingerprint(s, nBits=1024) for s in base_smiles]
    
    print(f"Generating {NUM_SAMPLES} samples for {NUM_TEMPLATES} classes...")
    
    # X size will be (NUM_SAMPLES, 1024) (float32 to save RAM during training compared to float64)
    # y size will be (NUM_SAMPLES,)
    X = np.zeros((NUM_SAMPLES, 1024), dtype=np.float32)
    y = np.zeros(NUM_SAMPLES, dtype=np.int64)
    
    np.random.seed(42)
    
    for i in range(NUM_SAMPLES):
        template_idx = i % NUM_TEMPLATES
        base_fp = base_fps[template_idx]
        
        # Add significant random uniform bit flips representing different 
        # R-group attachments to the core reaction center
        noise = np.random.binomial(1, 0.15, 1024) 
        noisy_fp = np.logical_xor(base_fp, noise).astype(np.float32)
        
        X[i] = noisy_fp
        y[i] = template_idx
            
    np.save(os.path.join(output_dir, "X_train.npy"), X)
    np.save(os.path.join(output_dir, "y_train.npy"), y)
    print(f"Saved massive dataset shapes: X={X.shape}, y={y.shape}")

if __name__ == "__main__":
    prepare_dataset()
