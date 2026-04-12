// Two names, two i32 views on one 64-bit stack slot.
extern i32 main() {
  let lo<i32> hi<i32> @stack<64>(0x0000000200000001u64);
  if (lo != 1i32) {
    return 1;
  }
  if (hi != 2i32) {
    return 2;
  }
  lo = 3i32;
  if (hi != 2i32) {
    return 3;
  }
  return 0;
}
