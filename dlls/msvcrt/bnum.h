/*
 * Copyright 2020 Piotr Caban for CodeWeavers
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef __WINE_BNUM_H
#define __WINE_BNUM_H

#define EXP_BITS 11
#define MANT_BITS 53

static const int p10s[] = { 1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000 };

#define LIMB_DIGITS 9           /* each DWORD stores up to 9 digits */
#define LIMB_MAX 1000000000     /* 10^9 */
#define BNUM_IDX(i) ((i) & 127)
/* bnum represents real number with fixed decimal point (after 2 limbs) */
struct bnum {
    DWORD data[128]; /* circular buffer, base 10 number */
    int b; /* least significant digit position */
    int e; /* most significant digit position + 1 */
};

/* Returns integral part of bnum */
static inline ULONGLONG bnum_to_mant(struct bnum *b)
{
    ULONGLONG ret = (ULONGLONG)b->data[BNUM_IDX(b->e-1)] * LIMB_MAX;
    if(b->b != b->e-1) ret += b->data[BNUM_IDX(b->e-2)];
    return ret;
}

/* Returns TRUE if new most significant limb was added */
static inline BOOL bnum_lshift(struct bnum *b, int shift)
{
    DWORD rest = 0;
    ULONGLONG tmp;
    int i;

    /* The limbs number can change by up to 1 so shift <= 29 */
    assert(shift <= 29);

    for(i=b->b; i<b->e; i++) {
        tmp = ((ULONGLONG)b->data[BNUM_IDX(i)] << shift) + rest;
        rest = tmp / LIMB_MAX;
        b->data[BNUM_IDX(i)] = tmp % LIMB_MAX;

        if(i == b->b && !b->data[BNUM_IDX(i)])
            b->b++;
    }

    if(rest) {
        b->data[BNUM_IDX(b->e)] = rest;
        b->e++;

        if(BNUM_IDX(b->b) == BNUM_IDX(b->e)) {
            if(b->data[BNUM_IDX(b->b)]) b->data[BNUM_IDX(b->b+1)] |= 1;
            b->b++;
        }
        return TRUE;
    }
    return FALSE;
}

/* Returns TRUE if most significant limb was removed */
static inline BOOL bnum_rshift(struct bnum *b, int shift)
{
    DWORD tmp, rest = 0;
    BOOL ret = FALSE;
    int i;

    /* Compute LIMB_MAX << shift without accuracy loss */
    assert(shift <= 9);

    for(i=b->e-1; i>=b->b; i--) {
        tmp = b->data[BNUM_IDX(i)] & ((1<<shift)-1);
        b->data[BNUM_IDX(i)] = (b->data[BNUM_IDX(i)] >> shift) + rest;
        rest = (LIMB_MAX >> shift) * tmp;
        if(i==b->e-1 && !b->data[BNUM_IDX(i)]) {
            b->e--;
            ret = TRUE;
        }
    }

    if(rest) {
        if(BNUM_IDX(b->b-1) == BNUM_IDX(b->e)) {
            if(rest) b->data[BNUM_IDX(b->b)] |= 1;
        } else {
            b->b--;
            b->data[BNUM_IDX(b->b)] = rest;
        }
    }
    return ret;
}

static inline void bnum_mult(struct bnum *b, int mult)
{
    DWORD rest = 0;
    ULONGLONG tmp;
    int i;

    assert(mult <= LIMB_MAX);

    for(i=b->b; i<b->e; i++) {
        tmp = ((ULONGLONG)b->data[BNUM_IDX(i)] * mult) + rest;
        rest = tmp / LIMB_MAX;
        b->data[BNUM_IDX(i)] = tmp % LIMB_MAX;

        if(i == b->b && !b->data[BNUM_IDX(i)])
            b->b++;
    }

    if(rest) {
        b->data[BNUM_IDX(b->e)] = rest;
        b->e++;

        if(BNUM_IDX(b->b) == BNUM_IDX(b->e)) {
            if(b->data[BNUM_IDX(b->b)]) b->data[BNUM_IDX(b->b+1)] |= 1;
            b->b++;
        }
    }
}

#endif /* __WINE_BNUM_H */
