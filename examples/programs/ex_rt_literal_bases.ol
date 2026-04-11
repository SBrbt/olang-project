// Test binary (0b) and octal (0o) literals with various suffixes.
extern fn main() -> i32 {
  // Binary literal
  let bin: i32 = 0b1010;
  if (bin != 10) { return 1; }

  // Octal literal
  let oct: i32 = 0o77;
  if (oct != 63) { return 2; }

  // Binary literal with bN suffix
  let bval: b32 = 0b11110000b32;
  if (bval != 240b32) { return 3; }

  // Hex with various suffixes
  let h8: u8 = 0xFFu8;
  if (h8 != 255u8) { return 4; }

  let h16: u16 = 0xABCDu16;
  if (h16 != 43981u16) { return 5; }

  let h64: u64 = 0xDEADBEEFu64;
  if (h64 != 3735928559u64) { return 6; }

  return 0;
}
