#include <iostream>
#include <cuda.h>
#include <cmath>
#include <chrono>

__global__ void fluidStepKernel(double* u, double* v, double* u_new, double* v_new, int N, double diff, double dt) {
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;

    if (x < 1 || x >= N-1 || y < 1 || y >= N-1) return;

    int idx = y * N + x;
    u_new[idx] = u[idx] + diff * dt * (
        u[(y-1)*N + x] + u[(y+1)*N + x] + u[y*N + (x-1)] + u[y*N + (x+1)] - 4*u[idx]
    );
    v_new[idx] = v[idx] + diff * dt * (
        v[(y-1)*N + x] + v[(y+1)*N + x] + v[y*N + (x-1)] + v[y*N + (x+1)] - 4*v[idx]
    );
}

int main() {
    static const int N = 512;
    static const int STEPS = 10;
    static const double DIFF = 0.1;
    static const double DT = 0.1;
    static const int BLOCK = 16;

    auto cpu_start = std::chrono::high_resolution_clock::now();

    double* h_u = new double[N*N];
    double* h_v = new double[N*N];
    for (int i = 0; i < N*N; ++i) { h_u[i] = 1.0; h_v[i] = 0.0; }

    double *d_u, *d_v, *d_u_new, *d_v_new;
    cudaMalloc(&d_u, N*N*sizeof(double));
    cudaMalloc(&d_v, N*N*sizeof(double));
    cudaMalloc(&d_u_new, N*N*sizeof(double));
    cudaMalloc(&d_v_new, N*N*sizeof(double));

    cudaMemcpy(d_u, h_u, N*N*sizeof(double), cudaMemcpyHostToDevice);
    cudaMemcpy(d_v, h_v, N*N*sizeof(double), cudaMemcpyHostToDevice);

    dim3 threads(BLOCK, BLOCK);
    dim3 blocks((N+BLOCK-1)/BLOCK, (N+BLOCK-1)/BLOCK);

    cudaEvent_t start, stop;
    cudaEventCreate(&start);
    cudaEventCreate(&stop);
    cudaEventRecord(start);

    for (int t = 0; t < STEPS; ++t) {
        fluidStepKernel<<<blocks, threads>>>(d_u, d_v, d_u_new, d_v_new, N, DIFF, DT);
        cudaDeviceSynchronize();
        std::swap(d_u, d_u_new);
        std::swap(d_v, d_v_new);
    }

    cudaEventRecord(stop);
    cudaEventSynchronize(stop);
    float gpu_time_ms = 0;
    cudaEventElapsedTime(&gpu_time_ms, start, stop);

    cudaMemcpy(h_u, d_u, N*N*sizeof(double), cudaMemcpyDeviceToHost);
    cudaMemcpy(h_v, d_v, N*N*sizeof(double), cudaMemcpyDeviceToHost);

    double checksum = 0.0;
    for (int i = 0; i < N*N; ++i) checksum += std::sqrt(h_u[i]*h_u[i] + h_v[i]*h_v[i]);

    auto cpu_end = std::chrono::high_resolution_clock::now();
    double cpu_time_ms = std::chrono::duration<double, std::milli>(cpu_end - cpu_start).count();

    size_t dynamic_mem = 4*N*N*sizeof(double);
    size_t static_mem  = sizeof(N)+sizeof(STEPS)+sizeof(DIFF)+sizeof(DT)+sizeof(BLOCK);

    std::cout << checksum << std::endl;
    std::cout << "cpu_time_ms,gpu_time_ms,dynamic_mem_bytes,static_mem_bytes\n";
    std::cout << cpu_time_ms << "," << gpu_time_ms << "," << dynamic_mem << "," << static_mem << std::endl;

    delete[] h_u;
    delete[] h_v;
    cudaFree(d_u); cudaFree(d_v); cudaFree(d_u_new); cudaFree(d_v_new);

    return 0;
}