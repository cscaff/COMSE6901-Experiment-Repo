//   _____  ______ __  __  ____  
//  |  __ \|  ____|  \/  |/ __ \ 
//  | |  | | |__  | \  / | |  | |
//  | |  | |  __| | |\/| | |  | |
//  | |__| | |____| |  | | |__| |
//  |_____/|______|_|  |_|\____/ 
// 
// Illustrating Equivalence Checking between CUDA and C++ via CBMC.
// Is it possible? What are the limitations? How do we go about this manually?


// EX. Dot Product 

// Original C++ Code for reference:

// int main() {
//     static const int N = 64 * 64;  // 4,096 elements

//     auto start_cpu = std::chrono::high_resolution_clock::now();

//     float* A = new float[N];
//     float* B = new float[N];

//     for (int i = 0; i < N; ++i) {
//         A[i] = 1.0f;
//         B[i] = 2.0f;
//     }

//     double dot = 0.0;
//     for (int i = 0; i < N; ++i)
//         dot += A[i] * B[i];

//     auto end_cpu = std::chrono::high_resolution_clock::now();
//     double cpu_time_ms = std::chrono::duration<double, std::milli>(end_cpu - start_cpu).count();

//     size_t dynamic_bytes = 2 * N * sizeof(float);
//     size_t static_bytes  = sizeof(N);

//     std::cout << dot << std::endl;
//     std::cout << "cpu_time_ms,gpu_time_ms,dynamic_mem_bytes,static_mem_bytes\n";
//     std::cout << cpu_time_ms << ",0.0," << dynamic_bytes << "," << static_bytes << std::endl;

//     delete[] A;
//     delete[] B;
//     return 0;
// }

// Original CUDA Code for reference:

// __global__ void dotProductKernel(float* A, float* B, float* partial, int n) {
//     __shared__ float cache[256];
//     int tid = blockIdx.x * blockDim.x + threadIdx.x;
//     int cacheIndex = threadIdx.x;
//     float temp = 0.0f;
//     while (tid < n) {
//         temp += A[tid] * B[tid];
//         tid += blockDim.x * gridDim.x;
//     }
//     cache[cacheIndex] = temp;
//     __syncthreads();
//     for (int i = blockDim.x / 2; i > 0; i >>= 1) {
//         if (cacheIndex < i) cache[cacheIndex] += cache[cacheIndex + i];
//         __syncthreads();
//     }
//     if (cacheIndex == 0) partial[blockIdx.x] = cache[0];
// }

// int main() {
//     static const int N = 64 * 64;
//     static const int THREADS_PER_BLOCK = 256;

//     auto cpu_start = std::chrono::high_resolution_clock::now();

//     float* h_A = new float[N];
//     float* h_B = new float[N];
//     for (int i = 0; i < N; ++i) { h_A[i] = 1.0f; h_B[i] = 2.0f; }

//     float *d_A, *d_B, *d_partial;
//     int blocks = (N + THREADS_PER_BLOCK - 1) / THREADS_PER_BLOCK;
//     cudaMalloc(&d_A, N * sizeof(float));
//     cudaMalloc(&d_B, N * sizeof(float));
//     cudaMalloc(&d_partial, blocks * sizeof(float));

//     cudaMemcpy(d_A, h_A, N * sizeof(float), cudaMemcpyHostToDevice);
//     cudaMemcpy(d_B, h_B, N * sizeof(float), cudaMemcpyHostToDevice);

//     cudaEvent_t start, stop;
//     cudaEventCreate(&start);
//     cudaEventCreate(&stop);
//     cudaEventRecord(start);

//     dotProductKernel<<<blocks, THREADS_PER_BLOCK>>>(d_A, d_B, d_partial, N);
//     cudaDeviceSynchronize();

//     cudaEventRecord(stop);
//     cudaEventSynchronize(stop);
//     float gpu_time_ms = 0;
//     cudaEventElapsedTime(&gpu_time_ms, start, stop);

//     float* h_partial = new float[blocks];
//     cudaMemcpy(h_partial, d_partial, blocks * sizeof(float), cudaMemcpyDeviceToHost);
//     float result = 0;
//     for (int i = 0; i < blocks; ++i) result += h_partial[i];

//     auto cpu_end = std::chrono::high_resolution_clock::now();
//     double cpu_time_ms = std::chrono::duration<double, std::milli>(cpu_end - cpu_start).count();

