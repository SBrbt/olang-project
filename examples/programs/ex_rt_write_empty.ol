#include "posix_abi.ol"

extern i32 main() {
  posix_write(1i64, "unused", 0i64);
  return 0;
}
