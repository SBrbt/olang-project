// Test bitwise NOT (~) and negation (-) on bN types.
extern i32 main() {
  let zero<b32> @stack<32>(reinterpret<b32>(0u32));

  // ~ on b32
  let a<b32> @stack<32>(reinterpret<b32>(0u32));
  let na<b32> @stack<32>(~a);
  if (na == zero) { return 1; }

  let b<b32> @stack<32>(255b32);
  let nb<b32> @stack<32>(~b);
  if (nb == b) { return 2; }

  // negation on b32 (two's complement)
  let c<b32> @stack<32>(1b32);
  let nc<b32> @stack<32>(-c);
  if (nc == c) { return 3; }

  return 0;
}
