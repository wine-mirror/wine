/*
 * Ntdll path functions
 *
 * Copyright 2002, 2003, 2004 Alexandre Julliard
 * Copyright 2003 Eric Pouech
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

#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winternl.h"
#include "wine/unicode.h"
#include "wine/debug.h"
#include "wine/library.h"
#include "ntdll_misc.h"

WINE_DEFAULT_DEBUG_CHANNEL(file);

static const WCHAR DeviceRootW[] = {'\\','\\','.','\\',0};
static const WCHAR NTDosPrefixW[] = {'\\','?','?','\\',0};
static const WCHAR UncPfxW[] = {'U','N','C','\\',0};

/* FIXME: hack! */
HANDLE (WINAPI *pCreateFileW)( LPCWSTR filename, DWORD access, DWORD sharing,
                               LPSECURITY_ATTRIBUTES sa, DWORD creation,
                               DWORD attributes, HANDLE template );

#define IS_SEPARATOR(ch)  ((ch) == '\\' || (ch) == '/')

#define MAX_DOS_DRIVES 26

struct drive_info
{
    dev_t dev;
    ino_t ino;
};

/***********************************************************************
 *           get_drives_info
 *
 * Retrieve device/inode number for all the drives. Helper for find_drive_root.
 */
static inline int get_drives_info( struct drive_info info[MAX_DOS_DRIVES] )
{
    const char *config_dir = wine_get_config_dir();
    char *buffer, *p;
    struct stat st;
    int i, ret;

    buffer = RtlAllocateHeap( GetProcessHeap(), 0, strlen(config_dir) + sizeof("/dosdevices/a:") );
    if (!buffer) return 0;
    strcpy( buffer, config_dir );
    strcat( buffer, "/dosdevices/a:" );
    p = buffer + strlen(buffer) - 2;

    for (i = ret = 0; i < MAX_DOS_DRIVES; i++)
    {
        *p = 'a' + i;
        if (!stat( buffer, &st ))
        {
            info[i].dev = st.st_dev;
            info[i].ino = st.st_ino;
            ret++;
        }
        else
        {
            info[i].dev = 0;
            info[i].ino = 0;
        }
    }
    RtlFreeHeap( GetProcessHeap(), 0, buffer );
    return ret;
}


/***********************************************************************
 *           remove_last_component
 *
 * Remove the last component of the path. Helper for find_drive_root.
 */
static inline int remove_last_component( const WCHAR *path, int len )
{
    int level = 0;

    while (level < 1)
    {
        /* find start of the last path component */
        int prev = len;
        if (prev <= 1) break;  /* reached root */
        while (prev > 1 && !IS_SEPARATOR(path[prev - 1])) prev--;
        /* does removing it take us up a level? */
        if (len - prev != 1 || path[prev] != '.')  /* not '.' */
        {
            if (len - prev == 2 && path[prev] == '.' && path[prev+1] == '.')  /* is it '..'? */
                level--;
            else
                level++;
        }
        /* strip off trailing slashes */
        while (prev > 1 && IS_SEPARATOR(path[prev - 1])) prev--;
        len = prev;
    }
    return len;
}


/***********************************************************************
 *           find_drive_root
 *
 * Find a drive for which the root matches the beginning of the given path.
 * This can be used to translate a Unix path into a drive + DOS path.
 * Return value is the drive, or -1 on error. On success, ppath is modified
 * to point to the beginning of the DOS path.
 */
