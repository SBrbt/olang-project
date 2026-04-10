# Linux x86_64（`olc` 固定读这里）

- **`asm/runtime/`** — `krt.kasm`、`olrt.kasm`
- **`asm/lib/`** — `libposix.kasm`
- **`link/exe.json`** — `alinker` 脚本：`segments` + **`elf`**（ELF 头/Phdr 类型等，不由链接器内置常量决定）；见 `specs/elf-layout.md`
- **驱动脚本** — [`../olc`](../olc)（bash：调用 `bin/olang`、`kasm`、`alinker` 并读本目录下的 `link/`、`asm/`）
