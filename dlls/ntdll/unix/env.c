/*
 * Ntdll environment functions
 *
 * Copyright 1996, 1998 Alexandre Julliard
 * Copyright 2003       Eric Pouech
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

#if 0
#pragma makedep unix
#endif

#include "config.h"

#include <assert.h>
#include <errno.h>
#include <locale.h>
#include <langinfo.h>
#include <fcntl.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#ifdef HAVE_SYS_STAT_H
# include <sys/stat.h>
#endif
#ifdef HAVE_SYS_PRCTL_H
# include <sys/prctl.h>
#endif
#ifdef HAVE_PWD_H
# include <pwd.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifdef __APPLE__
# include <CoreFoundation/CFLocale.h>
# include <CoreFoundation/CFString.h>
#endif

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"
#include "winbase.h"
#include "winnls.h"
#include "wine/condrv.h"
#include "wine/debug.h"
#include "unix_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(environ);

USHORT *uctable = NULL, *lctable = NULL;
SIZE_T startup_info_size = 0;

int main_argc = 0;
char **main_argv = NULL;
char **main_envp = NULL;
WCHAR **main_wargv = NULL;

static LCID user_lcid, system_lcid;
static LANGID user_ui_language, system_ui_language;

static char system_locale[LOCALE_NAME_MAX_LENGTH];
static char user_locale[LOCALE_NAME_MAX_LENGTH];

static struct
{
    USHORT *data;
    USHORT *dbcs;
    USHORT *mbtable;
    void   *wctable;
} unix_cp;

static void *read_nls_file( const char *name )
{
    const char *dir = build_dir ? build_dir : data_dir;
    struct stat st;
    char *path;
    void *data, *ret = NULL;
    int fd;

    if (!(path = malloc( strlen(dir) + 22 ))) return NULL;
    sprintf( path, "%s/nls/%s.nls", dir, name );
    if ((fd = open( path, O_RDONLY )) != -1)
    {
        fstat( fd, &st );
        if ((data = malloc( st.st_size )) && st.st_size > 0x1000 &&
            read( fd, data, st.st_size ) == st.st_size)
        {
            ret = data;
        }
        else
        {
            free( data );
            data = NULL;
        }
        close( fd );
    }
    else ERR( "failed to load %s\n", path );
    free( path );
    return ret;
}


static int get_utf16( const WCHAR *src, unsigned int srclen, unsigned int *ch )
{
    if (IS_HIGH_SURROGATE( src[0] ))
    {
        if (srclen <= 1) return 0;
        if (!IS_LOW_SURROGATE( src[1] )) return 0;
        *ch = 0x10000 + ((src[0] & 0x3ff) << 10) + (src[1] & 0x3ff);
        return 2;
    }
    if (IS_LOW_SURROGATE( src[0] )) return 0;
    *ch = src[0];
    return 1;
}


#ifdef __APPLE__

/* The Apple filesystem enforces NFD so we need the compose tables to put it back into NFC */

struct norm_table
{
    WCHAR   name[13];      /* 00 file name */
    USHORT  checksum[3];   /* 1a checksum? */
    USHORT  version[4];    /* 20 Unicode version */
    USHORT  form;          /* 28 normalization form */
    USHORT  len_factor;    /* 2a factor for length estimates */
    USHORT  unknown1;      /* 2c */
    USHORT  decomp_size;   /* 2e decomposition hash size */
    USHORT  comp_size;     /* 30 composition hash size */
    USHORT  unknown2;      /* 32 */
    USHORT  classes;       /* 34 combining classes table offset */
    USHORT  props_level1;  /* 36 char properties table level 1 offset */
    USHORT  props_level2;  /* 38 char properties table level 2 offset */
    USHORT  decomp_hash;   /* 3a decomposition hash table offset */
    USHORT  decomp_map;    /* 3c decomposition character map table offset */
    USHORT  decomp_seq;    /* 3e decomposition character sequences offset */
    USHORT  comp_hash;     /* 40 composition hash table offset */
    USHORT  comp_seq;      /* 42 composition character sequences offset */
    /* BYTE[]       combining class values */
    /* BYTE[0x2200] char properties index level 1 */
    /* BYTE[]       char properties index level 2 */
    /* WORD[]       decomposition hash table */
    /* WORD[]       decomposition character map */
    /* WORD[]       decomposition character sequences */
    /* WORD[]       composition hash table */
    /* WORD[]       composition character sequences */
};

static struct norm_table *nfc_table;

static void init_unix_codepage(void)
{
    nfc_table = read_nls_file( "normnfc" );
}

static void put_utf16( WCHAR *dst, unsigned int ch )
{
    if (ch >= 0x10000)
    {
        ch -= 0x10000;
        dst[0] = 0xd800 | (ch >> 10);
        dst[1] = 0xdc00 | (ch & 0x3ff);
    }
    else dst[0] = ch;
}

static BYTE rol( BYTE val, BYTE count )
{
    return (val << count) | (val >> (8 - count));
}


static BYTE get_char_props( const struct norm_table *info, unsigned int ch )
{
    const BYTE *level1 = (const BYTE *)((const USHORT *)info + info->props_level1);
    const BYTE *level2 = (const BYTE *)((const USHORT *)info + info->props_level2);
    BYTE off = level1[ch / 128];

    if (!off || off >= 0xfb) return rol( off, 5 );
    return level2[(off - 1) * 128 + ch % 128];
}

