// Test struct aggregate copy (assignment between struct variables).
type Vec2 = struct {
  x: i64,
  y: i64,
};

extern i32 main() {
  let a_raw stack[128];
  let a <Vec2> a_raw;
  store[a.x, 100i64];
  store[a.y, 200i64];

  let b_raw stack[128];
  let b <Vec2> b_raw;
  store[b, a];

  if (load[b.x] != 100i64) { return 1; }

  // Modify b, verify a unchanged
  store[b.x, 999i64];
  if (load[a.x] != 100i64) { return 3; }
  if (load[b.x] != 999i64) { return 4; }

  return 0;
}
