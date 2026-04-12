#include "codegen_x64.h"

#include "../frontend/ast.h"
#include "../ir.h"
#include "reloc/x64_reloc.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FRAME_SLOTS 256

/* Bytes below saved rbx: slot s lives at rbp-8*(s+2); need sub rsp >= 8*num_slots, 16-byte aligned. */
static int32_t frame_bytes_for_slot_count(int num_slots) {
  if (num_slots <= 0) return 0;
  {
    uint64_t need = 8ull * (uint64_t)num_slots;
    return (int32_t)((need + 15ull) & ~15ull);
  }
}

typedef struct CG {
  OlProgram *p;
  OobjObject *obj;
  uint8_t *tx;
  size_t tx_len;
  uint8_t *ro;
  size_t ro_len;
  uint8_t *data;
  size_t data_len;
  uint8_t *bss;
  size_t bss_len;
  char err[256];

  int next_slot;
  struct {
    char name[128];
    int slot;
    OlTypeRef ty;  /* Element / value type; when indirect, slot holds ptr to this type */
    unsigned char indirect;
  } loc[256];
  int nloc;

  struct {
    size_t patch;
    int lbl;
    int insn_len; /* bytes from patch to next insn */
  } fix[128];
  int nfix;

  size_t lbl_pos[64];
  int lbl_set[64];
  int nlbl;
  int failed;
  int loop_head[16];
  int loop_done[16];
  int loop_depth;
  int fn_exit_lbl; /* unified return target during gen_function */
} CG;

static uint32_t type_size_bytes(const OlProgram *p, const OlTypeRef *t) {
  switch (t->kind) {
    case OL_TY_VOID:
      return 0;
    case OL_TY_BOOL:
    case OL_TY_U8:
    case OL_TY_I8:
    case OL_TY_B8:
      return 1u;
    case OL_TY_U16:
    case OL_TY_I16:
    case OL_TY_F16:
    case OL_TY_B16:
      return 2u;
    case OL_TY_I32:
    case OL_TY_U32:
    case OL_TY_F32:
    case OL_TY_B32:
      return 4u;
    case OL_TY_I64:
    case OL_TY_U64:
    case OL_TY_F64:
    case OL_TY_B64:
    case OL_TY_PTR:
      return 8u;
    case OL_TY_ALIAS: {
      int k = ol_program_find_typedef(p, t->alias_name);
      if (k < 0) return 0;
      return p->typedefs[k].size_bytes;
    }
  }
  return 0;
}

static int type_is_u32_like(const OlTypeRef *t) { return t->kind == OL_TY_U32; }
static int type_is_i32_like(const OlTypeRef *t) { return t->kind == OL_TY_I32; }
static int type_is_byte_like(const OlTypeRef *t) { return t->kind == OL_TY_BOOL || t->kind == OL_TY_U8 || t->kind == OL_TY_I8 || t->kind == OL_TY_B8; }
static int type_is_word_like(const OlTypeRef *t) { return t->kind == OL_TY_U16 || t->kind == OL_TY_I16 || t->kind == OL_TY_B16; }
static int type_is_unsigned_like(const OlTypeRef *t) {
  return t->kind == OL_TY_U8 || t->kind == OL_TY_U16 || t->kind == OL_TY_U32 || t->kind == OL_TY_U64 ||
         t->kind == OL_TY_B8 || t->kind == OL_TY_B16 || t->kind == OL_TY_B32 || t->kind == OL_TY_B64;
}
static int type_is_float(const OlTypeRef *t) { return t->kind == OL_TY_F16 || t->kind == OL_TY_F32 || t->kind == OL_TY_F64; }

static uint16_t f32_to_f16_bits(float fval) {
  union { float f; uint32_t i; } u;
  uint32_t bits, sign, exp, mant;
  u.f = fval;
  bits = u.i;
  sign = (bits >> 16) & 0x8000u;
  exp = (bits >> 23) & 0xffu;
  mant = bits & 0x7fffffu;
  if (exp == 0xff) return (uint16_t)(sign | 0x7c00u | (mant ? 0x200u : 0u));
  if (exp > 142) return (uint16_t)(sign | 0x7c00u);
  if (exp < 113) return (uint16_t)sign;
  return (uint16_t)(sign | (uint16_t)((exp - 112u) << 10) | (uint16_t)(mant >> 13));
}

static void cg_err(CG *g, const char *m) { snprintf(g->err, sizeof(g->err), "%s", m); }

#define CG_FAIL(g, msg) \
  do { \
    cg_err((g), (msg)); \
    (g)->failed = 1; \
  } while (0)

static void txb(CG *g, uint8_t b) {
  uint8_t *n;
  if (g->failed) return;
  n = (uint8_t *)realloc(g->tx, g->tx_len + 1);
  if (!n) {
    CG_FAIL(g, "oom codegen");
    return;
  }
  g->tx = n;
  g->tx[g->tx_len++] = b;
}

static void tx_copy(CG *g, const uint8_t *p, size_t n) {
  size_t i;
  if (g->failed) return;
  for (i = 0; i < n; ++i) txb(g, p[i]);
}

static int emit_load_at_rax(CG *g, uint32_t sz) {
  if (sz == 1u) {
    tx_copy(g, (uint8_t[]){0x0f, 0xb6, 0x00}, 3); /* movzx eax, byte [rax] */
    return 1;
  }
  if (sz == 2u) {
    tx_copy(g, (uint8_t[]){0x0f, 0xb7, 0x00}, 3); /* movzx eax, word [rax] */
    return 1;
  }
  if (sz == 4u) {
    tx_copy(g, (uint8_t[]){0x8b, 0x00}, 2); /* mov eax, [rax] */
    return 1;
  }
  if (sz == 8u) {
    tx_copy(g, (uint8_t[]){0x48, 0x8b, 0x00}, 3); /* mov rax, [rax] */
    return 1;
  }
  return 0;
}

static int alloc_slot(CG *g) {
  if (g->next_slot >= FRAME_SLOTS) {
    CG_FAIL(g, "too many temporaries / locals (frame overflow)");
    return -1;
  }
  return g->next_slot++;
}

/* Allocate n consecutive slots, return the first slot index. */
static int alloc_slots(CG *g, int n) {
  if (n <= 0) return -1;
  if (g->next_slot + n > FRAME_SLOTS) {
    CG_FAIL(g, "too many temporaries / locals (frame overflow)");
    return -1;
  }
  int first = g->next_slot;
  g->next_slot += n;
  return first;
}

static int new_lbl(CG *g) {
  if (g->nlbl >= 64) {
    CG_FAIL(g, "too many labels");
    return -1;
  }
  g->lbl_set[g->nlbl] = 0;
  return g->nlbl++;
}

static void bind_lbl(CG *g, int id) {
  if (id < 0 || id >= 64) return;
  g->lbl_pos[id] = g->tx_len;
  g->lbl_set[id] = 1;
}

static void add_fix(CG *g, size_t patch_off, int lbl, int insn_len) {
  if (g->failed) return;
  if (g->nfix >= 128) {
    CG_FAIL(g, "too many branch fixups");
    return;
  }
  g->fix[g->nfix].patch = patch_off;
  g->fix[g->nfix].lbl = lbl;
  g->fix[g->nfix].insn_len = insn_len;
  g->nfix++;
}

static void resolve_fixups(CG *g) {
  int i;
  for (i = 0; i < g->nfix; ++i) {
    int li = g->fix[i].lbl;
    size_t p = g->fix[i].patch;
    int il = g->fix[i].insn_len;
    int64_t full;
    int32_t rel;
    if (li < 0 || !g->lbl_set[li]) {
      CG_FAIL(g, "unresolved branch label in codegen");
      continue;
    }
    full = (int64_t)g->lbl_pos[li] - (int64_t)(p + (size_t)il);
    if (full < (int64_t)INT32_MIN || full > (int64_t)INT32_MAX) {
      CG_FAIL(g, "branch displacement out of range");
      continue;
    }
    rel = (int32_t)full;
    if (p + 4 > g->tx_len) continue;
    g->tx[p + 0] = (uint8_t)(rel & 0xff);
    g->tx[p + 1] = (uint8_t)((rel >> 8) & 0xff);
    g->tx[p + 2] = (uint8_t)((rel >> 16) & 0xff);
    g->tx[p + 3] = (uint8_t)((rel >> 24) & 0xff);
  }
  g->nfix = 0;
}

/* [rbp] = saved rbp, [rbp-8] = saved rbx, params/locals from [rbp-16] (slot 0). */
static int32_t slot_disp(int slot) { return -(int32_t)(8 * ((size_t)slot + 2u)); }

/* gpr: 0=rax 1=rcx 2=rdx 3=rbx */
static void emit_mov_gpr_from_slot(CG *g, int gpr, int slot) {
  int32_t d = slot_disp(slot);
  uint8_t rex = 0x48;
  uint8_t modrm8 = (uint8_t)(0x45u | (uint8_t)((unsigned)gpr << 3u));
  uint8_t modrm32 = (uint8_t)(0x85u | (uint8_t)((unsigned)gpr << 3u));
  if (d >= -128 && d <= 127)
    tx_copy(g, (uint8_t[]){rex, 0x8b, modrm8, (uint8_t)(uint8_t)d}, 4);
  else {
    tx_copy(g, (uint8_t[]){rex, 0x8b, modrm32}, 3);
    tx_copy(g, (uint8_t *)&d, 4);
  }
}

static void emit_mov_r64_rbp(CG *g, int dst_reg /*0=rax*/, int slot) {
  if (dst_reg != 0) {
    cg_err(g, "use emit_mov_gpr_from_slot");
    return;
  }
  emit_mov_gpr_from_slot(g, 0, slot);
}

static void emit_mov_rbp_r64(CG *g, int slot, int src_reg /*0=rax*/) {
  int32_t d = slot_disp(slot);
  uint8_t rex = 0x48;
  if (src_reg != 0) {
    cg_err(g, "only rax src");
    return;
  }
  if (d >= -128 && d <= 127) {
    tx_copy(g, (uint8_t[]){rex, 0x89, (uint8_t)(0x45 | (0 << 3)), (uint8_t)(uint8_t)d}, 4);
  } else {
    tx_copy(g, (uint8_t[]){rex, 0x89, (uint8_t)(0x85 | (0 << 3))}, 3);
    tx_copy(g, (uint8_t *)&d, 4);
  }
}

static void emit_mov_rax_imm64(CG *g, int64_t v) {
  uint8_t seq[10];
  seq[0] = 0x48;
  seq[1] = 0xb8;
  memcpy(seq + 2, &v, 8);
  tx_copy(g, seq, 10);
}

static void emit_mov_eax_from_slot(CG *g, int slot) {
  int32_t d = slot_disp(slot);
  if (d >= -128 && d <= 127)
    tx_copy(g, (uint8_t[]){0x8b, 0x45, (uint8_t)(uint8_t)d}, 3);
  else {
    tx_copy(g, (uint8_t[]){0x8b, 0x85}, 2);
    tx_copy(g, (uint8_t *)&d, 4);
  }
}

static void emit_mov_eax_zext8_from_slot(CG *g, int slot) {
  int32_t d = slot_disp(slot);
  if (d >= -128 && d <= 127)
    tx_copy(g, (uint8_t[]){0x0f, 0xb6, 0x45, (uint8_t)(uint8_t)d}, 4);
  else {
    tx_copy(g, (uint8_t[]){0x0f, 0xb6, 0x85}, 3);
    tx_copy(g, (uint8_t *)&d, 4);
  }
}

static void emit_mov_eax_zext16_from_slot(CG *g, int slot) {
  int32_t d = slot_disp(slot);
  if (d >= -128 && d <= 127)
    tx_copy(g, (uint8_t[]){0x0f, 0xb7, 0x45, (uint8_t)(uint8_t)d}, 4); /* movzx eax, word [rbp+d] */
  else {
    tx_copy(g, (uint8_t[]){0x0f, 0xb7, 0x85}, 3);
    tx_copy(g, (uint8_t *)&d, 4);
  }
}

static void emit_branch_cond_to_rax(CG *g, const OlExpr *cond, int slot) {
  if (cond && type_is_byte_like(&cond->ty))
    emit_mov_eax_zext8_from_slot(g, slot);
  else if (cond && type_is_word_like(&cond->ty))
    emit_mov_eax_zext16_from_slot(g, slot);
  else if (cond && (type_is_i32_like(&cond->ty) || type_is_u32_like(&cond->ty)))
    emit_mov_eax_from_slot(g, slot);
  else
    emit_mov_r64_rbp(g, 0, slot);
}

/* OLang ABI parameter registers: r12, r13, r14, r15, rdi, rsi, r8, r9 */
static void emit_mov_r12_rax(CG *g) { tx_copy(g, (uint8_t[]){0x49, 0x89, 0xc4}, 3); }
static void emit_mov_r13_rax(CG *g) { tx_copy(g, (uint8_t[]){0x49, 0x89, 0xc5}, 3); }
static void emit_mov_r14_rax(CG *g) { tx_copy(g, (uint8_t[]){0x49, 0x89, 0xc6}, 3); }
static void emit_mov_r15_rax(CG *g) { tx_copy(g, (uint8_t[]){0x49, 0x89, 0xc7}, 3); }
static void emit_mov_rdi_rax(CG *g) { tx_copy(g, (uint8_t[]){0x48, 0x89, 0xc7}, 3); }
static void emit_mov_rsi_rax(CG *g) { tx_copy(g, (uint8_t[]){0x48, 0x89, 0xc6}, 3); }
static void emit_mov_r8_rax(CG *g) { tx_copy(g, (uint8_t[]){0x49, 0x89, 0xc0}, 3); }
static void emit_mov_r9_rax(CG *g) { tx_copy(g, (uint8_t[]){0x49, 0x89, 0xc1}, 3); }

