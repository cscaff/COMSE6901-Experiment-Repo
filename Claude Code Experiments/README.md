# Claude Code Based LEAN Generator and Verifier

This simple directory serves as an experiment to see how well Claude Code can generate LEAN definitions and prove LEAN theorems.


# Notes

I think a major thing to make sure is ensuring the mathematical model we derive is faithful to the original kernel.

# Test Run 1 (Matrix Multiplication):

**Prompt:**: 

"""
Given the following CUDA kernel:

```
__global__ void matMulKernel(int* A, int* B, int* C, int N) {
    int row = blockIdx.y * blockDim.y + threadIdx.y;
    int col = blockIdx.x * blockDim.x + threadIdx.x;
    
    if (row < N && col < N) {
        int sum = 0;

       for (int k = 0; k < N; ++k)
            sum += A[row * N + k] * B[k * N + col];
            C[row * N + col] = sum;
}
```

You will:

1. Infer a mathematical mapping of the CUDA kernel.

2. Translate and formalize the derived mathematical model into LEAN 4.
   
3. Prove that the derived mathematical CUDA model is FUNCTIONALLY equal to the MathLib Matrix Multiplication definition.

4. Store learned knowledge in expert.md.
"""

### Mathematical Model

**Pattern**: One-to-One Output-Parallel

Thread index mapping (global 2D grid):
```
row = blockIdx.y * blockDim.y + threadIdx.y
col = blockIdx.x * blockDim.x + threadIdx.x
```

**Domain**: `row, col ∈ Fin N` (i.e., [0, N) enforced by the if-guard)

Each thread `(row, col)` owns exactly one output element and computes:
```
C(row, col) := Σ_{k : Fin N} A(row, k) × B(k, col)
```

*Christian Notes*: LLM skipped past modeling the 1D flat indexing to 2D indexing. If it were faithful to the original code, it would have modeled that.

**Parallel semantics:**
- The thread-to-output mapping is bijective: N² threads → N² distinct output positions
- No inter-thread communication (no `__shared__` memory, no `__syncthreads`)
- Memory access: thread `(row, col)` reads all of `A[row, *]` (global) and all of `B[*, col]` (global)
- Inner loop `for k = 0..N-1` is a sequential accumulation within a single thread:
  `sum = 0; for each k: sum = sum + A[row,k]*B[k,col]`
  This is a left-associative accumulation: `((0 + f(0)) + f(1)) + ... + f(N-1)`

*Christian Notes*: Correctly models parallel aspects of the matrix multiplication: Thread Coverage and Disjoint Property, lack of shared memory.

**Formal model:**
```
Let A, B : Fin N → Fin N → ℤ
cudaMatMulKernel(A, B) := λ row col. Σ_{k : Fin N} A(row, k) × B(k, col)
where Σ is a left-fold (matching the sequential CUDA accumulation).
```

*Christian Notes*: Fin. N -> Fin N -> Z models taking row and column and returning the value stored? It is a function that maps indices and columns to an integer value.

---

### Lean 4 Definition (standalone, no Mathlib)

```lean
-- isum models the CUDA inner for-loop: sum = 0; for k in [0,N): sum += f(k)
-- Left-recursive: isum n f = isum (n-1) f' + f(n-1), unfolding to ((0+f(0))+f(1))+...+f(n-1)
def isum : (n : Nat) → (Fin n → Int) → Int
  | 0, _ => 0
  | n + 1, f =>
    isum n (fun i => f ⟨i.val, Nat.lt_succ_of_lt i.isLt⟩) +
    f ⟨n, Nat.lt_succ_self n⟩

-- N×N integer matrix as a function (matches CUDA int* with row-major indexing)
abbrev CudaMatrix (N : Nat) := Fin N → Fin N → Int

-- Thread (row, col) computes its single output element
-- One-to-one pattern: each thread independently owns C[row,col]
def cudaThreadCompute (N : Nat) (A B : CudaMatrix N) (row col : Fin N) : Int :=
  isum N (fun k => A row k * B k col)

-- Full kernel: all threads execute in parallel, each writing to a distinct C[i,j]
def cudaMatMulKernel (N : Nat) (A B : CudaMatrix N) : CudaMatrix N :=
  fun row col => cudaThreadCompute N A B row col

-- Verified eval: 1×1 → 12, 2×2 → (19, 22, 43, 50) ✓
```

*Christian's Notes*: isum recursively performs sum += f(k) for the inner most loop values of k.

