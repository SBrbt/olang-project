// b8: bitwise ops and ~ (other widths covered in ex_rt_binary_ops / ex_rt_b16_b64).
extern i32 main() {
  let a<b8> @stack<8>(240b8);
  let b<b8> @stack<8>(15b8);
  if ((a & b) != reinterpret<b8>(0u8)) {
    return 1;
  }
  if ((a | b) != 255b8) {
    return 2;
  }
  if ((a ^ b) != 255b8) {
    return 3;
  }
  let sh<b8> @stack<8>(1b8 << 3b8);
  if (sh != 8b8) {
    return 4;
  }
  let nb<b8> @stack<8>(~a);
  if (nb != 15b8) {
    return 5;
  }
  return 0;
}