static void emit_lea_rax_rip(CG *g, const char *sym) {
  size_t reloc_at;
  tx_copy(g, (uint8_t[]){0x48, 0x8d, 0x05}, 3);
  reloc_at = g->tx_len;
  tx_copy(g, (uint8_t[]){0, 0, 0, 0}, 4);
  if (!x64_add_pc32_reloc(g->obj, 0, reloc_at, sym, 0)) CG_FAIL(g, "reloc oom");
}

static void emit_mov_from_global(CG *g, const char *sym, uint32_t sz) {
  size_t reloc_at;
  if (sz == 1u) {
    tx_copy(g, (uint8_t[]){0x0f, 0xb6, 0x05}, 3); /* movzx eax, byte [rip+d] */
  } else if (sz == 2u) {
    tx_copy(g, (uint8_t[]){0x0f, 0xb7, 0x05}, 3); /* movzx eax, word [rip+d] */
  } else if (sz == 4u) {
    tx_copy(g, (uint8_t[]){0x8b, 0x05}, 2); /* mov eax, [rip+d] */
  } else if (sz == 8u) {
    tx_copy(g, (uint8_t[]){0x48, 0x8b, 0x05}, 3); /* mov rax, [rip+d] */
  } else {
    CG_FAIL(g, "global load size");
    return;
  }
  reloc_at = g->tx_len;
  tx_copy(g, (uint8_t[]){0, 0, 0, 0}, 4);
  if (!x64_add_pc32_reloc(g->obj, 0, reloc_at, sym, 0)) CG_FAIL(g, "global load reloc");
}

static void emit_mov_to_global(CG *g, const char *sym, uint32_t sz) {
  size_t reloc_at;
  if (sz == 1u) {
    tx_copy(g, (uint8_t[]){0x88, 0x05}, 2); /* mov byte [rip+d], al */
  } else if (sz == 2u) {
    tx_copy(g, (uint8_t[]){0x66, 0x89, 0x05}, 3); /* mov word [rip+d], ax */
  } else if (sz == 4u) {
    tx_copy(g, (uint8_t[]){0x89, 0x05}, 2); /* mov dword [rip+d], eax */
  } else if (sz == 8u) {
    tx_copy(g, (uint8_t[]){0x48, 0x89, 0x05}, 3); /* mov qword [rip+d], rax */
  } else {
    CG_FAIL(g, "global store size");
    return;
  }
  reloc_at = g->tx_len;
  tx_copy(g, (uint8_t[]){0, 0, 0, 0}, 4);
  if (!x64_add_pc32_reloc(g->obj, 0, reloc_at, sym, 0)) CG_FAIL(g, "global store reloc");
}

static void emit_call_sym(CG *g, const char *sym) {
  size_t reloc_at;
  txb(g, 0xe8);
  reloc_at = g->tx_len;
  tx_copy(g, (uint8_t[]){0, 0, 0, 0}, 4);
  if (!x64_add_call_reloc(g->obj, reloc_at, sym)) CG_FAIL(g, "call reloc oom");
}

/* Emits push rbp; mov rbp,rsp; push rbx; sub rsp, imm32 (imm32 patched later). Returns offset of imm32 in g->tx. */
static size_t emit_prologue_frame_placeholder(CG *g) {
  tx_copy(g, (uint8_t[]){0x55, 0x48, 0x89, 0xe5}, 4);
  txb(g, 0x53);
  tx_copy(g, (uint8_t[]){0x48, 0x81, 0xec}, 3);
  {
    size_t imm_off = g->tx_len;
    int32_t z = 0;
    tx_copy(g, (uint8_t *)&z, 4);
    return imm_off;
  }
}

static void patch_u32_at(CG *g, size_t off, int32_t v) {
  if (off + 4 > g->tx_len) return;
  memcpy(g->tx + off, &v, 4);
}

static void emit_epilogue(CG *g, int32_t frame_bytes) {
  tx_copy(g, (uint8_t[]){0x48, 0x81, 0xc4}, 3);
  tx_copy(g, (uint8_t *)&frame_bytes, 4);
  txb(g, 0x5b);
  txb(g, 0x5d);
  txb(g, 0xc3);
}

static void emit_test_rax(CG *g) { tx_copy(g, (uint8_t[]){0x48, 0x85, 0xc0}, 3); }

static void emit_je_rel32(CG *g, int lbl) {
  size_t p = g->tx_len + 2;
  tx_copy(g, (uint8_t[]){0x0f, 0x84, 0, 0, 0, 0}, 6);
  /* rel32 occupies p..p+3; next insn at p+4. */
  add_fix(g, p, lbl, 4);
}

static void emit_jmp_rel32(CG *g, int lbl) {
  size_t p = g->tx_len + 1;
  tx_copy(g, (uint8_t[]){0xe9, 0, 0, 0, 0}, 5);
  add_fix(g, p, lbl, 4);
}

static int lookup_slot(CG *g, const char *name) {
  int i;
  for (i = g->nloc - 1; i >= 0; --i) {
    if (strcmp(g->loc[i].name, name) == 0) return g->loc[i].slot;
  }
  return -1;
}

/* Index into g->loc[] for innermost binding of name, or -1. */
static int find_local_loc(CG *g, const char *name) {
  int i;
  for (i = g->nloc - 1; i >= 0; --i) {
    if (strcmp(g->loc[i].name, name) == 0) return i;
  }
  return -1;
}

static OlTypeRef *lookup_local_ty(CG *g, const char *name) {
  int i;
  for (i = g->nloc - 1; i >= 0; --i) {
    if (strcmp(g->loc[i].name, name) == 0) return &g->loc[i].ty;
  }
  return NULL;
}

static OlFuncDef *lookup_funcdef(CG *g, const char *name) {
  size_t i;
  for (i = 0; i < g->p->func_count; ++i) {
    if (strcmp(g->p->funcs[i].name, name) == 0) return &g->p->funcs[i];
  }
  return NULL;
}

static const char *sym_for_fn_ref(CG *g, const char *user) {
  OlFuncDef *f = lookup_funcdef(g, user);
  if (f && f->link_name[0]) return f->link_name;
  return user;
}

static int bind_local(CG *g, const char *name, int slot, const OlTypeRef *ty) {
  if (g->nloc >= 256) {
    CG_FAIL(g, "too many locals");
    return 0;
  }
  snprintf(g->loc[g->nloc].name, sizeof(g->loc[g->nloc].name), "%s", name);
  g->loc[g->nloc].slot = slot;
  if (ty) {
    g->loc[g->nloc].ty = *ty;
  } else {
    memset(&g->loc[g->nloc].ty, 0, sizeof(OlTypeRef));
  }
  g->loc[g->nloc].indirect = 0u;
  g->nloc++;
  return 1;
}

static void enter_scope(CG *g, int *save) {
  *save = g->nloc;
}

static void exit_scope(CG *g, int save) { g->nloc = save; }

