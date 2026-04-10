#include "../common/link_script.h"

#include <stdio.h>
#include <string.h>

#define ERR_SZ 256

static int fail(const char *msg) {
  fprintf(stderr, "link_script_test: %s\n", msg);
  return 1;
}

static int test_minimal_ok(void) {
  static const char *json =
      "{"
      "\"entry\": \"_start\","
      "\"prepend_call_stub\": false,"
      "\"segments\": ["
      "{\"file_off\":\"0x1000\",\"vaddr\":\"0x401000\",\"flags\":\"rx\",\"align\":\"0x1000\"},"
      "{\"file_off\":\"0x2000\",\"vaddr\":\"0x402000\",\"flags\":\"rw\",\"align\":\"0x1000\"}"
      "],"
      "\"layout\": ["
      "{\"op\":\"write_payload\",\"file_off\":\"0x1000\"}"
      "]"
      "}";
  LinkScript s;
  char err[ERR_SZ];
  if (!link_script_load_json_buffer(json, &s, err, sizeof(err))) {
    fprintf(stderr, "minimal: %s\n", err);
    link_script_free(&s);
    return 1;
  }
  if (strcmp(s.entry, "_start") != 0) return fail("minimal: bad entry");
  if (s.segment_count != 2u) return fail("minimal: segment_count (legacy two load groups need two segments)");
  if (s.layout_op_count != 1u) return fail("minimal: layout_op_count");
  if (s.load_page_align != 0) return fail("minimal: load_page_align should default 0");
  if (s.load_group_count != 2u) return fail("minimal: default load groups");
  if (strcmp(s.load_groups[0].section_names[0], ".text") != 0 || strcmp(s.load_groups[1].section_names[0], ".data") != 0) return fail("minimal: default section names");
  if (s.call_stub_len != 0u) return fail("minimal: call_stub_len");
  link_script_free(&s);
  return 0;
}

static int test_note_with_entry_substring(void) {
  static const char *json =
      "{"
      "\"note\": \"\\\"entry\\\": fake\","
      "\"entry\": \"_start\","
      "\"prepend_call_stub\": true,"
      "\"segments\": ["
      "{\"file_off\":\"0x1000\",\"vaddr\":\"0x401000\",\"flags\":\"rx\",\"align\":\"0x1000\"},"
      "{\"file_off\":\"0x2000\",\"vaddr\":\"0x402000\",\"flags\":\"rw\",\"align\":\"0x1000\"}"
      "],"
      "\"layout\": ["
      "{\"op\":\"write_payload\",\"file_off\":\"0x1000\"}"
      "]"
      "}";
  LinkScript s;
  char err[ERR_SZ];
  if (!link_script_load_json_buffer(json, &s, err, sizeof(err))) {
    fprintf(stderr, "note: %s\n", err);
    link_script_free(&s);
    return 1;
  }
  if (strcmp(s.entry, "_start") != 0) return fail("note: entry not _start");
  link_script_free(&s);
  return 0;
}

static int test_missing_entry(void) {
  static const char *json =
      "{"
      "\"prepend_call_stub\": false,"
      "\"segments\": [{\"file_off\":\"0x1000\",\"vaddr\":\"0x401000\",\"flags\":\"r\",\"align\":\"0x1000\"}],"
      "\"layout\": [{\"op\":\"write_payload\",\"file_off\":\"0x1000\"}]"
      "}";
  LinkScript s;
  char err[ERR_SZ];
  if (link_script_load_json_buffer(json, &s, err, sizeof(err))) {
    link_script_free(&s);
    return fail("missing entry: expected failure");
  }
  if (strstr(err, "entry") == NULL) return fail("missing entry: wrong error");
  link_script_free(&s);
  return 0;
}

static int test_missing_segments(void) {
  static const char *json =
      "{"
      "\"entry\": \"_start\","
      "\"prepend_call_stub\": false,"
      "\"layout\": [{\"op\":\"write_payload\",\"file_off\":\"0x1000\"}]"
      "}";
  LinkScript s;
  char err[ERR_SZ];
  if (link_script_load_json_buffer(json, &s, err, sizeof(err))) {
    link_script_free(&s);
    return fail("missing segments: expected failure");
  }
  if (strstr(err, "segments") == NULL) return fail("missing segments: wrong error");
  link_script_free(&s);
  return 0;
}

