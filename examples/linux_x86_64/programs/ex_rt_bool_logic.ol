extern i32 main() {
  let a<bool> @stack<8>(true);
  let b<bool> @stack<8>(false);
  let ok<bool> @stack<8>(load<a> && !load<b>);
  let alt<bool> @stack<8>(load<b> || load<ok>);
  if (load<ok> && load<alt>) {
    return 0;
  }
  return 1;
}
