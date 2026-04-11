// Test == on bN types.
extern fn main() -> i32 {
  let a: b32 = 42b32;
  let b: b32 = 42b32;
  let c: b32 = 99b32;
  if (a == b) {} else { return 1; }
  if (a != c) {} else { return 2; }
  return 0;
}
