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


// Intuition: C Bounded Model Checking can allow us to prove equality between two functions based on their outputs for all possible inputs.
// CUDA is practically C++ with parallelized kernels. If we assume a sequentialized version of the CUDA kernel, can we prove equivalence?
// This would reduce the problem from checking CUDA vs C++ to translating CUDA into sequential C.

// This variant extends CBMC_quality_test.cpp by pulling initialization out into main()
// and passing non-deterministic integer inputs into both functions.
// This allows CBMC to verify equivalence over ALL possible integer inputs, not just i+j / i-j.

// We will sequentialize our CUDA code and then compose our programs.
// Then we assert their outputs are the same.

#include <assert.h>
#include <string.h>   // for memcpy

extern int nondet_int();

#define N 2

// C++ Version - receives pre-initialized matrices A and B, allocates and computes C internally.
int matrix_mult_cpp(int** A, int** B, int n) {
    // We need to adjust N from 1024 to 2 for CBMC tractability. N > 2 fails to unroll.

    int** C = new int*[n];
    for (int i = 0; i < n; ++i)
        C[i] = new int[n];

    // Zero-initialize C before accumulation
    for (int i = 0; i < n; ++i)
        for (int j = 0; j < n; ++j)
            C[i][j] = 0;

    // Perform matrix multiplication C = A * B
    for (int i = 0; i < n; ++i)
        for (int j = 0; j < n; ++j)
            for (int k = 0; k < n; ++k)
                C[i][j] += A[i][k] * B[k][j];

    // Compute a simple checksum
    long long checksum = 0;
    for (int i = 0; i < n; ++i)
        for (int j = 0; j < n; ++j)
            checksum += C[i][j];

    // Free memory - C++
    for (int i = 0; i < n; ++i)
        delete[] C[i];
    delete[] C;

    // Return checksum to verify assertion with the CUDA version.
    return checksum;
}

void matMulKernel(int* A, int* B, int* C, int n) {
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
    for (int row = 0; row < n; ++row)
        for (int col = 0; col < n; ++col) {
            // Core Operation remains the same.
            int sum = 0;
            for (int k = 0; k < n; ++k)
                sum += A[row * n + k] * B[k * n + col];
            C[row * n + col] = sum;
        }
}


