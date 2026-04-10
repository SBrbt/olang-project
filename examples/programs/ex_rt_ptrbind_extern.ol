extern fn posix_write(fd: i64, buf: ptr, n: i64) -> i64;

extern fn main() -> i32 {
  ptrbind pw as ptr from posix_write;
  return 0;
}
