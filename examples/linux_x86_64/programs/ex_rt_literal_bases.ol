// Test binary (0b) and octal (0o) literals with various suffixes.
extern i32 main() {
  // Binary literal
  let bin<i32> @stack<32>(0b1010);
  if (load<bin> != 10) { return 1; }

  // Octal literal
  let oct<i32> @stack<32>(0o77);
  if (load<oct> != 63) { return 2; }

  // Binary literal with bN suffix
  let bval<b32> @stack<32>(0b11110000b32);
  if (load<bval> != 240b32) { return 3; }

  // Hex with various suffixes
  let h8<u8> @stack<8>(0xFFu8);
  if (load<h8> != 255u8) { return 4; }

  let h16<u16> @stack<16>(0xABCDu16);
  if (load<h16> != 43981u16) { return 5; }

  let h64<u64> @stack<64>(0xDEADBEEFu64);
  if (load<h64> != 3735928559u64) { return 6; }

  return 0;
}
