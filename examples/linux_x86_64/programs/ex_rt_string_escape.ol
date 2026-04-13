#include "posix_abi.ol"

extern i32 main() {
  posix_write(1i32, "a\tb\n\\c\n", 7u64);
  return 0;
}
