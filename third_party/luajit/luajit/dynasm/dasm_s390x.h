/*
** DynASM s390x encoding engine.
** Copyright (C) 2005-2017 Mike Pall. All rights reserved.
** Released under the MIT license. See dynasm.lua for full copyright notice.
*/

#include <stddef.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#define DASM_ARCH		"s390x"

#ifndef DASM_EXTERN
#define DASM_EXTERN(a,b,c,d)	0
#endif

/* Action definitions. */
enum {
  DASM_STOP, DASM_SECTION, DASM_ESC, DASM_REL_EXT,
  /* The following actions need a buffer position. */
  DASM_ALIGN, DASM_REL_LG, DASM_LABEL_LG,
  /* The following actions also have an argument. */
  DASM_REL_PC, DASM_LABEL_PC,
  DASM_DISP12, DASM_DISP20,
  DASM_IMM8, DASM_IMM16, DASM_IMM32,
  DASM_LEN8R,DASM_LEN4HR,DASM_LEN4LR,
  DASM__MAX
};

/* Maximum number of section buffer positions for a single dasm_put() call. */
#define DASM_MAXSECPOS		25

/* DynASM encoder status codes. Action list offset or number are or'ed in. */
#define DASM_S_OK		0x00000000
#define DASM_S_NOMEM		0x01000000
#define DASM_S_PHASE		0x02000000
#define DASM_S_MATCH_SEC	0x03000000
#define DASM_S_RANGE_I		0x11000000
#define DASM_S_RANGE_SEC	0x12000000
#define DASM_S_RANGE_LG		0x13000000
#define DASM_S_RANGE_PC		0x14000000
#define DASM_S_RANGE_REL	0x15000000
#define DASM_S_UNDEF_LG		0x21000000
#define DASM_S_UNDEF_PC		0x22000000

/* Macros to convert positions (8 bit section + 24 bit index). */
#define DASM_POS2IDX(pos)	((pos)&0x00ffffff)
#define DASM_POS2BIAS(pos)	((pos)&0xff000000)
#define DASM_SEC2POS(sec)	((sec)<<24)
#define DASM_POS2SEC(pos)	((pos)>>24)
#define DASM_POS2PTR(D, pos)	(D->sections[DASM_POS2SEC(pos)].rbuf + (pos))

/* Action list type. */
typedef const unsigned short *dasm_ActList;

/* Per-section structure. */
typedef struct dasm_Section {
  int *rbuf;                    /* Biased buffer pointer (negative section bias). */
  int *buf;                     /* True buffer pointer. */
  size_t bsize;                 /* Buffer size in bytes. */
  int pos;                      /* Biased buffer position. */
  int epos;                     /* End of biased buffer position - max single put. */
  int ofs;                      /* Byte offset into section. */
} dasm_Section;

/* Core structure holding the DynASM encoding state. */
struct dasm_State {
  size_t psize;                 /* Allocated size of this structure. */
  dasm_ActList actionlist;      /* Current actionlist pointer. */
  int *lglabels;                /* Local/global chain/pos ptrs. */
  size_t lgsize;
  int *pclabels;                /* PC label chains/pos ptrs. */
  size_t pcsize;
  void **globals;               /* Array of globals. */
  dasm_Section *section;        /* Pointer to active section. */
  size_t codesize;              /* Total size of all code sections. */
  int maxsection;               /* 0 <= sectionidx < maxsection. */
  int status;                   /* Status code. */
  dasm_Section sections[1];     /* All sections. Alloc-extended. */
};

/* The size of the core structure depends on the max. number of sections. */
#define DASM_PSZ(ms)	(sizeof(dasm_State)+(ms-1)*sizeof(dasm_Section))


/* Initialize DynASM state. */
void dasm_init(Dst_DECL, int maxsection)
{
  dasm_State *D;
  size_t psz = 0;
  Dst_REF = NULL;
  DASM_M_GROW(Dst, struct dasm_State, Dst_REF, psz, DASM_PSZ(maxsection));
  D = Dst_REF;
  D->psize = psz;
  D->lglabels = NULL;
  D->lgsize = 0;
  D->pclabels = NULL;
  D->pcsize = 0;
  D->globals = NULL;
  D->maxsection = maxsection;
  memset((void *)D->sections, 0, maxsection * sizeof(dasm_Section));
}

/* Free DynASM state. */
void dasm_free(Dst_DECL)
{
  dasm_State *D = Dst_REF;
  int i;
  for (i = 0; i < D->maxsection; i++)
    if (D->sections[i].buf)
      DASM_M_FREE(Dst, D->sections[i].buf, D->sections[i].bsize);
  if (D->pclabels)
    DASM_M_FREE(Dst, D->pclabels, D->pcsize);
  if (D->lglabels)
    DASM_M_FREE(Dst, D->lglabels, D->lgsize);
  DASM_M_FREE(Dst, D, D->psize);
}

