/*
 * 16-bit resource functions
 *
 * Copyright 1993 Robert J. Amstadt
 * Copyright 1997 Alex Korobka
 * Copyright 1998 Ulrich Weigand
 * Copyright 1995, 2003 Alexandre Julliard
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

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "windef.h"
#include "winbase.h"
#include "wownt32.h"
#include "wine/winbase16.h"
#include "wine/winuser16.h"
#include "wine/unicode.h"
#include "module.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(resource);

/* handle conversions */
#define HRSRC_32(h16)   ((HRSRC)(ULONG_PTR)(h16))
#define HGLOBAL_32(h16) ((HGLOBAL)(ULONG_PTR)(h16))

static inline NE_MODULE *get_module( HMODULE16 mod )
{
    if (!mod) mod = TASK_GetCurrent()->hModule;
    return NE_GetPtr( mod );
}

#define HRSRC_MAP_BLOCKSIZE 16

typedef struct _HRSRC_ELEM
{
    HRSRC hRsrc;
    WORD  type;
} HRSRC_ELEM;

typedef struct _HRSRC_MAP
{
    int nAlloc;
    int nUsed;
    HRSRC_ELEM *elem;
} HRSRC_MAP;


/**********************************************************************
 *          MapHRsrc32To16
 */
static HRSRC16 MapHRsrc32To16( NE_MODULE *pModule, HRSRC hRsrc32, WORD type )
{
    HRSRC_MAP *map = (HRSRC_MAP *)pModule->hRsrcMap;
    HRSRC_ELEM *newElem;
    int i;

    /* On first call, initialize HRSRC map */
    if ( !map )
    {
        if ( !(map = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(HRSRC_MAP) ) ) )
        {
            ERR("Cannot allocate HRSRC map\n" );
            return 0;
        }
        pModule->hRsrcMap = map;
    }

    /* Check whether HRSRC32 already in map */
    for ( i = 0; i < map->nUsed; i++ )
        if ( map->elem[i].hRsrc == hRsrc32 )
            return (HRSRC16)(i + 1);

    /* If no space left, grow table */
    if ( map->nUsed == map->nAlloc )
    {

	if (map->elem)
    	    newElem = (HRSRC_ELEM *)HeapReAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                    map->elem, (map->nAlloc + HRSRC_MAP_BLOCKSIZE) * sizeof(HRSRC_ELEM) );
	else
    	    newElem = (HRSRC_ELEM *)HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                    (map->nAlloc + HRSRC_MAP_BLOCKSIZE) * sizeof(HRSRC_ELEM) );

        if ( !newElem )
        {
            ERR("Cannot grow HRSRC map\n" );
            return 0;
        }
        map->elem = newElem;
        map->nAlloc += HRSRC_MAP_BLOCKSIZE;
    }

    /* Add HRSRC32 to table */
    map->elem[map->nUsed].hRsrc = hRsrc32;
    map->elem[map->nUsed].type  = type;
    map->nUsed++;

    return (HRSRC16)map->nUsed;
}

/**********************************************************************
 *          MapHRsrc16To32
 */
static HRSRC MapHRsrc16To32( NE_MODULE *pModule, HRSRC16 hRsrc16 )
{
    HRSRC_MAP *map = (HRSRC_MAP *)pModule->hRsrcMap;
    if ( !map || !hRsrc16 || hRsrc16 > map->nUsed ) return 0;

    return map->elem[hRsrc16-1].hRsrc;
}

/**********************************************************************
 *          MapHRsrc16ToType
 */
static WORD MapHRsrc16ToType( NE_MODULE *pModule, HRSRC16 hRsrc16 )
{
    HRSRC_MAP *map = (HRSRC_MAP *)pModule->hRsrcMap;
    if ( !map || !hRsrc16 || hRsrc16 > map->nUsed ) return 0;

    return map->elem[hRsrc16-1].type;
}


/**********************************************************************
 *          get_res_name
 *
 * Convert a resource name from '#xxx' form to numerical id.
 */
static inline LPCSTR get_res_name( LPCSTR name )
{
    if (HIWORD(name) && name[0] == '#') name = (LPCSTR)atoi( name + 1 );
    return name;
}


/**********************************************************************
 *          next_typeinfo
 */
