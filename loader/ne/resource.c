/*
 * NE resource functions
 *
 * Copyright 1993 Robert J. Amstadt
 * Copyright 1995 Alexandre Julliard
 * Copyright 1997 Alex Korobka
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#include "windef.h"
#include "wine/winbase16.h"
#include "wine/library.h"
#include "module.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(resource);

#define NEXT_TYPEINFO(pTypeInfo) ((NE_TYPEINFO *)((char*)((pTypeInfo) + 1) + \
                                   (pTypeInfo)->count * sizeof(NE_NAMEINFO)))

static FARPROC16 DefResourceHandlerProc = (FARPROC16)0xffffffff;

/* already defined in segment.c glue code */
extern WORD CALLBACK NE_CallTo16_word_www(FARPROC16,WORD,WORD,WORD);

/***********************************************************************
 *           NE_FindNameTableId
 *
 * Find the type and resource id from their names.
 * Return value is MAKELONG( typeId, resId ), or 0 if not found.
 */
static DWORD NE_FindNameTableId( NE_MODULE *pModule, LPCSTR typeId, LPCSTR resId )
{
    NE_TYPEINFO *pTypeInfo = (NE_TYPEINFO *)((char *)pModule + pModule->res_table + 2);
    NE_NAMEINFO *pNameInfo;
    HGLOBAL16 handle;
    WORD *p;
    DWORD ret = 0;
    int count;

    for (; pTypeInfo->type_id != 0;
	   pTypeInfo = (NE_TYPEINFO *)((char*)(pTypeInfo+1) +
					pTypeInfo->count * sizeof(NE_NAMEINFO)))
    {
	if (pTypeInfo->type_id != 0x800f) continue;
	pNameInfo = (NE_NAMEINFO *)(pTypeInfo + 1);
	for (count = pTypeInfo->count; count > 0; count--, pNameInfo++)
	{
            TRACE("NameTable entry: type=%04x id=%04x\n",
                              pTypeInfo->type_id, pNameInfo->id );
            handle = LoadResource16( pModule->self,
				   (HRSRC16)((int)pNameInfo - (int)pModule) );
            for(p = (WORD*)LockResource16(handle); p && *p; p = (WORD *)((char*)p+*p))
            {
                TRACE("  type=%04x '%s' id=%04x '%s'\n",
                                  p[1], (char *)(p+3), p[2],
                                  (char *)(p+3)+strlen((char *)(p+3))+1 );
                /* Check for correct type */

                if (p[1] & 0x8000)
                {
                    if (!HIWORD(typeId)) continue;
                    if (strcasecmp( typeId, (char *)(p + 3) )) continue;
                }
                else if (HIWORD(typeId) || (((DWORD)typeId & ~0x8000)!= p[1]))
                  continue;

                /* Now check for the id */

                if (p[2] & 0x8000)
                {
                    if (!HIWORD(resId)) continue;
                    if (strcasecmp( resId, (char*)(p+3)+strlen((char*)(p+3))+1 )) continue;

                }
                else if (HIWORD(resId) || (((DWORD)resId & ~0x8000) != p[2]))
                  continue;

                /* If we get here, we've found the entry */

                TRACE("  Found!\n" );
                ret = MAKELONG( p[1], p[2] );
                break;
            }
            FreeResource16( handle );
            if (ret) return ret;
	}
    }
    return 0;
}

/***********************************************************************
 *           NE_FindTypeSection
 *
 * Find header struct for a particular resource type.
 */
