//   _____  ______ __  __  ____  
//  |  __ \|  ____|  \/  |/ __ \ 
//  | |  | | |__  | \  / | |  | |
//  | |  | |  __| | |\/| | |  | |
//  | |__| | |____| |  | | |__| |
//  |_____/|______|_|  |_|\____/ 
// 
// Illustrating Equivalence Checking between CUDA and C++ via CBMC.
// Is it possible? What are the limitations? How do we go about this manually?


// EX. Matrix Multiplication 


// Original C++ Code for reference:

// int main() {
//     // === CHANGE N for different variants: 64, 512, 1024 ===
//     static const int N = 1024; // Matrix size N x N
//     auto start_cpu = std::chrono::high_resolution_clock::now();

//     // Allocate matrices
//     int** A = new int*[N];
//     int** B = new int*[N];
//     int** C = new int*[N];
//     for (int i = 0; i < N; ++i) {
//         A[i] = new int[N];
//         B[i] = new int[N];
//         C[i] = new int[N];
//     }

//     // Initialize matrices
//     for (int i = 0; i < N; ++i)
//         for (int j = 0; j < N; ++j) {
//             A[i][j] = i + j;
//             B[i][j] = i - j;
//             C[i][j] = 0;
//         }

//     // Perform matrix multiplication C = A * B
//     for (int i = 0; i < N; ++i)
//         for (int j = 0; j < N; ++j)
//             for (int k = 0; k < N; ++k)
//                 C[i][j] += A[i][k] * B[k][j];

//     // Compute a simple checksum
//     long long checksum = 0;
//     for (int i = 0; i < N; ++i)
//         for (int j = 0; j < N; ++j)
//             checksum += C[i][j];

//     auto end_cpu = std::chrono::high_resolution_clock::now();
//     double cpu_time_ms = std::chrono::duration<double, std::milli>(end_cpu - start_cpu).count();
//     double gpu_time_ms = 0.0;

//     size_t dynamic_mem = 3 * N * N * sizeof(int); // A, B, C
//     size_t static_mem  = sizeof(N);

//     std::cout << checksum << std::endl;
//     std::cout << "cpu_time_ms,gpu_time_ms,dynamic_mem_bytes,static_mem_bytes\n";
//     std::cout << cpu_time_ms << "," << gpu_time_ms << "," << dynamic_mem << "," << static_mem << std::endl;

//     // Free memory
//     for (int i = 0; i < N; ++i) {
//         delete[] A[i];
//         delete[] B[i];
//         delete[] C[i];
//     }
//     delete[] A;
//     delete[] B;
//     delete[] C;

//     return 0;
// }

// Original CUDA Code for reference:

// __global__ void matMulKernel(int* A, int* B, int* C, int N) {
//     int row = blockIdx.y * blockDim.y + threadIdx.y;
//     int col = blockIdx.x * blockDim.x + threadIdx.x;

//     if (row < N && col < N) {
//         int sum = 0;
//         for (int k = 0; k < N; ++k)
//             sum += A[row * N + k] * B[k * N + col];
//         C[row * N + col] = sum;
//     }
// }

// int main() {
//     // === CHANGE N for different variants: 64, 512, 1024 ===
//     static const int N = 1024; 
//     auto cpu_start = std::chrono::high_resolution_clock::now();

//     // Allocate host matrices
//     int* h_A = new int[N*N];
//     int* h_B = new int[N*N];
//     int* h_C = new int[N*N];

//     // Initialize
//     for (int i = 0; i < N; ++i)
//         for (int j = 0; j < N; ++j) {
//             h_A[i*N + j] = i + j;
//             h_B[i*N + j] = i - j;
//             h_C[i*N + j] = 0;
//         }

//     // Allocate device matrices
//     int *d_A, *d_B, *d_C;
//     cudaMalloc(&d_A, N*N*sizeof(int));
//     cudaMalloc(&d_B, N*N*sizeof(int));
//     cudaMalloc(&d_C, N*N*sizeof(int));

//     cudaMemcpy(d_A, h_A, N*N*sizeof(int), cudaMemcpyHostToDevice);
//     cudaMemcpy(d_B, h_B, N*N*sizeof(int), cudaMemcpyHostToDevice);

