/*
 * Utility routines
 *
 * Copyright 1998 Bertho A. Stultiens
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

#include "config.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#include "../tools.h"
#include "wrc.h"
#include "utils.h"
#include "parser.h"

/* #define WANT_NEAR_INDICATION */

#ifdef WANT_NEAR_INDICATION
void make_print(char *str)
{
	while(*str)
	{
		if(!isprint(*str))
			*str = ' ';
		str++;
	}
}
#endif

static void generic_msg(const char *s, const char *t, const char *n, va_list ap)
{
	fprintf(stderr, "%s:%d:%d: %s: ", input_name ? input_name : "stdin", line_number, char_number, t);
	vfprintf(stderr, s, ap);
#ifdef WANT_NEAR_INDICATION
	{
		char *cpy;
		if(n)
		{
			cpy = xstrdup(n);
			make_print(cpy);
			fprintf(stderr, " near '%s'", cpy);
			free(cpy);
		}
	}
#endif
}


int parser_error(const char *s, ...)
{
	va_list ap;
	va_start(ap, s);
	generic_msg(s, "Error", parser_text, ap);
        fputc( '\n', stderr );
	va_end(ap);
	exit(1);
	return 1;
}

int parser_warning(const char *s, ...)
{
	va_list ap;
	va_start(ap, s);
	generic_msg(s, "Warning", parser_text, ap);
	va_end(ap);
	return 0;
}

void fatal_perror( const char *msg, ... )
{
        va_list valist;
        va_start( valist, msg );
	fprintf(stderr, "Error: ");
        vfprintf( stderr, msg, valist );
        perror( " " );
        va_end( valist );
        exit(2);
}

void error(const char *s, ...)
{
	va_list ap;
	va_start(ap, s);
	fprintf(stderr, "Error: ");
	vfprintf(stderr, s, ap);
	va_end(ap);
	exit(2);
}

void warning(const char *s, ...)
{
	va_list ap;
	va_start(ap, s);
	fprintf(stderr, "Warning: ");
	vfprintf(stderr, s, ap);
	va_end(ap);
}

void chat(const char *s, ...)
{
	if(debuglevel & DEBUGLEVEL_CHAT)
	{
		va_list ap;
		va_start(ap, s);
		fprintf(stderr, "FYI: ");
		vfprintf(stderr, s, ap);
		va_end(ap);
	}
}

int compare_striA( const char *str1, const char *str2 )
{
    for (;;)
    {
        /* only the A-Z range is case-insensitive */
        char ch1 = (*str1 >= 'a' && *str1 <= 'z') ? *str1 + 'A' - 'a' : *str1;
        char ch2 = (*str2 >= 'a' && *str2 <= 'z') ? *str2 + 'A' - 'a' : *str2;
        if (!ch1 || ch1 != ch2) return ch1 - ch2;
        str1++;
        str2++;
    }
}

int compare_striW( const WCHAR *str1, const WCHAR *str2 )
{
    for (;;)
    {
        /* only the A-Z range is case-insensitive */
        WCHAR ch1 = (*str1 >= 'a' && *str1 <= 'z') ? *str1 + 'A' - 'a' : *str1;
        WCHAR ch2 = (*str2 >= 'a' && *str2 <= 'z') ? *str2 + 'A' - 'a' : *str2;
        if (!ch1 || ch1 != ch2) return ch1 - ch2;
        str1++;
        str2++;
    }
}

int compare_striAW( const char *str1, const WCHAR *str2 )
{
    for (;;)
    {
        /* only the A-Z range is case-insensitive */
        WCHAR ch1 = (*str1 >= 'a' && *str1 <= 'z') ? *str1 + 'A' - 'a' : (unsigned char)*str1;
        WCHAR ch2 = (*str2 >= 'a' && *str2 <= 'z') ? *str2 + 'A' - 'a' : *str2;
        if (!ch1 || ch1 != ch2) return ch1 - ch2;
        str1++;
        str2++;
    }
}

