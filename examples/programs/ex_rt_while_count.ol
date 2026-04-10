// Phase1 has no reassignment; countdown uses recursion (not a mutating while).
fn down(n: i32) -> i32 {
  if (n == 0) {
    return 0;
  }
  return down(n - 1);
}

extern fn main() -> i32 {
  return down(25);
}
