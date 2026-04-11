#include "posix_abi.ol"

extern fn main() -> i32 {
  ptrbind pw as ptr from posix_write;
  return 0;
}
