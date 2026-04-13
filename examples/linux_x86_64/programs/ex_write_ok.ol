#include "posix_abi.ol"

extern i32 main() {
  posix_write(1i32, "OK\n", 3u64);
  return 0;
}
