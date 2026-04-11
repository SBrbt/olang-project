// Test struct aggregate copy (assignment between struct variables).
type Vec2 = struct {
  x: i64,
  y: i64,
};

extern fn main() -> i32 {
  let a: Vec2;
  a.x = 100i64;
  a.y = 200i64;

  let b: Vec2;
  b = a;

  if (b.x != 100i64) { return 1; }
  if (b.y != 200i64) { return 2; }

  // Modify b, verify a unchanged
  b.x = 999i64;
  if (a.x != 100i64) { return 3; }
  if (b.x != 999i64) { return 4; }

  return 0;
}
