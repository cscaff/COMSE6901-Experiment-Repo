#include <assert.h>

// Reference implementation: left-to-right accumulation.
int add_left(int a, int b, int c) {
    return (a + b) + c;
}

// Buggy implementation: off-by-one error introduced in the final step.
int add_wrong(int a, int b, int c) {
    return a + (b + c) + 1;
}

int main() {
    int a, b, c;

    // Assumptions: bound inputs so no signed overflow occurs.
    __CPROVER_assume(a >= -100 && a <= 100);
    __CPROVER_assume(b >= -100 && b <= 100);
    __CPROVER_assume(c >= -100 && c <= 100);

    // Assertion: the two implementations must agree on all bounded inputs.
    // This is FALSE — add_wrong always returns one more than add_left.
    assert(add_left(a, b, c) == add_wrong(a, b, c));

    return 0;
}

// To run:
//   cbmc add_equiv.c --unwind 1 --trace

// ====== RESULT ======
/*
$ cbmc add_equiv.c --unwind 1 --trace
CBMC version 6.8.0 (cbmc-6.8.0) 64-bit arm64 macos
Type-checking add_equiv
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

** Results:
add_equiv.c function add_left
[add_left.overflow.1] line 5 arithmetic overflow on signed + in a + b: SUCCESS
[add_left.overflow.2] line 5 arithmetic overflow on signed + in a + b + c: SUCCESS

add_equiv.c function add_wrong
[add_wrong.overflow.1] line 10 arithmetic overflow on signed + in b + c: SUCCESS
[add_wrong.overflow.2] line 10 arithmetic overflow on signed + in a + b + c: SUCCESS
[add_wrong.overflow.3] line 10 arithmetic overflow on signed + in a + b + c + 1: SUCCESS

add_equiv.c function main
[main.assertion.1] line 23 assertion add_left(a, b, c) == add_wrong(a, b, c): FAILURE

Trace for main.assertion.1:

  INPUT a: 91
  INPUT b: -37
  INPUT c: 23

  Assumption: a >= -100 && a <= 100
  Assumption: b >= -100 && b <= 100
  Assumption: c >= -100 && c <= 100

  return_value_add_left  = 77   // (91 + -37) + 23 = 77
  return_value_add_wrong = 78   // 91 + (-37 + 23) + 1 = 78

Violated property:
  file add_equiv.c function main line 23 thread 0
  assertion add_left(a, b, c) == add_wrong(a, b, c)
  FALSE

** 1 of 6 failed (2 iterations)
VERIFICATION FAILED
*/
