// Forward extern declaration, then main calling it, then definition.
extern i32 callee();

extern i32 main() {
  return callee();
}

extern i32 callee() {
  return 0;
}
