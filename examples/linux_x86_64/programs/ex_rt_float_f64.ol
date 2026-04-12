// Test f64 arithmetic and comparisons.
extern i32 main() {
  let a<f64> @stack<64>(3.14);
  let b<f64> @stack<64>(2.0);
  let sum<f64> @stack<64>(load<a> + load<b>);
  let diff<f64> @stack<64>(load<a> - load<b>);
  let prod<f64> @stack<64>(load<a> * load<b>);
  let quot<f64> @stack<64>(load<a> / load<b>);

  if (load<sum> > 5.0) {
    if (load<diff> > 1.0) {
      if (load<prod> > 6.0) {
        if (load<quot> > 1.5) {
          if (load<quot> < 1.6) {
            let neg<f64> @stack<64>(-load<a>);
            if (load<neg> < 0.0) {
              return 0;
            }
          }
        }
      }
    }
  }
  return 1;
}
