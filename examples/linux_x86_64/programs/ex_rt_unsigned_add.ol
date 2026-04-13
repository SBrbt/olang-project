// Test + on unsigned types and bool == !=.
extern i32 main() {
  let a stack[32, 100u32];
  let b stack[32, 200u32];
  let s stack[32, load[a] + load[b]];
  if (load[s] != 300u32) { return 1; }

  let c stack[64, 50u64];
  let d stack[64, 75u64];
  let t stack[64, load[c] + load[d]];
  if (load[t] != 125u64) { return 2; }

  // bool == !=
  let x stack[8, true];
  let y stack[8, true];
  let z stack[8, false];
  if (load[x] == load[y]) {} else { return 3; }
  if (load[x] != load[z]) {} else { return 4; }

  return 0;
}
