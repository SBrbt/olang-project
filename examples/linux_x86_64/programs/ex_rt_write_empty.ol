#include "posix_abi.ol"

extern i32 main() {
  posix_write(1i32, "unused", 0u64);
  return 0;
}
