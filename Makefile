CC := gcc
CFLAGS := -std=c11 -Wall -Wextra -Wpedantic -O2

# olang/kasm/obinutils only need oobj; alinker also needs link_script.c
OOBJ_SRC := common/oobj.c
ALINKER_COMMON_SRC := common/oobj.c common/link_script.c
COMMON_INC := -Icommon
OLANG_SRC := \
	olang/src/ir.c \
	olang/src/frontend/lexer.c \
	olang/src/frontend/parser.c \
	olang/src/frontend/sema.c \
	olang/src/backend/x64_backend.c \
	olang/src/backend/codegen_x64.c \
	olang/src/backend/target_info/x64_target_info.c \
	olang/src/backend/target_info/target_registry.c \
	olang/src/backend/reloc/x64_reloc.c

BIN_DIR := bin

all: $(BIN_DIR)/alinker $(BIN_DIR)/kasm $(BIN_DIR)/olang $(BIN_DIR)/obinutils $(BIN_DIR)/olprep

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(BIN_DIR)/alinker: $(BIN_DIR) alinker/src/main.c alinker/src/link_core.c $(ALINKER_COMMON_SRC)
	$(CC) $(CFLAGS) $(COMMON_INC) -o $@ alinker/src/main.c alinker/src/link_core.c $(ALINKER_COMMON_SRC)

$(BIN_DIR)/kasm: $(BIN_DIR) kasm/src/main.c kasm/src/kasm_asm.c kasm/src/kasm_isa.c $(OOBJ_SRC)
	$(CC) $(CFLAGS) $(COMMON_INC) -Ikasm/src -o $@ kasm/src/main.c kasm/src/kasm_asm.c kasm/src/kasm_isa.c $(OOBJ_SRC)

$(BIN_DIR)/olang: $(BIN_DIR) olang/src/main.c $(OLANG_SRC) $(OOBJ_SRC)
	$(CC) $(CFLAGS) $(COMMON_INC) -o $@ olang/src/main.c $(OLANG_SRC) $(OOBJ_SRC)

$(BIN_DIR)/obinutils: $(BIN_DIR) obinutils/src/main.c $(OOBJ_SRC)
	$(CC) $(CFLAGS) $(COMMON_INC) -o $@ obinutils/src/main.c $(OOBJ_SRC)

$(BIN_DIR)/olprep: $(BIN_DIR) preproc/src/main.c
	$(CC) $(CFLAGS) -D_POSIX_C_SOURCE=200809L -o $@ preproc/src/main.c

clean:
	rm -rf $(BIN_DIR) examples/out

check: all check-link-script check-alinker check-kasm check-preproc check-olang
	@echo "OK: all checks passed"

check-preproc: $(BIN_DIR)/olprep
	bash tests/preproc/include.sh

check-olang: $(BIN_DIR)/olang
	bash tests/olang/run_programs_olc.sh
	bash tests/olang/check_olang_bounds.sh

check-alinker: $(BIN_DIR)/alinker
	bash tests/alinker/smoke.sh
	bash tests/alinker/pc64.sh
	bash tests/alinker/multi_obj.sh

check-kasm: $(BIN_DIR)/kasm $(BIN_DIR)/obinutils
	bash tests/kasm/label_comment.sh
	bash tests/kasm/bytes_tab.sh

$(BIN_DIR)/link_script_test: $(BIN_DIR) tests/link_script_test.c common/link_script.c
	$(CC) $(CFLAGS) $(COMMON_INC) -o $@ tests/link_script_test.c common/link_script.c

check-link-script: $(BIN_DIR)/link_script_test
	$(BIN_DIR)/link_script_test

.PHONY: all clean check check-link-script check-alinker check-kasm check-preproc check-olang
