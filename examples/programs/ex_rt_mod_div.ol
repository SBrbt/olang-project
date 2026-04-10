extern fn main() -> i32 {
  let q: i64 = 17i64 / 5i64;
  let r: i64 = 17i64 % 5i64;
  if (q == 3i64) {
    if (r == 2i64) {
      return 0;
    }
  }
  return 1;
}
