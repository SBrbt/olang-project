// Test u64 arithmetic, comparisons, and large values.
extern i32 main() {
  let a stack[64, 1000000u64];
  let b stack[64, 999999u64];
  let big stack[64, 18446744073709551615u64];

  if (load[a] > load[b]) {
    if (load[a] - load[b] == 1u64) {
      if (load[a] * 2u64 == 2000000u64) {
        if (load[a] / 10u64 == 100000u64) {
          if (load[a] % 3u64 == 1u64) {
            if (load[big] != 0u64) {
              return 0;
            }
          }
        }
      }
    }
  }
  return 1;
}
