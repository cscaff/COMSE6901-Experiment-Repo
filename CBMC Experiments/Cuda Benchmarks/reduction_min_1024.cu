#include <iostream>
#include <cuda.h>
#include <chrono>

__global__ void reduceMinKernel(float* data, float* block_mins, int N) {
    extern __shared__ float sdata[];
    int tid = threadIdx.x;
    int i = blockIdx.x * blockDim.x + threadIdx.x;

    sdata[tid] = (i < N) ? data[i] : 1e30f;
    __syncthreads();

    for (int s = blockDim.x/2; s > 0; s >>= 1) {
        if (tid < s) {
            if (sdata[tid + s] < sdata[tid]) sdata[tid] = sdata[tid + s];
        }
        __syncthreads();
    }

    if (tid == 0) block_mins[blockIdx.x] = sdata[0];
}

int main() {
    static const int N = 1024*1024;
    auto cpu_start = std::chrono::high_resolution_clock::now();

    float* h_data = new float[N];
    for (int i = 0; i < N; ++i) h_data[i] = (float)((N - i) % 1000 + 1);

    float *d_data, *d_block_mins;
    int blockSize = 256;
    int gridSize = (N + blockSize - 1) / blockSize;

    cudaMalloc(&d_data, N*sizeof(float));
    cudaMalloc(&d_block_mins, gridSize*sizeof(float));
    cudaMemcpy(d_data, h_data, N*sizeof(float), cudaMemcpyHostToDevice);

    cudaEvent_t start, stop;
    cudaEventCreate(&start);
    cudaEventCreate(&stop);
    cudaEventRecord(start);

    reduceMinKernel<<<gridSize, blockSize, blockSize*sizeof(float)>>>(d_data, d_block_mins, N);
    cudaDeviceSynchronize();

    cudaEventRecord(stop);
    cudaEventSynchronize(stop);
    float gpu_time_ms;
    cudaEventElapsedTime(&gpu_time_ms, start, stop);

    float* h_block_mins = new float[gridSize];
    cudaMemcpy(h_block_mins, d_block_mins, gridSize*sizeof(float), cudaMemcpyDeviceToHost);

    float min_val = h_block_mins[0];
    for (int i = 1; i < gridSize; ++i) if (h_block_mins[i] < min_val) min_val = h_block_mins[i];

    auto cpu_end = std::chrono::high_resolution_clock::now();
    double cpu_time_ms = std::chrono::duration<double,std::milli>(cpu_end - cpu_start).count();

    size_t dynamic_mem = N*sizeof(float) + gridSize*sizeof(float);
    size_t static_mem  = sizeof(N);

    std::cout << min_val << std::endl;
    std::cout << "cpu_time_ms,gpu_time_ms,dynamic_mem_bytes,static_mem_bytes\n";
    std::cout << cpu_time_ms << "," << gpu_time_ms << "," << dynamic_mem << "," << static_mem << std::endl;

    delete[] h_data;
    delete[] h_block_mins;
    cudaFree(d_data);
    cudaFree(d_block_mins);

    return 0;
}