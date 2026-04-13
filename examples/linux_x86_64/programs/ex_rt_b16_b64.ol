// Test b16 and b64 types with bitwise ops, arithmetic, and literals.
extern i32 main() {
  // b16
  let a stack[16, 255b16];
  let b stack[16, 15b16];
  let and16 stack[16, load[a] & load[b]];
  if (load[and16] != 15b16) { return 1; }
  let shl16 stack[16, 1b16 << 4b16];
  if (load[shl16] != 16b16) { return 2; }
  let sum16 stack[16, load[a] + load[b]];
  if (load[sum16] != 270b16) { return 3; }

  // b64
  let c stack[64, 1000b64];
  let d stack[64, 7b64];
  let or64 stack[64, load[c] | load[d]];
  if (load[or64] != 1007b64) { return 4; }
  let xor64 stack[64, load[c] ^ load[c]];
  let zero64 stack[64, 1b64 ^ 1b64];
  if (load[xor64] != load[zero64]) { return 5; }
  let prod64 stack[64, load[c] * 3b64];
  if (load[prod64] != 3000b64) { return 6; }

  return 0;
}