static inline NE_TYPEINFO *next_typeinfo( NE_TYPEINFO *info )
{
    return (NE_TYPEINFO *)((char*)(info + 1) + info->count * sizeof(NE_NAMEINFO));
}


/**********************************************************************
 *          get_default_res_handler
 */
static inline FARPROC16 get_default_res_handler(void)
{
    static FARPROC16 handler;

    if (!handler) handler = GetProcAddress16( GetModuleHandle16("KERNEL"), "DefResourceHandler" );
    return handler;
}


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
static NE_TYPEINFO *NE_FindTypeSection( LPBYTE pResTab, NE_TYPEINFO *pTypeInfo, LPCSTR typeId )
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
            pTypeInfo = next_typeinfo(pTypeInfo);
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
            pTypeInfo = next_typeinfo(pTypeInfo);
        }
    }
    return NULL;
}


/***********************************************************************
 *           NE_FindResourceFromType
 *
 * Find a resource once the type info structure has been found.
 */
static NE_NAMEINFO *NE_FindResourceFromType( LPBYTE pResTab, NE_TYPEINFO *pTypeInfo, LPCSTR resId )
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
            if (pNameInfo->id == id) return pNameInfo;
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


/**********************************************************************
 *	SetResourceHandler	(KERNEL.67)
 */
FARPROC16 WINAPI SetResourceHandler16( HMODULE16 hModule, LPCSTR typeId, FARPROC16 resourceHandler )
{
    LPBYTE pResTab;
    NE_TYPEINFO *pTypeInfo;
    FARPROC16 prevHandler = NULL;
    NE_MODULE *pModule = NE_GetPtr( hModule );

    if (!pModule || !pModule->res_table) return NULL;

    pResTab = (LPBYTE)pModule + pModule->res_table;
    pTypeInfo = (NE_TYPEINFO *)(pResTab + 2);

    TRACE("module=%04x type=%s\n", hModule, debugstr_a(typeId) );

    for (;;)
    {
        if (!(pTypeInfo = NE_FindTypeSection( pResTab, pTypeInfo, typeId )))
            break;
        memcpy_unaligned( &prevHandler, &pTypeInfo->resloader, sizeof(FARPROC16) );
        memcpy_unaligned( &pTypeInfo->resloader, &resourceHandler, sizeof(FARPROC16) );
        pTypeInfo = next_typeinfo(pTypeInfo);
    }
    if (!prevHandler) prevHandler = get_default_res_handler();
    return prevHandler;
}


/**********************************************************************
 *	    ConvertDialog32To16   (KERNEL.615)
 */