/*
 *****************************************************************************
 * Function	: compare_name_id
 * Syntax	: int compare_name_id(const name_id_t *n1, const name_id_t *n2)
 * Input	:
 * Output	:
 * Description	:
 * Remarks	:
 *****************************************************************************
*/
int compare_name_id(const name_id_t *n1, const name_id_t *n2)
{
    if (n1->type != n2->type) return n1->type == name_ord ? 1 : -1;
    if (n1->type == name_ord) return n1->name.i_name - n2->name.i_name;

    if (n1->name.s_name->type == str_char)
    {
        if (n2->name.s_name->type == str_char)
            return compare_striA(n1->name.s_name->str.cstr, n2->name.s_name->str.cstr);
        return compare_striAW(n1->name.s_name->str.cstr, n2->name.s_name->str.wstr);
    }
    else
    {
        if (n2->name.s_name->type == str_char)
            return -compare_striAW(n2->name.s_name->str.cstr, n1->name.s_name->str.wstr);
        return compare_striW(n1->name.s_name->str.wstr, n2->name.s_name->str.wstr);
    }
}

#ifdef _WIN32

int is_valid_codepage(int id)
{
    return IsValidCodePage( id );
}

static WCHAR *codepage_to_unicode( int codepage, const char *src, int srclen, int *dstlen )
{
    WCHAR *dst = xmalloc( (srclen + 1) * sizeof(WCHAR) );
    DWORD ret = MultiByteToWideChar( codepage, MB_ERR_INVALID_CHARS, src, srclen, dst, srclen );
    if (!ret) return NULL;
    dst[ret] = 0;
    *dstlen = ret;
    return dst;
}

#else  /* _WIN32 */

struct nls_info
{
    unsigned short  codepage;
    unsigned short  unidef;
    unsigned short  trans_unidef;
    unsigned short *cp2uni;
    unsigned short *dbcs_offsets;
};

static struct nls_info nlsinfo[128];

static void init_nls_info( struct nls_info *info, unsigned short *ptr )
{
    unsigned short hdr_size = ptr[0];

    info->codepage      = ptr[1];
    info->unidef        = ptr[4];
    info->trans_unidef  = ptr[6];
    ptr += hdr_size;
    info->cp2uni = ++ptr;
    ptr += 256;
    if (*ptr++) ptr += 256;  /* glyph table */
    info->dbcs_offsets  = *ptr ? ptr + 1 : NULL;
}

static const struct nls_info *get_nls_info( unsigned int codepage )
{
    unsigned short *data;
    char *path;
    unsigned int i;
    size_t size;

    for (i = 0; i < ARRAY_SIZE(nlsinfo) && nlsinfo[i].codepage; i++)
        if (nlsinfo[i].codepage == codepage) return &nlsinfo[i];

    assert( i < ARRAY_SIZE(nlsinfo) );

    for (i = 0; nlsdirs[i]; i++)
    {
        path = strmake( "%s/c_%03u.nls", nlsdirs[i], codepage );
        if ((data = read_file( path, &size )))
        {
            free( path );
            init_nls_info( &nlsinfo[i], data );
            return &nlsinfo[i];
        }
        free( path );
    }
    return NULL;
}

int is_valid_codepage(int cp)
{
    return cp == CP_UTF8 || get_nls_info( cp );
}

static WCHAR *codepage_to_unicode( int codepage, const char *src, int srclen, int *dstlen )
{
    const struct nls_info *info = get_nls_info( codepage );
    unsigned int i;
    WCHAR dbch, *dst = xmalloc( (srclen + 1) * sizeof(WCHAR) );

    if (!info) error( "codepage %u not supported\n", codepage );

    if (info->dbcs_offsets)
    {
        for (i = 0; srclen; i++, srclen--, src++)
        {
            unsigned short off = info->dbcs_offsets[(unsigned char)*src];
            if (off)
            {
                if (srclen == 1) return NULL;
                dbch = (src[0] << 8) | (unsigned char)src[1];
                src++;
                srclen--;
                dst[i] = info->dbcs_offsets[off + (unsigned char)*src];
                if (dst[i] == info->unidef && dbch != info->trans_unidef) return NULL;
            }
            else
            {
                dst[i] = info->cp2uni[(unsigned char)*src];
                if (dst[i] == info->unidef && *src != info->trans_unidef) return NULL;
            }
        }
    }
    else
    {
        for (i = 0; i < srclen; i++)
        {
            dst[i] = info->cp2uni[(unsigned char)src[i]];
            if (dst[i] == info->unidef && src[i] != info->trans_unidef) return NULL;
        }
    }
    dst[i] = 0;
    *dstlen = i;
    return dst;
}

