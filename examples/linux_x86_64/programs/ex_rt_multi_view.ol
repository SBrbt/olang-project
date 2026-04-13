// Two scalar views of one 64-bit blob: use a struct + fields (not types inside stack[...]).
type F32I32 = struct { x: f32, n: i32 };

extern i32 main() {
  let s_raw stack[64];
  let s <F32I32> s_raw;
  store[s.x, 1.0f32];
  store[s.n, 42i32];
  if (load[s.x] != 1.0f32) {
    return 1;
  }
  if (load[s.n] != 42i32) {
    return 2;
  }
  let x2 stack[32, load[s.x] * 2.0f32];
  let n2 stack[32, load[s.n] + 8i32];
  if (load[x2] != 2.0f32) {
    return 3;
  }
  if (load[n2] != 50i32) {
    return 4;
  }
  store[s.x, 3.0f32];
  if (load[s.n] != 42i32) {
    return 5;
  }
  if (load[s.x] != 3.0f32) {
    return 6;
  }
  return 0;
}