VOID WINAPI ConvertDialog32To16( LPVOID dialog32, DWORD size, LPVOID dialog16 )
{
    LPVOID p = dialog32;
    WORD nbItems, data, dialogEx;
    DWORD style;

    style = *((DWORD *)dialog16)++ = *((DWORD *)p)++;
    dialogEx = (style == 0xffff0001);  /* DIALOGEX resource */
    if (dialogEx)
    {
        *((DWORD *)dialog16)++ = *((DWORD *)p)++; /* helpID */
        *((DWORD *)dialog16)++ = *((DWORD *)p)++; /* exStyle */
        style = *((DWORD *)dialog16)++ = *((DWORD *)p)++; /* style */
    }
    else
        ((DWORD *)p)++; /* exStyle ignored in 16-bit standard dialog */

    nbItems = *((BYTE *)dialog16)++ = (BYTE)*((WORD *)p)++;
    *((WORD *)dialog16)++ = *((WORD *)p)++; /* x */
    *((WORD *)dialog16)++ = *((WORD *)p)++; /* y */
    *((WORD *)dialog16)++ = *((WORD *)p)++; /* cx */
    *((WORD *)dialog16)++ = *((WORD *)p)++; /* cy */

    /* Transfer menu name */
    switch (*((WORD *)p))
    {
    case 0x0000:  ((WORD *)p)++; *((BYTE *)dialog16)++ = 0; break;
    case 0xffff:  ((WORD *)p)++; *((BYTE *)dialog16)++ = 0xff;
                  *((WORD *)dialog16)++ = *((WORD *)p)++; break;
    default:      WideCharToMultiByte( CP_ACP, 0, (LPWSTR)p, -1, (LPSTR)dialog16, 0x7fffffff, NULL,NULL );
                  ((LPSTR)dialog16) += strlen( (LPSTR)dialog16 ) + 1;
                  ((LPWSTR)p) += strlenW( (LPWSTR)p ) + 1;
                  break;
    }

    /* Transfer class name */
    switch (*((WORD *)p))
    {
    case 0x0000:  ((WORD *)p)++; *((BYTE *)dialog16)++ = 0; break;
    case 0xffff:  ((WORD *)p)++; *((BYTE *)dialog16)++ = 0xff;
                  *((WORD *)dialog16)++ = *((WORD *)p)++; break;
    default:      WideCharToMultiByte( CP_ACP, 0, (LPWSTR)p, -1, (LPSTR)dialog16, 0x7fffffff, NULL,NULL );
                  ((LPSTR)dialog16) += strlen( (LPSTR)dialog16 ) + 1;
                  ((LPWSTR)p) += strlenW( (LPWSTR)p ) + 1;
                  break;
    }

    /* Transfer window caption */
    WideCharToMultiByte( CP_ACP, 0, (LPWSTR)p, -1, (LPSTR)dialog16, 0x7fffffff, NULL,NULL );
    ((LPSTR)dialog16) += strlen( (LPSTR)dialog16 ) + 1;
    ((LPWSTR)p) += strlenW( (LPWSTR)p ) + 1;

    /* Transfer font info */
    if (style & DS_SETFONT)
    {
        *((WORD *)dialog16)++ = *((WORD *)p)++;  /* pointSize */
        if (dialogEx)
        {
            *((WORD *)dialog16)++ = *((WORD *)p)++; /* weight */
            *((WORD *)dialog16)++ = *((WORD *)p)++; /* italic */
        }
        WideCharToMultiByte( CP_ACP, 0, (LPWSTR)p, -1, (LPSTR)dialog16, 0x7fffffff, NULL,NULL );  /* faceName */
        ((LPSTR)dialog16) += strlen( (LPSTR)dialog16 ) + 1;
        ((LPWSTR)p) += strlenW( (LPWSTR)p ) + 1;
    }

    /* Transfer dialog items */
    while (nbItems)
    {
        /* align on DWORD boundary (32-bit only) */
        p = (LPVOID)((((int)p) + 3) & ~3);

        if (dialogEx)
        {
            *((DWORD *)dialog16)++ = *((DWORD *)p)++; /* helpID */
            *((DWORD *)dialog16)++ = *((DWORD *)p)++; /* exStyle */
            *((DWORD *)dialog16)++ = *((DWORD *)p)++; /* style */
        }
        else
        {
            style = *((DWORD *)p)++; /* save style */
            ((DWORD *)p)++;          /* ignore exStyle */
        }

        *((WORD *)dialog16)++ = *((WORD *)p)++; /* x */
        *((WORD *)dialog16)++ = *((WORD *)p)++; /* y */
        *((WORD *)dialog16)++ = *((WORD *)p)++; /* cx */
        *((WORD *)dialog16)++ = *((WORD *)p)++; /* cy */

        if (dialogEx)
            *((DWORD *)dialog16)++ = *((DWORD *)p)++; /* ID */
        else
        {
            *((WORD *)dialog16)++ = *((WORD *)p)++; /* ID */
            *((DWORD *)dialog16)++ = style;  /* style from above */
        }

        /* Transfer class name */
        switch (*((WORD *)p))
        {
        case 0x0000:  ((WORD *)p)++; *((BYTE *)dialog16)++ = 0; break;
        case 0xffff:  ((WORD *)p)++;
                      *((BYTE *)dialog16)++ = (BYTE)*((WORD *)p)++; break;
        default:      WideCharToMultiByte( CP_ACP, 0, (LPWSTR)p, -1, (LPSTR)dialog16, 0x7fffffff, NULL,NULL );
                      ((LPSTR)dialog16) += strlen( (LPSTR)dialog16 ) + 1;
                      ((LPWSTR)p) += strlenW( (LPWSTR)p ) + 1;
                      break;
        }

        /* Transfer window name */
        switch (*((WORD *)p))
        {
        case 0x0000:  ((WORD *)p)++; *((BYTE *)dialog16)++ = 0; break;
        case 0xffff:  ((WORD *)p)++; *((BYTE *)dialog16)++ = 0xff;
                      *((WORD *)dialog16)++ = *((WORD *)p)++; break;
        default:      WideCharToMultiByte( CP_ACP, 0, (LPWSTR)p, -1, (LPSTR)dialog16, 0x7fffffff, NULL,NULL );
                      ((LPSTR)dialog16) += strlen( (LPSTR)dialog16 ) + 1;
                      ((LPWSTR)p) += strlenW( (LPWSTR)p ) + 1;
                      break;
        }

        /* Transfer data */
        data = *((WORD *)p)++;
        if (dialogEx)
            *((WORD *)dialog16)++ = data;
        else
            *((BYTE *)dialog16)++ = (BYTE)data;

        if (data)
        {
            memcpy( dialog16, p, data );
            (LPSTR)dialog16 += data;
            (LPSTR)p += data;
        }

        /* Next item */
        nbItems--;
    }
}


