#include <iostream>
#include <cuda.h>
#include <cmath>
#include <chrono>

__global__ void fftKernel(double* real, double* imag, int N, int step) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    int m = step << 1;
    if (i >= N / 2) return;
    int j = (i % step) + (i / step) * m;
    double PI = 3.14159265358979323846;
    double ang = -2.0 * PI * (i % step) / m;
    double wr = cos(ang);
    double wi = sin(ang);
    double ur = real[j];
    double ui = imag[j];
    double vr = real[j + step] * wr - imag[j + step] * wi;
    double vi = real[j + step] * wi + imag[j + step] * wr;
    real[j] = ur + vr;
    imag[j] = ui + vi;
    real[j + step] = ur - vr;
    imag[j + step] = ui - vi;
}

int main() {
    static const int N = 1 << 18; // 262,144 samples
    static const double PI = 3.14159265358979323846;
    static const int THREADS = 256;

    auto cpu_start = std::chrono::high_resolution_clock::now();

    double* h_real = new double[N];
    double* h_imag = new double[N];
    for (int i = 0; i < N; ++i) {
        h_real[i] = sin(2 * PI * i / N);
        h_imag[i] = 0.0;
    }

    double *d_real, *d_imag;
    cudaMalloc(&d_real, N * sizeof(double));
    cudaMalloc(&d_imag, N * sizeof(double));

    cudaMemcpy(d_real, h_real, N * sizeof(double), cudaMemcpyHostToDevice);
    cudaMemcpy(d_imag, h_imag, N * sizeof(double), cudaMemcpyHostToDevice);

    // GPU timing
    cudaEvent_t start, stop;
    cudaEventCreate(&start);
    cudaEventCreate(&stop);
    cudaEventRecord(start);

    for (int step = 1; step < N; step <<= 1) {
        int blocks = (N / 2 + THREADS - 1) / THREADS;
        fftKernel<<<blocks, THREADS>>>(d_real, d_imag, N, step);
        cudaDeviceSynchronize();
    }

    cudaEventRecord(stop);
    cudaEventSynchronize(stop);
    float gpu_time_ms = 0;
    cudaEventElapsedTime(&gpu_time_ms, start, stop);

    cudaMemcpy(h_real, d_real, N * sizeof(double), cudaMemcpyDeviceToHost);
    cudaMemcpy(h_imag, d_imag, N * sizeof(double), cudaMemcpyDeviceToHost);

    double checksum = 0.0;
    for (int i = 0; i < N; ++i)
        checksum += sqrt(h_real[i] * h_real[i] + h_imag[i] * h_imag[i]);

    auto cpu_end = std::chrono::high_resolution_clock::now();
    double cpu_time_ms = std::chrono::duration<double, std::milli>(cpu_end - cpu_start).count();

    size_t dynamic_bytes = 2 * N * sizeof(double);
    size_t static_bytes = sizeof(N) + sizeof(PI) + sizeof(THREADS);

    std::cout << checksum << std::endl;
    std::cout << "cpu_time_ms,gpu_time_ms,dynamic_mem_bytes,static_mem_bytes\n";
    std::cout << cpu_time_ms << "," << gpu_time_ms << "," << dynamic_bytes << "," << static_bytes << std::endl;

    delete[] h_real;
    delete[] h_imag;
    cudaFree(d_real);
    cudaFree(d_imag);
    return 0;
}