static int find_drive_root( LPCWSTR *ppath )
{
    /* Starting with the full path, check if the device and inode match any of
     * the wine 'drives'. If not then remove the last path component and try
     * again. If the last component was a '..' then skip a normal component
     * since it's a directory that's ascended back out of.
     */
    int drive, lenA, lenW;
    char *buffer, *p;
    const WCHAR *path = *ppath;
    struct stat st;
    struct drive_info info[MAX_DOS_DRIVES];

    /* get device and inode of all drives */
    if (!get_drives_info( info )) return -1;

    /* strip off trailing slashes */
    lenW = strlenW(path);
    while (lenW > 1 && IS_SEPARATOR(path[lenW - 1])) lenW--;

    /* convert path to Unix encoding */
    lenA = ntdll_wcstoumbs( 0, path, lenW, NULL, 0, NULL, NULL );
    if (!(buffer = RtlAllocateHeap( GetProcessHeap(), 0, lenA + 1 ))) return -1;
    lenA = ntdll_wcstoumbs( 0, path, lenW, buffer, lenA, NULL, NULL );
    buffer[lenA] = 0;
    for (p = buffer; *p; p++) if (*p == '\\') *p = '/';

    for (;;)
    {
        if (!stat( buffer, &st ) && S_ISDIR( st.st_mode ))
        {
            /* Find the drive */
            for (drive = 0; drive < MAX_DOS_DRIVES; drive++)
            {
                if ((info[drive].dev == st.st_dev) && (info[drive].ino == st.st_ino))
                {
                    if (lenW == 1) lenW = 0;  /* preserve root slash in returned path */
                    TRACE( "%s -> drive %c:, root=%s, name=%s\n",
                           debugstr_w(path), 'A' + drive, debugstr_a(buffer), debugstr_w(path + lenW));
                    *ppath += lenW;
                    RtlFreeHeap( GetProcessHeap(), 0, buffer );
                    return drive;
                }
            }
        }
        if (lenW <= 1) break;  /* reached root */
        lenW = remove_last_component( path, lenW );

        /* we only need the new length, buffer already contains the converted string */
        lenA = ntdll_wcstoumbs( 0, path, lenW, NULL, 0, NULL, NULL );
        buffer[lenA] = 0;
    }
    RtlFreeHeap( GetProcessHeap(), 0, buffer );
    return -1;
}


/***********************************************************************
 *             RtlDetermineDosPathNameType_U   (NTDLL.@)
 */
DOS_PATHNAME_TYPE WINAPI RtlDetermineDosPathNameType_U( PCWSTR path )
{
    if (IS_SEPARATOR(path[0]))
    {
        if (!IS_SEPARATOR(path[1])) return ABSOLUTE_PATH;       /* "/foo" */
        if (path[2] != '.') return UNC_PATH;                    /* "//foo" */
        if (IS_SEPARATOR(path[3])) return DEVICE_PATH;          /* "//./foo" */
        if (path[3]) return UNC_PATH;                           /* "//.foo" */
        return UNC_DOT_PATH;                                    /* "//." */
    }
    else
    {
        if (!path[0] || path[1] != ':') return RELATIVE_PATH;   /* "foo" */
        if (IS_SEPARATOR(path[2])) return ABSOLUTE_DRIVE_PATH;  /* "c:/foo" */
        return RELATIVE_DRIVE_PATH;                             /* "c:foo" */
    }
}

/******************************************************************
 *		RtlDoesFileExists_U
 *
 * FIXME: should not use CreateFileW
 */
BOOLEAN WINAPI RtlDoesFileExists_U(LPCWSTR file_name)
{
    HANDLE handle = pCreateFileW( file_name, 0, FILE_SHARE_READ|FILE_SHARE_WRITE,
                                  NULL, OPEN_EXISTING, 0, 0 );
    if (handle == INVALID_HANDLE_VALUE) return FALSE;
    NtClose( handle );
    return TRUE;
}

/***********************************************************************
 *             RtlIsDosDeviceName_U   (NTDLL.@)
 *
 * Check if the given DOS path contains a DOS device name.
 *
 * Returns the length of the device name in the low word and its
 * position in the high word (both in bytes, not WCHARs), or 0 if no
 * device name is found.
 */