static int test_segment_nested_decoy_file_off(void) {
  static const char *json =
      "{"
      "\"entry\": \"_start\","
      "\"prepend_call_stub\": false,"
      "\"segments\": ["
      "{\"note\": \"\\\"file_off\\\": decoy\", \"file_off\":\"0x1000\",\"vaddr\":\"0x401000\",\"flags\":\"r\",\"align\":\"0x1000\"},"
      "{\"file_off\":\"0x2000\",\"vaddr\":\"0x402000\",\"flags\":\"rw\",\"align\":\"0x1000\"}"
      "],"
      "\"layout\": ["
      "{\"op\":\"write_payload\",\"file_off\":\"0x1000\"}"
      "]"
      "}";
  LinkScript s;
  char err[ERR_SZ];
  if (!link_script_load_json_buffer(json, &s, err, sizeof(err))) {
    fprintf(stderr, "seg_decoy: %s\n", err);
    link_script_free(&s);
    return 1;
  }
  if (s.segments[0].file_off != 0x1000ull) return fail("seg_decoy: file_off");
  if (s.segments[0].vaddr != 0x401000ull) return fail("seg_decoy: vaddr");
  link_script_free(&s);
  return 0;
}

static int test_layout_blob_nested_decoy_hex(void) {
  static const char *json =
      "{"
      "\"entry\": \"_start\","
      "\"prepend_call_stub\": false,"
      "\"segments\": ["
      "{\"file_off\":\"0x1000\",\"vaddr\":\"0x401000\",\"flags\":\"r\",\"align\":\"0x1000\"},"
      "{\"file_off\":\"0x2000\",\"vaddr\":\"0x402000\",\"flags\":\"rw\",\"align\":\"0x1000\"}"
      "],"
      "\"layout\": ["
      "{\"op\":\"write_blob\",\"file_off\":\"0x0\",\"note\":\"\\\"hex\\\":fake\",\"hex\":\"00\"}"
      "]"
      "}";
  LinkScript s;
  char err[ERR_SZ];
  if (!link_script_load_json_buffer(json, &s, err, sizeof(err))) {
    fprintf(stderr, "layout_decoy: %s\n", err);
    link_script_free(&s);
    return 1;
  }
  if (s.layout_op_count != 1u || s.layout_ops[0].kind != LINK_LAYOUT_OP_WRITE_BLOB) return fail("layout_decoy: op");
  if (s.layout_ops[0].blob_len != 1u || !s.layout_ops[0].blob || s.layout_ops[0].blob[0] != 0) return fail("layout_decoy: hex payload");
  link_script_free(&s);
  return 0;
}

static int test_entry_too_long(void) {
  char json[1024];
  char *p;
  size_t j;
  LinkScript s;
  char err[ERR_SZ];
  strcpy(json, "{\"entry\": \"");
  p = json + strlen(json);
  for (j = 0; j < 128; j++) *p++ = 'a';
  strcpy(p,
         "\",\"prepend_call_stub\": false,"
         "\"segments\": [{\"file_off\":\"0x1000\",\"vaddr\":\"0x401000\",\"flags\":\"r\",\"align\":\"0x1000\"},"
         "{\"file_off\":\"0x2000\",\"vaddr\":\"0x402000\",\"flags\":\"rw\",\"align\":\"0x1000\"}],"
         "\"layout\": [{\"op\":\"write_payload\",\"file_off\":\"0x1000\"}]}");
  if (link_script_load_json_buffer(json, &s, err, sizeof(err))) {
    link_script_free(&s);
    return fail("entry too long: expected failure");
  }
  if (strstr(err, "entry") == NULL && strstr(err, "too long") == NULL) return fail("entry too long: wrong error");
  link_script_free(&s);
  return 0;
}

static int test_explicit_rx_rw_sections(void) {
  static const char *json =
      "{"
      "\"entry\": \"_start\","
      "\"prepend_call_stub\": false,"
      "\"rx_sections\": [\".text\", \".rodata\"],"
      "\"rw_sections\": [\".data\", \".bss\"],"
      "\"segments\": ["
      "{\"file_off\":\"0x1000\",\"vaddr\":\"0x401000\",\"flags\":\"r\",\"align\":\"0x1000\"},"
      "{\"file_off\":\"0x2000\",\"vaddr\":\"0x402000\",\"flags\":\"rw\",\"align\":\"0x1000\"}"
      "],"
      "\"layout\": ["
      "{\"op\":\"write_payload\",\"file_off\":\"0x1000\"}"
      "]"
      "}";
  LinkScript s;
  char err[ERR_SZ];
  if (!link_script_load_json_buffer(json, &s, err, sizeof(err))) {
    fprintf(stderr, "rx_rw: %s\n", err);
    link_script_free(&s);
    return 1;
  }
  if (s.load_group_count != 2u) return fail("rx_rw: group count");
  if (strcmp(s.load_groups[0].section_names[0], ".text") != 0 || strcmp(s.load_groups[1].section_names[1], ".bss") != 0) return fail("rx_rw: names");
  link_script_free(&s);
  return 0;
}

