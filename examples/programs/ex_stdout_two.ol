// Two writes + reassignment to locals (stdout order AB).
#include "posix_abi.ol"

extern fn main() -> i32 {
  let a: ptr = "A";
  posix_write(1i64, a, 1i64);
  a = "B";
  posix_write(1i64, a, 1i64);
  return 0;
}
