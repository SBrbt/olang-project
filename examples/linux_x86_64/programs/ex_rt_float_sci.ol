// Floating literals with exponent notation (see lexer / float parsing).
extern i32 main() {
  let a stack[64, 1e2];
  if (load[a] < 99.9) {
    return 1;
  }
  if (load[a] > 100.1) {
    return 2;
  }
  let b stack[32, 1e-3f32];
  if (load[b] < 0.0009f32) {
    return 3;
  }
  if (load[b] > 0.0011f32) {
    return 4;
  }
  let c stack[64, 2.5e+1];
  if (load[c] < 24.9) {
    return 5;
  }
  if (load[c] > 25.1) {
    return 6;
  }
  return 0;
}
