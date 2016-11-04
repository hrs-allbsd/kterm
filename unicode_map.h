#include <stdint.h>

#define U_error          (-1)
#define U_latin          (256)
#define U_kanji          (8836)

#define UMAP(n, c)       ((n) << 24 | (c))
#define UMAP_GSET(c)     (((c) >> 24) & 0xff)
#define UMAP_CHAR(c)     ((c) & 0xffff)

extern int32_t ucode_latin[][U_latin];
extern int32_t ucode_kanji1[U_kanji];
extern int32_t ucode_kanji2[U_kanji];

extern int32_t *unicode0_map;
extern int32_t *unicode2_map;

void make_unicode_map();
