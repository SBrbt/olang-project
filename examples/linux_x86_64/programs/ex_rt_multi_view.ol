// Heterogeneous multi-binding on one 64-bit stack slot: f32 (low) + i32 (high).
// Init pattern: 1.0f32 at bytes 0..4, 42i32 at bytes 4..8 (little-endian u64 0x0000002A3F800000).
extern i32 main() {
  let x<f32> let n<i32> @stack<64>(0x0000002A3F800000u64);
  if (load<x> != 1.0f32) {
    return 1;
  }
  if (load<n> != 42i32) {
    return 2;
  }
  // Float-only and int-only uses of the two views
  let x2<f32> @stack<32>(load<x> * 2.0f32);
  let n2<i32> @stack<32>(load<n> + 8i32);
  if (load<x2> != 2.0f32) {
    return 3;
  }
  if (load<n2> != 50i32) {
    return 4;
  }
  // store<f32> only touches the low 32 bits; high half (n) stays 42
  store<x, 3.0f32>;
  if (load<n> != 42i32) {
    return 5;
  }
  if (load<x> != 3.0f32) {
    return 6;
  }
  return 0;
}