static int test_call_stub_hex(void) {
  static const char *json =
      "{"
      "\"entry\": \"_start\","
      "\"prepend_call_stub\": false,"
      "\"call_stub_hex\": \"E80000000089C7B83C0000000F05\","
      "\"segments\": ["
      "{\"file_off\":\"0x1000\",\"vaddr\":\"0x401000\",\"flags\":\"r\",\"align\":\"0x1000\"},"
      "{\"file_off\":\"0x2000\",\"vaddr\":\"0x402000\",\"flags\":\"rw\",\"align\":\"0x1000\"}"
      "],"
      "\"layout\": ["
      "{\"op\":\"write_payload\",\"file_off\":\"0x1000\"}"
      "]"
      "}";
  LinkScript s;
  char err[ERR_SZ];
  static const unsigned char want[14] = {0xE8, 0, 0, 0, 0, 0x89, 0xC7, 0xB8, 0x3C, 0x00, 0x00, 0x00, 0x0F, 0x05};
  size_t j;
  if (!link_script_load_json_buffer(json, &s, err, sizeof(err))) {
    fprintf(stderr, "call_stub_hex: %s\n", err);
    link_script_free(&s);
    return 1;
  }
  if (s.call_stub_len != 14u) return fail("call_stub_hex: len");
  for (j = 0; j < 14u; j++) {
    if (s.call_stub_bytes[j] != want[j]) return fail("call_stub_hex: byte");
  }
  link_script_free(&s);
  return 0;
}

static int test_load_groups_explicit(void) {
  static const char *json =
      "{"
      "\"entry\": \"_start\","
      "\"prepend_call_stub\": false,"
      "\"load_groups\": ["
      "{\"sections\": [\".text\", \".rodata\"], \"align_after\": \"0x2000\"},"
      "{\"sections\": [\".data\", \".bss\"]}"
      "],"
      "\"segments\": ["
      "{\"file_off\":\"0x1000\",\"vaddr\":\"0x401000\",\"flags\":\"r\",\"align\":\"0x1000\"},"
      "{\"file_off\":\"0x2000\",\"vaddr\":\"0x402000\",\"flags\":\"rw\",\"align\":\"0x1000\"}"
      "],"
      "\"layout\": ["
      "{\"op\":\"write_payload\",\"file_off\":\"0x1000\"}"
      "]"
      "}";
  LinkScript s;
  char err[ERR_SZ];
  if (!link_script_load_json_buffer(json, &s, err, sizeof(err))) {
    fprintf(stderr, "load_groups: %s\n", err);
    link_script_free(&s);
    return 1;
  }
  if (s.load_group_count != 2u) return fail("load_groups: count");
  if (s.load_groups[0].align_after != 0x2000ull) return fail("load_groups: align_after");
  if (strcmp(s.load_groups[1].section_names[0], ".data") != 0) return fail("load_groups: g1");
  link_script_free(&s);
  return 0;
}

static int test_three_segments_parsed(void) {
  static const char *json =
      "{"
      "\"entry\": \"_start\","
      "\"prepend_call_stub\": false,"
      "\"segments\": ["
      "{\"file_off\":\"0x1000\",\"vaddr\":\"0x401000\",\"flags\":\"rx\",\"align\":\"0x1000\"},"
      "{\"file_off\":\"0x2000\",\"vaddr\":\"0x402000\",\"flags\":\"rw\",\"align\":\"0x1000\"},"
      "{\"file_off\":\"0x5000\",\"vaddr\":\"0x405000\",\"flags\":\"r\",\"align\":\"0x1000\"}"
      "],"
      "\"layout\": ["
      "{\"op\":\"write_payload\",\"file_off\":\"0x1000\"}"
      "]"
      "}";
  LinkScript s;
  char err[ERR_SZ];
  if (!link_script_load_json_buffer(json, &s, err, sizeof(err))) {
    fprintf(stderr, "three_seg: %s\n", err);
    link_script_free(&s);
    return 1;
  }
  if (s.segment_count != 3u) return fail("three_seg: count");
  if (s.segments[2].file_off != 0x5000ull || s.segments[2].vaddr != 0x405000ull) return fail("three_seg: seg2");
  link_script_free(&s);
  return 0;
}

