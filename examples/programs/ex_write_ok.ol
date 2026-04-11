#include "posix_abi.ol"

extern fn main() -> i32 {
  posix_write(1i64, "OK\n", 3i64);
  return 0;
}
