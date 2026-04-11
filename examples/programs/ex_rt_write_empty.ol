#include "posix_abi.ol"

extern fn main() -> i32 {
  posix_write(1i64, "unused", 0i64);
  return 0;
}
