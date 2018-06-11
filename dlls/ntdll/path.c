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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"
#include "wine/port.h"

#include <stdarg.h>
#include <sys/types.h>
#include <errno.h>
#ifdef HAVE_SYS_STAT_H
# include <sys/stat.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winioctl.h"
#include "wine/unicode.h"
#include "wine/debug.h"
#include "wine/library.h"
#include "ntdll_misc.h"

WINE_DEFAULT_DEBUG_CHANNEL(file);

static const WCHAR DeviceRootW[] = {'\\','\\','.','\\',0};
static const WCHAR NTDosPrefixW[] = {'\\','?','?','\\',0};
static const WCHAR UncPfxW[] = {'U','N','C','\\',0};

#define IS_SEPARATOR(ch)  ((ch) == '\\' || (ch) == '/')

/***********************************************************************
 *           remove_last_componentA
 *
 * Remove the last component of the path. Helper for find_drive_rootA.
 */
static inline unsigned int remove_last_componentA( const char *path, unsigned int len )
{
    int level = 0;

    while (level < 1)
    {
        /* find start of the last path component */
        unsigned int prev = len;
        if (prev <= 1) break;  /* reached root */
        while (prev > 1 && path[prev - 1] != '/') prev--;
        /* does removing it take us up a level? */
        if (len - prev != 1 || path[prev] != '.')  /* not '.' */
        {
            if (len - prev == 2 && path[prev] == '.' && path[prev+1] == '.')  /* is it '..'? */
                level--;
            else
                level++;
        }
        /* strip off trailing slashes */
        while (prev > 1 && path[prev - 1] == '/') prev--;
        len = prev;
    }
    return len;
}


/***********************************************************************
 *           find_drive_rootA
 *
 * Find a drive for which the root matches the beginning of the given path.
 * This can be used to translate a Unix path into a drive + DOS path.
 * Return value is the drive, or -1 on error. On success, ppath is modified
 * to point to the beginning of the DOS path.
 */
static NTSTATUS find_drive_rootA( LPCSTR *ppath, unsigned int len, int *drive_ret )
{
    /* Starting with the full path, check if the device and inode match any of
     * the wine 'drives'. If not then remove the last path component and try
     * again. If the last component was a '..' then skip a normal component
     * since it's a directory that's ascended back out of.
     */
    int drive;
    char *buffer;
    const char *path = *ppath;
    struct stat st;
    struct drive_info info[MAX_DOS_DRIVES];

    /* get device and inode of all drives */
    if (!DIR_get_drives_info( info )) return STATUS_OBJECT_PATH_NOT_FOUND;

    /* strip off trailing slashes */
    while (len > 1 && path[len - 1] == '/') len--;

    /* make a copy of the path */
    if (!(buffer = RtlAllocateHeap( GetProcessHeap(), 0, len + 1 ))) return STATUS_NO_MEMORY;
    memcpy( buffer, path, len );
    buffer[len] = 0;

    for (;;)
    {
        if (!stat( buffer, &st ) && S_ISDIR( st.st_mode ))
        {
            /* Find the drive */
            for (drive = 0; drive < MAX_DOS_DRIVES; drive++)
            {
                if ((info[drive].dev == st.st_dev) && (info[drive].ino == st.st_ino))
                {
                    if (len == 1) len = 0;  /* preserve root slash in returned path */
                    TRACE( "%s -> drive %c:, root=%s, name=%s\n",
                           debugstr_a(path), 'A' + drive, debugstr_a(buffer), debugstr_a(path + len));
                    *ppath += len;
                    *drive_ret = drive;
                    RtlFreeHeap( GetProcessHeap(), 0, buffer );
                    return STATUS_SUCCESS;
                }
            }
        }
        if (len <= 1) break;  /* reached root */
        len = remove_last_componentA( buffer, len );
        buffer[len] = 0;
    }
    RtlFreeHeap( GetProcessHeap(), 0, buffer );
    return STATUS_OBJECT_PATH_NOT_FOUND;
}


/***********************************************************************
 *           remove_last_componentW
 *
 * Remove the last component of the path. Helper for find_drive_rootW.
 */
static inline int remove_last_componentW( const WCHAR *path, int len )
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
 *           find_drive_rootW
 *
 * Find a drive for which the root matches the beginning of the given path.
 * This can be used to translate a Unix path into a drive + DOS path.
 * Return value is the drive, or -1 on error. On success, ppath is modified
 * to point to the beginning of the DOS path.
 */