ULONG WINAPI RtlIsDosDeviceName_U( PCWSTR dos_name )
{
    static const WCHAR consoleW[] = {'\\','\\','.','\\','C','O','N',0};
    static const WCHAR auxW[3] = {'A','U','X'};
    static const WCHAR comW[3] = {'C','O','M'};
    static const WCHAR conW[3] = {'C','O','N'};
    static const WCHAR lptW[3] = {'L','P','T'};
    static const WCHAR nulW[3] = {'N','U','L'};
    static const WCHAR prnW[3] = {'P','R','N'};

    const WCHAR *start, *end, *p;

    switch(RtlDetermineDosPathNameType_U( dos_name ))
    {
    case INVALID_PATH:
    case UNC_PATH:
        return 0;
    case DEVICE_PATH:
        if (!strcmpiW( dos_name, consoleW ))
            return MAKELONG( sizeof(conW), 4 * sizeof(WCHAR) );  /* 4 is length of \\.\ prefix */
        return 0;
    default:
        break;
    }

    end = dos_name + strlenW(dos_name) - 1;
    if (end >= dos_name && *end == ':') end--;  /* remove trailing ':' */

    /* find start of file name */
    for (start = end; start >= dos_name; start--)
    {
        if (IS_SEPARATOR(start[0])) break;
        /* check for ':' but ignore if before extension (for things like NUL:.txt) */
        if (start[0] == ':' && start[1] != '.') break;
    }
    start++;

    /* remove extension */
    if ((p = strchrW( start, '.' )))
    {
        end = p - 1;
        if (end >= dos_name && *end == ':') end--;  /* remove trailing ':' before extension */
    }
    else
    {
        /* no extension, remove trailing spaces */
        while (end >= dos_name && *end == ' ') end--;
    }

    /* now we have a potential device name between start and end, check it */
    switch(end - start + 1)
    {
    case 3:
        if (strncmpiW( start, auxW, 3 ) &&
            strncmpiW( start, conW, 3 ) &&
            strncmpiW( start, nulW, 3 ) &&
            strncmpiW( start, prnW, 3 )) break;
        return MAKELONG( 3 * sizeof(WCHAR), (start - dos_name) * sizeof(WCHAR) );
    case 4:
        if (strncmpiW( start, comW, 3 ) && strncmpiW( start, lptW, 3 )) break;
        if (*end <= '0' || *end > '9') break;
        return MAKELONG( 4 * sizeof(WCHAR), (start - dos_name) * sizeof(WCHAR) );
    default:  /* can't match anything */
        break;
    }
    return 0;
}


/**************************************************************************
 *                 RtlDosPathNameToNtPathName_U		[NTDLL.@]
 *
 * dos_path: a DOS path name (fully qualified or not)
 * ntpath:   pointer to a UNICODE_STRING to hold the converted
 *           path name
 * file_part:will point (in ntpath) to the file part in the path
 * cd:       directory reference (optional)
 *
 * FIXME:
 *      + fill the cd structure
 */
BOOLEAN  WINAPI RtlDosPathNameToNtPathName_U(PWSTR dos_path,
                                             PUNICODE_STRING ntpath,
                                             PWSTR* file_part,
                                             CURDIR* cd)
{
    static const WCHAR LongFileNamePfxW[4] = {'\\','\\','?','\\'};
    ULONG sz, ptr_sz, offset;
    WCHAR local[MAX_PATH];
    LPWSTR ptr;

    TRACE("(%s,%p,%p,%p)\n",
          debugstr_w(dos_path), ntpath, file_part, cd);

    if (cd)
    {
        FIXME("Unsupported parameter\n");
        memset(cd, 0, sizeof(*cd));
    }

    if (!dos_path || !*dos_path) return FALSE;

    if (!memcmp(dos_path, LongFileNamePfxW, sizeof(LongFileNamePfxW)))
    {
        dos_path += sizeof(LongFileNamePfxW) / sizeof(WCHAR);
        ptr = NULL;
        ptr_sz = 0;
    }
    else
    {
        ptr = local;
        ptr_sz = sizeof(local);
    }
    sz = RtlGetFullPathName_U(dos_path, ptr_sz, ptr, file_part);
    if (sz == 0) return FALSE;
    if (sz > ptr_sz)
    {
        ptr = RtlAllocateHeap(GetProcessHeap(), 0, sz);
        sz = RtlGetFullPathName_U(dos_path, sz, ptr, file_part);
    }

    ntpath->MaximumLength = sz + (4 /* unc\ */ + 4 /* \??\ */) * sizeof(WCHAR);
    ntpath->Buffer = RtlAllocateHeap(GetProcessHeap(), 0, ntpath->MaximumLength);
    if (!ntpath->Buffer)
    {
        if (ptr != local) RtlFreeHeap(GetProcessHeap(), 0, ptr);
        return FALSE;
    }

    strcpyW(ntpath->Buffer, NTDosPrefixW);
    offset = 0;
    switch (RtlDetermineDosPathNameType_U(ptr))
    {
    case UNC_PATH: /* \\foo */
        if (ptr[2] != '?')
        {
            offset = 2;
            strcatW(ntpath->Buffer, UncPfxW);
        }
        break;
    case DEVICE_PATH: /* \\.\foo */
        offset = 4;
        break;
    default: break; /* needed to keep gcc quiet */
    }

    strcatW(ntpath->Buffer, ptr + offset);
    ntpath->Length = strlenW(ntpath->Buffer) * sizeof(WCHAR);

    if (file_part && *file_part)
        *file_part = ntpath->Buffer + ntpath->Length / sizeof(WCHAR) - strlenW(*file_part);

    /* FIXME: cd filling */

    if (ptr != local) RtlFreeHeap(GetProcessHeap(), 0, ptr);
    return TRUE;
}

