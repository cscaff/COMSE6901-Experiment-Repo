# CUDA → LEAN 4 Expert Knowledge Base

## CUDA Thread/Memory Patterns

### Pattern 1: One-to-One Output-Parallel (matMul, element-wise ops)

**Signature**: Each thread owns exactly one output element. No inter-thread communication.

**Canonical example**: `matMulKernel`
```cuda
int row = blockIdx.y * blockDim.y + threadIdx.y;
int col = blockIdx.x * blockDim.x + threadIdx.x;
if (row < N && col < N) {
    int sum = 0;
    for (int k = 0; k < N; ++k)
        sum += A[row * N + k] * B[k * N + col];
    C[row * N + col] = sum;
}
```

**Mathematical model**:
- Thread index: (row, col) where row = globalY, col = globalX
- Thread (row, col) computes: `C(row, col) = Σ_{k=0}^{N-1} A(row,k) × B(k,col)`
- Bijective thread→output mapping: N² distinct threads → N² distinct output cells
- Memory: each thread reads row `row` of A and column `col` of B (global memory, no shared)

**LEAN 4 model**:
```lean
def isum : (n : Nat) → (Fin n → Int) → Int
  | 0, _ => 0
  | n + 1, f =>
    isum n (fun i => f ⟨i.val, Nat.lt_succ_of_lt i.isLt⟩) +
    f ⟨n, Nat.lt_succ_self n⟩

def cudaMatMulKernel (N : Nat) (A B : Fin N → Fin N → Int) : Fin N → Fin N → Int :=
  fun row col => isum N (fun k => A row k * B k col)
```

**Key**: `isum` models the sequential accumulation `sum += f(k)` for k = 0..N-1 with left-to-right association, matching the CUDA for-loop exactly.

---

## LEAN 4 Techniques for CUDA Formalization

### Summation: `isum` vs `Finset.sum`

`isum` models CUDA's sequential loop. To prove equality with Mathlib's abstract `Finset.sum`:

**Bridge lemma pattern**:
```lean
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
```

**Key Mathlib lemma**: `Fin.sum_univ_castSucc` (in `Mathlib.Algebra.BigOperators.Fin`):
```
∑ i : Fin (n+1), f i = ∑ i : Fin n, f (Fin.castSucc i) + f (Fin.last n)
```
This splits off the LAST element, matching `isum`'s right-recursive structure.

**Alternative**: `Fin.sum_univ_succ` splits off the FIRST element:
```
∑ i : Fin (n+1), f i = f 0 + ∑ i : Fin n, f i.succ
```
Use this if your `isum` variant adds the first element first.

### Fin Equality (Proof Irrelevance)

Two `Fin n` values with the same `.val` are equal:
```lean
Fin.ext rfl : ⟨i.val, h1⟩ = Fin.castSucc i  -- when (Fin.castSucc i).val = i.val
congr_arg f (Fin.ext rfl)                      -- lifts to f ⟨i.val, h1⟩ = f (Fin.castSucc i)
```

Key: `Fin.castSucc i` preserves `.val` by definition; `Fin.last n` has `.val = n`.

### simp_rw vs simp only

- `simp only [h]` cannot rewrite inside λ-binders
- `simp_rw [h]` CAN rewrite under binders — essential when your CUDA model uses `fun i => f ⟨i.val, _⟩`

### Matrix Type Typing (Critical)

When proving `cudaKernel N A B = A * B`:
- `A B` MUST be typed as `Matrix (Fin N) (Fin N) Int`, NOT `Fin N → Fin N → Int`
- Even though `Matrix m n α = m → n → α` definitionally, the `*` instance and `Matrix.mul_apply` simp lemma require the `Matrix` type annotation
- `Matrix.mul_apply : (M * N) i k = ∑ j, M i j * N j k` (in `Mathlib.Data.Matrix.Basic`)

### Functional Equality Proof Template

```lean
theorem cudaKernel_eq_mathlib (N : Nat) (A B : Matrix (Fin N) (Fin N) Int) :
    cudaKernel N A B = A * B := by
  funext row col
  simp only [cudaKernel, ..., Matrix.mul_apply]
  exact isum_eq_finset_sum N (fun k => A row k * B k col)
```

---

## Tool Observations (Lean 4.30.0-rc2)

### lean_check.sh Error Detection Bug

In Lean 4.30.0-rc2, error messages use the format:
```
file.lean:38:29: error(lean.unknownIdentifier): Unknown constant `Matrix.mul`
```

The `lean_check.sh` script greps for `': error:'` (colon-error-colon), which does NOT match `': error('` (colon-error-paren). This means genuine errors are silently reported as STATUS: PASS.

**Workaround**: Always read the raw lean output section of `lean_check.sh` output, not just the STATUS line. Empty lean output = true pass. Any content in lean output = inspect manually.

---

## Functional Equality Results

| CUDA Kernel | Pattern | LEAN Def | Mathlib Counterpart | Result |
|-------------|---------|----------|---------------------|--------|
| matMulKernel (N×N, int) | One-to-one output-parallel | `cudaMatMulKernel` via `isum` | `Matrix.mul` (`*`) on `Matrix (Fin N) (Fin N) Int` | **PROVEN EQUAL** ✓ |

**Imports needed for proof**:
- `Mathlib.Data.Matrix.Basic` — for `Matrix.mul_apply`
- `Mathlib.Algebra.BigOperators.Fin` — for `Fin.sum_univ_castSucc`
- Or just `import Mathlib` to pull everything

---

## Counter-Examples

None derived for matMulKernel. The CUDA kernel and Mathlib matrix multiplication are functionally equal for all N and all integer matrices A, B.

**Note**: Counter-examples would arise if:
- Integer overflow is modeled (CUDA uses 32-bit int, Mathlib uses unbounded Int)
- Thread bounds are violated (the `if (row < N && col < N)` guard; in the LEAN model we use `Fin N` which enforces bounds structurally)