/* Setup global label array. Must be called before dasm_setup(). */
void dasm_setupglobal(Dst_DECL, void **gl, unsigned int maxgl)
{
  dasm_State *D = Dst_REF;
  D->globals = gl;
  DASM_M_GROW(Dst, int, D->lglabels, D->lgsize, (10 + maxgl) * sizeof(int));
}

/* Grow PC label array. Can be called after dasm_setup(), too. */
void dasm_growpc(Dst_DECL, unsigned int maxpc)
{
  dasm_State *D = Dst_REF;
  size_t osz = D->pcsize;
  DASM_M_GROW(Dst, int, D->pclabels, D->pcsize, maxpc * sizeof(int));
  memset((void *)(((unsigned char *)D->pclabels) + osz), 0, D->pcsize - osz);
}

/* Setup encoder. */
void dasm_setup(Dst_DECL, const void *actionlist)
{
  dasm_State *D = Dst_REF;
  int i;
  D->actionlist = (dasm_ActList) actionlist;
  D->status = DASM_S_OK;
  D->section = &D->sections[0];
  memset((void *)D->lglabels, 0, D->lgsize);
  if (D->pclabels)
    memset((void *)D->pclabels, 0, D->pcsize);
  for (i = 0; i < D->maxsection; i++) {
    D->sections[i].pos = DASM_SEC2POS(i);
    D->sections[i].rbuf = D->sections[i].buf - D->sections[i].pos;
    D->sections[i].ofs = 0;
  }
}