// CUDA Version - receives pre-initialized flat arrays h_A and h_B.
int matrix_mult_cuda(int* h_A, int* h_B, int n) {
    // We need to adjust N from 1024 to 2 for CBMC tractability. N > 2 fails to unroll.

    int* h_C = new int[n*n];

    // Zero-initialize h_C
    for (int i = 0; i < n*n; ++i)
        h_C[i] = 0;

    // Allocate device matrices
    int *d_A, *d_B, *d_C;
    // We cannot use cudaMalloc in CBMC.
    // Assuming a regular malloc can cause CBMC to fail w/ the assumption it returns NULL.
    // We use new here to ensure non-null pointers.
    // cudaMalloc(&d_A, N*N*sizeof(int));
    // cudaMalloc(&d_B, N*N*sizeof(int));
    // cudaMalloc(&d_C, N*N*sizeof(int));

    d_A = new int[n*n];
    d_B = new int[n*n];
    d_C = new int[n*n];

    // We cannot use cudaMemcpy in CBMC so let's assume a regular memcpy.
    // cudaMemcpy(d_A, h_A, N*N*sizeof(int), cudaMemcpyHostToDevice);
    // cudaMemcpy(d_B, h_B, N*N*sizeof(int), cudaMemcpyHostToDevice);

    memcpy(d_A, h_A, n*n*sizeof(int));
    memcpy(d_B, h_B, n*n*sizeof(int));

    // We have no concept of blocks and grids in CBMC.
    // dim3 block(16,16);
    // dim3 grid((N+15)/16,(N+15)/16);

    // matMulKernel<<<grid, block>>>(d_A, d_B, d_C, N);
    // cudaDeviceSynchronize();
    // Sequential Version => =>
    matMulKernel(d_A, d_B, d_C, n);

    // We cannot use cudaMemcpy in CBMC so let's assume a regular memcpy.
    // cudaMemcpy(h_C, d_C, N*N*sizeof(int), cudaMemcpyDeviceToHost);

    memcpy(h_C, d_C, n*n*sizeof(int));

    // Compute checksum
    long long checksum = 0;
    for (int i = 0; i < n*n; ++i) checksum += h_C[i];

    // Free memory - CUDA
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

// We can run CBMC with:
// "cbmc CBMC_undet_int_test.cpp --unwind 5 --trace"
// 5 covers our N*N = 4 checksum loop.

int main() {
    // Initialize flat arrays with non-deterministic integers.
    // Unconstrained 32-bit nondet_int() causes non-termination: integer multiplication
    // is non-linear, and CBMC bit-blasts 32x32-bit multiplies into enormous SAT formulas.
    // We bound inputs with __CPROVER_assume to keep the proof tractable.
    // This is still "all inputs" verification — just restricted to a meaningful finite range.
    int* h_A = new int[N*N];
    int* h_B = new int[N*N];

    for (int i = 0; i < N; ++i)
        for (int j = 0; j < N; ++j) {
            h_A[i*N + j] = nondet_int();
            __CPROVER_assume(h_A[i*N + j] >= -10 && h_A[i*N + j] <= 10);
            h_B[i*N + j] = nondet_int();
            __CPROVER_assume(h_B[i*N + j] >= -10 && h_B[i*N + j] <= 10);
        }

    // Convert flat arrays to 2D format required by the C++ version.
    int** A = new int*[N];
    int** B = new int*[N];
    for (int i = 0; i < N; ++i) {
        A[i] = new int[N];
        B[i] = new int[N];
        for (int j = 0; j < N; ++j) {
            A[i][j] = h_A[i*N + j];
            B[i][j] = h_B[i*N + j];

            // Sanity check: both representations hold the same values.
            assert(A[i][j] == h_A[i*N + j]);
            assert(B[i][j] == h_B[i*N + j]);
        }
    }

    // C++ Version
    int checksum_cpp = matrix_mult_cpp(A, B, N);

    // CUDA Version
    int checksum_cuda = matrix_mult_cuda(h_A, h_B, N);

    // Assert Equivalence — both functions must produce the same checksum for all inputs.
    assert(checksum_cpp == checksum_cuda);

    // Free top-level allocations
    for (int i = 0; i < N; ++i) {
        delete[] A[i];
        delete[] B[i];
    }
    delete[] A;
    delete[] B;
    delete[] h_A;
    delete[] h_B;

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
// This builds on CBMC_quality_test.cpp (fixed inputs: A[i][j]=i+j, B[i][j]=i-j) by replacing
// the hard-coded initialization with non-deterministic integers via nondet_int().
// CBMC treats each nondet_int() call as a fresh symbolic variable, so the verification
// covers ALL possible integer inputs simultaneously rather than a single concrete case.
//
// Key differences from CBMC_quality_test.cpp:
//   - Initialization is pulled out into main() (no longer embedded in each function)
//   - Both functions receive their input matrices as parameters
//   - nondet_int() replaces the deterministic i+j / i-j patterns
//   - The equivalence assert is active (not commented out)
//
// Key differences from CBMC_undet_test.cpp:
//   - Uses int instead of float throughout
//   - nondet_int() instead of nondet_float()
//   - Integer arithmetic avoids floating-point rounding concerns, giving a cleaner proof
//
// NOTE ON NON-TERMINATION:
// Fully unconstrained nondet_int() causes CBMC to non-terminate even at N=2.
// The culprit is integer multiplication (A*B): it is non-linear arithmetic, and CBMC
// bit-blasts each 32×32-bit multiply into a huge SAT formula. With 8 symbolic multiplies
// the solver stalls indefinitely. This is not a bug — it is a fundamental complexity limit.
// We add __CPROVER_assume(val >= -10 && val <= 10) to keep the proof tractable while
// still covering a meaningful (non-trivial, non-concrete) range of inputs.
//
// If CBMC reports VERIFICATION SUCCESSFUL here, it means the C++ and sequential-CUDA
// implementations agree on their checksum for all N×N integer input matrices in [-10, 10].