/******************************************************************
 *		RtlDosSearchPath_U
 *
 * Searchs a file of name 'name' into a ';' separated list of paths
 * (stored in paths)
 * Doesn't seem to search elsewhere than the paths list
 * Stores the result in buffer (file_part will point to the position
 * of the file name in the buffer)
 * FIXME:
 * - how long shall the paths be ??? (MAX_PATH or larger with \\?\ constructs ???)
 */
ULONG WINAPI RtlDosSearchPath_U(LPCWSTR paths, LPCWSTR search, LPCWSTR ext, 
                                ULONG buffer_size, LPWSTR buffer, 
                                LPWSTR* file_part)
{
    DOS_PATHNAME_TYPE type = RtlDetermineDosPathNameType_U(search);
    ULONG len = 0;

    if (type == RELATIVE_PATH)
    {
        ULONG allocated = 0, needed, filelen;
        WCHAR *name = NULL;

        filelen = 1 /* for \ */ + strlenW(search) + 1 /* \0 */;

        /* Windows only checks for '.' without worrying about path components */
        if (strchrW( search, '.' )) ext = NULL;
        if (ext != NULL) filelen += strlenW(ext);

        while (*paths)
        {
            LPCWSTR ptr;

            for (needed = 0, ptr = paths; *ptr != 0 && *ptr++ != ';'; needed++);
            if (needed + filelen > allocated)
            {
                if (!name) name = RtlAllocateHeap(GetProcessHeap(), 0,
                                                  (needed + filelen) * sizeof(WCHAR));
                else
                {
                    WCHAR *newname = RtlReAllocateHeap(GetProcessHeap(), 0, name,
                                                       (needed + filelen) * sizeof(WCHAR));
                    if (!newname) RtlFreeHeap(GetProcessHeap(), 0, name);
                    name = newname;
                }
                if (!name) return 0;
                allocated = needed + filelen;
            }
            memmove(name, paths, needed * sizeof(WCHAR));
            /* append '\\' if none is present */
            if (needed > 0 && name[needed - 1] != '\\') name[needed++] = '\\';
            strcpyW(&name[needed], search);
            if (ext) strcatW(&name[needed], ext);
            if (RtlDoesFileExists_U(name))
            {
                len = RtlGetFullPathName_U(name, buffer_size, buffer, file_part);
                break;
            }
            paths = ptr;
        }
        RtlFreeHeap(GetProcessHeap(), 0, name);
    }
    else if (RtlDoesFileExists_U(search))
    {
        len = RtlGetFullPathName_U(search, buffer_size, buffer, file_part);
    }

    return len;
}


