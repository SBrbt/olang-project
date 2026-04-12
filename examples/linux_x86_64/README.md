# Linux x86_64（本目录即平台包）

- **`olc`** — [`olc`](olc) 驱动脚本（bash：`olprep` → `olang` / `kasm` / `alinker`；`-I` 指向本目录下 `include/`）
- **`programs/`** — `ex_*.ol` 等示例源码（`make check` 会跑）
- **`include/`** — 如 `posix_abi.ol`（与 `libposix.kasm` 配套）
- **`out/`** — 本地构建输出目录（勿提交；`make` 清理会删）
- **`asm/runtime/`** — `krt.kasm`、`olrt.kasm`
- **`asm/lib/`** — `libposix.kasm`
- **`link/linux_elf_exe.json`** — Linux x86_64 **ELF 可执行文件** 链接脚本（`layout` / `call_stub_hex` 等全在脚本里）；见 `docs/internals/specs/elf-layout_zh.md`

裸机扁平映像示例见 **[`../bare_x86_64/`](../bare_x86_64/)**（与 Linux 无关）。
