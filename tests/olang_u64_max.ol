// Test u64 literal with max value (2^64 - 1)
extern fn main() -> i32 {
  let a: u64 = 18446744073709551615u64;
  let b: u64 = 9223372036854775808u64;
  // a + 1 should wrap to 0 in u64
  if (a + 1u64 == 0u64 && b == 9223372036854775808u64) {
    return 0;
  }
  return 1;
}
