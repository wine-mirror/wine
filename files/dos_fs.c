/*
 * DOS file system functions
 *
 * Copyright 1993 Erik Bos
 * Copyright 1996 Alexandre Julliard
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

#include "config.h"

#include <sys/types.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#ifdef HAVE_SYS_ERRNO_H
#include <sys/errno.h>
#endif
#include <fcntl.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#include <time.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "ntstatus.h"
#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "wingdi.h"

#include "wine/unicode.h"
#include "wine/winbase16.h"
#include "drive.h"
#include "file.h"
#include "heap.h"
#include "msdos.h"
#include "winternl.h"
#include "wine/server.h"
#include "excpt.h"

#include "smb.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dosfs);
WINE_DECLARE_DEBUG_CHANNEL(file);

/* Define the VFAT ioctl to get both short and long file names */
/* FIXME: is it possible to get this to work on other systems? */
#ifdef linux
/* We want the real kernel dirent structure, not the libc one */
typedef struct
{
    long d_ino;
    long d_off;
    unsigned short d_reclen;
    char d_name[256];
} KERNEL_DIRENT;

#define VFAT_IOCTL_READDIR_BOTH  _IOR('r', 1, KERNEL_DIRENT [2] )

/* To avoid blocking on non-directories in DOSFS_OpenDir_VFAT*/
#ifndef O_DIRECTORY
# define O_DIRECTORY    0200000	/* must be directory */
#endif

#else   /* linux */
#undef VFAT_IOCTL_READDIR_BOTH  /* just in case... */
#endif  /* linux */

#define IS_OPTION_TRUE(ch) ((ch) == 'y' || (ch) == 'Y' || (ch) == 't' || (ch) == 'T' || (ch) == '1')

/* Chars we don't want to see in DOS file names */
#define INVALID_DOS_CHARS  "*?<>|\"+=,;[] \345"

static const DOS_DEVICE DOSFS_Devices[] =
/* name, device flags (see Int 21/AX=0x4400) */
{
    { {'C','O','N',0}, 0xc0d3 },
    { {'P','R','N',0}, 0xa0c0 },
    { {'N','U','L',0}, 0x80c4 },
    { {'A','U','X',0}, 0x80c0 },
    { {'L','P','T','1',0}, 0xa0c0 },
    { {'L','P','T','2',0}, 0xa0c0 },
    { {'L','P','T','3',0}, 0xa0c0 },
    { {'L','P','T','4',0}, 0xc0d3 },
    { {'C','O','M','1',0}, 0x80c0 },
    { {'C','O','M','2',0}, 0x80c0 },
    { {'C','O','M','3',0}, 0x80c0 },
    { {'C','O','M','4',0}, 0x80c0 },
    { {'S','C','S','I','M','G','R','$',0}, 0xc0c0 },
    { {'H','P','S','C','A','N',0}, 0xc0c0 },
    { {'E','M','M','X','X','X','X','0',0}, 0x0000 }
};

static const WCHAR devW[] = {'\\','D','e','v','i','c','e','\\',0};
static const WCHAR dosW[] = {'\\','D','o','s','D','e','v','i','c','e','s','\\',0};

static const WCHAR auxW[] = {'A','U','X',0};
static const WCHAR comW[] = {'C','O','M',0};
static const WCHAR lptW[] = {'L','P','T',0};
static const WCHAR nulW[] = {'N','U','L',0};

static const WCHAR nullW[] = {'N','u','l','l',0};
static const WCHAR parW[] = {'P','a','r','a','l','l','e','l',0};
static const WCHAR serW[] = {'S','e','r','i','a','l',0};
static const WCHAR oneW[] = {'1',0};

/*
 * Directory info for DOSFS_ReadDir
 * contains the names of *all* the files in the directory
 */
typedef struct
{
    int used;
    int size;
    WCHAR names[1];
} DOS_DIR;

/* Info structure for FindFirstFile handle */
typedef struct
{
    char *path; /* unix path */
    LPWSTR long_mask;
    LPWSTR short_mask;
    BYTE  attr;
    int   drive;
    int   cur_pos;
    union
    {
        DOS_DIR *dos_dir;
        SMB_DIR *smb_dir;
    } u;
} FIND_FIRST_INFO;


static WINE_EXCEPTION_FILTER(page_fault)
{
    if (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION)
        return EXCEPTION_EXECUTE_HANDLER;
    return EXCEPTION_CONTINUE_SEARCH;
}


/***********************************************************************
 *           DOSFS_ValidDOSName
 *
 * Return 1 if Unix file 'name' is also a valid MS-DOS name
 * (i.e. contains only valid DOS chars, lower-case only, fits in 8.3 format).
 * File name can be terminated by '\0', '\\' or '/'.
 */
static int DOSFS_ValidDOSName( LPCWSTR name, int ignore_case )
{
    static const char invalid_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ" INVALID_DOS_CHARS;
    const WCHAR *p = name;
    const char *invalid = ignore_case ? (invalid_chars + 26) : invalid_chars;
    int len = 0;

    if (*p == '.')
    {
        /* Check for "." and ".." */
        p++;
        if (*p == '.') p++;
        /* All other names beginning with '.' are invalid */
        return (IS_END_OF_NAME(*p));
    }
    while (!IS_END_OF_NAME(*p))
    {
        if (*p < 256 && strchr( invalid, (char)*p )) return 0;  /* Invalid char */
        if (*p == '.') break;  /* Start of the extension */
        if (++len > 8) return 0;  /* Name too long */
        p++;
    }
    if (*p != '.') return 1;  /* End of name */
    p++;
    if (IS_END_OF_NAME(*p)) return 0;  /* Empty extension not allowed */
    len = 0;
    while (!IS_END_OF_NAME(*p))
    {
        if (*p < 256 && strchr( invalid, (char)*p )) return 0;  /* Invalid char */
        if (*p == '.') return 0;  /* Second extension not allowed */
        if (++len > 3) return 0;  /* Extension too long */
        p++;
    }
    return 1;
}


/***********************************************************************
 *           DOSFS_ToDosFCBFormat
 *
 * Convert a file name to DOS FCB format (8+3 chars, padded with blanks),
 * expanding wild cards and converting to upper-case in the process.
 * File name can be terminated by '\0', '\\' or '/'.
 * Return FALSE if the name is not a valid DOS name.
 * 'buffer' must be at least 12 characters long.
 */
BOOL DOSFS_ToDosFCBFormat( LPCWSTR name, LPWSTR buffer )
{
    static const char invalid_chars[] = INVALID_DOS_CHARS;
    LPCWSTR p = name;
    int i;

    /* Check for "." and ".." */
    if (*p == '.')
    {
        p++;
        buffer[0] = '.';
        for(i = 1; i < 11; i++) buffer[i] = ' ';
        buffer[11] = 0;
        if (*p == '.')
        {
            buffer[1] = '.';
            p++;
        }
        return (!*p || (*p == '/') || (*p == '\\'));
    }

    for (i = 0; i < 8; i++)
    {
        switch(*p)
        {
        case '\0':
        case '\\':
        case '/':
        case '.':
            buffer[i] = ' ';
            break;
        case '?':
            p++;
            /* fall through */
        case '*':
            buffer[i] = '?';
            break;
        default:
            if (*p < 256 && strchr( invalid_chars, (char)*p )) return FALSE;
            buffer[i] = toupperW(*p);
            p++;
            break;
        }
    }

    if (*p == '*')
    {
        /* Skip all chars after wildcard up to first dot */
        while (*p && (*p != '/') && (*p != '\\') && (*p != '.')) p++;
    }
    else
    {
        /* Check if name too long */
        if (*p && (*p != '/') && (*p != '\\') && (*p != '.')) return FALSE;
    }
    if (*p == '.') p++;  /* Skip dot */

    for (i = 8; i < 11; i++)
    {
        switch(*p)
        {
        case '\0':
        case '\\':
        case '/':
            buffer[i] = ' ';
            break;
        case '.':
            return FALSE;  /* Second extension not allowed */
        case '?':
            p++;
            /* fall through */
        case '*':
            buffer[i] = '?';
            break;
        default:
            if (*p < 256 && strchr( invalid_chars, (char)*p )) return FALSE;
            buffer[i] = toupperW(*p);
            p++;
            break;
        }
    }
    buffer[11] = '\0';

    /* at most 3 character of the extension are processed
     * is something behind this ?
     */
    while (*p == '*' || *p == ' ') p++; /* skip wildcards and spaces */
    return IS_END_OF_NAME(*p);
}


/***********************************************************************
 *           DOSFS_ToDosDTAFormat
 *
 * Convert a file name from FCB to DTA format (name.ext, null-terminated)
 * converting to upper-case in the process.
 * File name can be terminated by '\0', '\\' or '/'.
 * 'buffer' must be at least 13 characters long.
 */
static void DOSFS_ToDosDTAFormat( LPCWSTR name, LPWSTR buffer )
{
    LPWSTR p;

    memcpy( buffer, name, 8 * sizeof(WCHAR) );
    p = buffer + 8;
    while ((p > buffer) && (p[-1] == ' ')) p--;
    *p++ = '.';
    memcpy( p, name + 8, 3 * sizeof(WCHAR) );
    p += 3;
    while (p[-1] == ' ') p--;
    if (p[-1] == '.') p--;
    *p = '\0';
}


/***********************************************************************
 *           DOSFS_MatchShort
 *
 * Check a DOS file name against a mask (both in FCB format).
 */
static int DOSFS_MatchShort( LPCWSTR mask, LPCWSTR name )
{
    int i;
    for (i = 11; i > 0; i--, mask++, name++)
        if ((*mask != '?') && (*mask != *name)) return 0;
    return 1;
}


/***********************************************************************
 *           DOSFS_MatchLong
 *
 * Check a long file name against a mask.
 *
 * Tests (done in W95 DOS shell - case insensitive):
 * *.txt			test1.test.txt				*
 * *st1*			test1.txt				*
 * *.t??????.t*			test1.ta.tornado.txt			*
 * *tornado*			test1.ta.tornado.txt			*
 * t*t				test1.ta.tornado.txt			*
 * ?est*			test1.txt				*
 * ?est???			test1.txt				-
 * *test1.txt*			test1.txt				*
 * h?l?o*t.dat			hellothisisatest.dat			*
 */