static int find_drive_rootW( LPCWSTR *ppath )
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
    if (!DIR_get_drives_info( info )) return -1;

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
        lenW = remove_last_componentW( path, lenW );

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
    static const WCHAR auxW[] = {'A','U','X'};
    static const WCHAR comW[] = {'C','O','M'};
    static const WCHAR conW[] = {'C','O','N'};
    static const WCHAR lptW[] = {'L','P','T'};
    static const WCHAR nulW[] = {'N','U','L'};
    static const WCHAR prnW[] = {'P','R','N'};

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
    case ABSOLUTE_DRIVE_PATH:
    case RELATIVE_DRIVE_PATH:
        start = dos_name + 2;  /* skip drive letter */
        break;
    default:
        start = dos_name;
        break;
    }

    /* find start of file name */
    for (p = start; *p; p++) if (IS_SEPARATOR(*p)) start = p + 1;

    /* truncate at extension and ':' */
    for (end = start; *end; end++) if (*end == '.' || *end == ':') break;
    end--;

    /* remove trailing spaces */
    while (end >= start && *end == ' ') end--;

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
 *                 RtlDosPathNameToNtPathName_U_WithStatus    [NTDLL.@]
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
NTSTATUS WINAPI RtlDosPathNameToNtPathName_U_WithStatus(const WCHAR *dos_path, UNICODE_STRING *ntpath,
    WCHAR **file_part, CURDIR *cd)
{
    static const WCHAR LongFileNamePfxW[] = {'\\','\\','?','\\'};
    ULONG sz, offset;
    WCHAR local[MAX_PATH];
    LPWSTR ptr;

    TRACE("(%s,%p,%p,%p)\n", debugstr_w(dos_path), ntpath, file_part, cd);

    if (cd)
    {
        FIXME("Unsupported parameter\n");
        memset(cd, 0, sizeof(*cd));
    }

    if (!dos_path || !*dos_path)
        return STATUS_OBJECT_NAME_INVALID;

    if (!strncmpW(dos_path, LongFileNamePfxW, 4))
    {
        ntpath->Length = strlenW(dos_path) * sizeof(WCHAR);
        ntpath->MaximumLength = ntpath->Length + sizeof(WCHAR);
        ntpath->Buffer = RtlAllocateHeap(GetProcessHeap(), 0, ntpath->MaximumLength);
        if (!ntpath->Buffer) return STATUS_NO_MEMORY;
        memcpy( ntpath->Buffer, dos_path, ntpath->MaximumLength );
        ntpath->Buffer[1] = '?';  /* change \\?\ to \??\ */
        if (file_part)
        {
            if ((ptr = strrchrW( ntpath->Buffer, '\\' )) && ptr[1]) *file_part = ptr + 1;
            else *file_part = NULL;
        }
        return STATUS_SUCCESS;
    }

    ptr = local;
    sz = RtlGetFullPathName_U(dos_path, sizeof(local), ptr, file_part);
    if (sz == 0) return STATUS_OBJECT_NAME_INVALID;

    if (sz > sizeof(local))
    {
        if (!(ptr = RtlAllocateHeap(GetProcessHeap(), 0, sz))) return STATUS_NO_MEMORY;
        sz = RtlGetFullPathName_U(dos_path, sz, ptr, file_part);
    }
    sz += (1 /* NUL */ + 4 /* unc\ */ + 4 /* \??\ */) * sizeof(WCHAR);
    if (sz > MAXWORD)
    {
        if (ptr != local) RtlFreeHeap(GetProcessHeap(), 0, ptr);
        return STATUS_OBJECT_NAME_INVALID;
    }

    ntpath->MaximumLength = sz;
    ntpath->Buffer = RtlAllocateHeap(GetProcessHeap(), 0, ntpath->MaximumLength);
    if (!ntpath->Buffer)
    {
        if (ptr != local) RtlFreeHeap(GetProcessHeap(), 0, ptr);
        return STATUS_NO_MEMORY;
    }

    strcpyW(ntpath->Buffer, NTDosPrefixW);
    switch (RtlDetermineDosPathNameType_U(ptr))
    {
    case UNC_PATH: /* \\foo */
        offset = 2;
        strcatW(ntpath->Buffer, UncPfxW);
        break;
    case DEVICE_PATH: /* \\.\foo */
        offset = 4;
        break;
    default:
        offset = 0;
        break;
    }

    strcatW(ntpath->Buffer, ptr + offset);
    ntpath->Length = strlenW(ntpath->Buffer) * sizeof(WCHAR);

    if (file_part && *file_part)
        *file_part = ntpath->Buffer + ntpath->Length / sizeof(WCHAR) - strlenW(*file_part);

    /* FIXME: cd filling */

    if (ptr != local) RtlFreeHeap(GetProcessHeap(), 0, ptr);
    return STATUS_SUCCESS;
}

