#include <assert.h>

void add3(float a, float b, float c) {
    // ((a + b) + c) == ((b + c) + a)
    float d = a + b;
    float e = d + c;
    float f = b + c;
    float g = f + a;

    // assert(e == g);
    assert(e / g < 1.001f);
    assert(g / e < 1.001f);
}

// To run: --t
// cbmc float_test_1.c --function add3 --unwind 1

// ====== RESULT =====
/*
(base) christianscaff@Christians-MacBook-Pro-10 example % cbmc float_test_1.c --function add3 --unwind 1 --trace
CBMC version 6.8.0 (cbmc-6.8.0) 64-bit arm64 macos
Type-checking float_test_1
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
float_test_1.c function add3
[add3.assertion.1] line 10 assertion e == g: FAILURE

Trace for add3.assertion.1:

State 11 file float_test_1.c function __CPROVER__start line 3 thread 0
----------------------------------------------------
  INPUT a: 4.701975e-38f (00000001 01111111 11111111 11111001)

State 14 file float_test_1.c function __CPROVER__start line 3 thread 0
----------------------------------------------------
  INPUT b: -2.350987e-37f (10000010 10011111 11111111 11110111)

State 17 file float_test_1.c function __CPROVER__start line 3 thread 0
----------------------------------------------------
  INPUT c: 1.880791e-37f (00000010 10000000 00000000 00000000)

State 20 file float_test_1.c function __CPROVER__start line 3 thread 0
----------------------------------------------------
  a=4.701975e-38f (00000001 01111111 11111111 11111001)

State 21 file float_test_1.c function __CPROVER__start line 3 thread 0
----------------------------------------------------
  b=-2.350987e-37f (10000010 10011111 11111111 11110111)

State 22 file float_test_1.c function __CPROVER__start line 3 thread 0
----------------------------------------------------
  c=1.880791e-37f (00000010 10000000 00000000 00000000)

State 24 file float_test_1.c function add3 line 5 thread 0
----------------------------------------------------
  d=-1.880789e-37f (10000010 01111111 11111111 11110000)

State 26 file float_test_1.c function add3 line 6 thread 0
----------------------------------------------------
  e=1.793662e-43f (00000000 00000000 00000000 10000000)

State 28 file float_test_1.c function add3 line 7 thread 0
----------------------------------------------------
  f=-4.701957e-38f (10000001 01111111 11111111 10111000)

State 30 file float_test_1.c function add3 line 8 thread 0
----------------------------------------------------
  g=1.821688e-43f (00000000 00000000 00000000 10000010)

Violated property:
  file float_test_1.c function add3 line 10 thread 0
  assertion e == g
  !((signed long int)(signed long int)!IEEE_FLOAT_EQUAL(e, g) != 0l)
*/