// Test aggregate type value copy semantics
type Pair = struct { a: i64, b: i64 };

extern fn main() -> i32 {
  let x: Pair;
  let y: Pair;
  x.a = 1i64;
  x.b = 2i64;
  y = x;        // Value copy, not reference
  x.b = 9i64;   // Modify x after copy
  // y should be independent of x
  if (y.a == 1i64 && y.b == 2i64) {
    return 0;
  }
  return 1;
}
