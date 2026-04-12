// Test bitwise NOT (~) and negation (-) on bN types.
extern i32 main() {
  let zero<b32> @stack<32>(b32<0u32>);

  // ~ on b32
  let a<b32> @stack<32>(b32<0u32>);
  let na<b32> @stack<32>(~load<a>);
  if (load<na> == load<zero>) { return 1; }

  let b<b32> @stack<32>(255b32);
  let nb<b32> @stack<32>(~load<b>);
  if (load<nb> == load<b>) { return 2; }

  // negation on b32 (two's complement)
  let c<b32> @stack<32>(1b32);
  let nc<b32> @stack<32>(-load<c>);
  if (load<nc> == load<c>) { return 3; }

  return 0;
}