NE_TYPEINFO *NE_FindTypeSection( LPBYTE pResTab,
			 	 NE_TYPEINFO *pTypeInfo, LPCSTR typeId )
{
    /* start from pTypeInfo */

    if (HIWORD(typeId) != 0)  /* Named type */
    {
	LPCSTR str = typeId;
	BYTE len = strlen( str );
	while (pTypeInfo->type_id)
	{
	    if (!(pTypeInfo->type_id & 0x8000))
	    {
		BYTE *p = pResTab + pTypeInfo->type_id;
		if ((*p == len) && !strncasecmp( p+1, str, len ))
		{
		    TRACE("  Found type '%s'\n", str );
		    return pTypeInfo;
		}
	    }
	    TRACE("  Skipping type %04x\n", pTypeInfo->type_id );
	    pTypeInfo = NEXT_TYPEINFO(pTypeInfo);
	}
    }
    else  /* Numeric type id */
    {
	WORD id = LOWORD(typeId) | 0x8000;
	while (pTypeInfo->type_id)
	{
            if (pTypeInfo->type_id == id)
	    {
		TRACE("  Found type %04x\n", id );
		return pTypeInfo;
	    }
	    TRACE("  Skipping type %04x\n", pTypeInfo->type_id );
	    pTypeInfo = NEXT_TYPEINFO(pTypeInfo);
	}
    }
    return NULL;
}

/***********************************************************************
 *           NE_FindResourceFromType
 *
 * Find a resource once the type info structure has been found.
 */
NE_NAMEINFO *NE_FindResourceFromType( LPBYTE pResTab,
                                      NE_TYPEINFO *pTypeInfo, LPCSTR resId )
{
    BYTE *p;
    int count;
    NE_NAMEINFO *pNameInfo = (NE_NAMEINFO *)(pTypeInfo + 1);

    if (HIWORD(resId) != 0)  /* Named resource */
    {
        LPCSTR str = resId;
        BYTE len = strlen( str );
        for (count = pTypeInfo->count; count > 0; count--, pNameInfo++)
        {
            if (pNameInfo->id & 0x8000) continue;
            p = pResTab + pNameInfo->id;
            if ((*p == len) && !strncasecmp( p+1, str, len ))
                return pNameInfo;
        }
    }
    else  /* Numeric resource id */
    {
        WORD id = LOWORD(resId) | 0x8000;
        for (count = pTypeInfo->count; count > 0; count--, pNameInfo++)
            if (pNameInfo->id == id)
	        return pNameInfo;
    }
    return NULL;
}


/***********************************************************************
 *           DefResourceHandler (KERNEL.456)
 *
 * This is the default LoadProc() function.
 */
HGLOBAL16 WINAPI NE_DefResourceHandler( HGLOBAL16 hMemObj, HMODULE16 hModule,
                                        HRSRC16 hRsrc )
{
    HANDLE fd;
    NE_MODULE* pModule = NE_GetPtr( hModule );
    if (pModule && (pModule->flags & NE_FFLAGS_BUILTIN))
    {
	HGLOBAL16 handle;
	WORD sizeShift = *(WORD *)((char *)pModule + pModule->res_table);
	NE_NAMEINFO* pNameInfo = (NE_NAMEINFO*)((char*)pModule + hRsrc);

        if ( hMemObj )
            handle = GlobalReAlloc16( hMemObj, pNameInfo->length << sizeShift, 0 );
        else
            handle = AllocResource16( hModule, hRsrc, 0 );

	if ( handle )
        {
            /* NOTE: hRsrcMap points to start of built-in resource data */
            memcpy( GlobalLock16( handle ),
                    (char *)pModule->hRsrcMap + (pNameInfo->offset << sizeShift),
                    pNameInfo->length << sizeShift );
        }
	return handle;
    }
    if (pModule && (fd = NE_OpenFile( pModule )) != INVALID_HANDLE_VALUE)
    {
	HGLOBAL16 handle;
	WORD sizeShift = *(WORD *)((char *)pModule + pModule->res_table);
	NE_NAMEINFO* pNameInfo = (NE_NAMEINFO*)((char*)pModule + hRsrc);

        TRACE("loading, pos=%d, len=%d\n",
		     (int)pNameInfo->offset << sizeShift,
		     (int)pNameInfo->length << sizeShift );
	if( hMemObj )
	    handle = GlobalReAlloc16( hMemObj, pNameInfo->length << sizeShift, 0 );
	else
	    handle = AllocResource16( hModule, hRsrc, 0 );

	if( handle )
	{
            DWORD res;
            SetFilePointer( fd, (int)pNameInfo->offset << sizeShift, NULL, SEEK_SET );
            ReadFile( fd, GlobalLock16( handle ), (int)pNameInfo->length << sizeShift,
                      &res, NULL );
	}
        CloseHandle(fd);
	return handle;
    }
    return (HGLOBAL16)0;
}