static BYTE get_combining_class( const struct norm_table *info, unsigned int c )
{
    const BYTE *classes = (const BYTE *)((const USHORT *)info + info->classes);
    BYTE class = get_char_props( info, c ) & 0x3f;

    if (class == 0x3f) return 0;
    return classes[class];
}

#define HANGUL_SBASE  0xac00
#define HANGUL_LBASE  0x1100
#define HANGUL_VBASE  0x1161
#define HANGUL_TBASE  0x11a7
#define HANGUL_LCOUNT 19
#define HANGUL_VCOUNT 21
#define HANGUL_TCOUNT 28
#define HANGUL_NCOUNT (HANGUL_VCOUNT * HANGUL_TCOUNT)
#define HANGUL_SCOUNT (HANGUL_LCOUNT * HANGUL_NCOUNT)

static unsigned int compose_hangul( unsigned int ch1, unsigned int ch2 )
{
    if (ch1 >= HANGUL_LBASE && ch1 < HANGUL_LBASE + HANGUL_LCOUNT)
    {
        int lindex = ch1 - HANGUL_LBASE;
        int vindex = ch2 - HANGUL_VBASE;
        if (vindex >= 0 && vindex < HANGUL_VCOUNT)
            return HANGUL_SBASE + (lindex * HANGUL_VCOUNT + vindex) * HANGUL_TCOUNT;
    }
    if (ch1 >= HANGUL_SBASE && ch1 < HANGUL_SBASE + HANGUL_SCOUNT)
    {
        int sindex = ch1 - HANGUL_SBASE;
        if (!(sindex % HANGUL_TCOUNT))
        {
            int tindex = ch2 - HANGUL_TBASE;
            if (tindex > 0 && tindex < HANGUL_TCOUNT) return ch1 + tindex;
        }
    }
    return 0;
}

static unsigned int compose_chars( const struct norm_table *info, unsigned int ch1, unsigned int ch2 )
{
    const USHORT *table = (const USHORT *)info + info->comp_hash;
    const WCHAR *chars = (const USHORT *)info + info->comp_seq;
    unsigned int hash, start, end, i, len, ch[3];

    hash = (ch1 + 95 * ch2) % info->comp_size;
    start = table[hash];
    end = table[hash + 1];
    while (start < end)
    {
        for (i = 0; i < 3; i++, start += len) len = get_utf16( chars + start, end - start, ch + i );
        if (ch[0] == ch1 && ch[1] == ch2) return ch[2];
    }
    return 0;
}

static unsigned int compose_string( const struct norm_table *info, WCHAR *str, unsigned int srclen )
{
    unsigned int i, ch, comp, len, start_ch = 0, last_starter = srclen;
    BYTE class, prev_class = 0;

    for (i = 0; i < srclen; i += len)
    {
        if (!(len = get_utf16( str + i, srclen - i, &ch ))) return 0;
        class = get_combining_class( info, ch );
        if (last_starter == srclen || (prev_class && prev_class >= class) ||
            (!(comp = compose_hangul( start_ch, ch )) &&
             !(comp = compose_chars( info, start_ch, ch ))))
        {
            if (!class)
            {
                last_starter = i;
                start_ch = ch;
            }
            prev_class = class;
        }
        else
        {
            int comp_len = 1 + (comp >= 0x10000);
            int start_len = 1 + (start_ch >= 0x10000);

            if (comp_len != start_len)
                memmove( str + last_starter + comp_len, str + last_starter + start_len,
                         (i - (last_starter + start_len)) * sizeof(WCHAR) );
            memmove( str + i + comp_len - start_len, str + i + len, (srclen - i - len) * sizeof(WCHAR) );
            srclen += comp_len - start_len - len;
            start_ch = comp;
            i = last_starter;
            len = comp_len;
            prev_class = 0;
            put_utf16( str + i, comp );
        }
    }
    return srclen;
}

#elif defined(__ANDROID__)  /* Android always uses UTF-8 */

static void init_unix_codepage(void) { }

#else  /* __APPLE__ || __ANDROID__ */

/* charset to codepage map, sorted by name */
static const struct { const char *name; UINT cp; } charset_names[] =
{
    { "ANSIX341968", 20127 },
    { "BIG5", 950 },
    { "BIG5HKSCS", 950 },
    { "CP1250", 1250 },
    { "CP1251", 1251 },
    { "CP1252", 1252 },
    { "CP1253", 1253 },
    { "CP1254", 1254 },
    { "CP1255", 1255 },
    { "CP1256", 1256 },
    { "CP1257", 1257 },
    { "CP1258", 1258 },
    { "CP932", 932 },
    { "CP936", 936 },
    { "CP949", 949 },
    { "CP950", 950 },
    { "EUCJP", 20932 },
    { "EUCKR", 949 },
    { "GB18030", 936  /* 54936 */ },
    { "GB2312", 936 },
    { "GBK", 936 },
    { "IBM037", 37 },
    { "IBM1026", 1026 },
    { "IBM424", 20424 },
    { "IBM437", 437 },
    { "IBM500", 500 },
    { "IBM850", 850 },
    { "IBM852", 852 },
    { "IBM855", 855 },
    { "IBM857", 857 },
    { "IBM860", 860 },
    { "IBM861", 861 },
    { "IBM862", 862 },
    { "IBM863", 863 },
    { "IBM864", 864 },
    { "IBM865", 865 },
    { "IBM866", 866 },
    { "IBM869", 869 },
    { "IBM874", 874 },
    { "IBM875", 875 },
    { "ISO88591", 28591 },
    { "ISO885913", 28603 },
    { "ISO885915", 28605 },
    { "ISO88592", 28592 },
    { "ISO88593", 28593 },
    { "ISO88594", 28594 },
    { "ISO88595", 28595 },
    { "ISO88596", 28596 },
    { "ISO88597", 28597 },
    { "ISO88598", 28598 },
    { "ISO88599", 28599 },
    { "KOI8R", 20866 },
    { "KOI8U", 21866 },
    { "TIS620", 28601 },
    { "UTF8", CP_UTF8 }
};

