/*
 * WideCharToMultiByte implementation
 *
 * Copyright 2000 Alexandre Julliard
 */

#include <string.h>

#include "winnls.h"
#include "wine/unicode.h"

/* wcstombs for single-byte code page */
static inline int wcstombs_sbcs( const struct sbcs_table *table,
                                 const WCHAR *src, unsigned int srclen,
                                 char *dst, unsigned int dstlen )
{
    const unsigned char  * const uni2cp_low = table->uni2cp_low;
    const unsigned short * const uni2cp_high = table->uni2cp_high;
    int ret = srclen;

    if (dstlen < srclen)
    {
        /* buffer too small: fill it up to dstlen and return error */
        srclen = dstlen;
        ret = -1;
    }

    for (;;)
    {
        switch(srclen)
        {
        default:
        case 16: dst[15] = uni2cp_low[uni2cp_high[src[15] >> 8] + (src[15] & 0xff)];
        case 15: dst[14] = uni2cp_low[uni2cp_high[src[14] >> 8] + (src[14] & 0xff)];
        case 14: dst[13] = uni2cp_low[uni2cp_high[src[13] >> 8] + (src[13] & 0xff)];
        case 13: dst[12] = uni2cp_low[uni2cp_high[src[12] >> 8] + (src[12] & 0xff)];
        case 12: dst[11] = uni2cp_low[uni2cp_high[src[11] >> 8] + (src[11] & 0xff)];
        case 11: dst[10] = uni2cp_low[uni2cp_high[src[10] >> 8] + (src[10] & 0xff)];
        case 10: dst[9]  = uni2cp_low[uni2cp_high[src[9]  >> 8] + (src[9]  & 0xff)];
        case 9:  dst[8]  = uni2cp_low[uni2cp_high[src[8]  >> 8] + (src[8]  & 0xff)];
        case 8:  dst[7]  = uni2cp_low[uni2cp_high[src[7]  >> 8] + (src[7]  & 0xff)];
        case 7:  dst[6]  = uni2cp_low[uni2cp_high[src[6]  >> 8] + (src[6]  & 0xff)];
        case 6:  dst[5]  = uni2cp_low[uni2cp_high[src[5]  >> 8] + (src[5]  & 0xff)];
        case 5:  dst[4]  = uni2cp_low[uni2cp_high[src[4]  >> 8] + (src[4]  & 0xff)];
        case 4:  dst[3]  = uni2cp_low[uni2cp_high[src[3]  >> 8] + (src[3]  & 0xff)];
        case 3:  dst[2]  = uni2cp_low[uni2cp_high[src[2]  >> 8] + (src[2]  & 0xff)];
        case 2:  dst[1]  = uni2cp_low[uni2cp_high[src[1]  >> 8] + (src[1]  & 0xff)];
        case 1:  dst[0]  = uni2cp_low[uni2cp_high[src[0]  >> 8] + (src[0]  & 0xff)];
        case 0: break;
        }
        if (srclen < 16) return ret;
        dst += 16;
        src += 16;
        srclen -= 16;
    }
}

/* slow version of wcstombs_sbcs that handles the various flags */
static int wcstombs_sbcs_slow( const struct sbcs_table *table, int flags,
                               const WCHAR *src, unsigned int srclen,
                               char *dst, unsigned int dstlen,
                               const char *defchar, int *used )
{
    const WCHAR * const cp2uni = table->cp2uni;
    const unsigned char  * const uni2cp_low = table->uni2cp_low;
    const unsigned short * const uni2cp_high = table->uni2cp_high;
    const unsigned char table_default = table->info.def_char & 0xff;
    int ret = srclen, tmp;

    if (dstlen < srclen)
    {
        /* buffer too small: fill it up to dstlen and return error */
        srclen = dstlen;
        ret = -1;
    }

    if (!defchar) defchar = &table_default;
    if (!used) used = &tmp;  /* avoid checking on every char */

    while (srclen)
    {
        unsigned char ch = uni2cp_low[uni2cp_high[*src >> 8] + (*src & 0xff)];
        if (((flags & WC_NO_BEST_FIT_CHARS) && (cp2uni[ch] != *src)) ||
            (ch == table_default && *src != table->info.def_unicode_char))
        {
            ch = *defchar;
            *used = 1;
        }
        *dst++ = ch;
        src++;
        srclen--;
    }
    return ret;
}