/***********************************************************************
 *           NE_InitResourceHandler
 *
 * Fill in 'resloader' fields in the resource table.
 */
BOOL NE_InitResourceHandler( HMODULE16 hModule )
{
    NE_MODULE *pModule = NE_GetPtr( hModule );
    NE_TYPEINFO *pTypeInfo = (NE_TYPEINFO *)((char *)pModule + pModule->res_table + 2);

    if ( DefResourceHandlerProc == (FARPROC16)0xffffffff )
    {
        HMODULE16 hModule = GetModuleHandle16( "KERNEL" );
        int ordinal = hModule? NE_GetOrdinal( hModule, "DefResourceHandler" ) : 0;

        if ( ordinal )
            DefResourceHandlerProc = NE_GetEntryPointEx( hModule, ordinal, FALSE );
        else
            DefResourceHandlerProc = NULL;
    }

    TRACE("InitResourceHandler[%04x]\n", hModule );

    while(pTypeInfo->type_id)
    {
        memcpy_unaligned( &pTypeInfo->resloader, &DefResourceHandlerProc, sizeof(FARPROC16) );
	pTypeInfo = NEXT_TYPEINFO(pTypeInfo);
    }
    return TRUE;
}


/**********************************************************************
 *	SetResourceHandler	(KERNEL.67)
 */
FARPROC16 WINAPI SetResourceHandler16( HMODULE16 hModule, LPCSTR typeId,
                                     FARPROC16 resourceHandler )
{
    FARPROC16 prevHandler = NULL;
    NE_MODULE *pModule = NE_GetPtr( hModule );
    LPBYTE pResTab = (LPBYTE)pModule + pModule->res_table;
    NE_TYPEINFO *pTypeInfo = (NE_TYPEINFO *)(pResTab + 2);

    if (!pModule || !pModule->res_table) return NULL;

    TRACE("module=%04x type=%s\n", hModule, debugstr_a(typeId) );

    for (;;)
    {
        if (!(pTypeInfo = NE_FindTypeSection( pResTab, pTypeInfo, typeId )))
            break;
        memcpy_unaligned( &prevHandler, &pTypeInfo->resloader, sizeof(FARPROC16) );
        memcpy_unaligned( &pTypeInfo->resloader, &resourceHandler, sizeof(FARPROC16) );
        pTypeInfo = NEXT_TYPEINFO(pTypeInfo);
    }
    return prevHandler;
}


/**********************************************************************
 *	    NE_FindResource
 */
HRSRC NE_FindResource( NE_MODULE *pModule, LPCSTR name, LPCSTR type )
{
    NE_TYPEINFO *pTypeInfo;
    NE_NAMEINFO *pNameInfo;
    LPBYTE pResTab;

    if (!pModule || !pModule->res_table) return 0;

    TRACE("module=%04x name=%s type=%s\n", pModule->self, debugstr_a(name), debugstr_a(type) );

    if (HIWORD(name))  /* Check for '#xxx' name */
    {
	LPCSTR ptr = name;
	if (ptr[0] == '#')
	    if (!(name = (LPCSTR)atoi( ptr + 1 )))
            {
                WARN("Incorrect resource name: %s\n", ptr);
                return 0;
	    }
    }

    if (HIWORD(type))  /* Check for '#xxx' type */
    {
	LPCSTR ptr = type;
	if (ptr[0] == '#')
            if (!(type = (LPCSTR)atoi( ptr + 1 )))
            {
                WARN("Incorrect resource type: %s\n", ptr);
                return 0;
            }
    }

    if (HIWORD(type) || HIWORD(name))
    {
        DWORD id = NE_FindNameTableId( pModule, type, name );
        if (id)  /* found */
        {
            type = (LPCSTR)(int)LOWORD(id);
            name = (LPCSTR)(int)HIWORD(id);
        }
    }

    pResTab = (LPBYTE)pModule + pModule->res_table;
    pTypeInfo = (NE_TYPEINFO *)( pResTab + 2 );

    for (;;)
    {
	if (!(pTypeInfo = NE_FindTypeSection( pResTab, pTypeInfo, type )))
            break;
        if ((pNameInfo = NE_FindResourceFromType( pResTab, pTypeInfo, name )))
        {
            TRACE("    Found id %08lx\n", (DWORD)name );
            return (HRSRC)( (char *)pNameInfo - (char *)pModule );
        }
        TRACE("    Not found, going on\n" );
        pTypeInfo = NEXT_TYPEINFO(pTypeInfo);
    }

    WARN("failed!\n");
    return 0;
}


