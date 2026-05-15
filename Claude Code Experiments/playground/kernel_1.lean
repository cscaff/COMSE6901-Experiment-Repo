import Mathlib.Data.Matrix.Basic
import Mathlib.Algebra.BigOperators.Fin

-- Mapped CUDA Definition


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
