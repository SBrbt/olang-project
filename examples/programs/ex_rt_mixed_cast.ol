extern fn main() -> i32 {
  let a: i64 = 1000i64;
  let b: i32 = cast<i32>(a);
  if (b == 1000) {
    return 0;
  }
  return 1;
}
