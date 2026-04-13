// Test b32 type with bitwise operators and arithmetic.
extern i32 main() {
  let a stack[32, 65280b32];
  let b stack[32, 4080b32];

  // Bitwise AND: 65280 & 4080 = 3840
  let and_r stack[32, load[a] & load[b]];
  if (load[and_r] != 3840b32) { return 1; }

  // Bitwise OR: 65280 | 4080 = 65520
  let or_r stack[32, load[a] | load[b]];
  if (load[or_r] != 65520b32) { return 2; }

  // Bitwise XOR: 65280 ^ 4080 = 61680
  let xor_r stack[32, load[a] ^ load[b]];
  if (load[xor_r] != 61680b32) { return 3; }

  // Left shift
  let shl_r stack[32, 1b32 << 8b32];
  if (load[shl_r] != 256b32) { return 5; }

  // Right shift
  let shr_r stack[32, 256b32 >> 4b32];
  if (load[shr_r] != 16b32) { return 6; }

  // Arithmetic on bN
  let c stack[32, 100b32];
  let d stack[32, 30b32];
  if (load[c] + load[d] != 130b32) { return 7; }
  if (load[c] - load[d] != 70b32) { return 8; }
  if (load[c] * load[d] != 3000b32) { return 9; }
  if (load[c] / load[d] != 3b32) { return 10; }
  if (load[c] % load[d] != 10b32) { return 11; }

  return 0;
}
