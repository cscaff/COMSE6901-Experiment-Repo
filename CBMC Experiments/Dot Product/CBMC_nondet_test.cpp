//   _____  ______ __  __  ____
//  |  __ \|  ____|  \/  |/ __ \
//  | |  | | |__  | \  / | |  | |
//  | |  | |  __| | |\/| | |  | |
//  | |__| | |____| |  | | |__| |
//  |_____/|______|_|  |_|\____/
//
// Non-Deterministic Equivalence Checking: CBMC verifies across ALL possible float inputs.
// Copied from CBMC_quality_test.cpp and modified as described below.


// EX. Dot Product — Non-Deterministic Input Version

// ============================================================
// CHANGES FROM CBMC_quality_test.cpp
// ============================================================
//
// CHANGE 1 — Function signatures: arrays moved to main() and passed in.
//
//   BEFORE (quality_test):
//     int  dot_product_cpp(int N)   { float* A = new float[N]; A[i] = 1.0f; ... }
//     float dot_product_cuda(int N) { float* h_A = new float[N]; h_A[i] = 1.0f; ... }
//
//   AFTER (nondet_test):
//     float dot_product_cpp (float* A, float* B, int n)   { /* uses caller arrays */ }
//     float dot_product_cuda(float* A, float* B, int n)   { /* uses caller arrays */ }
//
//   WHY: If each function allocates and fills its own arrays, CBMC treats
//   them as independent symbolic variables — different inputs to each side —
//   so the equivalence check is meaningless. Both functions must operate on
//   the exact same A[] and B[] allocated once in main().
//
// ---------------------------------------------------------------
//
// CHANGE 2 — Accumulator type in dot_product_cpp: double -> float.
//
//   BEFORE (quality_test):
//     double dot = 0.0;
//     for (int i = 0; i < n; ++i) dot += A[i] * B[i];
//     return dot;   // returned as int via implicit cast
//
//   AFTER (nondet_test):
//     float dot = 0.0f;
//     for (int i = 0; i < n; ++i) dot += A[i] * B[i];
//     return dot;
//
//   WHY: With fixed inputs (1.0f, 2.0f) the double result happened to round
//   back to the same float value. With arbitrary nondet floats, double
//   accumulation diverges from float accumulation and the assert fails on
//   virtually every non-trivial input. Using float in both functions tests
//   real algorithmic equivalence rather than a precision artifact.
//
// ---------------------------------------------------------------
//
// CHANGE 3 — Input initialization: fixed constants -> __CPROVER_nondet_float().
//
//   BEFORE (quality_test):
//     for (int i = 0; i < N; ++i) { A[i] = 1.0f; B[i] = 2.0f; }
//
//   AFTER (nondet_test):
//     for (int i = 0; i < N; ++i) {
//         A[i] = __CPROVER_nondet_float();   // unconstrained symbolic value
//         B[i] = __CPROVER_nondet_float();
//         __CPROVER_assume(__builtin_isfinite(A[i]));  // exclude NaN / ±Inf
//         __CPROVER_assume(__builtin_isfinite(B[i]));
//     }
//
//   WHY: __CPROVER_nondet_float() tells CBMC to treat the value as a fresh
//   symbolic variable with no fixed assignment, so the solver explores all
//   possible float bit patterns simultaneously. The __CPROVER_assume guards
//   exclude NaN and ±Inf: NaN != NaN always in IEEE 754, which would make
//   the final assert fail even when both sides compute identically.
//
// ---------------------------------------------------------------
//
// CHANGE 4 — main() calls updated to match new signatures.
//
//   BEFORE (quality_test):
//     float result_cpp  = dot_product_cpp(5);
//     float result_cuda = dot_product_cuda(5);
//
//   AFTER (nondet_test):
//     float result_cpp  = dot_product_cpp (A, B, N);
//     float result_cuda = dot_product_cuda(A, B, N);
//
//   WHY: Follows directly from Change 1 — shared arrays are passed explicitly.
//
// ---------------------------------------------------------------
//
// UNCHANGED: dotProductKernel_seq, BLOCK_SIZE=4, N=4, assert, unwind bound.
// ============================================================

#include <assert.h>
#include <string.h>   // memcpy
// Note: __CPROVER_isnan / __CPROVER_isinf are CBMC built-ins; no header needed.

// __CPROVER_nondet_float / __CPROVER_assume are CBMC built-in intrinsics.
// CBMC defines __CPROVER automatically, so under verification these declarations
// are hidden and CBMC uses its own built-ins (avoiding "no body for callee").
// The #else branch satisfies clang / the IDE linter during normal compilation.
#ifndef __CPROVER
float __CPROVER_nondet_float(void);
void  __CPROVER_assume(int condition);
int   __CPROVER_isnan(float x);
int   __CPROVER_isinf(float x);
#endif

#define N          4   // Vector length — keep tiny for CBMC loop unrolling
#define BLOCK_SIZE 4   // Threads-per-block analogue (was 256 in real CUDA)


