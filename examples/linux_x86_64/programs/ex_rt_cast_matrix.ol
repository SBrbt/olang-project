// Test runtime T(expr) conversion across widths and type families (i/u/b/f only).
extern i32 main() {
  // uN -> uN widening
  let a stack[8, 200u8];
  let b stack[32, u32((load[a]))];
  if (load[b] != 200u32) { return 1; }

  // uN -> uN narrowing
  let c stack[32, 300u32];
  let d stack[8, u8((load[c]))];
  if (load[d] != 44u8) { return 2; }

  // iN -> iN widening (sign extend)
  let e stack[8, 42i8];
  let f stack[32, i32((load[e]))];
  if (load[f] != 42) { return 3; }

  // iN -> iN narrowing
  let g stack[64, 257i64];
  let h stack[8, i8((load[g]))];
  if (load[h] != 1i8) { return 4; }

  // bN -> bN widening
  let bx stack[8, 171b8];
  let by stack[32, b32((load[bx]))];
  if (load[by] != 171b32) { return 5; }

  return 0;
}