static int gen_expr(CG *g, OlExpr *e) {
  int out, sl, sr;
  if (g->failed) return -1;
  switch (e->kind) {
    case OL_EX_INT:
      out = alloc_slot(g);
      emit_mov_rax_imm64(g, e->u.int_.int_val);
      emit_mov_rbp_r64(g, out, 0);
      return out;
    case OL_EX_FLOAT: {
      out = alloc_slot(g);
      if (e->ty.kind == OL_TY_F64) {
        union { double d; int64_t i; } u;
        u.d = e->u.float_.float_val;
        emit_mov_rax_imm64(g, u.i);
      } else if (e->ty.kind == OL_TY_F32) {
        union { float f; int32_t i; } u;
        u.f = (float)e->u.float_.float_val;
        emit_mov_rax_imm64(g, (int64_t)(uint32_t)u.i);
      } else {
        uint16_t h = f32_to_f16_bits((float)e->u.float_.float_val);
        emit_mov_rax_imm64(g, (int64_t)h);
      }
      emit_mov_rbp_r64(g, out, 0);
      return out;
    }
    case OL_EX_BOOL:
      out = alloc_slot(g);
      emit_mov_rax_imm64(g, e->u.bool_val ? 1 : 0);
      emit_mov_rbp_r64(g, out, 0);
      return out;
    case OL_EX_CHAR:
      out = alloc_slot(g);
      emit_mov_rax_imm64(g, (int64_t)e->u.char_val);
      emit_mov_rbp_r64(g, out, 0);
      return out;
    case OL_EX_NEG: {
      sl = gen_expr(g, e->u.neg.inner);
      if (sl < 0) return -1;
      if (type_is_float(&e->ty)) {
        int32_t d = slot_disp(sl);
        out = alloc_slot(g);
        if (e->ty.kind == OL_TY_F64) {
          /* movsd xmm0, [rbp+d] */
          if (d >= -128 && d <= 127) tx_copy(g, (uint8_t[]){0xf2, 0x0f, 0x10, 0x45, (uint8_t)(int8_t)d}, 5);
          else { tx_copy(g, (uint8_t[]){0xf2, 0x0f, 0x10, 0x85}, 4); tx_copy(g, (uint8_t*)&d, 4); }
          /* XOR sign bit: mov rax, 0x8000000000000000; movq xmm1, rax; xorpd xmm0, xmm1 */
          { uint8_t imm[10] = {0x48,0xb8}; int64_t sb = (int64_t)0x8000000000000000ull; memcpy(imm+2,&sb,8); tx_copy(g,imm,10); }
          tx_copy(g, (uint8_t[]){0x66, 0x48, 0x0f, 0x6e, 0xc8}, 5); /* movq xmm1, rax */
          tx_copy(g, (uint8_t[]){0x66, 0x0f, 0x57, 0xc1}, 4); /* xorpd xmm0, xmm1 */
          /* movsd [rbp+out], xmm0 */
          { int32_t od = slot_disp(out);
            if (od >= -128 && od <= 127) tx_copy(g, (uint8_t[]){0xf2, 0x0f, 0x11, 0x45, (uint8_t)(int8_t)od}, 5);
            else { tx_copy(g, (uint8_t[]){0xf2, 0x0f, 0x11, 0x85}, 4); tx_copy(g, (uint8_t*)&od, 4); } }
        } else {
          /* f32 (and f16 stored as f32-width in slot): xorps with sign bit */
          if (d >= -128 && d <= 127) tx_copy(g, (uint8_t[]){0xf3, 0x0f, 0x10, 0x45, (uint8_t)(int8_t)d}, 5);
          else { tx_copy(g, (uint8_t[]){0xf3, 0x0f, 0x10, 0x85}, 4); tx_copy(g, (uint8_t*)&d, 4); }
          emit_mov_rax_imm64(g, 0x80000000LL);
          tx_copy(g, (uint8_t[]){0x66, 0x0f, 0x6e, 0xc8}, 4); /* movd xmm1, eax */
          tx_copy(g, (uint8_t[]){0x0f, 0x57, 0xc1}, 3); /* xorps xmm0, xmm1 */
          { int32_t od = slot_disp(out);
            if (od >= -128 && od <= 127) tx_copy(g, (uint8_t[]){0xf3, 0x0f, 0x11, 0x45, (uint8_t)(int8_t)od}, 5);
            else { tx_copy(g, (uint8_t[]){0xf3, 0x0f, 0x11, 0x85}, 4); tx_copy(g, (uint8_t*)&od, 4); } }
        }
        return out;
      }
      emit_mov_r64_rbp(g, 0, sl);
      if (e->ty.kind == OL_TY_I32) {
        tx_copy(g, (uint8_t[]){0xf7, 0xd8}, 2); /* neg eax */
        tx_copy(g, (uint8_t[]){0x48, 0x63, 0xc0}, 3); /* movsxd rax, eax */
      } else if (e->ty.kind == OL_TY_I16) {
        tx_copy(g, (uint8_t[]){0x66, 0xf7, 0xd8}, 3); /* neg ax */
        tx_copy(g, (uint8_t[]){0x48, 0x0f, 0xbf, 0xc0}, 4); /* movsx rax, ax */
      } else if (e->ty.kind == OL_TY_I8) {
        tx_copy(g, (uint8_t[]){0xf6, 0xd8}, 2); /* neg al */
        tx_copy(g, (uint8_t[]){0x48, 0x0f, 0xbe, 0xc0}, 4); /* movsx rax, al */
      } else {
        tx_copy(g, (uint8_t[]){0x48, 0xf7, 0xd8}, 3); /* neg rax */
      }
      out = alloc_slot(g);
      emit_mov_rbp_r64(g, out, 0);
      return out;
    }
    case OL_EX_NOT: {
      sl = gen_expr(g, e->u.not_.inner);
      if (sl < 0) return -1;
      emit_mov_r64_rbp(g, 0, sl);
      tx_copy(g, (uint8_t[]){0x48, 0x85, 0xc0}, 3); /* test rax, rax */
      tx_copy(g, (uint8_t[]){0x0f, 0x94, 0xc0}, 3); /* sete al */
      tx_copy(g, (uint8_t[]){0x48, 0x0f, 0xb6, 0xc0}, 4); /* movzx rax, al */
      out = alloc_slot(g);
      emit_mov_rbp_r64(g, out, 0);
      return out;
    }
    case OL_EX_BNOT: {
      sl = gen_expr(g, e->u.bnot.inner);
      if (sl < 0) return -1;
      emit_mov_r64_rbp(g, 0, sl);
      tx_copy(g, (uint8_t[]){0x48, 0xf7, 0xd0}, 3); /* not rax */
      out = alloc_slot(g);
      emit_mov_rbp_r64(g, out, 0);
      return out;
    }
    case OL_EX_STR: {
      const char *sym = g->p->strings[e->u.str_idx].sym;
      out = alloc_slot(g);
      emit_lea_rax_rip(g, sym);
      emit_mov_rbp_r64(g, out, 0);
      return out;
    }
    case OL_EX_VAR: {
      int sl = lookup_slot(g, e->u.var_name);
      int gi;
      uint32_t gsz;
      int out;
      int li;
      /* Indirect local: slot holds ptr; rvalue loads element into a new temp slot */
      if (sl >= 0) {
        li = find_local_loc(g, e->u.var_name);
        if (li >= 0 && g->loc[li].indirect) {
          OlTypeRef *elt = &g->loc[li].ty;
          uint32_t esz = type_size_bytes(g->p, elt);
          emit_mov_gpr_from_slot(g, 0, sl);
          out = alloc_slot(g);
          if (type_is_float(elt)) {
            int32_t dout = slot_disp(out);
            if (elt->kind == OL_TY_F64) {
              tx_copy(g, (uint8_t[]){0xf2, 0x0f, 0x10, 0x00}, 4); /* movsd xmm0,[rax] */
              if (dout >= -128 && dout <= 127)
                tx_copy(g, (uint8_t[]){0xf2, 0x0f, 0x11, 0x45, (uint8_t)(int8_t)dout}, 5);
              else {
                tx_copy(g, (uint8_t[]){0xf2, 0x0f, 0x11, 0x85}, 4);
                tx_copy(g, (uint8_t *)&dout, 4);
              }
            } else if (elt->kind == OL_TY_F32) {
              tx_copy(g, (uint8_t[]){0xf3, 0x0f, 0x10, 0x00}, 4);
              if (dout >= -128 && dout <= 127)
                tx_copy(g, (uint8_t[]){0xf3, 0x0f, 0x11, 0x45, (uint8_t)(int8_t)dout}, 5);
              else {
                tx_copy(g, (uint8_t[]){0xf3, 0x0f, 0x11, 0x85}, 4);
                tx_copy(g, (uint8_t *)&dout, 4);
              }
            } else {
              if (!emit_load_at_rax(g, 2u)) {
                cg_err(g, "indirect f16 load");
                return -1;
              }
              emit_mov_rbp_r64(g, out, 0);
            }
            return out;
          }
          if (!emit_load_at_rax(g, esz)) {
            cg_err(g, "indirect load");
            return -1;
          }
          emit_mov_rbp_r64(g, out, 0);
          return out;
        }
        return sl;
      }
      /* Global variable: check if it's an aggregate type */
      gi = ol_program_find_global(g->p, e->u.var_name);
      if (gi < 0) return -1;
      gsz = type_size_bytes(g->p, &g->p->globals[(size_t)gi].ty);
      /* Aggregate type (>8 bytes): return address (lea), not load */
      if (gsz > 8u) {
        out = alloc_slot(g);
        emit_lea_rax_rip(g, g->p->globals[(size_t)gi].name);
        emit_mov_rbp_r64(g, out, 0);
        return out;
      }
      /* Scalar type: load value from section */
      emit_mov_from_global(g, g->p->globals[(size_t)gi].name, gsz);
      if (g->failed) return -1;
      out = alloc_slot(g);
      emit_mov_rbp_r64(g, out, 0);
      return out;
    }
    case OL_EX_BINARY: {
      int use_unsigned = type_is_unsigned_like(&e->u.binary.left->ty);
      int is_flt = type_is_float(&e->u.binary.left->ty);
      sl = gen_expr(g, e->u.binary.left);
      sr = gen_expr(g, e->u.binary.right);
      if (sl < 0 || sr < 0) return -1;
      if (is_flt) {
        /* SSE float binary ops: load both operands into xmm0/xmm1 */
        int32_t dl = slot_disp(sl), dr = slot_disp(sr);
        int is_d = (e->u.binary.left->ty.kind == OL_TY_F64);
        /* Load left into xmm0 */
        if (is_d) {
          if (dl >= -128 && dl <= 127) tx_copy(g, (uint8_t[]){0xf2, 0x0f, 0x10, 0x45, (uint8_t)(int8_t)dl}, 5);
          else { tx_copy(g, (uint8_t[]){0xf2, 0x0f, 0x10, 0x85}, 4); tx_copy(g, (uint8_t*)&dl, 4); }
        } else {
          if (dl >= -128 && dl <= 127) tx_copy(g, (uint8_t[]){0xf3, 0x0f, 0x10, 0x45, (uint8_t)(int8_t)dl}, 5);
          else { tx_copy(g, (uint8_t[]){0xf3, 0x0f, 0x10, 0x85}, 4); tx_copy(g, (uint8_t*)&dl, 4); }
        }
        /* Load right into xmm1 */
        if (is_d) {
          if (dr >= -128 && dr <= 127) tx_copy(g, (uint8_t[]){0xf2, 0x0f, 0x10, 0x4d, (uint8_t)(int8_t)dr}, 5);
          else { tx_copy(g, (uint8_t[]){0xf2, 0x0f, 0x10, 0x8d}, 4); tx_copy(g, (uint8_t*)&dr, 4); }
        } else {
          if (dr >= -128 && dr <= 127) tx_copy(g, (uint8_t[]){0xf3, 0x0f, 0x10, 0x4d, (uint8_t)(int8_t)dr}, 5);
          else { tx_copy(g, (uint8_t[]){0xf3, 0x0f, 0x10, 0x8d}, 4); tx_copy(g, (uint8_t*)&dr, 4); }
        }
        switch (e->u.binary.op) {
          case OL_BIN_ADD:
            tx_copy(g, is_d ? (uint8_t[]){0xf2, 0x0f, 0x58, 0xc1} : (uint8_t[]){0xf3, 0x0f, 0x58, 0xc1}, 4);
            break;
          case OL_BIN_SUB:
            tx_copy(g, is_d ? (uint8_t[]){0xf2, 0x0f, 0x5c, 0xc1} : (uint8_t[]){0xf3, 0x0f, 0x5c, 0xc1}, 4);
            break;
          case OL_BIN_MUL:
            tx_copy(g, is_d ? (uint8_t[]){0xf2, 0x0f, 0x59, 0xc1} : (uint8_t[]){0xf3, 0x0f, 0x59, 0xc1}, 4);
            break;
          case OL_BIN_DIV:
            tx_copy(g, is_d ? (uint8_t[]){0xf2, 0x0f, 0x5e, 0xc1} : (uint8_t[]){0xf3, 0x0f, 0x5e, 0xc1}, 4);
            break;
          case OL_BIN_EQ: case OL_BIN_NE: case OL_BIN_LT: case OL_BIN_GT: case OL_BIN_LE: case OL_BIN_GE: {
            /* ucomisd/ucomiss xmm0, xmm1 */
            if (is_d) tx_copy(g, (uint8_t[]){0x66, 0x0f, 0x2e, 0xc1}, 4);
            else tx_copy(g, (uint8_t[]){0x0f, 0x2e, 0xc1}, 3);
            /* Select setcc based on operation (unsigned branch for unordered float comparison) */
            switch (e->u.binary.op) {
              case OL_BIN_EQ: tx_copy(g, (uint8_t[]){0x0f, 0x94, 0xc0}, 3); break; /* sete al */
              case OL_BIN_NE: tx_copy(g, (uint8_t[]){0x0f, 0x95, 0xc0}, 3); break; /* setne al */
              case OL_BIN_LT: tx_copy(g, (uint8_t[]){0x0f, 0x92, 0xc0}, 3); break; /* setb al */
              case OL_BIN_GT: tx_copy(g, (uint8_t[]){0x0f, 0x97, 0xc0}, 3); break; /* seta al */
              case OL_BIN_LE: tx_copy(g, (uint8_t[]){0x0f, 0x96, 0xc0}, 3); break; /* setbe al */
              case OL_BIN_GE: tx_copy(g, (uint8_t[]){0x0f, 0x93, 0xc0}, 3); break; /* setae al */
              default: break;
            }
            tx_copy(g, (uint8_t[]){0x0f, 0xb6, 0xc0}, 3); /* movzx eax, al */
            out = alloc_slot(g);
            emit_mov_rbp_r64(g, out, 0);
            return out;
          }
          default:
            cg_err(g, "bad float binop");
            return -1;
        }
        /* Store xmm0 result to output slot */
        out = alloc_slot(g);
        { int32_t od = slot_disp(out);
          if (is_d) {
            if (od >= -128 && od <= 127) tx_copy(g, (uint8_t[]){0xf2, 0x0f, 0x11, 0x45, (uint8_t)(int8_t)od}, 5);
            else { tx_copy(g, (uint8_t[]){0xf2, 0x0f, 0x11, 0x85}, 4); tx_copy(g, (uint8_t*)&od, 4); }
          } else {
            if (od >= -128 && od <= 127) tx_copy(g, (uint8_t[]){0xf3, 0x0f, 0x11, 0x45, (uint8_t)(int8_t)od}, 5);
            else { tx_copy(g, (uint8_t[]){0xf3, 0x0f, 0x11, 0x85}, 4); tx_copy(g, (uint8_t*)&od, 4); }
          }
        }
        return out;
      }
      emit_mov_gpr_from_slot(g, 0, sl);
      emit_mov_gpr_from_slot(g, 3, sr);
      switch (e->u.binary.op) {
        case OL_BIN_ADD:
          tx_copy(g, (uint8_t[]){0x48, 0x01, 0xd8}, 3);
          break;
        case OL_BIN_SUB:
          tx_copy(g, (uint8_t[]){0x48, 0x29, 0xd8}, 3);
          break;
        case OL_BIN_MUL:
          tx_copy(g, (uint8_t[]){0x48, 0x0f, 0xaf, 0xc3}, 4);
          break;
        case OL_BIN_DIV:
        case OL_BIN_MOD:
          if (use_unsigned)
            tx_copy(g, (uint8_t[]){0x31, 0xd2, 0x48, 0xf7, 0xf3}, 5);
          else {
            tx_copy(g, (uint8_t[]){0x48, 0x99}, 2);
            tx_copy(g, (uint8_t[]){0x48, 0xf7, 0xfb}, 3);
          }
          if (e->u.binary.op == OL_BIN_MOD) tx_copy(g, (uint8_t[]){0x48, 0x89, 0xd0}, 3);
          break;
        case OL_BIN_EQ:
          tx_copy(g, (uint8_t[]){0x48, 0x39, 0xd8}, 3);
          tx_copy(g, (uint8_t[]){0x0f, 0x94, 0xc0}, 3);
          tx_copy(g, (uint8_t[]){0x0f, 0xb6, 0xc0}, 3);
          break;
        case OL_BIN_NE:
          tx_copy(g, (uint8_t[]){0x48, 0x39, 0xd8}, 3);
          tx_copy(g, (uint8_t[]){0x0f, 0x95, 0xc0}, 3);
          tx_copy(g, (uint8_t[]){0x0f, 0xb6, 0xc0}, 3);
          break;
        case OL_BIN_LT:
          tx_copy(g, (uint8_t[]){0x48, 0x39, 0xd8}, 3);
          tx_copy(g, use_unsigned ? (uint8_t[]){0x0f, 0x92, 0xc0} : (uint8_t[]){0x0f, 0x9c, 0xc0}, 3);
          tx_copy(g, (uint8_t[]){0x0f, 0xb6, 0xc0}, 3);
          break;
        case OL_BIN_GT:
          tx_copy(g, (uint8_t[]){0x48, 0x39, 0xd8}, 3);
          tx_copy(g, use_unsigned ? (uint8_t[]){0x0f, 0x97, 0xc0} : (uint8_t[]){0x0f, 0x9f, 0xc0}, 3);
          tx_copy(g, (uint8_t[]){0x0f, 0xb6, 0xc0}, 3);
          break;
        case OL_BIN_LE:
          tx_copy(g, (uint8_t[]){0x48, 0x39, 0xd8}, 3);
          tx_copy(g, use_unsigned ? (uint8_t[]){0x0f, 0x96, 0xc0} : (uint8_t[]){0x0f, 0x9e, 0xc0}, 3);
          tx_copy(g, (uint8_t[]){0x0f, 0xb6, 0xc0}, 3);
          break;
        case OL_BIN_GE:
          tx_copy(g, (uint8_t[]){0x48, 0x39, 0xd8}, 3);
          tx_copy(g, use_unsigned ? (uint8_t[]){0x0f, 0x93, 0xc0} : (uint8_t[]){0x0f, 0x9d, 0xc0}, 3);
          tx_copy(g, (uint8_t[]){0x0f, 0xb6, 0xc0}, 3);
          break;
        case OL_BIN_AND:
          tx_copy(g, (uint8_t[]){0x48, 0x85, 0xc0}, 3);
          tx_copy(g, (uint8_t[]){0x0f, 0x95, 0xc0}, 3);
          tx_copy(g, (uint8_t[]){0x48, 0x85, 0xdb}, 3);
          tx_copy(g, (uint8_t[]){0x0f, 0x95, 0xc3}, 3);
          tx_copy(g, (uint8_t[]){0x20, 0xd8}, 2);
          tx_copy(g, (uint8_t[]){0x0f, 0xb6, 0xc0}, 3);
          break;
        case OL_BIN_OR:
          tx_copy(g, (uint8_t[]){0x48, 0x09, 0xd8}, 3);
          tx_copy(g, (uint8_t[]){0x48, 0x85, 0xc0}, 3);
          tx_copy(g, (uint8_t[]){0x0f, 0x95, 0xc0}, 3);
          tx_copy(g, (uint8_t[]){0x0f, 0xb6, 0xc0}, 3);
          break;
        case OL_BIN_BAND:
          tx_copy(g, (uint8_t[]){0x48, 0x21, 0xd8}, 3); /* and rax, rbx */
          break;
        case OL_BIN_BOR:
          tx_copy(g, (uint8_t[]){0x48, 0x09, 0xd8}, 3); /* or rax, rbx */
          break;
        case OL_BIN_BXOR:
          tx_copy(g, (uint8_t[]){0x48, 0x31, 0xd8}, 3); /* xor rax, rbx */
          break;
        case OL_BIN_SHL:
          tx_copy(g, (uint8_t[]){0x48, 0x89, 0xd9}, 3); /* mov rcx, rbx */
          tx_copy(g, (uint8_t[]){0x48, 0xd3, 0xe0}, 3); /* shl rax, cl */
          break;
        case OL_BIN_SHR:
          tx_copy(g, (uint8_t[]){0x48, 0x89, 0xd9}, 3); /* mov rcx, rbx */
          tx_copy(g, (uint8_t[]){0x48, 0xd3, 0xe8}, 3); /* shr rax, cl */
          break;
        default:
          cg_err(g, "bad binop");
          return -1;
      }
      out = alloc_slot(g);
      emit_mov_rbp_r64(g, out, 0);
      return out;
    }
    case OL_EX_CALL: {
      size_t i;
      size_t stack_args = 0;
      int *slots = (int *)malloc(e->u.call.arg_count * sizeof(int));
      if (!slots) return -1;
      for (i = 0; i < e->u.call.arg_count; ++i) {
        slots[i] = gen_expr(g, e->u.call.args[i]);
        if (slots[i] < 0) {
          free(slots);
          return -1;
        }
      }
      /* Push stack-overflow args (9+) right-to-left */
      if (e->u.call.arg_count > 8) {
        stack_args = e->u.call.arg_count - 8;
        for (i = e->u.call.arg_count; i > 8; --i) {
          emit_mov_r64_rbp(g, 0, slots[i - 1]);
          /* push rax */
          txb(g, 0x50);
        }
      }
      /* OLang ABI register args: r12, r13, r14, r15, rdi, rsi, r8, r9 */
      for (i = 0; i < e->u.call.arg_count && i < 8; ++i) {
        emit_mov_r64_rbp(g, 0, slots[i]);
        switch (i) {
          case 0: emit_mov_r12_rax(g); break;
          case 1: emit_mov_r13_rax(g); break;
          case 2: emit_mov_r14_rax(g); break;
          case 3: emit_mov_r15_rax(g); break;
          case 4: emit_mov_rdi_rax(g); break;
          case 5: emit_mov_rsi_rax(g); break;
          case 6: emit_mov_r8_rax(g);  break;
          case 7: emit_mov_r9_rax(g);  break;
          default: break;
        }
      }
      free(slots);
      emit_call_sym(g, sym_for_fn_ref(g, e->u.call.callee));
      /* Clean up pushed stack args */
      if (stack_args > 0) {
        int32_t cleanup = (int32_t)(stack_args * 8);
        tx_copy(g, (uint8_t[]){0x48, 0x81, 0xc4}, 3); /* add rsp, imm32 */
        tx_copy(g, (uint8_t *)&cleanup, 4);
      }
      if (e->ty.kind == OL_TY_VOID) {
        out = alloc_slot(g);
        return out;
      }
      out = alloc_slot(g);
      emit_mov_rbp_r64(g, out, 0);
      return out;
    }
    case OL_EX_REINTERPRET: {
      sl = gen_expr(g, e->u.reinterpret_.inner);
      return sl;
    }
    case OL_EX_CAST: {
      OlTyKind from_k = e->u.cast_.inner->ty.kind;
      OlTyKind to_k = e->ty.kind;
      int from_flt = type_is_float(&e->u.cast_.inner->ty);
      int to_flt = type_is_float(&e->ty);
      sl = gen_expr(g, e->u.cast_.inner);
      if (sl < 0) return -1;
      out = alloc_slot(g);
      if (from_flt && to_flt) {
        /* float-to-float conversion */
        int32_t ds = slot_disp(sl), od = slot_disp(out);
        if (from_k == OL_TY_F32 && to_k == OL_TY_F64) {
          if (ds >= -128 && ds <= 127) tx_copy(g, (uint8_t[]){0xf3, 0x0f, 0x10, 0x45, (uint8_t)(int8_t)ds}, 5);
          else { tx_copy(g, (uint8_t[]){0xf3, 0x0f, 0x10, 0x85}, 4); tx_copy(g, (uint8_t*)&ds, 4); }
          tx_copy(g, (uint8_t[]){0xf3, 0x0f, 0x5a, 0xc0}, 4); /* cvtss2sd xmm0, xmm0 */
          if (od >= -128 && od <= 127) tx_copy(g, (uint8_t[]){0xf2, 0x0f, 0x11, 0x45, (uint8_t)(int8_t)od}, 5);
          else { tx_copy(g, (uint8_t[]){0xf2, 0x0f, 0x11, 0x85}, 4); tx_copy(g, (uint8_t*)&od, 4); }
        } else if (from_k == OL_TY_F64 && to_k == OL_TY_F32) {
          if (ds >= -128 && ds <= 127) tx_copy(g, (uint8_t[]){0xf2, 0x0f, 0x10, 0x45, (uint8_t)(int8_t)ds}, 5);
          else { tx_copy(g, (uint8_t[]){0xf2, 0x0f, 0x10, 0x85}, 4); tx_copy(g, (uint8_t*)&ds, 4); }
          tx_copy(g, (uint8_t[]){0xf2, 0x0f, 0x5a, 0xc0}, 4); /* cvtsd2ss xmm0, xmm0 */
          if (od >= -128 && od <= 127) tx_copy(g, (uint8_t[]){0xf3, 0x0f, 0x11, 0x45, (uint8_t)(int8_t)od}, 5);
          else { tx_copy(g, (uint8_t[]){0xf3, 0x0f, 0x11, 0x85}, 4); tx_copy(g, (uint8_t*)&od, 4); }
        } else if (from_k == OL_TY_F16 && to_k == OL_TY_F32) {
          /* f16→f32: load f16 bits to r12, call __olrt_f16_to_f32, store f32 bits from rax */
          emit_mov_r64_rbp(g, 0, sl);
          tx_copy(g, (uint8_t[]){0x49, 0x89, 0xc4}, 3); /* mov r12, rax */
          emit_call_sym(g, "__olrt_f16_to_f32");
          /* rax now has f32 bits; move to xmm0 then store */
          tx_copy(g, (uint8_t[]){0x66, 0x0f, 0x6e, 0xc0}, 4); /* movd xmm0, eax */
          if (od >= -128 && od <= 127) tx_copy(g, (uint8_t[]){0xf3, 0x0f, 0x11, 0x45, (uint8_t)(int8_t)od}, 5);
          else { tx_copy(g, (uint8_t[]){0xf3, 0x0f, 0x11, 0x85}, 4); tx_copy(g, (uint8_t*)&od, 4); }
        } else if (from_k == OL_TY_F32 && to_k == OL_TY_F16) {
          /* f32→f16: load f32 bits to r12, call __olrt_f32_to_f16, rax has f16 bits */
          if (ds >= -128 && ds <= 127) tx_copy(g, (uint8_t[]){0xf3, 0x0f, 0x10, 0x45, (uint8_t)(int8_t)ds}, 5);
          else { tx_copy(g, (uint8_t[]){0xf3, 0x0f, 0x10, 0x85}, 4); tx_copy(g, (uint8_t*)&ds, 4); }
          tx_copy(g, (uint8_t[]){0x66, 0x0f, 0x7e, 0xc0}, 4); /* movd eax, xmm0 */
          tx_copy(g, (uint8_t[]){0x49, 0x89, 0xc4}, 3); /* mov r12, rax */
          emit_call_sym(g, "__olrt_f32_to_f16");
          emit_mov_rbp_r64(g, out, 0);
        } else if (from_k == OL_TY_F16 && to_k == OL_TY_F64) {
          /* f16→f64: f16→f32 via olrt, then cvtss2sd */
          emit_mov_r64_rbp(g, 0, sl);
          tx_copy(g, (uint8_t[]){0x49, 0x89, 0xc4}, 3); /* mov r12, rax */
          emit_call_sym(g, "__olrt_f16_to_f32");
          tx_copy(g, (uint8_t[]){0x66, 0x0f, 0x6e, 0xc0}, 4); /* movd xmm0, eax */
          tx_copy(g, (uint8_t[]){0xf3, 0x0f, 0x5a, 0xc0}, 4); /* cvtss2sd xmm0, xmm0 */
          if (od >= -128 && od <= 127) tx_copy(g, (uint8_t[]){0xf2, 0x0f, 0x11, 0x45, (uint8_t)(int8_t)od}, 5);
          else { tx_copy(g, (uint8_t[]){0xf2, 0x0f, 0x11, 0x85}, 4); tx_copy(g, (uint8_t*)&od, 4); }
        } else if (from_k == OL_TY_F64 && to_k == OL_TY_F16) {
          /* f64→f16: cvtsd2ss then f32→f16 via olrt */
          if (ds >= -128 && ds <= 127) tx_copy(g, (uint8_t[]){0xf2, 0x0f, 0x10, 0x45, (uint8_t)(int8_t)ds}, 5);
          else { tx_copy(g, (uint8_t[]){0xf2, 0x0f, 0x10, 0x85}, 4); tx_copy(g, (uint8_t*)&ds, 4); }
          tx_copy(g, (uint8_t[]){0xf2, 0x0f, 0x5a, 0xc0}, 4); /* cvtsd2ss xmm0, xmm0 */
          tx_copy(g, (uint8_t[]){0x66, 0x0f, 0x7e, 0xc0}, 4); /* movd eax, xmm0 */
          tx_copy(g, (uint8_t[]){0x49, 0x89, 0xc4}, 3); /* mov r12, rax */
          emit_call_sym(g, "__olrt_f32_to_f16");
          emit_mov_rbp_r64(g, out, 0);
        } else {
          emit_mov_r64_rbp(g, 0, sl);
          emit_mov_rbp_r64(g, out, 0);
        }
        return out;
      }
      if (!from_flt && to_flt) {
        /* int-to-float */
        emit_mov_r64_rbp(g, 0, sl);
        if (to_k == OL_TY_F64) {
          tx_copy(g, (uint8_t[]){0xf2, 0x48, 0x0f, 0x2a, 0xc0}, 5); /* cvtsi2sd xmm0, rax */
          { int32_t od = slot_disp(out);
            if (od >= -128 && od <= 127) tx_copy(g, (uint8_t[]){0xf2, 0x0f, 0x11, 0x45, (uint8_t)(int8_t)od}, 5);
            else { tx_copy(g, (uint8_t[]){0xf2, 0x0f, 0x11, 0x85}, 4); tx_copy(g, (uint8_t*)&od, 4); } }
        } else if (to_k == OL_TY_F16) {
          /* int→f16: int→f32 via cvtsi2ss, then f32→f16 via olrt */
          tx_copy(g, (uint8_t[]){0xf3, 0x48, 0x0f, 0x2a, 0xc0}, 5); /* cvtsi2ss xmm0, rax */
          tx_copy(g, (uint8_t[]){0x66, 0x0f, 0x7e, 0xc0}, 4); /* movd eax, xmm0 */
          tx_copy(g, (uint8_t[]){0x49, 0x89, 0xc4}, 3); /* mov r12, rax */
          emit_call_sym(g, "__olrt_f32_to_f16");
          emit_mov_rbp_r64(g, out, 0);
        } else {
          tx_copy(g, (uint8_t[]){0xf3, 0x48, 0x0f, 0x2a, 0xc0}, 5); /* cvtsi2ss xmm0, rax */
          { int32_t od = slot_disp(out);
            if (od >= -128 && od <= 127) tx_copy(g, (uint8_t[]){0xf3, 0x0f, 0x11, 0x45, (uint8_t)(int8_t)od}, 5);
            else { tx_copy(g, (uint8_t[]){0xf3, 0x0f, 0x11, 0x85}, 4); tx_copy(g, (uint8_t*)&od, 4); } }
        }
        return out;
      }
      if (from_flt && !to_flt) {
        /* float-to-int */
        int32_t ds = slot_disp(sl);
        if (from_k == OL_TY_F64) {
          if (ds >= -128 && ds <= 127) tx_copy(g, (uint8_t[]){0xf2, 0x0f, 0x10, 0x45, (uint8_t)(int8_t)ds}, 5);
          else { tx_copy(g, (uint8_t[]){0xf2, 0x0f, 0x10, 0x85}, 4); tx_copy(g, (uint8_t*)&ds, 4); }
          tx_copy(g, (uint8_t[]){0xf2, 0x48, 0x0f, 0x2c, 0xc0}, 5); /* cvttsd2si rax, xmm0 */
        } else if (from_k == OL_TY_F16) {
          /* f16→int: f16→f32 via olrt, then cvttss2si */
          emit_mov_r64_rbp(g, 0, sl);
          tx_copy(g, (uint8_t[]){0x49, 0x89, 0xc4}, 3); /* mov r12, rax */
          emit_call_sym(g, "__olrt_f16_to_f32");
          tx_copy(g, (uint8_t[]){0x66, 0x0f, 0x6e, 0xc0}, 4); /* movd xmm0, eax */
          tx_copy(g, (uint8_t[]){0xf3, 0x48, 0x0f, 0x2c, 0xc0}, 5); /* cvttss2si rax, xmm0 */
        } else {
          if (ds >= -128 && ds <= 127) tx_copy(g, (uint8_t[]){0xf3, 0x0f, 0x10, 0x45, (uint8_t)(int8_t)ds}, 5);
          else { tx_copy(g, (uint8_t[]){0xf3, 0x0f, 0x10, 0x85}, 4); tx_copy(g, (uint8_t*)&ds, 4); }
          tx_copy(g, (uint8_t[]){0xf3, 0x48, 0x0f, 0x2c, 0xc0}, 5); /* cvttss2si rax, xmm0 */
        }
        emit_mov_rbp_r64(g, out, 0);
        return out;
      }
      /* int/ptr-to-int/ptr cast (existing logic) */
      emit_mov_r64_rbp(g, 0, sl);
      if (to_k == OL_TY_BOOL) {
        tx_copy(g, (uint8_t[]){0x48, 0x85, 0xc0}, 3);
        tx_copy(g, (uint8_t[]){0x0f, 0x95, 0xc0}, 3);
        tx_copy(g, (uint8_t[]){0x48, 0x0f, 0xb6, 0xc0}, 4);
      } else if (to_k == OL_TY_U8 || to_k == OL_TY_I8) {
        tx_copy(g, (uint8_t[]){0x25, 0xff, 0x00, 0x00, 0x00}, 5);
      } else if (to_k == OL_TY_U16 || to_k == OL_TY_I16 || to_k == OL_TY_B16) {
        tx_copy(g, (uint8_t[]){0x0f, 0xb7, 0xc0}, 3); /* movzx eax, ax */
      } else if (to_k == OL_TY_I32 || to_k == OL_TY_U32 || to_k == OL_TY_B32) {
        if (from_k == OL_TY_I8) tx_copy(g, (uint8_t[]){0x48, 0x0f, 0xbe, 0xc0}, 4); /* movsx rax, al */
        else if (from_k == OL_TY_I16) tx_copy(g, (uint8_t[]){0x48, 0x0f, 0xbf, 0xc0}, 4); /* movsx rax, ax */
        else if (from_k == OL_TY_I64 || from_k == OL_TY_U64 || from_k == OL_TY_B64) tx_copy(g, (uint8_t[]){0x8b, 0xc0}, 2); /* truncate */
        else tx_copy(g, (uint8_t[]){0x0f, 0xb6, 0xc0}, 3); /* movzx eax, al (u8/b8) */
      } else if (from_k == OL_TY_I32 && to_k == OL_TY_I64) {
        tx_copy(g, (uint8_t[]){0x48, 0x63, 0xc0}, 3);
      } else if (to_k == OL_TY_U64 || to_k == OL_TY_I64) {
        /* Widening to 64-bit from smaller types: zero-extend (already in rax) or sign-extend */
        if (from_k == OL_TY_I32) tx_copy(g, (uint8_t[]){0x48, 0x63, 0xc0}, 3);
        else if (from_k == OL_TY_I16) tx_copy(g, (uint8_t[]){0x48, 0x0f, 0xbf, 0xc0}, 4); /* movsx rax, ax */
        else if (from_k == OL_TY_I8) tx_copy(g, (uint8_t[]){0x48, 0x0f, 0xbe, 0xc0}, 4); /* movsx rax, al */
        else tx_copy(g, (uint8_t[]){0x8b, 0xc0}, 2); /* zero-extend 32-bit */
      } else if (from_k == OL_TY_I64 && to_k == OL_TY_PTR) {
        /* nop */
      } else if (from_k == OL_TY_PTR && to_k == OL_TY_I64) {
        /* nop */
      } else if (from_k == OL_TY_U64 && to_k == OL_TY_PTR) {
        /* nop */
      } else if (from_k == OL_TY_PTR && to_k == OL_TY_U64) {
        /* nop */
      } else if (from_k == OL_TY_I32 && to_k == OL_TY_PTR) {
        tx_copy(g, (uint8_t[]){0x48, 0x63, 0xc0}, 3);
      } else if (from_k == OL_TY_PTR && to_k == OL_TY_I32) {
        tx_copy(g, (uint8_t[]){0x8b, 0xc0}, 2);
      }
      emit_mov_rbp_r64(g, out, 0);
      return out;
    }
    case OL_EX_ADDR: {
      int s;
      int gi;
      int32_t d;
      int out;
      if (e->u.addr.addr_kind == OL_ADDR_LOCAL) {
        int li_addr;
        s = lookup_slot(g, e->u.addr.name);
        if (s < 0) return -1;
        li_addr = find_local_loc(g, e->u.addr.name);
        /* Logical binding through ptr: address is the pointer value in the slot, not &slot */
        if (li_addr >= 0 && g->loc[li_addr].indirect) {
          out = alloc_slot(g);
          emit_mov_r64_rbp(g, 0, s);
          emit_mov_rbp_r64(g, out, 0);
          return out;
        }
        d = slot_disp(s);
        if (d >= -128 && d <= 127) {
          tx_copy(g, (uint8_t[]){0x48, 0x8d, 0x45, (uint8_t)(uint8_t)d}, 4);
        } else {
          tx_copy(g, (uint8_t[]){0x48, 0x8d, 0x85}, 3);
          tx_copy(g, (uint8_t *)&d, 4);
        }
        out = alloc_slot(g);
        emit_mov_rbp_r64(g, out, 0);
        return out;
      }
      if (e->u.addr.addr_kind == OL_ADDR_GLOBAL) {
        gi = ol_program_find_global(g->p, e->u.addr.name);
        if (gi < 0) return -1;
        emit_lea_rax_rip(g, g->p->globals[(size_t)gi].name);
        if (g->failed) return -1;
        out = alloc_slot(g);
        emit_mov_rbp_r64(g, out, 0);
        return out;
      }
      if (e->u.addr.addr_kind == OL_ADDR_SYMBOL) {
        emit_lea_rax_rip(g, sym_for_fn_ref(g, e->u.addr.name));
        if (g->failed) return -1;
        out = alloc_slot(g);
        emit_mov_rbp_r64(g, out, 0);
        return out;
      }
      return -1;
    }
    case OL_EX_LOAD: {
      int ps = gen_expr(g, e->u.load.ptr);
      uint32_t sz = type_size_bytes(g->p, &e->u.load.elem_ty);
      if (ps < 0) return -1;
      emit_mov_r64_rbp(g, 0, ps);
      if (!emit_load_at_rax(g, sz)) {
        cg_err(g, "load size");
        return -1;
      }
      out = alloc_slot(g);
      emit_mov_rbp_r64(g, out, 0);
      return out;
    }
    case OL_EX_STORE: {
      int ps = gen_expr(g, e->u.store.ptr);
      int vs = gen_expr(g, e->u.store.val);
      uint32_t sz = type_size_bytes(g->p, &e->u.store.elem_ty);
      if (ps < 0 || vs < 0) return -1;
      emit_mov_gpr_from_slot(g, 0, ps);
      emit_mov_gpr_from_slot(g, 3, vs);
      if (sz == 8)
        tx_copy(g, (uint8_t[]){0x48, 0x89, 0x18}, 3); /* mov [rax], rbx */
      else if (sz == 4)
        tx_copy(g, (uint8_t[]){0x89, 0x18}, 2); /* mov [rax], ebx */
      else if (sz == 2)
        tx_copy(g, (uint8_t[]){0x66, 0x89, 0x18}, 3); /* mov word [rax], bx */
      else if (sz == 1)
        tx_copy(g, (uint8_t[]){0x88, 0x18}, 2); /* mov [rax], bl */
      else {
        cg_err(g, "store size");
        return -1;
      }
      return vs;
    }
    case OL_EX_FIELD: {
      int bi = gen_expr(g, e->u.field.obj);
      OlTypeRef st = e->u.field.obj->ty;
      const char *sname = st.alias_name;
      int ti = ol_program_find_typedef(g->p, sname);
      size_t fi;
      uint32_t off = 0;
      if (ti < 0) return -1;
      for (fi = 0; fi < g->p->typedefs[ti].field_count; ++fi) {
        if (strcmp(g->p->typedefs[ti].fields[fi].name, e->u.field.field) == 0) {
          off = g->p->typedefs[ti].fields[fi].offset;
          break;
        }
      }
      if (bi < 0) return -1;
      emit_mov_r64_rbp(g, 0, bi);
      if (off > 0) {
        /* Stack grows down, so field at positive offset is at LOWER address */
        /* Compute address = base - off */
        emit_mov_gpr_from_slot(g, 3, bi);       /* rbx = base */
        emit_mov_rax_imm64(g, (int64_t)off);    /* rax = off */
        tx_copy(g, (uint8_t[]){0x48, 0x29, 0xc3}, 3); /* sub rbx, rax (rbx = base - off) */
        tx_copy(g, (uint8_t[]){0x48, 0x89, 0xd8}, 3); /* mov rax, rbx */
      }
      {
        uint32_t fsz = type_size_bytes(g->p, &e->ty);
        /* For aggregate types (size > 8), return pointer to field */
        /* For scalar types, load the value */
        if (fsz > 8) {
          /* Aggregate field: pointer is already in rax, just store it */
        } else {
          /* Scalar field: load value */
          if (!emit_load_at_rax(g, fsz)) {
            cg_err(g, "field load size");
            return -1;
          }
        }
      }
      out = alloc_slot(g);
      emit_mov_rbp_r64(g, out, 0);
      return out;
    }
    case OL_EX_INDEX: {
      int ai, ii, li;
      int ti;
      uint32_t esz = 8;
      const char *arr_name = NULL;
      ai = gen_expr(g, e->u.index_.arr);
      ii = gen_expr(g, e->u.index_.index_expr);
      if (ai < 0 || ii < 0) return -1;
      /* Get array element size from type info */
      if (e->u.index_.arr->kind == OL_EX_VAR) {
        arr_name = e->u.index_.arr->u.var_name;
        /* Try local variable first */
        li = lookup_slot(g, arr_name);
        if (li >= 0 && g->loc[li].ty.kind == OL_TY_ALIAS) {
          ti = ol_program_find_typedef(g->p, g->loc[li].ty.alias_name);
          if (ti >= 0 && g->p->typedefs[ti].kind == OL_TYPEDEF_ARRAY) {
            const char *elem = g->p->typedefs[ti].elem_type;
            if (strcmp(elem, "i32") == 0 || strcmp(elem, "u32") == 0 || strcmp(elem, "f32") == 0 || strcmp(elem, "b32") == 0) esz = 4;
            else if (strcmp(elem, "u16") == 0 || strcmp(elem, "i16") == 0 || strcmp(elem, "f16") == 0 || strcmp(elem, "b16") == 0) esz = 2;
            else if (strcmp(elem, "u8") == 0 || strcmp(elem, "i8") == 0 || strcmp(elem, "bool") == 0 || strcmp(elem, "b8") == 0) esz = 1;
          }
        }
        /* If not local, check global variable */
        if (esz == 8) {
          int gi = ol_program_find_global(g->p, arr_name);
          if (gi >= 0) {
            uint32_t gsz = type_size_bytes(g->p, &g->p->globals[(size_t)gi].ty);
            if (gsz > 8u) { /* Aggregate global (array/struct) */
              if (g->p->globals[(size_t)gi].ty.kind == OL_TY_ALIAS) {
                ti = ol_program_find_typedef(g->p, g->p->globals[(size_t)gi].ty.alias_name);
                if (ti >= 0 && g->p->typedefs[ti].kind == OL_TYPEDEF_ARRAY) {
                  const char *elem = g->p->typedefs[ti].elem_type;
                  if (strcmp(elem, "i32") == 0 || strcmp(elem, "u32") == 0 || strcmp(elem, "f32") == 0 || strcmp(elem, "b32") == 0) esz = 4;
                  else if (strcmp(elem, "u16") == 0 || strcmp(elem, "i16") == 0 || strcmp(elem, "f16") == 0 || strcmp(elem, "b16") == 0) esz = 2;
                  else if (strcmp(elem, "u8") == 0 || strcmp(elem, "i8") == 0 || strcmp(elem, "bool") == 0 || strcmp(elem, "b8") == 0) esz = 1;
                }
              }
            }
          }
        }
      }
      emit_mov_gpr_from_slot(g, 3, ai);           /* rbx = array base */
      emit_mov_gpr_from_slot(g, 1, ii);           /* rcx = index */
      if (esz == 8) {
        tx_copy(g, (uint8_t[]){0x48, 0xc1, 0xe1, 0x03}, 4); /* shl rcx, 3 */
      } else if (esz == 4) {
        tx_copy(g, (uint8_t[]){0x48, 0xc1, 0xe1, 0x02}, 4); /* shl rcx, 2 */
      } else if (esz == 2) {
        tx_copy(g, (uint8_t[]){0x48, 0xd1, 0xe1}, 3); /* shl rcx, 1 */
      }
      tx_copy(g, (uint8_t[]){0x48, 0x01, 0xcb}, 3); /* add rbx, rcx */
      tx_copy(g, (uint8_t[]){0x48, 0x89, 0xd8}, 3); /* mov rax, rbx */
      {
        uint32_t ld = type_size_bytes(g->p, &e->ty);
        if (!emit_load_at_rax(g, ld)) {
          cg_err(g, "index load size");
          return -1;
        }
      }
      out = alloc_slot(g);
      emit_mov_rbp_r64(g, out, 0);
      return out;
    }
    default:
      cg_err(g, "expr not implemented");
      return -1;
  }
}

