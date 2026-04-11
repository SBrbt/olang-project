// Test all comparison operators on unsigned types.
extern fn main() -> i32 {
  let a: u32 = 100u32;
  let b: u32 = 200u32;

  if (a < b) {} else { return 1; }
  if (a <= b) {} else { return 2; }
  if (b > a) {} else { return 3; }
  if (b >= a) {} else { return 4; }
  if (a == a) {} else { return 5; }
  if (a != b) {} else { return 6; }
  if (a <= a) {} else { return 7; }
  if (a >= a) {} else { return 8; }

  return 0;
}
