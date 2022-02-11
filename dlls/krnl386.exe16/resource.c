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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

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
#include "kernel16_private.h"
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
    HRSRC_MAP *map = pModule->rsrc32_map;
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
        pModule->rsrc32_map = map;
    }

    /* Check whether HRSRC32 already in map */
    for ( i = 0; i < map->nUsed; i++ )
        if ( map->elem[i].hRsrc == hRsrc32 )
            return (HRSRC16)(i + 1);

    /* If no space left, grow table */
    if ( map->nUsed == map->nAlloc )
    {

	if (map->elem)
    	    newElem = HeapReAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                    map->elem, (map->nAlloc + HRSRC_MAP_BLOCKSIZE) * sizeof(HRSRC_ELEM) );
	else
    	    newElem = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
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
    HRSRC_MAP *map = pModule->rsrc32_map;
    if ( !map || !hRsrc16 || hRsrc16 > map->nUsed ) return 0;

    return map->elem[hRsrc16-1].hRsrc;
}

/**********************************************************************
 *          MapHRsrc16ToType
 */
static WORD MapHRsrc16ToType( NE_MODULE *pModule, HRSRC16 hRsrc16 )
{
    HRSRC_MAP *map = pModule->rsrc32_map;
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
    if (HIWORD(name) && name[0] == '#') name = MAKEINTRESOURCEA( atoi( name + 1 ) );
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
    NE_TYPEINFO *pTypeInfo = (NE_TYPEINFO *)((char *)pModule + pModule->ne_rsrctab + 2);
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
                                     (HRSRC16)((char *)pNameInfo - (char *)pModule) );
            for(p = LockResource16(handle); p && *p; p = (WORD *)((char*)p+*p))
            {
                TRACE("  type=%04x '%s' id=%04x '%s'\n",
                                  p[1], (char *)(p+3), p[2],
                                  (char *)(p+3)+strlen((char *)(p+3))+1 );
                /* Check for correct type */

                if (p[1] & 0x8000)
                {
                    if (!HIWORD(typeId)) continue;
                    if (stricmp( typeId, (char *)(p + 3) )) continue;
                }
                else if (HIWORD(typeId) || (((DWORD)typeId & ~0x8000)!= p[1]))
                  continue;

                /* Now check for the id */

                if (p[2] & 0x8000)
                {
                    if (!HIWORD(resId)) continue;
                    if (stricmp( resId, (char*)(p+3)+strlen((char*)(p+3))+1 )) continue;

                }
                else if (HIWORD(resId) || ((LOWORD(resId) & ~0x8000) != p[2]))
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
                if ((*p == len) && !_strnicmp( (char*)p+1, str, len ))
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
            if ((*p == len) && !_strnicmp( (char*)p+1, str, len ))
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
    HGLOBAL16 handle;
    WORD sizeShift;
    NE_NAMEINFO* pNameInfo;
    NE_MODULE* pModule = NE_GetPtr( hModule );

    if (!pModule) return 0;

    sizeShift = *(WORD *)((char *)pModule + pModule->ne_rsrctab);
    pNameInfo = (NE_NAMEINFO *)((char *)pModule + hRsrc);

    if ( hMemObj )
        handle = GlobalReAlloc16( hMemObj, pNameInfo->length << sizeShift, 0 );
    else
        handle = AllocResource16( hModule, hRsrc, 0 );

    if (handle)
    {
        if (!NE_READ_DATA( pModule, GlobalLock16( handle ),
                           (int)pNameInfo->offset << sizeShift,
                           (int)pNameInfo->length << sizeShift ))
        {
            GlobalFree16( handle );
            handle = 0;
        }
    }
    return handle;
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

    if (!pModule || !pModule->ne_rsrctab) return NULL;

    pResTab = (LPBYTE)pModule + pModule->ne_rsrctab;
    pTypeInfo = (NE_TYPEINFO *)(pResTab + 2);

    TRACE("module=%04x type=%s\n", hModule, debugstr_a(typeId) );

    for (;;)
    {
        if (!(pTypeInfo = NE_FindTypeSection( pResTab, pTypeInfo, typeId )))
            break;
        prevHandler = pTypeInfo->resloader;
        pTypeInfo->resloader = resourceHandler;
        pTypeInfo = next_typeinfo(pTypeInfo);
    }
    if (!prevHandler) prevHandler = get_default_res_handler();
    return prevHandler;
}

