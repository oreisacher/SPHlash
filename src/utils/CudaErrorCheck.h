//
// Created by ruut on 5/9/26.
//

#ifndef SPHLASH_CUDAERRORCHECK_H
#define SPHLASH_CUDAERRORCHECK_H

#define CUDA_CHECK(call) do {                         \
    cudaError_t err = call;                           \
    if (err != cudaSuccess) {                         \
        printf("CUDA error: %s (%s:%d)\n",            \
        cudaGetErrorString(err),                      \
        __FILE__, __LINE__);                          \
        exit(1);                                      \
    }                                                 \
} while(0)

#endif //SPHLASH_CUDAERRORCHECK_H
