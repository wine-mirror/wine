/*
 * Wine internal Unicode definitions
 *
 * Copyright 2000 Alexandre Julliard
 */

#ifndef __WINE_UNICODE_H
#define __WINE_UNICODE_H

/* code page info common to SBCS and DBCS */
struct cp_info
{
    unsigned int          codepage;          /* codepage id */
    unsigned int          char_size;         /* char size (1 or 2 bytes) */
    char                  def_char[2];       /* default char value */
    unsigned short        def_unicode_char;  /* default Unicode char value */
    const char           *name;              /* code page name */
};

struct sbcs_table
{
    struct cp_info        info;
    const unsigned short *cp2uni;            /* code page -> Unicode map */
    const unsigned char  *uni2cp_low;        /* Unicode -> code page map */
    const unsigned short *uni2cp_high;
};

struct dbcs_table
{
    struct cp_info        info;
    const unsigned short *cp2uni;            /* code page -> Unicode map */
    const unsigned char  *cp2uni_leadbytes;
    const unsigned short *uni2cp_low;        /* Unicode -> code page map */
    const unsigned short *uni2cp_high;
    unsigned char         lead_bytes[12];    /* lead bytes ranges */
};

union cptable
{
    struct cp_info    info;
    struct sbcs_table sbcs;
    struct dbcs_table dbcs;
};

extern const union cptable *cp_get_table( unsigned int codepage );
extern const union cptable *cp_enum_table( unsigned int index );

extern int cp_mbstowcs( const union cptable *table, int flags,
                        const char *src, int srclen,
                        unsigned short *dst, int dstlen );
extern int cp_wcstombs( const union cptable *table, int flags,
                        const unsigned short *src, int srclen,
                        char *dst, int dstlen );

static inline int is_dbcs_leadbyte( const union cptable *table, unsigned char ch )
{
    return (table->info.char_size == 2) && (table->dbcs.cp2uni_leadbytes[ch]);
}

#endif  /* __WINE_UNICODE_H */
