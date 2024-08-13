nvcc -ptx -std=c++11 -arch sm_80 write_float.cu -o write_float.ptx
ptxas -arch sm_80 write_float.ptx -o write_float.cubin
nvdisasm write_float.cubin > write_float.asm