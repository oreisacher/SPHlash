#include "kernel.h"

#include <cstdio>
#include <cuda_runtime.h>
#include <device_launch_parameters.h>

__global__ void add(int* a) {
    int i = threadIdx.x;
    a[i] += 10;
}

void launchAdd(int* d_a) {
    add<<<1, 5>>>(d_a);

    // Check for launch errors
    cudaError_t err = cudaGetLastError();
    if (err != cudaSuccess) {
        printf("CUDA Error: %s\n", cudaGetErrorString(err));
    }

    cudaDeviceSynchronize();
}