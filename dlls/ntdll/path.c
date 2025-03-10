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

#include <stdarg.h>
#include <sys/types.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winioctl.h"
#include "wine/debug.h"
#include "ntdll_misc.h"

WINE_DEFAULT_DEBUG_CHANNEL(file);

#define IS_SEPARATOR(ch)  ((ch) == '\\' || (ch) == '/')

/***********************************************************************
 *             RtlDetermineDosPathNameType_U   (NTDLL.@)
 */
RTL_PATH_TYPE WINAPI RtlDetermineDosPathNameType_U( PCWSTR path )
{
    if (IS_SEPARATOR(path[0]))
    {
        if (!IS_SEPARATOR(path[1])) return RtlPathTypeRooted;                   /* "/foo" */
        if (path[2] != '.' && path[2] != '?') return RtlPathTypeUncAbsolute;    /* "//foo" */
        if (IS_SEPARATOR(path[3])) return RtlPathTypeLocalDevice;               /* "//./foo" or "//?/foo" */
        if (path[3]) return RtlPathTypeUncAbsolute;                             /* "//.foo" or "//?foo" */
        return RtlPathTypeRootLocalDevice;                                      /* "//." or "//?" */
    }
    else
    {
        if (!path[0] || path[1] != ':') return RtlPathTypeRelative; /* "foo" */
        if (IS_SEPARATOR(path[2])) return RtlPathTypeDriveAbsolute; /* "c:/foo" */
        return RtlPathTypeDriveRelative;                            /* "c:foo" */
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
    static const WCHAR auxW[] = {'A','U','X'};
    static const WCHAR comW[] = {'C','O','M'};
    static const WCHAR conW[] = {'C','O','N'};
    static const WCHAR lptW[] = {'L','P','T'};
    static const WCHAR nulW[] = {'N','U','L'};
    static const WCHAR prnW[] = {'P','R','N'};
    static const WCHAR coninW[]  = {'C','O','N','I','N','$'};
    static const WCHAR conoutW[] = {'C','O','N','O','U','T','$'};

    const WCHAR *start, *end, *p;

    switch(RtlDetermineDosPathNameType_U( dos_name ))
    {
    case RtlPathTypeUnknown:
    case RtlPathTypeUncAbsolute:
        return 0;
    case RtlPathTypeLocalDevice:
        if (!wcsicmp( dos_name, L"\\\\.\\CON" ))
            return MAKELONG( sizeof(conW), 4 * sizeof(WCHAR) );  /* 4 is length of \\.\ prefix */
        return 0;
    case RtlPathTypeDriveAbsolute:
    case RtlPathTypeDriveRelative:
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
        if (wcsnicmp( start, auxW, 3 ) &&
            wcsnicmp( start, conW, 3 ) &&
            wcsnicmp( start, nulW, 3 ) &&
            wcsnicmp( start, prnW, 3 )) break;
        return MAKELONG( 3 * sizeof(WCHAR), (start - dos_name) * sizeof(WCHAR) );
    case 4:
        if (wcsnicmp( start, comW, 3 ) && wcsnicmp( start, lptW, 3 )) break;
        if (*end <= '0' || *end > '9') break;
        return MAKELONG( 4 * sizeof(WCHAR), (start - dos_name) * sizeof(WCHAR) );
    case 6:
        if (wcsnicmp( start, coninW, ARRAY_SIZE(coninW) )) break;
        return MAKELONG( sizeof(coninW), (start - dos_name) * sizeof(WCHAR) );
    case 7:
        if (wcsnicmp( start, conoutW, ARRAY_SIZE(conoutW) )) break;
        return MAKELONG( sizeof(conoutW), (start - dos_name) * sizeof(WCHAR) );
    default:  /* can't match anything */
        break;
    }
    return 0;
}

/******************************************************************
 *		is_valid_directory
 *
 * Helper for RtlDosPathNameToNtPathName_U_WithStatus.
 * Test if the path is an existing directory.
 */
static BOOL is_valid_directory(LPCWSTR path)
{
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING ntpath;
    IO_STATUS_BLOCK io;
    HANDLE handle;
    NTSTATUS nts;

    if (!RtlDosPathNameToNtPathName_U(path, &ntpath, NULL, NULL))
        return FALSE;

    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.Attributes = OBJ_CASE_INSENSITIVE;
    attr.ObjectName = &ntpath;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;

    nts = NtOpenFile(&handle, FILE_READ_ATTRIBUTES | SYNCHRONIZE, &attr, &io,
                     FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                     FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT);
    RtlFreeUnicodeString(&ntpath);
    if (nts != STATUS_SUCCESS)
        return FALSE;
    NtClose(handle);
    return TRUE;
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
    static const WCHAR global_prefix[] = {'\\','\\','?','\\'};
    static const WCHAR global_prefix2[] = {'\\','?','?','\\'};
    NTSTATUS nts = STATUS_SUCCESS;
    ULONG sz, offset, dosdev;
    WCHAR local[MAX_PATH];
    LPWSTR ptr = local;

    TRACE("(%s,%p,%p,%p)\n", debugstr_w(dos_path), ntpath, file_part, cd);

    if (cd)
    {
        FIXME("Unsupported parameter\n");
        memset(cd, 0, sizeof(*cd));
    }

    if (!dos_path || !*dos_path)
        return STATUS_OBJECT_NAME_INVALID;

    if (!memcmp(dos_path, global_prefix, sizeof(global_prefix)) ||
        (!memcmp(dos_path, global_prefix2, sizeof(global_prefix2)) && dos_path[4]) ||
        !wcsicmp( dos_path, L"\\\\.\\CON" ))
    {
        ntpath->Length = wcslen(dos_path) * sizeof(WCHAR);
        ntpath->MaximumLength = ntpath->Length + sizeof(WCHAR);
        ntpath->Buffer = RtlAllocateHeap(GetProcessHeap(), 0, ntpath->MaximumLength);
        if (!ntpath->Buffer) return STATUS_NO_MEMORY;
        memcpy( ntpath->Buffer, dos_path, ntpath->MaximumLength );
        ntpath->Buffer[1] = '?';  /* change \\?\ to \??\ */
        ntpath->Buffer[2] = '?';
        if (file_part)
        {
            if ((ptr = wcsrchr( ntpath->Buffer, '\\' )) && ptr[1]) *file_part = ptr + 1;
            else *file_part = NULL;
        }
        return STATUS_SUCCESS;
    }

    dosdev = RtlIsDosDeviceName_U(dos_path);
    if ((offset = HIWORD(dosdev)))
    {
        sz = offset + sizeof(WCHAR);

        if (sz > sizeof(local) &&
            (!(ptr = RtlAllocateHeap(GetProcessHeap(), 0, sz))))
            return STATUS_NO_MEMORY;

        memcpy(ptr, dos_path, offset);
        ptr[offset/sizeof(WCHAR)] = '\0';

        if (!is_valid_directory(ptr))
        {
            nts = STATUS_OBJECT_NAME_INVALID;
            goto out;
        }

        if (file_part) *file_part = NULL;

        sz = LOWORD(dosdev);

        wcscpy(ptr, L"\\\\.\\");
        memcpy(ptr + 4, dos_path + offset / sizeof(WCHAR), sz);
        ptr[4 + sz / sizeof(WCHAR)] = '\0';
        sz += 4 * sizeof(WCHAR);
    }
    else
    {
        sz = RtlGetFullPathName_U(dos_path, sizeof(local), ptr, file_part);
        if (sz == 0) return STATUS_OBJECT_NAME_INVALID;

        if (sz > sizeof(local))
        {
            if (!(ptr = RtlAllocateHeap(GetProcessHeap(), 0, sz))) return STATUS_NO_MEMORY;
            sz = RtlGetFullPathName_U(dos_path, sz, ptr, file_part);
        }
    }
    sz += (1 /* NUL */ + 4 /* unc\ */ + 4 /* \??\ */) * sizeof(WCHAR);
    if (sz > MAXWORD)
    {
        nts = STATUS_OBJECT_NAME_INVALID;
        goto out;
    }

    ntpath->MaximumLength = sz;
    ntpath->Buffer = RtlAllocateHeap(GetProcessHeap(), 0, ntpath->MaximumLength);
    if (!ntpath->Buffer)
    {
        nts = STATUS_NO_MEMORY;
        goto out;
    }

    wcscpy(ntpath->Buffer, L"\\??\\");
    switch (RtlDetermineDosPathNameType_U(ptr))
    {
    case RtlPathTypeUncAbsolute: /* \\foo */
        offset = 2;
        wcscat(ntpath->Buffer, L"UNC\\");
        break;
    case RtlPathTypeLocalDevice: /* \\.\foo */
        offset = 4;
        break;
    default:
        offset = 0;
        break;
    }

    wcscat(ntpath->Buffer, ptr + offset);
    ntpath->Length = wcslen(ntpath->Buffer) * sizeof(WCHAR);

    if (file_part && *file_part)
        *file_part = ntpath->Buffer + ntpath->Length / sizeof(WCHAR) - wcslen(*file_part);

    /* FIXME: cd filling */

out:
    if (ptr != local) RtlFreeHeap(GetProcessHeap(), 0, ptr);
    return nts;
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
 *        RtlDosPathNameToRelativeNtPathName_U [NTDLL.@]
 */
BOOLEAN WINAPI RtlDosPathNameToRelativeNtPathName_U(const WCHAR *dos_path, UNICODE_STRING *ntpath,
    WCHAR **file_part, RTL_RELATIVE_NAME *relative)
{
    return RtlDosPathNameToRelativeNtPathName_U_WithStatus(dos_path, ntpath, file_part, relative) == STATUS_SUCCESS;
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
    RTL_PATH_TYPE type = RtlDetermineDosPathNameType_U(search);
    ULONG len = 0;

    if (type == RtlPathTypeRelative)
    {
        ULONG allocated = 0, needed, filelen;
        WCHAR *name = NULL;

        filelen = 1 /* for \ */ + wcslen(search) + 1 /* \0 */;

        /* Windows only checks for '.' without worrying about path components */
        if (wcschr( search, '.' )) ext = NULL;
        if (ext != NULL) filelen += wcslen(ext);

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
            wcscpy(&name[needed], search);
            if (ext) wcscat(&name[needed], ext);
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
                memmove( p, next, (wcslen(next) + 1) * sizeof(WCHAR) );
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
                    memmove( p, next, (wcslen(next) + 1) * sizeof(WCHAR) );
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
            if (p > path + mark && p[-1] == '.') memmove( p-1, p, (wcslen(p) + 1) * sizeof(WCHAR) );
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
    case RtlPathTypeUncAbsolute:    /* \\foo   */
        ptr = skip_unc_prefix( name );
        mark = (ptr - name);
        break;

    case RtlPathTypeLocalDevice:    /* \\.\foo */
        mark = 4;
        break;

    case RtlPathTypeDriveAbsolute:  /* c:\foo  */
        reqsize = sizeof(WCHAR);
        tmp[0] = name[0];
        ins_str = tmp;
        dep = 1;
        mark = 3;
        break;

    case RtlPathTypeDriveRelative:  /* c:foo   */
        dep = 2;
        if (wcsnicmp( name, cd->Buffer, 2 ))
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

    case RtlPathTypeRelative:   /* foo     */
        reqsize = cd->Length;
        ins_str = cd->Buffer;
        if (cd->Buffer[1] != ':')
        {
            ptr = skip_unc_prefix( cd->Buffer );
            mark = ptr - cd->Buffer;
        }
        else mark = 3;
        break;

    case RtlPathTypeRooted:     /* \xxx    */
        if (name[0] == '/')  /* may be a Unix path */
        {
            char *unix_name;
            WCHAR *nt_str;
            ULONG buflen;
            NTSTATUS status;
            UNICODE_STRING str;
            OBJECT_ATTRIBUTES attr;

            nt_str = RtlAllocateHeap( GetProcessHeap(), 0, (wcslen(name) + 9) * sizeof(WCHAR) );
            wcscpy( nt_str, L"\\??\\unix" );
            wcscat( nt_str, name );
            RtlInitUnicodeString( &str, nt_str );
            InitializeObjectAttributes( &attr, &str, 0, 0, NULL );
            buflen = 3 * wcslen(name) + 1;
            unix_name = RtlAllocateHeap( GetProcessHeap(), 0, buflen );
            status = wine_nt_to_unix_file_name( &attr, unix_name, &buflen, FILE_OPEN_IF );
            if (!status || status == STATUS_NO_SUCH_FILE)
            {
                buflen = wcslen(name) + 9;
                status = wine_unix_to_nt_file_name( unix_name, nt_str, &buflen );
            }
            RtlFreeHeap( GetProcessHeap(), 0, unix_name );
            if (!status && buflen > 6 && nt_str[5] == ':')
            {
                reqsize = (buflen - 4) * sizeof(WCHAR);
                if (reqsize <= size)
                {
                    memcpy( buffer, nt_str + 4, reqsize );
                    collapse_path( buffer, 3 );
                    reqsize -= sizeof(WCHAR);
                }
                RtlFreeHeap( GetProcessHeap(), 0, nt_str );
                goto done;
            }
            RtlFreeHeap( GetProcessHeap(), 0, nt_str );
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

    case RtlPathTypeRootLocalDevice:    /* \\.     */
        reqsize = 4 * sizeof(WCHAR);
        dep = 3;
        tmp[0] = '\\';
        tmp[1] = '\\';
        tmp[2] = '.';
        tmp[3] = '\\';
        ins_str = tmp;
        mark = 4;
        break;

    case RtlPathTypeUnknown:
        goto done;
    }

    /* enough space ? */
    deplen = wcslen(name + dep) * sizeof(WCHAR);
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
    reqsize = wcslen(buffer) * sizeof(WCHAR);

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
        wcscpy(buffer, L"\\\\.\\");
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
    if (file_part && (ptr = wcsrchr(buffer, '\\')) != NULL && ptr >= buffer + 2 && *++ptr)
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
 *             RtlDoesFileExists_U   (NTDLL.@)
 */
BOOLEAN WINAPI RtlDoesFileExists_U(LPCWSTR file_name)
{
    UNICODE_STRING nt_name;
    FILE_BASIC_INFORMATION basic_info;
    OBJECT_ATTRIBUTES attr;
    NTSTATUS status;

    if (!RtlDosPathNameToNtPathName_U( file_name, &nt_name, NULL, NULL )) return FALSE;
    InitializeObjectAttributes( &attr, &nt_name, OBJ_CASE_INSENSITIVE, 0, NULL );
    status = NtQueryAttributesFile(&attr, &basic_info);
    RtlFreeUnicodeString( &nt_name );
    return !status;
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

    TRACE("(%lu %p)\n", buflen, buf);

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

    /* convert \??\UNC\ path to \\ prefix */
    if (size >= 4 && !wcsnicmp(ptr, L"UNC\\", 4))
    {
        ptr += 2;
        size -= 2;
        *ptr = '\\';
    }

    memcpy( curdir->DosPath.Buffer, ptr, size * sizeof(WCHAR));
    curdir->DosPath.Buffer[size] = 0;
    curdir->DosPath.Length = size * sizeof(WCHAR);

    TRACE( "curdir now %s %p\n", debugstr_w(curdir->DosPath.Buffer), curdir->Handle );

 out:
    RtlFreeUnicodeString( &newdir );
    RtlReleasePebLock();
    return nts;
}
