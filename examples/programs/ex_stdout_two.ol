// Two writes + reassignment to locals (stdout order AB).
extern fn posix_write(fd: i64, buf: ptr, n: i64) -> i64;

extern fn main() -> i32 {
  let a: ptr = "A";
  posix_write(1i64, a, 1i64);
  a = "B";
  posix_write(1i64, a, 1i64);
  return 0;
}
