// Test runtime T<expr> conversion across widths and type families.
extern i32 main() {
  // uN -> uN widening
  let a<u8> @stack<8>(200u8);
  let b<u32> @stack<32>(u32<(load<a>)>);
  if (load<b> != 200u32) { return 1; }

  // uN -> uN narrowing
  let c<u32> @stack<32>(300u32);
  let d<u8> @stack<8>(u8<(load<c>)>);
  if (load<d> != 44u8) { return 2; }

  // iN -> iN widening (sign extend)
  let e<i8> @stack<8>(42i8);
  let f<i32> @stack<32>(i32<(load<e>)>);
  if (load<f> != 42) { return 3; }

  // iN -> iN narrowing
  let g<i64> @stack<64>(257i64);
  let h<i8> @stack<8>(i8<(load<g>)>);
  if (load<h> != 1i8) { return 4; }

  // bool -> i32
  let bt<bool> @stack<8>(true);
  let bf<bool> @stack<8>(false);
  let it<i32> @stack<32>(i32<(load<bt>)>);
  let iff<i32> @stack<32>(i32<(load<bf>)>);
  if (load<it> != 1) { return 5; }
  if (load<iff> != 0) { return 6; }

  // i32 -> bool
  let yes<bool> @stack<8>(bool<42>);
  let no<bool> @stack<8>(bool<0>);
  if (!load<yes>) { return 7; }
  if (load<no>) { return 8; }

  // bN -> bN widening
  let bx<b8> @stack<8>(171b8);
  let by<b32> @stack<32>(b32<(load<bx>)>);
  if (load<by> != 171b32) { return 9; }

  return 0;
}
