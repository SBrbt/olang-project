// Two writes + reassignment to locals (stdout order AB).
#include "posix_abi.ol"

extern i32 main() {
  let a<ptr> @stack<64>("A");
  posix_write(1i64, load<a>, 1i64);
  store<a, "B">;
  posix_write(1i64, load<a>, 1i64);
  return 0;
}
