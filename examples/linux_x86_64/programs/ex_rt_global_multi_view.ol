// Two global names, two i32 views on one 64-bit .data blob.
let glo_lo<i32> glo_hi<i32> @data<64>(0x0000000200000001u64);

extern i32 main() {
  if (glo_lo != 1i32) {
    return 1;
  }
  if (glo_hi != 2i32) {
    return 2;
  }
  glo_lo = 3i32;
  if (glo_hi != 2i32) {
    return 3;
  }
  return 0;
}