#endif  /* _WIN32 */

static WCHAR *utf8_to_unicode( const char *src, int srclen, int *dstlen )
{
    static const char utf8_length[128] =
    {
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 0x80-0x8f */
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 0x90-0x9f */
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 0xa0-0xaf */
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 0xb0-0xbf */
        0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1, /* 0xc0-0xcf */
        1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, /* 0xd0-0xdf */
        2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, /* 0xe0-0xef */
        3,3,3,3,3,0,0,0,0,0,0,0,0,0,0,0  /* 0xf0-0xff */
    };
    static const unsigned char utf8_mask[4] = { 0x7f, 0x1f, 0x0f, 0x07 };

    const char *srcend = src + srclen;
    int len, res;
    WCHAR *ret, *dst;

    dst = ret = xmalloc( (srclen + 1) * sizeof(WCHAR) );
    while (src < srcend)
    {
        unsigned char ch = *src++;
        if (ch < 0x80)  /* special fast case for 7-bit ASCII */
        {
            *dst++ = ch;
            continue;
        }
        len = utf8_length[ch - 0x80];
        if (len && src + len <= srcend)
        {
            res = ch & utf8_mask[len];
            switch (len)
            {
            case 3:
                if ((ch = *src ^ 0x80) >= 0x40) break;
                res = (res << 6) | ch;
                src++;
                if (res < 0x10) break;
            case 2:
                if ((ch = *src ^ 0x80) >= 0x40) break;
                res = (res << 6) | ch;
                if (res >= 0x110000 >> 6) break;
                src++;
                if (res < 0x20) break;
                if (res >= 0xd800 >> 6 && res <= 0xdfff >> 6) break;
            case 1:
                if ((ch = *src ^ 0x80) >= 0x40) break;
                res = (res << 6) | ch;
                src++;
                if (res < 0x80) break;
                if (res <= 0xffff) *dst++ = res;
                else
                {
                    res -= 0x10000;
                    *dst++ = 0xd800 | (res >> 10);
                    *dst++ = 0xdc00 | (res & 0x3ff);
                }
                continue;
            }
        }
        *dst++ = 0xfffd;
    }
    *dst = 0;
    *dstlen = dst - ret;
    return ret;
}

static char *unicode_to_utf8( const WCHAR *src, int srclen, int *dstlen )
{
    char *ret, *dst;

    dst = ret = xmalloc( srclen * 3 + 1 );
    for ( ; srclen; srclen--, src++)
    {
        unsigned int ch = *src;

        if (ch < 0x80)  /* 0x00-0x7f: 1 byte */
        {
            *dst++ = ch;
            continue;
        }
        if (ch < 0x800)  /* 0x80-0x7ff: 2 bytes */
        {
            dst[1] = 0x80 | (ch & 0x3f);
            ch >>= 6;
            dst[0] = 0xc0 | ch;
            dst += 2;
            continue;
        }
        if (ch >= 0xd800 && ch <= 0xdbff && srclen > 1 && src[1] >= 0xdc00 && src[1] <= 0xdfff)
        {
            /* 0x10000-0x10ffff: 4 bytes */
            ch = 0x10000 + ((ch & 0x3ff) << 10) + (src[1] & 0x3ff);
            dst[3] = 0x80 | (ch & 0x3f);
            ch >>= 6;
            dst[2] = 0x80 | (ch & 0x3f);
            ch >>= 6;
            dst[1] = 0x80 | (ch & 0x3f);
            ch >>= 6;
            dst[0] = 0xf0 | ch;
            dst += 4;
            src++;
            srclen--;
            continue;
        }
        if (ch >= 0xd800 && ch <= 0xdfff) ch = 0xfffd;  /* invalid surrogate pair */

        /* 0x800-0xffff: 3 bytes */
        dst[2] = 0x80 | (ch & 0x3f);
        ch >>= 6;
        dst[1] = 0x80 | (ch & 0x3f);
        ch >>= 6;
        dst[0] = 0xe0 | ch;
        dst += 3;
    }
    *dst = 0;
    *dstlen = dst - ret;
    return ret;
}

