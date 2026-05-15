#include <iostream>
#include <cuda.h>
#include <chrono>

__global__ void bubbleSortBlock(float* data, int N) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    int stride = blockDim.x * gridDim.x;

    // Each thread handles multiple elements in its stride
    for (int start = idx; start < N; start += stride) {
        int end = min(start + blockDim.x, N);
        for (int i = start; i < end-1; ++i) {
            for (int j = start; j < end-i+start-1; ++j) {
                if (data[j] > data[j+1]) {
                    float tmp = data[j];
                    data[j] = data[j+1];
                    data[j+1] = tmp;
                }
            }
        }
    }
}

int main() {
    static const int N = 64*64;

    auto cpu_start = std::chrono::high_resolution_clock::now();

    float* h_data = new float[N];
    for (int i = 0; i < N; ++i) h_data[i] = (float)((N - i) % 1000 + 1);

    float* d_data;
    cudaMalloc(&d_data, N*sizeof(float));
    cudaMemcpy(d_data, h_data, N*sizeof(float), cudaMemcpyHostToDevice);

    cudaEvent_t start, stop;
    cudaEventCreate(&start);
    cudaEventCreate(&stop);

    int blockSize = 256;
    int gridSize = (N + blockSize - 1) / blockSize;

    cudaEventRecord(start);
    bubbleSortBlock<<<gridSize, blockSize>>>(d_data, N);
    cudaDeviceSynchronize();
    cudaEventRecord(stop);
    cudaEventSynchronize(stop);

    float gpu_time_ms;
    cudaEventElapsedTime(&gpu_time_ms, start, stop);

    cudaMemcpy(h_data, d_data, N*sizeof(float), cudaMemcpyDeviceToHost);

    float min_val = h_data[0];
    for (int i = 1; i < N; ++i) {
        if (h_data[i] < min_val) min_val = h_data[i];
    }

    auto cpu_end = std::chrono::high_resolution_clock::now();
    double cpu_time_ms = std::chrono::duration<double,std::milli>(cpu_end - cpu_start).count();

    size_t dynamic_mem = N*sizeof(float);
    size_t static_mem  = sizeof(N);

    std::cout << min_val << std::endl;
    std::cout << "cpu_time_ms,gpu_time_ms,dynamic_mem_bytes,static_mem_bytes\n";
    std::cout << cpu_time_ms << "," << gpu_time_ms << "," << dynamic_mem << "," << static_mem << std::endl;

    delete[] h_data;
    cudaFree(d_data);

    return 0;
}