/******************************************************************
 *		collapse_path
 *
 * Helper for RtlGetFullPathName_U.
 * Get rid of . and .. components in the path.
 */
static inline void collapse_path( WCHAR *path, UINT mark )
{
    WCHAR *p, *next;

    /* convert every / into a \ */
    for (p = path; *p; p++) if (*p == '/') *p = '\\';

    /* collapse duplicate backslashes */
    next = path + max( 1, mark );
    for (p = next; *p; p++) if (*p != '\\' || next[-1] != '\\') *next++ = *p;
    *next = 0;

    p = path + mark;
    while (*p)
    {
        if (*p == '.')
        {
            switch(p[1])
            {
            case '\\': /* .\ component */
                next = p + 2;
                memmove( p, next, (strlenW(next) + 1) * sizeof(WCHAR) );
                continue;
            case 0:  /* final . */
                if (p > path + mark) p--;
                *p = 0;
                continue;
            case '.':
                if (p[2] == '\\')  /* ..\ component */
                {
                    next = p + 3;
                    if (p > path + mark)
                    {
                        p--;
                        while (p > path + mark && p[-1] != '\\') p--;
                    }
                    memmove( p, next, (strlenW(next) + 1) * sizeof(WCHAR) );
                    continue;
                }
                else if (!p[2])  /* final .. */
                {
                    if (p > path + mark)
                    {
                        p--;
                        while (p > path + mark && p[-1] != '\\') p--;
                        if (p > path + mark) p--;
                    }
                    *p = 0;
                    continue;
                }
                break;
            }
        }
        /* skip to the next component */
        while (*p && *p != '\\') p++;
        if (*p == '\\') p++;
    }
}

/******************************************************************
 *		get_full_path_helper
 *
 * Helper for RtlGetFullPathName_U
 * Note: name and buffer are allowed to point to the same memory spot
 */