//     dim3 block(16,16);
//     dim3 grid((N+15)/16,(N+15)/16);

//     // GPU timing
//     cudaEvent_t start, stop;
//     cudaEventCreate(&start);
//     cudaEventCreate(&stop);
//     cudaEventRecord(start);

//     matMulKernel<<<grid, block>>>(d_A, d_B, d_C, N);
//     cudaDeviceSynchronize();

//     cudaEventRecord(stop);
//     cudaEventSynchronize(stop);
//     float gpu_time_ms = 0;
//     cudaEventElapsedTime(&gpu_time_ms, start, stop);

//     cudaMemcpy(h_C, d_C, N*N*sizeof(int), cudaMemcpyDeviceToHost);

//     // Compute checksum
//     long long checksum = 0;
//     for (int i = 0; i < N*N; ++i) checksum += h_C[i];

//     auto cpu_end = std::chrono::high_resolution_clock::now();
//     double cpu_time_ms = std::chrono::duration<double,std::milli>(cpu_end-cpu_start).count();

//     size_t dynamic_mem = 3 * N*N*sizeof(int);
//     size_t static_mem  = sizeof(N);

//     std::cout << checksum << std::endl;
//     std::cout << "cpu_time_ms,gpu_time_ms,dynamic_mem_bytes,static_mem_bytes\n";
//     std::cout << cpu_time_ms << "," << gpu_time_ms << "," << dynamic_mem << "," << static_mem << std::endl;

//     delete[] h_A;
//     delete[] h_B;
//     delete[] h_C;
//     cudaFree(d_A);
//     cudaFree(d_B);
//     cudaFree(d_C);

//     return 0;
// }

// Intuition: C Bounded Model Checking can allow us to prove equality between two functions based on their outputs for all possible inputs.
// CUDA is practically C++ with parallelized kernels. If we assume a sequentialized version of the CUDA kernel, can we prove equivalence?
// This would reduce the problem from checking CUDA vs C++ to translating CUDA into sequential C. 


// We will sequentialize our CUDA code and then compose our programs.
// Then we assert their outputs are the same.
                              
#include <assert.h>
#include <string.h>   // Added for memcpy


// C++ Version
int matrix_mult_cpp() {
    static const int N = 2; // Matrix size N x N
    // We need to adjust N from 1024 to 2 for CBMC tractability. N > 2 fails to unroll.

    // We will need to comment out our chrono usage as library inclusion will explode CBMC.
    // auto start_cpu = std::chrono::high_resolution_clock::now();

    // Allocate matrices
    int** A = new int*[N];
    int** B = new int*[N];
    int** C = new int*[N];
    for (int i = 0; i < N; ++i) {
        A[i] = new int[N];
        B[i] = new int[N];
        C[i] = new int[N];
    }

    // Initialize matrices
    for (int i = 0; i < N; ++i)
        for (int j = 0; j < N; ++j) {
            A[i][j] = i + j;
            B[i][j] = i - j;
            C[i][j] = 0;
        }

    // Perform matrix multiplication C = A * B
    for (int i = 0; i < N; ++i)
        for (int j = 0; j < N; ++j)
            for (int k = 0; k < N; ++k)
                C[i][j] += A[i][k] * B[k][j];

    // Compute a simple checksum
    long long checksum = 0;
    for (int i = 0; i < N; ++i)
        for (int j = 0; j < N; ++j)
            checksum += C[i][j];

    // auto end_cpu = std::chrono::high_resolution_clock::now();
    // double cpu_time_ms = std::chrono::duration<double, std::milli>(end_cpu - start_cpu).count();
    // double gpu_time_ms = 0.0;

    // iostream is C++ code. Including it may explode CBMC.
    // size_t dynamic_mem = 3 * N * N * sizeof(int); // A, B, C
    // size_t static_mem  = sizeof(N);

    // Unecessary for CBMC equivalence checking
    // std::cout << checksum << std::endl;
    // std::cout << "cpu_time_ms,gpu_time_ms,dynamic_mem_bytes,static_mem_bytes\n";
    // std::cout << cpu_time_ms << "," << gpu_time_ms << "," << dynamic_mem << "," << static_mem << std::endl;

    // Free memory - C++
    for (int i = 0; i < N; ++i) {
        delete[] A[i];
        delete[] B[i];
        delete[] C[i];
    }
    delete[] A;
    delete[] B;
    delete[] C;

    // Let's return the checksum to verify our assertion with the CUDA version.
    return checksum;
}

