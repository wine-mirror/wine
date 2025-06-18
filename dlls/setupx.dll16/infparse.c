/*
 * SetupX .inf file parsing functions
 *
 * Copyright 2000 Andreas Mohr for CodeWeavers
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
 *
 * FIXME:
 * - return values ???
 * - this should be reimplemented at some point to have its own
 *   file parsing instead of using profile functions,
 *   as some SETUPX exports probably demand that
 *   (IpSaveRestorePosition, IpFindNextMatchLine, ...).
 */

#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winternl.h"
#include "wingdi.h"
#include "winuser.h"
#include "winnls.h"
#include "setupapi.h"
#include "setupx16.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(setupapi);

#define MAX_HANDLES 16384
#define FIRST_HANDLE 32

static HINF handles[MAX_HANDLES];


static RETERR16 alloc_hinf16( HINF hinf, HINF16 *hinf16 )
{
    int i;
    for (i = 0; i < MAX_HANDLES; i++)
    {
        if (!handles[i])
        {
            handles[i] = hinf;
            *hinf16 = i + FIRST_HANDLE;
            return OK;
        }
    }
    return ERR_IP_OUT_OF_HANDLES;
}

static HINF get_hinf( HINF16 hinf16 )
{
    int idx = hinf16 - FIRST_HANDLE;
    if (idx < 0 || idx >= MAX_HANDLES) return 0;
    return handles[idx];
}


static HINF free_hinf16( HINF16 hinf16 )
{
    HINF ret;
    int idx = hinf16 - FIRST_HANDLE;

    if (idx < 0 || idx >= MAX_HANDLES) return 0;
    ret = handles[idx];
    handles[idx] = 0;
    return ret;
}

/* convert last error code to a RETERR16 value */
static RETERR16 get_last_error(void)
{
    switch(GetLastError())
    {
    case ERROR_EXPECTED_SECTION_NAME:
    case ERROR_BAD_SECTION_NAME_LINE:
    case ERROR_SECTION_NAME_TOO_LONG: return ERR_IP_INVALID_SECT_NAME;
    case ERROR_SECTION_NOT_FOUND: return ERR_IP_SECT_NOT_FOUND;
    case ERROR_LINE_NOT_FOUND: return ERR_IP_LINE_NOT_FOUND;
    default: return IP_ERROR;  /* FIXME */
    }
}

/* string substitution support, duplicated from setupapi/parser.c */

static const char *get_string_subst( HINF hinf, const char *str, unsigned int *len,
                                     char subst[MAX_INF_STRING_LENGTH], BOOL no_trailing_slash )
{
    int dirid;
    char *end;
    INFCONTEXT context;
    char buffer[MAX_INF_STRING_LENGTH];

    if (!*len)  /* empty string (%%) is replaced by single percent */
    {
        *len = 1;
        return "%";
    }
    memcpy( buffer, str, *len );
    buffer[*len] = 0;

    if (SetupFindFirstLineA( hinf, "Strings", buffer, &context ) &&
        SetupGetStringFieldA( &context, 1, subst, MAX_INF_STRING_LENGTH, NULL ))
    {
        *len = strlen( subst );
        return subst;
    }

    /* check for integer id */
    dirid = strtoul( buffer, &end, 10 );
    if (!*end && !CtlGetLddPath16( dirid, subst ))
    {
        *len = strlen( subst );
        if (no_trailing_slash && *len && subst[*len - 1] == '\\') *len -= 1;
        return subst;
    }
    return NULL;
}

static unsigned int string_subst( HINF hinf, const char *text, char *buffer )
{
    const char *start, *subst, *p;
    unsigned int len, total = 0;
    BOOL inside = FALSE;
    unsigned int size = MAX_INF_STRING_LENGTH;
    char tmp[MAX_INF_STRING_LENGTH];

    for (p = start = text; *p; p++)
    {
        if (*p != '%') continue;
        inside = !inside;
        if (inside)  /* start of a %xx% string */
        {
            len = p - start;
            if (len > size - 1) len = size - 1;
            if (buffer) memcpy( buffer + total, start, len );
            total += len;
            size -= len;
            start = p;
        }
        else /* end of the %xx% string, find substitution */
        {
            len = p - start - 1;
            subst = get_string_subst( hinf, start + 1, &len, tmp, p[1] == '\\' );
            if (!subst)
            {
                subst = start;
                len = p - start + 1;
            }
            if (len > size - 1) len = size - 1;
            if (buffer) memcpy( buffer + total, subst, len );
            total += len;
            size -= len;
            start = p + 1;
        }
    }

    if (start != p) /* unfinished string, copy it */
    {
        len = p - start;
        if (len > size - 1) len = size - 1;
        if (buffer) memcpy( buffer + total, start, len );
        total += len;
    }
    if (buffer && size) buffer[total] = 0;
    return total;
}


