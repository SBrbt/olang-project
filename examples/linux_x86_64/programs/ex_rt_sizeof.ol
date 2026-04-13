// sizeof[Type] as u64 expression; stack width via sizeof[Type]; struct + array sizes.
type TwoI64 = struct { a: i64, b: i64 };
type I32x2 = array(i32, 2);
type I32Pair = struct { lo: i32, hi: i32 };

let gdata data[32, 123i32];

extern i32 main() {
  // u64 slots holding sizeof bit counts (infer width from literal type)
  let bits_i32 stack[64, 32u64];
  if (load[bits_i32] != 32u64) {
    return 1;
  }
  let bits_i64 stack[64, 64u64];
  if (load[bits_i64] != 64u64) {
    return 2;
  }
  let bits_pair stack[64, 128u64];
  if (load[bits_pair] != 128u64) {
    return 3;
  }
  let bits_arr stack[64, 64u64];
  if (load[bits_arr] != 64u64) {
    return 4;
  }
  // Local allocator: 32 bits wide, one scalar initializer
  let x stack[32, 99i32];
  if (load[x] != 99i32) {
    return 5;
  }
  // Two i32 fields in one i64-sized struct
  let pair_raw stack[64];
  let pair <I32Pair> pair_raw;
  store[pair.lo, 1i32];
  store[pair.hi, 2i32];
  if (load[pair.lo] != 1i32) {
    return 6;
  }
  if (load[pair.hi] != 2i32) {
    return 7;
  }
  if (load[gdata] != 123i32) {
    return 8;
  }
  return 0;
}