static int DOSFS_MatchLong( LPCWSTR mask, LPCWSTR name, int case_sensitive )
{
    LPCWSTR lastjoker = NULL;
    LPCWSTR next_to_retry = NULL;
    static const WCHAR asterisk_dot_asterisk[] = {'*','.','*',0};

    TRACE("(%s, %s, %x)\n", debugstr_w(mask), debugstr_w(name), case_sensitive);

    if (!strcmpW( mask, asterisk_dot_asterisk )) return 1;
    while (*name && *mask)
    {
        if (*mask == '*')
        {
            mask++;
            while (*mask == '*') mask++;  /* Skip consecutive '*' */
            lastjoker = mask;
            if (!*mask) return 1; /* end of mask is all '*', so match */

            /* skip to the next match after the joker(s) */
            if (case_sensitive) while (*name && (*name != *mask)) name++;
            else while (*name && (toupperW(*name) != toupperW(*mask))) name++;

            if (!*name) break;
            next_to_retry = name;
        }
        else if (*mask != '?')
        {
            int mismatch = 0;
            if (case_sensitive)
            {
                if (*mask != *name) mismatch = 1;
            }
            else
            {
                if (toupperW(*mask) != toupperW(*name)) mismatch = 1;
            }
            if (!mismatch)
            {
                mask++;
                name++;
                if (*mask == '\0')
                {
                    if (*name == '\0')
                        return 1;
                    if (lastjoker)
                        mask = lastjoker;
                }
            }
            else /* mismatch ! */
            {
                if (lastjoker) /* we had an '*', so we can try unlimitedly */
                {
                    mask = lastjoker;

                    /* this scan sequence was a mismatch, so restart
                     * 1 char after the first char we checked last time */
                    next_to_retry++;
                    name = next_to_retry;
                }
                else
                    return 0; /* bad luck */
            }
        }
        else /* '?' */
        {
            mask++;
            name++;
        }
    }
    while ((*mask == '.') || (*mask == '*'))
        mask++;  /* Ignore trailing '.' or '*' in mask */
    return (!*name && !*mask);
}


/***********************************************************************
 *           DOSFS_AddDirEntry
 *
 *  Used to construct an array of filenames in DOSFS_OpenDir
 */
static BOOL DOSFS_AddDirEntry(DOS_DIR **dir, LPCWSTR name, LPCWSTR dosname)
{
    int extra1 = strlenW(name) + 1;
    int extra2 = strlenW(dosname) + 1;

    /* if we need more, at minimum double the size */
    if( (extra1 + extra2 + (*dir)->used) > (*dir)->size)
    {
        int more = (*dir)->size;
        DOS_DIR *t;

        if(more<(extra1+extra2))
            more = extra1+extra2;

        t = HeapReAlloc(GetProcessHeap(), 0, *dir, sizeof(**dir) + 
                        ((*dir)->size + more)*sizeof(WCHAR) );
        if(!t)
        {
            SetLastError( ERROR_NOT_ENOUGH_MEMORY );
            ERR("Out of memory caching directory structure %d %d %d\n",
                 (*dir)->size, more, (*dir)->used);
            return FALSE;
        }
        (*dir) = t;
        (*dir)->size += more;
    }

    /* at this point, the dir structure is big enough to hold these names */
    strcpyW(&(*dir)->names[(*dir)->used], name);
    (*dir)->used += extra1;
    strcpyW(&(*dir)->names[(*dir)->used], dosname);
    (*dir)->used += extra2;

    return TRUE;
}


/***********************************************************************
 *           DOSFS_OpenDir_VFAT
 */
static BOOL DOSFS_OpenDir_VFAT(UINT codepage, DOS_DIR **dir, const char *unix_path)
{
#ifdef VFAT_IOCTL_READDIR_BOTH
    KERNEL_DIRENT de[2];
    int fd = open( unix_path, O_RDONLY|O_DIRECTORY );
    BOOL r = TRUE;

    /* Check if the VFAT ioctl is supported on this directory */

    if ( fd<0 )
        return FALSE;

    while (1)
    {
        WCHAR long_name[MAX_PATH];
        WCHAR short_name[12];

        r = (ioctl( fd, VFAT_IOCTL_READDIR_BOTH, (long)de ) != -1);
        if(!r)
            break;
        if (!de[0].d_reclen)
            break;
        MultiByteToWideChar(codepage, 0, de[0].d_name, -1, long_name, MAX_PATH);
        if (!DOSFS_ToDosFCBFormat( long_name, short_name ))
            short_name[0] = '\0';
        if (de[1].d_name[0])
            MultiByteToWideChar(codepage, 0, de[1].d_name, -1, long_name, MAX_PATH);
        else
            MultiByteToWideChar(codepage, 0, de[0].d_name, -1, long_name, MAX_PATH);
        r = DOSFS_AddDirEntry(dir, long_name, short_name );
        if(!r)
            break;
    }
    if(r)
    {
        static const WCHAR empty_strW[] = { 0 };
        DOSFS_AddDirEntry(dir, empty_strW, empty_strW);
    }
    close(fd);
    return r;
#else
    return FALSE;
#endif  /* VFAT_IOCTL_READDIR_BOTH */
}


/***********************************************************************
 *           DOSFS_OpenDir_Normal
 *
 * Now use the standard opendir/readdir interface
 */
static BOOL DOSFS_OpenDir_Normal( UINT codepage, DOS_DIR **dir, const char *unix_path )
{
    DIR *unixdir = opendir( unix_path );
    BOOL r = TRUE;
    static const WCHAR empty_strW[] = { 0 };

    if(!unixdir)
        return FALSE;
    while(1)
    {
        WCHAR long_name[MAX_PATH];
        struct dirent *de = readdir(unixdir);

        if(!de)
            break;
        MultiByteToWideChar(codepage, 0, de->d_name, -1, long_name, MAX_PATH);
        r = DOSFS_AddDirEntry(dir, long_name, empty_strW);
        if(!r)
            break;
    }
    if(r)
        DOSFS_AddDirEntry(dir, empty_strW, empty_strW);
    closedir(unixdir);
    return r;
}

/***********************************************************************
 *           DOSFS_OpenDir
 */
static DOS_DIR *DOSFS_OpenDir( UINT codepage, const char *unix_path )
{
    const int init_size = 0x100;
    DOS_DIR *dir = HeapAlloc( GetProcessHeap(), 0, sizeof(*dir) + init_size*sizeof (WCHAR));
    BOOL r;

    TRACE("%s\n",debugstr_a(unix_path));

    if (!dir)
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return NULL;
    }
    dir->used = 0;
    dir->size = init_size;

    /* Treat empty path as root directory. This simplifies path split into
       directory and mask in several other places */
    if (!*unix_path) unix_path = "/";

    r = DOSFS_OpenDir_VFAT( codepage, &dir, unix_path);

    if(!r)
        r = DOSFS_OpenDir_Normal( codepage, &dir, unix_path);

    if(!r)
    {
        HeapFree(GetProcessHeap(), 0, dir);
        return NULL;
    }
    dir->used = 0;

    return dir;
}


/***********************************************************************
 *           DOSFS_CloseDir
 */
static void DOSFS_CloseDir( DOS_DIR *dir )
{
    HeapFree( GetProcessHeap(), 0, dir );
}


/***********************************************************************
 *           DOSFS_ReadDir
 */
static BOOL DOSFS_ReadDir( DOS_DIR *dir, LPCWSTR *long_name,
                             LPCWSTR *short_name )
{
    LPCWSTR sn, ln;

    if (!dir)
       return FALSE;

    /* the long pathname is first */
    ln = &dir->names[dir->used];
    if(ln[0])
        *long_name  = ln;
    else
        return FALSE;
    dir->used += (strlenW(ln) + 1);

    /* followed by the short path name */
    sn = &dir->names[dir->used];
    if(sn[0])
        *short_name = sn;
    else
        *short_name = NULL;
    dir->used += (strlenW(sn) + 1);

    return TRUE;
}


/***********************************************************************
 *           DOSFS_Hash
 *
 * Transform a Unix file name into a hashed DOS name. If the name is a valid
 * DOS name, it is converted to upper-case; otherwise it is replaced by a
 * hashed version that fits in 8.3 format.
 * File name can be terminated by '\0', '\\' or '/'.
 * 'buffer' must be at least 13 characters long.
 */
static void DOSFS_Hash( LPCWSTR name, LPWSTR buffer, BOOL dir_format,
                        BOOL ignore_case )
{
    static const char invalid_chars[] = INVALID_DOS_CHARS "~.";
    static const char hash_chars[32] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ012345";

    LPCWSTR p, ext;
    LPWSTR dst;
    unsigned short hash;
    int i;

    if (dir_format)
    {
        for(i = 0; i < 11; i++) buffer[i] = ' ';
        buffer[11] = 0;
    }

    if (DOSFS_ValidDOSName( name, ignore_case ))
    {
        /* Check for '.' and '..' */
        if (*name == '.')
        {
            buffer[0] = '.';
            if (!dir_format) buffer[1] = buffer[2] = '\0';
            if (name[1] == '.') buffer[1] = '.';
            return;
        }

        /* Simply copy the name, converting to uppercase */

        for (dst = buffer; !IS_END_OF_NAME(*name) && (*name != '.'); name++)
            *dst++ = toupperW(*name);
        if (*name == '.')
        {
            if (dir_format) dst = buffer + 8;
            else *dst++ = '.';
            for (name++; !IS_END_OF_NAME(*name); name++)
                *dst++ = toupperW(*name);
        }
        if (!dir_format) *dst = '\0';
        return;
    }

    /* Compute the hash code of the file name */
    /* If you know something about hash functions, feel free to */
    /* insert a better algorithm here... */
    if (ignore_case)
    {
        for (p = name, hash = 0xbeef; !IS_END_OF_NAME(p[1]); p++)
            hash = (hash<<3) ^ (hash>>5) ^ tolowerW(*p) ^ (tolowerW(p[1]) << 8);
        hash = (hash<<3) ^ (hash>>5) ^ tolowerW(*p); /* Last character */
    }
    else
    {
        for (p = name, hash = 0xbeef; !IS_END_OF_NAME(p[1]); p++)
            hash = (hash << 3) ^ (hash >> 5) ^ *p ^ (p[1] << 8);
        hash = (hash << 3) ^ (hash >> 5) ^ *p;  /* Last character */
    }

    /* Find last dot for start of the extension */
    for (p = name+1, ext = NULL; !IS_END_OF_NAME(*p); p++)
        if (*p == '.') ext = p;
    if (ext && IS_END_OF_NAME(ext[1]))
        ext = NULL;  /* Empty extension ignored */

    /* Copy first 4 chars, replacing invalid chars with '_' */
    for (i = 4, p = name, dst = buffer; i > 0; i--, p++)
    {
        if (IS_END_OF_NAME(*p) || (p == ext)) break;
        *dst++ = (*p < 256 && strchr( invalid_chars, (char)*p )) ? '_' : toupperW(*p);
    }
    /* Pad to 5 chars with '~' */
    while (i-- >= 0) *dst++ = '~';

    /* Insert hash code converted to 3 ASCII chars */
    *dst++ = hash_chars[(hash >> 10) & 0x1f];
    *dst++ = hash_chars[(hash >> 5) & 0x1f];
    *dst++ = hash_chars[hash & 0x1f];

    /* Copy the first 3 chars of the extension (if any) */
    if (ext)
    {
        if (!dir_format) *dst++ = '.';
        for (i = 3, ext++; (i > 0) && !IS_END_OF_NAME(*ext); i--, ext++)
            *dst++ = (*ext < 256 && strchr( invalid_chars, (char)*ext )) ? '_' : toupperW(*ext);
    }
    if (!dir_format) *dst = '\0';
}


/***********************************************************************
 *           DOSFS_FindUnixName
 *
 * Find the Unix file name in a given directory that corresponds to
 * a file name (either in Unix or DOS format).
 * File name can be terminated by '\0', '\\' or '/'.
 * Return TRUE if OK, FALSE if no file name matches.
 *
 * 'long_buf' must be at least 'long_len' characters long. If the long name
 * turns out to be larger than that, the function returns FALSE.
 * 'short_buf' must be at least 13 characters long.
 */