static inline DWORD get_dword(LPCVOID *p)
{
    DWORD ret = *(const DWORD *)*p;
    *p = (const DWORD *)*p + 1;
    return ret;
}

static inline void put_dword(LPVOID *p, DWORD d)
{
    *(DWORD*)*p = d;
    *p = (DWORD *)*p + 1;
}

static inline WORD get_word(LPCVOID *p)
{
    WORD ret = *(const WORD *)*p;
    *p = (const WORD *)*p + 1;
    return ret;
}

static inline void put_word(LPVOID *p, WORD w)
{
    *(WORD*)*p = w;
    *p = (WORD *)*p + 1;
}

static inline BYTE get_byte(LPCVOID *p)
{
    BYTE ret = *(const BYTE *)*p;
    *p = (const BYTE *)*p + 1;
    return ret;
}

static inline void put_byte(LPVOID *p, BYTE b)
{
    *(BYTE *)*p = b;
    *p = (BYTE *)*p + 1;
}

/* convert a resource name */
static void convert_name( LPVOID *dst, LPCVOID *src )
{
    int len;
    switch (*(const WORD *)*src)
    {
    case 0x0000:
        get_word( src );
        put_byte( dst, 0 );
        break;
    case 0xffff:
        get_word( src );
        put_byte( dst, 0xff );
        put_word( dst, get_word(src) );
        break;
    default:
        len = WideCharToMultiByte( CP_ACP, 0, *src, -1, *dst, 0x7fffffff, NULL,NULL );
        *dst = (char *)*dst + len;
        *src = (LPCWSTR)*src + lstrlenW( *src ) + 1;
        break;
    }
}

/**********************************************************************
 *	    ConvertDialog32To16   (KERNEL.615)
 */
VOID WINAPI ConvertDialog32To16( LPCVOID dialog32, DWORD size, LPVOID dialog16 )
{
    WORD nbItems, data, dialogEx;
    DWORD style;

    style = get_dword( &dialog32 );
    put_dword( &dialog16, style );
    dialogEx = (style == 0xffff0001);  /* DIALOGEX resource */
    if (dialogEx)
    {
        put_dword( &dialog16, get_dword( &dialog32 ) );  /* helpID */
        put_dword( &dialog16, get_dword( &dialog32 ) );  /* exStyle */
        style = get_dword( &dialog32 );
        put_dword( &dialog16, style );                   /* style */
    }
    else
        dialog32 = (const DWORD *)dialog32 + 1; /* exStyle ignored in 16-bit standard dialog */

    nbItems = get_word( &dialog32 );
    put_byte( &dialog16, nbItems );
    put_word( &dialog16, get_word( &dialog32 ) ); /* x */
    put_word( &dialog16, get_word( &dialog32 ) ); /* y */
    put_word( &dialog16, get_word( &dialog32 ) ); /* cx */
    put_word( &dialog16, get_word( &dialog32 ) ); /* cy */

    /* Transfer menu name */
    convert_name( &dialog16, &dialog32 );

    /* Transfer class name */
    convert_name( &dialog16, &dialog32 );

    /* Transfer window caption */
    WideCharToMultiByte( CP_ACP, 0, dialog32, -1, dialog16, 0x7fffffff, NULL, NULL );
    dialog16 = (LPSTR)dialog16 + strlen( dialog16 ) + 1;
    dialog32 = (LPCWSTR)dialog32 + lstrlenW( dialog32 ) + 1;

    /* Transfer font info */
    if (style & DS_SETFONT)
    {
        put_word( &dialog16, get_word( &dialog32 ) );  /* pointSize */
        if (dialogEx)
        {
            put_word( &dialog16, get_word( &dialog32 ) ); /* weight */
            put_word( &dialog16, get_word( &dialog32 ) ); /* italic */
        }
        WideCharToMultiByte( CP_ACP, 0, dialog32, -1, dialog16, 0x7fffffff, NULL, NULL );  /* faceName */
        dialog16 = (LPSTR)dialog16 + strlen( dialog16 ) + 1;
        dialog32 = (LPCWSTR)dialog32 + lstrlenW( dialog32 ) + 1;
    }

    /* Transfer dialog items */
    while (nbItems)
    {
        /* align on DWORD boundary (32-bit only) */
        dialog32 = (LPCVOID)(((UINT_PTR)dialog32 + 3) & ~3);

        if (dialogEx)
        {
            put_dword( &dialog16, get_dword( &dialog32 ) ); /* helpID */
            put_dword( &dialog16, get_dword( &dialog32 ) ); /* exStyle */
            put_dword( &dialog16, get_dword( &dialog32 ) ); /* style */
        }
        else
        {
            style = get_dword( &dialog32 );          /* save style */
            dialog32 = (const DWORD *)dialog32 + 1;  /* ignore exStyle */
        }

        put_word( &dialog16, get_word( &dialog32 ) ); /* x */
        put_word( &dialog16, get_word( &dialog32 ) ); /* y */
        put_word( &dialog16, get_word( &dialog32 ) ); /* cx */
        put_word( &dialog16, get_word( &dialog32 ) ); /* cy */

        if (dialogEx)
            put_dword( &dialog16, get_dword( &dialog32 ) ); /* ID */
        else
        {
            put_word( &dialog16, get_word( &dialog32 ) ); /* ID */
            put_dword( &dialog16, style );  /* style from above */
        }

        /* Transfer class name */
        switch (*(const WORD *)dialog32)
        {
        case 0x0000:
            get_word( &dialog32 );
            put_byte( &dialog16, 0 );
            break;
        case 0xffff:
            get_word( &dialog32 );
            put_byte( &dialog16, get_word( &dialog32 ) );
            break;
        default:
            WideCharToMultiByte( CP_ACP, 0, dialog32, -1, dialog16, 0x7fffffff, NULL, NULL );
            dialog16 = (LPSTR)dialog16 + strlen( dialog16 ) + 1;
            dialog32 = (LPCWSTR)dialog32 + lstrlenW( dialog32 ) + 1;
            break;
        }

        /* Transfer window name */
        convert_name( &dialog16, &dialog32 );

        /* Transfer data */
        data = get_word( &dialog32 );
        if (dialogEx)
            put_word(&dialog16, data);
        else
            put_byte(&dialog16,(BYTE)data);

        if (data)
        {
            memcpy( dialog16, dialog32, data );
            dialog16 = (BYTE *)dialog16 + data;
            dialog32 = (const BYTE *)dialog32 + data;
        }

        /* Next item */
        nbItems--;
    }
}


