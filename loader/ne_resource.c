#ifndef WINELIB
/*
 *
 * Copyright 1993 Robert J. Amstadt
 * Copyright 1995 Alexandre Julliard
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "windows.h"
#include "arch.h"
#include "dos_fs.h"
#include "global.h"
#include "ldt.h"
#include "module.h"
#include "neexe.h"
#include "resource.h"
#include "stddebug.h"
#include "debug.h"


/***********************************************************************
 *           NE_FindNameTableId
 *
 * Find the type and resource id from their names.
 * Return value is MAKELONG( typeId, resId ), or 0 if not found.
 */
static DWORD NE_FindNameTableId( HMODULE16 hModule, SEGPTR typeId, SEGPTR resId )
{
    NE_MODULE *pModule;
    NE_TYPEINFO *pTypeInfo;
    NE_NAMEINFO *pNameInfo;
    HGLOBAL16 handle;
    WORD *p;
    DWORD ret = 0;
    int count;

    if (!(pModule = MODULE_GetPtr( hModule ))) return 0;
    pTypeInfo = (NE_TYPEINFO *)((char *)pModule + pModule->res_table + 2);
    for (; pTypeInfo->type_id != 0;
             pTypeInfo = (NE_TYPEINFO *)((char*)(pTypeInfo+1) +
                                       pTypeInfo->count * sizeof(NE_NAMEINFO)))
    {
        if (pTypeInfo->type_id != 0x800f) continue;
        pNameInfo = (NE_NAMEINFO *)(pTypeInfo + 1);
        for (count = pTypeInfo->count; count > 0; count--, pNameInfo++)
        {
            dprintf_resource( stddeb, "NameTable entry: type=%04x id=%04x\n",
                              pTypeInfo->type_id, pNameInfo->id );
            handle = LoadResource16( hModule, 
				   (HANDLE)((int)pNameInfo - (int)pModule) );
            for(p = (WORD*)LockResource16(handle); p && *p; p = (WORD *)((char*)p+*p))
            {
                dprintf_resource( stddeb,"  type=%04x '%s' id=%04x '%s'\n",
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

                dprintf_resource( stddeb, "  Found!\n" );
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
        /* Search the names in the nametable */
        DWORD id = NE_FindNameTableId( hModule, typeId, resId );
        if (id)  /* found */
        {
            typeId = LOWORD(id);
            resId  = HIWORD(id);
        }
    }

    if (HIWORD(typeId) != 0)  /* Named type */
    {
        char *str = (char *)PTR_SEG_TO_LIN( typeId );
        BYTE len = strlen( str );
        while (pTypeInfo->type_id)
        {
            if (!(pTypeInfo->type_id & 0x8000))
            {
                BYTE *p = (BYTE*)pModule+pModule->res_table+pTypeInfo->type_id;
                if ((*p == len) && !lstrncmpi32A( p+1, str, len ))
                {
                    dprintf_resource( stddeb, "  Found type '%s'\n", str );
                    hRsrc = NE_FindResourceFromType(pModule, pTypeInfo, resId);
                    if (hRsrc)
                    {
                        dprintf_resource( stddeb, "    Found id %08lx\n", resId );
                        return hRsrc;
                    }
                    dprintf_resource( stddeb, "    Not found, going on\n" );
                }
            }
            dprintf_resource( stddeb, "  Skipping type %04x\n",
                              pTypeInfo->type_id );
            pTypeInfo = (NE_TYPEINFO *)((char*)(pTypeInfo+1) +
                                       pTypeInfo->count * sizeof(NE_NAMEINFO));
        }
    }
    else  /* Numeric type id */
    {
        WORD id = LOWORD(typeId) | 0x8000;
        while (pTypeInfo->type_id)
        {
            if (pTypeInfo->type_id == id)
            {
                dprintf_resource( stddeb, "  Found type %04x\n", id );
                hRsrc = NE_FindResourceFromType( pModule, pTypeInfo, resId );
                if (hRsrc)
                {
                    dprintf_resource( stddeb, "    Found id %08lx\n", resId );
                    return hRsrc;
                }
                dprintf_resource( stddeb, "    Not found, going on\n" );
            }
            dprintf_resource( stddeb, "  Skipping type %04x\n",
                              pTypeInfo->type_id );
            pTypeInfo = (NE_TYPEINFO *)((char*)(pTypeInfo+1) +
                                       pTypeInfo->count * sizeof(NE_NAMEINFO));
        }
    }
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
#ifndef WINELIB
    pNameInfo = (NE_NAMEINFO*)((char*)pModule + hRsrc);
#endif
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
    int fd;

    NE_MODULE *pModule = MODULE_GetPtr( hModule );
    if (!pModule || !pModule->res_table) return -1;
#ifndef WINELIB
    pNameInfo = (NE_NAMEINFO*)((char*)pModule + hRsrc);
#endif

    if ((fd = _lopen( NE_MODULE_NAME(pModule), OF_READ )) != -1)
    {
        WORD sizeShift = *(WORD *)((char *)pModule + pModule->res_table);
        _llseek( fd, (int)pNameInfo->offset << sizeShift, SEEK_SET );
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
#ifndef WINELIB
    pNameInfo = (NE_NAMEINFO*)((char*)pModule + hRsrc);
#endif
    return (DWORD)pNameInfo->length << sizeShift;
}


/***********************************************************************
 *           NE_LoadResource
 */
HGLOBAL16 NE_LoadResource( HMODULE16 hModule,  HRSRC16 hRsrc )
{
    NE_NAMEINFO *pNameInfo=NULL;
    WORD sizeShift;
    int fd;

    NE_MODULE *pModule = MODULE_GetPtr( hModule );
    if (!pModule || !pModule->res_table) return 0;
#ifndef WINELIB
    pNameInfo = (NE_NAMEINFO*)((char*)pModule + hRsrc);
#endif
    if (pNameInfo->handle)
    {
        pNameInfo->usage++;
        dprintf_resource( stddeb, "  Already loaded, new count=%d\n",
                          pNameInfo->usage );
        return pNameInfo->handle;
    }
    sizeShift = *(WORD *)((char *)pModule + pModule->res_table);
    dprintf_resource( stddeb, "  Loading, pos=%d, len=%d\n",
                      (int)pNameInfo->offset << sizeShift,
                      (int)pNameInfo->length << sizeShift );
    if ((fd = MODULE_OpenFile( hModule )) == -1) return 0;
    pNameInfo->handle = NE_AllocResource( hModule, hRsrc, 0 );
    pNameInfo->usage = 1;
    lseek( fd, (int)pNameInfo->offset << sizeShift, SEEK_SET );
    read( fd, GlobalLock16( pNameInfo->handle ),
          (int)pNameInfo->length << sizeShift );
    return pNameInfo->handle;
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
BOOL NE_FreeResource( HMODULE16 hModule, HGLOBAL16 handle )
{
    NE_TYPEINFO *pTypeInfo;
    NE_NAMEINFO *pNameInfo;
    WORD count;

    NE_MODULE *pModule = MODULE_GetPtr( hModule );
    if (!pModule || !pModule->res_table) return handle;
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
                }
                return 0;
            }
            pNameInfo++;
        }
        pTypeInfo = (NE_TYPEINFO *)pNameInfo;
    }
    fprintf( stderr, "NE_FreeResource: %04x %04x not found!\n", hModule, handle );
    return handle;
}
#endif /* WINELIB */
