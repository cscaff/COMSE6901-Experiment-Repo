#include <iostream>
#include <cuda.h>
#include <chrono>

__global__ void diffuse(float* grid, float* next, int W, int H) {
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;
    if (x >= W || y >= H) return;
    float center = grid[y*W + x];
    float up = (y>0)?grid[(y-1)*W + x]:center;
    float down = (y<H-1)?grid[(y+1)*W + x]:center;
    float left = (x>0)?grid[y*W + x-1]:center;
    float right = (x<W-1)?grid[y*W + x+1]:center;
    next[y*W + x] = center + 0.1f*(up + down + left + right - 4.0f*center);
}

int main() {
    static const int W = 256; static const int H = 256; static const int STEPS = 50; static const int BLOCK = 16;

    auto cpu_start = std::chrono::high_resolution_clock::now();

    float* h_grid = new float[W*H];
    float* h_next = new float[W*H];
    for (int y = 0; y < H; ++y)
        for (int x = 0; x < W; ++x)
            h_grid[y*W + x] = (float)((x^y) % 31);

    float *d_grid, *d_next; cudaMalloc(&d_grid, W*H*sizeof(float)); cudaMalloc(&d_next, W*H*sizeof(float));
    cudaMemcpy(d_grid, h_grid, W*H*sizeof(float), cudaMemcpyHostToDevice);

    dim3 threads(BLOCK,BLOCK); dim3 gridDim((W+BLOCK-1)/BLOCK,(H+BLOCK-1)/BLOCK);

    cudaEvent_t start, stop; cudaEventCreate(&start); cudaEventCreate(&stop); cudaEventRecord(start);
    for (int s=0; s<STEPS; ++s) {
        diffuse<<<gridDim, threads>>>(d_grid, d_next, W, H);
        cudaDeviceSynchronize();
        float* tmp = d_grid; d_grid = d_next; d_next = tmp;
    }
    cudaEventRecord(stop); cudaEventSynchronize(stop); float gpu_time_ms=0.0f; cudaEventElapsedTime(&gpu_time_ms,start,stop);

    cudaMemcpy(h_grid, d_grid, W*H*sizeof(float), cudaMemcpyDeviceToHost);
    double checksum=0.0; for (int i=0;i<W*H;++i) checksum+=h_grid[i];

    auto cpu_end = std::chrono::high_resolution_clock::now(); double cpu_time_ms=std::chrono::duration<double,std::milli>(cpu_end-cpu_start).count();

    size_t dynamic_bytes = 2ULL * W * H * sizeof(float);
    size_t static_bytes = sizeof(W)+sizeof(H)+sizeof(STEPS)+sizeof(BLOCK);

    std::cout << "Result: " << checksum << std::endl;
    std::cout << "cpu_time_ms,gpu_time_ms,dynamic_mem_bytes,static_mem_bytes\n";
    std::cout << cpu_time_ms << "," << gpu_time_ms << "," << dynamic_bytes << "," << static_bytes << std::endl;

    delete[] h_grid; delete[] h_next; cudaFree(d_grid); cudaFree(d_next); return 0;
}
