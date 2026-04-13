// u64 max literal and wrap on add.
extern i32 main() {
  let a stack[64, 18446744073709551615u64];
  let b stack[64, 9223372036854775808u64];
  if (load[a] + 1u64 == 0u64 && load[b] == 9223372036854775808u64) {
    return 0;
  }
  return 1;
}
