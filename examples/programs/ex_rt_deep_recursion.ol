fn fib(n: i32) -> i32 {
  if (n < 2) {
    return n;
  }
  let a: i32 = fib(n - 1);
  return fib(n - 2) + a;
}

extern fn main() -> i32 {
  if (fib(10) == 55) {
    return 0;
  }
  return 1;
}
