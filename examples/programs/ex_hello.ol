// Stdout smoke: string literal is ptr (rodata address).
extern fn posix_write(fd: i64, buf: ptr, n: i64) -> i64;

extern fn main() -> i32 {
  posix_write(1i64, "Hello from OLang\n", 17i64);
  return 0;
}
