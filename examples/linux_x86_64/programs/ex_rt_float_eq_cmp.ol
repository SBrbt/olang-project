// Test == != <= >= on f64 and f32.
extern i32 main() {
  // f64 == !=
  let a<f64> @stack<64>(1.0);
  let b<f64> @stack<64>(1.0);
  let c<f64> @stack<64>(2.0);
  if (a == b) {} else { return 1; }
  if (a != c) {} else { return 2; }
  if (a <= b) {} else { return 3; }
  if (c >= a) {} else { return 4; }

  // f32 == != <= >=
  let x<f32> @stack<32>(5.0f32);
  let y<f32> @stack<32>(5.0f32);
  let z<f32> @stack<32>(10.0f32);
  if (x == y) {} else { return 5; }
  if (x != z) {} else { return 6; }
  if (x <= y) {} else { return 7; }
  if (z >= x) {} else { return 8; }

  return 0;
}
