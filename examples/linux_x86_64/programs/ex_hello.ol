// Stdout smoke: string literal is ptr (rodata address).
#include "posix_abi.ol"

extern i32 main() {
  posix_write(1i32, "Hello from OLang\n", 17u64);
  return 0;
}
