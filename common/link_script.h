#ifndef TOOLCHAIN_LINK_SCRIPT_H
#define TOOLCHAIN_LINK_SCRIPT_H

#include <stddef.h>
#include <stdint.h>

#define LINK_SCRIPT_MAX_SEGMENTS 8
#define LINK_SCRIPT_MAX_LAYOUT_OPS 64
#define LINK_SCRIPT_MAX_EXPR_LEN 80
#define LINK_SCRIPT_SECTION_NAME_CAP 64
#define LINK_SCRIPT_MAX_SECTIONS_PER_GROUP 16
#define LINK_SCRIPT_MAX_LOAD_GROUPS 8
#define LINK_SCRIPT_MAX_STUB_BYTES 256

typedef struct LinkSegment {
  uint64_t file_off;
  uint64_t vaddr;
  uint32_t p_flags;
  uint64_t align;
} LinkSegment;

typedef enum LinkLayoutOpKind {
  LINK_LAYOUT_OP_WRITE_BLOB = 1,
  LINK_LAYOUT_OP_WRITE_U32_LE = 2,
  LINK_LAYOUT_OP_WRITE_U64_LE = 3,
  LINK_LAYOUT_OP_WRITE_PAYLOAD = 4
} LinkLayoutOpKind;

typedef struct LinkLayoutOp {
  LinkLayoutOpKind kind;
  uint64_t file_off;
  unsigned char *blob;
  size_t blob_len;
  char value_expr[LINK_SCRIPT_MAX_EXPR_LEN];
} LinkLayoutOp;

/* One contiguous load chunk: sections in order, then optional alignment padding before the next group. */
typedef struct LinkLoadGroup {
  char section_names[LINK_SCRIPT_MAX_SECTIONS_PER_GROUP][LINK_SCRIPT_SECTION_NAME_CAP];
  size_t section_count;
  /* After this group's section bytes, pad so (file_offset from image start) aligns to this value before the next group.
   * 0 = use top-level load_page_align, or 0x1000 if that is also 0 (non-last groups only). Last group ignores trailing align. */
  uint64_t align_after;
  /* Maps this load chunk to segments[i] in the script (PT_LOAD). -1 means use the load group index i (group 0 -> segment 0). */
  int32_t segment_index;
} LinkLoadGroup;

typedef struct LinkScript {
  char **inputs;
  size_t input_count;
  char entry[128];
  LinkSegment segments[LINK_SCRIPT_MAX_SEGMENTS];
  size_t segment_count;
  int prepend_call_stub;
  uint64_t entry_vma;
  uint64_t load_page_align;
  unsigned char call_stub_bytes[LINK_SCRIPT_MAX_STUB_BYTES];
  size_t call_stub_len;
  LinkLoadGroup load_groups[LINK_SCRIPT_MAX_LOAD_GROUPS];
  size_t load_group_count;
  LinkLayoutOp layout_ops[LINK_SCRIPT_MAX_LAYOUT_OPS];
  size_t layout_op_count;
} LinkScript;

void link_script_init(LinkScript *s);
void link_script_free(LinkScript *s);
int link_script_load_json_buffer(const char *buf, LinkScript *out, char *err, size_t err_len);
int link_script_load_json(const char *path, LinkScript *out, char *err, size_t err_len);

#endif
