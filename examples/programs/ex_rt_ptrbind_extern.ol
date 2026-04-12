#include "posix_abi.ol"

extern i32 main() {
  let pw<ptr> @stack<64>(addr posix_write);
  return 0;
}