static int gen_stmt(CG *g, OlFuncDef *fn, OlStmt *s);

static int gen_stmt(CG *g, OlFuncDef *fn, OlStmt *s) {
  while (s) {
    if (g->failed) return 0;
    switch (s->kind) {
      case OL_ST_BLOCK: {
        int sv;
        enter_scope(g, &sv);
        if (!gen_stmt(g, fn, s->u.block.first)) {
          exit_scope(g, sv);
          return 0;
        }
        exit_scope(g, sv);
        break;
      }
      case OL_ST_LET: {
        uint32_t let_sz = type_size_bytes(g->p, &s->u.let_.ty);
        int num_slots = (int)((let_sz + 7u) / 8u);  /* Round up to 8-byte slots */
        int is_aggregate = 0;
        int ptr_slot;
        /* Check if type is aggregate (struct or array) */
        if (s->u.let_.ty.kind == OL_TY_ALIAS) {
          int tdi = ol_program_find_typedef(g->p, s->u.let_.ty.alias_name);
          if (tdi >= 0) {
            OlTypeDefKind tdk = g->p->typedefs[(size_t)tdi].kind;
            if (tdk == OL_TYPEDEF_STRUCT || tdk == OL_TYPEDEF_ARRAY) {
              is_aggregate = 1;
            }
          }
        }
        if (!is_aggregate) {
          /* Scalar type: generate init expression, bind to result slot */
          ptr_slot = gen_expr(g, s->u.let_.init);
          if (ptr_slot < 0) return 0;
        } else {
          /* Aggregate type (array/struct):
           * 1. Allocate data space (num_slots slots)
           * 2. Create pointer variable (1 slot) holding data address
           */
          int data_slot = alloc_slots(g, num_slots);
          if (data_slot < 0) return 0;
          /* Compute data address: lea rax, [rbp + slot_disp(data_slot)] */
          ptr_slot = alloc_slot(g);
          if (ptr_slot < 0) return 0;
          {
            int32_t d = slot_disp(data_slot);
            uint8_t rex = 0x48;
            uint8_t modrm;
            if (d >= -128 && d <= 127) {
              modrm = (uint8_t)(0x45u | (0u << 3u)); /* rax, [rbp+disp8] */
              tx_copy(g, (uint8_t[]){rex, 0x8d, modrm, (uint8_t)(int8_t)d}, 4);
            } else {
              modrm = (uint8_t)(0x85u | (0u << 3u)); /* rax, [rbp+disp32] */
              tx_copy(g, (uint8_t[]){rex, 0x8d, modrm}, 3);
              tx_copy(g, (uint8_t *)&d, 4);
            }
            emit_mov_rbp_r64(g, ptr_slot, 0);
          }
        }
        if (!bind_local(g, s->u.let_.name, ptr_slot, &s->u.let_.ty)) return 0;
        break;
      }
      case OL_ST_ASSIGN: {
        OlExpr *lhs = s->u.assign_.lhs;
        int rs;
        if (!lhs) { cg_err(g, "assign has no lhs"); return 0; }
        rs = gen_expr(g, s->u.assign_.rhs);
        if (rs < 0) return 0;
        if (lhs->kind == OL_EX_VAR) {
          /* Simple variable assignment */
          int gi = ol_program_find_global(g->p, lhs->u.var_name);
          int ls = lookup_slot(g, lhs->u.var_name);
          uint32_t asz;
          if (gi >= 0) {
            asz = type_size_bytes(g->p, &g->p->globals[(size_t)gi].ty);
            emit_mov_r64_rbp(g, 0, rs);
            emit_mov_to_global(g, g->p->globals[(size_t)gi].name, asz);
            if (g->failed) return 0;
            break;
          }
          if (ls < 0) return 0;
          /* Check if this is an aggregate type assignment (struct/array) */
          {
            int is_aggregate = 0;
            uint32_t agg_sz = 0;
            OlTypeRef *target_ty = lookup_local_ty(g, lhs->u.var_name);
            if (target_ty && target_ty->kind == OL_TY_ALIAS) {
              int tdi = ol_program_find_typedef(g->p, target_ty->alias_name);
              if (tdi >= 0) {
                OlTypeDefKind tdk = g->p->typedefs[(size_t)tdi].kind;
                if (tdk == OL_TYPEDEF_STRUCT || tdk == OL_TYPEDEF_ARRAY) {
                  is_aggregate = 1;
                  agg_sz = g->p->typedefs[(size_t)tdi].size_bytes;
                }
              }
            }
            if (is_aggregate && agg_sz > 0) {
              /* Aggregate type: copy data word by word */
              uint32_t num_words = (agg_sz + 7u) / 8u;
              uint32_t w;
              for (w = 0; w < num_words; w++) {
                int32_t offset = -(int32_t)(w * 8);  /* Negative offset: field w is at start + offset */
                /* Load source data pointer to rax */
                emit_mov_gpr_from_slot(g, 0, rs);  /* rax = source data pointer (points to first field) */
                /* Add offset to rax (offset is 0 or negative) */
                if (offset != 0) {
                  /* offset is negative, use sub for clarity (sub rax, -offset) */
                  int32_t abs_offset = -offset;  /* Positive value to subtract */
                  if (abs_offset >= -128 && abs_offset <= 127) {
                    tx_copy(g, (uint8_t[]){0x48, 0x83, 0xe8, (uint8_t)(int8_t)abs_offset}, 4); /* sub rax, imm8 */
                  } else {
                    cg_err(g, "aggregate copy offset too large");
                    return 0;
                  }
                }
                /* Load word from [rax] to rdx */
                tx_copy(g, (uint8_t[]){0x48, 0x8b, 0x10}, 3); /* mov rdx, [rax] */
                /* Load target data pointer to rax */
                emit_mov_gpr_from_slot(g, 0, ls);  /* rax = target data pointer */
                /* Add offset to rax */
                if (offset != 0) {
                  int32_t abs_offset = -offset;
                  if (abs_offset >= -128 && abs_offset <= 127) {
                    tx_copy(g, (uint8_t[]){0x48, 0x83, 0xe8, (uint8_t)(int8_t)abs_offset}, 4); /* sub rax, imm8 */
                  } else {
                    cg_err(g, "aggregate copy offset too large");
                    return 0;
                  }
                }
                /* Store word from rdx to [rax] */
                tx_copy(g, (uint8_t[]){0x48, 0x89, 0x10}, 3); /* mov [rax], rdx */
              }
              break;
            }
          }
          /* Indirect local: store rhs through pointer in slot */
          {
            int lidx = find_local_loc(g, lhs->u.var_name);
            if (lidx >= 0 && g->loc[lidx].indirect) {
              OlTypeRef *elt = &g->loc[lidx].ty;
              uint32_t esz = type_size_bytes(g->p, elt);
              emit_mov_gpr_from_slot(g, 0, ls);
              if (type_is_float(elt)) {
                int32_t drs = slot_disp(rs);
                if (elt->kind == OL_TY_F64) {
                  if (drs >= -128 && drs <= 127)
                    tx_copy(g, (uint8_t[]){0xf2, 0x0f, 0x10, 0x45, (uint8_t)(int8_t)drs}, 5);
                  else {
                    tx_copy(g, (uint8_t[]){0xf2, 0x0f, 0x10, 0x85}, 4);
                    tx_copy(g, (uint8_t *)&drs, 4);
                  }
                  tx_copy(g, (uint8_t[]){0xf2, 0x0f, 0x11, 0x00}, 4);
                } else if (elt->kind == OL_TY_F32) {
                  if (drs >= -128 && drs <= 127)
                    tx_copy(g, (uint8_t[]){0xf3, 0x0f, 0x10, 0x45, (uint8_t)(int8_t)drs}, 5);
                  else {
                    tx_copy(g, (uint8_t[]){0xf3, 0x0f, 0x10, 0x85}, 4);
                    tx_copy(g, (uint8_t *)&drs, 4);
                  }
                  tx_copy(g, (uint8_t[]){0xf3, 0x0f, 0x11, 0x00}, 4);
                } else {
                  emit_mov_gpr_from_slot(g, 3, rs);
                  tx_copy(g, (uint8_t[]){0x66, 0x89, 0x18}, 3);
                }
                break;
              }
              emit_mov_gpr_from_slot(g, 3, rs);
              if (esz == 8u)
                tx_copy(g, (uint8_t[]){0x48, 0x89, 0x18}, 3);
              else if (esz == 4u)
                tx_copy(g, (uint8_t[]){0x89, 0x18}, 2);
              else if (esz == 2u)
                tx_copy(g, (uint8_t[]){0x66, 0x89, 0x18}, 3);
              else if (esz == 1u)
                tx_copy(g, (uint8_t[]){0x88, 0x18}, 2);
              else {
                cg_err(g, "indirect store size");
                return 0;
              }
              break;
            }
          }
          /* Non-aggregate: simple pointer/value copy */
          emit_mov_r64_rbp(g, 0, rs);
          emit_mov_rbp_r64(g, ls, 0);
          break;
        } else if (lhs->kind == OL_EX_INDEX) {
          /* Array index assignment: arr[idx] = value */
          uint32_t esz = 8;
          int ai, ii, li;
          const char *arr_name = NULL;
          /* Get array type info to determine element size */
          if (lhs->u.index_.arr->kind == OL_EX_VAR) {
            arr_name = lhs->u.index_.arr->u.var_name;
            /* Try local variable first */
            li = lookup_slot(g, arr_name);
            if (li >= 0 && g->loc[li].ty.kind == OL_TY_ALIAS) {
              int ti = ol_program_find_typedef(g->p, g->loc[li].ty.alias_name);
              if (ti >= 0 && g->p->typedefs[ti].kind == OL_TYPEDEF_ARRAY) {
                const char *elem = g->p->typedefs[ti].elem_type;
                if (strcmp(elem, "i32") == 0 || strcmp(elem, "u32") == 0 || strcmp(elem, "f32") == 0 || strcmp(elem, "b32") == 0) esz = 4;
                else if (strcmp(elem, "u16") == 0 || strcmp(elem, "i16") == 0 || strcmp(elem, "f16") == 0 || strcmp(elem, "b16") == 0) esz = 2;
                else if (strcmp(elem, "u8") == 0 || strcmp(elem, "i8") == 0 || strcmp(elem, "bool") == 0 || strcmp(elem, "b8") == 0) esz = 1;
              }
            }
            /* If not local, check global variable */
            if (esz == 8) {
              int gi = ol_program_find_global(g->p, arr_name);
              if (gi >= 0) {
                uint32_t gsz = type_size_bytes(g->p, &g->p->globals[(size_t)gi].ty);
                if (gsz > 8u) { /* Aggregate global (array/struct) */
                  if (g->p->globals[(size_t)gi].ty.kind == OL_TY_ALIAS) {
                    int ti = ol_program_find_typedef(g->p, g->p->globals[(size_t)gi].ty.alias_name);
                    if (ti >= 0 && g->p->typedefs[ti].kind == OL_TYPEDEF_ARRAY) {
                      const char *elem = g->p->typedefs[ti].elem_type;
                      if (strcmp(elem, "i32") == 0 || strcmp(elem, "u32") == 0 || strcmp(elem, "f32") == 0 || strcmp(elem, "b32") == 0) esz = 4;
                      else if (strcmp(elem, "u16") == 0 || strcmp(elem, "i16") == 0 || strcmp(elem, "f16") == 0 || strcmp(elem, "b16") == 0) esz = 2;
                      else if (strcmp(elem, "u8") == 0 || strcmp(elem, "i8") == 0 || strcmp(elem, "bool") == 0 || strcmp(elem, "b8") == 0) esz = 1;
                    }
                  }
                }
              }
            }
          }
          /* Generate array base address and index */
          ai = gen_expr(g, lhs->u.index_.arr);
          if (ai < 0) return 0;
          ii = gen_expr(g, lhs->u.index_.index_expr);
          if (ii < 0) return 0;
          /* arr base to rbx, index to rcx */
          emit_mov_gpr_from_slot(g, 3, ai);  /* rbx = arr base */
          emit_mov_gpr_from_slot(g, 1, ii);  /* rcx = index */
          /* Scale index */
          if (esz == 8) {
            tx_copy(g, (uint8_t[]){0x48, 0xc1, 0xe1, 0x03}, 4); /* shl rcx, 3 */
          } else if (esz == 4) {
            tx_copy(g, (uint8_t[]){0x48, 0xc1, 0xe1, 0x02}, 4); /* shl rcx, 2 */
          } else if (esz == 2) {
            tx_copy(g, (uint8_t[]){0x48, 0xd1, 0xe1}, 3); /* shl rcx, 1 */
          }
          /* Compute element address: rbx = rbx + rcx */
          tx_copy(g, (uint8_t[]){0x48, 0x01, 0xcb}, 3); /* add rbx, rcx */
          /* Store value: [rbx] = rax (value from rhs) */
          emit_mov_r64_rbp(g, 0, rs);  /* rax = rhs value */
          if (esz == 8) {
            tx_copy(g, (uint8_t[]){0x48, 0x89, 0x03}, 3); /* mov [rbx], rax */
          } else if (esz == 4) {
            tx_copy(g, (uint8_t[]){0x89, 0x03}, 2); /* mov [rbx], eax */
          } else if (esz == 2) {
            tx_copy(g, (uint8_t[]){0x66, 0x89, 0x03}, 3); /* mov word [rbx], ax */
          } else if (esz == 1) {
            tx_copy(g, (uint8_t[]){0x88, 0x03}, 2); /* mov [rbx], al */
          }
          break;
        } else if (lhs->kind == OL_EX_FIELD) {
          /* Struct field assignment: obj.field = value */
          int bi;
          OlTypeRef st;
          const char *sname;
          int ti;
          size_t fi;
          uint32_t off = 0;
          uint32_t fsz = 8;
          /* Get struct base address */
          bi = gen_expr(g, lhs->u.field.obj);
          if (bi < 0) return 0;
          /* Calculate field offset */
          st = lhs->u.field.obj->ty;
          sname = st.alias_name;
          ti = ol_program_find_typedef(g->p, sname);
          if (ti < 0) return 0;
          for (fi = 0; fi < g->p->typedefs[ti].field_count; ++fi) {
            if (strcmp(g->p->typedefs[ti].fields[fi].name, lhs->u.field.field) == 0) {
              off = g->p->typedefs[ti].fields[fi].offset;
              break;
            }
          }
          /* Get field size from the typedef info */
          {
            int fti = ol_program_find_typedef(g->p, sname);
            if (fti >= 0) {
              const char *ftype = g->p->typedefs[ti].fields[fi].type_name;
              if (strcmp(ftype, "bool") == 0 || strcmp(ftype, "u8") == 0 || strcmp(ftype, "i8") == 0 || strcmp(ftype, "b8") == 0) fsz = 1;
              else if (strcmp(ftype, "u16") == 0 || strcmp(ftype, "i16") == 0 || strcmp(ftype, "f16") == 0 || strcmp(ftype, "b16") == 0) fsz = 2;
              else if (strcmp(ftype, "i32") == 0 || strcmp(ftype, "u32") == 0 || strcmp(ftype, "f32") == 0 || strcmp(ftype, "b32") == 0) fsz = 4;
              else if (strcmp(ftype, "i64") == 0 || strcmp(ftype, "u64") == 0 || strcmp(ftype, "f64") == 0 || strcmp(ftype, "b64") == 0 || strcmp(ftype, "ptr") == 0) fsz = 8;
              else {
                /* Nested typedef - look up size */
                int k = ol_program_find_typedef(g->p, ftype);
                if (k >= 0) fsz = g->p->typedefs[k].size_bytes;
                if (fsz == 0) fsz = 8;
              }
            }
          }
          /* Load struct base into rbx */
          emit_mov_gpr_from_slot(g, 3, bi);  /* rbx = struct base */
          /* If offset > 0, subtract it (stack grows down) */
          if (off > 0) {
            emit_mov_rax_imm64(g, (int64_t)off);
            tx_copy(g, (uint8_t[]){0x48, 0x29, 0xc3}, 3); /* sub rbx, rax (rbx = base - off) */
          }
          /* For aggregate types (fsz > 8), copy data word by word */
          /* For scalar types, load and store */
          if (fsz > 8) {
            uint32_t num_words = (fsz + 7u) / 8u;
            uint32_t w;
            /* rbx = target field address (already computed) */
            /* Load source pointer to rcx */
            emit_mov_gpr_from_slot(g, 1, rs);  /* rcx = source pointer */
            for (w = 0; w < num_words; w++) {
              int32_t neg_offset = -(int32_t)(w * 8);
              /* Load from source: rax = [rcx + neg_offset] = [rcx - off8] */
              if (w == 0) {
                tx_copy(g, (uint8_t[]){0x48, 0x8b, 0x01}, 3); /* mov rax, [rcx] */
              } else {
                tx_copy(g, (uint8_t[]){0x48, 0x8b, 0x41, (uint8_t)(int8_t)neg_offset}, 4); /* mov rax, [rcx-off8] */
              }
              /* Store to target: [rbx + neg_offset] = rax = [rbx - off8] */
              if (w == 0) {
                tx_copy(g, (uint8_t[]){0x48, 0x89, 0x03}, 3); /* mov [rbx], rax */
              } else {
                tx_copy(g, (uint8_t[]){0x48, 0x89, 0x43, (uint8_t)(int8_t)neg_offset}, 4); /* mov [rbx-off8], rax */
              }
            }
          } else {
            /* Scalar: load value into rax */
            emit_mov_r64_rbp(g, 0, rs);  /* rax = rhs value */
            /* Store value at [rbx] */
            if (fsz == 8) {
              tx_copy(g, (uint8_t[]){0x48, 0x89, 0x03}, 3); /* mov [rbx], rax */
            } else if (fsz == 4) {
              tx_copy(g, (uint8_t[]){0x89, 0x03}, 2); /* mov [rbx], eax */
            } else if (fsz == 2) {
              tx_copy(g, (uint8_t[]){0x66, 0x89, 0x03}, 3); /* mov word [rbx], ax */
            } else if (fsz == 1) {
              tx_copy(g, (uint8_t[]){0x88, 0x03}, 2); /* mov [rbx], al */
            }
          }
          break;
        } else {
          cg_err(g, "unsupported assign target");
          return 0;
        }
      }
      case OL_ST_IF: {
        int sc, lelse, lend;
        sc = gen_expr(g, s->u.if_.cond);
        if (sc < 0) return 0;
        lelse = new_lbl(g);
        lend = new_lbl(g);
        if (lelse < 0 || lend < 0 || g->failed) return 0;
        emit_branch_cond_to_rax(g, s->u.if_.cond, sc);
        emit_test_rax(g);
        emit_je_rel32(g, lelse);
        if (!gen_stmt(g, fn, s->u.if_.then_arm)) return 0;
        emit_jmp_rel32(g, lend);
        bind_lbl(g, lelse);
        if (s->u.if_.else_arm) {
          if (!gen_stmt(g, fn, s->u.if_.else_arm)) return 0;
        }
        bind_lbl(g, lend);
        break;
      }
      case OL_ST_WHILE: {
        int head, done, sc;
        head = new_lbl(g);
        done = new_lbl(g);
        if (head < 0 || done < 0 || g->failed) return 0;
        if (g->loop_depth >= 16) { CG_FAIL(g, "too many nested loops"); return 0; }
        g->loop_head[g->loop_depth] = head;
        g->loop_done[g->loop_depth] = done;
        g->loop_depth++;
        bind_lbl(g, head);
        sc = gen_expr(g, s->u.while_.cond);
        if (sc < 0) { g->loop_depth--; return 0; }
        emit_branch_cond_to_rax(g, s->u.while_.cond, sc);
        emit_test_rax(g);
        emit_je_rel32(g, done);
        if (!gen_stmt(g, fn, s->u.while_.body)) { g->loop_depth--; return 0; }
        emit_jmp_rel32(g, head);
        bind_lbl(g, done);
        g->loop_depth--;
        break;
      }
      case OL_ST_BREAK:
        if (g->loop_depth <= 0) { CG_FAIL(g, "break outside loop"); return 0; }
        emit_jmp_rel32(g, g->loop_done[g->loop_depth - 1]);
        break;
      case OL_ST_CONTINUE:
        if (g->loop_depth <= 0) { CG_FAIL(g, "continue outside loop"); return 0; }
        emit_jmp_rel32(g, g->loop_head[g->loop_depth - 1]);
        break;
      case OL_ST_RETURN:
        if (s->u.ret.val) {
          int rs = gen_expr(g, s->u.ret.val);
          if (rs < 0) return 0;
          if (type_is_float(&fn->ret)) {
            /* Float return: load into xmm0 (standard convention) then also copy bits to rax for OLang ABI */
            int32_t rd = slot_disp(rs);
            if (fn->ret.kind == OL_TY_F64) {
              if (rd >= -128 && rd <= 127) tx_copy(g, (uint8_t[]){0xf2, 0x0f, 0x10, 0x45, (uint8_t)(int8_t)rd}, 5);
              else { tx_copy(g, (uint8_t[]){0xf2, 0x0f, 0x10, 0x85}, 4); tx_copy(g, (uint8_t*)&rd, 4); }
              tx_copy(g, (uint8_t[]){0x66, 0x48, 0x0f, 0x7e, 0xc0}, 5); /* movq rax, xmm0 */
            } else {
              if (rd >= -128 && rd <= 127) tx_copy(g, (uint8_t[]){0xf3, 0x0f, 0x10, 0x45, (uint8_t)(int8_t)rd}, 5);
              else { tx_copy(g, (uint8_t[]){0xf3, 0x0f, 0x10, 0x85}, 4); tx_copy(g, (uint8_t*)&rd, 4); }
              tx_copy(g, (uint8_t[]){0x66, 0x0f, 0x7e, 0xc0}, 4); /* movd eax, xmm0 */
            }
          } else if (type_is_byte_like(&fn->ret))
            emit_mov_eax_zext8_from_slot(g, rs);
          else if (type_is_word_like(&fn->ret))
            emit_mov_eax_zext16_from_slot(g, rs);
          else if (fn->ret.kind == OL_TY_I32 || fn->ret.kind == OL_TY_U32)
            emit_mov_eax_from_slot(g, rs);
          else
            emit_mov_r64_rbp(g, 0, rs);
        }
        emit_jmp_rel32(g, g->fn_exit_lbl);
        break;
      case OL_ST_EXPR:
        if (gen_expr(g, s->u.expr) < 0) return 0;
        break;
      case OL_ST_DEREF: {
        int li = find_local_loc(g, s->u.deref.bind);
        uint32_t dsz = type_size_bytes(g->p, &s->u.deref.ty);
        if (li < 0) return 0;
        if (dsz != 1u && dsz != 2u && dsz != 4u && dsz != 8u) {
          cg_err(g, "deref element size");
          return 0;
        }
        g->loc[li].indirect = 1u;
        g->loc[li].ty = s->u.deref.ty;
        break;
      }
      default:
        return 0;
    }
    s = s->next;
  }
  return 1;
}

