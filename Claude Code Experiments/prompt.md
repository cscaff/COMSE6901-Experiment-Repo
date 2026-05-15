You are an expert at understanding CUDA code and writing LEAN 4 code. Your goal is do the following:

Given the following CUDA kernel:

```CUDA
__global__ void dotProductKernel(float* A, float* B, float* partial, int n) {
    __shared__ float cache[256];
    int tid = blockIdx.x * blockDim.x + threadIdx.x;
    int cacheIndex = threadIdx.x;
    float temp = 0.0f;
    while (tid < n) {
        temp += A[tid] * B[tid];
        tid += blockDim.x * gridDim.x;
    }
    cache[cacheIndex] = temp;
    __syncthreads();
    for (int i = blockDim.x / 2; i > 0; i >>= 1) {
        if (cacheIndex < i) cache[cacheIndex] += cache[cacheIndex + i];
        __syncthreads();
    }
    if (cacheIndex == 0) partial[blockIdx.x] = cache[0];
}
```

You will:

1. Infer a mathematical mapping of the CUDA kernel. That mathematical definition must:
    - Faithfully represent thread/memory semantics. Consider that different CUDA kernels have different thread/memory patterns: one-to-one mapping, stencil/diffusion, shared cache, reduction, iterative launch, etc...
    - The mathematical model should represent that parallel pattern explicitly.
  
2. Translate and formalize the derived mathematical model into LEAN 4 using only the existing tools in `tools/` to verify that your translation is valid LEAN 4 code. The translated model must be valid LEAN 4 code. The model should not use Mathlib, only generic LEAN 4.

3. Prove that the derived mathematical CUDA model is FUNCTIONALLY equal to the MathLib Matrix Multiplication definition. That is for all equal inputs, into the CUDA LEAN definition and the MathLib definition, they both guarantee the exact same results. If you are unable to prove functional equality, return a LEAN counter example that proves the two functionally unequal.

4. Store your learned knowledge about LEAN 4 and CUDA thread/memory patterns in `expert.md/` such that you can use it as a knowledge-base in the future. Include known counter-examples you derive.

Constraints:

You must represent the CUDA kernel's thread/memory semantics as accurate to the source code as much as possible in your LEAN definition. You should never modify your LEAN definition in a manner that no longer represents the CUDA kernel in an attempt to prove equality.

You may use knowledge from `expert.md`.

You will never modify any files in `tools/`.

You will output a json file in `outputs/` with the responses to the above instructions:

EXAMPLE:
```
{
    model: "Record model used"
    prompt: "Record the given prompt here",
    math_model: "response to 1",
    lean_def: "response to 2",
    theorem: "response to 3",
    knowledge: "new knowledge from 4"
    timing: "Record the time it took."
}
```