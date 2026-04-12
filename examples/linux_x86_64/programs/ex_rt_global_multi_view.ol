// Same layout as ex_rt_multi_view.ol: f32 + i32 in one 64-bit .data blob.
let gx<f32> let gn<i32> @data<64>(0x0000002A3F800000u64);

extern i32 main() {
  if (load<gx> != 1.0f32) {
    return 1;
  }
  if (load<gn> != 42i32) {
    return 2;
  }
  let gx2<f32> @stack<32>(load<gx> * 2.0f32);
  let gn2<i32> @stack<32>(load<gn> + 8i32);
  if (load<gx2> != 2.0f32) {
    return 3;
  }
  if (load<gn2> != 50i32) {
    return 4;
  }
  store<gx, 3.0f32>;
  if (load<gn> != 42i32) {
    return 5;
  }
  if (load<gx> != 3.0f32) {
    return 6;
  }
  return 0;
}
