// RUN: %check_clang_tidy %s embedded-typesafe-register-access %t

//void f();
volatile uint32_t& x &= 0xDEADBEEF;

// CHECK-MESSAGES: :[[@LINE-1]]:6: warning: Found a raw compound assignment [embedded-typesafe-register-access]

// FIXME: Verify the applied fix.
//   * Make the CHECK patterns specific enough and try to make verified lines
//     unique to avoid incorrect matches.
//   * Use {{}} for regular expressions.
// CHECK-FIXES: {{^}}void awesome_f();{{$}}

// FIXME: Add something that doesn't trigger the check here.
// void awesome_f2();
int x = 1;
x = 0;
