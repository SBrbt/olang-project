// Aggregate value copy: `store[dest, src]` copies bits between same-shaped aggregates.
type Pair = struct { a: i64, b: i64 };

extern i32 main() {
  let x_raw stack[128];
  let x <Pair> x_raw;
  let y_raw stack[128];
  let y <Pair> y_raw;
  store[x.a, 1i64];
  store[x.b, 2i64];
  store[y, x];
  if (load[y.a] != 1i64) {
    return 1;
  }
  return 0;
}
