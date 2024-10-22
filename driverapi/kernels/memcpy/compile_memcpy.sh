#!/bin/bash

# NOTE: THIS FILE IS NOT PART OF CI
# Why? Because it would require having nvcc & ptxas installed, which we don't wan to assume

# Define the list of compute capabilities and corresponding architecture
declare -A compute_capabilities=(
    [5.0]="sm_50"
    [5.2]="sm_52"
    [5.3]="sm_53"
    [6.0]="sm_60"
    [6.1]="sm_61"
    [6.2]="sm_62"
    [7.0]="sm_70"
    [7.2]="sm_72"
    [7.5]="sm_75"
    [8.0]="sm_80"
    [8.6]="sm_86"
    [8.7]="sm_87"
    [8.9]="sm_89"
    [9.0]="sm_90"
)

# Define the CUDA source file and the output directory
source_file="memcpy.cu"
output_dir="output"

# Create the output directory if it does not exist
mkdir -p "$output_dir"

# Loop through each compute capability and run nvcc and ptxas
for capability in "${!compute_capabilities[@]}"; do
    arch="${compute_capabilities[$capability]}"
    ptx_file="$output_dir/memcpy_${arch}.ptx"
    ptxas_file="$output_dir/memcpy_${arch}.cubin"

    # Run nvcc to generate the PTX file
    nvcc -ptx -arch="$arch" "$source_file" -o "$ptx_file"

    # Run ptxas to compile the PTX file to SASS
    ptxas -arch="$arch" "$ptx_file" -o "$ptxas_file"

    echo "Processed compute capability $capability ($arch)"
done

echo "Processing complete."

python3 generate_header.py