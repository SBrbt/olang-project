fn two() -> i64 { return 2i64; }
fn three() -> i64 { return 3i64; }
fn mul(a: i64, b: i64) -> i64 { return a * b; }

extern fn main() -> i32 {
  let p: i64 = mul(two(), three());
  if (p == 6i64) {
    return 0;
  }
  return 1;
}
