/*
 * PE->NE resource conversion functions
 *
 * Copyright 1998 Ulrich Weigand
 */

#include "wine/winuser16.h"
#include "module.h"
#include "debug.h"
#include "debugtools.h"

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
    default:      lstrcpyWtoA( (LPSTR)dialog16, (LPWSTR)p );
                  ((LPSTR)dialog16) += lstrlenA( (LPSTR)dialog16 ) + 1;
                  ((LPWSTR)p) += lstrlenW( (LPWSTR)p ) + 1;
                  break;
    }

    /* Transfer class name */
    switch (*((WORD *)p))
    {
    case 0x0000:  ((WORD *)p)++; *((BYTE *)dialog16)++ = 0; break;
    case 0xffff:  ((WORD *)p)++; *((BYTE *)dialog16)++ = 0xff; 
                  *((WORD *)dialog16)++ = *((WORD *)p)++; break;
    default:      lstrcpyWtoA( (LPSTR)dialog16, (LPWSTR)p );
                  ((LPSTR)dialog16) += lstrlenA( (LPSTR)dialog16 ) + 1;
                  ((LPWSTR)p) += lstrlenW( (LPWSTR)p ) + 1;
                  break;
    }

    /* Transfer window caption */
    lstrcpyWtoA( (LPSTR)dialog16, (LPWSTR)p );
    ((LPSTR)dialog16) += lstrlenA( (LPSTR)dialog16 ) + 1;
    ((LPWSTR)p) += lstrlenW( (LPWSTR)p ) + 1;

    /* Transfer font info */
    if (style & DS_SETFONT)
    {
        *((WORD *)dialog16)++ = *((WORD *)p)++;  /* pointSize */
        if (dialogEx)
        {
            *((WORD *)dialog16)++ = *((WORD *)p)++; /* weight */
            *((WORD *)dialog16)++ = *((WORD *)p)++; /* italic */
        }
        lstrcpyWtoA( (LPSTR)dialog16, (LPWSTR)p );  /* faceName */
        ((LPSTR)dialog16) += lstrlenA( (LPSTR)dialog16 ) + 1;
        ((LPWSTR)p) += lstrlenW( (LPWSTR)p ) + 1;
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
        default:      lstrcpyWtoA( (LPSTR)dialog16, (LPWSTR)p );
                      ((LPSTR)dialog16) += lstrlenA( (LPSTR)dialog16 ) + 1;
                      ((LPWSTR)p) += lstrlenW( (LPWSTR)p ) + 1;
                      break;
        }

        /* Transfer window name */
        switch (*((WORD *)p))
        {
        case 0x0000:  ((WORD *)p)++; *((BYTE *)dialog16)++ = 0; break;
        case 0xffff:  ((WORD *)p)++; *((BYTE *)dialog16)++ = 0xff; 
                      *((WORD *)dialog16)++ = *((WORD *)p)++; break;
        default:      lstrcpyWtoA( (LPSTR)dialog16, (LPWSTR)p );
                      ((LPSTR)dialog16) += lstrlenA( (LPSTR)dialog16 ) + 1;
                      ((LPWSTR)p) += lstrlenW( (LPWSTR)p ) + 1;
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
    default:      ((LPWSTR)p) += lstrlenW( (LPWSTR)p ) + 1; break;
    }

    /* Skip class name */
    switch (*((WORD *)p))
    {
    case 0x0000:  ((WORD *)p)++; break;
    case 0xffff:  ((WORD *)p) += 2; break;
    default:      ((LPWSTR)p) += lstrlenW( (LPWSTR)p ) + 1; break;
    }

    /* Skip window caption */
    ((LPWSTR)p) += lstrlenW( (LPWSTR)p ) + 1; 

    /* Skip font info */
    if (style & DS_SETFONT)
    {
        ((WORD *)p)++;  /* pointSize */
        if (dialogEx)
        {
            ((WORD *)p)++; /* weight */
            ((WORD *)p)++; /* italic */
        }
        ((LPWSTR)p) += lstrlenW( (LPWSTR)p ) + 1;  /* faceName */
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
        default:      ((LPWSTR)p) += lstrlenW( (LPWSTR)p ) + 1; break;
        }

        /* Skip window name */
        switch (*((WORD *)p))
        {
        case 0x0000:  ((WORD *)p)++; break;
        case 0xffff:  ((WORD *)p) += 2; break;
        default:      ((LPWSTR)p) += lstrlenW( (LPWSTR)p ) + 1; break;
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
       
            lstrcpyWtoA( (LPSTR)menu16, (LPWSTR)p );
            ((LPSTR)menu16) += lstrlenA( (LPSTR)menu16 ) + 1;
            ((LPWSTR)p) += lstrlenW( (LPWSTR)p ) + 1;

            if ( flags & MF_END )
                level--;
        }
        else  /* extended */
        {
            *((DWORD *)menu16)++ = *((DWORD *)p)++;  /* fType */
            *((DWORD *)menu16)++ = *((DWORD *)p)++;  /* fState */
            *((WORD *)menu16)++ = (WORD)*((DWORD *)p)++; /* ID */
            flags = *((BYTE *)menu16)++ = (BYTE)*((WORD *)p)++;  
       
            lstrcpyWtoA( (LPSTR)menu16, (LPWSTR)p );
            ((LPSTR)menu16) += lstrlenA( (LPSTR)menu16 ) + 1;
            ((LPWSTR)p) += lstrlenW( (LPWSTR)p ) + 1;

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
       
            ((LPWSTR)p) += lstrlenW( (LPWSTR)p ) + 1;

            if ( flags & MF_END )
                level--;
        }
        else  /* extended */
        {
            ((DWORD *)p)++; /* fType */
            ((DWORD *)p)++; /* fState */
            ((DWORD *)p)++; /* ID */
            flags = *((WORD *)p)++; 
       
            ((LPWSTR)p) += lstrlenW( (LPWSTR)p ) + 1;

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
VOID ConvertAccelerator32To16( LPVOID acc32, DWORD size, LPVOID acc16 )
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
HGLOBAL16 NE_LoadPEResource( NE_MODULE *pModule, WORD type, LPVOID bits, DWORD size )
{
    HGLOBAL16 handle;

    TRACE( resource, "module=%04x type=%04x\n", pModule->self, type );
    if (!pModule || !bits || !size) return 0;

    handle = GlobalAlloc16( 0, size );
   
    switch (type)
    {
    case RT_MENU16:
        ConvertMenu32To16( bits, size, GlobalLock16( handle ) );
        break;

    case RT_DIALOG16:
        ConvertDialog32To16( bits, size, GlobalLock16( handle ) );
        break;

    case RT_ACCELERATOR16:
        ConvertAccelerator32To16( bits, size, GlobalLock16( handle ) );
        break;

    case RT_STRING16:
        FIXME( resource, "not yet implemented!\n" );
        /* fall through */

    default:
        memcpy( GlobalLock16( handle ), bits, size );
        break;
    }

    return handle;
}

/**********************************************************************
 *	    NE_FreePEResource
 */
BOOL16 NE_FreePEResource( NE_MODULE *pModule, HGLOBAL16 handle )
{
    GlobalFree16( handle );
    return 0;
}

