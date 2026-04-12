// Test == != <= >= on f64 and f32.
extern i32 main() {
  // f64 == !=
  let a<f64> @stack<64>(1.0);
  let b<f64> @stack<64>(1.0);
  let c<f64> @stack<64>(2.0);
  if (load<a> == load<b>) {} else { return 1; }
  if (load<a> != load<c>) {} else { return 2; }
  if (load<a> <= load<b>) {} else { return 3; }
  if (load<c> >= load<a>) {} else { return 4; }

  // f32 == != <= >=
  let x<f32> @stack<32>(5.0f32);
  let y<f32> @stack<32>(5.0f32);
  let z<f32> @stack<32>(10.0f32);
  if (load<x> == load<y>) {} else { return 5; }
  if (load<x> != load<z>) {} else { return 6; }
  if (load<x> <= load<y>) {} else { return 7; }
  if (load<z> >= load<x>) {} else { return 8; }

  return 0;
}