static int test_load_page_align(void) {
  static const char *json =
      "{"
      "\"entry\": \"_start\","
      "\"prepend_call_stub\": false,"
      "\"load_page_align\": \"0x2000\","
      "\"segments\": ["
      "{\"file_off\":\"0x1000\",\"vaddr\":\"0x401000\",\"flags\":\"r\",\"align\":\"0x1000\"},"
      "{\"file_off\":\"0x2000\",\"vaddr\":\"0x402000\",\"flags\":\"rw\",\"align\":\"0x1000\"}"
      "],"
      "\"layout\": ["
      "{\"op\":\"write_payload\",\"file_off\":\"0x1000\"}"
      "]"
      "}";
  LinkScript s;
  char err[ERR_SZ];
  if (!link_script_load_json_buffer(json, &s, err, sizeof(err))) {
    fprintf(stderr, "align: %s\n", err);
    link_script_free(&s);
    return 1;
  }
  if (s.load_page_align != 0x2000ull) return fail("load_page_align value");
  link_script_free(&s);
  return 0;
}

static int test_segment_index_parse(void) {
  static const char *json =
      "{"
      "\"entry\": \"_start\","
      "\"prepend_call_stub\": false,"
      "\"load_groups\": ["
      "{\"sections\":[\".text\"],\"segment_index\":\"0\"},"
      "{\"sections\":[\".data\"],\"segment_index\":\"0\"}"
      "],"
      "\"segments\": ["
      "{\"file_off\":\"0x1000\",\"vaddr\":\"0x401000\",\"flags\":\"rx\",\"align\":\"0x1000\"},"
      "{\"file_off\":\"0x2000\",\"vaddr\":\"0x402000\",\"flags\":\"rw\",\"align\":\"0x1000\"}"
      "],"
      "\"layout\": [{\"op\":\"write_payload\",\"file_off\":\"0x1000\"}]"
      "}";
  LinkScript s;
  char err[ERR_SZ];
  if (!link_script_load_json_buffer(json, &s, err, sizeof(err))) {
    fprintf(stderr, "segment_index_parse: %s\n", err);
    link_script_free(&s);
    return 1;
  }
  if (s.load_groups[0].segment_index != 0 || s.load_groups[1].segment_index != 0) return fail("segment_index values");
  link_script_free(&s);
  return 0;
}

static int test_segment_index_range_error(void) {
  static const char *json =
      "{"
      "\"entry\": \"_start\","
      "\"prepend_call_stub\": false,"
      "\"load_groups\": [{\"sections\":[\".text\"],\"segment_index\":\"1\"}],"
      "\"segments\": ["
      "{\"file_off\":\"0x1000\",\"vaddr\":\"0x401000\",\"flags\":\"rx\",\"align\":\"0x1000\"}"
      "],"
      "\"layout\": [{\"op\":\"write_payload\",\"file_off\":\"0x1000\"}]"
      "}";
  LinkScript s;
  char err[ERR_SZ];
  if (link_script_load_json_buffer(json, &s, err, sizeof(err))) {
    link_script_free(&s);
    return fail("segment_index_range: expected failure");
  }
  if (strstr(err, "segment") == NULL) return fail("segment_index_range: wrong error");
  link_script_free(&s);
  return 0;
}

static int test_brace_inside_string(void) {
  static const char *json =
      "{"
      "\"entry\": \"_start\","
      "\"prepend_call_stub\": false,"
      "\"note\": \"a } brace and { inside\","
      "\"segments\": ["
      "{\"file_off\":\"0x1000\",\"vaddr\":\"0x401000\",\"flags\":\"rx\",\"align\":\"0x1000\"},"
      "{\"file_off\":\"0x2000\",\"vaddr\":\"0x402000\",\"flags\":\"rw\",\"align\":\"0x1000\"}"
      "],"
      "\"layout\": [{\"op\":\"write_payload\",\"file_off\":\"0x1000\"}]"
      "}";
  LinkScript s;
  char err[ERR_SZ];
  if (!link_script_load_json_buffer(json, &s, err, sizeof(err))) {
    fprintf(stderr, "brace_in_string: %s\n", err);
    link_script_free(&s);
    return 1;
  }
  if (strcmp(s.entry, "_start") != 0) return fail("brace_in_string: bad entry");
  link_script_free(&s);
  return 0;
}

int main(void) {
  if (test_minimal_ok()) return 1;
  if (test_note_with_entry_substring()) return 1;
  if (test_missing_entry()) return 1;
  if (test_missing_segments()) return 1;
  if (test_segment_nested_decoy_file_off()) return 1;
  if (test_layout_blob_nested_decoy_hex()) return 1;
  if (test_load_page_align()) return 1;
  if (test_explicit_rx_rw_sections()) return 1;
  if (test_call_stub_hex()) return 1;
  if (test_three_segments_parsed()) return 1;
  if (test_load_groups_explicit()) return 1;
  if (test_segment_index_parse()) return 1;
  if (test_segment_index_range_error()) return 1;
  if (test_brace_inside_string()) return 1;
  if (test_entry_too_long()) return 1;
  printf("OK: link_script_test\n");
  return 0;
}