Looks like the LLM is modeling the idea of thread computation as a wrapper over the defined summation isum. I suppose this does model how the threads work. I worry from ProofWright though on how well this would extend to less simple models of thread usage. I know the paper mentioned it mainly worked for one-to-one mappings as such.

Looks like cudaMatMulKernel executes the idea of a thread per row and column. This makes sense.

---

### Lean 4 + Mathlib Theorem (STATUS: PASS, ERRORS: 0, WARNINGS: 0)

```lean
import Mathlib.Data.Matrix.Basic
import Mathlib.Algebra.BigOperators.Fin

-- Bridge lemma: the sequential CUDA loop equals Mathlib's abstract Finset.sum
lemma isum_eq_finset_sum (n : Nat) (f : Fin n → Int) :
    isum n f = ∑ k : Fin n, f k := by
  induction n with
  | zero => simp [isum]
  | succ n ih =>
    simp only [isum, Fin.sum_univ_castSucc]
    have h_cast : ∀ i : Fin n,
        f ⟨i.val, Nat.lt_succ_of_lt i.isLt⟩ = f (Fin.castSucc i) :=
      fun i => congr_arg f (Fin.ext rfl)
    have h_last : f ⟨n, Nat.lt_succ_self n⟩ = f (Fin.last n) :=
      congr_arg f (Fin.ext rfl)
    simp_rw [h_cast, h_last, ih]

-- Main theorem: CUDA kernel is functionally equal to Mathlib Matrix.mul
-- Note: A B typed as Matrix (not Fin N → Fin N → Int) to activate the * instance
theorem cudaMatMul_eq_mathlib (N : Nat) (A B : Matrix (Fin N) (Fin N) Int) :
    cudaMatMulKernel N A B = A * B := by
  funext row col
  simp only [cudaMatMulKernel, cudaThreadCompute, Matrix.mul_apply]
  exact isum_eq_finset_sum N (fun k => A row k * B k col)

-- Result: PROVEN FUNCTIONALLY EQUAL for all N and all integer matrices A, B.
```

*Christian's Notes*: Looks like the first thing the LLM attempts to do is prove that the thread-based definition of the summation is equal to MathLib's definition of a finite set summation. It does so inductively:

- Base case: zero
- Inductive Case: succ
- Defines lemma h_cast
- Defines lemma h_last

Then the LLM attempts to prove both functional definitions of a matrix multiplication equal by showing all entries are equal. It deos this by unfolding the definitions into a manner that makes use of the finite sums such that we can use the existing proof for equality of finite set summation.  
---

### Key Learnings

1. **CUDA ONE-TO-ONE PATTERN**: `matMulKernel` is one-to-one output-parallel — each thread maps bijectively to one output cell, no shared memory or reduction. Lean model: `λ row col. isum N (fun k => A row k * B k col)`.

2. **`isum` models the CUDA loop**: Define `isum` recursively, adding the last element at each step. This gives left-associative accumulation matching `sum += f(k)` for `k=0..N-1`.

3. **Bridge lemma**: `isum n f = ∑ k : Fin n, f k`. Proved by induction using `Fin.sum_univ_castSucc` (in `Mathlib.Algebra.BigOperators.Fin`), which splits `∑ i : Fin (n+1), f i` into `(∑ i : Fin n, f (Fin.castSucc i)) + f (Fin.last n)`. Use `simp_rw` (not `simp only`) to rewrite under binders.

4. **`Fin` equality**: `Fin.ext rfl` proves `⟨i.val, h1⟩ = Fin.castSucc i` and `⟨n, _⟩ = Fin.last n`. Use `congr_arg f (Fin.ext rfl)` to lift to function applications.

5. **Matrix type annotation**: For `Matrix.mul_apply` to fire in `simp`, arguments must be typed as `Matrix (Fin N) (Fin N) Int`, not as `Fin N → Fin N → Int`, even though they are definitionally equal.

6. **No counter-examples**: `cudaMatMulKernel` is genuinely equal to `Matrix.mul` over `Int` (unbounded integers). Divergence would occur only if modeling 32-bit overflow or if the thread-bound guard were removed.

---

**Timing**: ~6 minutes 20 seconds total (~30s standalone definition + eval checks, ~5m50s Mathlib proof including one fix for Matrix type annotation).

# Test Run 2 (Dot Product w/ Reduction)

Failed to terminate within 20 minutes...