#!/bin/bash
set -e

echo "Creating python virtual environment..."
python3 -m venv venv
source venv/bin/activate

echo "Installing requirements..."
pip install -r requirements.txt

echo "Running Data Prep..."
python data_prep.py

echo "Running Model Training..."
python train_pico_model.py

echo "Exporting Model to C Header..."
python export_to_c.py

echo "Pipeline complete!"
