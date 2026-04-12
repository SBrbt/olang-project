// Test == on bN types.
extern i32 main() {
  let a<b32> @stack<32>(42b32);
  let b<b32> @stack<32>(42b32);
  let c<b32> @stack<32>(99b32);
  if (a == b) {} else { return 1; }
  if (a != c) {} else { return 2; }
  return 0;
}
