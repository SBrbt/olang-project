#include "posix_abi.ol"

extern i32 main() {
  posix_write(1i64, "a\tb\n\\c\n", 7i64);
  return 0;
}
