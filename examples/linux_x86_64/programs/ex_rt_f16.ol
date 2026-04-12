// f16 literals and casts; f16+f16 is not lowered as half-precision math (see codegen),
// so combine via f32<…> for arithmetic.
extern i32 main() {
  let a<f16> @stack<16>(1.0f16);
  let as_f32<f32> @stack<32>(f32<(load<a>)>);
  let as_i<i32> @stack<32>(i32<(load<as_f32>)>);
  if (load<as_i> != 1) {
    return 1;
  }
  let x<f16> @stack<16>(2.5f16);
  let y<f16> @stack<16>(0.5f16);
  let xf<f32> @stack<32>(f32<(load<x>)>);
  let yf<f32> @stack<32>(f32<(load<y>)>);
  let sum<f32> @stack<32>(load<xf> + load<yf>);
  if (load<sum> < 2.9f32) {
    return 2;
  }
  if (load<sum> > 3.1f32) {
    return 3;
  }
  let back<f16> @stack<16>(f16<(load<sum>)>);
  let chk<f32> @stack<32>(f32<(load<back>)>);
  if (load<chk> < 2.9f32) {
    return 4;
  }
  if (load<chk> > 3.1f32) {
    return 5;
  }
  return 0;
}
