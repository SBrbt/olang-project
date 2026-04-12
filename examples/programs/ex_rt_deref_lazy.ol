// Lazy deref: `deref p as T` only binds; slot still holds ptr; reads/writes go through the pointer.
@data let mutcell: i32 = 0;

extern fn main() -> i32 {
  let p: ptr = addr mutcell;
  deref p as i32;
  p = 5;
  if (mutcell != 5) {
    return 1;
  }
  if (p != 5) {
    return 2;
  }
  return 0;
}
