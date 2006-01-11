/*
 * cabextract.c
 *
 * Copyright 2000-2002 Stuart Caie
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Principal author: Stuart Caie <kyzer@4u.net>
 *
 * Based on specification documents from Microsoft Corporation
 * Quantum decompression researched and implemented by Matthew Russoto
 * Huffman code adapted from unlzx by Dave Tritscher.
 * InfoZip team's INFLATE implementation adapted to MSZIP by Dirk Stoecker.
 * Major LZX fixes by Jae Jung.
 */
 
#include "config.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"

#include "cabinet.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(cabinet);

THOSE_ZIP_CONSTS;

/****************************************************************
 * QTMupdatemodel (internal)
 */
void QTMupdatemodel(struct QTMmodel *model, int sym) {
  struct QTMmodelsym temp;
  int i, j;

  for (i = 0; i < sym; i++) model->syms[i].cumfreq += 8;

  if (model->syms[0].cumfreq > 3800) {
    if (--model->shiftsleft) {
      for (i = model->entries - 1; i >= 0; i--) {
	/* -1, not -2; the 0 entry saves this */
	model->syms[i].cumfreq >>= 1;
	if (model->syms[i].cumfreq <= model->syms[i+1].cumfreq) {
	  model->syms[i].cumfreq = model->syms[i+1].cumfreq + 1;
	}
      }
    }
    else {
      model->shiftsleft = 50;
      for (i = 0; i < model->entries ; i++) {
	/* no -1, want to include the 0 entry */
	/* this converts cumfreqs into frequencies, then shifts right */
	model->syms[i].cumfreq -= model->syms[i+1].cumfreq;
	model->syms[i].cumfreq++; /* avoid losing things entirely */
	model->syms[i].cumfreq >>= 1;
      }

      /* now sort by frequencies, decreasing order -- this must be an
       * inplace selection sort, or a sort with the same (in)stability
       * characteristics
       */
      for (i = 0; i < model->entries - 1; i++) {
	for (j = i + 1; j < model->entries; j++) {
	  if (model->syms[i].cumfreq < model->syms[j].cumfreq) {
	    temp = model->syms[i];
	    model->syms[i] = model->syms[j];
	    model->syms[j] = temp;
	  }
	}
      }
    
      /* then convert frequencies back to cumfreq */
      for (i = model->entries - 1; i >= 0; i--) {
	model->syms[i].cumfreq += model->syms[i+1].cumfreq;
      }
      /* then update the other part of the table */
      for (i = 0; i < model->entries; i++) {
	model->tabloc[model->syms[i].sym] = i;
      }
    }
  }
}

/*************************************************************************
 * make_decode_table (internal)
 *
 * This function was coded by David Tritscher. It builds a fast huffman
 * decoding table out of just a canonical huffman code lengths table.
 *
 * PARAMS
 *   nsyms:  total number of symbols in this huffman tree.
 *   nbits:  any symbols with a code length of nbits or less can be decoded
 *           in one lookup of the table.
 *   length: A table to get code lengths from [0 to syms-1]
 *   table:  The table to fill up with decoded symbols and pointers.
 *
 * RETURNS
 *   OK:    0
 *   error: 1
 */
int make_decode_table(cab_ULONG nsyms, cab_ULONG nbits, cab_UBYTE *length, cab_UWORD *table) {
  register cab_UWORD sym;
  register cab_ULONG leaf;
  register cab_UBYTE bit_num = 1;
  cab_ULONG fill;
  cab_ULONG pos         = 0; /* the current position in the decode table */
  cab_ULONG table_mask  = 1 << nbits;
  cab_ULONG bit_mask    = table_mask >> 1; /* don't do 0 length codes */
  cab_ULONG next_symbol = bit_mask; /* base of allocation for long codes */

  /* fill entries for codes short enough for a direct mapping */
  while (bit_num <= nbits) {
    for (sym = 0; sym < nsyms; sym++) {
      if (length[sym] == bit_num) {
        leaf = pos;

        if((pos += bit_mask) > table_mask) return 1; /* table overrun */

        /* fill all possible lookups of this symbol with the symbol itself */
        fill = bit_mask;
        while (fill-- > 0) table[leaf++] = sym;
      }
    }
    bit_mask >>= 1;
    bit_num++;
  }

  /* if there are any codes longer than nbits */
  if (pos != table_mask) {
    /* clear the remainder of the table */
    for (sym = pos; sym < table_mask; sym++) table[sym] = 0;

    /* give ourselves room for codes to grow by up to 16 more bits */
    pos <<= 16;
    table_mask <<= 16;
    bit_mask = 1 << 15;

    while (bit_num <= 16) {
      for (sym = 0; sym < nsyms; sym++) {
        if (length[sym] == bit_num) {
          leaf = pos >> 16;
          for (fill = 0; fill < bit_num - nbits; fill++) {
            /* if this path hasn't been taken yet, 'allocate' two entries */
            if (table[leaf] == 0) {
              table[(next_symbol << 1)] = 0;
              table[(next_symbol << 1) + 1] = 0;
              table[leaf] = next_symbol++;
            }
            /* follow the path and select either left or right for next bit */
            leaf = table[leaf] << 1;
            if ((pos >> (15-fill)) & 1) leaf++;
          }
          table[leaf] = sym;

          if ((pos += bit_mask) > table_mask) return 1; /* table overflow */
        }
      }
      bit_mask >>= 1;
      bit_num++;
    }
  }

  /* full table? */
  if (pos == table_mask) return 0;

  /* either erroneous table, or all elements are 0 - let's find out. */
  for (sym = 0; sym < nsyms; sym++) if (length[sym]) return 1;
  return 0;
}

/*************************************************************************
 * checksum (internal)
 */
cab_ULONG checksum(cab_UBYTE *data, cab_UWORD bytes, cab_ULONG csum) {
  int len;
  cab_ULONG ul = 0;

  for (len = bytes >> 2; len--; data += 4) {
    csum ^= ((data[0]) | (data[1]<<8) | (data[2]<<16) | (data[3]<<24));
  }

  switch (bytes & 3) {
  case 3: ul |= *data++ << 16;
  case 2: ul |= *data++ <<  8;
  case 1: ul |= *data;
  }
  csum ^= ul;

  return csum;
}