/***********************************************************************
 *		IpOpen (SETUPX.2)
 *
 */
RETERR16 WINAPI IpOpen16( LPCSTR filename, HINF16 *hinf16 )
{
    HINF hinf = SetupOpenInfFileA( filename, NULL, INF_STYLE_WIN4, NULL );
    if (hinf == INVALID_HANDLE_VALUE) return get_last_error();
    return alloc_hinf16( hinf, hinf16 );
}


/***********************************************************************
 *		IpClose (SETUPX.4)
 */
RETERR16 WINAPI IpClose16( HINF16 hinf16 )
{
    HINF hinf = free_hinf16( hinf16 );
    if (!hinf) return ERR_IP_INVALID_HINF;
    SetupCloseInfFile( hinf );
    return OK;
}


/***********************************************************************
 *		IpGetProfileString (SETUPX.210)
 */
RETERR16 WINAPI IpGetProfileString16( HINF16 hinf16, LPCSTR section, LPCSTR entry,
                                      LPSTR buffer, WORD buflen )
{
    DWORD required_size;
    HINF hinf = get_hinf( hinf16 );

    if (!hinf) return ERR_IP_INVALID_HINF;
    if (!SetupGetLineTextA( NULL, hinf, section, entry, buffer, buflen, &required_size ))
        return get_last_error();
    TRACE("%p: section %s entry %s ret %s\n",
          hinf, debugstr_a(section), debugstr_a(entry), debugstr_a(buffer) );
    return OK;
}


/***********************************************************************
 *		GenFormStrWithoutPlaceHolders (SETUPX.103)
 *
 * ought to be pretty much implemented, I guess...
 */
void WINAPI GenFormStrWithoutPlaceHolders16( LPSTR dst, LPCSTR src, HINF16 hinf16 )
{
    HINF hinf = get_hinf( hinf16 );

    if (!hinf) return;

    string_subst( hinf, src, dst );
    TRACE( "%s -> %s\n", debugstr_a(src), debugstr_a(dst) );
}

/***********************************************************************
 *		GenInstall (SETUPX.101)
 *
 * generic installer function for .INF file sections
 *
 * This is not perfect - patch whenever you can !
 *
 * wFlags == GENINSTALL_DO_xxx
 * e.g. NetMeeting:
 * first call GENINSTALL_DO_REGSRCPATH | GENINSTALL_DO_FILES,
 * second call GENINSTALL_DO_LOGCONFIG | CFGAUTO | INI2REG | REG | INI
 */
RETERR16 WINAPI GenInstall16( HINF16 hinf16, LPCSTR section, WORD genflags )
{
    UINT flags = 0;
    HINF hinf = get_hinf( hinf16 );
    RETERR16 ret = OK;
    void *context;

    if (!hinf) return ERR_IP_INVALID_HINF;

    if (genflags & GENINSTALL_DO_FILES) flags |= SPINST_FILES;
    if (genflags & GENINSTALL_DO_INI) flags |= SPINST_INIFILES;
    if (genflags & GENINSTALL_DO_REG) flags |= SPINST_REGISTRY;
    if (genflags & GENINSTALL_DO_INI2REG) flags |= SPINST_INI2REG;
    if (genflags & GENINSTALL_DO_LOGCONFIG) flags |= SPINST_LOGCONFIG;
    if (genflags & GENINSTALL_DO_REGSRCPATH) FIXME( "unsupported flag: GENINSTALL_DO_REGSRCPATH\n" );
    if (genflags & GENINSTALL_DO_CFGAUTO) FIXME( "unsupported flag: GENINSTALL_DO_CFGAUTO\n" );
    if (genflags & GENINSTALL_DO_PERUSER) FIXME( "unsupported flag: GENINSTALL_DO_PERUSER\n" );

    context = SetupInitDefaultQueueCallback( 0 );
    if (!SetupInstallFromInfSectionA( 0, hinf, section, flags, 0, NULL,
                                      SP_COPY_NEWER_OR_SAME, SetupDefaultQueueCallbackA,
                                      context, 0, 0 ))
        ret = get_last_error();

    SetupTermDefaultQueueCallback( context );
    return ret;
}