/**************************************************************************
 *                 RtlDosPathNameToNtPathName_U    [NTDLL.@]
 *
 * See RtlDosPathNameToNtPathName_U_WithStatus
 */
BOOLEAN  WINAPI RtlDosPathNameToNtPathName_U(PCWSTR dos_path,
                                             PUNICODE_STRING ntpath,
                                             PWSTR* file_part,
                                             CURDIR* cd)
{
    return RtlDosPathNameToNtPathName_U_WithStatus(dos_path, ntpath, file_part, cd) == STATUS_SUCCESS;
}

/**************************************************************************
 *        RtlDosPathNameToRelativeNtPathName_U_WithStatus [NTDLL.@]
 *
 * See RtlDosPathNameToNtPathName_U_WithStatus (except the last parameter)
 */
NTSTATUS WINAPI RtlDosPathNameToRelativeNtPathName_U_WithStatus(const WCHAR *dos_path,
    UNICODE_STRING *ntpath, WCHAR **file_part, RTL_RELATIVE_NAME *relative)
{
    TRACE("(%s,%p,%p,%p)\n", debugstr_w(dos_path), ntpath, file_part, relative);

    if (relative)
    {
        FIXME("Unsupported parameter\n");
        memset(relative, 0, sizeof(*relative));
    }

    /* FIXME: fill parameter relative */

    return RtlDosPathNameToNtPathName_U_WithStatus(dos_path, ntpath, file_part, NULL);
}

/**************************************************************************
 *        RtlReleaseRelativeName [NTDLL.@]
 */
void WINAPI RtlReleaseRelativeName(RTL_RELATIVE_NAME *relative)
{
}

/******************************************************************
 *		RtlDosSearchPath_U
 *
 * Searches a file of name 'name' into a ';' separated list of paths
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
        if (*p == '\\')
        {
            /* remove last dot in previous dir name */
            if (p > path + mark && p[-1] == '.') memmove( p-1, p, (strlenW(p) + 1) * sizeof(WCHAR) );
            else p++;
        }
    }

    /* remove trailing spaces and dots (yes, Windows really does that, don't ask) */
    while (p > path + mark && (p[-1] == ' ' || p[-1] == '.')) p--;
    *p = 0;
}


/******************************************************************
 *		skip_unc_prefix
 *
 * Skip the \\share\dir\ part of a file name. Helper for RtlGetFullPathName_U.
 */
