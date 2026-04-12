// Test u64 literal with max value (2^64 - 1)
extern i32 main() {
  let a<u64> @stack<64>(18446744073709551615u64);
  let b<u64> @stack<64>(9223372036854775808u64);
  // a + 1 should wrap to 0 in u64
  if (a + 1u64 == 0u64 && b == 9223372036854775808u64) {
    return 0;
  }
  return 1;
}
