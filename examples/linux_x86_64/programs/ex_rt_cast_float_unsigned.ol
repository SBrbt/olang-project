// Test cast between unsigned integers and floats + bN narrowing.
extern i32 main() {
  // u32 -> f64
  let a stack[32, 42u32];
  let b stack[64, f64((load[a]))];
  if (load[b] < 41.0) { return 1; }
  if (load[b] > 43.0) { return 2; }

  // f64 -> u32
  let c stack[64, 99.7];
  let d stack[32, u32((load[c]))];
  if (load[d] != 99u32) { return 3; }

  // u64 -> f32
  let e stack[64, 1000u64];
  let f stack[32, f32((load[e]))];
  if (load[f] < 999.0f32) { return 4; }
  if (load[f] > 1001.0f32) { return 5; }

  // bN narrowing: b32 -> b8
  let g stack[32, 0b11111111b32];
  let h stack[8, b8((load[g]))];
  if (load[h] != 255b8) { return 6; }

  return 0;
}
