# Development-Phase

## How To Run Inference on C++
1. Download and unzip libtorch
   a. wget https://download.pytorch.org/libtorch/nightly/cpu/libtorch-shared-with-deps-latest.zip
   b. unzip libtorch-shared-with-deps-latest.zip 
2. Clone this repository: git clone  https://github.com/wahyu-adi-n/development-phase.git
3. go to build directory: cd build
4. Prepare data for inference. You can see on .ipynb file
5. Run this on terminal using this format root-path torchscript-model-path input-data-path: ./development-phase ../models/traced_model.pt ../tests/horse.dat 
6. Output the results as inference of models is:
   Loading model...
   Model loaded successfully
   
   49.71% horse
   22.91% dog
   9.15% bird
   8.12% cat
   6.54% deer
   
   DONE