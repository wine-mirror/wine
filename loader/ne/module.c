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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <ctype.h>

#include "winbase.h"
#include "wine/winbase16.h"
#include "wine/library.h"
#include "winerror.h"
#include "module.h"
#include "toolhelp.h"
#include "file.h"
#include "task.h"
#include "snoop.h"
#include "builtin16.h"
#include "stackframe.h"
#include "excpt.h"
#include "wine/exception.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(module);
WINE_DECLARE_DEBUG_CHANNEL(loaddll);

#include "pshpack1.h"
typedef struct _GPHANDLERDEF
{
    WORD selector;
    WORD rangeStart;
    WORD rangeEnd;
    WORD handler;
} GPHANDLERDEF;
#include "poppack.h"

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


/***********************************************************************
 *          GetModuleHandle16 (KERNEL32.@)
 */
HMODULE16 WINAPI GetModuleHandle16( LPCSTR name )
{
    HMODULE16	hModule = hFirstModule;
    LPSTR	s;
    BYTE	len, *name_table;
    char	tmpstr[MAX_PATH];
    NE_MODULE *pModule;

    TRACE("(%s)\n", name);

    if (!HIWORD(name))
    	return GetExePtr(LOWORD(name));

    len = strlen(name);
    if (!len)
    	return 0;

    lstrcpynA(tmpstr, name, sizeof(tmpstr));

    /* If 'name' matches exactly the module name of a module:
     * Return its handle.
     */
    for (hModule = hFirstModule; hModule ; hModule = pModule->next)
    {
	pModule = NE_GetPtr( hModule );
        if (!pModule) break;
        if (pModule->flags & NE_FFLAGS_WIN32) continue;

        name_table = (BYTE *)pModule + pModule->name_table;
        if ((*name_table == len) && !strncmp(name, name_table+1, len))
            return hModule;
    }

    /* If uppercased 'name' matches exactly the module name of a module:
     * Return its handle
     */
    for (s = tmpstr; *s; s++) *s = FILE_toupper(*s);

    for (hModule = hFirstModule; hModule ; hModule = pModule->next)
    {
	pModule = NE_GetPtr( hModule );
        if (!pModule) break;
        if (pModule->flags & NE_FFLAGS_WIN32) continue;

        name_table = (BYTE *)pModule + pModule->name_table;
	/* FIXME: the strncasecmp is WRONG. It should not be case insensitive,
	 * but case sensitive! (Unfortunately Winword 6 and subdlls have
	 * lowercased module names, but try to load uppercase DLLs, so this
	 * 'i' compare is just a quickfix until the loader handles that
	 * correctly. -MM 990705
	 */
        if ((*name_table == len) && !FILE_strncasecmp(tmpstr, name_table+1, len))
            return hModule;
    }

    /* If the base filename of 'name' matches the base filename of the module
     * filename of some module (case-insensitive compare):
     * Return its handle.
     */

    /* basename: search backwards in passed name to \ / or : */
    s = tmpstr + strlen(tmpstr);
    while (s > tmpstr)
    {
    	if (s[-1]=='/' || s[-1]=='\\' || s[-1]==':')
		break;
	s--;
    }

    /* search this in loaded filename list */
    for (hModule = hFirstModule; hModule ; hModule = pModule->next)
    {
    	char		*loadedfn;
	OFSTRUCT	*ofs;

	pModule = NE_GetPtr( hModule );
        if (!pModule) break;
	if (!pModule->fileinfo) continue;
        if (pModule->flags & NE_FFLAGS_WIN32) continue;

        ofs = (OFSTRUCT*)((BYTE *)pModule + pModule->fileinfo);
	loadedfn = ((char*)ofs->szPathName) + strlen(ofs->szPathName);
	/* basename: search backwards in pathname to \ / or : */
	while (loadedfn > (char*)ofs->szPathName)
	{
	    if (loadedfn[-1]=='/' || loadedfn[-1]=='\\' || loadedfn[-1]==':')
		    break;
	    loadedfn--;
	}
	/* case insensitive compare ... */
	if (!FILE_strcasecmp(loadedfn, s))
	    return hModule;
    }
    return 0;
}