/**********************************************************************
 *	    GetDialog32Size   (KERNEL.618)
 */
WORD WINAPI GetDialog32Size16( LPVOID dialog32 )
{
    LPVOID p = dialog32;
    WORD nbItems, data, dialogEx;
    DWORD style;

    style = *((DWORD *)p)++;
    dialogEx = (style == 0xffff0001);  /* DIALOGEX resource */
    if (dialogEx)
    {
        ((DWORD *)p)++; /* helpID */
        ((DWORD *)p)++; /* exStyle */
        style = *((DWORD *)p)++; /* style */
    }
    else
        ((DWORD *)p)++; /* exStyle */

    nbItems = *((WORD *)p)++;
    ((WORD *)p)++; /* x */
    ((WORD *)p)++; /* y */
    ((WORD *)p)++; /* cx */
    ((WORD *)p)++; /* cy */

    /* Skip menu name */
    switch (*((WORD *)p))
    {
    case 0x0000:  ((WORD *)p)++; break;
    case 0xffff:  ((WORD *)p) += 2; break;
    default:      ((LPWSTR)p) += strlenW( (LPWSTR)p ) + 1; break;
    }

    /* Skip class name */
    switch (*((WORD *)p))
    {
    case 0x0000:  ((WORD *)p)++; break;
    case 0xffff:  ((WORD *)p) += 2; break;
    default:      ((LPWSTR)p) += strlenW( (LPWSTR)p ) + 1; break;
    }

    /* Skip window caption */
    ((LPWSTR)p) += strlenW( (LPWSTR)p ) + 1;

    /* Skip font info */
    if (style & DS_SETFONT)
    {
        ((WORD *)p)++;  /* pointSize */
        if (dialogEx)
        {
            ((WORD *)p)++; /* weight */
            ((WORD *)p)++; /* italic */
        }
        ((LPWSTR)p) += strlenW( (LPWSTR)p ) + 1;  /* faceName */
    }

    /* Skip dialog items */
    while (nbItems)
    {
        /* align on DWORD boundary */
        p = (LPVOID)((((int)p) + 3) & ~3);

        if (dialogEx)
        {
            ((DWORD *)p)++; /* helpID */
            ((DWORD *)p)++; /* exStyle */
            ((DWORD *)p)++; /* style */
        }
        else
        {
            ((DWORD *)p)++; /* style */
            ((DWORD *)p)++; /* exStyle */
        }

        ((WORD *)p)++; /* x */
        ((WORD *)p)++; /* y */
        ((WORD *)p)++; /* cx */
        ((WORD *)p)++; /* cy */

        if (dialogEx)
            ((DWORD *)p)++; /* ID */
        else
            ((WORD *)p)++; /* ID */

        /* Skip class name */
        switch (*((WORD *)p))
        {
        case 0x0000:  ((WORD *)p)++; break;
        case 0xffff:  ((WORD *)p) += 2; break;
        default:      ((LPWSTR)p) += strlenW( (LPWSTR)p ) + 1; break;
        }

        /* Skip window name */
        switch (*((WORD *)p))
        {
        case 0x0000:  ((WORD *)p)++; break;
        case 0xffff:  ((WORD *)p) += 2; break;
        default:      ((LPWSTR)p) += strlenW( (LPWSTR)p ) + 1; break;
        }

        /* Skip data */
        data = *((WORD *)p)++;
        (LPSTR)p += data;

        /* Next item */
        nbItems--;
    }

    return (WORD)((LPSTR)p - (LPSTR)dialog32);
}


