//
// Created by ruut on 6/4/26.
//

#ifndef SPHLASH_CUDAUTILS_H
#define SPHLASH_CUDAUTILS_H

#include "utils/Logger.h"

#define CUDA_CHECK(call)                            \
    do {                                            \
        cudaError_t err = call;                     \
        if (err != cudaSuccess) {                   \
            LOG_ERROR(cudaGetErrorString(err))      \
        }                                           \
    } while (0)

#define CUDA_KERNEL_CHECK_ACT(label, onError)       \
    do {                                            \
        cudaError_t err = cudaGetLastError();       \
        if (err != cudaSuccess) {                   \
            LOG_ERROR(std::string("[CUDA ERROR] ") + label + " | " + cudaGetErrorString(err));     \
            onError;                                \
        }                                           \
        err = cudaDeviceSynchronize();              \
        if (err != cudaSuccess) {                   \
            LOG_ERROR(std::string("[CUDA RUNTIME ERROR] ") + label + " | " + cudaGetErrorString(err));     \
            onError;                                \
        }                                           \
    } while (0)

#define CUDA_KERNEL_CHECK(label)                    \
    do {                                            \
        cudaError_t err = cudaGetLastError();       \
        if (err != cudaSuccess) {                   \
            LOG_ERROR(std::string("[CUDA ERROR] ") + label + " | " + cudaGetErrorString(err));     \
        }                                           \
        err = cudaDeviceSynchronize();              \
        if (err != cudaSuccess) {                   \
            LOG_ERROR(std::string("[CUDA RUNTIME ERROR] ") + label + " | " + cudaGetErrorString(err));     \
        }                                           \
    } while (0)

#endif //SPHLASH_CUDAUTILS_H