/**********************************************************************
 *	    GetDialog32Size   (KERNEL.618)
 */
WORD WINAPI GetDialog32Size16( LPCVOID dialog32 )
{
    LPCVOID p = dialog32;
    WORD nbItems, data, dialogEx;
    DWORD style;

    style = get_dword(&p);
    dialogEx = (style == 0xffff0001);  /* DIALOGEX resource */
    if (dialogEx)
    {
        p = (const DWORD *)p + 1; /* helpID */
        p = (const DWORD *)p + 1; /* exStyle */
        style = get_dword(&p); /* style */
    }
    else
        p = (const DWORD *)p + 1; /* exStyle */

    nbItems = get_word(&p);
    p = (const WORD *)p + 1; /* x */
    p = (const WORD *)p + 1; /* y */
    p = (const WORD *)p + 1; /* cx */
    p = (const WORD *)p + 1; /* cy */

    /* Skip menu name */
    switch (*(const WORD *)p)
    {
    case 0x0000:  p = (const WORD *)p + 1; break;
    case 0xffff:  p = (const WORD *)p + 2; break;
    default:      p = (LPCWSTR)p + lstrlenW( p ) + 1; break;
    }

    /* Skip class name */
    switch (*(const WORD *)p)
    {
    case 0x0000:  p = (const WORD *)p + 1; break;
    case 0xffff:  p = (const WORD *)p + 2; break;
    default:      p = (LPCWSTR)p + lstrlenW( p ) + 1; break;
    }

    /* Skip window caption */
    p = (LPCWSTR)p + lstrlenW( p ) + 1;

    /* Skip font info */
    if (style & DS_SETFONT)
    {
        p = (const WORD *)p + 1;  /* pointSize */
        if (dialogEx)
        {
            p = (const WORD *)p + 1; /* weight */
            p = (const WORD *)p + 1; /* italic */
        }
        p = (LPCWSTR)p + lstrlenW( p ) + 1;  /* faceName */
    }

    /* Skip dialog items */
    while (nbItems)
    {
        /* align on DWORD boundary */
        p = (LPCVOID)(((UINT_PTR)p + 3) & ~3);

        if (dialogEx)
        {
            p = (const DWORD *)p + 1; /* helpID */
            p = (const DWORD *)p + 1; /* exStyle */
            p = (const DWORD *)p + 1; /* style */
        }
        else
        {
            p = (const DWORD *)p + 1; /* style */
            p = (const DWORD *)p + 1; /* exStyle */
        }

        p = (const WORD *)p + 1; /* x */
        p = (const WORD *)p + 1; /* y */
        p = (const WORD *)p + 1; /* cx */
        p = (const WORD *)p + 1; /* cy */

        if (dialogEx)
            p = (const DWORD *)p + 1; /* ID */
        else
            p = (const WORD *)p + 1; /* ID */

        /* Skip class name */
        switch (*(const WORD *)p)
        {
        case 0x0000:  p = (const WORD *)p + 1; break;
        case 0xffff:  p = (const WORD *)p + 2; break;
        default:      p = (LPCWSTR)p + lstrlenW( p ) + 1; break;
        }

        /* Skip window name */
        switch (*(const WORD *)p)
        {
        case 0x0000:  p = (const WORD *)p + 1; break;
        case 0xffff:  p = (const WORD *)p + 2; break;
        default:      p = (LPCWSTR)p + lstrlenW( p ) + 1; break;
        }

        /* Skip data */
        data = get_word(&p);
        p = (const BYTE *)p + data;

        /* Next item */
        nbItems--;
    }

    return (WORD)((LPCSTR)p - (LPCSTR)dialog32);
}


