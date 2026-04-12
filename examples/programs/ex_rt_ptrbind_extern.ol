#include "posix_abi.ol"

extern fn main() -> i32 {
  let pw: ptr = addr posix_write;
  return 0;
}