/* query necessary dst length for src string */
static inline int get_length_dbcs( const struct dbcs_table *table,
                                   const WCHAR *src, unsigned int srclen )
{
    const unsigned short * const uni2cp_low = table->uni2cp_low;
    const unsigned short * const uni2cp_high = table->uni2cp_high;
    int len;

    for (len = 0; srclen; srclen--, src++, len++)
    {
        if (uni2cp_low[uni2cp_high[*src >> 8] + (*src & 0xff)] & 0xff00) len++;
    }
    return len;
}

/* wcstombs for double-byte code page */
static inline int wcstombs_dbcs( const struct dbcs_table *table,
                                 const WCHAR *src, unsigned int srclen,
                                 char *dst, unsigned int dstlen )
{
    const unsigned short * const uni2cp_low = table->uni2cp_low;
    const unsigned short * const uni2cp_high = table->uni2cp_high;
    int len;

    for (len = dstlen; srclen && len; len--, srclen--, src++)
    {
        unsigned short res = uni2cp_low[uni2cp_high[*src >> 8] + (*src & 0xff)];
        if (res & 0xff00)
        {
            if (len == 1) break;  /* do not output a partial char */
            len--;
            *dst++ = res >> 8;
        }
        *dst++ = (char)res;
    }
    if (srclen) return -1;  /* overflow */
    return dstlen - len;
}

/* slow version of wcstombs_dbcs that handles the various flags */
static int wcstombs_dbcs_slow( const struct dbcs_table *table, int flags,
                               const WCHAR *src, unsigned int srclen,
                               char *dst, unsigned int dstlen,
                               const char *defchar, int *used )
{
    const WCHAR * const cp2uni = table->cp2uni;
    const unsigned short * const uni2cp_low = table->uni2cp_low;
    const unsigned short * const uni2cp_high = table->uni2cp_high;
    const unsigned char  * const cp2uni_lb = table->cp2uni_leadbytes;
    WCHAR defchar_value = table->info.def_char;
    int len, tmp;

    if (defchar) defchar_value = defchar[1] ? ((defchar[0] << 8) | defchar[1]) : defchar[0];
    if (!used) used = &tmp;  /* avoid checking on every char */

    for (len = dstlen; srclen && len; len--, srclen--, src++)
    {
        unsigned short res = uni2cp_low[uni2cp_high[*src >> 8] + (*src & 0xff)];

        if (res == table->info.def_char && *src != table->info.def_unicode_char)
        {
            res = defchar_value;
            *used = 1;
        }
        else if (flags & WC_NO_BEST_FIT_CHARS)
        {
            /* check if char maps back to the same Unicode value */
            if (res & 0xff00)
            {
                unsigned char off = cp2uni_lb[res >> 8];
                if (cp2uni[(off << 8) + (res & 0xff)] != *src)
                {
                    res = defchar_value;
                    *used = 1;
                }
            }
            else if (cp2uni[res & 0xff] != *src)
            {
                res = defchar_value;
                *used = 1;
            }
        }

        if (res & 0xff00)
        {
            if (len == 1) break;  /* do not output a partial char */
            len--;
            *dst++ = res >> 8;
        }
        *dst++ = (char)res;
    }
    if (srclen) return -1;  /* overflow */
    return dstlen - len;
}

/* wide char to multi byte string conversion */
/* return -1 on dst buffer overflow */
int cp_wcstombs( const union cptable *table, int flags,
                 const WCHAR *src, int srclen,
                 char *dst, int dstlen, const char *defchar, int *used )
{
    if (table->info.char_size == 1)
    {
        if (!dstlen) return srclen;
        if (flags || defchar || used)
            return wcstombs_sbcs_slow( &table->sbcs, flags, src, srclen,
                                       dst, dstlen, defchar, used );
        return wcstombs_sbcs( &table->sbcs, src, srclen, dst, dstlen );
    }
    else /* mbcs */
    {
        if (!dstlen) return get_length_dbcs( &table->dbcs, src, srclen );
        if (flags || defchar || used)
            return wcstombs_dbcs_slow( &table->dbcs, flags, src, srclen,
                                       dst, dstlen, defchar, used );
        return wcstombs_dbcs( &table->dbcs, src, srclen, dst, dstlen );
    }
}
