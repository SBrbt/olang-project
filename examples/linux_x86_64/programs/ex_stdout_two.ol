// Two writes + reassignment to locals (stdout order AB).
#include "posix_abi.ol"

extern i32 main() {
  let a stack[64, "A"];
  posix_write(1i32, load[a], 1u64);
  store[a, "B"];
  posix_write(1i32, load[a], 1u64);
  return 0;
}