/**********************************************************************
 *	    ConvertMenu32To16   (KERNEL.616)
 */
VOID WINAPI ConvertMenu32To16( LPCVOID menu32, DWORD size, LPVOID menu16 )
{
    WORD version, headersize, flags, level = 1;

    version = get_word( &menu32 );
    headersize = get_word( &menu32 );
    put_word( &menu16, version );
    put_word( &menu16, headersize );
    if ( headersize )
    {
        memcpy( menu16, menu32, headersize );
        menu16 = (BYTE *)menu16 + headersize;
        menu32 = (const BYTE *)menu32 + headersize;
    }

    while ( level )
        if ( version == 0 )  /* standard */
        {
            flags = get_word( &menu32 );
            put_word( &menu16, flags );
            if ( !(flags & MF_POPUP) )
                put_word( &menu16, get_word( &menu32 ) );  /* ID */
            else
                level++;

            WideCharToMultiByte( CP_ACP, 0, menu32, -1, menu16, 0x7fffffff, NULL, NULL );
            menu16 = (LPSTR)menu16 + strlen( menu16 ) + 1;
            menu32 = (LPCWSTR)menu32 + lstrlenW( menu32 ) + 1;

            if ( flags & MF_END )
                level--;
        }
        else  /* extended */
        {
            put_dword( &menu16, get_dword( &menu32 ) );  /* fType */
            put_dword( &menu16, get_dword( &menu32 ) );  /* fState */
            put_word( &menu16, get_dword( &menu32 ) );   /* ID */
            flags = get_word( &menu32 );
            put_byte(&menu16,flags);

            WideCharToMultiByte( CP_ACP, 0, menu32, -1, menu16, 0x7fffffff, NULL, NULL );
            menu16 = (LPSTR)menu16 + strlen( menu16 ) + 1;
            menu32 = (LPCWSTR)menu32 + lstrlenW( menu32 ) + 1;

            /* align on DWORD boundary (32-bit only) */
            menu32 = (LPCVOID)(((UINT_PTR)menu32 + 3) & ~3);

            /* If popup, transfer helpid */
            if ( flags & 1)
            {
                put_dword( &menu16, get_dword( &menu32 ) );
                level++;
            }

            if ( flags & MF_END )
                level--;
        }
}


/**********************************************************************
 *	    GetMenu32Size   (KERNEL.617)
 */
WORD WINAPI GetMenu32Size16( LPCVOID menu32 )
{
    LPCVOID p = menu32;
    WORD version, headersize, flags, level = 1;

    version = get_word(&p);
    headersize = get_word(&p);
    p = (const BYTE *)p + headersize;

    while ( level )
        if ( version == 0 )  /* standard */
        {
            flags = get_word(&p);
            if ( !(flags & MF_POPUP) )
                p = (const WORD *)p + 1;  /* ID */
            else
                level++;

            p = (LPCWSTR)p + lstrlenW( p ) + 1;

            if ( flags & MF_END )
                level--;
        }
        else  /* extended */
        {
            p = (const DWORD *)p + 1; /* fType */
            p = (const DWORD *)p + 1; /* fState */
            p = (const DWORD *)p + 1; /* ID */
            flags = get_word(&p);

            p = (LPCWSTR)p + lstrlenW( p ) + 1;

            /* align on DWORD boundary (32-bit only) */
            p = (LPCVOID)(((UINT_PTR)p + 3) & ~3);

            /* If popup, skip helpid */
            if ( flags & 1)
            {
                p = (const DWORD *)p + 1;
                level++;
            }

            if ( flags & MF_END )
                level--;
        }

    return (WORD)((LPCSTR)p - (LPCSTR)menu32);
}


