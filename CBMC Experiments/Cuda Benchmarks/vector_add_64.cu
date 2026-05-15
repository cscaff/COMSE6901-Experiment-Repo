#include <iostream>
#include <cuda.h>
#include <chrono>

__global__ void vectorAdd(const float* A, const float* B, float* C, int N) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < N) {
        C[idx] = A[idx] + B[idx];
    }
}

int main() {
    // ================== SIZE VARIANTS ==================
    static const int N64  = 64 * 1024;
    static const int N512 = 512 * 1024;
    static const int N1024 = 1024 * 1024;

    static const int N = N64; // Choose variant here

    auto cpu_start = std::chrono::high_resolution_clock::now();

    // ------------------ Host Memory --------------------
    float* h_A = new float[N];
    float* h_B = new float[N];
    float* h_C = new float[N];

    // ------------------ Initialization -----------------
    for (int i = 0; i < N; ++i) {
        h_A[i] = (float)(i % 1000);
        h_B[i] = (float)((N - i) % 1000);
    }

    // ------------------ Device Memory ------------------
    float *d_A, *d_B, *d_C;
    cudaMalloc(&d_A, N * sizeof(float));
    cudaMalloc(&d_B, N * sizeof(float));
    cudaMalloc(&d_C, N * sizeof(float));

    cudaMemcpy(d_A, h_A, N*sizeof(float), cudaMemcpyHostToDevice);
    cudaMemcpy(d_B, h_B, N*sizeof(float), cudaMemcpyHostToDevice);

    // ------------------ Kernel Launch ------------------
    int blockSize = 256;
    int gridSize  = (N + blockSize - 1) / blockSize;

    cudaEvent_t start, stop;
    cudaEventCreate(&start);
    cudaEventCreate(&stop);

    cudaEventRecord(start);
    vectorAdd<<<gridSize, blockSize>>>(d_A, d_B, d_C, N);
    cudaDeviceSynchronize();
    cudaEventRecord(stop);
    cudaEventSynchronize(stop);

    float gpu_time_ms;
    cudaEventElapsedTime(&gpu_time_ms, start, stop);

    // ------------------ Copy Back ----------------------
    cudaMemcpy(h_C, d_C, N*sizeof(float), cudaMemcpyDeviceToHost);

    // ------------------ Result Computation -------------
    float sum = 0.0f;
    for (int i = 0; i < N; ++i) {
        sum += h_C[i];
    }

    auto cpu_end = std::chrono::high_resolution_clock::now();
    double cpu_time_ms = std::chrono::duration<double,std::milli>(cpu_end - cpu_start).count();

    size_t dynamic_mem = 3 * N * sizeof(float); // host and device separately could also be added
    size_t static_mem  = sizeof(N64) + sizeof(N512) + sizeof(N1024) + sizeof(N);

    // ------------------ Output -------------------------
    std::cout << sum << std::endl;
    std::cout << "cpu_time_ms,gpu_time_ms,dynamic_mem_bytes,static_mem_bytes\n";
    std::cout << cpu_time_ms << "," << gpu_time_ms << "," << dynamic_mem << "," << static_mem << std::endl;

    // ------------------ Cleanup -----------------------
    delete[] h_A;
    delete[] h_B;
    delete[] h_C;
    cudaFree(d_A);
    cudaFree(d_B);
    cudaFree(d_C);

    return 0;
}