BOOL DOSFS_FindUnixName( const DOS_FULL_NAME *path, LPCWSTR name, char *long_buf,
                         INT long_len, LPWSTR short_buf, BOOL ignore_case)
{
    DOS_DIR *dir;
    LPCWSTR long_name, short_name;
    WCHAR dos_name[12], tmp_buf[13];
    BOOL ret;

    LPCWSTR p = strchrW( name, '/' );
    int len = p ? (int)(p - name) : strlenW(name);
    if ((p = strchrW( name, '\\' ))) len = min( (int)(p - name), len );
    /* Ignore trailing dots and spaces */
    while (len > 1 && (name[len-1] == '.' || name[len-1] == ' ')) len--;
    if (long_len < len + 1) return FALSE;

    TRACE("%s,%s\n", path->long_name, debugstr_w(name) );

    if (!DOSFS_ToDosFCBFormat( name, dos_name )) dos_name[0] = '\0';

    if (!(dir = DOSFS_OpenDir( DRIVE_GetCodepage(path->drive), path->long_name )))
    {
        WARN("(%s,%s): can't open dir: %s\n",
             path->long_name, debugstr_w(name), strerror(errno) );
        return FALSE;
    }

    while ((ret = DOSFS_ReadDir( dir, &long_name, &short_name )))
    {
        /* Check against Unix name */
        if (len == strlenW(long_name))
        {
            if (!ignore_case)
            {
                if (!strncmpW( long_name, name, len )) break;
            }
            else
            {
                if (!strncmpiW( long_name, name, len )) break;
            }
        }
        if (dos_name[0])
        {
            /* Check against hashed DOS name */
            if (!short_name)
            {
                DOSFS_Hash( long_name, tmp_buf, TRUE, ignore_case );
                short_name = tmp_buf;
            }
            if (!strcmpW( dos_name, short_name )) break;
        }
    }
    if (ret)
    {
        if (long_buf) WideCharToMultiByte(DRIVE_GetCodepage(path->drive), 0,
                                          long_name, -1, long_buf, long_len, NULL, NULL);
        if (short_buf)
        {
            if (short_name)
                DOSFS_ToDosDTAFormat( short_name, short_buf );
            else
                DOSFS_Hash( long_name, short_buf, FALSE, ignore_case );
        }
        TRACE("(%s,%s) -> %s (%s)\n", path->long_name, debugstr_w(name),
              debugstr_w(long_name), short_buf ? debugstr_w(short_buf) : "***");
    }
    else
        WARN("%s not found in '%s'\n", debugstr_w(name), path->long_name);
    DOSFS_CloseDir( dir );
    return ret;
}


/***********************************************************************
 *           DOSFS_GetDevice
 *
 * Check if a DOS file name represents a DOS device and return the device.
 */
const DOS_DEVICE *DOSFS_GetDevice( LPCWSTR name )
{
    unsigned int i;
    const WCHAR *p;

    if (!name) return NULL; /* if wine_server_handle_to_fd was used */
    if (name[0] && (name[1] == ':')) name += 2;
    if ((p = strrchrW( name, '/' ))) name = p + 1;
    if ((p = strrchrW( name, '\\' ))) name = p + 1;
    for (i = 0; i < sizeof(DOSFS_Devices)/sizeof(DOSFS_Devices[0]); i++)
    {
        const WCHAR *dev = DOSFS_Devices[i].name;
        if (!strncmpiW( dev, name, strlenW(dev) ))
        {
            p = name + strlenW( dev );
            if (!*p || (*p == '.') || (*p == ':')) return &DOSFS_Devices[i];
        }
    }
    return NULL;
}


/***********************************************************************
 *           DOSFS_GetDeviceByHandle
 */
const DOS_DEVICE *DOSFS_GetDeviceByHandle( HANDLE hFile )
{
    const DOS_DEVICE *ret = NULL;
    SERVER_START_REQ( get_device_id )
    {
        req->handle = hFile;
        if (!wine_server_call( req ))
        {
            if ((reply->id >= 0) &&
                (reply->id < sizeof(DOSFS_Devices)/sizeof(DOSFS_Devices[0])))
                ret = &DOSFS_Devices[reply->id];
        }
    }
    SERVER_END_REQ;
    return ret;
}


/**************************************************************************
 *         DOSFS_CreateCommPort
 */
static HANDLE DOSFS_CreateCommPort(LPCWSTR name, DWORD access, DWORD attributes, LPSECURITY_ATTRIBUTES sa)
{
    HANDLE ret;
    HKEY hkey;
    DWORD dummy;
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING nameW;
    WCHAR *devnameW;
    char tmp[128];
    char devname[40];

    static const WCHAR serialportsW[] = {'M','a','c','h','i','n','e','\\',
                                         'S','o','f','t','w','a','r','e','\\',
                                         'W','i','n','e','\\','W','i','n','e','\\',
                                         'C','o','n','f','i','g','\\',
                                         'S','e','r','i','a','l','P','o','r','t','s',0};

    TRACE_(file)("%s %lx %lx\n", debugstr_w(name), access, attributes);

    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.ObjectName = &nameW;
    attr.Attributes = 0;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;
    RtlInitUnicodeString( &nameW, serialportsW );

    if (NtOpenKey( &hkey, KEY_ALL_ACCESS, &attr )) return 0;

    RtlInitUnicodeString( &nameW, name );
    if (!NtQueryValueKey( hkey, &nameW, KeyValuePartialInformation, tmp, sizeof(tmp), &dummy ))
        devnameW = (WCHAR *)((KEY_VALUE_PARTIAL_INFORMATION *)tmp)->Data;
    else
        devnameW = NULL;

    NtClose( hkey );

    if (!devnameW) return 0;
    WideCharToMultiByte(CP_ACP, 0, devnameW, -1, devname, sizeof(devname), NULL, NULL);

    TRACE("opening %s as %s\n", devname, debugstr_w(name));

    SERVER_START_REQ( create_serial )
    {
        req->access  = access;
        req->inherit = (sa && (sa->nLength>=sizeof(*sa)) && sa->bInheritHandle);
        req->attributes = attributes;
        req->sharing = FILE_SHARE_READ|FILE_SHARE_WRITE;
        wine_server_add_data( req, devname, strlen(devname) );
        SetLastError(0);
        wine_server_call_err( req );
        ret = reply->handle;
    }
    SERVER_END_REQ;

    if(!ret)
        ERR("Couldn't open device '%s' ! (check permissions)\n",devname);
    else
        TRACE("return %p\n", ret );
    return ret;
}

/***********************************************************************
 *           DOSFS_OpenDevice
 *
 * Open a DOS device. This might not map 1:1 into the UNIX device concept.
 * Returns 0 on failure.
 */
HANDLE DOSFS_OpenDevice( LPCWSTR name, DWORD access, DWORD attributes, LPSECURITY_ATTRIBUTES sa )
{
    unsigned int i;
    const WCHAR *p;
    HANDLE handle;

    if (name[0] && (name[1] == ':')) name += 2;
    if ((p = strrchrW( name, '/' ))) name = p + 1;
    if ((p = strrchrW( name, '\\' ))) name = p + 1;
    for (i = 0; i < sizeof(DOSFS_Devices)/sizeof(DOSFS_Devices[0]); i++)
    {
        const WCHAR *dev = DOSFS_Devices[i].name;
        if (!strncmpiW( dev, name, strlenW(dev) ))
        {
            p = name + strlenW( dev );
            if (!*p || (*p == '.') || (*p == ':')) {
		static const WCHAR nulW[] = {'N','U','L',0};
		static const WCHAR conW[] = {'C','O','N',0};
		static const WCHAR scsimgrW[] = {'S','C','S','I','M','G','R','$',0};
		static const WCHAR hpscanW[] = {'H','P','S','C','A','N',0};
		static const WCHAR emmxxxx0W[] = {'E','M','M','X','X','X','X','0',0};
	    	/* got it */
		if (!strcmpiW(DOSFS_Devices[i].name, nulW))
                    return FILE_CreateFile( "/dev/null", access,
                                            FILE_SHARE_READ|FILE_SHARE_WRITE, sa,
                                            OPEN_EXISTING, 0, 0, TRUE, DRIVE_UNKNOWN );
		if (!strcmpiW(DOSFS_Devices[i].name, conW)) {
			HANDLE to_dup;
			switch (access & (GENERIC_READ|GENERIC_WRITE)) {
			case GENERIC_READ:
				to_dup = GetStdHandle( STD_INPUT_HANDLE );
				break;
			case GENERIC_WRITE:
				to_dup = GetStdHandle( STD_OUTPUT_HANDLE );
				break;
			default:
				FIXME("can't open CON read/write\n");
				return 0;
			}
			if (!DuplicateHandle( GetCurrentProcess(), to_dup, GetCurrentProcess(),
					      &handle, 0,
					      sa && (sa->nLength>=sizeof(*sa)) && sa->bInheritHandle,
					      DUPLICATE_SAME_ACCESS ))
			    handle = 0;
			return handle;
		}
		if (!strcmpiW(DOSFS_Devices[i].name, scsimgrW) ||
                    !strcmpiW(DOSFS_Devices[i].name, hpscanW) ||
                    !strcmpiW(DOSFS_Devices[i].name, emmxxxx0W))
                {
                    return FILE_CreateDevice( i, access, sa );
		}

                if( (handle=DOSFS_CreateCommPort(DOSFS_Devices[i].name,access,attributes,sa)) )
                    return handle;
                FIXME("device open %s not supported (yet)\n", debugstr_w(DOSFS_Devices[i].name));
    		return 0;
	    }
        }
    }
    return 0;
}


/***********************************************************************
 *           DOSFS_GetPathDrive
 *
 * Get the drive specified by a given path name (DOS or Unix format).
 */
static int DOSFS_GetPathDrive( LPCWSTR *name )
{
    int drive;
    LPCWSTR p = *name;

    if (*p && (p[1] == ':'))
    {
        drive = toupperW(*p) - 'A';
        *name += 2;
    }
    else if (*p == '/') /* Absolute Unix path? */
    {
        if ((drive = DRIVE_FindDriveRootW( name )) == -1)
        {
            MESSAGE("Warning: %s not accessible from a configured DOS drive\n", debugstr_w(*name) );
            /* Assume it really was a DOS name */
            drive = DRIVE_GetCurrentDrive();
        }
    }
    else drive = DRIVE_GetCurrentDrive();

    if (!DRIVE_IsValid(drive))
    {
        SetLastError( ERROR_INVALID_DRIVE );
        return -1;
    }
    return drive;
}


/***********************************************************************
 *           DOSFS_GetFullName
 *
 * Convert a file name (DOS or mixed DOS/Unix format) to a valid
 * Unix name / short DOS name pair.
 * Return FALSE if one of the path components does not exist. The last path
 * component is only checked if 'check_last' is non-zero.
 * The buffers pointed to by 'long_buf' and 'short_buf' must be
 * at least MAX_PATHNAME_LEN long.
 */
