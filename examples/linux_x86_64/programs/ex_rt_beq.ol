// Test == on bN types.
extern i32 main() {
  let a stack[32, 42b32];
  let b stack[32, 42b32];
  let c stack[32, 99b32];
  if (load[a] == load[b]) {} else { return 1; }
  if (load[a] != load[c]) {} else { return 2; }
  return 0;
}
