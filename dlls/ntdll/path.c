/*
 * Ntdll path functions
 *
 * Copyright 2002, 2003 Alexandre Julliard
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

#include "winternl.h"
#include "wine/unicode.h"
#include "wine/debug.h"
#include "ntdll_misc.h"

WINE_DEFAULT_DEBUG_CHANNEL(file);

static const WCHAR DeviceRootW[] = {'\\','\\','.','\\',0};
static const WCHAR NTDosPrefixW[] = {'\\','?','?','\\',0};
static const WCHAR UncPfxW[] = {'U','N','C','\\',0};

#define IS_SEPARATOR(ch)  ((ch) == '\\' || (ch) == '/')

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
 *
 */
BOOLEAN WINAPI RtlDoesFileExists_U(LPCWSTR file_name)
{       
    FIXME("(%s): stub\n", debugstr_w(file_name));
    
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
        ptr = RtlAllocateHeap(ntdll_get_process_heap(), 0, sz);
        sz = RtlGetFullPathName_U(dos_path, sz, ptr, file_part);
    }

    ntpath->MaximumLength = sz + (4 /* unc\ */ + 4 /* \??\ */) * sizeof(WCHAR);
    ntpath->Buffer = RtlAllocateHeap(ntdll_get_process_heap(), 0, ntpath->MaximumLength);
    if (!ntpath->Buffer)
    {
        if (ptr != local) RtlFreeHeap(ntdll_get_process_heap(), 0, ptr);
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
        *file_part = ntpath->Buffer + ntpath->Length / sizeof(WCHAR) - lstrlenW(*file_part);

    /* FIXME: cd filling */

    if (ptr != local) RtlFreeHeap(ntdll_get_process_heap(), 0, ptr);
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
        WCHAR*  name = NULL;

        filelen = 1 /* for \ */ + strlenW(search) + 1 /* \0 */;

        if (strchrW(search, '.') != NULL) ext = NULL;
        if (ext != NULL) filelen += strlenW(ext);

        while (*paths)
        {
            LPCWSTR ptr;

            for (needed = 0, ptr = paths; *ptr != 0 && *ptr++ != ';'; needed++);
            if (needed + filelen > allocated)
            {
                name = (WCHAR*)RtlReAllocateHeap(ntdll_get_process_heap(), 0,
                                                 name, (needed + filelen) * sizeof(WCHAR));
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
        RtlFreeHeap(ntdll_get_process_heap(), 0, name);
    }
    else if (RtlDoesFileExists_U(search))
    {
        len = RtlGetFullPathName_U(search, buffer_size, buffer, file_part);
    }

    return len;
}

/******************************************************************
 *		get_full_path_helper
 *
 * Helper for RtlGetFullPathName_U
 */
static ULONG get_full_path_helper(LPCWSTR name, LPWSTR buffer, ULONG size)
{
    ULONG               reqsize, mark = 0;
    DOS_PATHNAME_TYPE   type;
    LPWSTR              ptr;
    UNICODE_STRING*     cd;

    reqsize = sizeof(WCHAR); /* '\0' at the end */

    RtlAcquirePebLock();
    cd = &ntdll_get_process_pmts()->CurrentDirectoryName;

    switch (type = RtlDetermineDosPathNameType_U(name))
    {
    case UNC_PATH:              /* \\foo   */
    case DEVICE_PATH:           /* \\.\foo */
        if (reqsize <= size) buffer[0] = '\0';
        break;

    case ABSOLUTE_DRIVE_PATH:   /* c:\foo  */
        reqsize += sizeof(WCHAR);
        if (reqsize <= size)
        {
            buffer[0] = toupperW(name[0]);
            buffer[1] = '\0';
        }
        name++;
        break;

    case RELATIVE_DRIVE_PATH:   /* c:foo   */
        if (toupperW(name[0]) != toupperW(cd->Buffer[0]) || cd->Buffer[1] != ':')
        {
            WCHAR               drive[4];
            UNICODE_STRING      var, val;

            drive[0] = '=';
            drive[1] = name[0];
            drive[2] = ':';
            drive[3] = '\0';
            var.Length = 6;
            var.MaximumLength = 8;
            var.Buffer = drive;
            val.Length = 0;
            val.MaximumLength = size;
            val.Buffer = buffer;

            name += 2;

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
                reqsize += val.Length;
                /* append trailing \\ */
                reqsize += sizeof(WCHAR);
                if (reqsize <= size)
                {
                    buffer[reqsize / sizeof(WCHAR) - 2] = '\\';
                    buffer[reqsize / sizeof(WCHAR) - 1] = '\0';
                }
                break;
            case STATUS_VARIABLE_NOT_FOUND:
                reqsize += 3 * sizeof(WCHAR);
                if (reqsize <= size)
                {
                    buffer[0] = drive[1];
                    buffer[1] = ':';
                    buffer[2] = '\\';
                    buffer[3] = '\0';
                }
                break;
            default:
                ERR("Unsupported status code\n");
                break;
            }
            break;
        }
        name += 2;
        /* fall through */

    case RELATIVE_PATH:         /* foo     */
        reqsize += cd->Length;
        mark = cd->Length / sizeof(WCHAR);
        if (reqsize <= size)
            strcpyW(buffer, cd->Buffer);
        break;

    case ABSOLUTE_PATH:         /* \xxx    */
        if (cd->Buffer[1] == ':')
        {
            reqsize += 2 * sizeof(WCHAR);
            if (reqsize <= size)
            {
                buffer[0] = cd->Buffer[0];
                buffer[1] = ':';
                buffer[2] = '\0';
            }
        }
        else
        {
            unsigned    len;

            ptr = strchrW(cd->Buffer + 2, '\\');
            if (ptr) ptr = strchrW(ptr + 1, '\\');
            if (!ptr) ptr = cd->Buffer + strlenW(cd->Buffer);
            len = (ptr - cd->Buffer) * sizeof(WCHAR);
            reqsize += len;
            mark = len / sizeof(WCHAR);
            if (reqsize <= size)
            {
                memcpy(buffer, cd->Buffer, len);
                buffer[len / sizeof(WCHAR)] = '\0';
            }
            else
                buffer[0] = '\0';
        }
        break;

    case UNC_DOT_PATH:         /* \\.     */
        reqsize += 4 * sizeof(WCHAR);
        name += 3;
        if (reqsize <= size)
        {
            buffer[0] = '\\';
            buffer[1] = '\\';
            buffer[2] = '.';
            buffer[3] = '\\';
            buffer[4] = '\0';
        }
        break;

    case INVALID_PATH:
        reqsize = 0;
        goto done;
    }

    reqsize += strlenW(name) * sizeof(WCHAR);
    if (reqsize > size) goto done;

    strcatW(buffer, name);

    /* convert every / into a \ */
    for (ptr = buffer; ptr < buffer + size / sizeof(WCHAR); ptr++) 
        if (*ptr == '/') *ptr = '\\';

    reqsize -= sizeof(WCHAR); /* don't count trailing \0 */

    /* mark is non NULL for UNC names, so start path collapsing after server & share name
     * otherwise, it's a fully qualified DOS name, so start after the drive designation
     */
    for (ptr = buffer + (mark ? mark : 2); ptr < buffer + reqsize / sizeof(WCHAR); )
    {
        WCHAR* p = strchrW(ptr, '\\');
        if (!p) break;

        p++;
        if (p[0] == '.')
        {
            switch (p[1])
            {
            case '.':
                switch (p[2])
                {
                case '\\':
                    {
                        WCHAR*      prev = p - 2;
                        while (prev >= buffer + mark && *prev != '\\') prev--;
                        /* either collapse \foo\.. into \ or \.. into \ */
                        if (prev < buffer + mark) prev = p - 1;
                        reqsize -= (p + 2 - prev) * sizeof(WCHAR);
                        memmove(prev, p + 2, buffer + reqsize - prev + sizeof(WCHAR));
                    }
                    break;
                case '\0':
                    reqsize -= 2 * sizeof(WCHAR);
                    *p = 0;
                    break;
                }
                break;
            case '\\':
                reqsize -= 2 * sizeof(WCHAR);
                memmove(ptr + 2, ptr, buffer + reqsize - ptr + sizeof(WCHAR));
                break;
            }
        }
        ptr = p;
    }

done:
    RtlReleasePebLock();
    return reqsize;
}