BOOL DOSFS_GetFullName( LPCWSTR name, BOOL check_last, DOS_FULL_NAME *full )
{
    BOOL found;
    UINT flags, codepage;
    char *p_l, *root;
    LPWSTR p_s;
    static const WCHAR driveA_rootW[] = {'A',':','\\',0};
    static const WCHAR dos_rootW[] = {'\\',0};

    TRACE("%s (last=%d)\n", debugstr_w(name), check_last );

    if ((!*name) || (*name=='\n'))
    { /* error code for Win98 */
        SetLastError(ERROR_BAD_PATHNAME);
        return FALSE;
    }

    if ((full->drive = DOSFS_GetPathDrive( &name )) == -1) return FALSE;
    flags = DRIVE_GetFlags( full->drive );
    codepage = DRIVE_GetCodepage(full->drive);

    lstrcpynA( full->long_name, DRIVE_GetRoot( full->drive ),
                 sizeof(full->long_name) );
    if (full->long_name[1]) root = full->long_name + strlen(full->long_name);
    else root = full->long_name;  /* root directory */

    strcpyW( full->short_name, driveA_rootW );
    full->short_name[0] += full->drive;

    if ((*name == '\\') || (*name == '/'))  /* Absolute path */
    {
        while ((*name == '\\') || (*name == '/')) name++;
    }
    else  /* Relative path */
    {
        lstrcpynA( root + 1, DRIVE_GetUnixCwd( full->drive ),
                     sizeof(full->long_name) - (root - full->long_name) - 1 );
        if (root[1]) *root = '/';
        lstrcpynW( full->short_name + 3, DRIVE_GetDosCwd( full->drive ),
                   sizeof(full->short_name)/sizeof(full->short_name[0]) - 3 );
    }

    p_l = full->long_name[1] ? full->long_name + strlen(full->long_name)
                             : full->long_name;
    p_s = full->short_name[3] ? full->short_name + strlenW(full->short_name)
                              : full->short_name + 2;
    found = TRUE;

    while (*name && found)
    {
        /* Check for '.' and '..' */

        if (*name == '.')
        {
            if (IS_END_OF_NAME(name[1]))
            {
                name++;
                while ((*name == '\\') || (*name == '/')) name++;
                continue;
            }
            else if ((name[1] == '.') && IS_END_OF_NAME(name[2]))
            {
                name += 2;
                while ((*name == '\\') || (*name == '/')) name++;
                while ((p_l > root) && (*p_l != '/')) p_l--;
                while ((p_s > full->short_name + 2) && (*p_s != '\\')) p_s--;
                *p_l = *p_s = '\0';  /* Remove trailing separator */
                continue;
            }
        }

        /* Make sure buffers are large enough */

        if ((p_s >= full->short_name + sizeof(full->short_name)/sizeof(full->short_name[0]) - 14) ||
            (p_l >= full->long_name + sizeof(full->long_name) - 1))
        {
            SetLastError( ERROR_PATH_NOT_FOUND );
            return FALSE;
        }

        /* Get the long and short name matching the file name */

        if ((found = DOSFS_FindUnixName( full, name, p_l + 1,
                         sizeof(full->long_name) - (p_l - full->long_name) - 1,
                         p_s + 1, !(flags & DRIVE_CASE_SENSITIVE) )))
        {
            *p_l++ = '/';
            p_l   += strlen(p_l);
            *p_s++ = '\\';
            p_s   += strlenW(p_s);
            while (!IS_END_OF_NAME(*name)) name++;
        }
        else if (!check_last)
        {
            *p_l++ = '/';
            *p_s++ = '\\';
            while (!IS_END_OF_NAME(*name) &&
                   (p_s < full->short_name + sizeof(full->short_name)/sizeof(full->short_name[0]) - 1) &&
                   (p_l < full->long_name + sizeof(full->long_name) - 1))
            {
                WCHAR wch;
                *p_s++ = tolowerW(*name);
                /* If the drive is case-sensitive we want to create new */
                /* files in lower-case otherwise we can't reopen them   */
                /* under the same short name. */
                if (flags & DRIVE_CASE_SENSITIVE) wch = tolowerW(*name);
                else wch = *name;
                p_l += WideCharToMultiByte(codepage, 0, &wch, 1, p_l, 2, NULL, NULL);
                name++;
            }
	    /* Ignore trailing dots and spaces */
	    while(p_l[-1] == '.' || p_l[-1] == ' ') {
		--p_l;
		--p_s;
	    }
            *p_l = '\0';
            *p_s = '\0';
        }
        while ((*name == '\\') || (*name == '/')) name++;
    }

    if (!found)
    {
        if (check_last)
        {
            SetLastError( ERROR_FILE_NOT_FOUND );
            return FALSE;
        }
        if (*name)  /* Not last */
        {
            SetLastError( ERROR_PATH_NOT_FOUND );
            return FALSE;
        }
    }
    if (!full->long_name[0]) strcpy( full->long_name, "/" );
    if (!full->short_name[2]) strcpyW( full->short_name + 2, dos_rootW );
    TRACE("returning %s = %s\n", full->long_name, debugstr_w(full->short_name) );
    return TRUE;
}


/***********************************************************************
 *           GetShortPathNameW   (KERNEL32.@)
 *
 * NOTES
 *  observed:
 *  longpath=NULL: LastError=ERROR_INVALID_PARAMETER, ret=0
 *  longpath="" or invalid: LastError=ERROR_BAD_PATHNAME, ret=0
 *
 * more observations ( with NT 3.51 (WinDD) ):
 * longpath <= 8.3 -> just copy longpath to shortpath
 * longpath > 8.3  ->
 *             a) file does not exist -> return 0, LastError = ERROR_FILE_NOT_FOUND
 *             b) file does exist     -> set the short filename.
 * - trailing slashes are reproduced in the short name, even if the
 *   file is not a directory
 * - the absolute/relative path of the short name is reproduced like found
 *   in the long name
 * - longpath and shortpath may have the same address
 * Peter Ganten, 1999
 */
DWORD WINAPI GetShortPathNameW( LPCWSTR longpath, LPWSTR shortpath, DWORD shortlen )
{
    DOS_FULL_NAME full_name;
    WCHAR tmpshortpath[MAX_PATHNAME_LEN];
    const WCHAR *p;
    DWORD sp = 0, lp = 0;
    int drive;
    DWORD tmplen;
    UINT flags;
    BOOL unixabsolute = *longpath == '/';

    TRACE("%s\n", debugstr_w(longpath));

    if (!longpath) {
      SetLastError(ERROR_INVALID_PARAMETER);
      return 0;
    }
    if (!longpath[0]) {
      SetLastError(ERROR_BAD_PATHNAME);
      return 0;
    }

    /* check for drive letter */
    if (!unixabsolute && longpath[1] == ':' ) {
      tmpshortpath[0] = longpath[0];
      tmpshortpath[1] = ':';
      sp = 2;
    }

    if ( ( drive = DOSFS_GetPathDrive ( &longpath )) == -1 ) return 0;
    flags = DRIVE_GetFlags ( drive );

    if (unixabsolute && drive != DRIVE_GetCurrentDrive()) {
      tmpshortpath[0] = drive + 'A';
      tmpshortpath[1] = ':';
      sp = 2;
    }

    while ( longpath[lp] ) {

      /* check for path delimiters and reproduce them */
      if ( longpath[lp] == '\\' || longpath[lp] == '/' ) {
	if (!sp || tmpshortpath[sp-1]!= '\\')
        {
	    /* strip double "\\" */
	    tmpshortpath[sp] = '\\';
	    sp++;
        }
        tmpshortpath[sp]=0;/*terminate string*/
	lp++;
	continue;
      }

      tmplen = 0;
      for(p = longpath + lp; *p && *p != '/' && *p != '\\'; p++)
          tmplen++;
      lstrcpynW(tmpshortpath + sp, longpath + lp, tmplen + 1);

      /* Check, if the current element is a valid dos name */
      if ( DOSFS_ValidDOSName ( longpath + lp, !(flags & DRIVE_CASE_SENSITIVE) ) ) {
	sp += tmplen;
	lp += tmplen;
	continue;
      }

      /* Check if the file exists and use the existing file name */
      if ( DOSFS_GetFullName ( tmpshortpath, TRUE, &full_name ) ) {
	strcpyW(tmpshortpath + sp, strrchrW(full_name.short_name, '\\') + 1);
	sp += strlenW(tmpshortpath + sp);
	lp += tmplen;
	continue;
      }

      TRACE("not found!\n" );
      SetLastError ( ERROR_FILE_NOT_FOUND );
      return 0;
    }
    tmpshortpath[sp] = 0;

    tmplen = strlenW(tmpshortpath) + 1;
    if (tmplen <= shortlen)
    {
        strcpyW(shortpath, tmpshortpath);
        TRACE("returning %s\n", debugstr_w(shortpath));
        tmplen--; /* length without 0 */
    }

    return tmplen;
}


/***********************************************************************
 *           GetShortPathNameA   (KERNEL32.@)
 */
DWORD WINAPI GetShortPathNameA( LPCSTR longpath, LPSTR shortpath, DWORD shortlen )
{
    UNICODE_STRING longpathW;
    WCHAR shortpathW[MAX_PATH];
    DWORD ret, retW;

    if (!longpath)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    TRACE("%s\n", debugstr_a(longpath));

    if (!RtlCreateUnicodeStringFromAsciiz(&longpathW, longpath))
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return 0;
    }

    retW = GetShortPathNameW(longpathW.Buffer, shortpathW, MAX_PATH);

    if (!retW)
        ret = 0;
    else if (retW > MAX_PATH)
    {
        SetLastError(ERROR_FILENAME_EXCED_RANGE);
        ret = 0;
    }
    else
    {
        ret = WideCharToMultiByte(CP_ACP, 0, shortpathW, -1, NULL, 0, NULL, NULL);
        if (ret <= shortlen)
        {
            WideCharToMultiByte(CP_ACP, 0, shortpathW, -1, shortpath, shortlen, NULL, NULL);
            ret--; /* length without 0 */
        }
    }

    RtlFreeUnicodeString(&longpathW);
    return ret;
}


/***********************************************************************
 *           GetLongPathNameW   (KERNEL32.@)
 *
 * NOTES
 *  observed (Win2000):
 *  shortpath=NULL: LastError=ERROR_INVALID_PARAMETER, ret=0
 *  shortpath="":   LastError=ERROR_PATH_NOT_FOUND, ret=0
 */
DWORD WINAPI GetLongPathNameW( LPCWSTR shortpath, LPWSTR longpath, DWORD longlen )
{
    DOS_FULL_NAME full_name;
    const char *root;
    LPWSTR p;
    int drive;
    UINT codepage;
    DWORD ret, len = 0;

    if (!shortpath) {
      SetLastError(ERROR_INVALID_PARAMETER);
      return 0;
    }
    if (!shortpath[0]) {
      SetLastError(ERROR_PATH_NOT_FOUND);
      return 0;
    }

    TRACE("%s,%p,%ld\n", debugstr_w(shortpath), longpath, longlen);

    if(shortpath[0]=='\\' && shortpath[1]=='\\')
    {
        ERR("UNC pathname %s\n",debugstr_w(shortpath));
        lstrcpynW( longpath, full_name.short_name, longlen );
        return strlenW(longpath);
    }

    if (!DOSFS_GetFullName( shortpath, TRUE, &full_name )) return 0;

    root = full_name.long_name;
    drive = DRIVE_FindDriveRoot(&root);
    codepage = DRIVE_GetCodepage(drive);

    ret = MultiByteToWideChar(codepage, 0, root, -1, NULL, 0);
    ret += 3; /* A:\ */
    /* reproduce terminating slash */
    if (ret > 4) /* if not drive root */
    {
        len = strlenW(shortpath);
        if (shortpath[len - 1] == '\\' || shortpath[len - 1] == '/')
            len = 1;
    }
    ret += len;
    if (ret <= longlen)
    {
        longpath[0] = 'A' + drive;
        longpath[1] = ':';
        MultiByteToWideChar(codepage, 0, root, -1, longpath + 2, longlen - 2);
        for (p = longpath; *p; p++) if (*p == '/') *p = '\\';
        if (len)
        {
            longpath[ret - 2] = '\\';
            longpath[ret - 1] = 0;
        }
        TRACE("returning %s\n", debugstr_w(longpath));
        ret--; /* length without 0 */
    }
    return ret;
}