static void init_unix_cptable( USHORT *ptr )
{
    unix_cp.data = ptr;
    ptr += ptr[0];
    unix_cp.wctable = ptr + ptr[0] + 1;
    unix_cp.mbtable = ++ptr;
    ptr += 256;
    if (*ptr++) ptr += 256;  /* glyph table */
    if (*ptr) unix_cp.dbcs = ptr + 1; /* dbcs ranges */
}

static void init_unix_codepage(void)
{
    char charset_name[16];
    const char *name;
    size_t i, j;
    int min = 0, max = ARRAY_SIZE(charset_names) - 1;

    setlocale( LC_CTYPE, "" );
    if (!(name = nl_langinfo( CODESET ))) return;

    /* remove punctuation characters from charset name */
    for (i = j = 0; name[i] && j < sizeof(charset_name)-1; i++)
    {
        if (name[i] >= '0' && name[i] <= '9') charset_name[j++] = name[i];
        else if (name[i] >= 'A' && name[i] <= 'Z') charset_name[j++] = name[i];
        else if (name[i] >= 'a' && name[i] <= 'z') charset_name[j++] = name[i] + ('A' - 'a');
    }
    charset_name[j] = 0;

    while (min <= max)
    {
        int pos = (min + max) / 2;
        int res = strcmp( charset_names[pos].name, charset_name );
        if (!res)
        {
            if (charset_names[pos].cp != CP_UTF8)
            {
                char name[16];
                void *data;

                sprintf( name, "c_%03u", charset_names[pos].cp );
                if ((data = read_nls_file( name ))) init_unix_cptable( data );
            }
            return;
        }
        if (res > 0) max = pos - 1;
        else min = pos + 1;
    }
    ERR( "unrecognized charset '%s'\n", name );
}

#endif  /* __APPLE__ || __ANDROID__ */


static inline SIZE_T get_env_length( const WCHAR *env )
{
    const WCHAR *end = env;
    while (*end) end += wcslen(end) + 1;
    return end + 1 - env;
}


/***********************************************************************
 *           is_special_env_var
 *
 * Check if an environment variable needs to be handled specially when
 * passed through the Unix environment (i.e. prefixed with "WINE").
 */
static inline BOOL is_special_env_var( const char *var )
{
    return (!strncmp( var, "PATH=", sizeof("PATH=")-1 ) ||
            !strncmp( var, "PWD=", sizeof("PWD=")-1 ) ||
            !strncmp( var, "HOME=", sizeof("HOME=")-1 ) ||
            !strncmp( var, "TEMP=", sizeof("TEMP=")-1 ) ||
            !strncmp( var, "TMP=", sizeof("TMP=")-1 ) ||
            !strncmp( var, "QT_", sizeof("QT_")-1 ) ||
            !strncmp( var, "VK_", sizeof("VK_")-1 ));
}


static unsigned int decode_utf8_char( unsigned char ch, const char **str, const char *strend )
{
    /* number of following bytes in sequence based on first byte value (for bytes above 0x7f) */
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

    /* first byte mask depending on UTF-8 sequence length */
    static const unsigned char utf8_mask[4] = { 0x7f, 0x1f, 0x0f, 0x07 };

    unsigned int len = utf8_length[ch - 0x80];
    unsigned int res = ch & utf8_mask[len];
    const char *end = *str + len;

    if (end > strend)
    {
        *str = end;
        return ~0;
    }
    switch (len)
    {
    case 3:
        if ((ch = end[-3] ^ 0x80) >= 0x40) break;
        res = (res << 6) | ch;
        (*str)++;
        if (res < 0x10) break;
    case 2:
        if ((ch = end[-2] ^ 0x80) >= 0x40) break;
        res = (res << 6) | ch;
        if (res >= 0x110000 >> 6) break;
        (*str)++;
        if (res < 0x20) break;
        if (res >= 0xd800 >> 6 && res <= 0xdfff >> 6) break;
    case 1:
        if ((ch = end[-1] ^ 0x80) >= 0x40) break;
        res = (res << 6) | ch;
        (*str)++;
        if (res < 0x80) break;
        return res;
    }
    return ~0;
}


/******************************************************************
 *      ntdll_umbstowcs
 */
