#include <iostream>
#include <cuda.h>
#include <chrono>

__global__ void conv2D(const float* input, float* output, const float* kernel, int width, int height) {
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;

    if (x >= 1 && x < width-1 && y >= 1 && y < height-1) {
        float sum = 0.0f;
        for (int ky = 0; ky < 3; ++ky) {
            for (int kx = 0; kx < 3; ++kx) {
                int ix = x + kx - 1;
                int iy = y + ky - 1;
                sum += input[iy*width + ix] * kernel[ky*3 + kx];
            }
        }
        output[y*width + x] = sum;
    }
}

int main() {
    // ================== SIZE VARIANTS ==================
    static const int WIDTH64  = 64;
    static const int HEIGHT64 = 64;
    static const int WIDTH512  = 512;
    static const int HEIGHT512 = 512;
    static const int WIDTH1024  = 1024;
    static const int HEIGHT1024 = 1024;
    static const int WIDTH2048  = 2048;
    static const int HEIGHT2048 = 2048;

    static const int WIDTH = WIDTH64;
    static const int HEIGHT = HEIGHT64;
    static const int KERNEL_SIZE = 3;

    auto cpu_start = std::chrono::high_resolution_clock::now();

    // ------------------ Host Memory --------------------
    float* h_input  = new float[WIDTH*HEIGHT];
    float* h_output = new float[WIDTH*HEIGHT];
    float h_kernel[KERNEL_SIZE*KERNEL_SIZE] = {0.0f, -1.0f, 0.0f,
                                               -1.0f, 5.0f, -1.0f,
                                                0.0f, -1.0f, 0.0f};

    for (int i = 0; i < WIDTH*HEIGHT; ++i) {
        h_input[i] = (float)(i % 256);
        h_output[i] = 0.0f;
    }

    // ------------------ Device Memory ------------------
    float *d_input, *d_output, *d_kernel;
    cudaMalloc(&d_input, WIDTH*HEIGHT*sizeof(float));
    cudaMalloc(&d_output, WIDTH*HEIGHT*sizeof(float));
    cudaMalloc(&d_kernel, KERNEL_SIZE*KERNEL_SIZE*sizeof(float));

    cudaMemcpy(d_input, h_input, WIDTH*HEIGHT*sizeof(float), cudaMemcpyHostToDevice);
    cudaMemcpy(d_kernel, h_kernel, KERNEL_SIZE*KERNEL_SIZE*sizeof(float), cudaMemcpyHostToDevice);

    // ------------------ Kernel Launch ------------------
    dim3 blockSize(16,16);
    dim3 gridSize((WIDTH+15)/16, (HEIGHT+15)/16);

    cudaEvent_t start, stop;
    cudaEventCreate(&start);
    cudaEventCreate(&stop);

    cudaEventRecord(start);
    conv2D<<<gridSize, blockSize>>>(d_input, d_output, d_kernel, WIDTH, HEIGHT);
    cudaDeviceSynchronize();
    cudaEventRecord(stop);
    cudaEventSynchronize(stop);

    float gpu_time_ms;
    cudaEventElapsedTime(&gpu_time_ms, start, stop);

    cudaMemcpy(h_output, d_output, WIDTH*HEIGHT*sizeof(float), cudaMemcpyDeviceToHost);

    // ------------------ Result Computation -------------
    float total = 0.0f;
    for (int i = 0; i < WIDTH*HEIGHT; ++i) total += h_output[i];

    auto cpu_end = std::chrono::high_resolution_clock::now();
    double cpu_time_ms = std::chrono::duration<double,std::milli>(cpu_end - cpu_start).count();

    size_t dynamic_mem = sizeof(float)*(WIDTH*HEIGHT*2 + KERNEL_SIZE*KERNEL_SIZE);
    size_t static_mem  = sizeof(WIDTH64)+sizeof(HEIGHT64)+sizeof(WIDTH512)+sizeof(HEIGHT512)+
                         sizeof(WIDTH1024)+sizeof(HEIGHT1024)+
                         sizeof(WIDTH2048)+sizeof(HEIGHT2048)+
                         sizeof(KERNEL_SIZE)+sizeof(h_kernel);

    // ------------------ Output -------------------------
    std::cout << total << std::endl;
    std::cout << "cpu_time_ms,gpu_time_ms,dynamic_mem_bytes,static_mem_bytes\n";
    std::cout << cpu_time_ms << "," << gpu_time_ms << "," << dynamic_mem << "," << static_mem << std::endl;

    delete[] h_input;
    delete[] h_output;
    cudaFree(d_input);
    cudaFree(d_output);
    cudaFree(d_kernel);

    return 0;
}