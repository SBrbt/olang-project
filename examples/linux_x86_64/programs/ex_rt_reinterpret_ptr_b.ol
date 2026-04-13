// ptr <-> b64 / u64 / i64 views via <T>addr (same bits, different element types).
extern i32 main() {
  let a stack[64, 12345u64];
  let p <ptr>a;
  let b <b64>p;
  let c <u64>p;
  if (load[c] != 12345u64) { return 1; }
  if (load[b] != 12345b64) { return 3; }

  let d stack[64, 99i64];
  let q <ptr>d;
  let e <u64>q;
  if (load[e] != 99u64) { return 2; }

  return 0;
}
