/*
 *
 * Copyright 1993 Robert J. Amstadt
 * Copyright 1995 Alexandre Julliard
 *	     1997 Alex Korobka
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "windows.h"
#include "global.h"
#include "ldt.h"
#include "module.h"
#include "neexe.h"
#include "resource.h"
#include "debug.h"

#define  NEXT_TYPEINFO(pTypeInfo) ((NE_TYPEINFO *)((char*)((pTypeInfo) + 1) + \
                                   (pTypeInfo)->count * sizeof(NE_NAMEINFO)))

/***********************************************************************
 *           NE_FindNameTableId
 *
 * Find the type and resource id from their names.
 * Return value is MAKELONG( typeId, resId ), or 0 if not found.
 */
static DWORD NE_FindNameTableId( NE_MODULE *pModule, SEGPTR typeId, SEGPTR resId )
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
            dprintf_info(resource, "NameTable entry: type=%04x id=%04x\n",
                              pTypeInfo->type_id, pNameInfo->id );
            handle = LoadResource16( pModule->self, 
				   (HRSRC16)((int)pNameInfo - (int)pModule) );
            for(p = (WORD*)LockResource16(handle); p && *p; p = (WORD *)((char*)p+*p))
            {
                dprintf_info(resource,"  type=%04x '%s' id=%04x '%s'\n",
                                  p[1], (char *)(p+3), p[2],
                                  (char *)(p+3)+strlen((char *)(p+3))+1 );
                /* Check for correct type */

                if (p[1] & 0x8000)
                {
                    if (!HIWORD(typeId)) continue;
                    if (lstrcmpi32A( (char *)PTR_SEG_TO_LIN(typeId),
                                     (char *)(p + 3) )) continue;
                }
                else if (HIWORD(typeId) || ((typeId & ~0x8000)!= p[1]))
                  continue;

                /* Now check for the id */

                if (p[2] & 0x8000)
                {
                    if (!HIWORD(resId)) continue;
                    if (lstrcmpi32A( (char *)PTR_SEG_TO_LIN(resId),
                               (char*)(p+3)+strlen((char*)(p+3))+1 )) continue;
                    
                }
                else if (HIWORD(resId) || ((resId & ~0x8000) != p[2]))
                  continue;

                /* If we get here, we've found the entry */

                dprintf_info(resource, "  Found!\n" );
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
static NE_TYPEINFO* NE_FindTypeSection( NE_MODULE *pModule, 
					NE_TYPEINFO *pTypeInfo, SEGPTR typeId )
{
    /* start from pTypeInfo */

    if (HIWORD(typeId) != 0)  /* Named type */
    {
	char *str = (char *)PTR_SEG_TO_LIN( typeId );
	BYTE len = strlen( str );
	while (pTypeInfo->type_id)
	{
	    if (!(pTypeInfo->type_id & 0x8000))
	    {
		BYTE *p = (BYTE*)pModule + pModule->res_table + pTypeInfo->type_id;
		if ((*p == len) && !lstrncmpi32A( p+1, str, len ))
		{
		    dprintf_info(resource, "  Found type '%s'\n", str );
		    return pTypeInfo;
		}
	    }
	    dprintf_info(resource, "  Skipping type %04x\n", pTypeInfo->type_id );
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
		dprintf_info(resource, "  Found type %04x\n", id );
		return pTypeInfo;
	    }
	    dprintf_info(resource, "  Skipping type %04x\n", pTypeInfo->type_id );
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
static HRSRC16 NE_FindResourceFromType( NE_MODULE *pModule,
                                        NE_TYPEINFO *pTypeInfo, SEGPTR resId )
{
    BYTE *p;
    int count;
    NE_NAMEINFO *pNameInfo = (NE_NAMEINFO *)(pTypeInfo + 1);

    if (HIWORD(resId) != 0)  /* Named resource */
    {
        char *str = (char *)PTR_SEG_TO_LIN( resId );
        BYTE len = strlen( str );
        for (count = pTypeInfo->count; count > 0; count--, pNameInfo++)
        {
            if (pNameInfo->id & 0x8000) continue;
            p = (BYTE *)pModule + pModule->res_table + pNameInfo->id;
            if ((*p == len) && !lstrncmpi32A( p+1, str, len ))
                return (HRSRC16)((int)pNameInfo - (int)pModule);
        }
    }
    else  /* Numeric resource id */
    {
        WORD id = LOWORD(resId) | 0x8000;
        for (count = pTypeInfo->count; count > 0; count--, pNameInfo++)
            if (pNameInfo->id == id) 
	      return (HRSRC16)((int)pNameInfo - (int)pModule);
    }
    return 0;
}


/***********************************************************************
 *           NE_DefResourceHandler
 *
 * This is the default LoadProc() function. 
 */
HGLOBAL16 WINAPI NE_DefResourceHandler( HGLOBAL16 hMemObj, HMODULE16 hModule,
                                        HRSRC16 hRsrc )
{
    int  fd;
    NE_MODULE* pModule = MODULE_GetPtr( hModule );
    if ( pModule && (fd = MODULE_OpenFile( hModule )) >= 0)
    {
	HGLOBAL16 handle;
	WORD sizeShift = *(WORD *)((char *)pModule + pModule->res_table);
	NE_NAMEINFO* pNameInfo = (NE_NAMEINFO*)((char*)pModule + hRsrc);

        dprintf_info(resource, "NEResourceHandler: loading, pos=%d, len=%d\n",
                         (int)pNameInfo->offset << sizeShift,
                         (int)pNameInfo->length << sizeShift );
	if( hMemObj )
	    handle = GlobalReAlloc16( hMemObj, pNameInfo->length << sizeShift, 0 );
	else
	    handle = NE_AllocResource( hModule, hRsrc, 0 );

	if( handle )
	{
            lseek( fd, (int)pNameInfo->offset << sizeShift, SEEK_SET );
            read( fd, GlobalLock16( handle ), (int)pNameInfo->length << sizeShift );
	}
	return handle;
    }
    return (HGLOBAL16)0;
}

/***********************************************************************
 *           NE_InitResourceHandler
 *
 * Fill in 'resloader' fields in the resource table.
 */
BOOL32 NE_InitResourceHandler( HMODULE16 hModule )
{
    NE_MODULE *pModule = MODULE_GetPtr( hModule );
    NE_TYPEINFO *pTypeInfo = (NE_TYPEINFO *)((char *)pModule + pModule->res_table + 2);

    dprintf_info(resource,"InitResourceHandler[%04x]\n", hModule );

    while(pTypeInfo->type_id)
    {
	pTypeInfo->resloader = (DWORD)&NE_DefResourceHandler;
	pTypeInfo = NEXT_TYPEINFO(pTypeInfo);
    }
    return TRUE;
}

/***********************************************************************
 *           NE_SetResourceHandler
 */
FARPROC32 NE_SetResourceHandler( HMODULE16 hModule, SEGPTR typeId, 
				 FARPROC32 resourceHandler )
{
    NE_MODULE *pModule = MODULE_GetPtr( hModule );
    NE_TYPEINFO *pTypeInfo = (NE_TYPEINFO *)((char *)pModule + pModule->res_table + 2);
    FARPROC32  prevHandler = NULL;

    do
    {
	pTypeInfo = NE_FindTypeSection( pModule, pTypeInfo, typeId );
        if( pTypeInfo )
        {
	    prevHandler = (FARPROC32)pTypeInfo->resloader;
	    pTypeInfo->resloader = (DWORD)resourceHandler;
	    pTypeInfo = NEXT_TYPEINFO(pTypeInfo);
        }
    } while( pTypeInfo );
    return prevHandler;
}

/***********************************************************************
 *           NE_FindResource
 */
HRSRC16 NE_FindResource( HMODULE16 hModule, SEGPTR typeId, SEGPTR resId )
{
    NE_TYPEINFO *pTypeInfo;
    HRSRC16 hRsrc;

    NE_MODULE *pModule = MODULE_GetPtr( hModule );
    if (!pModule || !pModule->res_table) return 0;
    pTypeInfo = (NE_TYPEINFO *)((char *)pModule + pModule->res_table + 2);

    if (HIWORD(typeId) || HIWORD(resId))
    {
        DWORD id = NE_FindNameTableId( pModule, typeId, resId );
        if (id)  /* found */
        {
            typeId = LOWORD(id);
            resId  = HIWORD(id);
        }
    }

    do
    {
	pTypeInfo = NE_FindTypeSection( pModule, pTypeInfo, typeId );
        if( pTypeInfo )
	{
	    hRsrc = NE_FindResourceFromType(pModule, pTypeInfo, resId);
	    if( hRsrc )
	    {
		dprintf_info(resource, "    Found id %08lx\n", resId );
		return hRsrc;
	    }
	    dprintf_info(resource, "    Not found, going on\n" );
	    pTypeInfo = NEXT_TYPEINFO(pTypeInfo);
	}
    } while( pTypeInfo );

    dprintf_warn(resource, "failed!\n");
    return 0;
}


/***********************************************************************
 *           NE_AllocResource
 */
HGLOBAL16 NE_AllocResource( HMODULE16 hModule, HRSRC16 hRsrc, DWORD size )
{
    NE_NAMEINFO *pNameInfo=NULL;
    WORD sizeShift;

    NE_MODULE *pModule = MODULE_GetPtr( hModule );
    if (!pModule || !pModule->res_table) return 0;
    sizeShift = *(WORD *)((char *)pModule + pModule->res_table);
    pNameInfo = (NE_NAMEINFO*)((char*)pModule + hRsrc);
    if (size < (DWORD)pNameInfo->length << sizeShift)
        size = (DWORD)pNameInfo->length << sizeShift;
    return GLOBAL_Alloc( GMEM_FIXED, size, hModule, FALSE, FALSE, FALSE );
}


/***********************************************************************
 *           NE_AccessResource
 */
int NE_AccessResource( HMODULE16 hModule, HRSRC16 hRsrc )
{
    NE_NAMEINFO *pNameInfo=NULL;
    HFILE32 fd;

    NE_MODULE *pModule = MODULE_GetPtr( hModule );
    if (!pModule || !pModule->res_table) return -1;
    pNameInfo = (NE_NAMEINFO*)((char*)pModule + hRsrc);

    if ((fd = _lopen32( NE_MODULE_NAME(pModule), OF_READ )) != -1)
    {
        WORD sizeShift = *(WORD *)((char *)pModule + pModule->res_table);
        _llseek32( fd, (int)pNameInfo->offset << sizeShift, SEEK_SET );
    }
    return fd;
}


/***********************************************************************
 *           NE_SizeofResource
 */
DWORD NE_SizeofResource( HMODULE16 hModule, HRSRC16 hRsrc )
{
    NE_NAMEINFO *pNameInfo=NULL;
    WORD sizeShift;

    NE_MODULE *pModule = MODULE_GetPtr( hModule );
    if (!pModule || !pModule->res_table) return 0;
    sizeShift = *(WORD *)((char *)pModule + pModule->res_table);
    pNameInfo = (NE_NAMEINFO*)((char*)pModule + hRsrc);
    return (DWORD)pNameInfo->length << sizeShift;
}


/***********************************************************************
 *           NE_LoadResource
 */
HGLOBAL16 NE_LoadResource( HMODULE16 hModule,  HRSRC16 hRsrc )
{
    NE_TYPEINFO *pTypeInfo;
    NE_NAMEINFO *pNameInfo = NULL;
    NE_MODULE *pModule = MODULE_GetPtr( hModule );
    int d;

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
	RESOURCEHANDLER16 __r16loader;
	if (pNameInfo->handle
	    && !(GlobalFlags16(pNameInfo->handle) & GMEM_DISCARDED))
	{
	    pNameInfo->usage++;
	    dprintf_info(resource, "  Already loaded, new count=%d\n",
			      pNameInfo->usage );
	}
	else
	{
	    if (pTypeInfo->resloader)
	  	__r16loader = (RESOURCEHANDLER16)pTypeInfo->resloader;
	    else /* this is really bad */
	    {
		fprintf( stderr, "[%04x]: Missing resource handler!!!...\n", hModule);
		__r16loader = NE_DefResourceHandler;
	    }

	    /* Finally call resource loader */

	    if ((pNameInfo->handle = __r16loader(pNameInfo->handle, hModule, hRsrc)))
	    {
		pNameInfo->usage++;
		pNameInfo->flags |= NE_SEGFLAGS_LOADED;
	    }
	}
	return pNameInfo->handle;
    }
    return 0;
}


/***********************************************************************
 *           NE_LockResource
 */
SEGPTR NE_LockResource( HMODULE16 hModule, HGLOBAL16 handle )
{
    /* May need to reload the resource if discarded */

    return (SEGPTR)WIN16_GlobalLock16( handle );
}


/***********************************************************************
 *           NE_FreeResource
 */
BOOL32 NE_FreeResource( HMODULE16 hModule, HGLOBAL16 handle )
{
    NE_TYPEINFO *pTypeInfo;
    NE_NAMEINFO *pNameInfo;
    WORD count;

    NE_MODULE *pModule = MODULE_GetPtr( hModule );
    if (!handle || !pModule || !pModule->res_table) return handle;
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

    dprintf_info(resource, "NE_FreeResource[%04x]: no intrinsic resource for %04x\n", 
			      hModule, handle );
    GlobalFree16( handle ); /* it could have been DirectResAlloc()'ed */
    return handle;
}
