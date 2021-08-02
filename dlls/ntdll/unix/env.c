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

PEB *peb = NULL;
USHORT *uctable = NULL, *lctable = NULL;
SIZE_T startup_info_size = 0;
BOOL is_prefix_bootstrap = FALSE;

static const WCHAR bootstrapW[] = {'W','I','N','E','B','O','O','T','S','T','R','A','P','M','O','D','E'};

int main_argc = 0;
char **main_argv = NULL;
char **main_envp = NULL;
WCHAR **main_wargv = NULL;

static LCID user_lcid, system_lcid;
static LANGID user_ui_language, system_ui_language;

static char system_locale[LOCALE_NAME_MAX_LENGTH];
static char user_locale[LOCALE_NAME_MAX_LENGTH];

/* system directory with trailing backslash */
const WCHAR system_dir[] = {'\\','?','?','\\','C',':','\\','w','i','n','d','o','w','s','\\',
                            's','y','s','t','e','m','3','2','\\',0};

static struct
{
    USHORT *data;
    USHORT *dbcs;
    USHORT *mbtable;
    void   *wctable;
} unix_cp;

enum nls_section_type
{
    NLS_SECTION_SORTKEYS = 9,
    NLS_SECTION_CASEMAP = 10,
    NLS_SECTION_CODEPAGE = 11,
    NLS_SECTION_NORMALIZE = 12
};

static char *get_nls_file_path( ULONG type, ULONG id )
{
    const char *dir = build_dir ? build_dir : data_dir;
    const char *name = NULL;
    char *path, tmp[16];

    switch (type)
    {
    case NLS_SECTION_SORTKEYS: name = "sortdefault"; break;
    case NLS_SECTION_CASEMAP:  name = "l_intl"; break;
    case NLS_SECTION_CODEPAGE: name = tmp; sprintf( tmp, "c_%03u", id ); break;
    case NLS_SECTION_NORMALIZE:
        switch (id)
        {
        case NormalizationC:  name = "normnfc"; break;
        case NormalizationD:  name = "normnfd"; break;
        case NormalizationKC: name = "normnfkc"; break;
        case NormalizationKD: name = "normnfkd"; break;
        case 13:              name = "normidna"; break;
        }
        break;
    }
    if (!name) return NULL;
    if (!(path = malloc( strlen(dir) + strlen(name) + 10 ))) return NULL;
    sprintf( path, "%s/nls/%s.nls", dir, name );
    return path;
}

static void *read_nls_file( ULONG type, ULONG id )
{
    char *path = get_nls_file_path( type, id );
    struct stat st;
    void *data, *ret = NULL;
    int fd;

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
    else ERR( "failed to load %u/%u\n", type, id );
    free( path );
    return ret;
}

static NTSTATUS open_nls_data_file( ULONG type, ULONG id, HANDLE *file )
{
    static const WCHAR sortdirW[] = {'\\','?','?','\\','C',':','\\','w','i','n','d','o','w','s','\\',
                                     'g','l','o','b','a','l','i','z','a','t','i','o','n','\\',
                                     's','o','r','t','i','n','g','\\',0};

    NTSTATUS status = STATUS_OBJECT_NAME_NOT_FOUND;
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING valueW;
    WCHAR buffer[ARRAY_SIZE(sortdirW) + 16];
    char *p, *path = get_nls_file_path( type, id );

    if (!path) return STATUS_OBJECT_NAME_NOT_FOUND;

    /* try to open file in system dir */
    wcscpy( buffer, type == NLS_SECTION_SORTKEYS ? sortdirW : system_dir );
    p = strrchr( path, '/' ) + 1;
    ascii_to_unicode( buffer + wcslen(buffer), p, strlen(p) + 1 );
    init_unicode_string( &valueW, buffer );
    InitializeObjectAttributes( &attr, &valueW, 0, 0, NULL );

    status = open_unix_file( file, path, GENERIC_READ, &attr, 0, FILE_SHARE_READ,
                             FILE_OPEN, FILE_SYNCHRONOUS_IO_ALERT, NULL, 0 );
    free( path );
    if (status != STATUS_NO_SUCH_FILE) return status;

    if ((status = nt_to_unix_file_name( &attr, &path, FILE_OPEN ))) return status;
    status = open_unix_file( file, path, GENERIC_READ, &attr, 0, FILE_SHARE_READ,
                             FILE_OPEN, FILE_SYNCHRONOUS_IO_ALERT, NULL, 0 );
    free( path );
    return status;
}

