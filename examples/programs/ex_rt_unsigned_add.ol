// Test + on unsigned types and bool == !=.
extern fn main() -> i32 {
  let a: u32 = 100u32;
  let b: u32 = 200u32;
  let s: u32 = a + b;
  if (s != 300u32) { return 1; }

  let c: u64 = 50u64;
  let d: u64 = 75u64;
  let t: u64 = c + d;
  if (t != 125u64) { return 2; }

  // bool == !=
  let x: bool = true;
  let y: bool = true;
  let z: bool = false;
  if (x == y) {} else { return 3; }
  if (x != z) {} else { return 4; }

  return 0;
}