void matMulKernel(int* A, int* B, int* C, int N) {
    // No threads or blocks in our sequential version.
    // int row = blockIdx.y * blockDim.y + threadIdx.y;
    // int col = blockIdx.x * blockDim.x + threadIdx.x;

    // We transform our kernel into a nested loop over all rows and columns.
    // if (row < N && col < N) {
    //     int sum = 0;
    //     for (int k = 0; k < N; ++k)
    //         sum += A[row * N + k] * B[k * N + col];
    //     C[row * N + col] = sum;
    // } 
    // 
    // => => => =>
    for (int row = 0; row < N; ++row)
        for (int col = 0; col < N; ++col) {
            // Core Operation remains the same.
            int sum = 0;
            for (int k = 0; k < N; ++k)
                sum += A[row * N + k] * B[k * N + col];
            C[row * N + col] = sum;        
        }
}


int matrix_mult_cuda() {
    static const int N = 2;
    // We need to adjust N from 1024 to 2 for CBMC tractability. N > 2 fails to unroll.

    // We will need to comment out our chrono usage as library inclusion will explode CBMC.
    // auto cpu_start = std::chrono::high_resolution_clock::now();

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
    // We cannot use cudaMalloc in CBMC.
    // Assuming a regular malloc can cause CBMC to fail w/ the assumption it returns NULL.
    // We use new here to ensure non-null pointers.
    // cudaMalloc(&d_A, N*N*sizeof(int));
    // cudaMalloc(&d_B, N*N*sizeof(int));
    // cudaMalloc(&d_C, N*N*sizeof(int));

    d_A = new int[N*N];
    d_B = new int[N*N];
    d_C = new int[N*N];

    // We cannot use cudaMemcpy in CBMC so let's assume a regular memcpy.
    // cudaMemcpy(d_A, h_A, N*N*sizeof(int), cudaMemcpyHostToDevice);
    // cudaMemcpy(d_B, h_B, N*N*sizeof(int), cudaMemcpyHostToDevice);

    memcpy(d_A, h_A, N*N*sizeof(int));
    memcpy(d_B, h_B, N*N*sizeof(int));

    // We have no concept of blocks and grids in CBMC.
    // dim3 block(16,16);
    // dim3 grid((N+15)/16,(N+15)/16);

    // GPU timing (Unnecessary for CBMC)
    // cudaEvent_t start, stop;
    // cudaEventCreate(&start);
    // cudaEventCreate(&stop);
    // cudaEventRecord(start);

    // matMulKernel<<<grid, block>>>(d_A, d_B, d_C, N);
    // cudaDeviceSynchronize();
    // Sequential Version => => 
    matMulKernel(d_A, d_B, d_C, N);

    // Unecessary CUDA Boilerplate for CBMC
    // cudaEventRecord(stop);
    // cudaEventSynchronize(stop);
    // float gpu_time_ms = 0;
    // cudaEventElapsedTime(&gpu_time_ms, start, stop);

    // We cannot use cudaMemcpy in CBMC so let's assume a regular memcpy.
    // cudaMemcpy(h_C, d_C, N*N*sizeof(int), cudaMemcpyDeviceToHost);

    memcpy(h_C, d_C, N*N*sizeof(int));

    // Compute checksum
    long long checksum = 0;
    for (int i = 0; i < N*N; ++i) checksum += h_C[i];

    // Chronos timing removed for CBMC
    // auto cpu_end = std::chrono::high_resolution_clock::now();
    // double cpu_time_ms = std::chrono::duration<double,std::milli>(cpu_end-cpu_start).count();

    // iostream is C++ code. Including it may explode CBMC.
    // size_t dynamic_mem = 3 * N*N*sizeof(int);
    // size_t static_mem  = sizeof(N);

    // Unecessary for CBMC equivalence checking
    // std::cout << checksum << std::endl;
    // std::cout << "cpu_time_ms,gpu_time_ms,dynamic_mem_bytes,static_mem_bytes\n";
    // std::cout << cpu_time_ms << "," << gpu_time_ms << "," << dynamic_mem << "," << static_mem << std::endl;

    // Free memory - CUDA
    delete[] h_A;
    delete[] h_B;
    delete[] h_C;
    // cudaFree(d_A);
    // cudaFree(d_B);
    // cudaFree(d_C);
    
    // Because we use new instead of cudaMalloc, we must use delete here. 
    delete[] d_A;
    delete[] d_B;
    delete[] d_C;

    return checksum;
}