/**********************************************************************
 *	    ConvertMenu32To16   (KERNEL.616)
 */
VOID WINAPI ConvertMenu32To16( LPVOID menu32, DWORD size, LPVOID menu16 )
{
    LPVOID p = menu32;
    WORD version, headersize, flags, level = 1;

    version = *((WORD *)menu16)++ = *((WORD *)p)++;
    headersize = *((WORD *)menu16)++ = *((WORD *)p)++;
    if ( headersize )
    {
        memcpy( menu16, p, headersize );
        ((LPSTR)menu16) += headersize;
        ((LPSTR)p) += headersize;
    }

    while ( level )
        if ( version == 0 )  /* standard */
        {
            flags = *((WORD *)menu16)++ = *((WORD *)p)++;
            if ( !(flags & MF_POPUP) )
                *((WORD *)menu16)++ = *((WORD *)p)++;  /* ID */
            else
                level++;

            WideCharToMultiByte( CP_ACP, 0, (LPWSTR)p, -1, (LPSTR)menu16, 0x7fffffff, NULL,NULL );
            ((LPSTR)menu16) += strlen( (LPSTR)menu16 ) + 1;
            ((LPWSTR)p) += strlenW( (LPWSTR)p ) + 1;

            if ( flags & MF_END )
                level--;
        }
        else  /* extended */
        {
            *((DWORD *)menu16)++ = *((DWORD *)p)++;  /* fType */
            *((DWORD *)menu16)++ = *((DWORD *)p)++;  /* fState */
            *((WORD *)menu16)++ = (WORD)*((DWORD *)p)++; /* ID */
            flags = *((BYTE *)menu16)++ = (BYTE)*((WORD *)p)++;

            WideCharToMultiByte( CP_ACP, 0, (LPWSTR)p, -1, (LPSTR)menu16, 0x7fffffff, NULL,NULL );
            ((LPSTR)menu16) += strlen( (LPSTR)menu16 ) + 1;
            ((LPWSTR)p) += strlenW( (LPWSTR)p ) + 1;

            /* align on DWORD boundary (32-bit only) */
            p = (LPVOID)((((int)p) + 3) & ~3);

            /* If popup, transfer helpid */
            if ( flags & 1)
            {
                *((DWORD *)menu16)++ = *((DWORD *)p)++;
                level++;
            }

            if ( flags & MF_END )
                level--;
        }
}


/**********************************************************************
 *	    GetMenu32Size   (KERNEL.617)
 */
WORD WINAPI GetMenu32Size16( LPVOID menu32 )
{
    LPVOID p = menu32;
    WORD version, headersize, flags, level = 1;

    version = *((WORD *)p)++;
    headersize = *((WORD *)p)++;
    ((LPSTR)p) += headersize;

    while ( level )
        if ( version == 0 )  /* standard */
        {
            flags = *((WORD *)p)++;
            if ( !(flags & MF_POPUP) )
                ((WORD *)p)++;  /* ID */
            else
                level++;

            ((LPWSTR)p) += strlenW( (LPWSTR)p ) + 1;

            if ( flags & MF_END )
                level--;
        }
        else  /* extended */
        {
            ((DWORD *)p)++; /* fType */
            ((DWORD *)p)++; /* fState */
            ((DWORD *)p)++; /* ID */
            flags = *((WORD *)p)++;

            ((LPWSTR)p) += strlenW( (LPWSTR)p ) + 1;

            /* align on DWORD boundary (32-bit only) */
            p = (LPVOID)((((int)p) + 3) & ~3);

            /* If popup, skip helpid */
            if ( flags & 1)
            {
                ((DWORD *)p)++;
                level++;
            }

            if ( flags & MF_END )
                level--;
        }

    return (WORD)((LPSTR)p - (LPSTR)menu32);
}


/**********************************************************************
 *	    ConvertAccelerator32To16
 */
static void ConvertAccelerator32To16( LPVOID acc32, DWORD size, LPVOID acc16 )
{
    int type;

    do
    {
        /* Copy type */
        type = *((BYTE *)acc16)++ = *((BYTE *)acc32)++;
        /* Skip padding */
        ((BYTE *)acc32)++;
        /* Copy event and IDval */
        *((WORD *)acc16)++ = *((WORD *)acc32)++;
        *((WORD *)acc16)++ = *((WORD *)acc32)++;
        /* Skip padding */
        ((WORD *)acc32)++;

    } while ( !( type & 0x80 ) );
}


