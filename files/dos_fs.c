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
#include "wine/port.h"

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
#ifdef HAVE_LINUX_IOCTL_H
#include <linux/ioctl.h>
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
#include "file.h"
#include "winreg.h"
#include "winternl.h"
#include "wine/server.h"
#include "wine/exception.h"
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

/* Chars we don't want to see in DOS file names */
#define INVALID_DOS_CHARS  "*?<>|\"+=,;[] \345"

/* at some point we may want to allow Winelib apps to set this */
static const BOOL is_case_sensitive = FALSE;

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


/* return non-zero if c is the end of a directory name */
static inline int is_end_of_name(WCHAR c)
{
    return !c || (c == '/') || (c == '\\');
}

/***********************************************************************
 *           DOSFS_ValidDOSName
 *
 * Return 1 if Unix file 'name' is also a valid MS-DOS name
 * (i.e. contains only valid DOS chars, lower-case only, fits in 8.3 format).
 * File name can be terminated by '\0', '\\' or '/'.
 */
static int DOSFS_ValidDOSName( LPCWSTR name )
{
    static const char invalid_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ" INVALID_DOS_CHARS;
    const WCHAR *p = name;
    const char *invalid = !is_case_sensitive ? (invalid_chars + 26) : invalid_chars;
    int len = 0;

    if (*p == '.')
    {
        /* Check for "." and ".." */
        p++;
        if (*p == '.') p++;
        /* All other names beginning with '.' are invalid */
        return (is_end_of_name(*p));
    }
    while (!is_end_of_name(*p))
    {
        if (*p < 256 && strchr( invalid, (char)*p )) return 0;  /* Invalid char */
        if (*p == '.') break;  /* Start of the extension */
        if (++len > 8) return 0;  /* Name too long */
        p++;
    }
    if (*p != '.') return 1;  /* End of name */
    p++;
    if (is_end_of_name(*p)) return 0;  /* Empty extension not allowed */
    len = 0;
    while (!is_end_of_name(*p))
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
static BOOL DOSFS_ToDosFCBFormat( LPCWSTR name, LPWSTR buffer )
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
    return is_end_of_name(*p);
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
static BOOL DOSFS_OpenDir_VFAT(DOS_DIR **dir, const char *unix_path)
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
        MultiByteToWideChar(CP_UNIXCP, 0, de[0].d_name, -1, long_name, MAX_PATH);
        if (!DOSFS_ToDosFCBFormat( long_name, short_name ))
            short_name[0] = '\0';
        if (de[1].d_name[0])
            MultiByteToWideChar(CP_UNIXCP, 0, de[1].d_name, -1, long_name, MAX_PATH);
        else
            MultiByteToWideChar(CP_UNIXCP, 0, de[0].d_name, -1, long_name, MAX_PATH);
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
static BOOL DOSFS_OpenDir_Normal( DOS_DIR **dir, const char *unix_path )
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
        MultiByteToWideChar(CP_UNIXCP, 0, de->d_name, -1, long_name, MAX_PATH);
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
static DOS_DIR *DOSFS_OpenDir( const char *unix_path )
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

    r = DOSFS_OpenDir_VFAT( &dir, unix_path);

    if(!r)
        r = DOSFS_OpenDir_Normal( &dir, unix_path);

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
static void DOSFS_Hash( LPCWSTR name, LPWSTR buffer, BOOL dir_format )
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

    if (DOSFS_ValidDOSName( name ))
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

        for (dst = buffer; !is_end_of_name(*name) && (*name != '.'); name++)
            *dst++ = toupperW(*name);
        if (*name == '.')
        {
            if (dir_format) dst = buffer + 8;
            else *dst++ = '.';
            for (name++; !is_end_of_name(*name); name++)
                *dst++ = toupperW(*name);
        }
        if (!dir_format) *dst = '\0';
        return;
    }

    /* Compute the hash code of the file name */
    /* If you know something about hash functions, feel free to */
    /* insert a better algorithm here... */
    if (!is_case_sensitive)
    {
        for (p = name, hash = 0xbeef; !is_end_of_name(p[1]); p++)
            hash = (hash<<3) ^ (hash>>5) ^ tolowerW(*p) ^ (tolowerW(p[1]) << 8);
        hash = (hash<<3) ^ (hash>>5) ^ tolowerW(*p); /* Last character */
    }
    else
    {
        for (p = name, hash = 0xbeef; !is_end_of_name(p[1]); p++)
            hash = (hash << 3) ^ (hash >> 5) ^ *p ^ (p[1] << 8);
        hash = (hash << 3) ^ (hash >> 5) ^ *p;  /* Last character */
    }

    /* Find last dot for start of the extension */
    for (p = name+1, ext = NULL; !is_end_of_name(*p); p++)
        if (*p == '.') ext = p;
    if (ext && is_end_of_name(ext[1]))
        ext = NULL;  /* Empty extension ignored */

    /* Copy first 4 chars, replacing invalid chars with '_' */
    for (i = 4, p = name, dst = buffer; i > 0; i--, p++)
    {
        if (is_end_of_name(*p) || (p == ext)) break;
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
        for (i = 3, ext++; (i > 0) && !is_end_of_name(*ext); i--, ext++)
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
                         INT long_len, LPWSTR short_buf )
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

    if (!(dir = DOSFS_OpenDir( path->long_name )))
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
            if (is_case_sensitive)
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
                DOSFS_Hash( long_name, tmp_buf, TRUE );
                short_name = tmp_buf;
            }
            if (!strcmpW( dos_name, short_name )) break;
        }
    }
    if (ret)
    {
        if (long_buf) WideCharToMultiByte(CP_UNIXCP, 0, long_name, -1, long_buf, long_len, NULL, NULL);
        if (short_buf)
        {
            if (short_name)
                DOSFS_ToDosDTAFormat( short_name, short_buf );
            else
                DOSFS_Hash( long_name, short_buf, FALSE );
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
            if (is_end_of_name(name[1]))
            {
                name++;
                while ((*name == '\\') || (*name == '/')) name++;
                continue;
            }
            else if ((name[1] == '.') && is_end_of_name(name[2]))
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
                         sizeof(full->long_name) - (p_l - full->long_name) - 1, p_s + 1 )))
        {
            *p_l++ = '/';
            p_l   += strlen(p_l);
            *p_s++ = '\\';
            p_s   += strlenW(p_s);
            while (!is_end_of_name(*name)) name++;
        }
        else if (!check_last)
        {
            *p_l++ = '/';
            *p_s++ = '\\';
            while (!is_end_of_name(*name) &&
                   (p_s < full->short_name + sizeof(full->short_name)/sizeof(full->short_name[0]) - 1) &&
                   (p_l < full->long_name + sizeof(full->long_name) - 1))
            {
                WCHAR wch;
                *p_s++ = tolowerW(*name);
                /* If the drive is case-sensitive we want to create new */
                /* files in lower-case otherwise we can't reopen them   */
                /* under the same short name. */
                if (is_case_sensitive) wch = tolowerW(*name);
                else wch = *name;
                p_l += WideCharToMultiByte(CP_UNIXCP, 0, &wch, 1, p_l, 2, NULL, NULL);
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
 *           wine_get_unix_file_name (KERNEL32.@) Not a Windows API
 *
 * Return the full Unix file name for a given path.
 */
BOOL WINAPI wine_get_unix_file_name( LPCWSTR dosW, LPSTR buffer, DWORD len )
{
    BOOL ret;
    DOS_FULL_NAME path;

    ret = DOSFS_GetFullName( dosW, FALSE, &path );
    if (ret && len)
    {
        strncpy( buffer, path.long_name, len );
        buffer[len - 1] = 0; /* ensure 0 termination */
    }
    return ret;
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
    LONGLONG ret;

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
      ret = (((LONGLONG)nMultiplicand * nMultiplier) + (nDivisor/2)) / nDivisor;
    else
      ret = (((LONGLONG)nMultiplicand * nMultiplier) - (nDivisor/2)) / nDivisor;

    if ((ret > 2147483647) || (ret < -2147483647)) return -1;
    return ret;
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

    li.u.LowPart = ft->dwLowDateTime;
    li.u.HighPart = ft->dwHighDateTime;
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