static int gen_function(CG *g, OlFuncDef *fn) {
  size_t start = g->tx_len;
  size_t i;
  size_t frame_imm_patch;
  int32_t fb;
  (void)start;
  g->next_slot = (int)fn->param_count;
  g->nloc = 0;
  g->nlbl = 0;
  memset(g->lbl_set, 0, sizeof(g->lbl_set));
  g->fn_exit_lbl = new_lbl(g);
  if (g->fn_exit_lbl < 0) return 0;

  frame_imm_patch = emit_prologue_frame_placeholder(g);
  if (g->failed) return 0;
  /* OLang ABI: spill register params r12,r13,r14,r15,rdi,rsi,r8,r9 to stack slots */
  {
    static const uint8_t pre[8][3] = {
        {0x4c, 0x89, 0x65}, /* mov [rbp+d], r12 */
        {0x4c, 0x89, 0x6d}, /* mov [rbp+d], r13 */
        {0x4c, 0x89, 0x75}, /* mov [rbp+d], r14 */
        {0x4c, 0x89, 0x7d}, /* mov [rbp+d], r15 */
        {0x48, 0x89, 0x7d}, /* mov [rbp+d], rdi */
        {0x48, 0x89, 0x75}, /* mov [rbp+d], rsi */
        {0x4c, 0x89, 0x45}, /* mov [rbp+d], r8  */
        {0x4c, 0x89, 0x4d}, /* mov [rbp+d], r9  */
    };
    for (i = 0; i < fn->param_count; ++i) {
      int slot = (int)i;
      int32_t d = slot_disp(slot);
      if (i < 8) {
        if (d < -128 || d > 127) {
          cg_err(g, "param frame too large");
          return 0;
        }
        tx_copy(g, pre[i], 3);
        txb(g, (uint8_t)(int8_t)d);
      } else {
        /* Stack-overflow params: load from [rbp + 16 + (i-8)*8] into rax, store to slot */
        int32_t src_off = 16 + (int32_t)(i - 8) * 8;
        /* mov rax, [rbp + src_off] */
        tx_copy(g, (uint8_t[]){0x48, 0x8b, 0x85}, 3);
        tx_copy(g, (uint8_t *)&src_off, 4);
        /* mov [rbp + d], rax */
        if (d >= -128 && d <= 127) {
          tx_copy(g, (uint8_t[]){0x48, 0x89, 0x45, (uint8_t)(int8_t)d}, 4);
        } else {
          tx_copy(g, (uint8_t[]){0x48, 0x89, 0x85}, 3);
          tx_copy(g, (uint8_t *)&d, 4);
        }
      }
      if (!bind_local(g, fn->params[i].name, slot, &fn->params[i].ty)) return 0;
    }
  }

  if (!gen_stmt(g, fn, fn->body)) return 0;
  if (g->failed) return 0;

  bind_lbl(g, g->fn_exit_lbl);
  fb = frame_bytes_for_slot_count(g->next_slot);
  patch_u32_at(g, frame_imm_patch, fb);
  emit_epilogue(g, fb);

  resolve_fixups(g);
  if (g->failed) return 0;
  return 1;
}

