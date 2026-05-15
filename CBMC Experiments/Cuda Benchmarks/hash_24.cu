#include <iostream>
#include <cuda.h>
#include <chrono>

__global__ void hashKernel(unsigned int* data, unsigned int* partial, int N, int MOD) {
    extern __shared__ unsigned int sdata[];
    int tid = threadIdx.x;
    int idx = blockIdx.x * blockDim.x + threadIdx.x;

    unsigned int val = 0;
    if (idx < N) {
        val = (data[idx] * (idx + 1)) % MOD;
    }
    sdata[tid] = val;
    __syncthreads();

    // Reduction in shared memory
    for (int s = blockDim.x/2; s > 0; s >>= 1) {
        if (tid < s) sdata[tid] = (sdata[tid] + sdata[tid + s]) % MOD;
        __syncthreads();
    }

    if (tid == 0) partial[blockIdx.x] = sdata[0];
}

int main() {
    static const int N = 1 << 24; // ~16 million
    static const int MOD = 1000000007;
    static const int BLOCK = 256;

    auto cpu_start = std::chrono::high_resolution_clock::now();

    unsigned int* h_data = new unsigned int[N];
    for (int i = 0; i < N; ++i) h_data[i] = i;

    unsigned int *d_data, *d_partial;
    int num_blocks = (N + BLOCK - 1) / BLOCK;
    cudaMalloc(&d_data, N*sizeof(unsigned int));
    cudaMalloc(&d_partial, num_blocks*sizeof(unsigned int));
    cudaMemcpy(d_data, h_data, N*sizeof(unsigned int), cudaMemcpyHostToDevice);

    cudaEvent_t start, stop;
    cudaEventCreate(&start);
    cudaEventCreate(&stop);
    cudaEventRecord(start);

    hashKernel<<<num_blocks, BLOCK, BLOCK*sizeof(unsigned int)>>>(d_data, d_partial, N, MOD);
    cudaDeviceSynchronize();

    cudaEventRecord(stop);
    cudaEventSynchronize(stop);
    float gpu_time_ms = 0;
    cudaEventElapsedTime(&gpu_time_ms, start, stop);

    unsigned int* h_partial = new unsigned int[num_blocks];
    cudaMemcpy(h_partial, d_partial, num_blocks*sizeof(unsigned int), cudaMemcpyDeviceToHost);

    // Final reduction on CPU
    unsigned int hash = 0;
    for (int i = 0; i < num_blocks; ++i) hash = (hash + h_partial[i]) % MOD;

    auto cpu_end = std::chrono::high_resolution_clock::now();
    double cpu_time_ms = std::chrono::duration<double, std::milli>(cpu_end - cpu_start).count();

    size_t dynamic_mem = N*sizeof(unsigned int) + num_blocks*sizeof(unsigned int);
    size_t static_mem  = sizeof(N) + sizeof(MOD) + sizeof(BLOCK);

    std::cout << hash << std::endl;
    std::cout << "cpu_time_ms,gpu_time_ms,dynamic_mem_bytes,static_mem_bytes\n";
    std::cout << cpu_time_ms << "," << gpu_time_ms << "," << dynamic_mem << "," << static_mem << std::endl;

    delete[] h_data;
    delete[] h_partial;
    cudaFree(d_data);
    cudaFree(d_partial);

    return 0;
}