string_t *convert_string_unicode( const string_t *str, int codepage )
{
    string_t *ret = xmalloc(sizeof(*ret));

    ret->type = str_unicode;
    ret->loc = str->loc;

    if (str->type == str_char)
    {
        if (!codepage) parser_error( "Current language is Unicode only, cannot convert string" );

        if (codepage == CP_UTF8)
            ret->str.wstr = utf8_to_unicode( str->str.cstr, str->size, &ret->size );
        else
            ret->str.wstr = codepage_to_unicode( codepage, str->str.cstr, str->size, &ret->size );
        if (!ret->str.wstr) parser_error( "Invalid character in string '%.*s' for codepage %u",
                                          str->size, str->str.cstr, codepage );
    }
    else
    {
        ret->size     = str->size;
        ret->str.wstr = xmalloc(sizeof(WCHAR)*(ret->size+1));
        memcpy( ret->str.wstr, str->str.wstr, ret->size * sizeof(WCHAR) );
        ret->str.wstr[ret->size] = 0;
    }
    return ret;
}

char *convert_string_utf8( const string_t *str, int codepage )
{
    int len;
    string_t *wstr = convert_string_unicode( str, codepage );
    char *ret = unicode_to_utf8( wstr->str.wstr, wstr->size, &len );
    free_string( wstr );
    return ret;
}

void free_string(string_t *str)
{
    if (str->type == str_unicode) free( str->str.wstr );
    else free( str->str.cstr );
    free( str );
}

/* check if the string is valid utf8 despite a different codepage being in use */
int check_valid_utf8( const string_t *str, int codepage )
{
    int i, count;
    WCHAR *wstr;

    if (!check_utf8) return 0;
    if (!codepage) return 0;
    if (codepage == CP_UTF8) return 0;
    if (!is_valid_codepage( codepage )) return 0;

    for (i = count = 0; i < str->size; i++)
    {
        if ((unsigned char)str->str.cstr[i] >= 0xf5) goto done;
        if ((unsigned char)str->str.cstr[i] >= 0xc2) { count++; continue; }
        if ((unsigned char)str->str.cstr[i] >= 0x80) goto done;
    }
    if (!count) return 0;  /* no 8-bit chars at all */

    wstr = utf8_to_unicode( str->str.cstr, str->size, &count );
    for (i = 0; i < count; i++) if (wstr[i] == 0xfffd) break;
    free( wstr );
    return (i == count);

done:
    check_utf8 = 0;  /* at least one 8-bit non-utf8 string found, stop checking */
    return 0;
}


struct lang2cp
{
    unsigned short lang;
    unsigned short sublang;
    unsigned int   cp;
};

