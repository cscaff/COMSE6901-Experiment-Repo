#include <iostream>
#include <cuda.h>
#include <chrono>

__global__ void reduceKernel(const float* data, float* partial, int N) {
    __shared__ float cache[256];
    int tid = blockIdx.x * blockDim.x + threadIdx.x;
    int lane = threadIdx.x;
    float val = 0.0f;
    while (tid < N) { val += data[tid]; tid += blockDim.x * gridDim.x; }
    cache[lane] = val; __syncthreads();
    for (int s = blockDim.x/2; s>0; s >>= 1) { if (lane < s) cache[lane] += cache[lane+s]; __syncthreads(); }
    if (lane==0) partial[blockIdx.x]=cache[0];
}

int main() {
    static const int N = 1 << 20; static const int TPB = 256;

    auto cpu_start = std::chrono::high_resolution_clock::now();

    float* h_data = new float[N];
    for (int i=0;i<N;++i) h_data[i] = (float)((i % 100) + 1);

    float *d_data; cudaMalloc(&d_data, N*sizeof(float)); cudaMemcpy(d_data,h_data,N*sizeof(float),cudaMemcpyHostToDevice);
    int blocks = (N + TPB - 1)/TPB; float* d_partial; cudaMalloc(&d_partial, blocks*sizeof(float));

    cudaEvent_t start, stop; cudaEventCreate(&start); cudaEventCreate(&stop); cudaEventRecord(start);
    reduceKernel<<<blocks, TPB>>>(d_data, d_partial, N); cudaDeviceSynchronize();
    cudaEventRecord(stop); cudaEventSynchronize(stop); float gpu_time_ms=0.0f; cudaEventElapsedTime(&gpu_time_ms,start,stop);

    float* h_partial = new float[blocks]; cudaMemcpy(h_partial,d_partial,blocks*sizeof(float),cudaMemcpyDeviceToHost);
    double sum=0.0; for (int i=0;i<blocks;++i) sum += h_partial[i];

    auto cpu_end = std::chrono::high_resolution_clock::now(); double cpu_time_ms=std::chrono::duration<double,std::milli>(cpu_end-cpu_start).count();

    size_t dynamic_bytes = (size_t)(N + blocks)*sizeof(float);
    size_t static_bytes = sizeof(N)+sizeof(TPB);

    std::cout << "Result: " << sum << std::endl;
    std::cout << "cpu_time_ms,gpu_time_ms,dynamic_mem_bytes,static_mem_bytes\n";
    std::cout << cpu_time_ms << "," << gpu_time_ms << "," << dynamic_bytes << "," << static_bytes << std::endl;

    delete[] h_data; delete[] h_partial; cudaFree(d_data); cudaFree(d_partial); return 0;
}
