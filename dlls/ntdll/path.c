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

WINE_DEFAULT_DEBUG_CHANNEL(file);

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
