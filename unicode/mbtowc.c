/*
 * MultiByteToWideChar implementation
 *
 * Copyright 2000 Alexandre Julliard
 */

#include <string.h>

#include "winnls.h"
#include "wine/unicode.h"

/* check src string for invalid chars; return non-zero if invalid char found */
static inline int check_invalid_chars_sbcs( const struct sbcs_table *table,
                                            const unsigned char *src, unsigned int srclen )
{
    const WCHAR * const cp2uni = table->cp2uni;
    while (srclen)
    {
        if (cp2uni[*src] == table->info.def_unicode_char && *src != table->info.def_char)
            break;
        src++;
        srclen--;
    }
    return srclen;
}

/* mbstowcs for single-byte code page */
/* all lengths are in characters, not bytes */
static inline int mbstowcs_sbcs( const struct sbcs_table *table,
                                 const unsigned char *src, unsigned int srclen,
                                 WCHAR *dst, unsigned int dstlen )
{
    const WCHAR * const cp2uni = table->cp2uni;
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
        case 16: dst[15] = cp2uni[src[15]];
        case 15: dst[14] = cp2uni[src[14]];
        case 14: dst[13] = cp2uni[src[13]];
        case 13: dst[12] = cp2uni[src[12]];
        case 12: dst[11] = cp2uni[src[11]];
        case 11: dst[10] = cp2uni[src[10]];
        case 10: dst[9]  = cp2uni[src[9]];
        case 9:  dst[8]  = cp2uni[src[8]];
        case 8:  dst[7]  = cp2uni[src[7]];
        case 7:  dst[6]  = cp2uni[src[6]];
        case 6:  dst[5]  = cp2uni[src[5]];
        case 5:  dst[4]  = cp2uni[src[4]];
        case 4:  dst[3]  = cp2uni[src[3]];
        case 3:  dst[2]  = cp2uni[src[2]];
        case 2:  dst[1]  = cp2uni[src[1]];
        case 1:  dst[0]  = cp2uni[src[0]];
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
                                   const unsigned char *src, unsigned int srclen )
{
    const unsigned char * const cp2uni_lb = table->cp2uni_leadbytes;
    int len;

    for (len = 0; srclen; srclen--, src++, len++)
    {
        if (cp2uni_lb[*src])
        {
            if (!--srclen) break;  /* partial char, ignore it */
            src++;
        }
    }
    return len;
}

/* check src string for invalid chars; return non-zero if invalid char found */
static inline int check_invalid_chars_dbcs( const struct dbcs_table *table,
                                            const unsigned char *src, unsigned int srclen )
{
    const WCHAR * const cp2uni = table->cp2uni;
    const unsigned char * const cp2uni_lb = table->cp2uni_leadbytes;

    while (srclen)
    {
        unsigned char off = cp2uni_lb[*src];
        if (off)  /* multi-byte char */
        {
            if (srclen == 1) break;  /* partial char, error */
            if (cp2uni[(off << 8) + src[1]] == table->info.def_unicode_char &&
                ((src[0] << 8) | src[1]) != table->info.def_char) break;
            src++;
            srclen--;
        }
        else if (cp2uni[*src] == table->info.def_unicode_char &&
                 *src != table->info.def_char) break;
        src++;
        srclen--;
    }
    return srclen;
}

/* mbstowcs for double-byte code page */
/* all lengths are in characters, not bytes */
static inline int mbstowcs_dbcs( const struct dbcs_table *table,
                                 const unsigned char *src, unsigned int srclen,
                                 WCHAR *dst, unsigned int dstlen )
{
    const WCHAR * const cp2uni = table->cp2uni;
    const unsigned char * const cp2uni_lb = table->cp2uni_leadbytes;
    int len;

    for (len = dstlen; srclen && len; len--, srclen--, src++, dst++)
    {
        unsigned char off = cp2uni_lb[*src];
        if (off)
        {
            if (!--srclen) break;  /* partial char, ignore it */
            src++;
            *dst = cp2uni[(off << 8) + *src];
        }
        else *dst = cp2uni[*src];
    }
    if (srclen) return -1;  /* overflow */
    return dstlen - len;
}


/* return -1 on dst buffer overflow, -2 on invalid input char */
int cp_mbstowcs( const union cptable *table, int flags,
                 const char *src, int srclen,
                 WCHAR *dst, int dstlen )
{
    if (table->info.char_size == 1)
    {
        if (flags & MB_ERR_INVALID_CHARS)
        {
            if (check_invalid_chars_sbcs( &table->sbcs, src, srclen )) return -2;
        }
        if (!dstlen) return srclen;
        return mbstowcs_sbcs( &table->sbcs, src, srclen, dst, dstlen );
    }
    else /* mbcs */
    {
        if (flags & MB_ERR_INVALID_CHARS)
        {
            if (check_invalid_chars_dbcs( &table->dbcs, src, srclen )) return -2;
        }
        if (!dstlen) return get_length_dbcs( &table->dbcs, src, srclen );
        return mbstowcs_dbcs( &table->dbcs, src, srclen, dst, dstlen );
    }
}
