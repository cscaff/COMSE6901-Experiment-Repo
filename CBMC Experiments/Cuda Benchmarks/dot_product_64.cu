#include <iostream>
#include <cuda.h>
#include <chrono>

__global__ void dotProductKernel(float* A, float* B, float* partial, int n) {
    __shared__ float cache[256];
    int tid = blockIdx.x * blockDim.x + threadIdx.x;
    int cacheIndex = threadIdx.x;
    float temp = 0.0f;
    while (tid < n) {
        temp += A[tid] * B[tid];
        tid += blockDim.x * gridDim.x;
    }
    cache[cacheIndex] = temp;
    __syncthreads();
    for (int i = blockDim.x / 2; i > 0; i >>= 1) {
        if (cacheIndex < i) cache[cacheIndex] += cache[cacheIndex + i];
        __syncthreads();
    }
    if (cacheIndex == 0) partial[blockIdx.x] = cache[0];
}

int main() {
    static const int N = 64 * 64;
    static const int THREADS_PER_BLOCK = 256;

    auto cpu_start = std::chrono::high_resolution_clock::now();

    float* h_A = new float[N];
    float* h_B = new float[N];
    for (int i = 0; i < N; ++i) { h_A[i] = 1.0f; h_B[i] = 2.0f; }

    float *d_A, *d_B, *d_partial;
    int blocks = (N + THREADS_PER_BLOCK - 1) / THREADS_PER_BLOCK;
    cudaMalloc(&d_A, N * sizeof(float));
    cudaMalloc(&d_B, N * sizeof(float));
    cudaMalloc(&d_partial, blocks * sizeof(float));

    cudaMemcpy(d_A, h_A, N * sizeof(float), cudaMemcpyHostToDevice);
    cudaMemcpy(d_B, h_B, N * sizeof(float), cudaMemcpyHostToDevice);

    cudaEvent_t start, stop;
    cudaEventCreate(&start);
    cudaEventCreate(&stop);
    cudaEventRecord(start);

    dotProductKernel<<<blocks, THREADS_PER_BLOCK>>>(d_A, d_B, d_partial, N);
    cudaDeviceSynchronize();

    cudaEventRecord(stop);
    cudaEventSynchronize(stop);
    float gpu_time_ms = 0;
    cudaEventElapsedTime(&gpu_time_ms, start, stop);

    float* h_partial = new float[blocks];
    cudaMemcpy(h_partial, d_partial, blocks * sizeof(float), cudaMemcpyDeviceToHost);
    float result = 0;
    for (int i = 0; i < blocks; ++i) result += h_partial[i];

    auto cpu_end = std::chrono::high_resolution_clock::now();
    double cpu_time_ms = std::chrono::duration<double, std::milli>(cpu_end - cpu_start).count();

    // Memory stats in bytes
    size_t dynamic_bytes = (2 * N * sizeof(float)) + (blocks * sizeof(float));
    size_t static_bytes  = sizeof(N) + sizeof(THREADS_PER_BLOCK);

    std::cout << result << std::endl;
    std::cout << "cpu_time_ms,gpu_time_ms,dynamic_mem_bytes,static_mem_bytes\n";
    std::cout << cpu_time_ms << "," << gpu_time_ms << "," << dynamic_bytes << "," << static_bytes << std::endl;

    delete[] h_A; delete[] h_B; delete[] h_partial;
    cudaFree(d_A); cudaFree(d_B); cudaFree(d_partial);
    return 0;
}