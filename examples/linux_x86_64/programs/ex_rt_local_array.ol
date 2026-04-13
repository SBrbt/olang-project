type Int3 = array(i32, 3);

extern i32 main() {
  let arr_raw stack[96];
  let arr <Int3> arr_raw;
  // Initialize via index
  store[arr[0], 10i32];
  store[arr[1], 20i32];
  store[arr[2], 30i32];
  // Sum and verify
  let sum stack[32, load[arr[0]] + load[arr[1]] + load[arr[2]]];
  // Return 0 if sum is 60 (expected), 1 otherwise
  if (load[sum] == 60i32) {
    return 0;
  }
  return 1;
}
