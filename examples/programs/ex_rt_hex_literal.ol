extern fn main() -> i32 {
  let x: i64 = 0xFFi64;
  if (x == 255i64) {
    return 0;
  }
  return 1;
}
