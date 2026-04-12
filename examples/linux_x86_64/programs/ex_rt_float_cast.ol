// Test cast between float and integer types.
extern i32 main() {
  // i64 -> f64
  let a<f64> @stack<64>(f64<42i64>);
  if (load<a> < 41.0) { return 1; }
  if (load<a> > 43.0) { return 2; }

  // f64 -> i64 (truncation)
  let b<i64> @stack<64>(i64<3.99>);
  if (load<b> != 3i64) { return 3; }

  // i32 -> f32
  let c<f32> @stack<32>(f32<100>);
  if (load<c> < 99.0f32) { return 4; }
  if (load<c> > 101.0f32) { return 5; }

  // f32 -> i32 (truncation)
  let d<i32> @stack<32>(i32<7.9f32>);
  if (load<d> != 7) { return 6; }

  // f32 -> f64 widening
  let e<f32> @stack<32>(1.5f32);
  let f<f64> @stack<64>(f64<(load<e>)>);
  if (load<f> < 1.4) { return 7; }
  if (load<f> > 1.6) { return 8; }

  // f64 -> f32 narrowing
  let g<f64> @stack<64>(2.5);
  let h<f32> @stack<32>(f32<(load<g>)>);
  if (load<h> < 2.4f32) { return 9; }
  if (load<h> > 2.6f32) { return 10; }

  return 0;
}
