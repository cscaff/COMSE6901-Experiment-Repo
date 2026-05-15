#include <iostream>
#include <cuda.h>
#include <chrono>

__global__ void matMulKernel(int* A, int* B, int* C, int N) {
    int row = blockIdx.y * blockDim.y + threadIdx.y;
    int col = blockIdx.x * blockDim.x + threadIdx.x;

    if (row < N && col < N) {
        int sum = 0;
        for (int k = 0; k < N; ++k)
            sum += A[row * N + k] * B[k * N + col];
        C[row * N + col] = sum;
    }
}

int main() {
    // === CHANGE N for different variants: 64, 512, 1024 ===
    static const int N = 64; 
    auto cpu_start = std::chrono::high_resolution_clock::now();

    // Allocate host matrices
    int* h_A = new int[N*N];
    int* h_B = new int[N*N];
    int* h_C = new int[N*N];

    // Initialize
    for (int i = 0; i < N; ++i)
        for (int j = 0; j < N; ++j) {
            h_A[i*N + j] = i + j;
            h_B[i*N + j] = i - j;
            h_C[i*N + j] = 0;
        }

    // Allocate device matrices
    int *d_A, *d_B, *d_C;
    cudaMalloc(&d_A, N*N*sizeof(int));
    cudaMalloc(&d_B, N*N*sizeof(int));
    cudaMalloc(&d_C, N*N*sizeof(int));

    cudaMemcpy(d_A, h_A, N*N*sizeof(int), cudaMemcpyHostToDevice);
    cudaMemcpy(d_B, h_B, N*N*sizeof(int), cudaMemcpyHostToDevice);

    dim3 block(16,16);
    dim3 grid((N+15)/16,(N+15)/16);

    // GPU timing
    cudaEvent_t start, stop;
    cudaEventCreate(&start);
    cudaEventCreate(&stop);
    cudaEventRecord(start);

    matMulKernel<<<grid, block>>>(d_A, d_B, d_C, N);
    cudaDeviceSynchronize();

    cudaEventRecord(stop);
    cudaEventSynchronize(stop);
    float gpu_time_ms = 0;
    cudaEventElapsedTime(&gpu_time_ms, start, stop);

    cudaMemcpy(h_C, d_C, N*N*sizeof(int), cudaMemcpyDeviceToHost);

    // Compute checksum
    long long checksum = 0;
    for (int i = 0; i < N*N; ++i) checksum += h_C[i];

    auto cpu_end = std::chrono::high_resolution_clock::now();
    double cpu_time_ms = std::chrono::duration<double,std::milli>(cpu_end-cpu_start).count();

    size_t dynamic_mem = 3 * N*N*sizeof(int);
    size_t static_mem  = sizeof(N);

    std::cout << checksum << std::endl;
    std::cout << "cpu_time_ms,gpu_time_ms,dynamic_mem_bytes,static_mem_bytes\n";
    std::cout << cpu_time_ms << "," << gpu_time_ms << "," << dynamic_mem << "," << static_mem << std::endl;

    delete[] h_A;
    delete[] h_B;
    delete[] h_C;
    cudaFree(d_A);
    cudaFree(d_B);
    cudaFree(d_C);

    return 0;
}