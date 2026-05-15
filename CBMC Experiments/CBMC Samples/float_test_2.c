#include <assert.h>

#define N 5

void array_sum(/*float* a*/){
    // Assumption
    // __CPROVER_assume(a != 0);
    // __CPROVER_assume(__CPROVER_r_ok(a, N*sizeof(float)));

    float a[] = {0, 0, 0, 0, 0};
    // Init
    float c = 0;
    float d = 0;

    // Add  /SUM C
    unsigned int i;
    for (i = 0; i < N;  i++) {
        c += a[i];
    }

    // Add /SUM C
    for (i = N - 1; i >= 0; i--) {
        d += a[i];
    }

    assert(c == d);
}


/*
(base) christianscaff@Christians-MacBook-Pro-10 example % cbmc float_test_2.c --function array_sum --unwind 1 --trace
CBMC version 6.8.0 (cbmc-6.8.0) 64-bit arm64 macos
Type-checking float_test_2
Generating GOTO Program
Adding CPROVER library (arm64)
Removal of function pointers and virtual functions
Generic Property Instrumentation
Starting Bounded Model Checking
Passing problem to propositional reduction
converting SSA
Running propositional reduction
SAT checker: instance is SATISFIABLE
Building error trace
Running propositional reduction
SAT checker: instance is SATISFIABLE
Building error trace
Running propositional reduction
SAT checker inconsistent: instance is UNSATISFIABLE

** Results:
float_test_2.c function array_sum
[array_sum.overflow.1] line 12 arithmetic overflow on signed + in i + 1: UNKNOWN
[array_sum.unwind.0] line 12 unwinding assertion loop 0: FAILURE
[array_sum.pointer_dereference.1] line 13 dereference failure: pointer NULL in a[(signed long int)i]: FAILURE
[array_sum.pointer_dereference.2] line 13 dereference failure: pointer invalid in a[(signed long int)i]: UNKNOWN
[array_sum.pointer_dereference.3] line 13 dereference failure: deallocated dynamic object in a[(signed long int)i]: UNKNOWN
[array_sum.pointer_dereference.4] line 13 dereference failure: dead object in a[(signed long int)i]: UNKNOWN
[array_sum.pointer_dereference.5] line 13 dereference failure: pointer outside object bounds in a[(signed long int)i]: FAILURE
[array_sum.pointer_dereference.6] line 13 dereference failure: invalid integer address in a[(signed long int)i]: UNKNOWN
[array_sum.overflow.2] line 17 arithmetic overflow on signed - in i - 1: UNKNOWN
[array_sum.pointer_dereference.7] line 18 dereference failure: pointer NULL in a[(signed long int)i]: UNKNOWN
[array_sum.pointer_dereference.8] line 18 dereference failure: pointer invalid in a[(signed long int)i]: UNKNOWN
[array_sum.pointer_dereference.9] line 18 dereference failure: deallocated dynamic object in a[(signed long int)i]: UNKNOWN
[array_sum.pointer_dereference.10] line 18 dereference failure: dead object in a[(signed long int)i]: UNKNOWN
[array_sum.pointer_dereference.11] line 18 dereference failure: pointer outside object bounds in a[(signed long int)i]: UNKNOWN
[array_sum.pointer_dereference.12] line 18 dereference failure: invalid integer address in a[(signed long int)i]: UNKNOWN
[array_sum.assertion.1] line 21 assertion c == d: UNKNOWN

Trace for array_sum.unwind.0:

State 14 file float_test_2.c function __CPROVER__start line 5 thread 0
----------------------------------------------------
  INPUT tmp: 1.862646e-9f (00110001 00000000 00000000 00000010)

State 15 file float_test_2.c function __CPROVER__start line 5 thread 0
----------------------------------------------------
  INPUT a: &tmp!0@1 (00000010 00000000 00000000 00000000 00000000 00000000 00000000 00000000)

State 18 file float_test_2.c function __CPROVER__start line 5 thread 0
----------------------------------------------------
  a=&tmp!0@1 (00000010 00000000 00000000 00000000 00000000 00000000 00000000 00000000)

State 20 file float_test_2.c function array_sum line 7 thread 0
----------------------------------------------------
  c=0.0f (00000000 00000000 00000000 00000000)

State 22 file float_test_2.c function array_sum line 8 thread 0
----------------------------------------------------
  d=0.0f (00000000 00000000 00000000 00000000)

State 24 file float_test_2.c function array_sum line 11 thread 0
----------------------------------------------------
  i=0 (00000000 00000000 00000000 00000000)

State 26 file float_test_2.c function array_sum line 12 thread 0
----------------------------------------------------
  i=0 (00000000 00000000 00000000 00000000)

State 32 file float_test_2.c function array_sum line 13 thread 0
----------------------------------------------------
  c=1.862646e-9f (00110001 00000000 00000000 00000010)

State 33 file float_test_2.c function array_sum line 12 thread 0
----------------------------------------------------
  i=1 (00000000 00000000 00000000 00000001)

Violated property:
  file float_test_2.c function array_sum line 12 thread 0
  unwinding assertion loop 0



Trace for array_sum.pointer_dereference.1:

State 14 file float_test_2.c function __CPROVER__start line 5 thread 0
----------------------------------------------------
  INPUT tmp: 0.0f (00000000 00000000 00000000 00000000)

State 15 file float_test_2.c function __CPROVER__start line 5 thread 0
----------------------------------------------------
  INPUT a: ((float *)NULL) (00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000)

State 18 file float_test_2.c function __CPROVER__start line 5 thread 0
----------------------------------------------------
  a=((float *)NULL) (00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000)

State 20 file float_test_2.c function array_sum line 7 thread 0
----------------------------------------------------
  c=0.0f (00000000 00000000 00000000 00000000)

State 22 file float_test_2.c function array_sum line 8 thread 0
----------------------------------------------------
  d=0.0f (00000000 00000000 00000000 00000000)

State 24 file float_test_2.c function array_sum line 11 thread 0
----------------------------------------------------
  i=0 (00000000 00000000 00000000 00000000)

State 26 file float_test_2.c function array_sum line 12 thread 0
----------------------------------------------------
  i=0 (00000000 00000000 00000000 00000000)

Violated property:
  file float_test_2.c function array_sum line 13 thread 0
  dereference failure: pointer NULL in a[(signed long int)i]
  !(__CPROVER_POINTER_OBJECT(((float *)NULL)) == __CPROVER_POINTER_OBJECT(a))



Trace for array_sum.pointer_dereference.5:

State 14 file float_test_2.c function __CPROVER__start line 5 thread 0
----------------------------------------------------
  INPUT tmp: 0.0f (00000000 00000000 00000000 00000000)

State 15 file float_test_2.c function __CPROVER__start line 5 thread 0
----------------------------------------------------
  INPUT a: ((float *)NULL) (00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000)

State 18 file float_test_2.c function __CPROVER__start line 5 thread 0
----------------------------------------------------
  a=((float *)NULL) (00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000)

State 20 file float_test_2.c function array_sum line 7 thread 0
----------------------------------------------------
  c=0.0f (00000000 00000000 00000000 00000000)

State 22 file float_test_2.c function array_sum line 8 thread 0
----------------------------------------------------
  d=0.0f (00000000 00000000 00000000 00000000)

State 24 file float_test_2.c function array_sum line 11 thread 0
----------------------------------------------------
  i=0 (00000000 00000000 00000000 00000000)

State 26 file float_test_2.c function array_sum line 12 thread 0
----------------------------------------------------
  i=0 (00000000 00000000 00000000 00000000)

Violated property:
  file float_test_2.c function array_sum line 13 thread 0
  dereference failure: pointer NULL in a[(signed long int)i]
  !(__CPROVER_POINTER_OBJECT(((float *)NULL)) == __CPROVER_POINTER_OBJECT(a))


Violated property:
  file float_test_2.c function array_sum line 13 thread 0
  dereference failure: pointer outside object bounds in a[(signed long int)i]
  (unsigned __CPROVER_bitvector[65])__CPROVER_OBJECT_SIZE(a) >= (unsigned __CPROVER_bitvector[65])(__CPROVER_POINTER_OFFSET(a) + (unsigned long int)(signed long int)i * 4ul) + 4



** 3 of 16 failed (3 iterations)
VERIFICATION FAILED
*/