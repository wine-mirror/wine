/*
 * NE modules
 *
 * Copyright 1995 Alexandre Julliard
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

#include <assert.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <ctype.h>

#include "windef.h"
#include "winbase.h"
#include "wine/winbase16.h"
#include "wine/library.h"
#include "winerror.h"
#include "module.h"
#include "toolhelp.h"
#include "file.h"
#include "task.h"
#include "builtin16.h"
#include "stackframe.h"
#include "excpt.h"
#include "wine/exception.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(module);
WINE_DECLARE_DEBUG_CHANNEL(loaddll);

#define hFirstModule (pThhook->hExeHead)


/***********************************************************************
 *           NE_GetPtr
 */
NE_MODULE *NE_GetPtr( HMODULE16 hModule )
{
    return (NE_MODULE *)GlobalLock16( GetExePtr(hModule) );
}


/**********************************************************************
 *	    GetModuleFileName      (KERNEL.49)
 *
 * Comment: see GetModuleFileNameA
 *
 * Even if invoked by second instance of a program,
 * it still returns path of first one.
 */
INT16 WINAPI GetModuleFileName16( HINSTANCE16 hModule, LPSTR lpFileName,
                                  INT16 nSize )
{
    NE_MODULE *pModule;

    /* Win95 does not query hModule if set to 0 !
     * Is this wrong or maybe Win3.1 only ? */
    if (!hModule) hModule = GetCurrentTask();

    if (!(pModule = NE_GetPtr( hModule ))) return 0;
    lstrcpynA( lpFileName, NE_MODULE_NAME(pModule), nSize );
    if (pModule->expected_version >= 0x400)
	GetLongPathNameA(NE_MODULE_NAME(pModule), lpFileName, nSize);
    TRACE("%04x -> '%s'\n", hModule, lpFileName );
    return strlen(lpFileName);
}
