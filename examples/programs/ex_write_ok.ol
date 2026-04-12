#include "posix_abi.ol"

extern i32 main() {
  posix_write(1i64, "OK\n", 3i64);
  return 0;
}
