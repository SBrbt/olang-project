extern fn posix_write(fd: i64, buf: ptr, n: i64) -> i64;

extern fn main() -> i32 {
  posix_write(1i64, "unused", 0i64);
  return 0;
}
