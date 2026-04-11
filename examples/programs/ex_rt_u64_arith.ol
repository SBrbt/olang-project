// Test u64 arithmetic, comparisons, and large values.
extern fn main() -> i32 {
  let a: u64 = 1000000u64;
  let b: u64 = 999999u64;
  let big: u64 = 18446744073709551615u64;

  if (a > b) {
    if (a - b == 1u64) {
      if (a * 2u64 == 2000000u64) {
        if (a / 10u64 == 100000u64) {
          if (a % 3u64 == 1u64) {
            if (big != 0u64) {
              return 0;
            }
          }
        }
      }
    }
  }
  return 1;
}
