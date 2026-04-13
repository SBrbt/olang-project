// stack[...] may only introduce one name per placement; use chained let + ref or a struct.
extern i32 main() {
  let a let b stack[64, 0u64];
  return 0;
}