DWORD ntdll_umbstowcs( const char *src, DWORD srclen, WCHAR *dst, DWORD dstlen )
{
    DWORD reslen;

    if (unix_cp.data)
    {
        DWORD i;

        if (unix_cp.dbcs)
        {
            for (i = dstlen; srclen && i; i--, srclen--, src++, dst++)
            {
                USHORT off = unix_cp.dbcs[(unsigned char)*src];
                if (off && srclen > 1)
                {
                    src++;
                    srclen--;
                    *dst = unix_cp.dbcs[off + (unsigned char)*src];
                }
                else *dst = unix_cp.mbtable[(unsigned char)*src];
            }
            reslen = dstlen - i;
        }
        else
        {
            reslen = min( srclen, dstlen );
            for (i = 0; i < reslen; i++) dst[i] = unix_cp.mbtable[(unsigned char)src[i]];
        }
    }
    else  /* utf-8 */
    {
        unsigned int res;
        const char *srcend = src + srclen;
        WCHAR *dstend = dst + dstlen;

        while ((dst < dstend) && (src < srcend))
        {
            unsigned char ch = *src++;
            if (ch < 0x80)  /* special fast case for 7-bit ASCII */
            {
                *dst++ = ch;
                continue;
            }
            if ((res = decode_utf8_char( ch, &src, srcend )) <= 0xffff)
            {
                *dst++ = res;
            }
            else if (res <= 0x10ffff)  /* we need surrogates */
            {
                res -= 0x10000;
                *dst++ = 0xd800 | (res >> 10);
                if (dst == dstend) break;
                *dst++ = 0xdc00 | (res & 0x3ff);
            }
            else
            {
                *dst++ = 0xfffd;
            }
        }
        reslen = dstlen - (dstend - dst);
#ifdef __APPLE__  /* work around broken Mac OS X filesystem that enforces NFD */
        if (reslen && nfc_table) reslen = compose_string( nfc_table, dst - reslen, reslen );
#endif
    }
    return reslen;
}


/******************************************************************
 *      ntdll_wcstoumbs
 */
int ntdll_wcstoumbs( const WCHAR *src, DWORD srclen, char *dst, DWORD dstlen, BOOL strict )
{
    DWORD i, reslen;

    if (unix_cp.data)
    {
        if (unix_cp.dbcs)
        {
            const unsigned short *uni2cp = unix_cp.wctable;
            for (i = dstlen; srclen && i; i--, srclen--, src++)
            {
                unsigned short ch = uni2cp[*src];
                if (ch >> 8)
                {
                    if (strict && unix_cp.dbcs[unix_cp.dbcs[ch >> 8] + (ch & 0xff)] != *src) return -1;
                    if (i == 1) break;  /* do not output a partial char */
                    i--;
                    *dst++ = ch >> 8;
                }
                else
                {
                    if (unix_cp.mbtable[ch] != *src) return -1;
                    *dst++ = (char)ch;
                }
            }
            reslen = dstlen - i;
        }
        else
        {
            const unsigned char *uni2cp = unix_cp.wctable;
            reslen = min( srclen, dstlen );
            for (i = 0; i < reslen; i++)
            {
                unsigned char ch = uni2cp[src[i]];
                if (strict && unix_cp.mbtable[ch] != src[i]) return -1;
                dst[i] = ch;
            }
        }
    }
    else  /* utf-8 */
    {
        char *end;
        unsigned int val;

        for (end = dst + dstlen; srclen; srclen--, src++)
        {
            WCHAR ch = *src;

            if (ch < 0x80)  /* 0x00-0x7f: 1 byte */
            {
                if (dst > end - 1) break;
                *dst++ = ch;
                continue;
            }
            if (ch < 0x800)  /* 0x80-0x7ff: 2 bytes */
            {
                if (dst > end - 2) break;
                dst[1] = 0x80 | (ch & 0x3f);
                ch >>= 6;
                dst[0] = 0xc0 | ch;
                dst += 2;
                continue;
            }
            if (!get_utf16( src, srclen, &val ))
            {
                if (strict) return -1;
                val = 0xfffd;
            }
            if (val < 0x10000)  /* 0x800-0xffff: 3 bytes */
            {
                if (dst > end - 3) break;
                dst[2] = 0x80 | (val & 0x3f);
                val >>= 6;
                dst[1] = 0x80 | (val & 0x3f);
                val >>= 6;
                dst[0] = 0xe0 | val;
                dst += 3;
            }
            else   /* 0x10000-0x10ffff: 4 bytes */
            {
                if (dst > end - 4) break;
                dst[3] = 0x80 | (val & 0x3f);
                val >>= 6;
                dst[2] = 0x80 | (val & 0x3f);
                val >>= 6;
                dst[1] = 0x80 | (val & 0x3f);
                val >>= 6;
                dst[0] = 0xf0 | val;
                dst += 4;
                src++;
                srclen--;
            }
        }
        reslen = dstlen - (end - dst);
    }
    return reslen;
}


/***********************************************************************
 *           build_envp
 *
 * Build the environment of a new child process.
 */