typedef struct {
  char sec[24];
  uint64_t off;
  char sym[128];
} PendingAbs64;

typedef struct {
  int kind; /* -1 none, 0 .rodata, 1 .data, 2 .bss */
  uint64_t off;
} GlobLay;

static int sec_index_named(const OobjObject *o, const char *name) {
  size_t i;
  for (i = 0; i < o->section_count; ++i) {
    if (strcmp(o->sections[i].name, name) == 0) return (int)i;
  }
  return -1;
}

static size_t align_sz(size_t x, size_t al) {
  return (x + al - 1u) / al * al;
}

static int ol_codegen_x64_emit_globals(CG *g, GlobLay *gl, PendingAbs64 *abs64, size_t *n_abs64, char *err, size_t err_len) {
  size_t i;
  for (i = 0; i < g->p->global_count; ++i) gl[i].kind = -1;
  for (i = 0; i < g->p->global_count; ++i) {
    OlGlobalDef *gd = &g->p->globals[i];
    uint32_t sz = type_size_bytes(g->p, &gd->ty);
    size_t al = (sz >= 8u) ? 8u : ((sz >= 4u) ? 4u : 1u);
    int use_ro = 0, use_data = 0, use_bss = 0;
    const char *secname = ".data";

    if (gd->section == OL_GSEC_RODATA)
      use_ro = 1;
    else if (gd->section == OL_GSEC_DATA)
      use_data = 1;
    else if (gd->section == OL_GSEC_BSS)
      use_bss = 1;
    else if (gd->section == OL_GSEC_CUSTOM) {
      use_data = 1;
      secname = ".data";
    } else {
      if (!gd->init)
        use_bss = 1;
      else if ((gd->init->kind == OL_EX_INT && gd->init->u.int_.int_val == 0) ||
               (gd->init->kind == OL_EX_BOOL && gd->init->u.bool_val == 0) ||
               (gd->init->kind == OL_EX_CHAR && gd->init->u.char_val == 0) ||
               (gd->init->kind == OL_EX_FLOAT && gd->init->u.float_.float_val == 0.0))
        use_bss = 1;
      else
        use_data = 1;
    }

    if (use_bss) {
      uint8_t *nb;
      size_t na = align_sz(g->bss_len, al);
      size_t pad = na - g->bss_len;
      size_t add = pad + (size_t)sz;
      nb = (uint8_t *)realloc(g->bss, g->bss_len + add);
      if (!nb) {
        snprintf(err, err_len, "oom bss");
        return 0;
      }
      memset(nb + g->bss_len, 0, add);
      g->bss = nb;
      gl[i].kind = 2;
      gl[i].off = (uint64_t)na;
      g->bss_len += add;
      continue;
    }

    if (use_ro) {
      size_t na = align_sz(g->ro_len, al);
      size_t pad = na - g->ro_len;
      uint8_t *nr;
      size_t off;
      if (pad) {
        nr = (uint8_t *)realloc(g->ro, g->ro_len + pad);
        if (!nr) {
          snprintf(err, err_len, "oom ro align");
          return 0;
        }
        memset(nr + g->ro_len, 0, pad);
        g->ro = nr;
        g->ro_len += pad;
      }
      off = g->ro_len;
      nr = (uint8_t *)realloc(g->ro, g->ro_len + (size_t)sz);
      if (!nr) {
        snprintf(err, err_len, "oom ro global");
        return 0;
      }
      g->ro = nr;
      memset(g->ro + g->ro_len, 0, (size_t)sz);
      if (gd->init->kind == OL_EX_INT || gd->init->kind == OL_EX_BOOL || gd->init->kind == OL_EX_CHAR) {
        uint64_t v = 0;
        if (gd->init->kind == OL_EX_INT)
          v = (uint64_t)gd->init->u.int_.int_val;
        else if (gd->init->kind == OL_EX_BOOL)
          v = (uint64_t)(gd->init->u.bool_val ? 1u : 0u);
        else
          v = (uint64_t)gd->init->u.char_val;
        if (sz == 1u) g->ro[g->ro_len] = (uint8_t)v;
        else if (sz == 2u) { uint16_t u = (uint16_t)v; memcpy(g->ro + g->ro_len, &u, 2); }
        else if (sz == 4u) { uint32_t u = (uint32_t)v; memcpy(g->ro + g->ro_len, &u, 4); }
        else { memcpy(g->ro + g->ro_len, &v, 8); }
      } else if (gd->init->kind == OL_EX_FLOAT) {
        if (sz == 8u) { double d = gd->init->u.float_.float_val; memcpy(g->ro + g->ro_len, &d, 8); }
        else if (sz == 4u) { float f = (float)gd->init->u.float_.float_val; memcpy(g->ro + g->ro_len, &f, 4); }
        else if (sz == 2u) { uint16_t h = f32_to_f16_bits((float)gd->init->u.float_.float_val); memcpy(g->ro + g->ro_len, &h, 2); }
      } else if (gd->init->kind == OL_EX_STR) {
        OlStringLit *sl = &g->p->strings[gd->init->u.str_idx];
        if (*n_abs64 >= 64) {
          snprintf(err, err_len, "too many abs relocs");
          return 0;
        }
        abs64[*n_abs64].off = (uint64_t)off;
        snprintf(abs64[*n_abs64].sec, sizeof(abs64[*n_abs64].sec), "%s", ".rodata");
        snprintf(abs64[*n_abs64].sym, sizeof(abs64[*n_abs64].sym), "%s", sl->sym);
        (*n_abs64)++;
      } else {
        snprintf(err, err_len, "bad rodata global init");
        return 0;
      }
      g->ro_len += (size_t)sz;
      gl[i].kind = 0;
      gl[i].off = (uint64_t)off;
      continue;
    }

    if (use_data) {
      size_t na = align_sz(g->data_len, al);
      size_t pad = na - g->data_len;
      uint8_t *nd;
      size_t off;
      if (pad) {
        nd = (uint8_t *)realloc(g->data, g->data_len + pad);
        if (!nd) {
          snprintf(err, err_len, "oom data align");
          return 0;
        }
        memset(nd + g->data_len, 0, pad);
        g->data = nd;
        g->data_len += pad;
      }
      off = g->data_len;
      nd = (uint8_t *)realloc(g->data, g->data_len + (size_t)sz);
      if (!nd) {
        snprintf(err, err_len, "oom data global");
        return 0;
      }
      g->data = nd;
      memset(g->data + g->data_len, 0, (size_t)sz);
      if (!gd->init) {
        snprintf(err, err_len, ".data global needs initializer");
        return 0;
      }
      if (gd->init->kind == OL_EX_INT || gd->init->kind == OL_EX_BOOL || gd->init->kind == OL_EX_CHAR) {
        uint64_t v = 0;
        if (gd->init->kind == OL_EX_INT)
          v = (uint64_t)gd->init->u.int_.int_val;
        else if (gd->init->kind == OL_EX_BOOL)
          v = (uint64_t)(gd->init->u.bool_val ? 1u : 0u);
        else
          v = (uint64_t)gd->init->u.char_val;
        if (sz == 1u) g->data[g->data_len] = (uint8_t)v;
        else if (sz == 2u) { uint16_t u = (uint16_t)v; memcpy(g->data + g->data_len, &u, 2); }
        else if (sz == 4u) { uint32_t u = (uint32_t)v; memcpy(g->data + g->data_len, &u, 4); }
        else { memcpy(g->data + g->data_len, &v, 8); }
      } else if (gd->init->kind == OL_EX_FLOAT) {
        if (sz == 8u) { double d = gd->init->u.float_.float_val; memcpy(g->data + g->data_len, &d, 8); }
        else if (sz == 4u) { float f = (float)gd->init->u.float_.float_val; memcpy(g->data + g->data_len, &f, 4); }
        else if (sz == 2u) { uint16_t h = f32_to_f16_bits((float)gd->init->u.float_.float_val); memcpy(g->data + g->data_len, &h, 2); }
      } else if (gd->init->kind == OL_EX_STR) {
        OlStringLit *sl = &g->p->strings[gd->init->u.str_idx];
        if (*n_abs64 >= 64) {
          snprintf(err, err_len, "too many abs relocs");
          return 0;
        }
        abs64[*n_abs64].off = (uint64_t)off;
        snprintf(abs64[*n_abs64].sec, sizeof(abs64[*n_abs64].sec), "%s", secname);
        snprintf(abs64[*n_abs64].sym, sizeof(abs64[*n_abs64].sym), "%s", sl->sym);
        (*n_abs64)++;
      } else {
        snprintf(err, err_len, "bad data global init");
        return 0;
      }
      g->data_len += (size_t)sz;
      gl[i].kind = 1;
      gl[i].off = (uint64_t)off;
    }
  }
  return 1;
}