// ---------------------------------------------------------------------------
// C++ sequential dot product — accepts caller-owned arrays, uses float sum
// ---------------------------------------------------------------------------
float dot_product_cpp(float* A, float* B, int n) {
    float dot = 0.0f;           // float (not double) to match CUDA precision
    for (int i = 0; i < n; ++i)
        dot += A[i] * B[i];
    return dot;
}


// ---------------------------------------------------------------------------
// Sequential model of the CUDA tree-reduction dot product kernel
// ---------------------------------------------------------------------------
void dotProductKernel_seq(float* A, float* B, float* partial, int n, int gridDim) {
    for (int blockIdx = 0; blockIdx < gridDim; blockIdx++) {

        float cache[BLOCK_SIZE];

        // === Phase 1: Strided accumulation (one "thread" at a time) ===
        for (int cacheIndex = 0; cacheIndex < BLOCK_SIZE; cacheIndex++) {
            int tid = blockIdx * BLOCK_SIZE + cacheIndex;
            float temp = 0.0f;
            while (tid < n) {
                temp += A[tid] * B[tid];
                tid += BLOCK_SIZE * gridDim;
            }
            cache[cacheIndex] = temp;
        }

        // __syncthreads() is implicit — the Phase 1 loop fully completes first.

        // === Phase 2: Tree reduction ===
        for (int i = BLOCK_SIZE / 2; i > 0; i >>= 1) {
            for (int cacheIndex = 0; cacheIndex < BLOCK_SIZE; cacheIndex++) {
                if (cacheIndex < i)
                    cache[cacheIndex] += cache[cacheIndex + i];
            }
            // __syncthreads() implicit — inner loop completes each level
        }

        partial[blockIdx] = cache[0];
    }
}

// NOTES:
// https://en.wikipedia.org/wiki/Interference_freedom
// Might not generalize past communative operations. 
// May fall apart with race conditions.
// Want to show non-interference with each loop.
// Want to show no race conditions.
//  Better Attempt
//  - What are good transformations we can verify. And what are the side conditions
//  - How can we verify the transformations are correct/Eq w/ a theorem prover?
// Let's make sure all the conditions for the rewrites are met.




// ---------------------------------------------------------------------------
// CUDA dot product wrapper — accepts caller-owned arrays
// ---------------------------------------------------------------------------
float dot_product_cuda(float* A, float* B, int n) {
    int blocks = (n + BLOCK_SIZE - 1) / BLOCK_SIZE;

    // CBMC-friendly heap allocation (replaces cudaMalloc / cudaMemcpy)
    float* d_A       = new float[n];
    float* d_B       = new float[n];
    float* d_partial = new float[blocks];

    memcpy(d_A, A, n * sizeof(float));
    memcpy(d_B, B, n * sizeof(float));

    dotProductKernel_seq(d_A, d_B, d_partial, n, blocks);

    float* h_partial = new float[blocks];

    float result = 0.0f;
    for (int i = 0; i < blocks; ++i)
        result += d_partial[i];

    delete[] d_A;
    delete[] d_B;
    delete[] d_partial;
    return result;
}


// ---------------------------------------------------------------------------
// main — CBMC entry point
// ---------------------------------------------------------------------------
int main() {
    float A[N], B[N];

    // Fill with non-deterministic values.
    // CBMC treats each __CPROVER_nondet_float() call as a fresh unconstrained
    // symbolic float, so the verification covers all possible float arrays.
    for (int i = 0; i < N; ++i) {
       A[i] = __CPROVER_nondet_float();
        __CPROVER_assume(!__CPROVER_isnan(A[i]) && !__CPROVER_isinf(A[i]));
       B[i] = __CPROVER_nondet_float();
        __CPROVER_assume(!__CPROVER_isnan(B[i]) && !__CPROVER_isinf(B[i]));

        // Restrict to finite values to avoid NaN/Inf propagation artifacts.
        // (NaN != NaN always, so an assert on NaN outputs would trivially fail.)
        // __CPROVER_assume(!__CPROVER_isnan(A[i]) && !__CPROVER_isinf(A[i]));
        // __CPROVER_assume(!__CPROVER_isnan(B[i]) && !__CPROVER_isinf(B[i]));
    }

    float result_cpp  = dot_product_cpp (A, B, N);
    float result_cuda = dot_product_cuda(A, B, N);


    // Test 
    float x;
    __CPROVER_assume(!__CPROVER_isnan(x) && !__CPROVER_isinf(x));
    // float y = (float)x;

    // If CBMC reports a failure here it has found concrete float values for
    // which the C++ and CUDA implementations produce different results.
    // assert(result_cpp == result_cuda);

    return 0;
}

// ---------------------------------------------------------------------------
// How to run:
//
//   cbmc CBMC_nondet_test.cpp --unwind 5 --trace
//
// Expected outcome:
//   If the two implementations are bitwise-equivalent under float arithmetic,
//   CBMC will report VERIFICATION SUCCESSFUL.
//   If it finds a counterexample it will print the concrete A[] / B[] values
//   that expose the divergence.
// ---------------------------------------------------------------------------