// ====== RESULT ======
// Command: cbmc CBMC_quality_test.cpp --unwind 5 --trace
// --unwind 5 covers the N*N=4 checksum loop plus one extra iteration for the unwind check.
//
/*
CBMC version 6.8.0 (cbmc-6.8.0) 64-bit arm64 macos
Type-checking CBMC_quality_test
Generating GOTO Program
Adding CPROVER library (arm64)
Removal of function pointers and virtual functions
Generic Property Instrumentation
Starting Bounded Model Checking
Passing problem to propositional reduction
converting SSA
Running propositional reduction
SAT checker: instance is UNSATISFIABLE

** Results:
  [... 405 memory-safety and overflow checks across matrix_mult_cpp,
       matMulKernel, matrix_mult_cuda, and memcpy -- all SUCCESS ...]

CBMC_quality_test.cpp function main
[main.assertion.1] line 379 assertion checksum_cpp == checksum_cuda: SUCCESS

** 0 of 407 failed (1 iterations)
VERIFICATION SUCCESSFUL
*/

int main() {
    // C++ Version
    int checksum_cpp = matrix_mult_cpp();

    // CUDA Version
    int checksum_cuda = matrix_mult_cuda();
    
    // Assert Equivalence
    assert(checksum_cpp == checksum_cuda);

    return 0;
}

//   ______ _             _   _   _       _            
//  |  ____(_)           | | | \ | |     | |           
//  | |__   _ _ __   __ _| | |  \| | ___ | |_ ___  ___ 
//  |  __| | | '_ \ / _` | | | . ` |/ _ \| __/ _ \/ __|
//  | |    | | | | | (_| | | | |\  | (_) | ||  __/\__ \
//  |_|    |_|_| |_|\__,_|_| |_| \_|\___/ \__\___||___/
//
// What can we learn from this short experiment?
// 
// We were able to show equivalence based on output for a trivially small "N" and an expected set of inputs.
// What questions does this bring to the table?
//
// 1. For starters, how do we define equivalence? 
// Can we claim f(x) and g(x) are equal if f(x) = 3 and g(x) = 3 given x = 4?
// I would argue it is not sufficient to make that claim. The given C++ and CUDA code initialize our matrices.
// Thus, we only test it on that set of inputs. Given we know the structure of f(x) and g(x) (That is they are matrix multipliers),
// it is easier to recognize the change of input has no effect on the control flow. Nonetheless, that is no formal claim.
// It is a great start though. Illustrating that f(x) and g(x) emit the exact same outputs as long as they are initalized to
// the same starting input already says a lot about functional equality.
// If we deem it sufficient to prove two functions' output equality given equal inputs, we are already making strides in the right direction.
// We should attempt to show they function on all possible inputs next.
//
// 2. Assuming function output equality is sufficient for overall equality, what constitutes a function?
// It is easy in small kernel examples. However, how would we discern those kernels in an actual program?
// While experimenting with this kernel, asserting equality for intermediate functions in between the start and termination,
// we may encounter poor assertions. That is because the kernels may not progress at the same rate. 
// If we assert at the wrong moment, we may incorrectly invalidate a valid function equality.
// 
// 3. How do we convert our CUDA code to CBMC-ready code?
// Looking at the comments I placed, it is quickly noted that we have to make a lot of changes to our CUDA code
// such that is actually able to be utilized by CBMC. How do we go about translating CUDA code into sequential C++
// and how do we go about ensuring that faithfully represents our original CUDA program?


// The next big step would be to check equality on all inputs. Then, we need to start testing this on other examples.
// If we can manually test enough kernels, can we start to identify key processes that can be automated and generalized?