static ULONG get_full_path_helper(LPCWSTR name, LPWSTR buffer, ULONG size)
{
    ULONG                       reqsize = 0, mark = 0, dep = 0, deplen;
    DOS_PATHNAME_TYPE           type;
    LPWSTR                      ins_str = NULL;
    LPCWSTR                     ptr;
    const UNICODE_STRING*       cd;
    WCHAR                       tmp[4];

    RtlAcquirePebLock();

    cd = &NtCurrentTeb()->Peb->ProcessParameters->CurrentDirectoryName;

    switch (type = RtlDetermineDosPathNameType_U(name))
    {
    case UNC_PATH:              /* \\foo   */
        ptr = name + 2;
        while (*ptr && !IS_SEPARATOR(*ptr)) ptr++;  /* share name */
        while (*ptr && IS_SEPARATOR(*ptr)) ptr++;
        while (*ptr && !IS_SEPARATOR(*ptr)) ptr++;  /* dir name */
        mark = (ptr - name);
        break;

    case DEVICE_PATH:           /* \\.\foo */
        mark = 4;
        break;

    case ABSOLUTE_DRIVE_PATH:   /* c:\foo  */
        reqsize = sizeof(WCHAR);
        tmp[0] = toupperW(name[0]);
        ins_str = tmp;
        dep = 1;
        mark = 3;
        break;

    case RELATIVE_DRIVE_PATH:   /* c:foo   */
        dep = 2;
        if (toupperW(name[0]) != toupperW(cd->Buffer[0]) || cd->Buffer[1] != ':')
        {
            UNICODE_STRING      var, val;

            tmp[0] = '=';
            tmp[1] = name[0];
            tmp[2] = ':';
            tmp[3] = '\0';
            var.Length = 3 * sizeof(WCHAR);
            var.MaximumLength = 4 * sizeof(WCHAR);
            var.Buffer = tmp;
            val.Length = 0;
            val.MaximumLength = size;
            val.Buffer = RtlAllocateHeap(GetProcessHeap(), 0, size);

            switch (RtlQueryEnvironmentVariable_U(NULL, &var, &val))
            {
            case STATUS_SUCCESS:
                /* FIXME: Win2k seems to check that the environment variable actually points 
                 * to an existing directory. If not, root of the drive is used
                 * (this seems also to be the only spot in RtlGetFullPathName that the 
                 * existence of a part of a path is checked)
                 */
                /* fall thru */
            case STATUS_BUFFER_TOO_SMALL:
                reqsize = val.Length + sizeof(WCHAR); /* append trailing '\\' */
                val.Buffer[val.Length / sizeof(WCHAR)] = '\\';
                ins_str = val.Buffer;
                break;
            case STATUS_VARIABLE_NOT_FOUND:
                reqsize = 3 * sizeof(WCHAR);
                tmp[0] = name[0];
                tmp[1] = ':';
                tmp[2] = '\\';
                ins_str = tmp;
                break;
            default:
                ERR("Unsupported status code\n");
                break;
            }
            mark = 3;
            break;
        }
        /* fall through */

    case RELATIVE_PATH:         /* foo     */
        reqsize = cd->Length;
        ins_str = cd->Buffer;
        if (cd->Buffer[1] != ':')
        {
            ptr = strchrW(cd->Buffer + 2, '\\');
            if (ptr) ptr = strchrW(ptr + 1, '\\');
            if (!ptr) ptr = cd->Buffer + strlenW(cd->Buffer);
            mark = ptr - cd->Buffer;
        }
        else mark = 3;
        break;

    case ABSOLUTE_PATH:         /* \xxx    */
        if (name[0] == '/')  /* may be a Unix path */
        {
            const WCHAR *ptr = name;
            int drive = find_drive_root( &ptr );
            if (drive != -1)
            {
                reqsize = 3 * sizeof(WCHAR);
                tmp[0] = 'A' + drive;
                tmp[1] = ':';
                tmp[2] = '\\';
                ins_str = tmp;
                mark = 3;
                dep = ptr - name;
                break;
            }
        }
        if (cd->Buffer[1] == ':')
        {
            reqsize = 2 * sizeof(WCHAR);
            tmp[0] = cd->Buffer[0];
            tmp[1] = ':';
            ins_str = tmp;
            mark = 3;
        }
        else
        {
            ptr = strchrW(cd->Buffer + 2, '\\');
            if (ptr) ptr = strchrW(ptr + 1, '\\');
            if (!ptr) ptr = cd->Buffer + strlenW(cd->Buffer);
            reqsize = (ptr - cd->Buffer) * sizeof(WCHAR);
            mark = reqsize / sizeof(WCHAR);
            ins_str = cd->Buffer;
        }
        break;

    case UNC_DOT_PATH:         /* \\.     */
        reqsize = 4 * sizeof(WCHAR);
        dep = 3;
        tmp[0] = '\\';
        tmp[1] = '\\';
        tmp[2] = '.';
        tmp[3] = '\\';
        ins_str = tmp;
        mark = 4;
        break;

    case INVALID_PATH:
        goto done;
    }

    /* enough space ? */
    deplen = strlenW(name + dep) * sizeof(WCHAR);
    if (reqsize + deplen + sizeof(WCHAR) > size)
    {
        /* not enough space, return need size (including terminating '\0') */
        reqsize += deplen + sizeof(WCHAR);
        goto done;
    }

    memmove(buffer + reqsize / sizeof(WCHAR), name + dep, deplen + sizeof(WCHAR));
    if (reqsize) memcpy(buffer, ins_str, reqsize);
    reqsize += deplen;

    if (ins_str && ins_str != tmp && ins_str != cd->Buffer)
        RtlFreeHeap(GetProcessHeap(), 0, ins_str);

    collapse_path( buffer, mark );
    reqsize = strlenW(buffer) * sizeof(WCHAR);

done:
    RtlReleasePebLock();
    return reqsize;
}

/******************************************************************
 *		RtlGetFullPathName_U  (NTDLL.@)
 *
 * Returns the number of bytes written to buffer (not including the
 * terminating NULL) if the function succeeds, or the required number of bytes
 * (including the terminating NULL) if the buffer is too small.
 *
 * file_part will point to the filename part inside buffer (except if we use
 * DOS device name, in which case file_in_buf is NULL)
 *
 */
