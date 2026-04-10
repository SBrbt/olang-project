extern fn main() -> i32 {
  let a: i32 = 123i32;
  let b: i64 = 456i64;
  if (a == 123) {
    if (b == 456i64) {
      return 0;
    }
  }
  return 1;
}
