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
                                 const unsigned short *src, unsigned int srclen,
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

/* query necessary dst length for src string */
static inline int get_length_dbcs( const struct dbcs_table *table,
                                   const unsigned short *src, unsigned int srclen )
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
                                 const unsigned short *src, unsigned int srclen,
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

/* wide char to multi byte string conversion */
/* return -1 on dst buffer overflow */
int cp_wcstombs( const union cptable *table, int flags,
                 const unsigned short *src, int srclen,
                 char *dst, int dstlen )
{
    if (table->info.char_size == 1)
    {
        if (!dstlen) return srclen;
        return wcstombs_sbcs( &table->sbcs, src, srclen, dst, dstlen );
    }
    else /* mbcs */
    {
        if (!dstlen) return get_length_dbcs( &table->dbcs, src, srclen );
        return wcstombs_sbcs( &table->sbcs, src, srclen, dst, dstlen );
    }
}
