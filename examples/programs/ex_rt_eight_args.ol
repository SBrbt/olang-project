fn sum8(a: i64, b: i64, c: i64, d: i64, e: i64, f: i64, g: i64, h: i64) -> i64 {
  return a + b + c + d + e + f + g + h;
}

extern fn main() -> i32 {
  let s: i64 = sum8(1i64, 2i64, 3i64, 4i64, 5i64, 6i64, 7i64, 8i64);
  if (s == 36i64) {
    return 0;
  }
  return 1;
}