DWORD WINAPI RtlGetFullPathName_U(const WCHAR* name, ULONG size, WCHAR* buffer,
                                  WCHAR** file_part)
{
    WCHAR*      ptr;
    DWORD       dosdev;
    DWORD       reqsize;

    TRACE("(%s %lu %p %p)\n", debugstr_w(name), size, buffer, file_part);

    if (!name || !*name) return 0;

    if (file_part) *file_part = NULL;

    /* check for DOS device name */
    dosdev = RtlIsDosDeviceName_U(name);
    if (dosdev)
    {
        DWORD   offset = HIWORD(dosdev) / sizeof(WCHAR); /* get it in WCHARs, not bytes */
        DWORD   sz = LOWORD(dosdev); /* in bytes */

        if (8 + sz + 2 > size) return sz + 10;
        strcpyW(buffer, DeviceRootW);
        memmove(buffer + 4, name + offset, sz);
        buffer[4 + sz / sizeof(WCHAR)] = '\0';
        /* file_part isn't set in this case */
        return sz + 8;
    }

    reqsize = get_full_path_helper(name, buffer, size);
    if (reqsize > size)
    {
        LPWSTR tmp = RtlAllocateHeap(GetProcessHeap(), 0, reqsize);
        reqsize = get_full_path_helper(name, tmp, reqsize);
        if (reqsize > size)  /* it may have worked the second time */
        {
            RtlFreeHeap(GetProcessHeap(), 0, tmp);
            return reqsize + sizeof(WCHAR);
        }
        memcpy( buffer, tmp, reqsize + sizeof(WCHAR) );
        RtlFreeHeap(GetProcessHeap(), 0, tmp);
    }

    /* find file part */
    if (file_part && (ptr = strrchrW(buffer, '\\')) != NULL && ptr >= buffer + 2 && *++ptr)
        *file_part = ptr;
    return reqsize;
}

/*************************************************************************
 * RtlGetLongestNtPathLength    [NTDLL.@]
 *
 * Get the longest allowed path length
 *
 * PARAMS
 *  None.
 *
 * RETURNS
 *  The longest allowed path length (277 characters under Win2k).
 */
DWORD WINAPI RtlGetLongestNtPathLength(void)
{
    return 277;
}

/******************************************************************
 *             RtlIsNameLegalDOS8Dot3   (NTDLL.@)
 *
 * Returns TRUE iff unicode is a valid DOS (8+3) name.
 * If the name is valid, oem gets filled with the corresponding OEM string
 * spaces is set to TRUE if unicode contains spaces
 */
BOOLEAN WINAPI RtlIsNameLegalDOS8Dot3( const UNICODE_STRING *unicode,
                                       OEM_STRING *oem, BOOLEAN *spaces )
{
    static const char* illegal = "*?<>|\"+=,;[]:/\\\345";
    int dot = -1;
    unsigned int i;
    char buffer[12];
    OEM_STRING oem_str;
    BOOLEAN got_space = FALSE;

    if (!oem)
    {
        oem_str.Length = sizeof(buffer);
        oem_str.MaximumLength = sizeof(buffer);
        oem_str.Buffer = buffer;
        oem = &oem_str;
    }
    if (RtlUpcaseUnicodeStringToCountedOemString( oem, unicode, FALSE ) != STATUS_SUCCESS)
        return FALSE;

    if (oem->Length > 12) return FALSE;

    /* a starting . is invalid, except for . and .. */
    if (oem->Buffer[0] == '.')
    {
        if (oem->Length != 1 && (oem->Length != 2 || oem->Buffer[1] != '.')) return FALSE;
        if (spaces) *spaces = FALSE;
        return TRUE;
    }

    for (i = 0; i < oem->Length; i++)
    {
        switch (oem->Buffer[i])
        {
        case ' ':
            /* leading/trailing spaces not allowed */
            if (!i || i == oem->Length-1 || oem->Buffer[i+1] == '.') return FALSE;
            got_space = TRUE;
            break;
        case '.':
            if (dot != -1) return FALSE;
            dot = i;
            break;
        default:
            if (strchr(illegal, oem->Buffer[i])) return FALSE;
            break;
        }
    }
    /* check file part is shorter than 8, extension shorter than 3
     * dot cannot be last in string
     */
    if (dot == -1)
    {
        if (oem->Length > 8) return FALSE;
    }
    else
    {
        if (dot > 8 || (oem->Length - dot > 4) || dot == oem->Length - 1) return FALSE;
    }
    if (spaces) *spaces = got_space;
    return TRUE;
}

