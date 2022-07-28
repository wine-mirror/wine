/*
 * Unicode routines for use inside the server
 *
 * Copyright (C) 1999 Alexandre Julliard
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

#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <limits.h>
#ifdef HAVE_SYS_SYSCTL_H
# include <sys/sysctl.h>
#endif

#include "windef.h"
#include "winternl.h"
#include "request.h"
#include "unicode.h"
#include "file.h"

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

/* minimum Unicode value depending on UTF-8 sequence length */
static const unsigned int utf8_minval[4] = { 0x0, 0x80, 0x800, 0x10000 };

static unsigned short *casemap;

static inline char to_hex( char ch )
{
    if (isdigit(ch)) return ch - '0';
    return tolower(ch) - 'a' + 10;
}

static inline WCHAR to_lower( WCHAR ch )
{
    return ch + casemap[casemap[casemap[ch >> 8] + ((ch >> 4) & 0x0f)] + (ch & 0x0f)];
}

int memicmp_strW( const WCHAR *str1, const WCHAR *str2, data_size_t len )
{
    int ret = 0;

    for (len /= sizeof(WCHAR); len; str1++, str2++, len--)
        if ((ret = to_lower(*str1) - to_lower(*str2))) break;
    return ret;
}

unsigned int hash_strW( const WCHAR *str, data_size_t len, unsigned int hash_size )
{
    unsigned int i, hash = 0;

    for (i = 0; i < len / sizeof(WCHAR); i++) hash = hash * 65599 + to_lower( str[i] );
    return hash % hash_size;
}

WCHAR *ascii_to_unicode_str( const char *str, struct unicode_str *ret )
{
    data_size_t i, len = strlen(str);
    WCHAR *p;

    ret->len = len * sizeof(WCHAR);
    ret->str = p = mem_alloc( ret->len );
    if (p) for (i = 0; i < len; i++) p[i] = (unsigned char)str[i];
    return p;
}

/* parse an escaped string back into Unicode */
/* return the number of chars read from the input, or -1 on output overflow */
int parse_strW( WCHAR *buffer, data_size_t *len, const char *src, char endchar )
{
    WCHAR *dest = buffer;
    WCHAR *end = buffer + *len / sizeof(WCHAR);
    const char *p = src;
    unsigned char ch;

    while (*p && *p != endchar && dest < end)
    {
        if (*p == '\\')
        {
            p++;
            if (!*p) break;
            switch(*p)
            {
            case 'a': *dest++ = '\a'; p++; continue;
            case 'b': *dest++ = '\b'; p++; continue;
            case 'e': *dest++ = '\e'; p++; continue;
            case 'f': *dest++ = '\f'; p++; continue;
            case 'n': *dest++ = '\n'; p++; continue;
            case 'r': *dest++ = '\r'; p++; continue;
            case 't': *dest++ = '\t'; p++; continue;
            case 'v': *dest++ = '\v'; p++; continue;
            case 'x':  /* hex escape */
                p++;
                if (!isxdigit(*p)) *dest = 'x';
                else
                {
                    *dest = to_hex(*p++);
                    if (isxdigit(*p)) *dest = (*dest * 16) + to_hex(*p++);
                    if (isxdigit(*p)) *dest = (*dest * 16) + to_hex(*p++);
                    if (isxdigit(*p)) *dest = (*dest * 16) + to_hex(*p++);
                }
                dest++;
                continue;
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':  /* octal escape */
                *dest = *p++ - '0';
                if (*p >= '0' && *p <= '7') *dest = (*dest * 8) + (*p++ - '0');
                if (*p >= '0' && *p <= '7') *dest = (*dest * 8) + (*p++ - '0');
                dest++;
                continue;
            }
            /* unrecognized escape: fall through to normal char handling */
        }


        ch = *p++;
        if (ch < 0x80) *dest++ = ch;
        else  /* parse utf8 char */
        {
            int charlen = utf8_length[ch-0x80];
            unsigned int res = ch & utf8_mask[charlen];

            switch(charlen)
            {
            case 3:
                if ((ch = *p ^ 0x80) >= 0x40) break;
                res = (res << 6) | ch;
                p++;
            case 2:
                if ((ch = *p ^ 0x80) >= 0x40) break;
                res = (res << 6) | ch;
                p++;
            case 1:
                if ((ch = *p ^ 0x80) >= 0x40) break;
                res = (res << 6) | ch;
                p++;
                if (res < utf8_minval[charlen]) break;
                if (res > 0x10ffff) break;
                if (res <= 0xffff) *dest++ = res;
                else /* we need surrogates */
                {
                    res -= 0x10000;
                    *dest++ = 0xd800 | (res >> 10);
                    if (dest < end) *dest++ = 0xdc00 | (res & 0x3ff);
                }
                continue;
            }
            /* ignore invalid char */
        }
    }
    if (dest >= end) return -1;  /* overflow */
    *dest++ = 0;
    if (!*p) return -1;  /* delimiter not found */
    *len = (dest - buffer) * sizeof(WCHAR);
    return p + 1 - src;
}