char **build_envp( const WCHAR *envW )
{
    static const char * const unix_vars[] = { "PATH", "TEMP", "TMP", "HOME" };
    char **envp;
    char *env, *p;
    int count = 1, length, lenW;
    unsigned int i;

    lenW = get_env_length( envW );
    if (!(env = malloc( lenW * 3 ))) return NULL;
    length = ntdll_wcstoumbs( envW, lenW, env, lenW * 3, FALSE );

    for (p = env; *p; p += strlen(p) + 1, count++)
        if (is_special_env_var( p )) length += 4; /* prefix it with "WINE" */

    for (i = 0; i < ARRAY_SIZE( unix_vars ); i++)
    {
        if (!(p = getenv(unix_vars[i]))) continue;
        length += strlen(unix_vars[i]) + strlen(p) + 2;
        count++;
    }

    if ((envp = malloc( count * sizeof(*envp) + length )))
    {
        char **envptr = envp;
        char *dst = (char *)(envp + count);

        /* some variables must not be modified, so we get them directly from the unix env */
        for (i = 0; i < ARRAY_SIZE( unix_vars ); i++)
        {
            if (!(p = getenv( unix_vars[i] ))) continue;
            *envptr++ = strcpy( dst, unix_vars[i] );
            strcat( dst, "=" );
            strcat( dst, p );
            dst += strlen(dst) + 1;
        }

        /* now put the Windows environment strings */
        for (p = env; *p; p += strlen(p) + 1)
        {
            if (*p == '=') continue;  /* skip drive curdirs, this crashes some unix apps */
            if (!strncmp( p, "WINEPRELOADRESERVE=", sizeof("WINEPRELOADRESERVE=")-1 )) continue;
            if (!strncmp( p, "WINELOADERNOEXEC=", sizeof("WINELOADERNOEXEC=")-1 )) continue;
            if (!strncmp( p, "WINESERVERSOCKET=", sizeof("WINESERVERSOCKET=")-1 )) continue;
            if (is_special_env_var( p ))  /* prefix it with "WINE" */
            {
                *envptr++ = strcpy( dst, "WINE" );
                strcat( dst, p );
            }
            else
            {
                *envptr++ = strcpy( dst, p );
            }
            dst += strlen(dst) + 1;
        }
        *envptr = 0;
    }
    free( env );
    return envp;
}


/***********************************************************************
 *           set_process_name
 *
 * Change the process name in the ps output.
 */
static int set_process_name( int argc, char *argv[] )
{
    BOOL shift_strings;
    char *p, *name;
    int i;

#ifdef HAVE_SETPROCTITLE
    setproctitle("-%s", argv[1]);
    shift_strings = FALSE;
#else
    p = argv[0];

    shift_strings = (argc >= 2);
    for (i = 1; i < argc; i++)
    {
        p += strlen(p) + 1;
        if (p != argv[i])
        {
            shift_strings = FALSE;
            break;
        }
    }
#endif

    if (shift_strings)
    {
        int offset = argv[1] - argv[0];
        char *end = argv[argc-1] + strlen(argv[argc-1]) + 1;
        memmove( argv[0], argv[1], end - argv[1] );
        memset( end - offset, 0, offset );
        for (i = 1; i < argc; i++)
            argv[i-1] = argv[i] - offset;
        argv[i-1] = NULL;
    }
    else
    {
        /* remove argv[0] */
        memmove( argv, argv + 1, argc * sizeof(argv[0]) );
    }

    name = argv[0];
    if ((p = strrchr( name, '\\' ))) name = p + 1;
    if ((p = strrchr( name, '/' ))) name = p + 1;

#if defined(HAVE_SETPROGNAME)
    setprogname( name );
#endif

#ifdef HAVE_PRCTL
#ifndef PR_SET_NAME
# define PR_SET_NAME 15
#endif
    prctl( PR_SET_NAME, name );
#endif  /* HAVE_PRCTL */
    return argc - 1;
}


/***********************************************************************
 *              build_wargv
 *
 * Build the Unicode argv array.
 */
static WCHAR **build_wargv( char **argv )
{
    int argc;
    WCHAR *p, **wargv;
    DWORD total = 0;

    for (argc = 0; argv[argc]; argc++) total += strlen(argv[argc]) + 1;

    wargv = malloc( total * sizeof(WCHAR) + (argc + 1) * sizeof(*wargv) );
    p = (WCHAR *)(wargv + argc + 1);
    for (argc = 0; argv[argc]; argc++)
    {
        DWORD reslen = ntdll_umbstowcs( argv[argc], strlen(argv[argc]) + 1, p, total );
        wargv[argc] = p;
        p += reslen;
        total -= reslen;
    }
    wargv[argc] = NULL;
    return wargv;
}


/* Unix format is: lang[_country][.charset][@modifier]
 * Windows format is: lang[-script][-country][_modifier] */
static BOOL unix_to_win_locale( const char *unix_name, char *win_name )
{
    static const char sep[] = "_.@";
    char buffer[LOCALE_NAME_MAX_LENGTH];
    char *p, *country = NULL, *modifier = NULL;

    if (!unix_name || !unix_name[0] || !strcmp( unix_name, "C" ))
    {
        unix_name = getenv( "LC_ALL" );
        if (!unix_name || !unix_name[0]) return FALSE;
    }

    if (strlen( unix_name ) >= LOCALE_NAME_MAX_LENGTH) return FALSE;
    strcpy( buffer, unix_name );
    if (!(p = strpbrk( buffer, sep )))
    {
        if (!strcmp( buffer, "POSIX" ) || !strcmp( buffer, "C" ))
            strcpy( win_name, "en-US" );
        else
            strcpy( win_name, buffer );
        return TRUE;
    }

    if (*p == '_')
    {
        *p++ = 0;
        country = p;
        p = strpbrk( p, sep + 1 );
    }
    if (p && *p == '.')
    {
        *p++ = 0;
        /* charset, ignore */
        p = strchr( p, '@' );
    }
    if (p)
    {
        *p++ = 0;
        modifier = p;
    }

    /* rebuild a Windows name */

    strcpy( win_name, buffer );
    if (modifier)
    {
        if (!strcmp( modifier, "latin" )) strcat( win_name, "-Latn" );
        else if (!strcmp( modifier, "euro" )) {} /* ignore */
        else return FALSE;
    }
    if (country)
    {
        p = win_name + strlen(win_name);
        *p++ = '-';
        strcpy( p, country );
    }
    return TRUE;
}