static NTSTATUS get_nls_section_name( ULONG type, ULONG id, WCHAR name[32] )
{
    char buffer[32];

    switch (type)
    {
    case NLS_SECTION_SORTKEYS:
        if (id) return STATUS_INVALID_PARAMETER_1;
        strcpy( buffer, "\\NLS\\NlsSectionSORTDEFAULT" );
        break;
    case NLS_SECTION_CASEMAP:
        if (id) return STATUS_UNSUCCESSFUL;
        strcpy( buffer, "\\NLS\\NlsSectionLANG_INTL" );
        break;
    case NLS_SECTION_CODEPAGE:
        sprintf( buffer, "\\NLS\\NlsSectionCP%03u", id);
        break;
    case NLS_SECTION_NORMALIZE:
        sprintf( buffer, "\\NLS\\NlsSectionNORM%08x", id);
        break;
    default:
        return STATUS_INVALID_PARAMETER_1;
    }
    ascii_to_unicode( name, buffer, strlen(buffer) + 1 );
    return STATUS_SUCCESS;
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
    nfc_table = read_nls_file( NLS_SECTION_NORMALIZE, NormalizationC );
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
                void *data = read_nls_file( NLS_SECTION_CODEPAGE, charset_names[pos].cp );
                if (data) init_unix_cptable( data );
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


#define STARTS_WITH(var,str) (!strncmp( var, str, sizeof(str) - 1 ))

/***********************************************************************
 *           is_special_env_var
 *
 * Check if an environment variable needs to be handled specially when
 * passed through the Unix environment (i.e. prefixed with "WINE").
 */
static BOOL is_special_env_var( const char *var )
{
    return (STARTS_WITH( var, "PATH=" ) ||
            STARTS_WITH( var, "PWD=" ) ||
            STARTS_WITH( var, "HOME=" ) ||
            STARTS_WITH( var, "TEMP=" ) ||
            STARTS_WITH( var, "TMP=" ) ||
            STARTS_WITH( var, "QT_" ) ||
            STARTS_WITH( var, "VK_" ));
}

/* check if an environment variable changes dynamically in every new process */
static BOOL is_dynamic_env_var( const char *var )
{
    return (STARTS_WITH( var, "WINEDLLOVERRIDES=" ) ||
            STARTS_WITH( var, "WINEDATADIR=" ) ||
            STARTS_WITH( var, "WINEHOMEDIR=" ) ||
            STARTS_WITH( var, "WINEBUILDDIR=" ) ||
            STARTS_WITH( var, "WINECONFIGDIR=" ) ||
            STARTS_WITH( var, "WINEDLLDIR" ) ||
            STARTS_WITH( var, "WINEUNIXCP=" ) ||
            STARTS_WITH( var, "WINELOCALE=" ) ||
            STARTS_WITH( var, "WINEUSERLOCALE=" ) ||
            STARTS_WITH( var, "WINEUSERNAME=" ) ||
            STARTS_WITH( var, "WINEPRELOADRESERVE=" ) ||
            STARTS_WITH( var, "WINELOADERNOEXEC=" ) ||
            STARTS_WITH( var, "WINESERVERSOCKET=" ));
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
    {
        if (is_dynamic_env_var( p )) continue;
        if (is_special_env_var( p )) length += 4; /* prefix it with "WINE" */
    }

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
            if (is_dynamic_env_var( p )) continue;
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
static void set_process_name( const char *name )
{
    char *p;

#ifdef HAVE_SETPROCTITLE
    setproctitle("-%s", name );
#endif
    if ((p = strrchr( name, '\\' ))) name = p + 1;
    if ((p = strrchr( name, '/' ))) name = p + 1;
#ifdef HAVE_SETPROGNAME
    setprogname( name );
#endif
#ifdef HAVE_PRCTL
#ifndef PR_SET_NAME
# define PR_SET_NAME 15
#endif
    prctl( PR_SET_NAME, name );
#endif
}


/***********************************************************************
 *           rebuild_argv
 *
 * Build the main argv by removing argv[0].
 */
static void rebuild_argv(void)
{
    BOOL shift_strings = FALSE;
    int i;

#ifndef HAVE_SETPROCTITLE
    for (i = 1; i < main_argc; i++)
        if (main_argv[i - 1] + strlen( main_argv[i - 1] ) + 1 != main_argv[i]) break;
    shift_strings = (i == main_argc);
#endif
    if (shift_strings)
    {
        int offset = main_argv[1] - main_argv[0];
        char *end = main_argv[main_argc - 1] + strlen(main_argv[main_argc - 1]) + 1;
        memmove( main_argv[0], main_argv[1], end - main_argv[1] );
        memset( end - offset, 0, offset );
        for (i = 1; i < main_argc; i++) main_argv[i - 1] = main_argv[i] - offset;
    }
    else memmove( main_argv, main_argv + 1, (main_argc - 1) * sizeof(main_argv[0]) );

    main_argv[--main_argc] = NULL;
    set_process_name( main_argv[0] );
}


/***********************************************************************
 *           prepend_argv
 *
 * Prepend values to the main argv, removing the original argv[0].
 */
static void prepend_argv( const char **args, int count )
{
    char **new_argv;
    char *p, *end;
    BOOL write_strings = FALSE;
    int i, total = 0, new_argc = main_argc + count - 1;

    for (i = 0; i < count; i++) total += strlen(args[i]) + 1;
    for (i = 1; i < main_argc; i++) total += strlen(main_argv[i]) + 1;

    new_argv = malloc( (new_argc + 1) * sizeof(*new_argv) + total );
    p = (char *)(new_argv + new_argc + 1);
    for (i = 0; i < count; i++)
    {
        new_argv[i] = p;
        strcpy( p, args[i] );
        p += strlen(p) + 1;
    }
    for (i = 1; i < main_argc; i++)
    {
        new_argv[count + i - 1] = p;
        strcpy( p, main_argv[i] );
        p += strlen(p) + 1;
    }
    new_argv[new_argc] = NULL;

    /* copy what we can over the original argv */

#ifndef HAVE_SETPROCTITLE
    for (i = 1; i < main_argc; i++)
        if (main_argv[i - 1] + strlen(main_argv[i - 1]) + 1 != main_argv[i]) break;
    write_strings = (i == main_argc);
#endif
    if (write_strings)
    {
        p = main_argv[0];
        end = main_argv[main_argc - 1] + strlen(main_argv[main_argc - 1]) + 1;

        for (i = 0; i < min( new_argc, main_argc ) && p < end; i++)
        {
            int len = min( end - p - 1, strlen(new_argv[i]) );

            memcpy( p, new_argv[i], len );
            main_argv[i] = p;
            p += len;
            *p++ = 0;
        }
        memset( p, 0, end - p );
        main_argv[i] = NULL;
    }
    else
    {
        memcpy( main_argv, new_argv, min( new_argc, main_argc ) * sizeof(*new_argv) );
        main_argv[min( new_argc, main_argc )] = NULL;
    }

    main_argc = new_argc;
    main_argv = new_argv;
    set_process_name( main_argv[0] );
}


/***********************************************************************
 *              build_wargv
 *
 * Build the Unicode argv array, replacing argv[0] by the image name.
 */
static WCHAR **build_wargv( const WCHAR *image )
{
    int argc;
    WCHAR *p, **wargv;
    DWORD total = wcslen(image) + 1;

    for (argc = 1; main_argv[argc]; argc++) total += strlen(main_argv[argc]) + 1;

    wargv = malloc( total * sizeof(WCHAR) + (argc + 1) * sizeof(*wargv) );
    p = (WCHAR *)(wargv + argc + 1);
    wargv[0] = p;
    wcscpy( p, image );
    total -= wcslen( p ) + 1;
    p += wcslen( p ) + 1;
    for (argc = 1; main_argv[argc]; argc++)
    {
        DWORD reslen = ntdll_umbstowcs( main_argv[argc], strlen(main_argv[argc]) + 1, p, total );
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
            locale_string = CFStringCreateWithFormat(NULL, NULL, CFSTR("%@-%@"), lang, country);
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
                    locale_string = CFStringCreateWithFormat( NULL, NULL, CFSTR("%@-%@"), lang, country );
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

    if ((case_table = read_nls_file( NLS_SECTION_CASEMAP, 0 )))
    {
        uctable = case_table + 2;
        lctable = case_table + case_table[1] + 2;
    }

    main_argc = argc;
    main_argv = argv;
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
static WCHAR *get_initial_environment( SIZE_T *pos, SIZE_T *size )
{
    char **e;
    WCHAR *env, *ptr, *end;

    /* estimate needed size */
    *size = 1;
    for (e = main_envp; *e; e++) *size += strlen(*e) + 1;

    if (!(env = malloc( *size * sizeof(WCHAR) ))) return NULL;
    ptr = env;
    end = env + *size - 1;
    for (e = main_envp; *e && ptr < end; e++)
    {
        char *str = *e;

        /* skip Unix special variables and use the Wine variants instead */
        if (!strncmp( str, "WINE", 4 ))
        {
            if (is_special_env_var( str + 4 )) str += 4;
            else if (!strcmp( str, "WINEDLLOVERRIDES=help" ))
            {
                MESSAGE( overrides_help_message );
                exit(0);
            }
        }
        else if (is_special_env_var( str )) continue;  /* skip it */

        if (is_dynamic_env_var( str )) continue;
        ptr += ntdll_umbstowcs( str, strlen(str) + 1, ptr, end - ptr );
    }
    *pos = ptr - env;
    return env;
}


static WCHAR *find_env_var( WCHAR *env, SIZE_T size, const WCHAR *name, SIZE_T namelen )
{
    WCHAR *p = env;

    while (p < env + size)
    {
        if (!wcsnicmp( p, name, namelen ) && p[namelen] == '=') return p;
        p += wcslen(p) + 1;
    }
    return NULL;
}

static WCHAR *get_env_var( WCHAR *env, SIZE_T size, const WCHAR *name, SIZE_T namelen )
{
    WCHAR *ret = NULL, *var = find_env_var( env, size, name, namelen );

    if (var)
    {
        var += namelen + 1;  /* skip name */
        if ((ret = malloc( (wcslen(var) + 1) * sizeof(WCHAR) ))) wcscpy( ret, var );
    }
    return ret;
}

/* set an environment variable, replacing it if it exists */
static void set_env_var( WCHAR **env, SIZE_T *pos, SIZE_T *size,
                         const WCHAR *name, SIZE_T namelen, const WCHAR *value )
{
    WCHAR *p;
    SIZE_T len;

    /* remove existing string */
    if ((p = find_env_var( *env, *pos, name, namelen )))
    {
        len = wcslen(p) + 1;
        memmove( p, p + len, (*pos - (p + len - *env)) * sizeof(WCHAR) );
        *pos -= len;
    }

    if (!value) return;
    len = wcslen( value );
    if (*pos + namelen + len + 3 > *size)
    {
        *size = max( *size * 2, *pos + namelen + len + 3 );
        *env = realloc( *env, *size * sizeof(WCHAR) );
    }
    memcpy( *env + *pos, name, namelen * sizeof(WCHAR) );
    (*env)[*pos + namelen] = '=';
    memcpy( *env + *pos + namelen + 1, value, (len + 1) * sizeof(WCHAR) );
    *pos += namelen + len + 2;
}

static void append_envW( WCHAR **env, SIZE_T *pos, SIZE_T *size, const char *name, const WCHAR *value )
{
    WCHAR nameW[32];

    ascii_to_unicode( nameW, name, strlen(name) + 1 );
    set_env_var( env, pos, size, nameW, wcslen(nameW), value );
}

static void append_envA( WCHAR **env, SIZE_T *pos, SIZE_T *size, const char *name, const char *value )
{
    if (value)
    {
        SIZE_T len = strlen(value) + 1;
        WCHAR *valueW = malloc( len * sizeof(WCHAR) );
        ntdll_umbstowcs( value, len, valueW, len );
        append_envW( env, pos, size, name, valueW );
        free( valueW );
    }
    else append_envW( env, pos, size, name, NULL );
}

/* set an environment variable for one of the wine path variables */
static void add_path_var( WCHAR **env, SIZE_T *pos, SIZE_T *size, const char *name, const char *path )
{
    WCHAR *nt_name = NULL;

    if (path && unix_to_nt_file_name( path, &nt_name )) return;
    append_envW( env, pos, size, name, nt_name );
    free( nt_name );
}


/*************************************************************************
 *		add_dynamic_environment
 *
 * Add the environment variables that can differ between processes.
 */
static void add_dynamic_environment( WCHAR **env, SIZE_T *pos, SIZE_T *size )
{
    const char *overrides = getenv( "WINEDLLOVERRIDES" );
    DWORD i;
    char str[22];

    add_path_var( env, pos, size, "WINEDATADIR", data_dir );
    add_path_var( env, pos, size, "WINEHOMEDIR", home_dir );
    add_path_var( env, pos, size, "WINEBUILDDIR", build_dir );
    add_path_var( env, pos, size, "WINECONFIGDIR", config_dir );
    for (i = 0; dll_paths[i]; i++)
    {
        sprintf( str, "WINEDLLDIR%u", i );
        add_path_var( env, pos, size, str, dll_paths[i] );
    }
    sprintf( str, "WINEDLLDIR%u", i );
    append_envW( env, pos, size, str, NULL );
    append_envA( env, pos, size, "WINEUSERNAME", user_name );
    append_envA( env, pos, size, "WINEDLLOVERRIDES", overrides );
    if (unix_cp.data)
    {
        sprintf( str, "%u", unix_cp.data[1] );
        append_envA( env, pos, size, "WINEUNIXCP", str );
    }
    else append_envW( env, pos, size, "WINEUNIXCP", NULL );
    append_envA( env, pos, size, "WINELOCALE", system_locale );
    append_envA( env, pos, size, "WINEUSERLOCALE",
                 strcmp( user_locale, system_locale ) ? user_locale : NULL );
    append_envA( env, pos, size, "SystemDrive", "C:" );
    append_envA( env, pos, size, "SystemRoot", "C:\\windows" );
}


static WCHAR *expand_value( WCHAR *env, SIZE_T size, const WCHAR *src, SIZE_T src_len )
{
    SIZE_T len, retlen = src_len, count = 0;
    const WCHAR *var;
    WCHAR *ret;

    ret = malloc( retlen * sizeof(WCHAR) );
    while (src_len)
    {
        if (*src != '%')
        {
            for (len = 0; len < src_len; len++) if (src[len] == '%') break;
            var = src;
            src += len;
            src_len -= len;
        }
        else  /* we are at the start of a variable */
        {
            for (len = 1; len < src_len; len++) if (src[len] == '%') break;
            if (len < src_len)
            {
                if ((var = find_env_var( env, size, src + 1, len - 1 )))
                {
                    src += len + 1;  /* skip the variable name */
                    src_len -= len + 1;
                    var += len;
                    len = wcslen(var);
                }
                else
                {
                    var = src;  /* copy original name instead */
                    len++;
                    src += len;
                    src_len -= len;
                }
            }
            else  /* unfinished variable name, ignore it */
            {
                var = src;
                src += len;
                src_len = 0;
            }
        }
        if (len >= retlen - count)
        {
            retlen *= 2;
            ret = realloc( ret, retlen * sizeof(WCHAR) );
        }
        memcpy( ret + count, var, len * sizeof(WCHAR) );
        count += len;
    }
    ret[count] = 0;
    return ret;
}

/***********************************************************************
 *           add_registry_variables
 *
 * Set environment variables by enumerating the values of a key;
 * helper for add_registry_environment().
 * Note that Windows happily truncates the value if it's too big.
 */
static void add_registry_variables( WCHAR **env, SIZE_T *pos, SIZE_T *size, HANDLE key )
{
    static const WCHAR pathW[] = {'P','A','T','H'};
    NTSTATUS status;
    DWORD index = 0, info_size, namelen, datalen;
    WCHAR *data, *value, *p;
    WCHAR buffer[offsetof(KEY_VALUE_FULL_INFORMATION, Name[1024]) / sizeof(WCHAR)];
    KEY_VALUE_FULL_INFORMATION *info = (KEY_VALUE_FULL_INFORMATION *)buffer;

    for (;;)
    {
        status = NtEnumerateValueKey( key, index++, KeyValueFullInformation,
                                      buffer, sizeof(buffer) - sizeof(WCHAR), &info_size );
        if (status != STATUS_SUCCESS && status != STATUS_BUFFER_OVERFLOW) break;

        value = data = buffer + info->DataOffset / sizeof(WCHAR);
        datalen = info->DataLength / sizeof(WCHAR);
        namelen = info->NameLength / sizeof(WCHAR);

        if (datalen && !data[datalen - 1]) datalen--;  /* don't count terminating null if any */
        if (!datalen) continue;
        data[datalen] = 0;
        if (info->Type == REG_EXPAND_SZ) value = expand_value( *env, *pos, data, datalen );

        /* PATH is magic */
        if (namelen == 4 && !wcsnicmp( info->Name, pathW, 4 ) && (p = find_env_var( *env, *pos, pathW, 4 )))
        {
            static const WCHAR sepW[] = {';',0};
            WCHAR *newpath = malloc( (wcslen(p) - 3 + wcslen(value)) * sizeof(WCHAR) );
            wcscpy( newpath, p + 5 );
            wcscat( newpath, sepW );
            wcscat( newpath, value );
            if (value != data) free( value );
            value = newpath;
        }

        set_env_var( env, pos, size, info->Name, namelen, value );
        if (value != data) free( value );
    }
}


/***********************************************************************
 *           get_registry_value
 */
static WCHAR *get_registry_value( WCHAR *env, SIZE_T pos, HKEY key, const WCHAR *name )
{
    WCHAR buffer[offsetof(KEY_VALUE_PARTIAL_INFORMATION, Data[1024 * sizeof(WCHAR)])];
    KEY_VALUE_PARTIAL_INFORMATION *info = (KEY_VALUE_PARTIAL_INFORMATION *)buffer;
    DWORD len, size = sizeof(buffer) - sizeof(WCHAR);
    WCHAR *ret = NULL;
    UNICODE_STRING nameW;

    init_unicode_string( &nameW, name );
    if (NtQueryValueKey( key, &nameW, KeyValuePartialInformation, buffer, size, &size )) return NULL;
    if (size <= offsetof( KEY_VALUE_PARTIAL_INFORMATION, Data )) return NULL;
    len = size - offsetof( KEY_VALUE_PARTIAL_INFORMATION, Data );

    if (info->Type == REG_EXPAND_SZ)
    {
        ret = expand_value( env, pos, (WCHAR *)info->Data, len / sizeof(WCHAR) );
    }
    else
    {
        ret = malloc( len + sizeof(WCHAR) );
        memcpy( ret, info->Data, len );
        ret[len / sizeof(WCHAR)] = 0;
    }
    return ret;
}


/***********************************************************************
 *           add_registry_environment
 *
 * Set the environment variables specified in the registry.
 */
static void add_registry_environment( WCHAR **env, SIZE_T *pos, SIZE_T *size )
{
    static const WCHAR syskeyW[] = {'\\','R','e','g','i','s','t','r','y',
        '\\','M','a','c','h','i','n','e',
        '\\','S','y','s','t','e','m',
        '\\','C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t',
        '\\','C','o','n','t','r','o','l',
        '\\','S','e','s','s','i','o','n',' ','M','a','n','a','g','e','r',
        '\\','E','n','v','i','r','o','n','m','e','n','t',0};
    static const WCHAR profileW[] = {'\\','R','e','g','i','s','t','r','y',
        '\\','M','a','c','h','i','n','e','\\',
        'S','o','f','t','w','a','r','e','\\',
        'M','i','c','r','o','s','o','f','t','\\',
        'W','i','n','d','o','w','s',' ','N','T','\\',
        'C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\',
        'P','r','o','f','i','l','e','L','i','s','t',0};
    static const WCHAR computerW[] = {'\\','R','e','g','i','s','t','r','y',
        '\\','M','a','c','h','i','n','e',
        '\\','S','y','s','t','e','m',
        '\\','C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t',
        '\\','C','o','n','t','r','o','l',
        '\\','C','o','m','p','u','t','e','r','N','a','m','e',
        '\\','A','c','t','i','v','e','C','o','m','p','u','t','e','r','N','a','m','e',0};
    static const WCHAR curversionW[] = {'\\','R','e','g','i','s','t','r','y',
        '\\','M','a','c','h','i','n','e',
        '\\','S','o','f','t','w','a','r','e',
        '\\','M','i','c','r','o','s','o','f','t',
        '\\','W','i','n','d','o','w','s',
        '\\','C','u','r','r','e','n','t','V','e','r','s','i','o','n',0};
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING nameW;
    WCHAR *value;
    HANDLE key;

    InitializeObjectAttributes( &attr, &nameW, 0, 0, NULL );
    init_unicode_string( &nameW, syskeyW );
    if (!NtOpenKey( &key, KEY_READ, &attr ))
    {
        add_registry_variables( env, pos, size, key );
        NtClose( key );
    }
    if (!open_hkcu_key( "Environment", &key ))
    {
        add_registry_variables( env, pos, size, key );
        NtClose( key );
    }
    if (!open_hkcu_key( "Volatile Environment", &key ))
    {
        add_registry_variables( env, pos, size, key );
        NtClose( key );
    }

    /* set the user profile variables */
    init_unicode_string( &nameW, profileW );
    if (!NtOpenKey( &key, KEY_READ, &attr ))
    {
        static const WCHAR progdataW[] = {'P','r','o','g','r','a','m','D','a','t','a',0};
        static const WCHAR allusersW[] = {'A','L','L','U','S','E','R','S','P','R','O','F','I','L','E',0};
        static const WCHAR publicW[] = {'P','U','B','L','I','C',0};
        if ((value = get_registry_value( *env, *pos, key, progdataW )))
        {
            set_env_var( env, pos, size, allusersW, wcslen(allusersW), value );
            set_env_var( env, pos, size, progdataW, wcslen(progdataW), value );
            free( value );
        }
        if ((value = get_registry_value( *env, *pos, key, publicW )))
        {
            set_env_var( env, pos, size, publicW, wcslen(publicW), value );
            free( value );
        }
        NtClose( key );
    }

    /* set the ProgramFiles variables */
    init_unicode_string( &nameW, curversionW );
    if (!NtOpenKey( &key, KEY_READ | KEY_WOW64_64KEY, &attr ))
    {
        static const WCHAR progdirW[] = {'P','r','o','g','r','a','m','F','i','l','e','s','D','i','r',0};
        static const WCHAR progdirx86W[] = {'P','r','o','g','r','a','m','F','i','l','e','s','D','i','r',' ','(','x','8','6',')',0};
        static const WCHAR progfilesW[] = {'P','r','o','g','r','a','m','F','i','l','e','s',0};
        static const WCHAR prog6432W[] = {'P','r','o','g','r','a','m','W','6','4','3','2',0};
        static const WCHAR progx86W[] = {'P','r','o','g','r','a','m','F','i','l','e','s','(','x','8','6',')',0};
        static const WCHAR commondirW[] = {'C','o','m','m','o','n','F','i','l','e','s','D','i','r',0};
        static const WCHAR commondirx86W[] = {'C','o','m','m','o','n','F','i','l','e','s','D','i','r',' ','(','x','8','6',')',0};
        static const WCHAR commonfilesW[] = {'C','o','m','m','o','n','P','r','o','g','r','a','m','F','i','l','e','s',0};
        static const WCHAR common6432W[] = {'C','o','m','m','o','n','P','r','o','g','r','a','m','W','6','4','3','2',0};
        static const WCHAR commonx86W[] = {'C','o','m','m','o','n','P','r','o','g','r','a','m','F','i','l','e','s','(','x','8','6',')',0};

        if ((value = get_registry_value( *env, *pos, key, progdirx86W )))
        {
            set_env_var( env, pos, size, progx86W, wcslen(progx86W), value );
            free( value );
            if ((value = get_registry_value( *env, *pos, key, progdirW )))
                set_env_var( env, pos, size, prog6432W, wcslen(prog6432W), value );
        }
        else
        {
            if ((value = get_registry_value( *env, *pos, key, progdirW )))
                set_env_var( env, pos, size, progfilesW, wcslen(progfilesW), value );
        }
        free( value );

        if ((value = get_registry_value( *env, *pos, key, commondirx86W )))
        {
            set_env_var( env, pos, size, commonx86W, wcslen(commonx86W), value );
            free( value );
            if ((value = get_registry_value( *env, *pos, key, commondirW )))
                set_env_var( env, pos, size, common6432W, wcslen(common6432W), value );
        }
        else
        {
            if ((value = get_registry_value( *env, *pos, key, commondirW )))
                set_env_var( env, pos, size, commonfilesW, wcslen(commonfilesW), value );
        }
        free( value );
        NtClose( key );
    }

    /* set the computer name */
    init_unicode_string( &nameW, computerW );
    if (!NtOpenKey( &key, KEY_READ, &attr ))
    {
        static const WCHAR computernameW[] = {'C','O','M','P','U','T','E','R','N','A','M','E',0};
        if ((value = get_registry_value( *env, *pos, key, computernameW )))
        {
            set_env_var( env, pos, size, computernameW, wcslen(computernameW), value );
            free( value );
        }
        NtClose( key );
    }
}


/*************************************************************************
 *		get_initial_console
 *
 * Return the initial console handles.
 */
static void get_initial_console( RTL_USER_PROCESS_PARAMETERS *params )
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
static WCHAR *get_initial_directory(void)
{
    static const WCHAR backslashW[] = {'\\',0};
    static const WCHAR windows_dir[] = {'\\','?','?','\\','C',':','\\','w','i','n','d','o','w','s','\\',0};
    const char *pwd;
    char *cwd;
    int size;
    WCHAR *ret = NULL;

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
        if (!unix_to_nt_file_name( pwd, &ret ))
        {
            ULONG len = wcslen( ret );
            if (len && ret[len - 1] != '\\')
            {
                /* add trailing backslash */
                WCHAR *tmp = malloc( (len + 2) * sizeof(WCHAR) );
                wcscpy( tmp, ret );
                wcscat( tmp, backslashW );
                free( ret );
                ret = tmp;
            }
            free( cwd );
            return ret;
        }
    }

    /* still not initialized */
    MESSAGE("Warning: could not find DOS drive for current working directory '%s', "
            "starting in the Windows directory.\n", cwd ? cwd : "" );
    ret = malloc( sizeof(windows_dir) );
    wcscpy( ret, windows_dir );
    free( cwd );
    return ret;
}


/***********************************************************************
 *           build_command_line
 *
 * Build the command line of a process from the argv array.
 *
 * We must quote and escape characters so that the argv array can be rebuilt
 * from the command line:
 * - spaces and tabs must be quoted
 *   'a b'   -> '"a b"'
 * - quotes must be escaped
 *   '"'     -> '\"'
 * - if '\'s are followed by a '"', they must be doubled and followed by '\"',
 *   resulting in an odd number of '\' followed by a '"'
 *   '\"'    -> '\\\"'
 *   '\\"'   -> '\\\\\"'
 * - '\'s are followed by the closing '"' must be doubled,
 *   resulting in an even number of '\' followed by a '"'
 *   ' \'    -> '" \\"'
 *   ' \\'    -> '" \\\\"'
 * - '\'s that are not followed by a '"' can be left as is
 *   'a\b'   == 'a\b'
 *   'a\\b'  == 'a\\b'
 */
static WCHAR *build_command_line( WCHAR **wargv )
{
    int len;
    WCHAR **arg, *ret;
    LPWSTR p;

    len = 1;
    for (arg = wargv; *arg; arg++) len += 3 + 2 * wcslen( *arg );
    if (!(ret = malloc( len * sizeof(WCHAR) ))) return NULL;

    p = ret;
    for (arg = wargv; *arg; arg++)
    {
        BOOL has_space, has_quote;
        int i, bcount;
        WCHAR *a;

        /* check for quotes and spaces in this argument (first arg is always quoted) */
        has_space = (arg == wargv) || !**arg || wcschr( *arg, ' ' ) || wcschr( *arg, '\t' );
        has_quote = wcschr( *arg, '"' ) != NULL;

        /* now transfer it to the command line */
        if (has_space) *p++ = '"';
        if (has_quote || has_space)
        {
            bcount = 0;
            for (a = *arg; *a; a++)
            {
                if (*a == '\\') bcount++;
                else
                {
                    if (*a == '"') /* double all the '\\' preceding this '"', plus one */
                        for (i = 0; i <= bcount; i++) *p++ = '\\';
                    bcount = 0;
                }
                *p++ = *a;
            }
        }
        else
        {
            wcscpy( p, *arg );
            p += wcslen( p );
        }
        if (has_space)
        {
            /* Double all the '\' preceding the closing quote */
            for (i = 0; i < bcount; i++) *p++ = '\\';
            *p++ = '"';
        }
        *p++ = ' ';
    }
    if (p > ret) p--;  /* remove last space */
    *p = 0;
    if (p - ret >= 32767)
    {
        ERR( "command line too long (%u)\n", (DWORD)(p - ret) );
        NtTerminateProcess( GetCurrentProcess(), 1 );
    }
    return ret;
}


/***********************************************************************
 *           run_wineboot
 */
static void run_wineboot( WCHAR *env, SIZE_T size )
{
    static const WCHAR eventW[] = {'\\','K','e','r','n','e','l','O','b','j','e','c','t','s',
        '\\','_','_','w','i','n','e','b','o','o','t','_','e','v','e','n','t',0};
    static const WCHAR appnameW[] = {'\\','?','?','\\','C',':','\\','w','i','n','d','o','w','s',
        '\\','s','y','s','t','e','m','3','2','\\','w','i','n','e','b','o','o','t','.','e','x','e',0};
    static const WCHAR cmdlineW[] = {'"','C',':','\\','w','i','n','d','o','w','s','\\',
        's','y','s','t','e','m','3','2','\\','w','i','n','e','b','o','o','t','.','e','x','e','"',
        ' ','-','-','i','n','i','t',0};
    RTL_USER_PROCESS_PARAMETERS params = { sizeof(params), sizeof(params) };
    PS_ATTRIBUTE_LIST ps_attr;
    PS_CREATE_INFO create_info;
    HANDLE process, thread, handles[2];
    UNICODE_STRING nameW;
    OBJECT_ATTRIBUTES attr;
    LARGE_INTEGER timeout;
    NTSTATUS status;
    int count = 1;

    init_unicode_string( &nameW, eventW );
    InitializeObjectAttributes( &attr, &nameW, OBJ_OPENIF, 0, NULL );
    status = NtCreateEvent( &handles[0], EVENT_ALL_ACCESS, &attr, NotificationEvent, 0 );
    if (status == STATUS_OBJECT_NAME_EXISTS) goto wait;
    if (status)
    {
        ERR( "failed to create wineboot event, expect trouble\n" );
        return;
    }

    env[size] = 0;
    params.Flags           = PROCESS_PARAMS_FLAG_NORMALIZED;
    params.Environment     = env;
    params.EnvironmentSize = size;
    init_unicode_string( &params.CurrentDirectory.DosPath, system_dir + 4 );
    init_unicode_string( &params.ImagePathName, appnameW + 4 );
    init_unicode_string( &params.CommandLine, cmdlineW );
    init_unicode_string( &params.WindowTitle, appnameW + 4 );
    init_unicode_string( &nameW, appnameW );

    ps_attr.TotalLength = sizeof(ps_attr);
    ps_attr.Attributes[0].Attribute    = PS_ATTRIBUTE_IMAGE_NAME;
    ps_attr.Attributes[0].Size         = sizeof(appnameW) - sizeof(WCHAR);
    ps_attr.Attributes[0].ValuePtr     = (WCHAR *)appnameW;
    ps_attr.Attributes[0].ReturnLength = NULL;

    wine_server_fd_to_handle( 2, GENERIC_WRITE | SYNCHRONIZE, OBJ_INHERIT, &params.hStdError );

    if (NtCurrentTeb64() && !NtCurrentTeb64()->TlsSlots[WOW64_TLS_FILESYSREDIR])
    {
        NtCurrentTeb64()->TlsSlots[WOW64_TLS_FILESYSREDIR] = TRUE;
        status = NtCreateUserProcess( &process, &thread, PROCESS_ALL_ACCESS, THREAD_ALL_ACCESS,
                                      NULL, NULL, 0, THREAD_CREATE_FLAGS_CREATE_SUSPENDED, &params,
                                      &create_info, &ps_attr );
        NtCurrentTeb64()->TlsSlots[WOW64_TLS_FILESYSREDIR] = FALSE;
    }
    else
        status = NtCreateUserProcess( &process, &thread, PROCESS_ALL_ACCESS, THREAD_ALL_ACCESS,
                                      NULL, NULL, 0, THREAD_CREATE_FLAGS_CREATE_SUSPENDED, &params,
                                      &create_info, &ps_attr );
    NtClose( params.hStdError );

    if (status)
    {
        ERR( "failed to start wineboot %x\n", status );
        NtClose( handles[0] );
        return;
    }
    NtResumeThread( thread, NULL );
    NtClose( thread );
    handles[count++] = process;

wait:
    timeout.QuadPart = (ULONGLONG)5 * 60 * 1000 * -10000;
    if (NtWaitForMultipleObjects( count, handles, TRUE, FALSE, &timeout ) == WAIT_TIMEOUT)
        ERR( "boot event wait timed out\n" );
    while (count) NtClose( handles[--count] );
}


static inline void copy_unicode_string( WCHAR **src, WCHAR **dst, UNICODE_STRING *str, UINT len )
{
    str->Buffer = *dst;
    str->Length = len;
    str->MaximumLength = len + sizeof(WCHAR);
    memcpy( *dst, *src, len );
    (*dst)[len / sizeof(WCHAR)] = 0;
    *src += len / sizeof(WCHAR);
    *dst += len / sizeof(WCHAR) + 1;
}

static inline void put_unicode_string( WCHAR *src, WCHAR **dst, UNICODE_STRING *str )
{
    copy_unicode_string( &src, dst, str, wcslen(src) * sizeof(WCHAR) );
}

static inline WCHAR *get_dos_path( WCHAR *nt_path )
{
    if (nt_path[4] && nt_path[5] == ':') return nt_path + 4; /* skip the \??\ prefix */
    nt_path[1] = '\\'; /* change \??\ to \\?\ */
    return nt_path;
}

static inline const WCHAR *get_params_string( const RTL_USER_PROCESS_PARAMETERS *params,
                                              const UNICODE_STRING *str )
{
    if (params->Flags & PROCESS_PARAMS_FLAG_NORMALIZED) return str->Buffer;
    return (const WCHAR *)((const char *)params + (UINT_PTR)str->Buffer);
}

static inline DWORD append_string( void **ptr, const RTL_USER_PROCESS_PARAMETERS *params,
                                   const UNICODE_STRING *str )
{
    const WCHAR *buffer = get_params_string( params, str );
    memcpy( *ptr, buffer, str->Length );
    *ptr = (WCHAR *)*ptr + str->Length / sizeof(WCHAR);
    return str->Length;
}

#ifdef _WIN64
static inline void dup_unicode_string( const UNICODE_STRING *src, WCHAR **dst, UNICODE_STRING32 *str )
#else
static inline void dup_unicode_string( const UNICODE_STRING *src, WCHAR **dst, UNICODE_STRING64 *str )
#endif
{
    if (!src->Buffer) return;
    str->Buffer = PtrToUlong( *dst );
    str->Length = src->Length;
    str->MaximumLength = src->MaximumLength;
    memcpy( *dst, src->Buffer, src->MaximumLength );
    *dst += src->MaximumLength / sizeof(WCHAR);
}


/*************************************************************************
 *		build_wow64_parameters
 */
static void *build_wow64_parameters( const RTL_USER_PROCESS_PARAMETERS *params )
{
#ifdef _WIN64
    RTL_USER_PROCESS_PARAMETERS32 *wow64_params = NULL;
#else
    RTL_USER_PROCESS_PARAMETERS64 *wow64_params = NULL;
#endif
    NTSTATUS status;
    WCHAR *dst;
    SIZE_T size = (sizeof(*wow64_params)
                   + params->CurrentDirectory.DosPath.MaximumLength
                   + params->DllPath.MaximumLength
                   + params->ImagePathName.MaximumLength
                   + params->CommandLine.MaximumLength
                   + params->WindowTitle.MaximumLength
                   + params->Desktop.MaximumLength
                   + params->ShellInfo.MaximumLength
                   + params->RuntimeInfo.MaximumLength
                   + params->EnvironmentSize);

    status = NtAllocateVirtualMemory( NtCurrentProcess(), (void **)&wow64_params, 0, &size,
                                      MEM_COMMIT, PAGE_READWRITE );
    assert( !status );

    wow64_params->AllocationSize  = size;
    wow64_params->Size            = size;
    wow64_params->Flags           = params->Flags;
    wow64_params->DebugFlags      = params->DebugFlags;
    wow64_params->ConsoleHandle   = HandleToULong( params->ConsoleHandle );
    wow64_params->ConsoleFlags    = params->ConsoleFlags;
    wow64_params->hStdInput       = HandleToULong( params->hStdInput );
    wow64_params->hStdOutput      = HandleToULong( params->hStdOutput );
    wow64_params->hStdError       = HandleToULong( params->hStdError );
    wow64_params->dwX             = params->dwX;
    wow64_params->dwY             = params->dwY;
    wow64_params->dwXSize         = params->dwXSize;
    wow64_params->dwYSize         = params->dwYSize;
    wow64_params->dwXCountChars   = params->dwXCountChars;
    wow64_params->dwYCountChars   = params->dwYCountChars;
    wow64_params->dwFillAttribute = params->dwFillAttribute;
    wow64_params->dwFlags         = params->dwFlags;
    wow64_params->wShowWindow     = params->wShowWindow;

    dst = (WCHAR *)(wow64_params + 1);
    dup_unicode_string( &params->CurrentDirectory.DosPath, &dst, &wow64_params->CurrentDirectory.DosPath );
    dup_unicode_string( &params->DllPath, &dst, &wow64_params->DllPath );
    dup_unicode_string( &params->ImagePathName, &dst, &wow64_params->ImagePathName );
    dup_unicode_string( &params->CommandLine, &dst, &wow64_params->CommandLine );
    dup_unicode_string( &params->WindowTitle, &dst, &wow64_params->WindowTitle );
    dup_unicode_string( &params->Desktop, &dst, &wow64_params->Desktop );
    dup_unicode_string( &params->ShellInfo, &dst, &wow64_params->ShellInfo );
    dup_unicode_string( &params->RuntimeInfo, &dst, &wow64_params->RuntimeInfo );

    wow64_params->Environment = PtrToUlong( dst );
    wow64_params->EnvironmentSize = params->EnvironmentSize;
    memcpy( dst, params->Environment, params->EnvironmentSize );
    return wow64_params;
}


/*************************************************************************
 *		init_peb
 */
static void init_peb( RTL_USER_PROCESS_PARAMETERS *params, void *module )
{
    peb->ImageBaseAddress           = module;
    peb->ProcessParameters          = params;
    peb->OSMajorVersion             = 6;
    peb->OSMinorVersion             = 1;
    peb->OSBuildNumber              = 0x1db1;
    peb->OSPlatformId               = VER_PLATFORM_WIN32_NT;
    peb->ImageSubSystem             = main_image_info.SubSystemType;
    peb->ImageSubSystemMajorVersion = main_image_info.MajorSubsystemVersion;
    peb->ImageSubSystemMinorVersion = main_image_info.MinorSubsystemVersion;

    if (NtCurrentTeb()->WowTebOffset)
    {
        void *wow64_params = build_wow64_parameters( params );
#ifdef _WIN64
        PEB32 *wow64_peb = (PEB32 *)((char *)peb + page_size);
#else
        PEB64 *wow64_peb = (PEB64 *)((char *)peb - page_size);
#endif
        wow64_peb->ImageBaseAddress           = PtrToUlong( peb->ImageBaseAddress );
        wow64_peb->ProcessParameters          = PtrToUlong( wow64_params );
        wow64_peb->NumberOfProcessors         = peb->NumberOfProcessors;
        wow64_peb->OSMajorVersion             = peb->OSMajorVersion;
        wow64_peb->OSMinorVersion             = peb->OSMinorVersion;
        wow64_peb->OSBuildNumber              = peb->OSBuildNumber;
        wow64_peb->OSPlatformId               = peb->OSPlatformId;
        wow64_peb->ImageSubSystem             = peb->ImageSubSystem;
        wow64_peb->ImageSubSystemMajorVersion = peb->ImageSubSystemMajorVersion;
        wow64_peb->ImageSubSystemMinorVersion = peb->ImageSubSystemMinorVersion;
        wow64_peb->SessionId                  = peb->SessionId;
    }
}


/*************************************************************************
 *		build_initial_params
 *
 * Build process parameters from scratch, for processes without a parent.
 */
static RTL_USER_PROCESS_PARAMETERS *build_initial_params( void **module )
{
    static const WCHAR valueW[] = {'1',0};
    static const WCHAR pathW[] = {'P','A','T','H'};
    RTL_USER_PROCESS_PARAMETERS *params = NULL;
    SIZE_T size, env_pos, env_size;
    WCHAR *dst, *image, *cmdline, *path, *bootstrap;
    WCHAR *env = get_initial_environment( &env_pos, &env_size );
    WCHAR *curdir = get_initial_directory();
    NTSTATUS status;

    /* store the initial PATH value */
    path = get_env_var( env, env_pos, pathW, 4 );
    add_dynamic_environment( &env, &env_pos, &env_size );
    add_registry_environment( &env, &env_pos, &env_size );
    bootstrap = get_env_var( env, env_pos, bootstrapW, ARRAY_SIZE(bootstrapW) );
    set_env_var( &env, &env_pos, &env_size, bootstrapW, ARRAY_SIZE(bootstrapW), valueW );
    is_prefix_bootstrap = TRUE;
    env[env_pos] = 0;
    run_wineboot( env, env_pos );

    /* reload environment now that wineboot has run */
    set_env_var( &env, &env_pos, &env_size, pathW, 4, path );  /* reset PATH */
    free( path );
    set_env_var( &env, &env_pos, &env_size, bootstrapW, ARRAY_SIZE(bootstrapW), bootstrap );
    is_prefix_bootstrap = !!bootstrap;
    free( bootstrap );
    add_registry_environment( &env, &env_pos, &env_size );
    env[env_pos++] = 0;

    status = load_main_exe( NULL, main_argv[1], curdir, &image, module );
    if (!status)
    {
        if (main_image_info.ImageCharacteristics & IMAGE_FILE_DLL) status = STATUS_INVALID_IMAGE_FORMAT;
        if (main_image_info.ImageFlags & IMAGE_FLAGS_ComPlusNativeReady)
            main_image_info.Machine = native_machine;
        if (main_image_info.Machine != current_machine) status = STATUS_INVALID_IMAGE_FORMAT;
    }

    if (status)  /* try launching it through start.exe */
    {
        static const char *args[] = { "start.exe", "/exec" };
        free( image );
        if (*module) NtUnmapViewOfSection( GetCurrentProcess(), *module );
        load_start_exe( &image, module );
        prepend_argv( args, 2 );
    }
    else rebuild_argv();

    main_wargv = build_wargv( get_dos_path( image ));
    cmdline = build_command_line( main_wargv );

    TRACE( "image %s cmdline %s dir %s\n",
           debugstr_w(main_wargv[0]), debugstr_w(cmdline), debugstr_w(curdir) );

    size = (sizeof(*params)
            + MAX_PATH * sizeof(WCHAR)  /* curdir */
            + (wcslen( cmdline ) + 1) * sizeof(WCHAR)  /* command line */
            + (wcslen( main_wargv[0] ) + 1) * sizeof(WCHAR) * 2 /* image path + window title */
            + env_pos * sizeof(WCHAR));

    status = NtAllocateVirtualMemory( NtCurrentProcess(), (void **)&params, 0, &size,
                                      MEM_COMMIT, PAGE_READWRITE );
    assert( !status );

    params->AllocationSize  = size;
    params->Size            = size;
    params->Flags           = PROCESS_PARAMS_FLAG_NORMALIZED;
    params->wShowWindow     = 1; /* SW_SHOWNORMAL */

    params->CurrentDirectory.DosPath.Buffer = (WCHAR *)(params + 1);
    wcscpy( params->CurrentDirectory.DosPath.Buffer, get_dos_path( curdir ));
    params->CurrentDirectory.DosPath.Length = wcslen(params->CurrentDirectory.DosPath.Buffer) * sizeof(WCHAR);
    params->CurrentDirectory.DosPath.MaximumLength = MAX_PATH * sizeof(WCHAR);
    dst = params->CurrentDirectory.DosPath.Buffer + MAX_PATH;

    put_unicode_string( main_wargv[0], &dst, &params->ImagePathName );
    put_unicode_string( cmdline, &dst, &params->CommandLine );
    put_unicode_string( main_wargv[0], &dst, &params->WindowTitle );
    free( image );
    free( cmdline );
    free( curdir );

    params->Environment = dst;
    params->EnvironmentSize = env_pos * sizeof(WCHAR);
    memcpy( dst, env, env_pos * sizeof(WCHAR) );
    free( env );

    get_initial_console( params );

    return params;
}


/*************************************************************************
 *		init_startup_info
 */
void init_startup_info(void)
{
    WCHAR *src, *dst, *env, *image;
    void *module = NULL;
    NTSTATUS status;
    SIZE_T size, info_size, env_size, env_pos;
    RTL_USER_PROCESS_PARAMETERS *params = NULL;
    startup_info_t *info;

    if (!startup_info_size)
    {
        params = build_initial_params( &module );
        init_peb( params, module );
        return;
    }

    info = malloc( startup_info_size );

    SERVER_START_REQ( get_startup_info )
    {
        wine_server_set_reply( req, info, startup_info_size );
        status = wine_server_call( req );
        info_size = reply->info_size;
        env_size = (wine_server_reply_size( reply ) - info_size) / sizeof(WCHAR);
    }
    SERVER_END_REQ;
    assert( !status );

    env = malloc( env_size * sizeof(WCHAR) );
    memcpy( env, (char *)info + info_size, env_size * sizeof(WCHAR) );
    env_pos = env_size - 1;
    add_dynamic_environment( &env, &env_pos, &env_size );
    is_prefix_bootstrap = !!find_env_var( env, env_pos, bootstrapW, ARRAY_SIZE(bootstrapW) );
    env[env_pos++] = 0;

    size = (sizeof(*params)
            + MAX_PATH * sizeof(WCHAR)  /* curdir */
            + info->dllpath_len + sizeof(WCHAR)
            + info->imagepath_len + sizeof(WCHAR)
            + info->cmdline_len + sizeof(WCHAR)
            + info->title_len + sizeof(WCHAR)
            + info->desktop_len + sizeof(WCHAR)
            + info->shellinfo_len + sizeof(WCHAR)
            + info->runtime_len + sizeof(WCHAR)
            + env_pos * sizeof(WCHAR));

    status = NtAllocateVirtualMemory( NtCurrentProcess(), (void **)&params, 0, &size,
                                      MEM_COMMIT, PAGE_READWRITE );
    assert( !status );

    params->AllocationSize  = size;
    params->Size            = size;
    params->Flags           = PROCESS_PARAMS_FLAG_NORMALIZED;
    params->DebugFlags      = info->debug_flags;
    params->ConsoleHandle   = wine_server_ptr_handle( info->console );
    params->ConsoleFlags    = info->console_flags;
    params->hStdInput       = wine_server_ptr_handle( info->hstdin );
    params->hStdOutput      = wine_server_ptr_handle( info->hstdout );
    params->hStdError       = wine_server_ptr_handle( info->hstderr );
    params->dwX             = info->x;
    params->dwY             = info->y;
    params->dwXSize         = info->xsize;
    params->dwYSize         = info->ysize;
    params->dwXCountChars   = info->xchars;
    params->dwYCountChars   = info->ychars;
    params->dwFillAttribute = info->attribute;
    params->dwFlags         = info->flags;
    params->wShowWindow     = info->show;

    src = (WCHAR *)(info + 1);
    dst = (WCHAR *)(params + 1);

    /* curdir is special */
    copy_unicode_string( &src, &dst, &params->CurrentDirectory.DosPath, info->curdir_len );
    params->CurrentDirectory.DosPath.MaximumLength = MAX_PATH * sizeof(WCHAR);
    dst = params->CurrentDirectory.DosPath.Buffer + MAX_PATH;

    if (info->dllpath_len) copy_unicode_string( &src, &dst, &params->DllPath, info->dllpath_len );
    copy_unicode_string( &src, &dst, &params->ImagePathName, info->imagepath_len );
    copy_unicode_string( &src, &dst, &params->CommandLine, info->cmdline_len );
    copy_unicode_string( &src, &dst, &params->WindowTitle, info->title_len );
    copy_unicode_string( &src, &dst, &params->Desktop, info->desktop_len );
    copy_unicode_string( &src, &dst, &params->ShellInfo, info->shellinfo_len );
    if (info->runtime_len)
    {
        /* runtime info isn't a real string */
        params->RuntimeInfo.MaximumLength = params->RuntimeInfo.Length = info->runtime_len;
        params->RuntimeInfo.Buffer = dst;
        memcpy( dst, src, info->runtime_len );
        src += (info->runtime_len + 1) / sizeof(WCHAR);
        dst += (info->runtime_len + 1) / sizeof(WCHAR);
    }
    assert( (char *)src == (char *)info + info_size );

    params->Environment = dst;
    params->EnvironmentSize = env_pos * sizeof(WCHAR);
    memcpy( dst, env, env_pos * sizeof(WCHAR) );
    free( env );
    free( info );

    status = load_main_exe( params->ImagePathName.Buffer, NULL,
                            params->CommandLine.Buffer, &image, &module );
    if (status)
    {
        MESSAGE( "wine: failed to start %s\n", debugstr_us(&params->ImagePathName) );
        NtTerminateProcess( GetCurrentProcess(), status );
    }
    rebuild_argv();
    main_wargv = build_wargv( get_dos_path( image ));
    free( image );
    init_peb( params, module );
}


/***********************************************************************
 *           create_startup_info
 */
void *create_startup_info( const UNICODE_STRING *nt_image, const RTL_USER_PROCESS_PARAMETERS *params,
                           DWORD *info_size )
{
    startup_info_t *info;
    UNICODE_STRING dos_image = *nt_image;
    DWORD size;
    void *ptr;

    dos_image.Buffer = get_dos_path( nt_image->Buffer );
    dos_image.Length = nt_image->Length - (dos_image.Buffer - nt_image->Buffer) * sizeof(WCHAR);

    size = sizeof(*info);
    size += params->CurrentDirectory.DosPath.Length;
    size += params->DllPath.Length;
    size += dos_image.Length;
    size += params->CommandLine.Length;
    size += params->WindowTitle.Length;
    size += params->Desktop.Length;
    size += params->ShellInfo.Length;
    size += params->RuntimeInfo.Length;
    size = (size + 1) & ~1;
    *info_size = size;

    if (!(info = calloc( size, 1 ))) return NULL;

    info->debug_flags   = params->DebugFlags;
    info->console_flags = params->ConsoleFlags;
    info->console       = wine_server_obj_handle( params->ConsoleHandle );
    info->hstdin        = wine_server_obj_handle( params->hStdInput );
    info->hstdout       = wine_server_obj_handle( params->hStdOutput );
    info->hstderr       = wine_server_obj_handle( params->hStdError );
    info->x             = params->dwX;
    info->y             = params->dwY;
    info->xsize         = params->dwXSize;
    info->ysize         = params->dwYSize;
    info->xchars        = params->dwXCountChars;
    info->ychars        = params->dwYCountChars;
    info->attribute     = params->dwFillAttribute;
    info->flags         = params->dwFlags;
    info->show          = params->wShowWindow;

    ptr = info + 1;
    info->curdir_len = append_string( &ptr, params, &params->CurrentDirectory.DosPath );
    info->dllpath_len = append_string( &ptr, params, &params->DllPath );
    info->imagepath_len = append_string( &ptr, params, &dos_image );
    info->cmdline_len = append_string( &ptr, params, &params->CommandLine );
    info->title_len = append_string( &ptr, params, &params->WindowTitle );
    info->desktop_len = append_string( &ptr, params, &params->Desktop );
    info->shellinfo_len = append_string( &ptr, params, &params->ShellInfo );
    info->runtime_len = append_string( &ptr, params, &params->RuntimeInfo );
    return info;
}


/**************************************************************************
 *      NtGetNlsSectionPtr  (NTDLL.@)
 */
NTSTATUS WINAPI NtGetNlsSectionPtr( ULONG type, ULONG id, void *unknown, void **ptr, SIZE_T *size )
{
    UNICODE_STRING nameW;
    OBJECT_ATTRIBUTES attr;
    WCHAR name[32];
    HANDLE handle, file;
    NTSTATUS status;

    if ((status = get_nls_section_name( type, id, name ))) return status;

    init_unicode_string( &nameW, name );
    InitializeObjectAttributes( &attr, &nameW, 0, 0, NULL );
    if ((status = NtOpenSection( &handle, SECTION_MAP_READ, &attr )))
    {
        if ((status = open_nls_data_file( type, id, &file ))) return status;
        attr.Attributes = OBJ_OPENIF | OBJ_PERMANENT;
        status = NtCreateSection( &handle, SECTION_MAP_READ, &attr, NULL, PAGE_READONLY, SEC_COMMIT, file );
        NtClose( file );
        if (status == STATUS_OBJECT_NAME_EXISTS) status = STATUS_SUCCESS;
    }
    if (!status)
    {
        *ptr = NULL;
        *size = 0;
        status = NtMapViewOfSection( handle, GetCurrentProcess(), ptr, 0, 0, NULL, size,
                                     ViewShare, 0, PAGE_READONLY );
    }
    NtClose( handle );
    return status;
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