/* dump a Unicode string with proper escaping */
int dump_strW( const WCHAR *str, data_size_t len, FILE *f, const char escape[2] )
{
    static const char escapes[32] = ".......abtnvfr.............e....";
    char buffer[256];
    char *pos = buffer;
    int count = 0;

    for (len /= sizeof(WCHAR); len; str++, len--)
    {
        if (pos > buffer + sizeof(buffer) - 8)
        {
            fwrite( buffer, pos - buffer, 1, f );
            count += pos - buffer;
            pos = buffer;
        }
        if (*str > 127)  /* hex escape */
        {
            if (len > 1 && str[1] < 128 && isxdigit((char)str[1]))
                pos += sprintf( pos, "\\x%04x", *str );
            else
                pos += sprintf( pos, "\\x%x", *str );
            continue;
        }
        if (*str < 32)  /* octal or C escape */
        {
            if (escapes[*str] != '.')
                pos += sprintf( pos, "\\%c", escapes[*str] );
            else if (len > 1 && str[1] >= '0' && str[1] <= '7')
                pos += sprintf( pos, "\\%03o", *str );
            else
                pos += sprintf( pos, "\\%o", *str );
            continue;
        }
        if (*str == '\\' || *str == escape[0] || *str == escape[1]) *pos++ = '\\';
        *pos++ = *str;
    }
    fwrite( buffer, pos - buffer, 1, f );
    count += pos - buffer;
    return count;
}

static char *get_nls_dir(void)
{
    char *p, *dir, *ret;
    const char *nlsdir = BIN_TO_NLSDIR;

#if defined(__linux__) || defined(__FreeBSD_kernel__) || defined(__NetBSD__)
    dir = realpath( "/proc/self/exe", NULL );
#elif defined (__FreeBSD__) || defined(__DragonFly__)
    static int pathname[] = { CTL_KERN, KERN_PROC, KERN_PROC_PATHNAME, -1 };
    size_t dir_size = PATH_MAX;
    dir = malloc( dir_size );
    if (dir)
    {
        if (sysctl( pathname, ARRAY_SIZE( pathname ), dir, &dir_size, NULL, 0 ))
        {
            free( dir );
            dir = NULL;
        }
    }
#else
    dir = realpath( server_argv0, NULL );
#endif
    if (!dir) return NULL;
    if (!(p = strrchr( dir, '/' )))
    {
        free( dir );
        return NULL;
    }
    *(++p) = 0;
    if (p > dir + 8 && !strcmp( p - 8, "/server/" )) nlsdir = "../nls";  /* inside build tree */
    if ((ret = malloc( strlen(dir) + strlen( nlsdir ) + 1 )))
    {
        strcpy( ret, dir );
        strcat( ret, nlsdir );
    }
    free( dir );
    return ret;
}

/* load the case mapping table */
struct fd *load_intl_file(void)
{
    static const char *nls_dirs[] = { NULL, NLSDIR, "/usr/local/share/wine/nls", "/usr/share/wine/nls" };
    static const WCHAR nt_pathW[] = {'C',':','\\','w','i','n','d','o','w','s','\\',
        's','y','s','t','e','m','3','2','\\','l','_','i','n','t','l','.','n','l','s',0};
    static const struct unicode_str nt_name = { nt_pathW, sizeof(nt_pathW) };
    unsigned int i, offset, size;
    unsigned short data;
    char *path;
    struct fd *fd = NULL;
    int unix_fd;
    mode_t mode = 0600;

    nls_dirs[0] = get_nls_dir();
    for (i = 0; i < ARRAY_SIZE( nls_dirs ); i++)
    {
        if (!nls_dirs[i]) continue;
        if (!(path = malloc( strlen(nls_dirs[i]) + sizeof("/l_intl.nls" )))) continue;
        strcpy( path, nls_dirs[i] );
        strcat( path, "/l_intl.nls" );
        if ((fd = open_fd( NULL, path, nt_name, O_RDONLY, &mode, FILE_READ_DATA,
                           FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                           FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT ))) break;
        free( path );
    }
    if (!fd) fatal_error( "failed to load l_intl.nls\n" );
    unix_fd = get_unix_fd( fd );
    /* read initial offset */
    if (pread( unix_fd, &data, sizeof(data), 0 ) != sizeof(data) || !data) goto failed;
    offset = data;
    /* read size of uppercase table */
    if (pread( unix_fd, &data, sizeof(data), offset * 2 ) != sizeof(data) || !data) goto failed;
    offset += data;
    /* read size of lowercase table */
    if (pread( unix_fd, &data, sizeof(data), offset * 2 ) != sizeof(data) || !data) goto failed;
    offset++;
    size = data - 1;
    /* read lowercase table */
    if (!(casemap = malloc( size * 2 ))) goto failed;
    if (pread( unix_fd, casemap, size * 2, offset * 2 ) != size * 2) goto failed;
    free( path );
    return fd;

failed:
    fatal_error( "invalid format for casemap table %s\n", path );
}