#ifdef DASM_CHECKS
#define CK(x, st) \
  do { if (!(x)) { \
    D->status = DASM_S_##st|(p-D->actionlist-1); return; } } while (0)
#define CKPL(kind, st) \
  do { if ((size_t)((char *)pl-(char *)D->kind##labels) >= D->kind##size) { \
    D->status = DASM_S_RANGE_##st|(p-D->actionlist-1); return; } } while (0)
#else
#define CK(x, st)	((void)0)
#define CKPL(kind, st)	((void)0)
#endif

/* Pass 1: Store actions and args, link branches/labels, estimate offsets. */
void dasm_put(Dst_DECL, int start, ...)
{
  va_list ap;
  dasm_State *D = Dst_REF;
  dasm_ActList p = D->actionlist + start;
  dasm_Section *sec = D->section;
  int pos = sec->pos, ofs = sec->ofs;
  int *b;

  if (pos >= sec->epos) {
    DASM_M_GROW(Dst, int, sec->buf, sec->bsize,
                sec->bsize + 2 * DASM_MAXSECPOS * sizeof(int));
    sec->rbuf = sec->buf - DASM_POS2BIAS(pos);
    sec->epos =
      (int)sec->bsize / sizeof(int) - DASM_MAXSECPOS + DASM_POS2BIAS(pos);
  }

  b = sec->rbuf;
  b[pos++] = start;

  va_start(ap, start);
  while (1) {
    unsigned short ins = *p++;
    unsigned short action = ins;
    if (action >= DASM__MAX) {
      ofs += 2;
      continue;
    }

    int *pl, n = action >= DASM_REL_PC ? va_arg(ap, int) : 0;
    switch (action) {
    case DASM_STOP:
      goto stop;
    case DASM_SECTION:
      n = *p++ & 255;
      CK(n < D->maxsection, RANGE_SEC);
      D->section = &D->sections[n];
      goto stop;
    case DASM_ESC:
      p++;
      ofs += 2;
      break;
    case DASM_REL_EXT:
      p++;
      ofs += 4;
      break;
    case DASM_ALIGN:
      ofs += *p++;
      b[pos++] = ofs;
      break;
    case DASM_REL_LG:
      if (p[-2] >> 12 == 0xc) { /* RIL instruction needs 32-bit immediate. */
        ofs += 2;
      }
      n = *p++ - 10;
      pl = D->lglabels + n;
      /* Bkwd rel or global. */
      if (n >= 0) {
        CK(n >= 10 || *pl < 0, RANGE_LG);
        CKPL(lg, LG);
        goto putrel;
      }
      pl += 10;
      n = *pl;
      if (n < 0)
        n = 0;                  /* Start new chain for fwd rel if label exists. */
      goto linkrel;
    case DASM_REL_PC:
      if (p[-2] >> 12 == 0xc) { /* RIL instruction needs 32-bit immediate. */
        ofs += 2;
      }
      pl = D->pclabels + n;
      CKPL(pc, PC);
    putrel:
      n = *pl;
      if (n < 0) {              /* Label exists. Get label pos and store it. */
        b[pos] = -n;
      } else {
      linkrel:
        b[pos] = n;             /* Else link to rel chain, anchored at label. */
        *pl = pos;
      }
      ofs += 2;
      pos++;
      break;
    case DASM_LABEL_LG:
      pl = D->lglabels + *p++ - 10;
      CKPL(lg, LG);
      goto putlabel;
    case DASM_LABEL_PC:
      pl = D->pclabels + n;
      CKPL(pc, PC);
    putlabel:
      n = *pl;                  /* n > 0: Collapse rel chain and replace with label pos. */
      while (n > 0) {
        int *pb = DASM_POS2PTR(D, n);
        n = *pb;
        *pb = pos;
      }
      *pl = -pos;               /* Label exists now. */
      b[pos++] = ofs;           /* Store pass1 offset estimate. */
      break;
    case DASM_IMM8:
      b[pos++] = n;
      break;
    case DASM_IMM16:
      CK(((short)n) == n || ((unsigned short)n) == n, RANGE_I);     /* TODO: is this the right way to handle unsigned immediates? */
      ofs += 2;
      b[pos++] = n;
      break;
    case DASM_IMM32:
      ofs += 4;
      b[pos++] = n;
      break;
    case DASM_DISP20:
      CK(-(1 << 19) <= n && n < (1 << 19), RANGE_I);
      b[pos++] = n;
      break;
    case DASM_DISP12:
      CK((n >> 12) == 0, RANGE_I);
      b[pos++] = n;
      break;
    case DASM_LEN8R:
      CK(n >= 1 && n <= 256, RANGE_I);
      b[pos++] = n;
      break;
    case DASM_LEN4HR:
    case DASM_LEN4LR:
      CK(n >= 1 && n <= 128, RANGE_I);
      b[pos++] = n;
      break;
    }
  }
stop:
  va_end(ap);
  sec->pos = pos;
  sec->ofs = ofs;
}

#undef CK

/* Pass 2: Link sections, shrink aligns, fix label offsets. */
int dasm_link(Dst_DECL, size_t * szp)
{
  dasm_State *D = Dst_REF;
  int secnum;
  int ofs = 0;

#ifdef DASM_CHECKS
  *szp = 0;
  if (D->status != DASM_S_OK)
    return D->status;
  {
    int pc;
    for (pc = 0; pc * sizeof(int) < D->pcsize; pc++)
      if (D->pclabels[pc] > 0)
        return DASM_S_UNDEF_PC | pc;
  }
#endif

  {                             /* Handle globals not defined in this translation unit. */
    int idx;
    for (idx = 20; idx * sizeof(int) < D->lgsize; idx++) {
      int n = D->lglabels[idx];
      /* Undefined label: Collapse rel chain and replace with marker (< 0). */
      while (n > 0) {
        int *pb = DASM_POS2PTR(D, n);
        n = *pb;
        *pb = -idx;
      }
    }
  }

  /* Combine all code sections. No support for data sections (yet). */
  for (secnum = 0; secnum < D->maxsection; secnum++) {
    dasm_Section *sec = D->sections + secnum;
    int *b = sec->rbuf;
    int pos = DASM_SEC2POS(secnum);
    int lastpos = sec->pos;

    while (pos != lastpos) {
      dasm_ActList p = D->actionlist + b[pos++];
      while (1) {
        unsigned short ins = *p++;
        unsigned short action = ins;
        switch (action) {
        case DASM_STOP:
        case DASM_SECTION:
          goto stop;
        case DASM_ESC:
          p++;
          break;
        case DASM_REL_EXT:
          p++;
          break;
        case DASM_ALIGN:
          ofs -= (b[pos++] + ofs) & *p++;
          break;
        case DASM_REL_LG:
        case DASM_REL_PC:
          p++;
          pos++;
          break;
        case DASM_LABEL_LG:
        case DASM_LABEL_PC:
          p++;
          b[pos++] += ofs;
          break;
        case DASM_IMM8:
        case DASM_IMM16:
        case DASM_IMM32:
        case DASM_DISP20:
        case DASM_DISP12:
        case DASM_LEN8R:
        case DASM_LEN4HR:
        case DASM_LEN4LR:
          pos++;
          break;
        }
      }
    stop:(void)0;
    }
    ofs += sec->ofs;            /* Next section starts right after current section. */
  }

  D->codesize = ofs;            /* Total size of all code sections */
  *szp = ofs;
  return DASM_S_OK;
}

#ifdef DASM_CHECKS
#define CK(x, st) \
  do { if (!(x)) return DASM_S_##st|(p-D->actionlist-1); } while (0)
#else
#define CK(x, st)	((void)0)
#endif

/* Pass 3: Encode sections. */
int dasm_encode(Dst_DECL, void *buffer)
{
  dasm_State *D = Dst_REF;
  char *base = (char *)buffer;
  unsigned short *cp = (unsigned short *)buffer;
  int secnum;

  /* Encode all code sections. No support for data sections (yet). */
  for (secnum = 0; secnum < D->maxsection; secnum++) {
    dasm_Section *sec = D->sections + secnum;
    int *b = sec->buf;
    int *endb = sec->rbuf + sec->pos;

    while (b != endb) {
      dasm_ActList p = D->actionlist + *b++;
      while (1) {
        unsigned short ins = *p++;
        unsigned short action = ins;
        int n = (action >= DASM_ALIGN && action < DASM__MAX) ? *b++ : 0;
        switch (action) {
        case DASM_STOP:
        case DASM_SECTION:
          goto stop;
        case DASM_ESC:
          *cp++ = *p++;
          break;
        case DASM_REL_EXT:
          n = DASM_EXTERN(Dst, (unsigned char *)cp, *p++, 1) - 4;
          goto patchrel;
        case DASM_ALIGN:
          ins = *p++;
          /* TODO: emit 4-byte noprs instead of 2-byte nops where possible. */
          while ((((char *)cp - base) & ins))
            *cp++ = 0x0700;     /* nop */
          break;
        case DASM_REL_LG:
          CK(n >= 0, UNDEF_LG);
        case DASM_REL_PC:
          CK(n >= 0, UNDEF_PC);
          n = *DASM_POS2PTR(D, n) - (int)((char *)cp - base);
          p++;                  /* skip argument */
        patchrel:
          /* Offsets are halfword aligned (so need to be halved). */
          n += 2;               /* Offset is relative to start of instruction. */
          if (cp[-1] >> 12 == 0xc) {
            *cp++ = n >> 17;
          } else {
            CK(-(1 << 16) <= n && n < (1 << 16) && (n & 1) == 0, RANGE_LG);
          }
          *cp++ = n >> 1;
          break;
        case DASM_LABEL_LG:
          ins = *p++;
          if (ins >= 20)
            D->globals[ins - 20] = (void *)(base + n);
          break;
        case DASM_LABEL_PC:
          break;
        case DASM_IMM8:
          cp[-1] |= n & 0xff;  
          break;
        case DASM_IMM16:
          *cp++ = n;
          break;
        case DASM_IMM32:
          *cp++ = n >> 16;
          *cp++ = n;
          break;
        case DASM_DISP20:
          cp[-2] |= n & 0xfff;
          cp[-1] |= (n >> 4) & 0xff00;
          break;
        case DASM_DISP12:
          cp[-1] |= n & 0xfff;
          break;
        case DASM_LEN8R:
          cp[-1] |= (n - 1) & 0xff;
          break;
        case DASM_LEN4HR:
          cp[-1] |= ((n - 1) << 4) & 0xf0;
          break;
        case DASM_LEN4LR:
          cp[-1] |= (n - 1) & 0x0f;
          break;
        default:
          *cp++ = ins;
          break;
        }
      }
    stop:(void)0;
    }
  }

  if (base + D->codesize != (char *)cp) /* Check for phase errors. */
    return DASM_S_PHASE;
  return DASM_S_OK;
}

#undef CK

/* Get PC label offset. */
int dasm_getpclabel(Dst_DECL, unsigned int pc)
{
  dasm_State *D = Dst_REF;
  if (pc * sizeof(int) < D->pcsize) {
    int pos = D->pclabels[pc];
    if (pos < 0)
      return *DASM_POS2PTR(D, -pos);
    if (pos > 0)
      return -1;                /* Undefined. */
  }
  return -2;                    /* Unused or out of range. */
}

#ifdef DASM_CHECKS
/* Optional sanity checker to call between isolated encoding steps. */
int dasm_checkstep(Dst_DECL, int secmatch)
{
  dasm_State *D = Dst_REF;
  if (D->status == DASM_S_OK) {
    int i;
    for (i = 1; i <= 9; i++) {
      if (D->lglabels[i] > 0) {
        D->status = DASM_S_UNDEF_LG | i;
        break;
      }
      D->lglabels[i] = 0;
    }
  }
  if (D->status == DASM_S_OK && secmatch >= 0 &&
      D->section != &D->sections[secmatch])
    D->status = DASM_S_MATCH_SEC | (D->section - D->sections);
  return D->status;
}
#endif