/**********************************************************************
 *	    ConvertAccelerator32To16
 */
static void ConvertAccelerator32To16( LPCVOID acc32, DWORD size, LPVOID acc16 )
{
    BYTE type;

    do
    {
        /* Copy type */
        type = get_byte(&acc32);
        put_byte(&acc16, type);
        /* Skip padding */
        get_byte(&acc32);
        /* Copy event and IDval */
        put_word(&acc16, get_word(&acc32));
        put_word(&acc16, get_word(&acc32));
        /* Skip padding */
        get_word(&acc32);

    } while ( !( type & 0x80 ) );
}


/**********************************************************************
 *	    NE_LoadPEResource
 */
static HGLOBAL16 NE_LoadPEResource( NE_MODULE *pModule, WORD type, LPCVOID bits, DWORD size )
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
    if (!pModule || !pModule->ne_rsrctab || !hRsrc) return 0;

    TRACE("module=%04x res=%04x size=%ld\n", hModule, hRsrc, size );

    sizeShift = *(WORD *)((char *)pModule + pModule->ne_rsrctab);
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

    if (!pModule || !pModule->ne_rsrctab || !hRsrc) return -1;

    TRACE("module=%04x res=%04x\n", pModule->self, hRsrc );

    if ((fd = _lopen16( NE_MODULE_NAME(pModule), OF_READ )) != HFILE_ERROR16)
    {
        WORD sizeShift = *(WORD *)((char *)pModule + pModule->ne_rsrctab);
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

    if (!pModule->ne_rsrctab) return 0;

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
    pResTab = (LPBYTE)pModule + pModule->ne_rsrctab;
    pTypeInfo = (NE_TYPEINFO *)( pResTab + 2 );

    for (;;)
    {
        if (!(pTypeInfo = NE_FindTypeSection( pResTab, pTypeInfo, type ))) break;
        if ((pNameInfo = NE_FindResourceFromType( pResTab, pTypeInfo, name )))
        {
            TRACE("    Found id %p\n", name );
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

    d = pModule->ne_rsrctab + 2;
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
            FARPROC16 resloader = pTypeInfo->resloader;
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
    return WOWGlobalLock16( handle );
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
    NE_MODULE *pModule;

    TRACE("(%x, %x)\n", hModule, hRsrc );

    if (!hRsrc) return 0;
    if (!(pModule = get_module( hModule ))) return 0;
    if (pModule->ne_rsrctab)
    {
        WORD sizeShift = *(WORD *)((char *)pModule + pModule->ne_rsrctab);
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


/**********************************************************************
 *          FreeResource     (KERNEL.63)
 */
BOOL16 WINAPI FreeResource16( HGLOBAL16 handle )
{
    FARPROC16 proc;
    HMODULE16 user;
    NE_MODULE *pModule = NE_GetPtr( FarGetOwner16( handle ) );

    TRACE("(%04x)\n", handle );

    /* Try NE resource first */

    if (pModule && pModule->ne_rsrctab)
    {
        NE_TYPEINFO *pTypeInfo = (NE_TYPEINFO *)((char *)pModule + pModule->ne_rsrctab + 2);
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
                    return FALSE;
                }
                pNameInfo++;
            }
            pTypeInfo = (NE_TYPEINFO *)pNameInfo;
        }
    }

    /* If this failed, call USER.DestroyIcon32; this will check
       whether it is a shared cursor/icon; if not it will call
       GlobalFree16() */
    user = GetModuleHandle16( "user" );
    if (user && (proc = GetProcAddress16( user, "DestroyIcon32" )))
    {
        WORD args[2];
        DWORD result;

        args[1] = handle;
        args[0] = 1;  /* CID_RESOURCE */
        WOWCallback16Ex( (SEGPTR)proc, WCB16_PASCAL, sizeof(args), args, &result );
        return LOWORD(result);
    }
    else
        return GlobalFree16( handle );
}