/******************************************************************
 *		RtlGetFullPathName_U  (NTDLL.@)
 *
 * Returns the number of bytes written to buffer (not including the
 * terminating NULL) if the function succeeds, or the required number of bytes
 * (including the terminating NULL) if the buffer is to small.
 *
 * file_part will point to the filename part inside buffer (except if we use
 * DOS device name, in which case file_in_buf is NULL)
 *
 */
DWORD WINAPI RtlGetFullPathName_U(const WCHAR* name, ULONG size, WCHAR* buffer,
                                  WCHAR** file_part)
{
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
        LPWSTR  tmp = RtlAllocateHeap(ntdll_get_process_heap(), 0, reqsize);
        reqsize = get_full_path_helper(name, tmp, reqsize) + sizeof(WCHAR);
        RtlFreeHeap(ntdll_get_process_heap(), 0, tmp);
    }
    else
    {
        WCHAR*      ptr;
        /* find file part */
        if (file_part && (ptr = strrchrW(buffer, '\\')) != NULL && ptr >= buffer + 2 && *++ptr)
            *file_part = ptr;
    }
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
            /* FIXME: check for invalid chars */
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

    us = &ntdll_get_process_pmts()->CurrentDirectoryName;
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

    curdir = &ntdll_get_process_pmts()->CurrentDirectoryName;
    size = curdir->MaximumLength;

    buf = RtlAllocateHeap(ntdll_get_process_heap(), 0, size);
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
    if (buf) RtlFreeHeap(ntdll_get_process_heap(), 0, buf);

    RtlReleasePebLock();

    return nts;
}