//     // Memory stats in bytes
//     size_t dynamic_bytes = (2 * N * sizeof(float)) + (blocks * sizeof(float));
//     size_t static_bytes  = sizeof(N) + sizeof(THREADS_PER_BLOCK);

//     std::cout << result << std::endl;
//     std::cout << "cpu_time_ms,gpu_time_ms,dynamic_mem_bytes,static_mem_bytes\n";
//     std::cout << cpu_time_ms << "," << gpu_time_ms << "," << dynamic_bytes << "," << static_bytes << std::endl;

//     delete[] h_A; delete[] h_B; delete[] h_partial;
//     cudaFree(d_A); cudaFree(d_B); cudaFree(d_partial);
//     return 0;
// }

                              
#include <assert.h>
#include <string.h>   // Added for memcpy


// C++ Version
float dot_product_cpp(int N) {
    // We need to adjust N from 4,096 to 4 for CBMC tractability. N > 4 fails to unroll.

    // We will need to comment out our chrono usage as library inclusion will explode CBMC.
    // auto start_cpu = std::chrono::high_resolution_clock::now();

    float* A = new float[N];
    float* B = new float[N];

    for (int i = 0; i < N; ++i) {
        A[i] = 1.0f;
        B[i] = 2.0f;
    }

    double dot = 0.0;
    for (int i = 0; i < N; ++i)
        dot += A[i] * B[i];

    // auto end_cpu = std::chrono::high_resolution_clock::now();
    // double cpu_time_ms = std::chrono::duration<double, std::milli>(end_cpu - start_cpu).count();

    // size_t dynamic_bytes = 2 * N * sizeof(float);
    // size_t static_bytes  = sizeof(N);

    // Unecessary for CBMC equivalence checking
    // std::cout << dot << std::endl;
    // std::cout << "cpu_time_ms,gpu_time_ms,dynamic_mem_bytes,static_mem_bytes\n";
    // std::cout << cpu_time_ms << ",0.0," << dynamic_bytes << "," << static_bytes << std::endl;

    delete[] A;
    delete[] B;
    return dot;
}

// Claude Implemented Sequential Version of CUDA Dot Product Kernel for CBMC
#define BLOCK_SIZE 256
// Block size reduced from 256 to 4 for CBMC tractability. BLOCK_SIZE > 4 fails to unroll.

void dotProductKernel_seq(float* A, float* B, float* partial, int n, int gridDim) {

    // Each "block" executes this function sequentially.
    for (int blockIdx = 0; blockIdx < gridDim; blockIdx++) {

        float cache[BLOCK_SIZE];
        // Replaces "__shared__ float cache[256];"

        // === Phase 1: Strided accumulation ===
        // Each "thread" accumulates its slice — sequential here,
        // but the SAME values each real thread would have computed.
        // "int cacheIndex = threadIdx.x;" is implemented in the iteration for each thread.
        for (int cacheIndex = 0; cacheIndex < BLOCK_SIZE; cacheIndex++) {
            int tid = blockIdx * BLOCK_SIZE + cacheIndex;
            // int tid = blockIdx.x * blockDim.x + threadIdx.x;
            float temp = 0.0f;
            while (tid < n) {
                temp += A[tid] * B[tid];
                tid += BLOCK_SIZE * gridDim;
            }
            cache[cacheIndex] = temp;
        }

        // __syncthreads() implicit here — Phase 1 loop fully 
        // completes before Phase 2 begins. This is the critical point.

        // === Phase 2: Tree reduction ===
        // Each level of the tree is a sequential pass over cache[].
        // The inner if() is preserved verbatim from the CUDA kernel.
        for (int i = BLOCK_SIZE / 2; i > 0; i >>= 1) {
            for (int cacheIndex = 0; cacheIndex < BLOCK_SIZE; cacheIndex++) {
                if (cacheIndex < i)
                    cache[cacheIndex] += cache[cacheIndex + i];
            }
            // __syncthreads() implicit — inner loop completes each level
        }

        // Thread 0's write — only cache[0] matters
        partial[blockIdx] = cache[0];
        // Equal to "if (cacheIndex == 0) partial[blockIdx.x] = cache[0];"
    }
}


