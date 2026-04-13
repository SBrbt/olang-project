// Test bitwise NOT (~) and negation (-) on bN types.
extern i32 main() {
  let zero stack[32, 1b32 ^ 1b32];

  // ~ on b32
  let a stack[32, 1b32 ^ 1b32];
  let na stack[32, ~load[a]];
  if (load[na] == load[zero]) { return 1; }

  let b stack[32, 255b32];
  let nb stack[32, ~load[b]];
  if (load[nb] == load[b]) { return 2; }

  // negation on b32 (two's complement)
  let c stack[32, 1b32];
  let nc stack[32, -load[c]];
  if (load[nc] == load[c]) { return 3; }

  return 0;
}