/***********************************************************************
 *           GetLongPathNameA   (KERNEL32.@)
 */
DWORD WINAPI GetLongPathNameA( LPCSTR shortpath, LPSTR longpath, DWORD longlen )
{
    UNICODE_STRING shortpathW;
    WCHAR longpathW[MAX_PATH];
    DWORD ret, retW;

    if (!shortpath)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    TRACE("%s\n", debugstr_a(shortpath));

    if (!RtlCreateUnicodeStringFromAsciiz(&shortpathW, shortpath))
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return 0;
    }

    retW = GetLongPathNameW(shortpathW.Buffer, longpathW, MAX_PATH);

    if (!retW)
        ret = 0;
    else if (retW > MAX_PATH)
    {
        SetLastError(ERROR_FILENAME_EXCED_RANGE);
        ret = 0;
    }
    else
    {
        ret = WideCharToMultiByte(CP_ACP, 0, longpathW, -1, NULL, 0, NULL, NULL);
        if (ret <= longlen)
        {
            WideCharToMultiByte(CP_ACP, 0, longpathW, -1, longpath, longlen, NULL, NULL);
            ret--; /* length without 0 */
        }
    }

    RtlFreeUnicodeString(&shortpathW);
    return ret;
}


/***********************************************************************
 *           DOSFS_DoGetFullPathName
 *
 * Implementation of GetFullPathNameA/W.
 *
 * bon@elektron 000331:
 * A test for GetFullPathName with many pathological cases
 * now gives identical output for Wine and OSR2
 */
static DWORD DOSFS_DoGetFullPathName( LPCWSTR name, DWORD len, LPWSTR result )
{
    DWORD ret;
    DOS_FULL_NAME full_name;
    LPWSTR p, q;
    char *p_l;
    const char * root;
    WCHAR drivecur[] = {'C',':','.',0};
    WCHAR driveletter=0;
    int namelen,drive=0;
    static const WCHAR bkslashW[] = {'\\',0};
    static const WCHAR dotW[] = {'.',0};
    static const WCHAR updir_slashW[] = {'\\','.','.','\\',0};
    static const WCHAR curdirW[] = {'\\','.','\\',0};
    static const WCHAR updirW[] = {'\\','.','.',0};

    if (!name[0])
    {
        SetLastError(ERROR_BAD_PATHNAME);
        return 0;
    }

    TRACE("passed %s\n", debugstr_w(name));

    if (name[1]==':')
      /*drive letter given */
      {
	driveletter = name[0];
      }
    if ((name[1]==':') && ((name[2]=='\\') || (name[2]=='/')))
      /*absolute path given */
      {
        strncpyW(full_name.short_name, name, MAX_PATHNAME_LEN);
        full_name.short_name[MAX_PATHNAME_LEN - 1] = 0; /* ensure 0 termination */
        drive = toupperW(name[0]) - 'A';
      }
    else
      {
	if (driveletter)
	  drivecur[0]=driveletter;
        else if ((name[0]=='\\') || (name[0]=='/'))
          strcpyW(drivecur, bkslashW);
        else
	  strcpyW(drivecur, dotW);

	if (!DOSFS_GetFullName( drivecur, FALSE, &full_name ))
	  {
	    FIXME("internal: error getting drive/path\n");
	    return 0;
	  }
	/* find path that drive letter substitutes*/
	drive = toupperW(full_name.short_name[0]) - 'A';
	root= DRIVE_GetRoot(drive);
	if (!root)
	  {
	    FIXME("internal: error getting DOS Drive Root\n");
	    return 0;
	  }
	if (!strcmp(root,"/"))
	  {
	    /* we have just the last / and we need it. */
	    p_l = full_name.long_name;
	  }
	else
	  {
	    p_l = full_name.long_name + strlen(root);
	  }
	/* append long name (= unix name) to drive */
	MultiByteToWideChar(DRIVE_GetCodepage(drive), 0, p_l, -1,
                            full_name.short_name + 2, MAX_PATHNAME_LEN - 3);
	/* append name to treat */
	namelen= strlenW(full_name.short_name);
	p = (LPWSTR)name;
	if (driveletter)
	  p += 2; /* skip drive name when appending */
	if (namelen + 2 + strlenW(p) > MAX_PATHNAME_LEN)
	  {
	    FIXME("internal error: buffer too small\n");
	     return 0;
	  }
	full_name.short_name[namelen++] ='\\';
	full_name.short_name[namelen] = 0;
	strncpyW(full_name.short_name + namelen, p, MAX_PATHNAME_LEN - namelen);
	full_name.short_name[MAX_PATHNAME_LEN - 1] = 0; /* ensure 0 termination */
      }
    /* reverse all slashes */
    for (p=full_name.short_name;
	 p < full_name.short_name + strlenW(full_name.short_name);
	 p++)
      {
	if ( *p == '/' )
	  *p = '\\';
      }
     /* Use memmove, as areas overlap */
     /* Delete .. */
    while ((p = strstrW(full_name.short_name, updir_slashW)))
      {
	if (p > full_name.short_name+2)
	  {
	    *p = 0;
            q = strrchrW(full_name.short_name, '\\');
            memmove(q+1, p+4, (strlenW(p+4)+1) * sizeof(WCHAR));
	  }
	else
	  {
            memmove(full_name.short_name+3, p+4, (strlenW(p+4)+1) * sizeof(WCHAR));
	  }
      }
    if ((full_name.short_name[2]=='.')&&(full_name.short_name[3]=='.'))
	{
	  /* This case istn't treated yet : c:..\test */
	  memmove(full_name.short_name+2,full_name.short_name+4,
                  (strlenW(full_name.short_name+4)+1) * sizeof(WCHAR));
	}
     /* Delete . */
    while ((p = strstrW(full_name.short_name, curdirW)))
      {
	*(p+1) = 0;
        memmove(p+1, p+3, (strlenW(p+3)+1) * sizeof(WCHAR));
      }
    if (!(DRIVE_GetFlags(drive) & DRIVE_CASE_PRESERVING))
        for (p = full_name.short_name; *p; p++) *p = toupperW(*p);
    namelen = strlenW(full_name.short_name);
    if (!strcmpW(full_name.short_name+namelen-3, updirW))
	{
	  /* one more strange case: "c:\test\test1\.."
	   return "c:\test" */
	  *(full_name.short_name+namelen-3)=0;
          q = strrchrW(full_name.short_name, '\\');
	  *q =0;
	}
    if (full_name.short_name[namelen-1]=='.')
	full_name.short_name[(namelen--)-1] =0;
    if (!driveletter)
      if (full_name.short_name[namelen-1]=='\\')
	full_name.short_name[(namelen--)-1] =0;
    TRACE("got %s\n", debugstr_w(full_name.short_name));

    /* If the lpBuffer buffer is too small, the return value is the
    size of the buffer, in characters, required to hold the path
    plus the terminating \0 (tested against win95osr2, bon 001118)
    . */
    ret = strlenW(full_name.short_name);
    if (ret >= len )
      {
	/* don't touch anything when the buffer is not large enough */
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
	return ret+1;
      }
    if (result)
    {
        strncpyW( result, full_name.short_name, len );
        result[len - 1] = 0; /* ensure 0 termination */
    }

    TRACE("returning %s\n", debugstr_w(full_name.short_name) );
    return ret;
}


/***********************************************************************
 *           GetFullPathNameA   (KERNEL32.@)
 * NOTES
 *   if the path closed with '\', *lastpart is 0
 */
DWORD WINAPI GetFullPathNameA( LPCSTR name, DWORD len, LPSTR buffer,
                                 LPSTR *lastpart )
{
    UNICODE_STRING nameW;
    WCHAR bufferW[MAX_PATH];
    DWORD ret, retW;

    if (!name)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    if (!RtlCreateUnicodeStringFromAsciiz(&nameW, name))
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return 0;
    }

    retW = GetFullPathNameW( nameW.Buffer, MAX_PATH, bufferW, NULL);

    if (!retW)
        ret = 0;
    else if (retW > MAX_PATH)
    {
        SetLastError(ERROR_FILENAME_EXCED_RANGE);
        ret = 0;
    }
    else
    {
        ret = WideCharToMultiByte(CP_ACP, 0, bufferW, -1, NULL, 0, NULL, NULL);
        if (ret <= len)
        {
            WideCharToMultiByte(CP_ACP, 0, bufferW, -1, buffer, len, NULL, NULL);
            ret--; /* length without 0 */

            if (lastpart)
            {
                LPSTR p = buffer + strlen(buffer);

                if (*p != '\\')
                {
                    while ((p > buffer + 2) && (*p != '\\')) p--;
                    *lastpart = p + 1;
                }
                else *lastpart = NULL;
            }
        }
    }

    RtlFreeUnicodeString(&nameW);
    return ret;
}


/***********************************************************************
 *           GetFullPathNameW   (KERNEL32.@)
 */
DWORD WINAPI GetFullPathNameW( LPCWSTR name, DWORD len, LPWSTR buffer,
                                 LPWSTR *lastpart )
{
    DWORD ret = DOSFS_DoGetFullPathName( name, len, buffer );
    if (ret && (ret<=len) && buffer && lastpart)
    {
        LPWSTR p = buffer + strlenW(buffer);
        if (*p != (WCHAR)'\\')
        {
            while ((p > buffer + 2) && (*p != (WCHAR)'\\')) p--;
            *lastpart = p + 1;
        }
        else *lastpart = NULL;
    }
    return ret;
}


/***********************************************************************
 *           wine_get_unix_file_name (KERNEL32.@) Not a Windows API
 *
 * Return the full Unix file name for a given path.
 * FIXME: convert dos file name to unicode
 */
BOOL WINAPI wine_get_unix_file_name( LPCSTR dos, LPSTR buffer, DWORD len )
{
    BOOL ret;
    DOS_FULL_NAME path;
    WCHAR dosW[MAX_PATHNAME_LEN];

    MultiByteToWideChar(CP_ACP, 0, dos, -1, dosW, MAX_PATHNAME_LEN);
    ret = DOSFS_GetFullName( dosW, FALSE, &path );
    if (ret && len)
    {
        strncpy( buffer, path.long_name, len );
        buffer[len - 1] = 0; /* ensure 0 termination */
    }
    return ret;
}


/***********************************************************************
 *           get_show_dir_symlinks_option
 */
