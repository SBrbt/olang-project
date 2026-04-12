// Test cast between unsigned integers and floats + bN narrowing.
extern i32 main() {
  // u32 -> f64
  let a<u32> @stack<32>(42u32);
  let b<f64> @stack<64>(cast<f64>(a));
  if (b < 41.0) { return 1; }
  if (b > 43.0) { return 2; }

  // f64 -> u32
  let c<f64> @stack<64>(99.7);
  let d<u32> @stack<32>(cast<u32>(c));
  if (d != 99u32) { return 3; }

  // u64 -> f32
  let e<u64> @stack<64>(1000u64);
  let f<f32> @stack<32>(cast<f32>(e));
  if (f < 999.0f32) { return 4; }
  if (f > 1001.0f32) { return 5; }

  // bN narrowing: b32 -> b8
  let g<b32> @stack<32>(0b11111111b32);
  let h<b8> @stack<8>(cast<b8>(g));
  if (h != 255b8) { return 6; }

  return 0;
}
