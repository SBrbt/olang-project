// Custom ELF section for this global (writable init); link script must list it (e.g. .ol_custom in linux_elf_exe.json).
let gsec<i32> @section(".ol_custom")<32>(42);

extern i32 main() {
  if (gsec != 42) {
    return 1;
  }
  return 0;
}