static BOOL get_show_dir_symlinks_option(void)
{
    static const WCHAR WineW[] = {'M','a','c','h','i','n','e','\\',
                                  'S','o','f','t','w','a','r','e','\\',
                                  'W','i','n','e','\\','W','i','n','e','\\',
                                  'C','o','n','f','i','g','\\','W','i','n','e',0};
    static const WCHAR ShowDirSymlinksW[] = {'S','h','o','w','D','i','r','S','y','m','l','i','n','k','s',0};

    char tmp[80];
    HKEY hkey;
    DWORD dummy;
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING nameW;
    BOOL ret = FALSE;

    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.ObjectName = &nameW;
    attr.Attributes = 0;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;
    RtlInitUnicodeString( &nameW, WineW );

    if (!NtOpenKey( &hkey, KEY_ALL_ACCESS, &attr ))
    {
        RtlInitUnicodeString( &nameW, ShowDirSymlinksW );
        if (!NtQueryValueKey( hkey, &nameW, KeyValuePartialInformation, tmp, sizeof(tmp), &dummy ))
        {
            WCHAR *str = (WCHAR *)((KEY_VALUE_PARTIAL_INFORMATION *)tmp)->Data;
            ret = IS_OPTION_TRUE( str[0] );
        }
        NtClose( hkey );
    }
    return ret;
}


/***********************************************************************
 *           DOSFS_FindNextEx
 */
static int DOSFS_FindNextEx( FIND_FIRST_INFO *info, WIN32_FIND_DATAW *entry )
{
    DWORD attr = info->attr | FA_UNUSED | FA_ARCHIVE | FA_RDONLY;
    UINT flags = DRIVE_GetFlags( info->drive );
    char *p, buffer[MAX_PATHNAME_LEN];
    const char *drive_path;
    int drive_root;
    LPCWSTR long_name, short_name;
    BY_HANDLE_FILE_INFORMATION fileinfo;
    WCHAR dos_name[13];
    BOOL is_symlink;

    if ((info->attr & ~(FA_UNUSED | FA_ARCHIVE | FA_RDONLY)) == FA_LABEL)
    {
        if (info->cur_pos) return 0;
        entry->dwFileAttributes  = FILE_ATTRIBUTE_LABEL;
        RtlSecondsSince1970ToTime( (time_t)0, (LARGE_INTEGER *)&entry->ftCreationTime );
        RtlSecondsSince1970ToTime( (time_t)0, (LARGE_INTEGER *)&entry->ftLastAccessTime );
        RtlSecondsSince1970ToTime( (time_t)0, (LARGE_INTEGER *)&entry->ftLastWriteTime );
        entry->nFileSizeHigh     = 0;
        entry->nFileSizeLow      = 0;
        entry->dwReserved0       = 0;
        entry->dwReserved1       = 0;
        DOSFS_ToDosDTAFormat( DRIVE_GetLabel( info->drive ), entry->cFileName );
        strcpyW( entry->cAlternateFileName, entry->cFileName );
        info->cur_pos++;
        TRACE("returning %s (%s) as label\n",
               debugstr_w(entry->cFileName), debugstr_w(entry->cAlternateFileName));
        return 1;
    }

    drive_path = info->path + strlen(DRIVE_GetRoot( info->drive ));
    while ((*drive_path == '/') || (*drive_path == '\\')) drive_path++;
    drive_root = !*drive_path;

    lstrcpynA( buffer, info->path, sizeof(buffer) - 1 );
    strcat( buffer, "/" );
    p = buffer + strlen(buffer);

    while (DOSFS_ReadDir( info->u.dos_dir, &long_name, &short_name ))
    {
        info->cur_pos++;

        /* Don't return '.' and '..' in the root of the drive */
        if (drive_root && (long_name[0] == '.') &&
            (!long_name[1] || ((long_name[1] == '.') && !long_name[2])))
            continue;

        /* Check the long mask */

        if (info->long_mask && *info->long_mask)
        {
            if (!DOSFS_MatchLong( info->long_mask, long_name,
                                  flags & DRIVE_CASE_SENSITIVE )) continue;
        }

        /* Check the short mask */

        if (info->short_mask)
        {
            if (!short_name)
            {
                DOSFS_Hash( long_name, dos_name, TRUE,
                            !(flags & DRIVE_CASE_SENSITIVE) );
                short_name = dos_name;
            }
            if (!DOSFS_MatchShort( info->short_mask, short_name )) continue;
        }

        /* Check the file attributes */
        WideCharToMultiByte(DRIVE_GetCodepage(info->drive), 0, long_name, -1,
                            p, sizeof(buffer) - (int)(p - buffer), NULL, NULL);
        if (!FILE_Stat( buffer, &fileinfo, &is_symlink ))
        {
            WARN("can't stat %s\n", buffer);
            continue;
        }
        if (is_symlink && (fileinfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            static int show_dir_symlinks = -1;
            if (show_dir_symlinks == -1)
                show_dir_symlinks = get_show_dir_symlinks_option();
            if (!show_dir_symlinks) continue;
        }

        if (fileinfo.dwFileAttributes & ~attr) continue;

        /* We now have a matching entry; fill the result and return */

        entry->dwFileAttributes = fileinfo.dwFileAttributes;
        entry->ftCreationTime   = fileinfo.ftCreationTime;
        entry->ftLastAccessTime = fileinfo.ftLastAccessTime;
        entry->ftLastWriteTime  = fileinfo.ftLastWriteTime;
        entry->nFileSizeHigh    = fileinfo.nFileSizeHigh;
        entry->nFileSizeLow     = fileinfo.nFileSizeLow;

        if (short_name)
            DOSFS_ToDosDTAFormat( short_name, entry->cAlternateFileName );
        else
            DOSFS_Hash( long_name, entry->cAlternateFileName, FALSE,
                        !(flags & DRIVE_CASE_SENSITIVE) );

        lstrcpynW( entry->cFileName, long_name, sizeof(entry->cFileName)/sizeof(entry->cFileName[0]) );
        if (!(flags & DRIVE_CASE_PRESERVING)) strlwrW( entry->cFileName );
        TRACE("returning %s (%s) %02lx %ld\n",
              debugstr_w(entry->cFileName), debugstr_w(entry->cAlternateFileName),
              entry->dwFileAttributes, entry->nFileSizeLow );
        return 1;
    }
    return 0;  /* End of directory */
}

/***********************************************************************
 *           DOSFS_FindNext
 *
 * Find the next matching file. Return the number of entries read to find
 * the matching one, or 0 if no more entries.
 * 'short_mask' is the 8.3 mask (in FCB format), 'long_mask' is the long
 * file name mask. Either or both can be NULL.
 *
 * NOTE: This is supposed to be only called by the int21 emulation
 *       routines. Thus, we should own the Win16Mutex anyway.
 *       Nevertheless, we explicitly enter it to ensure the static
 *       directory cache is protected.
 */
int DOSFS_FindNext( const char *path, const char *short_mask,
                    const char *long_mask, int drive, BYTE attr,
                    int skip, WIN32_FIND_DATAA *entry )
{
    static FIND_FIRST_INFO info;
    LPCWSTR short_name, long_name;
    int count;
    UNICODE_STRING short_maskW, long_maskW;
    WIN32_FIND_DATAW entryW;

    TRACE("(%s, %s, %s, %x, %x, %x, %p)\n", debugstr_a(path),
          debugstr_a(short_mask), debugstr_a(long_mask), drive, attr, skip,
          entry);

    _EnterWin16Lock();

    RtlCreateUnicodeStringFromAsciiz(&short_maskW, short_mask);
    RtlCreateUnicodeStringFromAsciiz(&long_maskW, long_mask);

    /* Check the cached directory */
    if (!(info.u.dos_dir && info.path == path && !strcmpW(info.short_mask, short_maskW.Buffer)
                   && !strcmpW(info.long_mask, long_maskW.Buffer) && info.drive == drive
                   && info.attr == attr && info.cur_pos <= skip))
    {
        /* Not in the cache, open it anew */
        if (info.u.dos_dir) DOSFS_CloseDir( info.u.dos_dir );

        info.path = (LPSTR)path;
        RtlFreeHeap(GetProcessHeap(), 0, info.long_mask);
        RtlFreeHeap(GetProcessHeap(), 0, info.short_mask);
        info.long_mask = long_maskW.Buffer;
        info.short_mask = short_maskW.Buffer;
        info.attr = attr;
        info.drive = drive;
        info.cur_pos = 0;
        info.u.dos_dir = DOSFS_OpenDir( DRIVE_GetCodepage(drive), info.path );
    }
    else
    {
        RtlFreeUnicodeString(&short_maskW);
        RtlFreeUnicodeString(&long_maskW);
    }

    /* Skip to desired position */
    while (info.cur_pos < skip)
        if (info.u.dos_dir && DOSFS_ReadDir( info.u.dos_dir, &long_name, &short_name ))
            info.cur_pos++;
        else
            break;

    if (info.u.dos_dir && info.cur_pos == skip && DOSFS_FindNextEx( &info, &entryW ))
    {
        WideCharToMultiByte(CP_ACP, 0, entryW.cFileName, -1,
                            entry->cFileName, sizeof(entry->cFileName), NULL, NULL);
        WideCharToMultiByte(CP_ACP, 0, entryW.cAlternateFileName, -1,
                            entry->cAlternateFileName, sizeof(entry->cAlternateFileName), NULL, NULL);
        count = info.cur_pos - skip;

        entry->dwFileAttributes = entryW.dwFileAttributes;
        entry->nFileSizeHigh    = entryW.nFileSizeHigh;
        entry->nFileSizeLow     = entryW.nFileSizeLow;
        entry->ftCreationTime   = entryW.ftCreationTime;
        entry->ftLastAccessTime = entryW.ftLastAccessTime;
        entry->ftLastWriteTime  = entryW.ftLastWriteTime;

    }
    else
        count = 0;

    if (!count)
    {
        if (info.u.dos_dir) DOSFS_CloseDir( info.u.dos_dir );
        memset( &info, '\0', sizeof(info) );
    }

    _LeaveWin16Lock();

    return count;
}

/*************************************************************************
 *           FindFirstFileExW  (KERNEL32.@)
 */
HANDLE WINAPI FindFirstFileExW(
	LPCWSTR lpFileName,
	FINDEX_INFO_LEVELS fInfoLevelId,
	LPVOID lpFindFileData,
	FINDEX_SEARCH_OPS fSearchOp,
	LPVOID lpSearchFilter,
	DWORD dwAdditionalFlags)
{
    HGLOBAL handle;
    FIND_FIRST_INFO *info;

    if (!lpFileName)
    {
        SetLastError(ERROR_PATH_NOT_FOUND);
        return INVALID_HANDLE_VALUE;
    }

    if ((fSearchOp != FindExSearchNameMatch) || (dwAdditionalFlags != 0))
    {
        FIXME("options not implemented 0x%08x 0x%08lx\n", fSearchOp, dwAdditionalFlags );
        return INVALID_HANDLE_VALUE;
    }

    switch(fInfoLevelId)
    {
      case FindExInfoStandard:
        {
          WIN32_FIND_DATAW * data = (WIN32_FIND_DATAW *) lpFindFileData;
          char *p;
          INT long_mask_len;
          UINT codepage;

          data->dwReserved0 = data->dwReserved1 = 0x0;
          if (lpFileName[0] == '\\' && lpFileName[1] == '\\')
          {
              ERR("UNC path name\n");
              if (!(handle = GlobalAlloc(GMEM_MOVEABLE, sizeof(FIND_FIRST_INFO)))) break;

              info = (FIND_FIRST_INFO *)GlobalLock( handle );
              info->u.smb_dir = SMB_FindFirst(lpFileName);
              if(!info->u.smb_dir)
              {
                 GlobalUnlock( handle );
                 GlobalFree(handle);
                 break;
              }

              info->drive = -1;

              GlobalUnlock( handle );
          }
          else
          {
            DOS_FULL_NAME full_name;

            if (lpFileName[0] && lpFileName[1] == ':')
            {
                /* don't allow root directories */
                if (!lpFileName[2] ||
                    ((lpFileName[2] == '/' || lpFileName[2] == '\\') && !lpFileName[3]))
                {
                    SetLastError(ERROR_FILE_NOT_FOUND);
                    return INVALID_HANDLE_VALUE;
                }
            }
            if (!DOSFS_GetFullName( lpFileName, FALSE, &full_name )) break;
            if (!(handle = GlobalAlloc(GMEM_MOVEABLE, sizeof(FIND_FIRST_INFO)))) break;
            info = (FIND_FIRST_INFO *)GlobalLock( handle );
            info->path = HeapAlloc( GetProcessHeap(), 0, strlen(full_name.long_name)+1 );
            strcpy( info->path, full_name.long_name );

            codepage = DRIVE_GetCodepage(full_name.drive);
            p = strrchr( info->path, '/' );
            *p++ = '\0';
            long_mask_len = MultiByteToWideChar(codepage, 0, p, -1, NULL, 0);
            info->long_mask = HeapAlloc( GetProcessHeap(), 0, long_mask_len * sizeof(WCHAR) );
            MultiByteToWideChar(codepage, 0, p, -1, info->long_mask, long_mask_len);

            info->short_mask = NULL;
            info->attr = 0xff;
            info->drive = full_name.drive;
            info->cur_pos = 0;

            info->u.dos_dir = DOSFS_OpenDir( codepage, info->path );
            GlobalUnlock( handle );
          }
          if (!FindNextFileW( handle, data ))
          {
              FindClose( handle );
              SetLastError( ERROR_FILE_NOT_FOUND );
              break;
          }
          return handle;
        }
        break;
      default:
        FIXME("fInfoLevelId 0x%08x not implemented\n", fInfoLevelId );
    }
    return INVALID_HANDLE_VALUE;
}

/*************************************************************************
 *           FindFirstFileA   (KERNEL32.@)
 */
HANDLE WINAPI FindFirstFileA(
	LPCSTR lpFileName,
	WIN32_FIND_DATAA *lpFindData )
{
    return FindFirstFileExA(lpFileName, FindExInfoStandard, lpFindData,
                            FindExSearchNameMatch, NULL, 0);
}

/*************************************************************************
 *           FindFirstFileExA   (KERNEL32.@)
 */
HANDLE WINAPI FindFirstFileExA(
	LPCSTR lpFileName,
	FINDEX_INFO_LEVELS fInfoLevelId,
	LPVOID lpFindFileData,
	FINDEX_SEARCH_OPS fSearchOp,
	LPVOID lpSearchFilter,
	DWORD dwAdditionalFlags)
{
    HANDLE handle;
    WIN32_FIND_DATAA *dataA;
    WIN32_FIND_DATAW dataW;
    UNICODE_STRING pathW;

    if (!lpFileName)
    {
        SetLastError(ERROR_PATH_NOT_FOUND);
        return INVALID_HANDLE_VALUE;
    }

    if (!RtlCreateUnicodeStringFromAsciiz(&pathW, lpFileName))
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return INVALID_HANDLE_VALUE;
    }

    handle = FindFirstFileExW(pathW.Buffer, fInfoLevelId, &dataW, fSearchOp, lpSearchFilter, dwAdditionalFlags);
    RtlFreeUnicodeString(&pathW);
    if (handle == INVALID_HANDLE_VALUE) return handle;

    dataA = (WIN32_FIND_DATAA *) lpFindFileData;
    dataA->dwFileAttributes = dataW.dwFileAttributes;
    dataA->ftCreationTime   = dataW.ftCreationTime;
    dataA->ftLastAccessTime = dataW.ftLastAccessTime;
    dataA->ftLastWriteTime  = dataW.ftLastWriteTime;
    dataA->nFileSizeHigh    = dataW.nFileSizeHigh;
    dataA->nFileSizeLow     = dataW.nFileSizeLow;
    WideCharToMultiByte( CP_ACP, 0, dataW.cFileName, -1,
                         dataA->cFileName, sizeof(dataA->cFileName), NULL, NULL );
    WideCharToMultiByte( CP_ACP, 0, dataW.cAlternateFileName, -1,
                         dataA->cAlternateFileName, sizeof(dataA->cAlternateFileName), NULL, NULL );
    return handle;
}

