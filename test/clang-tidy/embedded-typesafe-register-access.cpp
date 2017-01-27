// RUN: %check_clang_tidy %s embedded-typesafe-register-access %t

volatile uint32_t& x;
uint32_t y = 0;

x &= 0xDEADBEEF;
// CHECK-MESSAGES: :[[@LINE-1]]:6: warning: Found a raw compound assignment [embedded-typesafe-register-access]
// CHECK-FIXES: x &= deadbeef;

y &= 0xDEADBEEF;