/**********************************************************************
 *	    NE_LoadPEResource
 */
static HGLOBAL16 NE_LoadPEResource( NE_MODULE *pModule, WORD type, LPVOID bits, DWORD size )
{
    HGLOBAL16 handle;

    TRACE("module=%04x type=%04x\n", pModule->self, type );

    handle = GlobalAlloc16( 0, size );

    switch (type)
    {
    case RT_MENU:
        ConvertMenu32To16( bits, size, GlobalLock16( handle ) );
        break;
    case RT_DIALOG:
        ConvertDialog32To16( bits, size, GlobalLock16( handle ) );
        break;
    case RT_ACCELERATOR:
        ConvertAccelerator32To16( bits, size, GlobalLock16( handle ) );
        break;
    case RT_STRING:
        FIXME("not yet implemented!\n" );
        /* fall through */
    default:
        memcpy( GlobalLock16( handle ), bits, size );
        break;
    }
    return handle;
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
 *          FindResource     (KERNEL.60)
 */
HRSRC16 WINAPI FindResource16( HMODULE16 hModule, LPCSTR name, LPCSTR type )
{
    NE_TYPEINFO *pTypeInfo;
    NE_NAMEINFO *pNameInfo;
    LPBYTE pResTab;
    NE_MODULE *pModule = get_module( hModule );

    if (!pModule) return 0;

    if (pModule->module32)
    {
        /* 32-bit PE module */
        HRSRC hRsrc32 = FindResourceA( pModule->module32, name, type );
        return MapHRsrc32To16( pModule, hRsrc32, HIWORD(type) ? 0 : LOWORD(type) );
    }

    TRACE("module=%04x name=%s type=%s\n", pModule->self, debugstr_a(name), debugstr_a(type) );

    if (!pModule->res_table) return 0;

    type = get_res_name( type );
    name = get_res_name( name );

    if (HIWORD(type) || HIWORD(name))
    {
        DWORD id = NE_FindNameTableId( pModule, type, name );
        if (id)  /* found */
        {
            type = (LPCSTR)(ULONG_PTR)LOWORD(id);
            name = (LPCSTR)(ULONG_PTR)HIWORD(id);
        }
    }
    pResTab = (LPBYTE)pModule + pModule->res_table;
    pTypeInfo = (NE_TYPEINFO *)( pResTab + 2 );

    for (;;)
    {
        if (!(pTypeInfo = NE_FindTypeSection( pResTab, pTypeInfo, type ))) break;
        if ((pNameInfo = NE_FindResourceFromType( pResTab, pTypeInfo, name )))
        {
            TRACE("    Found id %08lx\n", (DWORD)name );
            return (HRSRC16)( (char *)pNameInfo - (char *)pModule );
        }
        pTypeInfo = next_typeinfo(pTypeInfo);
    }
    return 0;
}


/**********************************************************************
 *          LoadResource     (KERNEL.61)
 */
HGLOBAL16 WINAPI LoadResource16( HMODULE16 hModule, HRSRC16 hRsrc )
{
    NE_TYPEINFO *pTypeInfo;
    NE_NAMEINFO *pNameInfo = NULL;
    NE_MODULE *pModule = get_module( hModule );
    int d;

    if (!hRsrc || !pModule) return 0;

    if (pModule->module32)
    {
        /* load 32-bit resource and convert it */
        HRSRC hRsrc32 = MapHRsrc16To32( pModule, hRsrc );
        WORD type     = MapHRsrc16ToType( pModule, hRsrc );
        HGLOBAL hMem  = LoadResource( pModule->module32, hRsrc32 );
        DWORD size    = SizeofResource( pModule->module32, hRsrc32 );
        if (!hMem) return 0;
        return NE_LoadPEResource( pModule, type, LockResource( hMem ), size );
    }

    /* first, verify hRsrc (just an offset from pModule to the needed pNameInfo) */

    d = pModule->res_table + 2;
    pTypeInfo = (NE_TYPEINFO *)((char *)pModule + d);
    while( hRsrc > d )
    {
        if (pTypeInfo->type_id == 0) break; /* terminal entry */
        d += sizeof(NE_TYPEINFO) + pTypeInfo->count * sizeof(NE_NAMEINFO);
        if (hRsrc < d)
        {
            if( ((d - hRsrc)%sizeof(NE_NAMEINFO)) == 0 )
            {
                pNameInfo = (NE_NAMEINFO *)((char *)pModule + hRsrc);
                break;
            }
            else break; /* NE_NAMEINFO boundary mismatch */
        }
        pTypeInfo = (NE_TYPEINFO *)((char *)pModule + d);
    }

    if (pNameInfo)
    {
        if (pNameInfo->handle && !(GlobalFlags16(pNameInfo->handle) & GMEM_DISCARDED))
        {
            pNameInfo->usage++;
            TRACE("  Already loaded, new count=%d\n", pNameInfo->usage );
        }
        else
        {
            FARPROC16 resloader;
            memcpy_unaligned( &resloader, &pTypeInfo->resloader, sizeof(FARPROC16) );
            if (resloader && resloader != get_default_res_handler())
            {
                WORD args[3];
                DWORD ret;

                args[2] = pNameInfo->handle;
                args[1] = pModule->self;
                args[0] = hRsrc;
                WOWCallback16Ex( (DWORD)resloader, WCB16_PASCAL, sizeof(args), args, &ret );
                pNameInfo->handle = LOWORD(ret);
            }
            else
                pNameInfo->handle = NE_DefResourceHandler( pNameInfo->handle, pModule->self, hRsrc );

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
 *          LockResource   (KERNEL.62)
 */
SEGPTR WINAPI WIN16_LockResource16( HGLOBAL16 handle )
{
    TRACE("(%04x)\n", handle );
    /* May need to reload the resource if discarded */
    return K32WOWGlobalLock16( handle );
}


/**********************************************************************
 *          LockResource16 (KERNEL32.@)
 */
LPVOID WINAPI LockResource16( HGLOBAL16 handle )
{
    return MapSL( WIN16_LockResource16(handle) );
}


/**********************************************************************
 *          SizeofResource   (KERNEL.65)
 */
DWORD WINAPI SizeofResource16( HMODULE16 hModule, HRSRC16 hRsrc )
{
    NE_MODULE *pModule = NE_GetPtr( hModule );

    TRACE("(%x, %x)\n", hModule, hRsrc );

    if (!hRsrc) return 0;
    if (!(pModule = get_module( hModule ))) return 0;
    if (pModule->res_table)
    {
        WORD sizeShift = *(WORD *)((char *)pModule + pModule->res_table);
        NE_NAMEINFO *pNameInfo = (NE_NAMEINFO*)((char*)pModule + hRsrc);
        return (DWORD)pNameInfo->length << sizeShift;
    }
    if (pModule->module32)
    {
        /* 32-bit PE module */
        return SizeofResource( pModule->module32, MapHRsrc16To32( pModule, hRsrc ) );
    }
    return 0;
}


typedef WORD (WINAPI *pDestroyIcon32Proc)( HGLOBAL16 handle, UINT16 flags );

/**********************************************************************
 *          FreeResource     (KERNEL.63)
 */
BOOL16 WINAPI FreeResource16( HGLOBAL16 handle )
{
    pDestroyIcon32Proc proc;
    HMODULE user;
    NE_MODULE *pModule = NE_GetPtr( FarGetOwner16( handle ) );

    TRACE("(%04x)\n", handle );

    /* Try NE resource first */

    if (pModule && pModule->res_table)
    {
        NE_TYPEINFO *pTypeInfo = (NE_TYPEINFO *)((char *)pModule + pModule->res_table + 2);
        while (pTypeInfo->type_id)
        {
            WORD count;
            NE_NAMEINFO *pNameInfo = (NE_NAMEINFO *)(pTypeInfo + 1);
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
    }

    /* If this failed, call USER.DestroyIcon32; this will check
       whether it is a shared cursor/icon; if not it will call
       GlobalFree16() */
    user = GetModuleHandleA( "user32.dll" );
    if (user && (proc = (pDestroyIcon32Proc)GetProcAddress( user, "DestroyIcon32" )))
        return proc( handle, 1 /*CID_RESOURCE*/ );
    else
        return GlobalFree16( handle );
}
