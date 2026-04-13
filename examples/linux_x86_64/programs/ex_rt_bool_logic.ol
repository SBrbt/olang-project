extern i32 main() {
  let a stack[8, true];
  let b stack[8, false];
  let ok stack[8, load[a] && !load[b]];
  let alt stack[8, load[b] || load[ok]];
  if (load[ok] && load[alt]) {
    return 0;
  }
  return 1;
}
