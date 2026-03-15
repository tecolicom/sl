/*
 * art/qblock.h - Quarter-block character encoding utilities
 *
 * Provides encoding/decoding between pixel bitmaps and Unicode
 * quarter-block characters (U+2580-259F), plus half-column shifting
 * for smooth sub-column animation movement.
 */

#ifndef SL_QBLOCK_H
#define SL_QBLOCK_H

#include <string.h>
#include <stdlib.h>

/*
 * Quarter-block characters indexed by 4-bit pattern: UL UR LL LR
 *
 *   0=" " 1=▗ 2=▖ 3=▄ 4=▝ 5=▐ 6=▞ 7=▟
 *   8=▘  9=▚ A=▌ B=▙ C=▀ D=▜ E=▛ F=█
 */
static const char *qblock[16] = {
    " ", "▗", "▖", "▄", "▝", "▐", "▞", "▟",
    "▘", "▚", "▌", "▙", "▀", "▜", "▛", "█",
};

/* Decode a UTF-8 quarter-block character to its 4-bit pattern.
   Advances *p past the character. */
static int qb_decode(const char **p) {
    if (**p == ' ') { (*p)++; return 0; }
    for (int i = 1; i < 16; i++) {
        int len = strlen(qblock[i]);
        if (strncmp(*p, qblock[i], len) == 0) {
            *p += len;
            return i;
        }
    }
    (*p)++;
    return 0;
}

/* Encode a pair of pixel rows into a quarter-block string.
   Pixel rows use '@' for filled, ' ' for empty.
   Normal: sample (0,1)(2,3)... → width w/2.
   Shifted: sample (-1,0)(1,2)... → width w/2+1. */
static char *qb_encode_row(const char *upper, const char *lower, int w, int shifted) {
    char buf[256];
    int pos = 0;
    for (int c = shifted ? -1 : 0; c < w; c += 2) {
        int ul = (c >= 0 && upper[c]   != ' ') ? 8 : 0;
        int ur = (c+1 < w && upper[c+1] != ' ') ? 4 : 0;
        int ll = (c >= 0 && lower[c]   != ' ') ? 2 : 0;
        int lr = (c+1 < w && lower[c+1] != ' ') ? 1 : 0;
        const char *ch = qblock[ul | ur | ll | lr];
        int len = strlen(ch);
        memcpy(buf + pos, ch, len);
        pos += len;
    }
    buf[pos] = '\0';
    return strdup(buf);
}

/* Buffer-based encoding (no malloc).
   Same as qb_encode_row but writes to caller-provided buffer.
   Returns number of bytes written (excluding NUL). */
static int qb_encode_row_buf(const char *upper, const char *lower,
                              int w, int shifted, char *out, int outsize) {
    int pos = 0;
    for (int c = shifted ? -1 : 0; c < w; c += 2) {
        int ul = (c >= 0 && upper[c]   != ' ') ? 8 : 0;
        int ur = (c+1 < w && upper[c+1] != ' ') ? 4 : 0;
        int ll = (c >= 0 && lower[c]   != ' ') ? 2 : 0;
        int lr = (c+1 < w && lower[c+1] != ' ') ? 1 : 0;
        const char *ch = qblock[ul | ur | ll | lr];
        int len = strlen(ch);
        if (pos + len >= outsize) break;
        memcpy(out + pos, ch, len);
        pos += len;
    }
    out[pos] = '\0';
    return pos;
}

/* Generate half-column-shifted version of a quarter-block string.
   Decodes to pixel rows, re-encodes shifted right by 1 pixel column. */
static char *qblock_shift(const char *src) {
    if (!*src) return strdup(src);
    char upper[256], lower[256];
    int w = 0;
    const char *p = src;
    while (*p) {
        int bits = qb_decode(&p);
        upper[w]   = (bits & 8) ? '@' : ' ';
        upper[w+1] = (bits & 4) ? '@' : ' ';
        lower[w]   = (bits & 2) ? '@' : ' ';
        lower[w+1] = (bits & 1) ? '@' : ' ';
        w += 2;
    }
    upper[w] = lower[w] = '\0';
    return qb_encode_row(upper, lower, w, 1);
}

#endif
