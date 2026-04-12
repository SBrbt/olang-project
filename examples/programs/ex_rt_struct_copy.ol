// Test struct aggregate copy (assignment between struct variables).
type Vec2 = struct {
  x: i64,
  y: i64,
};

extern i32 main() {
  let a<Vec2> @stack<128>();
  a.x = 100i64;
  a.y = 200i64;

  let b<Vec2> @stack<128>();
  b = a;

  if (b.x != 100i64) { return 1; }
  if (b.y != 200i64) { return 2; }

  // Modify b, verify a unchanged
  b.x = 999i64;
  if (a.x != 100i64) { return 3; }
  if (b.x != 999i64) { return 4; }

  return 0;
}