/******************************************************************
 *		RtlGetCurrentDirectory_U (NTDLL.@)
 *
 */
NTSTATUS WINAPI RtlGetCurrentDirectory_U(ULONG buflen, LPWSTR buf)
{
    UNICODE_STRING*     us;
    ULONG               len;

    TRACE("(%lu %p)\n", buflen, buf);

    RtlAcquirePebLock();

    us = &NtCurrentTeb()->Peb->ProcessParameters->CurrentDirectoryName;
    len = us->Length / sizeof(WCHAR);
    if (us->Buffer[len - 1] == '\\' && us->Buffer[len - 2] != ':')
        len--;

    if (buflen / sizeof(WCHAR) > len)
    {
        memcpy(buf, us->Buffer, len * sizeof(WCHAR));
        buf[len] = '\0';
    }
    else
    {
        len++;
    }

    RtlReleasePebLock();

    return len * sizeof(WCHAR);
}

/******************************************************************
 *		RtlSetCurrentDirectory_U (NTDLL.@)
 *
 */
NTSTATUS WINAPI RtlSetCurrentDirectory_U(const UNICODE_STRING* dir)
{
    UNICODE_STRING*     curdir;
    NTSTATUS            nts = STATUS_SUCCESS;
    ULONG               size;
    PWSTR               buf = NULL;

    TRACE("(%s)\n", debugstr_w(dir->Buffer));

    RtlAcquirePebLock();

    curdir = &NtCurrentTeb()->Peb->ProcessParameters->CurrentDirectoryName;
    size = curdir->MaximumLength;

    buf = RtlAllocateHeap(GetProcessHeap(), 0, size);
    if (buf == NULL)
    {
        nts = STATUS_NO_MEMORY;
        goto out;
    }

    size = RtlGetFullPathName_U(dir->Buffer, size, buf, 0);
    if (!size)
    {
        nts = STATUS_OBJECT_NAME_INVALID;
        goto out;
    }

    switch (RtlDetermineDosPathNameType_U(buf))
    {
    case ABSOLUTE_DRIVE_PATH:
    case UNC_PATH:
        break;
    default:
        FIXME("Don't support those cases yes\n");
        nts = STATUS_NOT_IMPLEMENTED;
        goto out;
    }

    /* FIXME: should check that the directory actually exists,
     * and fill CurrentDirectoryHandle accordingly 
     */

    /* append trailing \ if missing */
    if (buf[size / sizeof(WCHAR) - 1] != '\\')
    {
        buf[size / sizeof(WCHAR)] = '\\';
        buf[size / sizeof(WCHAR) + 1] = '\0';
        size += sizeof(WCHAR);
    }

    memmove(curdir->Buffer, buf, size + sizeof(WCHAR));
    curdir->Length = size;

#if 0
    if (curdir->Buffer[1] == ':')
    {
        UNICODE_STRING  env;
        WCHAR           var[4];

        var[0] = '=';
        var[1] = curdir->Buffer[0];
        var[2] = ':';
        var[3] = 0;
        env.Length = 3 * sizeof(WCHAR);
        env.MaximumLength = 4 * sizeof(WCHAR);
        env.Buffer = var;

        RtlSetEnvironmentVariable(NULL, &env, curdir);
    }
#endif

 out:
    if (buf) RtlFreeHeap(GetProcessHeap(), 0, buf);

    RtlReleasePebLock();

    return nts;
}
