/*
 * Unicode sort key generation
 *
 * Copyright 2003 Dmitry Timoshkov
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
 */
#include "wine/unicode.h"

extern int get_decomposition(WCHAR src, WCHAR *dst, unsigned int dstlen);

/*
 * flags - normalization NORM_* flags
 *
 * FIXME: 'variable' flag not handled
 */
int wine_get_sortkey(int flags, const WCHAR *src, int srclen, char *dst, int dstlen)
{
    extern const unsigned int collation_table[];
    WCHAR dummy[4]; /* no decomposition is larger than 4 chars */
    int key_len[4];
    char *key_ptr[4];
    const WCHAR *src_save = src;
    int srclen_save = srclen;

    key_len[0] = key_len[1] = key_len[2] = key_len[3] = 0;
    for (; srclen; srclen--, src++)
    {
        int decomposed_len = get_decomposition(*src, dummy, 4);
        if (decomposed_len)
        {
            int i;
            for (i = 0; i < decomposed_len; i++)
            {
                WCHAR wch = dummy[i];
                unsigned int ce;

                /* tests show that win2k just ignores NORM_IGNORENONSPACE,
                 * and skips white space and punctuation characters for
                 * NORM_IGNORESYMBOLS.
                 */
                if ((flags & NORM_IGNORESYMBOLS) && (get_char_typeW(wch) & (C1_PUNCT | C1_SPACE)))
                    continue;

                if (flags & NORM_IGNORECASE) wch = tolowerW(wch);

                ce = collation_table[collation_table[wch >> 8] + (wch & 0xff)];
                if (ce != (unsigned int)-1)
                {
                    if (ce >> 16) key_len[0] += 2;
                    if ((ce >> 8) & 0xff) key_len[1]++;
                    if ((ce >> 4) & 0x0f) key_len[2]++;
                    /*if (ce & 1)
                    {
                        if (wch >> 8) key_len[3]++;
                        key_len[3]++;
                    }*/
                }
                /*else
                {
                    key_len[0] += 2;
                    if (wch >> 8) key_len[0]++;
                    if (wch & 0xff) key_len[0]++;
		}*/
            }
        }
    }

    if (!dstlen) /* compute length */
        /* 4 * '\1' + 1 * '\0' + key length */
        return key_len[0] + key_len[1] + key_len[2] + key_len[3] + 4 + 1;

    if (dstlen < key_len[0] + key_len[1] + key_len[2] + key_len[3] + 4 + 1)
        return 0; /* overflow */

    src = src_save;
    srclen = srclen_save;

    key_ptr[0] = dst;
    key_ptr[1] = key_ptr[0] + key_len[0] + 1;
    key_ptr[2] = key_ptr[1] + key_len[1] + 1;
    key_ptr[3] = key_ptr[2] + key_len[2] + 1;

    for (; srclen; srclen--, src++)
    {
        int decomposed_len = get_decomposition(*src, dummy, 4);
        if (decomposed_len)
        {
            int i;
            for (i = 0; i < decomposed_len; i++)
            {
                WCHAR wch = dummy[i];
                unsigned int ce;

                /* tests show that win2k just ignores NORM_IGNORENONSPACE,
                 * and skips white space and punctuation characters for
                 * NORM_IGNORESYMBOLS.
                 */
                if ((flags & NORM_IGNORESYMBOLS) && (get_char_typeW(wch) & (C1_PUNCT | C1_SPACE)))
                    continue;

                if (flags & NORM_IGNORECASE) wch = tolowerW(wch);

                ce = collation_table[collation_table[wch >> 8] + (wch & 0xff)];
                if (ce != (unsigned int)-1)
                {
                    WCHAR key;
                    if ((key = ce >> 16))
                    {
                        *key_ptr[0]++ = key >> 8;
                        *key_ptr[0]++ = key & 0xff;
                    }
                    /* make key 1 start from 2 */
                    if ((key = (ce >> 8) & 0xff)) *key_ptr[1]++ = key + 1;
                    /* make key 2 start from 2 */
                    if ((key = (ce >> 4) & 0x0f)) *key_ptr[2]++ = key + 1;
                    /* key 3 is always a character code */
                    /*if (ce & 1)
                    {
                        if (wch >> 8) *key_ptr[3]++ = wch >> 8;
                        if (wch & 0xff) *key_ptr[3]++ = wch & 0xff;
                    }*/
                }
                /*else
                {
                    *key_ptr[0]++ = 0xff;
                    *key_ptr[0]++ = 0xfe;
                    if (wch >> 8) *key_ptr[0]++ = wch >> 8;
                    if (wch & 0xff) *key_ptr[0]++ = wch & 0xff;
                }*/
            }
        }
    }

    *key_ptr[0] = '\1';
    *key_ptr[1] = '\1';
    *key_ptr[2] = '\1';
    *key_ptr[3]++ = '\1';
    *key_ptr[3] = 0;

    return key_ptr[3] - dst;
}
