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
 *           NE_RegisterModule
 */
void NE_RegisterModule( NE_MODULE *pModule )
{
    pModule->next = hFirstModule;
    hFirstModule = pModule->self;
}


/***********************************************************************
 *           NE_InitResourceHandler
 *
 * Fill in 'resloader' fields in the resource table.
 */
void NE_InitResourceHandler( NE_MODULE *pModule )
{
    static FARPROC16 proc;

    NE_TYPEINFO *pTypeInfo = (NE_TYPEINFO *)((char *)pModule + pModule->res_table + 2);

    TRACE("InitResourceHandler[%04x]\n", pModule->self );

    if (!proc) proc = GetProcAddress16( GetModuleHandle16("KERNEL"), "DefResourceHandler" );

    while(pTypeInfo->type_id)
    {
        memcpy_unaligned( &pTypeInfo->resloader, &proc, sizeof(FARPROC16) );
        pTypeInfo = (NE_TYPEINFO *)((char*)(pTypeInfo + 1) + pTypeInfo->count * sizeof(NE_NAMEINFO));
    }
}


/***********************************************************************
 *           NE_GetOrdinal
 *
 * Lookup the ordinal for a given name.
 */
WORD NE_GetOrdinal( HMODULE16 hModule, const char *name )
{
    unsigned char buffer[256], *cpnt;
    BYTE len;
    NE_MODULE *pModule;

    if (!(pModule = NE_GetPtr( hModule ))) return 0;
    if (pModule->flags & NE_FFLAGS_WIN32) return 0;

    TRACE("(%04x,'%s')\n", hModule, name );

      /* First handle names of the form '#xxxx' */

    if (name[0] == '#') return atoi( name + 1 );

      /* Now copy and uppercase the string */

    strcpy( buffer, name );
    for (cpnt = buffer; *cpnt; cpnt++) *cpnt = FILE_toupper(*cpnt);
    len = cpnt - buffer;

      /* First search the resident names */

    cpnt = (char *)pModule + pModule->name_table;

      /* Skip the first entry (module name) */
    cpnt += *cpnt + 1 + sizeof(WORD);
    while (*cpnt)
    {
        if (((BYTE)*cpnt == len) && !memcmp( cpnt+1, buffer, len ))
        {
            WORD ordinal;
            memcpy( &ordinal, cpnt + *cpnt + 1, sizeof(ordinal) );
            TRACE("  Found: ordinal=%d\n", ordinal );
            return ordinal;
        }
        cpnt += *cpnt + 1 + sizeof(WORD);
    }

      /* Now search the non-resident names table */

    if (!pModule->nrname_handle) return 0;  /* No non-resident table */
    cpnt = (char *)GlobalLock16( pModule->nrname_handle );

      /* Skip the first entry (module description string) */
    cpnt += *cpnt + 1 + sizeof(WORD);
    while (*cpnt)
    {
        if (((BYTE)*cpnt == len) && !memcmp( cpnt+1, buffer, len ))
        {
            WORD ordinal;
            memcpy( &ordinal, cpnt + *cpnt + 1, sizeof(ordinal) );
            TRACE("  Found: ordinal=%d\n", ordinal );
            return ordinal;
        }
        cpnt += *cpnt + 1 + sizeof(WORD);
    }
    return 0;
}


/***********************************************************************
 *		NE_GetEntryPoint
 */
FARPROC16 WINAPI NE_GetEntryPoint( HMODULE16 hModule, WORD ordinal )
{
    return NE_GetEntryPointEx( hModule, ordinal, TRUE );
}

/***********************************************************************
 *		NE_GetEntryPointEx
 */
FARPROC16 NE_GetEntryPointEx( HMODULE16 hModule, WORD ordinal, BOOL16 snoop )
{
    NE_MODULE *pModule;
    WORD sel, offset, i;

    ET_ENTRY *entry;
    ET_BUNDLE *bundle;

    if (!(pModule = NE_GetPtr( hModule ))) return 0;
    assert( !(pModule->flags & NE_FFLAGS_WIN32) );

    bundle = (ET_BUNDLE *)((BYTE *)pModule + pModule->entry_table);
    while ((ordinal < bundle->first + 1) || (ordinal > bundle->last))
    {
	if (!(bundle->next))
            return 0;
	bundle = (ET_BUNDLE *)((BYTE *)pModule + bundle->next);
    }

    entry = (ET_ENTRY *)((BYTE *)bundle+6);
    for (i=0; i < (ordinal - bundle->first - 1); i++)
	entry++;

    sel = entry->segnum;
    memcpy( &offset, &entry->offs, sizeof(WORD) );

    if (sel == 0xfe) sel = 0xffff;  /* constant entry */
    else sel = GlobalHandleToSel16(NE_SEG_TABLE(pModule)[sel-1].hSeg);
    if (sel==0xffff)
	return (FARPROC16)MAKESEGPTR( sel, offset );
    if (!snoop)
	return (FARPROC16)MAKESEGPTR( sel, offset );
    else
	return (FARPROC16)SNOOP16_GetProcAddress16(hModule,ordinal,(FARPROC16)MAKESEGPTR( sel, offset ));
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

/***********************************************************************
 *           GetProcAddress   (KERNEL.50)
 */
FARPROC16 WINAPI GetProcAddress16( HMODULE16 hModule, LPCSTR name )
{
    WORD ordinal;
    FARPROC16 ret;

    if (!hModule) hModule = GetCurrentTask();
    hModule = GetExePtr( hModule );

    if (HIWORD(name) != 0)
    {
        ordinal = NE_GetOrdinal( hModule, name );
        TRACE("%04x '%s'\n", hModule, name );
    }
    else
    {
        ordinal = LOWORD(name);
        TRACE("%04x %04x\n", hModule, ordinal );
    }
    if (!ordinal) return (FARPROC16)0;

    ret = NE_GetEntryPoint( hModule, ordinal );

    TRACE("returning %08x\n", (UINT)ret );
    return ret;
}


/***************************************************************************
 *              HasGPHandler                    (KERNEL.338)
 */
SEGPTR WINAPI HasGPHandler16( SEGPTR address )
{
    HMODULE16 hModule;
    int gpOrdinal;
    SEGPTR gpPtr;
    GPHANDLERDEF *gpHandler;

    if (    (hModule = FarGetOwner16( SELECTOROF(address) )) != 0
         && (gpOrdinal = NE_GetOrdinal( hModule, "__GP" )) != 0
         && (gpPtr = (SEGPTR)NE_GetEntryPointEx( hModule, gpOrdinal, FALSE )) != 0
         && !IsBadReadPtr16( gpPtr, sizeof(GPHANDLERDEF) )
         && (gpHandler = MapSL( gpPtr )) != NULL )
    {
        while (gpHandler->selector)
        {
            if (    SELECTOROF(address) == gpHandler->selector
                 && OFFSETOF(address)   >= gpHandler->rangeStart
                 && OFFSETOF(address)   <  gpHandler->rangeEnd  )
                return MAKESEGPTR( gpHandler->selector, gpHandler->handler );
            gpHandler++;
        }
    }

    return 0;
}