/**********************************************************************
 *	    AllocResource    (KERNEL.66)
 */
HGLOBAL16 WINAPI AllocResource16( HMODULE16 hModule, HRSRC16 hRsrc, DWORD size)
{
    NE_NAMEINFO *pNameInfo=NULL;
    WORD sizeShift;
    HGLOBAL16 ret;

    NE_MODULE *pModule = NE_GetPtr( hModule );
    if (!pModule || !pModule->res_table || !hRsrc) return 0;

    TRACE("module=%04x res=%04x size=%ld\n", hModule, hRsrc, size );

    sizeShift = *(WORD *)((char *)pModule + pModule->res_table);
    pNameInfo = (NE_NAMEINFO*)((char*)pModule + hRsrc);
    if (size < (DWORD)pNameInfo->length << sizeShift)
        size = (DWORD)pNameInfo->length << sizeShift;
    ret = GlobalAlloc16( GMEM_FIXED, size );
    if (ret) FarSetOwner16( ret, hModule );
    return ret;
}


/**********************************************************************
 *      DirectResAlloc    (KERNEL.168)
 *
 * Check Schulman, p. 232 for details
 */
HGLOBAL16 WINAPI DirectResAlloc16( HINSTANCE16 hInstance, WORD wType,
                                 UINT16 wSize )
{
    HGLOBAL16 ret;
    TRACE("(%04x,%04x,%04x)\n", hInstance, wType, wSize );
    if (!(hInstance = GetExePtr( hInstance ))) return 0;
    if(wType != 0x10)	/* 0x10 is the only observed value, passed from
                           CreateCursorIndirect. */
        TRACE("(wType=%x)\n", wType);
    ret = GlobalAlloc16( GMEM_MOVEABLE, wSize );
    if (ret) FarSetOwner16( ret, hInstance );
    return ret;
}


/**********************************************************************
 *          AccessResource (KERNEL.64)
 */
INT16 WINAPI AccessResource16( HINSTANCE16 hModule, HRSRC16 hRsrc )
{
    HFILE16 fd;
    NE_MODULE *pModule = NE_GetPtr( hModule );

    if (!pModule || !pModule->res_table || !hRsrc) return -1;

    TRACE("module=%04x res=%04x\n", pModule->self, hRsrc );

    if ((fd = _lopen16( NE_MODULE_NAME(pModule), OF_READ )) != HFILE_ERROR16)
    {
        WORD sizeShift = *(WORD *)((char *)pModule + pModule->res_table);
        NE_NAMEINFO *pNameInfo = (NE_NAMEINFO*)((char*)pModule + hRsrc);
        _llseek16( fd, (int)pNameInfo->offset << sizeShift, SEEK_SET );
    }
    return fd;
}


/**********************************************************************
 *	    NE_SizeofResource
 */
