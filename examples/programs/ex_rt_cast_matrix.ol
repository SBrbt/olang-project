// Test runtime cast<T> across different widths and type families.
extern i32 main() {
  // uN -> uN widening
  let a<u8> @stack<8>(200u8);
  let b<u32> @stack<32>(cast<u32>(a));
  if (b != 200u32) { return 1; }

  // uN -> uN narrowing
  let c<u32> @stack<32>(300u32);
  let d<u8> @stack<8>(cast<u8>(c));
  if (d != 44u8) { return 2; }

  // iN -> iN widening (sign extend)
  let e<i8> @stack<8>(-5i8);
  let f<i32> @stack<32>(cast<i32>(e));
  if (f != -5) { return 3; }

  // iN -> iN narrowing
  let g<i64> @stack<64>(257i64);
  let h<i8> @stack<8>(cast<i8>(g));
  if (h != 1i8) { return 4; }

  // bool -> i32
  let bt<bool> @stack<8>(true);
  let bf<bool> @stack<8>(false);
  let it<i32> @stack<32>(cast<i32>(bt));
  let iff<i32> @stack<32>(cast<i32>(bf));
  if (it != 1) { return 5; }
  if (iff != 0) { return 6; }

  // i32 -> bool
  let yes<bool> @stack<8>(cast<bool>(42));
  let no<bool> @stack<8>(cast<bool>(0));
  if (!yes) { return 7; }
  if (no) { return 8; }

  // bN -> bN widening
  let bx<b8> @stack<8>(171b8);
  let by<b32> @stack<32>(cast<b32>(bx));
  if (by != 171b32) { return 9; }

  return 0;
}