/*************************************************************************
 *           FindFirstFileW   (KERNEL32.@)
 */
HANDLE WINAPI FindFirstFileW( LPCWSTR lpFileName, WIN32_FIND_DATAW *lpFindData )
{
    return FindFirstFileExW(lpFileName, FindExInfoStandard, lpFindData,
                            FindExSearchNameMatch, NULL, 0);
}

/*************************************************************************
 *           FindNextFileW   (KERNEL32.@)
 */
BOOL WINAPI FindNextFileW( HANDLE handle, WIN32_FIND_DATAW *data )
{
    FIND_FIRST_INFO *info;
    BOOL ret = FALSE;
    DWORD gle = ERROR_NO_MORE_FILES;

    if ((handle == INVALID_HANDLE_VALUE) ||
       !(info = (FIND_FIRST_INFO *)GlobalLock( handle )))
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return ret;
    }
    if (info->drive == -1)
    {
        ret = SMB_FindNext( info->u.smb_dir, data );
        if(!ret)
        {
            SMB_CloseDir( info->u.smb_dir );
            HeapFree( GetProcessHeap(), 0, info->path );
        }
        goto done;
    }
    else if (!info->path || !info->u.dos_dir)
    {
        goto done;
    }
    else if (!DOSFS_FindNextEx( info, data ))
    {
        DOSFS_CloseDir( info->u.dos_dir ); info->u.dos_dir = NULL;
        HeapFree( GetProcessHeap(), 0, info->path );
        info->path = NULL;
        HeapFree( GetProcessHeap(), 0, info->long_mask );
        info->long_mask = NULL;
        goto done;
    }
    ret = TRUE;
done:
    GlobalUnlock( handle );
    if( !ret ) SetLastError( gle );
    return ret;
}


/*************************************************************************
 *           FindNextFileA   (KERNEL32.@)
 */
BOOL WINAPI FindNextFileA( HANDLE handle, WIN32_FIND_DATAA *data )
{
    WIN32_FIND_DATAW dataW;
    if (!FindNextFileW( handle, &dataW )) return FALSE;
    data->dwFileAttributes = dataW.dwFileAttributes;
    data->ftCreationTime   = dataW.ftCreationTime;
    data->ftLastAccessTime = dataW.ftLastAccessTime;
    data->ftLastWriteTime  = dataW.ftLastWriteTime;
    data->nFileSizeHigh    = dataW.nFileSizeHigh;
    data->nFileSizeLow     = dataW.nFileSizeLow;
    WideCharToMultiByte( CP_ACP, 0, dataW.cFileName, -1,
                         data->cFileName, sizeof(data->cFileName), NULL, NULL );
    WideCharToMultiByte( CP_ACP, 0, dataW.cAlternateFileName, -1,
                         data->cAlternateFileName,
                         sizeof(data->cAlternateFileName), NULL, NULL );
    return TRUE;
}

/*************************************************************************
 *           FindClose   (KERNEL32.@)
 */
BOOL WINAPI FindClose( HANDLE handle )
{
    FIND_FIRST_INFO *info;

    if (handle == INVALID_HANDLE_VALUE) goto error;

    __TRY
    {
        if ((info = (FIND_FIRST_INFO *)GlobalLock( handle )))
        {
            if (info->u.dos_dir) DOSFS_CloseDir( info->u.dos_dir );
            if (info->path) HeapFree( GetProcessHeap(), 0, info->path );
            if (info->long_mask) HeapFree( GetProcessHeap(), 0, info->long_mask );
        }
    }
    __EXCEPT(page_fault)
    {
        WARN("Illegal handle %p\n", handle);
        SetLastError( ERROR_INVALID_HANDLE );
        return FALSE;
    }
    __ENDTRY
    if (!info) goto error;
    GlobalUnlock( handle );
    GlobalFree( handle );
    return TRUE;

 error:
    SetLastError( ERROR_INVALID_HANDLE );
    return FALSE;
}

/***********************************************************************
 *           MulDiv   (KERNEL32.@)
 * RETURNS
 *	Result of multiplication and division
 *	-1: Overflow occurred or Divisor was 0
 */
INT WINAPI MulDiv(
	     INT nMultiplicand,
	     INT nMultiplier,
	     INT nDivisor)
{
#if SIZEOF_LONG_LONG >= 8
    long long ret;

    if (!nDivisor) return -1;

    /* We want to deal with a positive divisor to simplify the logic. */
    if (nDivisor < 0)
    {
      nMultiplicand = - nMultiplicand;
      nDivisor = -nDivisor;
    }

    /* If the result is positive, we "add" to round. else, we subtract to round. */
    if ( ( (nMultiplicand <  0) && (nMultiplier <  0) ) ||
	 ( (nMultiplicand >= 0) && (nMultiplier >= 0) ) )
      ret = (((long long)nMultiplicand * nMultiplier) + (nDivisor/2)) / nDivisor;
    else
      ret = (((long long)nMultiplicand * nMultiplier) - (nDivisor/2)) / nDivisor;

    if ((ret > 2147483647) || (ret < -2147483647)) return -1;
    return ret;
#else
    if (!nDivisor) return -1;

    /* We want to deal with a positive divisor to simplify the logic. */
    if (nDivisor < 0)
    {
      nMultiplicand = - nMultiplicand;
      nDivisor = -nDivisor;
    }

    /* If the result is positive, we "add" to round. else, we subtract to round. */
    if ( ( (nMultiplicand <  0) && (nMultiplier <  0) ) ||
	 ( (nMultiplicand >= 0) && (nMultiplier >= 0) ) )
      return ((nMultiplicand * nMultiplier) + (nDivisor/2)) / nDivisor;

    return ((nMultiplicand * nMultiplier) - (nDivisor/2)) / nDivisor;

#endif
}


/***********************************************************************
 *           DosDateTimeToFileTime   (KERNEL32.@)
 */
