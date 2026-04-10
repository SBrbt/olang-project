extern fn main() -> i32 {
  let a: bool = true;
  let b: bool = false;
  let ok: bool = a && !b;
  let alt: bool = b || ok;
  if (ok && alt) {
    return 0;
  }
  return 1;
}
