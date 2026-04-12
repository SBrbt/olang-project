// Test f32 arithmetic and comparisons.
extern i32 main() {
  let a<f32> @stack<32>(10.5f32);
  let b<f32> @stack<32>(3.5f32);
  let sum<f32> @stack<32>(load<a> + load<b>);
  let diff<f32> @stack<32>(load<a> - load<b>);
  let prod<f32> @stack<32>(load<a> * load<b>);
  let quot<f32> @stack<32>(load<a> / load<b>);

  if (load<sum> == 14.0f32) {
    if (load<diff> == 7.0f32) {
      if (load<prod> > 36.0f32) {
        if (load<quot> == 3.0f32) {
          return 0;
        }
      }
    }
  }
  return 1;
}