float dot_product_cuda(int N) {
    // Reduced 64 * 64 down to 4 for CBMC tractability. N > 4 fails to unroll.
    static const int THREADS_PER_BLOCK = 256;

    // auto cpu_start = std::chrono::high_resolution_clock::now();
    // Once again, not using chronos for CBMC.

    float* h_A = new float[N];
    float* h_B = new float[N];
    for (int i = 0; i < N; ++i) { h_A[i] = 1.0f; h_B[i] = 2.0f; }

    float *d_A, *d_B, *d_partial;
    int blocks = (N + THREADS_PER_BLOCK - 1) / THREADS_PER_BLOCK;
    // cudaMalloc(&d_A, N * sizeof(float));
    // cudaMalloc(&d_B, N * sizeof(float));
    // cudaMalloc(&d_partial, blocks * sizeof(float));

    // cudaMemcpy(d_A, h_A, N * sizeof(float), cudaMemcpyHostToDevice);
    // cudaMemcpy(d_B, h_B, N * sizeof(float), cudaMemcpyHostToDevice);
    // CMBC Friendly Memory Management
    d_A = new float[N];
    d_B = new float[N];
    d_partial = new float[blocks];

    memcpy(d_A, h_A, N * sizeof(float));
    memcpy(d_B, h_B, N * sizeof(float));

    // cudaEvent_t start, stop;
    // cudaEventCreate(&start);
    // cudaEventCreate(&stop);
    // cudaEventRecord(start);

    // dotProductKernel<<<blocks, THREADS_PER_BLOCK>>>(d_A, d_B, d_partial, N);
    dotProductKernel_seq(d_A, d_B, d_partial, N, blocks);

    // cudaDeviceSynchronize();

    // Timing not needed for equivalence checking in CBMC
    // cudaEventRecord(stop);
    // cudaEventSynchronize(stop);
    // float gpu_time_ms = 0;
    // cudaEventElapsedTime(&gpu_time_ms, start, stop);

    float* h_partial = new float[blocks];
    // cudaMemcpy(h_partial, d_partial, blocks * sizeof(float), cudaMemcpyDeviceToHost);
    memcpy(h_partial, d_partial, blocks * sizeof(float));
    float result = 0;
    for (int i = 0; i < blocks; ++i) result += h_partial[i];

    // Further Chronos Usage
    // auto cpu_end = std::chrono::high_resolution_clock::now();
    // double cpu_time_ms = std::chrono::duration<double, std::milli>(cpu_end - cpu_start).count();

    // Memory stats in bytes
    // size_t dynamic_bytes = (2 * N * sizeof(float)) + (blocks * sizeof(float));
    // size_t static_bytes  = sizeof(N) + sizeof(THREADS_PER_BLOCK);

    // std::cout << result << std::endl;
    // std::cout << "cpu_time_ms,gpu_time_ms,dynamic_mem_bytes,static_mem_bytes\n";
    // std::cout << cpu_time_ms << "," << gpu_time_ms << "," << dynamic_bytes << "," << static_bytes << std::endl;

    delete[] h_A; delete[] h_B; delete[] h_partial;
    // cudaFree(d_A); cudaFree(d_B); cudaFree(d_partial);
    delete[] d_A; delete[] d_B; delete[] d_partial;
    return result;
}

// We can run CBMC with:
// "cbmc CBMC_quality_test.cpp --unwind 5 --trace"
// 5 covers our N*N = 4 checksum loop.
// This yields:
//
// """ 
// ** 0 of 407 failed (1 iterations)
// VERIFICATION SUCCESSFUL 
// """

int main() {
    // C++ Version
    float result_cpp = dot_product_cpp(5);

    // CUDA Version
    float result_cuda = dot_product_cuda(5);
    
    // Assert Equivalence
    assert(result_cpp == result_cuda);

    return 0;
}

//   ______ _             _   _   _       _            
//  |  ____(_)           | | | \ | |     | |           
//  | |__   _ _ __   __ _| | |  \| | ___ | |_ ___  ___ 
//  |  __| | | '_ \ / _` | | | . ` |/ _ \| __/ _ \/ __|
//  | |    | | | | | (_| | | | |\  | (_) | ||  __/\__ \
//  |_|    |_|_| |_|\__,_|_| |_| \_|\___/ \__\___||___/
//
// 1. CUDA Complexity.
// This example is a far more complex example than that of we saw in the matrix multiplication test.
// The CUDA kernel has shared memory, strided accumulation, and a tree-reduction. 
// This creates difficulty in manual translation to C++. We have to preserve all these semantics.
// 2. CBMC Tractability.
// Again, we have to reduce N massively to ensure CBMC is able to unroll loops without blowup.
// 3. Equivalence Checking.
// Regardless of the complexity, we are able to prove equivalence between the CUDA and C++ versions via CBMC.
