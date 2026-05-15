#include <iostream>
#include <cuda.h>
#include <chrono>

__global__ void matmulKernel(const float* A, const float* B, float* C, int M, int K, int N) {
    int row = blockIdx.y * blockDim.y + threadIdx.y;
    int col = blockIdx.x * blockDim.x + threadIdx.x;
    if (row < M && col < N) {
        float acc = 0.0f;
        for (int kk=0; kk<K; ++kk) acc += A[row*K + kk] * B[kk*N + col];
        C[row*N + col] = acc;
    }
}

int main() {
    static const int M = 256; static const int K = 256; static const int N = 256; static const int BLOCK = 16;

    auto cpu_start = std::chrono::high_resolution_clock::now();

    float* h_A = new float[M*K]; float* h_B = new float[K*N]; float* h_C = new float[M*N];
    for (int i=0;i<M*K;++i) h_A[i] = (float)((i%13)+1);
    for (int i=0;i<K*N;++i) h_B[i] = (float)(((i*7)%17)+1);

    float *d_A,*d_B,*d_C; cudaMalloc(&d_A,M*K*sizeof(float)); cudaMalloc(&d_B,K*N*sizeof(float)); cudaMalloc(&d_C,M*N*sizeof(float));
    cudaMemcpy(d_A,h_A,M*K*sizeof(float),cudaMemcpyHostToDevice); cudaMemcpy(d_B,h_B,K*N*sizeof(float),cudaMemcpyHostToDevice);

    dim3 threads(BLOCK,BLOCK); dim3 grid((N+BLOCK-1)/BLOCK,(M+BLOCK-1)/BLOCK);
    cudaEvent_t start, stop; cudaEventCreate(&start); cudaEventCreate(&stop); cudaEventRecord(start);
    matmulKernel<<<grid, threads>>>(d_A,d_B,d_C,M,K,N); cudaDeviceSynchronize();
    cudaEventRecord(stop); cudaEventSynchronize(stop); float gpu_time_ms=0.0f; cudaEventElapsedTime(&gpu_time_ms,start,stop);

    cudaMemcpy(h_C,d_C,M*N*sizeof(float),cudaMemcpyDeviceToHost);
    double checksum=0.0; for (int i=0;i<M*N;++i) checksum += h_C[i];

    auto cpu_end = std::chrono::high_resolution_clock::now(); double cpu_time_ms=std::chrono::duration<double,std::milli>(cpu_end-cpu_start).count();

    size_t dynamic_bytes = (size_t)(M*K + K*N + M*N) * sizeof(float);
    size_t static_bytes = sizeof(M)+sizeof(K)+sizeof(N)+sizeof(BLOCK);

    std::cout << "Result: " << checksum << std::endl;
    std::cout << "cpu_time_ms,gpu_time_ms,dynamic_mem_bytes,static_mem_bytes\n";
    std::cout << cpu_time_ms << "," << gpu_time_ms << "," << dynamic_bytes << "," << static_bytes << std::endl;

    delete[] h_A; delete[] h_B; delete[] h_C; cudaFree(d_A); cudaFree(d_B); cudaFree(d_C); return 0;
}
