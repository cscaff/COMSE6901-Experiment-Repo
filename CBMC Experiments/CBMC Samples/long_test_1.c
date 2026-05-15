#include <assert.h>

void add3(long a, long b, long c) {
    // ((a + b) + c) == ((b + c) + a)
    a &= 0x7FFFFFFF;
    b &= 0x7FFFFFFF;
    c &= 0x7FFFFFFF;

    long d = a + b;
    long e = d + c;
    long f = b + c;
    long g = f + a;

    assert(e == g);
}


/*
(base) christianscaff@Christians-MacBook-Pro-10 example % cbmc long_test_1.c --function add3 --unwind 1 --trace
CBMC version 6.8.0 (cbmc-6.8.0) 64-bit arm64 macos
Type-checking long_test_1
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
long_test_1.c function add3
[add3.overflow.1] line 9 arithmetic overflow on signed + in a + b: SUCCESS
[add3.overflow.2] line 10 arithmetic overflow on signed + in d + c: SUCCESS
[add3.overflow.3] line 11 arithmetic overflow on signed + in b + c: SUCCESS
[add3.overflow.4] line 12 arithmetic overflow on signed + in f + a: SUCCESS
[add3.assertion.1] line 14 assertion e == g: SUCCESS

** 0 of 5 failed (1 iterations)
VERIFICATION SUCCESSFUL
(base) christianscaff@Christians-MacBook-Pro-10 example % 
*/