int ol_codegen_x64(const OlTargetInfo *ti, OlProgram *program, OobjObject *out, char *err, size_t err_len) {
  CG g;
  size_t i;
  PendingAbs64 abs64[64];
  size_t n_abs64 = 0;
  GlobLay gl[128];
  int ix_ro, ix_data, ix_bss;
  memset(&g, 0, sizeof(g));
  g.p = program;
  g.obj = out;
  oobj_init(out);
  snprintf(out->target, sizeof(out->target), "%s", ti->name);

  if (program->global_count > 128) {
    snprintf(err, err_len, "too many globals");
    oobj_free(out);
    return 0;
  }

  for (i = 0; i < program->string_count; ++i) {
    OlStringLit *sl = &program->strings[i];
    uint8_t *nr = (uint8_t *)realloc(g.ro, g.ro_len + sl->len + 1u);
    if (!nr) {
      snprintf(err, err_len, "oom rodata");
      free(g.tx);
      free(g.ro);
      free(g.data);
      free(g.bss);
      return 0;
    }
    g.ro = nr;
    memcpy(g.ro + g.ro_len, sl->bytes, sl->len + 1u);
    g.ro_len += sl->len + 1u;
  }

  if (!ol_codegen_x64_emit_globals(&g, gl, abs64, &n_abs64, err, err_len)) {
    free(g.tx);
    free(g.ro);
    free(g.data);
    free(g.bss);
    oobj_free(out);
    return 0;
  }

  for (i = 0; i < program->func_count; ++i) {
    OlFuncDef *fn = &program->funcs[i];
    size_t fstart = g.tx_len;
    if (!gen_function(&g, fn)) {
      snprintf(err, err_len, "codegen: %s", g.err[0] ? g.err : "failed");
      free(g.tx);
      free(g.ro);
      free(g.data);
      free(g.bss);
      oobj_free(out);
      return 0;
    }
    if (!oobj_append_symbol(out, fn->link_name, 0, (uint64_t)fstart, fn->is_export ? 1 : 0)) {
      snprintf(err, err_len, "oom fn sym");
      free(g.tx);
      free(g.ro);
      free(g.data);
      free(g.bss);
      oobj_free(out);
      return 0;
    }
  }

  if (!oobj_append_section(out, ".text", 16u, 5u, g.tx, g.tx_len)) {
    snprintf(err, err_len, "oom text section");
    free(g.tx);
    free(g.ro);
    free(g.data);
    free(g.bss);
    oobj_free(out);
    return 0;
  }
  free(g.tx);
  g.tx = NULL;

  if (g.ro_len > 0) {
    if (!oobj_append_section(out, ".rodata", 16u, 4u, g.ro, g.ro_len)) {
      snprintf(err, err_len, "oom rodata section");
      free(g.ro);
      free(g.data);
      free(g.bss);
      oobj_free(out);
      return 0;
    }
  }
  free(g.ro);
  g.ro = NULL;

  if (g.data_len > 0) {
    if (!oobj_append_section(out, ".data", 8u, 3u, g.data, g.data_len)) {
      snprintf(err, err_len, "oom data section");
      free(g.data);
      free(g.bss);
      oobj_free(out);
      return 0;
    }
  }
  free(g.data);
  g.data = NULL;

  if (g.bss_len > 0) {
    if (!oobj_append_section(out, ".bss", 8u, 3u, g.bss, g.bss_len)) {
      snprintf(err, err_len, "oom bss section");
      free(g.bss);
      oobj_free(out);
      return 0;
    }
  }
  free(g.bss);
  g.bss = NULL;

  ix_ro = sec_index_named(out, ".rodata");
  ix_data = sec_index_named(out, ".data");
  ix_bss = sec_index_named(out, ".bss");

  if (program->string_count > 0 && ix_ro < 0) {
    snprintf(err, err_len, "internal: rodata section missing for strings");
    oobj_free(out);
    return 0;
  }

  if (ix_ro >= 0) {
    size_t base = 0;
    for (i = 0; i < program->string_count; ++i) {
      OlStringLit *sl = &program->strings[i];
      if (!oobj_append_symbol(out, sl->sym, (int32_t)ix_ro, (uint64_t)base, 1)) {
        snprintf(err, err_len, "oom str sym");
        oobj_free(out);
        return 0;
      }
      base += sl->len + 1u;
    }
  }

  for (i = 0; i < program->global_count; ++i) {
    int secix;
    if (gl[i].kind < 0) continue;
    if (gl[i].kind == 0) {
      secix = ix_ro;
    } else if (gl[i].kind == 1) {
      secix = ix_data;
    } else {
      secix = ix_bss;
    }
    if (secix < 0) {
      snprintf(err, err_len, "internal: global section missing");
      oobj_free(out);
      return 0;
    }
    if (!oobj_append_symbol(out, program->globals[i].name, secix, gl[i].off, 1)) {
      snprintf(err, err_len, "oom global sym");
      oobj_free(out);
      return 0;
    }
  }

  for (i = 0; i < n_abs64; ++i) {
    int seci = sec_index_named(out, abs64[i].sec);
    if (seci < 0) {
      snprintf(err, err_len, "abs reloc: missing section %s", abs64[i].sec);
      oobj_free(out);
      return 0;
    }
    if (!oobj_append_reloc(out, (uint32_t)seci, abs64[i].off, abs64[i].sym, OOBJ_RELOC_ABS64, 0)) {
      snprintf(err, err_len, "oom abs reloc");
      oobj_free(out);
      return 0;
    }
  }

  for (i = 0; i < program->extern_count; ++i) {
    size_t k;
    int skip = 0;
    for (k = 0; k < program->func_count; ++k) {
      if (program->funcs[k].is_export && strcmp(program->funcs[k].name, program->externs[i].name) == 0) {
        skip = 1;
        break;
      }
    }
    if (skip) continue;
    if (oobj_find_symbol(out, program->externs[i].name) < 0) {
      if (!oobj_append_symbol(out, program->externs[i].name, -1, 0, 1)) {
        snprintf(err, err_len, "oom extern sym");
        oobj_free(out);
        return 0;
      }
    }
  }

  return 1;
}