BOOL WINAPI DosDateTimeToFileTime( WORD fatdate, WORD fattime, LPFILETIME ft)
{
    struct tm newtm;
#ifndef HAVE_TIMEGM
    struct tm *gtm;
    time_t time1, time2;
#endif

    newtm.tm_sec  = (fattime & 0x1f) * 2;
    newtm.tm_min  = (fattime >> 5) & 0x3f;
    newtm.tm_hour = (fattime >> 11);
    newtm.tm_mday = (fatdate & 0x1f);
    newtm.tm_mon  = ((fatdate >> 5) & 0x0f) - 1;
    newtm.tm_year = (fatdate >> 9) + 80;
#ifdef HAVE_TIMEGM
    RtlSecondsSince1970ToTime( timegm(&newtm), (LARGE_INTEGER *)ft );
#else
    time1 = mktime(&newtm);
    gtm = gmtime(&time1);
    time2 = mktime(gtm);
    RtlSecondsSince1970ToTime( 2*time1-time2, (LARGE_INTEGER *)ft );
#endif
    return TRUE;
}


/***********************************************************************
 *           FileTimeToDosDateTime   (KERNEL32.@)
 */
BOOL WINAPI FileTimeToDosDateTime( const FILETIME *ft, LPWORD fatdate,
                                     LPWORD fattime )
{
    LARGE_INTEGER       li;
    ULONG               t;
    time_t              unixtime;
    struct tm*          tm;

    li.s.LowPart = ft->dwLowDateTime;
    li.s.HighPart = ft->dwHighDateTime;
    RtlTimeToSecondsSince1970( &li, &t );
    unixtime = t;
    tm = gmtime( &unixtime );
    if (fattime)
        *fattime = (tm->tm_hour << 11) + (tm->tm_min << 5) + (tm->tm_sec / 2);
    if (fatdate)
        *fatdate = ((tm->tm_year - 80) << 9) + ((tm->tm_mon + 1) << 5)
                   + tm->tm_mday;
    return TRUE;
}


/***********************************************************************
 *           QueryDosDeviceA   (KERNEL32.@)
 *
 * returns array of strings terminated by \0, terminated by \0
 */
DWORD WINAPI QueryDosDeviceA(LPCSTR devname,LPSTR target,DWORD bufsize)
{
    DWORD ret = 0, retW;
    LPWSTR targetW = (LPWSTR)HeapAlloc(GetProcessHeap(),0,
                                     bufsize * sizeof(WCHAR));
    UNICODE_STRING devnameW;

    if(devname) RtlCreateUnicodeStringFromAsciiz(&devnameW, devname);
    else devnameW.Buffer = NULL;

    retW = QueryDosDeviceW(devnameW.Buffer, targetW, bufsize);

    ret = WideCharToMultiByte(CP_ACP, 0, targetW, retW, target,
                                        bufsize, NULL, NULL);

    RtlFreeUnicodeString(&devnameW);
    if (targetW) HeapFree(GetProcessHeap(),0,targetW);
    return ret;
}


/***********************************************************************
 *           QueryDosDeviceW   (KERNEL32.@)
 *
 * returns array of strings terminated by \0, terminated by \0
 *
 * FIXME
 *      - Win9x returns for all calls ERROR_INVALID_PARAMETER 
 *	- the returned devices for devname == NULL is far from complete
 *      - its not checked that the returned device exist
 */
DWORD WINAPI QueryDosDeviceW(LPCWSTR devname,LPWSTR target,DWORD bufsize)
{
    const WCHAR *pDev, *pName, *pNum = NULL;
    int    numsiz=0;
    DWORD  ret;

    TRACE("(%s,...)\n", debugstr_w(devname));
    if (!devname) {
	/* return known MSDOS devices */
        DWORD ret = 0;
	int i;
        static const WCHAR devices[][5] = {{'A','U','X',0},
                                           {'C','O','M','1',0},
                                           {'C','O','M','2',0},
                                           {'L','P','T','1',0},
                                           {'N','U','L',0,}};
	for(i=0; (i< (sizeof(devices)/sizeof(devices[0]))); i++) {
	    DWORD len = strlenW(devices[i]);
	    if(target && (bufsize >= ret + len + 2)) {
		lstrcpyW(target+ret, devices[i]);
                ret += len + 1;
	    } else {
                /* in this case WinXP returns 0 */
                FIXME("function return is wrong for WinXP!\n");
		SetLastError(ERROR_INSUFFICIENT_BUFFER);
		break;
	    }
	}
        /* append drives here */
	if(target && bufsize > 0) target[ret++] = 0;
	FIXME("Returned list is not complete\n");
        return ret;
    }
    /* In theory all that are possible and have been defined.
     * Now just those below, since mirc uses it to check for special files.
     *
     * (It is more complex, and supports netmounted stuff, and \\.\ stuff,
     *  but currently we just ignore that.)
     */
    if (!strcmpiW(devname, auxW)) {
        pDev   = dosW;
        pName  = comW;
        numsiz = 1;
        pNum   = oneW;
    } else if (!strcmpiW(devname, nulW)) {
        pDev  = devW;
        pName = nullW;
    } else if (!strncmpiW(devname, comW, strlenW(comW))) {
        pDev  = devW;
        pName = serW;
        pNum  = devname + strlenW(comW);
        for(numsiz=0; isdigitW(*(pNum+numsiz)); numsiz++);
        if(*(pNum + numsiz)) {
	    SetLastError(ERROR_FILE_NOT_FOUND);
	    return 0;
	}
    } else if (!strncmpiW(devname, lptW, strlenW(lptW))) {
        pDev  = devW;
        pName = parW;
        pNum  = devname + strlenW(lptW);
        for(numsiz=0; isdigitW(*(pNum+numsiz)); numsiz++);
        if(*(pNum + numsiz)) {
	    SetLastError(ERROR_FILE_NOT_FOUND);
	return 0;
    }
    } else {
	/* This might be a DOS device we do not handle yet ... */
	FIXME("(%s) not detected as DOS device!\n",debugstr_w(devname));

        /* Win9x set the error ERROR_INVALID_PARAMETER */
	SetLastError(ERROR_FILE_NOT_FOUND);
	return 0;
}
    FIXME("device %s may not exist on this computer\n", debugstr_w(devname));

    ret = strlenW(pDev) + strlenW(pName) + numsiz + 2;
    if (ret > bufsize) ret = 0;
    if (target && ret) {
        lstrcpyW(target,pDev);
        lstrcatW(target,pName);
        if (pNum) lstrcatW(target,pNum);
        target[ret-1] = 0;
    }
    return ret;
}


/***********************************************************************
 *           DefineDosDeviceA       (KERNEL32.@)
 */
BOOL WINAPI DefineDosDeviceA(DWORD flags,LPCSTR devname,LPCSTR targetpath) {
	FIXME("(0x%08lx,%s,%s),stub!\n",flags,devname,targetpath);
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}

/*
   --- 16 bit functions ---
*/

/*************************************************************************
 *           FindFirstFile   (KERNEL.413)
 */
HANDLE16 WINAPI FindFirstFile16( LPCSTR path, WIN32_FIND_DATAA *data )
{
    DOS_FULL_NAME full_name;
    HGLOBAL16 handle;
    FIND_FIRST_INFO *info;
    WCHAR pathW[MAX_PATH];
    char *p;
    INT long_mask_len;
    UINT codepage;

    data->dwReserved0 = data->dwReserved1 = 0x0;
    if (!path) return INVALID_HANDLE_VALUE16;
    MultiByteToWideChar(CP_ACP, 0, path, -1, pathW, MAX_PATH);
    if (!DOSFS_GetFullName( pathW, FALSE, &full_name ))
        return INVALID_HANDLE_VALUE16;
    if (!(handle = GlobalAlloc16( GMEM_MOVEABLE, sizeof(FIND_FIRST_INFO) )))
        return INVALID_HANDLE_VALUE16;
    info = (FIND_FIRST_INFO *)GlobalLock16( handle );
    info->path = HeapAlloc( GetProcessHeap(), 0, strlen(full_name.long_name)+1 );
    strcpy( info->path, full_name.long_name );

    codepage = DRIVE_GetCodepage(full_name.drive);
    p = strrchr( info->path, '/' );
    *p++ = '\0';
    long_mask_len = MultiByteToWideChar(codepage, 0, p, -1, NULL, 0);
    info->long_mask = HeapAlloc( GetProcessHeap(), 0, long_mask_len * sizeof(WCHAR) );
    MultiByteToWideChar(codepage, 0, p, -1, info->long_mask, long_mask_len);

    info->short_mask = NULL;
    info->attr = 0xff;
    info->drive = full_name.drive;
    info->cur_pos = 0;

    info->u.dos_dir = DOSFS_OpenDir( codepage, info->path );

    GlobalUnlock16( handle );
    if (!FindNextFile16( handle, data ))
    {
        FindClose16( handle );
        SetLastError( ERROR_NO_MORE_FILES );
        return INVALID_HANDLE_VALUE16;
    }
    return handle;
}

/*************************************************************************
 *           FindNextFile   (KERNEL.414)
 */
BOOL16 WINAPI FindNextFile16( HANDLE16 handle, WIN32_FIND_DATAA *data )
{
    FIND_FIRST_INFO *info;
    WIN32_FIND_DATAW dataW;
    BOOL ret = FALSE;
    DWORD gle = ERROR_NO_MORE_FILES;

    if ((handle == INVALID_HANDLE_VALUE16) ||
       !(info = (FIND_FIRST_INFO *)GlobalLock16( handle )))
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return ret;
    }
    if (!info->path || !info->u.dos_dir)
    {
        goto done;
    }
    if (!DOSFS_FindNextEx( info, &dataW ))
    {
        DOSFS_CloseDir( info->u.dos_dir ); info->u.dos_dir = NULL;
        HeapFree( GetProcessHeap(), 0, info->path );
        info->path = NULL;
        HeapFree( GetProcessHeap(), 0, info->long_mask );
        info->long_mask = NULL;
        goto done;
    }

    ret = TRUE;

    data->dwFileAttributes = dataW.dwFileAttributes;
    data->ftCreationTime   = dataW.ftCreationTime;
    data->ftLastAccessTime = dataW.ftLastAccessTime;
    data->ftLastWriteTime  = dataW.ftLastWriteTime;
    data->nFileSizeHigh    = dataW.nFileSizeHigh;
    data->nFileSizeLow     = dataW.nFileSizeLow;
    WideCharToMultiByte( CP_ACP, 0, dataW.cFileName, -1,
                         data->cFileName, sizeof(data->cFileName), NULL, NULL );
    WideCharToMultiByte( CP_ACP, 0, dataW.cAlternateFileName, -1,
                         data->cAlternateFileName,
                         sizeof(data->cAlternateFileName), NULL, NULL );
done:
    if( !ret ) SetLastError( gle );
    GlobalUnlock16( handle );

    return ret;
}

/*************************************************************************
 *           FindClose   (KERNEL.415)
 */
BOOL16 WINAPI FindClose16( HANDLE16 handle )
{
    FIND_FIRST_INFO *info;

    if ((handle == INVALID_HANDLE_VALUE16) ||
        !(info = (FIND_FIRST_INFO *)GlobalLock16( handle )))
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return FALSE;
    }
    if (info->u.dos_dir) DOSFS_CloseDir( info->u.dos_dir );
    if (info->path) HeapFree( GetProcessHeap(), 0, info->path );
    if (info->long_mask) HeapFree( GetProcessHeap(), 0, info->long_mask );
    GlobalUnlock16( handle );
    GlobalFree16( handle );
    return TRUE;
}