/* language to codepage conversion table */
/* specific sublanguages need only be specified if their codepage */
/* differs from the default (SUBLANG_NEUTRAL) */
static const struct lang2cp lang2cps[] =
{
    { LANG_AFRIKAANS,      SUBLANG_NEUTRAL,              1252 },
    { LANG_ALBANIAN,       SUBLANG_NEUTRAL,              1250 },
    { LANG_ALSATIAN,       SUBLANG_NEUTRAL,              1252 },
    { LANG_AMHARIC,        SUBLANG_NEUTRAL,              0    },
    { LANG_ARABIC,         SUBLANG_NEUTRAL,              1256 },
    { LANG_ARMENIAN,       SUBLANG_NEUTRAL,              0    },
    { LANG_ASSAMESE,       SUBLANG_NEUTRAL,              0    },
    { LANG_ASTURIAN,       SUBLANG_NEUTRAL,              1252 },
    { LANG_AZERI,          SUBLANG_NEUTRAL,              1254 },
    { LANG_AZERI,          SUBLANG_AZERI_CYRILLIC,       1251 },
    { LANG_BASHKIR,        SUBLANG_NEUTRAL,              1251 },
    { LANG_BASQUE,         SUBLANG_NEUTRAL,              1252 },
    { LANG_BELARUSIAN,     SUBLANG_NEUTRAL,              1251 },
    { LANG_BENGALI,        SUBLANG_NEUTRAL,              0    },
    { LANG_BOSNIAN,        SUBLANG_NEUTRAL,              1250 },
    { LANG_BOSNIAN,        SUBLANG_BOSNIAN_BOSNIA_HERZEGOVINA_CYRILLIC, 1251 },
    { LANG_BRETON,         SUBLANG_NEUTRAL,              1252 },
    { LANG_BULGARIAN,      SUBLANG_NEUTRAL,              1251 },
    { LANG_CATALAN,        SUBLANG_NEUTRAL,              1252 },
    { LANG_CHINESE,        SUBLANG_NEUTRAL,              950  },
    { LANG_CHINESE,        SUBLANG_CHINESE_SIMPLIFIED,   936  },
    { LANG_CHINESE,        SUBLANG_CHINESE_SINGAPORE,    936  },
#ifdef LANG_CORNISH
    { LANG_CORNISH,        SUBLANG_NEUTRAL,              1252 },
#endif /* LANG_CORNISH */
    { LANG_CORSICAN,       SUBLANG_NEUTRAL,              1252 },
    { LANG_CROATIAN,       SUBLANG_NEUTRAL,              1250 },
    { LANG_CZECH,          SUBLANG_NEUTRAL,              1250 },
    { LANG_DANISH,         SUBLANG_NEUTRAL,              1252 },
    { LANG_DARI,           SUBLANG_NEUTRAL,              1256 },
    { LANG_DIVEHI,         SUBLANG_NEUTRAL,              0    },
    { LANG_DUTCH,          SUBLANG_NEUTRAL,              1252 },
    { LANG_ENGLISH,        SUBLANG_NEUTRAL,              1252 },
#ifdef LANG_ESPERANTO
    { LANG_ESPERANTO,      SUBLANG_NEUTRAL,              1252 },
#endif /* LANG_ESPERANTO */
    { LANG_ESTONIAN,       SUBLANG_NEUTRAL,              1257 },
    { LANG_FAEROESE,       SUBLANG_NEUTRAL,              1252 },
    { LANG_FILIPINO,       SUBLANG_NEUTRAL,              1252 },
    { LANG_FINNISH,        SUBLANG_NEUTRAL,              1252 },
    { LANG_FRENCH,         SUBLANG_NEUTRAL,              1252 },
    { LANG_FRISIAN,        SUBLANG_NEUTRAL,              1252 },
#ifdef LANG_MANX_GAELIC
    { LANG_MANX_GAELIC,    SUBLANG_NEUTRAL,              1252 },
#endif /* LANG_MANX_GAELIC */
    { LANG_GALICIAN,       SUBLANG_NEUTRAL,              1252 },
    { LANG_GEORGIAN,       SUBLANG_NEUTRAL,              0    },
    { LANG_GERMAN,         SUBLANG_NEUTRAL,              1252 },
    { LANG_GREEK,          SUBLANG_NEUTRAL,              1253 },
    { LANG_GREENLANDIC,    SUBLANG_NEUTRAL,              1252 },
    { LANG_GUJARATI,       SUBLANG_NEUTRAL,              0    },
    { LANG_HAUSA,          SUBLANG_NEUTRAL,              1252 },
    { LANG_HEBREW,         SUBLANG_NEUTRAL,              1255 },
    { LANG_HINDI,          SUBLANG_NEUTRAL,              0    },
    { LANG_HUNGARIAN,      SUBLANG_NEUTRAL,              1250 },
    { LANG_ICELANDIC,      SUBLANG_NEUTRAL,              1252 },
    { LANG_IGBO,           SUBLANG_NEUTRAL,              1252 },
    { LANG_INDONESIAN,     SUBLANG_NEUTRAL,              1252 },
    { LANG_INUKTITUT,      SUBLANG_NEUTRAL,              0    },
    { LANG_INUKTITUT,      SUBLANG_INUKTITUT_CANADA_LATIN, 0  },
    { LANG_INVARIANT,      SUBLANG_NEUTRAL,              0    },
    { LANG_IRISH,          SUBLANG_NEUTRAL,              1252 },
    { LANG_ITALIAN,        SUBLANG_NEUTRAL,              1252 },
    { LANG_JAPANESE,       SUBLANG_NEUTRAL,              932  },
    { LANG_KANNADA,        SUBLANG_NEUTRAL,              0    },
    { LANG_KAZAK,          SUBLANG_NEUTRAL,              1251 },
    { LANG_KHMER,          SUBLANG_NEUTRAL,              0    },
    { LANG_KICHE,          SUBLANG_NEUTRAL,              1252 },
    { LANG_KINYARWANDA,    SUBLANG_NEUTRAL,              1252 },
    { LANG_KONKANI,        SUBLANG_NEUTRAL,              0    },
    { LANG_KOREAN,         SUBLANG_NEUTRAL,              949  },
    { LANG_KYRGYZ,         SUBLANG_NEUTRAL,              1251 },
    { LANG_LAO,            SUBLANG_NEUTRAL,              0    },
    { LANG_LATVIAN,        SUBLANG_NEUTRAL,              1257 },
    { LANG_LITHUANIAN,     SUBLANG_NEUTRAL,              1257 },
    { LANG_LOWER_SORBIAN,  SUBLANG_NEUTRAL,              1252 },
    { LANG_LUXEMBOURGISH,  SUBLANG_NEUTRAL,              1252 },
    { LANG_MACEDONIAN,     SUBLANG_NEUTRAL,              1251 },
    { LANG_MALAY,          SUBLANG_NEUTRAL,              1252 },
    { LANG_MALAYALAM,      SUBLANG_NEUTRAL,              0    },
    { LANG_MALTESE,        SUBLANG_NEUTRAL,              0    },
    { LANG_MAORI,          SUBLANG_NEUTRAL,              0    },
    { LANG_MAPUDUNGUN,     SUBLANG_NEUTRAL,              1252 },
    { LANG_MARATHI,        SUBLANG_NEUTRAL,              0    },
    { LANG_MOHAWK,         SUBLANG_NEUTRAL,              1252 },
    { LANG_MONGOLIAN,      SUBLANG_NEUTRAL,              1251 },
    { LANG_NEPALI,         SUBLANG_NEUTRAL,              0    },
    { LANG_NEUTRAL,        SUBLANG_NEUTRAL,              1252 },
    { LANG_NORWEGIAN,      SUBLANG_NEUTRAL,              1252 },
    { LANG_OCCITAN,        SUBLANG_NEUTRAL,              1252 },
    { LANG_ORIYA,          SUBLANG_NEUTRAL,              0    },
    { LANG_PASHTO,         SUBLANG_NEUTRAL,              0    },
    { LANG_PERSIAN,        SUBLANG_NEUTRAL,              1256 },
    { LANG_POLISH,         SUBLANG_NEUTRAL,              1250 },
    { LANG_PORTUGUESE,     SUBLANG_NEUTRAL,              1252 },
    { LANG_PUNJABI,        SUBLANG_NEUTRAL,              0    },
    { LANG_QUECHUA,        SUBLANG_NEUTRAL,              1252 },
    { LANG_ROMANIAN,       SUBLANG_NEUTRAL,              1250 },
    { LANG_ROMANSH,        SUBLANG_NEUTRAL,              1252 },
    { LANG_RUSSIAN,        SUBLANG_NEUTRAL,              1251 },
    { LANG_SAMI,           SUBLANG_NEUTRAL,              1252 },
    { LANG_SANSKRIT,       SUBLANG_NEUTRAL,              0    },
    { LANG_SCOTTISH_GAELIC,SUBLANG_NEUTRAL,              1252 },
    { LANG_SERBIAN,        SUBLANG_NEUTRAL,              1250 },
    { LANG_SERBIAN,        SUBLANG_SERBIAN_CYRILLIC,     1251 },
    { LANG_SINHALESE,      SUBLANG_NEUTRAL,              0    },
    { LANG_SLOVAK,         SUBLANG_NEUTRAL,              1250 },
    { LANG_SLOVENIAN,      SUBLANG_NEUTRAL,              1250 },
    { LANG_SOTHO,          SUBLANG_NEUTRAL,              1252 },
    { LANG_SPANISH,        SUBLANG_NEUTRAL,              1252 },
    { LANG_SWAHILI,        SUBLANG_NEUTRAL,              1252 },
    { LANG_SWEDISH,        SUBLANG_NEUTRAL,              1252 },
    { LANG_SYRIAC,         SUBLANG_NEUTRAL,              0    },
    { LANG_TAJIK,          SUBLANG_NEUTRAL,              1251 },
    { LANG_TAMAZIGHT,      SUBLANG_NEUTRAL,              1252 },
    { LANG_TAMIL,          SUBLANG_NEUTRAL,              0    },
    { LANG_TATAR,          SUBLANG_NEUTRAL,              1251 },
    { LANG_TELUGU,         SUBLANG_NEUTRAL,              0    },
    { LANG_THAI,           SUBLANG_NEUTRAL,              874  },
    { LANG_TIBETAN,        SUBLANG_NEUTRAL,              0    },
    { LANG_TSWANA,         SUBLANG_NEUTRAL,              1252 },
    { LANG_TURKISH,        SUBLANG_NEUTRAL,              1254 },
    { LANG_TURKMEN,        SUBLANG_NEUTRAL,              1250 },
    { LANG_UIGHUR,         SUBLANG_NEUTRAL,              1256 },
    { LANG_UKRAINIAN,      SUBLANG_NEUTRAL,              1251 },
    { LANG_UPPER_SORBIAN,  SUBLANG_NEUTRAL,              1252 },
    { LANG_URDU,           SUBLANG_NEUTRAL,              1256 },
    { LANG_UZBEK,          SUBLANG_NEUTRAL,              1254 },
    { LANG_UZBEK,          SUBLANG_UZBEK_CYRILLIC,       1251 },
    { LANG_VIETNAMESE,     SUBLANG_NEUTRAL,              1258 },
#ifdef LANG_WALON
    { LANG_WALON,          SUBLANG_NEUTRAL,              1252 },
#endif /* LANG_WALON */
    { LANG_WELSH,          SUBLANG_NEUTRAL,              1252 },
    { LANG_WOLOF,          SUBLANG_NEUTRAL,              1252 },
    { LANG_XHOSA,          SUBLANG_NEUTRAL,              1252 },
    { LANG_YAKUT,          SUBLANG_NEUTRAL,              1251 },
    { LANG_YI,             SUBLANG_NEUTRAL,              0    },
    { LANG_YORUBA,         SUBLANG_NEUTRAL,              1252 },
    { LANG_ZULU,           SUBLANG_NEUTRAL,              1252 }
};

int get_language_codepage( unsigned short lang, unsigned short sublang )
{
    unsigned int i;
    int cp = -1, defcp = -1;

    for (i = 0; i < ARRAY_SIZE(lang2cps); i++)
    {
        if (lang2cps[i].lang != lang) continue;
        if (lang2cps[i].sublang == sublang)
        {
            cp = lang2cps[i].cp;
            break;
        }
        if (lang2cps[i].sublang == SUBLANG_NEUTRAL) defcp = lang2cps[i].cp;
    }

    if (cp == -1) cp = defcp;
    return cp;
}
