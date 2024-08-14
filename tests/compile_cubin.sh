#! /bin/bash
compile_cubin() {
    if [ -z "$1" ]; then
        echo "Usage: ./compile_cubin.sh <filename_base>. (e.g ./compile_cubin.sh write_float/write_float)"
        return 1
    fi

    filename="$1"
    
    nvcc -ptx -std=c++11 -arch=sm_80 "${filename}.cu" -o "${filename}.ptx"
    ptxas -arch=sm_80 "${filename}.ptx" -o "${filename}.cubin"
    nvdisasm "${filename}.cubin" > "${filename}.asm"
    
    echo "Successfully compiled and disassembled ${filename}.cu"
}

compile_cubin "$1"