/******************************************************************
 *		init_locale
 */
static void init_locale(void)
{
    setlocale( LC_ALL, "" );
    if (!unix_to_win_locale( setlocale( LC_CTYPE, NULL ), system_locale )) system_locale[0] = 0;
    if (!unix_to_win_locale( setlocale( LC_MESSAGES, NULL ), user_locale )) user_locale[0] = 0;

#ifdef __APPLE__
    if (!system_locale[0])
    {
        CFLocaleRef locale = CFLocaleCopyCurrent();
        CFStringRef lang = CFLocaleGetValue( locale, kCFLocaleLanguageCode );
        CFStringRef country = CFLocaleGetValue( locale, kCFLocaleCountryCode );
        CFStringRef locale_string;

        if (country)
            locale_string = CFStringCreateWithFormat(NULL, NULL, CFSTR("%@_%@"), lang, country);
        else
            locale_string = CFStringCreateCopy(NULL, lang);

        CFStringGetCString(locale_string, system_locale, sizeof(system_locale), kCFStringEncodingUTF8);
        CFRelease(locale);
        CFRelease(locale_string);
    }
    if (!user_locale[0])
    {
        /* Retrieve the preferred language as chosen in System Preferences. */
        CFArrayRef preferred_langs = CFLocaleCopyPreferredLanguages();
        if (preferred_langs && CFArrayGetCount( preferred_langs ))
        {
            CFStringRef preferred_lang = CFArrayGetValueAtIndex( preferred_langs, 0 );
            CFDictionaryRef components = CFLocaleCreateComponentsFromLocaleIdentifier( NULL, preferred_lang );
            if (components)
            {
                CFStringRef lang = CFDictionaryGetValue( components, kCFLocaleLanguageCode );
                CFStringRef country = CFDictionaryGetValue( components, kCFLocaleCountryCode );
                CFLocaleRef locale = NULL;
                CFStringRef locale_string;

                if (!country)
                {
                    locale = CFLocaleCopyCurrent();
                    country = CFLocaleGetValue( locale, kCFLocaleCountryCode );
                }
                if (country)
                    locale_string = CFStringCreateWithFormat( NULL, NULL, CFSTR("%@_%@"), lang, country );
                else
                    locale_string = CFStringCreateCopy( NULL, lang );
                CFStringGetCString( locale_string, user_locale, sizeof(user_locale), kCFStringEncodingUTF8 );
                CFRelease( locale_string );
                if (locale) CFRelease( locale );
                CFRelease( components );
            }
        }
        if (preferred_langs) CFRelease( preferred_langs );
    }
#endif
    setlocale( LC_NUMERIC, "C" );  /* FIXME: oleaut32 depends on this */
}


/***********************************************************************
 *              init_environment
 */
void init_environment( int argc, char *argv[], char *envp[] )
{
    USHORT *case_table;

    init_unix_codepage();
    init_locale();

    if ((case_table = read_nls_file( "l_intl" )))
    {
        uctable = case_table + 2;
        lctable = case_table + case_table[1] + 2;
    }

    main_argc = set_process_name( argc, argv );
    main_argv = argv;
    main_wargv = build_wargv( argv );
    main_envp = envp;
}


static const char overrides_help_message[] =
    "Syntax:\n"
    "  WINEDLLOVERRIDES=\"entry;entry;entry...\"\n"
    "    where each entry is of the form:\n"
    "        module[,module...]={native|builtin}[,{b|n}]\n"
    "\n"
    "    Only the first letter of the override (native or builtin)\n"
    "    is significant.\n\n"
    "Example:\n"
    "  WINEDLLOVERRIDES=\"comdlg32=n,b;shell32,shlwapi=b\"\n";

/*************************************************************************
 *		get_initial_environment
 *
 * Return the initial environment.
 */
NTSTATUS CDECL get_initial_environment( WCHAR **wargv[], WCHAR *env, SIZE_T *size )
{
    char **e;
    WCHAR *ptr = env, *end = env + *size;

    *wargv = main_wargv;
    for (e = main_envp; *e && ptr < end; e++)
    {
        char *str = *e;

        /* skip Unix special variables and use the Wine variants instead */
        if (!strncmp( str, "WINE", 4 ))
        {
            if (is_special_env_var( str + 4 )) str += 4;
            else if (!strncmp( str, "WINEPRELOADRESERVE=", 19 )) continue;  /* skip it */
            else if (!strcmp( str, "WINEDLLOVERRIDES=help" ))
            {
                MESSAGE( overrides_help_message );
                exit(0);
            }
        }
        else if (is_special_env_var( str )) continue;  /* skip it */

        ptr += ntdll_umbstowcs( str, strlen(str) + 1, ptr, end - ptr );
    }

    if (ptr < end)
    {
        *ptr++ = 0;
        *size = ptr - env;
        return STATUS_SUCCESS;
    }

    /* estimate needed size */
    for (e = main_envp, *size = 1; *e; e++) if (!is_special_env_var( *e )) *size += strlen(*e) + 1;
    return STATUS_BUFFER_TOO_SMALL;
}


