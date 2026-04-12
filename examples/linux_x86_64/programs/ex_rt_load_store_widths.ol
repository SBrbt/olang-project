// load/store through addr for several scalar widths (not only i32).
extern i32 main() {
  let x8<i8> @stack<8>(-5i8);
  store<i8>(addr x8, 7i8);
  if (load<i8>(addr x8) != 7i8) {
    return 1;
  }

  let x16<i16> @stack<16>(1000i16);
  store<i16>(addr x16, 42i16);
  if (load<i16>(addr x16) != 42i16) {
    return 2;
  }

  let x64<i64> @stack<64>(123456789i64);
  store<i64>(addr x64, -1i64);
  if (load<i64>(addr x64) != -1i64) {
    return 3;
  }

  let u32v<u32> @stack<32>(0xFFFFFFFFu32);
  store<u32>(addr u32v, 0u32);
  if (load<u32>(addr u32v) != 0u32) {
    return 4;
  }

  return 0;
}
