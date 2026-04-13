// Same-width views via addr[name] + let (not cast on variables).
extern i32 main() {
  // i32 <-> u32 (both 4 bytes)
  let x stack[32, 42];
  let y <u32>x;
  if (load[y] != 42u32) { return 1; }

  // u32 -> b32 (both 4 bytes)
  let z <b32>y;
  if (load[z] != 42b32) { return 2; }

  // b32 -> i32
  let w <i32>z;
  if (load[w] != 42) { return 3; }

  // i64 <-> u64 (both 8 bytes)
  let a stack[64, 100i64];
  let b <u64>a;
  if (load[b] != 100u64) { return 4; }

  // u64 <-> ptr (both 8 bytes)
  let p <ptr>b;
  let c <u64>p;
  if (load[c] != 100u64) { return 5; }

  // i8 <-> u8 (both 1 byte)
  let s stack[8, 7i8];
  let t <u8>s;
  if (load[t] != 7u8) { return 6; }

  return 0;
}