/* append a variable to the environment */
static void append_envA( WCHAR *env, SIZE_T *pos, const char *name, const char *value )
{
    SIZE_T i = *pos;

    while (*name) env[i++] = (unsigned char)*name++;
    if (value)
    {
        env[i++] = '=';
        i += ntdll_umbstowcs( value, strlen(value), env + i, strlen(value) );
    }
    env[i++] = 0;
    *pos = i;
}

static void append_envW( WCHAR *env, SIZE_T *pos, const char *name, const WCHAR *value )
{
    SIZE_T i = *pos;

    while (*name) env[i++] = (unsigned char)*name++;
    if (value)
    {
        env[i++] = '=';
        while (*value) env[i++] = *value++;
    }
    env[i++] = 0;
    *pos = i;
}

/* set an environment variable for one of the wine path variables */
static void add_path_var( WCHAR *env, SIZE_T *pos, const char *name, const char *path )
{
    WCHAR *nt_name;

    if (!path) append_envW( env, pos, name, NULL );
    else
    {
        if (unix_to_nt_file_name( path, &nt_name )) return;
        append_envW( env, pos, name, nt_name );
        free( nt_name );
    }
}


/*************************************************************************
 *		get_startup_info
 *
 * Get the startup information from the server.
 */
NTSTATUS CDECL get_startup_info( startup_info_t *info, SIZE_T *total_size, SIZE_T *info_size )
{
    NTSTATUS status;

    if (*total_size < startup_info_size)
    {
        *total_size = startup_info_size;
        return STATUS_BUFFER_TOO_SMALL;
    }
    SERVER_START_REQ( get_startup_info )
    {
        wine_server_set_reply( req, info, *total_size );
        status = wine_server_call( req );
        *total_size = wine_server_reply_size( reply );
        *info_size = reply->info_size;
    }
    SERVER_END_REQ;
    return status;
}


/*************************************************************************
 *		get_dynamic_environment
 *
 * Get the environment variables that can differ between processes.
 */
NTSTATUS CDECL get_dynamic_environment( WCHAR *env, SIZE_T *size )
{
    const char *overrides = getenv( "WINEDLLOVERRIDES" );
    SIZE_T alloc, pos = 0;
    WCHAR *buffer;
    DWORD i;
    char dlldir[22];
    NTSTATUS status = STATUS_SUCCESS;

    alloc = 20 * 7;  /* 7 variable names */
    if (data_dir) alloc += strlen( data_dir ) + 9;
    if (home_dir) alloc += strlen( home_dir ) + 9;
    if (build_dir) alloc += strlen( build_dir ) + 9;
    if (config_dir) alloc += strlen( config_dir ) + 9;
    if (user_name) alloc += strlen( user_name );
    if (overrides) alloc += strlen( overrides );
    for (i = 0; dll_paths[i]; i++) alloc += 20 + strlen( dll_paths[i] ) + 9;

    if (!(buffer = malloc( alloc * sizeof(WCHAR) ))) return STATUS_NO_MEMORY;
    pos = 0;
    add_path_var( buffer, &pos, "WINEDATADIR", data_dir );
    add_path_var( buffer, &pos, "WINEHOMEDIR", home_dir );
    add_path_var( buffer, &pos, "WINEBUILDDIR", build_dir );
    add_path_var( buffer, &pos, "WINECONFIGDIR", config_dir );
    for (i = 0; dll_paths[i]; i++)
    {
        sprintf( dlldir, "WINEDLLDIR%u", i );
        add_path_var( buffer, &pos, dlldir, dll_paths[i] );
    }
    sprintf( dlldir, "WINEDLLDIR%u", i );
    append_envW( buffer, &pos, dlldir, NULL );
    append_envA( buffer, &pos, "WINEUSERNAME", user_name );
    append_envA( buffer, &pos, "WINEDLLOVERRIDES", overrides );
    assert( pos <= alloc );

    if (pos < *size)
    {
        memcpy( env, buffer, pos * sizeof(WCHAR) );
        env[pos] = 0;
    }
    else status = STATUS_BUFFER_TOO_SMALL;
    *size = pos + 1;
    free( buffer );
    return status;
}


/*************************************************************************
 *		get_initial_console
 *
 * Return the initial console handles.
 */