static const WCHAR *skip_unc_prefix( const WCHAR *ptr )
{
    ptr += 2;
    while (*ptr && !IS_SEPARATOR(*ptr)) ptr++;  /* share name */
    while (IS_SEPARATOR(*ptr)) ptr++;
    while (*ptr && !IS_SEPARATOR(*ptr)) ptr++;  /* dir name */
    while (IS_SEPARATOR(*ptr)) ptr++;
    return ptr;
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
    LPWSTR                      ins_str = NULL;
    LPCWSTR                     ptr;
    const UNICODE_STRING*       cd;
    WCHAR                       tmp[4];

    /* return error if name only consists of spaces */
    for (ptr = name; *ptr; ptr++) if (*ptr != ' ') break;
    if (!*ptr) return 0;

    RtlAcquirePebLock();

    if (NtCurrentTeb()->Tib.SubSystemTib)  /* FIXME: hack */
        cd = &((WIN16_SUBSYSTEM_TIB *)NtCurrentTeb()->Tib.SubSystemTib)->curdir.DosPath;
    else
        cd = &NtCurrentTeb()->Peb->ProcessParameters->CurrentDirectory.DosPath;

    switch (RtlDetermineDosPathNameType_U(name))
    {
    case UNC_PATH:              /* \\foo   */
        ptr = skip_unc_prefix( name );
        mark = (ptr - name);
        break;

    case DEVICE_PATH:           /* \\.\foo */
        mark = 4;
        break;

    case ABSOLUTE_DRIVE_PATH:   /* c:\foo  */
        reqsize = sizeof(WCHAR);
        tmp[0] = name[0];
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
                /* fall through */
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
                RtlFreeHeap(GetProcessHeap(), 0, val.Buffer);
                break;
            default:
                ERR("Unsupported status code\n");
                RtlFreeHeap(GetProcessHeap(), 0, val.Buffer);
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
            ptr = skip_unc_prefix( cd->Buffer );
            mark = ptr - cd->Buffer;
        }
        else mark = 3;
        break;

    case ABSOLUTE_PATH:         /* \xxx    */
        if (name[0] == '/')  /* may be a Unix path */
        {
            const WCHAR *ptr = name;
            int drive = find_drive_rootW( &ptr );
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
            ptr = skip_unc_prefix( cd->Buffer );
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

    if (ins_str != tmp && ins_str != cd->Buffer)
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

    TRACE("(%s %u %p %p)\n", debugstr_w(name), size, buffer, file_part);

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
    if (!reqsize) return 0;
    if (reqsize > size)
    {
        LPWSTR tmp = RtlAllocateHeap(GetProcessHeap(), 0, reqsize);
        reqsize = get_full_path_helper(name, tmp, reqsize);
        if (reqsize + sizeof(WCHAR) > size)  /* it may have worked the second time */
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
    return MAX_NT_PATH_LENGTH;
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
    static const char illegal[] = "*?<>|\"+=,;[]:/\\\345";
    int dot = -1;
    int i;
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
    if (oem->Length > 0 && oem->Buffer[0] == '.')
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
ULONG WINAPI RtlGetCurrentDirectory_U(ULONG buflen, LPWSTR buf)
{
    UNICODE_STRING*     us;
    ULONG               len;

    TRACE("(%u %p)\n", buflen, buf);

    RtlAcquirePebLock();

    if (NtCurrentTeb()->Tib.SubSystemTib)  /* FIXME: hack */
        us = &((WIN16_SUBSYSTEM_TIB *)NtCurrentTeb()->Tib.SubSystemTib)->curdir.DosPath;
    else
        us = &NtCurrentTeb()->Peb->ProcessParameters->CurrentDirectory.DosPath;

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
    FILE_FS_DEVICE_INFORMATION device_info;
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING newdir;
    IO_STATUS_BLOCK io;
    CURDIR *curdir;
    HANDLE handle;
    NTSTATUS nts;
    ULONG size;
    PWSTR ptr;

    newdir.Buffer = NULL;

    RtlAcquirePebLock();

    if (NtCurrentTeb()->Tib.SubSystemTib)  /* FIXME: hack */
        curdir = &((WIN16_SUBSYSTEM_TIB *)NtCurrentTeb()->Tib.SubSystemTib)->curdir;
    else
        curdir = &NtCurrentTeb()->Peb->ProcessParameters->CurrentDirectory;

    if (!RtlDosPathNameToNtPathName_U( dir->Buffer, &newdir, NULL, NULL ))
    {
        nts = STATUS_OBJECT_NAME_INVALID;
        goto out;
    }

    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.Attributes = OBJ_CASE_INSENSITIVE;
    attr.ObjectName = &newdir;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;

    nts = NtOpenFile( &handle, FILE_TRAVERSE | SYNCHRONIZE, &attr, &io, FILE_SHARE_READ | FILE_SHARE_WRITE,
                      FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT );
    if (nts != STATUS_SUCCESS) goto out;

    /* don't keep the directory handle open on removable media */
    if (!NtQueryVolumeInformationFile( handle, &io, &device_info,
                                       sizeof(device_info), FileFsDeviceInformation ) &&
        (device_info.Characteristics & FILE_REMOVABLE_MEDIA))
    {
        NtClose( handle );
        handle = 0;
    }

    if (curdir->Handle) NtClose( curdir->Handle );
    curdir->Handle = handle;

    /* append trailing \ if missing */
    size = newdir.Length / sizeof(WCHAR);
    ptr = newdir.Buffer;
    ptr += 4;  /* skip \??\ prefix */
    size -= 4;
    if (size && ptr[size - 1] != '\\') ptr[size++] = '\\';

    memcpy( curdir->DosPath.Buffer, ptr, size * sizeof(WCHAR));
    curdir->DosPath.Buffer[size] = 0;
    curdir->DosPath.Length = size * sizeof(WCHAR);

    TRACE( "curdir now %s %p\n", debugstr_w(curdir->DosPath.Buffer), curdir->Handle );

 out:
    RtlFreeUnicodeString( &newdir );
    RtlReleasePebLock();
    return nts;
}


/******************************************************************
 *           wine_unix_to_nt_file_name  (NTDLL.@) Not a Windows API
 */
NTSTATUS CDECL wine_unix_to_nt_file_name( const ANSI_STRING *name, UNICODE_STRING *nt )
{
    static const WCHAR prefixW[] = {'\\','?','?','\\','A',':','\\'};
    static const WCHAR unix_prefixW[] = {'\\','?','?','\\','u','n','i','x'};
    unsigned int lenW, lenA = name->Length;
    const char *path = name->Buffer;
    char *cwd;
    WCHAR *p;
    NTSTATUS status;
    int drive;

    if (!lenA || path[0] != '/')
    {
        char *newcwd, *end;
        size_t size;

        if ((status = DIR_get_unix_cwd( &cwd )) != STATUS_SUCCESS) return status;

        size = strlen(cwd) + lenA + 1;
        if (!(newcwd = RtlReAllocateHeap( GetProcessHeap(), 0, cwd, size )))
        {
            status = STATUS_NO_MEMORY;
            goto done;
        }
        cwd = newcwd;
        end = cwd + strlen(cwd);
        if (end > cwd && end[-1] != '/') *end++ = '/';
        memcpy( end, path, lenA );
        lenA += end - cwd;
        path = cwd;

        status = find_drive_rootA( &path, lenA, &drive );
        lenA -= (path - cwd);
    }
    else
    {
        cwd = NULL;
        status = find_drive_rootA( &path, lenA, &drive );
        lenA -= (path - name->Buffer);
    }

    if (status != STATUS_SUCCESS)
    {
        if (status == STATUS_OBJECT_PATH_NOT_FOUND)
        {
            lenW = ntdll_umbstowcs( 0, path, lenA, NULL, 0 );
            nt->Buffer = RtlAllocateHeap( GetProcessHeap(), 0,
                                          (lenW + 1) * sizeof(WCHAR) + sizeof(unix_prefixW) );
            if (nt->Buffer == NULL)
            {
                status = STATUS_NO_MEMORY;
                goto done;
            }
            memcpy( nt->Buffer, unix_prefixW, sizeof(unix_prefixW) );
            ntdll_umbstowcs( 0, path, lenA, nt->Buffer + sizeof(unix_prefixW)/sizeof(WCHAR), lenW );
            lenW += sizeof(unix_prefixW)/sizeof(WCHAR);
            nt->Buffer[lenW] = 0;
            nt->Length = lenW * sizeof(WCHAR);
            nt->MaximumLength = nt->Length + sizeof(WCHAR);
            for (p = nt->Buffer + sizeof(unix_prefixW)/sizeof(WCHAR); *p; p++) if (*p == '/') *p = '\\';
            status = STATUS_SUCCESS;
        }
        goto done;
    }
    while (lenA && path[0] == '/') { lenA--; path++; }

    lenW = ntdll_umbstowcs( 0, path, lenA, NULL, 0 );
    if (!(nt->Buffer = RtlAllocateHeap( GetProcessHeap(), 0,
                                        (lenW + 1) * sizeof(WCHAR) + sizeof(prefixW) )))
    {
        status = STATUS_NO_MEMORY;
        goto done;
    }

    memcpy( nt->Buffer, prefixW, sizeof(prefixW) );
    nt->Buffer[4] += drive;
    ntdll_umbstowcs( 0, path, lenA, nt->Buffer + sizeof(prefixW)/sizeof(WCHAR), lenW );
    lenW += sizeof(prefixW)/sizeof(WCHAR);
    nt->Buffer[lenW] = 0;
    nt->Length = lenW * sizeof(WCHAR);
    nt->MaximumLength = nt->Length + sizeof(WCHAR);
    for (p = nt->Buffer + sizeof(prefixW)/sizeof(WCHAR); *p; p++) if (*p == '/') *p = '\\';

done:
    RtlFreeHeap( GetProcessHeap(), 0, cwd );
    return status;
}