DWORD NE_SizeofResource( NE_MODULE *pModule, HRSRC hRsrc )
{
    NE_NAMEINFO *pNameInfo=NULL;
    WORD sizeShift;

    if (!pModule || !pModule->res_table) return 0;

    TRACE("module=%04x res=%p\n", pModule->self, hRsrc );

    sizeShift = *(WORD *)((char *)pModule + pModule->res_table);
    pNameInfo = (NE_NAMEINFO*)((char*)pModule + LOWORD(hRsrc));
    return (DWORD)pNameInfo->length << sizeShift;
}


/**********************************************************************
 *	    NE_LoadResource
 */
HGLOBAL16 NE_LoadResource( NE_MODULE *pModule, HRSRC16 hRsrc )
{
    NE_TYPEINFO *pTypeInfo;
    NE_NAMEINFO *pNameInfo = NULL;
    int d;

    TRACE("module=%04x res=%04x\n", pModule->self, hRsrc );
    if (!hRsrc || !pModule || !pModule->res_table) return 0;

    /* First, verify hRsrc (just an offset from pModule to the needed pNameInfo) */

    d = pModule->res_table + 2;
    pTypeInfo = (NE_TYPEINFO *)((char *)pModule + d);
    while( hRsrc > d )
    {
	if (pTypeInfo->type_id == 0)
		break; /* terminal entry */
	d += sizeof(NE_TYPEINFO) + pTypeInfo->count * sizeof(NE_NAMEINFO);
	if (hRsrc < d)
	{
	    if( ((d - hRsrc)%sizeof(NE_NAMEINFO)) == 0 )
	    {
		pNameInfo = (NE_NAMEINFO *)(((char *)pModule) + hRsrc);
		break;
	    }
	    else
		break; /* NE_NAMEINFO boundary mismatch */
	}
	pTypeInfo = (NE_TYPEINFO *)(((char *)pModule) + d);
    }

    if (pNameInfo)
    {
	if (pNameInfo->handle
	    && !(GlobalFlags16(pNameInfo->handle) & GMEM_DISCARDED))
	{
	    pNameInfo->usage++;
	    TRACE("  Already loaded, new count=%d\n",
			      pNameInfo->usage );
	}
	else
	{
            FARPROC16 resloader;
            memcpy_unaligned( &resloader, &pTypeInfo->resloader, sizeof(FARPROC16) );
	    if ( resloader && resloader != DefResourceHandlerProc )
                pNameInfo->handle = NE_CallTo16_word_www(
                    resloader, pNameInfo->handle, pModule->self, hRsrc );
	    else
                pNameInfo->handle = NE_DefResourceHandler(
                                         pNameInfo->handle, pModule->self, hRsrc );

	    if (pNameInfo->handle)
	    {
		pNameInfo->usage++;
		pNameInfo->flags |= NE_SEGFLAGS_LOADED;
	    }
	}
	return pNameInfo->handle;
    }
    return 0;
}


/**********************************************************************
 *	    NE_FreeResource
 */
BOOL16 NE_FreeResource( NE_MODULE *pModule, HGLOBAL16 handle )
{
    NE_TYPEINFO *pTypeInfo;
    NE_NAMEINFO *pNameInfo;
    WORD count;

    if (!handle || !pModule || !pModule->res_table) return handle;

    TRACE("handle=%04x\n", handle );

    pTypeInfo = (NE_TYPEINFO *)((char *)pModule + pModule->res_table + 2);
    while (pTypeInfo->type_id)
    {
        pNameInfo = (NE_NAMEINFO *)(pTypeInfo + 1);
        for (count = pTypeInfo->count; count > 0; count--)
        {
            if (pNameInfo->handle == handle)
            {
                if (pNameInfo->usage > 0) pNameInfo->usage--;
                if (pNameInfo->usage == 0)
                {
                    GlobalFree16( pNameInfo->handle );
                    pNameInfo->handle = 0;
		    pNameInfo->flags &= ~NE_SEGFLAGS_LOADED;
                }
                return 0;
            }
            pNameInfo++;
        }
        pTypeInfo = (NE_TYPEINFO *)pNameInfo;
    }

    return handle;
}