void CDECL get_initial_console( RTL_USER_PROCESS_PARAMETERS *params )
{
    int output_fd = -1;

    wine_server_fd_to_handle( 0, GENERIC_READ|SYNCHRONIZE,  OBJ_INHERIT, &params->hStdInput );
    wine_server_fd_to_handle( 1, GENERIC_WRITE|SYNCHRONIZE, OBJ_INHERIT, &params->hStdOutput );
    wine_server_fd_to_handle( 2, GENERIC_WRITE|SYNCHRONIZE, OBJ_INHERIT, &params->hStdError );

    /* mark tty handles for kernelbase, see init_console */
    if (params->hStdInput && isatty(0))
    {
        params->ConsoleHandle = CONSOLE_HANDLE_SHELL;
        params->hStdInput = (HANDLE)((UINT_PTR)params->hStdInput | 1);
    }
    if (params->hStdError && isatty(2))
    {
        params->ConsoleHandle = CONSOLE_HANDLE_SHELL;
        params->hStdError = (HANDLE)((UINT_PTR)params->hStdError | 1);
        output_fd = 2;
    }
    if (params->hStdOutput && isatty(1))
    {
        params->ConsoleHandle = CONSOLE_HANDLE_SHELL;
        params->hStdOutput = (HANDLE)((UINT_PTR)params->hStdOutput | 1);
        output_fd = 1;
    }

    if (output_fd != -1)
    {
        struct winsize size;
        if (!ioctl( output_fd, TIOCGWINSZ, &size ))
        {
            params->dwXCountChars = size.ws_col;
            params->dwYCountChars = size.ws_row;
        }
    }
}


/*************************************************************************
 *		get_initial_directory
 *
 * Get the current directory at startup.
 */
void CDECL get_initial_directory( UNICODE_STRING *dir )
{
    const char *pwd;
    char *cwd;
    int size;

    dir->Length = 0;

    /* try to get it from the Unix cwd */

    for (size = 1024; ; size *= 2)
    {
        if (!(cwd = malloc( size ))) break;
        if (getcwd( cwd, size )) break;
        free( cwd );
        if (errno == ERANGE) continue;
        cwd = NULL;
        break;
    }

    /* try to use PWD if it is valid, so that we don't resolve symlinks */

    pwd = getenv( "PWD" );
    if (cwd)
    {
        struct stat st1, st2;

        if (!pwd || stat( pwd, &st1 ) == -1 ||
            (!stat( cwd, &st2 ) && (st1.st_dev != st2.st_dev || st1.st_ino != st2.st_ino)))
            pwd = cwd;
    }

    if (pwd)
    {
        WCHAR *nt_name;

        if (!unix_to_nt_file_name( pwd, &nt_name ))
        {
            /* skip the \??\ prefix */
            ULONG len = wcslen( nt_name );
            if (len > 6 && nt_name[5] == ':')
            {
                dir->Length = (len - 4) * sizeof(WCHAR);
                memcpy( dir->Buffer, nt_name + 4, dir->Length );
            }
            else  /* change \??\ to \\?\ */
            {
                dir->Length = len * sizeof(WCHAR);
                memcpy( dir->Buffer, nt_name, dir->Length );
                dir->Buffer[1] = '\\';
            }
            free( nt_name );
        }
    }

    if (!dir->Length)  /* still not initialized */
        MESSAGE("Warning: could not find DOS drive for current working directory '%s', "
                "starting in the Windows directory.\n", cwd ? cwd : "" );
    free( cwd );
    chdir( "/" ); /* avoid locking removable devices */
}


/*************************************************************************
 *		get_unix_codepage_data
 *
 * Return the Unix codepage data.
 */
USHORT * CDECL get_unix_codepage_data(void)
{
    return unix_cp.data;
}


/*************************************************************************
 *		get_locales
 *
 * Return the system and user locales. Buffers must be at least LOCALE_NAME_MAX_LENGTH chars long.
 */
void CDECL get_locales( WCHAR *sys, WCHAR *user )
{
    ntdll_umbstowcs( system_locale, strlen(system_locale) + 1, sys, LOCALE_NAME_MAX_LENGTH );
    ntdll_umbstowcs( user_locale, strlen(user_locale) + 1, user, LOCALE_NAME_MAX_LENGTH );
}


/**********************************************************************
 *      NtQueryDefaultLocale  (NTDLL.@)
 */
NTSTATUS WINAPI NtQueryDefaultLocale( BOOLEAN user, LCID *lcid )
{
    *lcid = user ? user_lcid : system_lcid;
    return STATUS_SUCCESS;
}


/**********************************************************************
 *      NtSetDefaultLocale  (NTDLL.@)
 */
NTSTATUS WINAPI NtSetDefaultLocale( BOOLEAN user, LCID lcid )
{
    if (user) user_lcid = lcid;
    else
    {
        system_lcid = lcid;
        system_ui_language = LANGIDFROMLCID(lcid); /* there is no separate call to set it */
    }
    return STATUS_SUCCESS;
}


/**********************************************************************
 *      NtQueryDefaultUILanguage  (NTDLL.@)
 */
NTSTATUS WINAPI NtQueryDefaultUILanguage( LANGID *lang )
{
    *lang = user_ui_language;
    return STATUS_SUCCESS;
}


/**********************************************************************
 *      NtSetDefaultUILanguage  (NTDLL.@)
 */
NTSTATUS WINAPI NtSetDefaultUILanguage( LANGID lang )
{
    user_ui_language = lang;
    return STATUS_SUCCESS;
}


/**********************************************************************
 *      NtQueryInstallUILanguage  (NTDLL.@)
 */
NTSTATUS WINAPI NtQueryInstallUILanguage( LANGID *lang )
{
    *lang = system_ui_language;
    return STATUS_SUCCESS;
}
