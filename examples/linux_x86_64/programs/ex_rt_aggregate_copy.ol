// Aggregate value copy: `store<dest, src>` copies bits between same-shaped aggregates.
type Pair = struct { a: i64, b: i64 };

extern i32 main() {
  let x<Pair> @stack<128>();
  let y<Pair> @stack<128>();
  store<x.a, 1i64>;
  store<x.b, 2i64>;
  store<y, x>;
  if (load<y.a> != 1i64) {
    return 1;
  }
  return 0;
}
