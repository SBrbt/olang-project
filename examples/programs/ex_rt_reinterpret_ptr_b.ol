// Test reinterpret between ptr<->b64 and i64<->ptr.
extern fn main() -> i32 {
  // ptr <-> b64
  let a: u64 = 12345u64;
  let p: ptr = reinterpret<ptr>(a);
  let b: b64 = reinterpret<b64>(p);
  let c: u64 = reinterpret<u64>(p);
  if (c != 12345u64) { return 1; }

  // i64 <-> ptr
  let d: i64 = 99i64;
  let q: ptr = reinterpret<ptr>(reinterpret<u64>(d));
  let e: u64 = reinterpret<u64>(q);
  if (e != 99u64) { return 2; }

  return 0;
}
