// Test b32 type with bitwise operators and arithmetic.
extern fn main() -> i32 {
  let a: b32 = 65280b32;
  let b: b32 = 4080b32;

  // Bitwise AND: 65280 & 4080 = 3840
  let and_r: b32 = a & b;
  if (and_r != 3840b32) { return 1; }

  // Bitwise OR: 65280 | 4080 = 65520
  let or_r: b32 = a | b;
  if (or_r != 65520b32) { return 2; }

  // Bitwise XOR: 65280 ^ 4080 = 61680
  let xor_r: b32 = a ^ b;
  if (xor_r != 61680b32) { return 3; }

  // Left shift
  let shl_r: b32 = 1b32 << 8b32;
  if (shl_r != 256b32) { return 5; }

  // Right shift
  let shr_r: b32 = 256b32 >> 4b32;
  if (shr_r != 16b32) { return 6; }

  // Arithmetic on bN
  let c: b32 = 100b32;
  let d: b32 = 30b32;
  if (c + d != 130b32) { return 7; }
  if (c - d != 70b32) { return 8; }
  if (c * d != 3000b32) { return 9; }
  if (c / d != 3b32) { return 10; }
  if (c % d != 10b32) { return 11; }

  return 0;
}
