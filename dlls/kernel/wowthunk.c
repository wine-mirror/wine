/*
 * Win32 WOW Generic Thunk API
 *
 * Copyright 1999 Ulrich Weigand
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

#include "wine/winbase16.h"
#include "winbase.h"
#include "winerror.h"
#include "wownt32.h"
#include "winternl.h"
#include "file.h"
#include "task.h"
#include "miscemu.h"
#include "stackframe.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(thunk);

/*
 * These are the 16-bit side WOW routines.  They reside in wownt16.h
 * in the SDK; since we don't support Win16 source code anyway, I've
 * placed them here for compilation with Wine ...
 */

DWORD WINAPI GetVDMPointer32W16(SEGPTR,UINT16);

DWORD WINAPI LoadLibraryEx32W16(LPCSTR,DWORD,DWORD);
DWORD WINAPI GetProcAddress32W16(DWORD,LPCSTR);
DWORD WINAPI FreeLibrary32W16(DWORD);

#define CPEX_DEST_STDCALL   0x00000000L
#define CPEX_DEST_CDECL     0x80000000L

DWORD WINAPI CallProcExW16(VOID);
DWORD WINAPI CallProcEx32W16(VOID);

/*
 *  32-bit WOW routines (in WOW32, but actually forwarded to KERNEL32)
 */

/**********************************************************************
 *           K32WOWGetDescriptor        (KERNEL32.70)
 */
BOOL WINAPI K32WOWGetDescriptor( SEGPTR segptr, LPLDT_ENTRY ldtent )
{
    return GetThreadSelectorEntry( GetCurrentThread(),
                                   segptr >> 16, ldtent );
}

/**********************************************************************
 *           K32WOWGetVDMPointer        (KERNEL32.56)
 */
LPVOID WINAPI K32WOWGetVDMPointer( DWORD vp, DWORD dwBytes, BOOL fProtectedMode )
{
    /* FIXME: add size check too */

    if ( fProtectedMode )
        return MapSL( vp );
    else
        return DOSMEM_MapRealToLinear( vp );
}

/**********************************************************************
 *           K32WOWGetVDMPointerFix     (KERNEL32.68)
 */
LPVOID WINAPI K32WOWGetVDMPointerFix( DWORD vp, DWORD dwBytes, BOOL fProtectedMode )
{
    /*
     * Hmmm. According to the docu, we should call:
     *
     *          GlobalFix16( SELECTOROF(vp) );
     *
     * But this is unnecessary under Wine, as we never move global
     * memory segments in linear memory anyway.
     *
     * (I'm not so sure what we are *supposed* to do if
     *  fProtectedMode is TRUE, anyway ...)
     */

    return K32WOWGetVDMPointer( vp, dwBytes, fProtectedMode );
}

/**********************************************************************
 *           K32WOWGetVDMPointerUnfix   (KERNEL32.69)
 */
VOID WINAPI K32WOWGetVDMPointerUnfix( DWORD vp )
{
    /*
     * See above why we don't call:
     *
     * GlobalUnfix16( SELECTOROF(vp) );
     *
     */
}

/**********************************************************************
 *           K32WOWGlobalAlloc16        (KERNEL32.59)
 */
WORD WINAPI K32WOWGlobalAlloc16( WORD wFlags, DWORD cb )
{
    return (WORD)GlobalAlloc16( wFlags, cb );
}

/**********************************************************************
 *           K32WOWGlobalFree16         (KERNEL32.62)
 */
WORD WINAPI K32WOWGlobalFree16( WORD hMem )
{
    return (WORD)GlobalFree16( (HGLOBAL16)hMem );
}

/**********************************************************************
 *           K32WOWGlobalUnlock16       (KERNEL32.61)
 */
BOOL WINAPI K32WOWGlobalUnlock16( WORD hMem )
{
    return (BOOL)GlobalUnlock16( (HGLOBAL16)hMem );
}

/**********************************************************************
 *           K32WOWGlobalAllocLock16    (KERNEL32.63)
 */
DWORD WINAPI K32WOWGlobalAllocLock16( WORD wFlags, DWORD cb, WORD *phMem )
{
    WORD hMem = K32WOWGlobalAlloc16( wFlags, cb );
    if (phMem) *phMem = hMem;

    return K32WOWGlobalLock16( hMem );
}

/**********************************************************************
 *           K32WOWGlobalLockSize16     (KERNEL32.65)
 */
DWORD WINAPI K32WOWGlobalLockSize16( WORD hMem, PDWORD pcb )
{
    if ( pcb )
        *pcb = GlobalSize16( (HGLOBAL16)hMem );

    return K32WOWGlobalLock16( hMem );
}

/**********************************************************************
 *           K32WOWGlobalUnlockFree16   (KERNEL32.64)
 */
WORD WINAPI K32WOWGlobalUnlockFree16( DWORD vpMem )
{
    if ( !K32WOWGlobalUnlock16( HIWORD(vpMem) ) )
        return FALSE;

    return K32WOWGlobalFree16( HIWORD(vpMem) );
}


/**********************************************************************
 *           K32WOWYield16              (KERNEL32.66)
 */
VOID WINAPI K32WOWYield16( void )
{
    /*
     * This does the right thing for both Win16 and Win32 tasks.
     * More or less, at least :-/
     */
    Yield16();
}

/**********************************************************************
 *           K32WOWDirectedYield16       (KERNEL32.67)
 */
VOID WINAPI K32WOWDirectedYield16( WORD htask16 )
{
    /*
     * Argh.  Our scheduler doesn't like DirectedYield by Win32
     * tasks at all.  So we do hope that this routine is indeed
     * only ever called by Win16 tasks that have thunked up ...
     */
    DirectedYield16( (HTASK16)htask16 );
}


/***********************************************************************
 *           K32WOWHandle32              (KERNEL32.57)
 */
HANDLE WINAPI K32WOWHandle32( WORD handle, WOW_HANDLE_TYPE type )
{
    switch ( type )
    {
    case WOW_TYPE_HWND:
    case WOW_TYPE_HMENU:
    case WOW_TYPE_HDWP:
    case WOW_TYPE_HDROP:
    case WOW_TYPE_HDC:
    case WOW_TYPE_HFONT:
    case WOW_TYPE_HRGN:
    case WOW_TYPE_HBITMAP:
    case WOW_TYPE_HBRUSH:
    case WOW_TYPE_HPALETTE:
    case WOW_TYPE_HPEN:
    case WOW_TYPE_HACCEL:
        return (HANDLE)(ULONG_PTR)handle;

    case WOW_TYPE_HMETAFILE:
        FIXME( "conversion of metafile handles not supported yet\n" );
        return (HANDLE)(ULONG_PTR)handle;

    case WOW_TYPE_HTASK:
        return (HANDLE)TASK_GetPtr(handle)->teb->tid;

    case WOW_TYPE_FULLHWND:
        FIXME( "conversion of full window handles not supported yet\n" );
        return (HANDLE)(ULONG_PTR)handle;

    default:
        ERR( "handle 0x%04x of unknown type %d\n", handle, type );
        return (HANDLE)(ULONG_PTR)handle;
    }
}

/***********************************************************************
 *           K32WOWHandle16              (KERNEL32.58)
 */
WORD WINAPI K32WOWHandle16( HANDLE handle, WOW_HANDLE_TYPE type )
{
    switch ( type )
    {
    case WOW_TYPE_HWND:
    case WOW_TYPE_HMENU:
    case WOW_TYPE_HDWP:
    case WOW_TYPE_HDROP:
    case WOW_TYPE_HDC:
    case WOW_TYPE_HFONT:
    case WOW_TYPE_HRGN:
    case WOW_TYPE_HBITMAP:
    case WOW_TYPE_HBRUSH:
    case WOW_TYPE_HPALETTE:
    case WOW_TYPE_HPEN:
    case WOW_TYPE_HACCEL:
    case WOW_TYPE_FULLHWND:
    	if ( HIWORD(handle ) )
        	ERR( "handle %p of type %d has non-zero HIWORD\n", handle, type );
        return LOWORD(handle);

    case WOW_TYPE_HMETAFILE:
        FIXME( "conversion of metafile handles not supported yet\n" );
        return LOWORD(handle);

    case WOW_TYPE_HTASK:
        return THREAD_IdToTEB((DWORD)handle)->htask16;

    default:
        ERR( "handle %p of unknown type %d\n", handle, type );
        return LOWORD(handle);
    }
}

/**********************************************************************
 *           K32WOWCallback16Ex         (KERNEL32.55)
 */
BOOL WINAPI K32WOWCallback16Ex( DWORD vpfn16, DWORD dwFlags,
                                DWORD cbArgs, LPVOID pArgs, LPDWORD pdwRetCode )
{
    DWORD ret;

    /*
     * Arguments must be prepared in the correct order by the caller
     * (both for PASCAL and CDECL calling convention), so we simply
     * copy them to the 16-bit stack ...
     */
    memcpy( (LPBYTE)CURRENT_STACK16 - cbArgs, (LPBYTE)pArgs, cbArgs );


    /*
     * Actually, we should take care whether the called routine cleans up
     * its stack or not.  Fortunately, our wine_call_to_16 core doesn't rely on
     * the callee to do so; after the routine has returned, the 16-bit
     * stack pointer is always reset to the position it had before.
     */

    ret = wine_call_to_16( (FARPROC16)vpfn16, cbArgs );

    if ( pdwRetCode )
        *pdwRetCode = ret;

    return TRUE;  /* success */
}

/**********************************************************************
 *           K32WOWCallback16            (KERNEL32.54)
 */
DWORD WINAPI K32WOWCallback16( DWORD vpfn16, DWORD dwParam )
{
    DWORD ret;

    if ( !K32WOWCallback16Ex( vpfn16, WCB16_PASCAL,
                           sizeof(DWORD), &dwParam, &ret ) )
        ret = 0L;

    return ret;
}


/*
 *  16-bit WOW routines (in KERNEL)
 */

/**********************************************************************
 *           GetVDMPointer32W      (KERNEL.516)
 */
DWORD WINAPI GetVDMPointer32W16( SEGPTR vp, UINT16 fMode )
{
    GlobalPageLock16(GlobalHandle16(SELECTOROF(vp)));
    return (DWORD)K32WOWGetVDMPointer( vp, 0, (DWORD)fMode );
}

/***********************************************************************
 *           LoadLibraryEx32W      (KERNEL.513)
 */
DWORD WINAPI LoadLibraryEx32W16( LPCSTR lpszLibFile, DWORD hFile, DWORD dwFlags )
{
    HMODULE hModule;
    DOS_FULL_NAME full_name;
    DWORD mutex_count;
    UNICODE_STRING libfileW;
    LPCWSTR filenameW;
    static const WCHAR dllW[] = {'.','D','L','L',0};

    if (!lpszLibFile)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    if (!RtlCreateUnicodeStringFromAsciiz(&libfileW, lpszLibFile))
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return 0;
    }

    /* if the file can not be found, call LoadLibraryExA anyway, since it might be
       a buildin module. This case is handled in MODULE_LoadLibraryExA */

    filenameW = libfileW.Buffer;
    if ( DIR_SearchPath( NULL, filenameW, dllW, &full_name, FALSE ) )
        filenameW = full_name.short_name;

    ReleaseThunkLock( &mutex_count );
    hModule = LoadLibraryExW( filenameW, (HANDLE)hFile, dwFlags );
    RestoreThunkLock( mutex_count );

    RtlFreeUnicodeString(&libfileW);

    return (DWORD)hModule;
}

/***********************************************************************
 *           GetProcAddress32W     (KERNEL.515)
 */
DWORD WINAPI GetProcAddress32W16( DWORD hModule, LPCSTR lpszProc )
{
    return (DWORD)GetProcAddress( (HMODULE)hModule, lpszProc );
}

/***********************************************************************
 *           FreeLibrary32W        (KERNEL.514)
 */
DWORD WINAPI FreeLibrary32W16( DWORD hLibModule )
{
    BOOL retv;
    DWORD mutex_count;

    ReleaseThunkLock( &mutex_count );
    retv = FreeLibrary( (HMODULE)hLibModule );
    RestoreThunkLock( mutex_count );
    return (DWORD)retv;
}


/**********************************************************************
 *           WOW_CallProc32W
 */
static DWORD WOW_CallProc32W16( BOOL Ex )
{
    DWORD nrofargs, argconvmask;
    FARPROC proc32;
    DWORD *args, ret;
    DWORD mutex_count;
    VA_LIST16 valist;
    int i;
    int aix;

    ReleaseThunkLock( &mutex_count );

    VA_START16( valist );
    nrofargs    = VA_ARG16( valist, DWORD );
    argconvmask = VA_ARG16( valist, DWORD );
    proc32      = VA_ARG16( valist, FARPROC );
    TRACE("(%ld,%ld,%p, Ex%d args[",nrofargs,argconvmask,proc32,Ex);
    args = (DWORD*)HeapAlloc( GetProcessHeap(), 0, sizeof(DWORD)*nrofargs );
    if(args == NULL) proc32 = NULL; /* maybe we should WARN here? */
    /* CallProcEx doesn't need its args reversed */
    for (i=0;i<nrofargs;i++) {
            if (Ex) {
               aix = i;
            } else {
               aix = nrofargs - i - 1;
            }
            if (argconvmask & (1<<i))
            {
                SEGPTR ptr = VA_ARG16( valist, SEGPTR );
                if (args) args[aix] = (DWORD)MapSL(ptr);
                if (TRACE_ON(thunk)) DPRINTF("%08lx(%p),",ptr,MapSL(ptr));
            }
            else
            {
		DWORD arg = VA_ARG16( valist, DWORD );
                if (args) args[aix] = arg;
                if (TRACE_ON(thunk)) DPRINTF("%ld,", arg);
            }
    }
    if (TRACE_ON(thunk)) DPRINTF("])\n");
    VA_END16( valist );

    /*
     * FIXME:  If ( nrofargs & CPEX_DEST_CDECL ) != 0, we should call a
     *         32-bit CDECL routine ...
     */

    if (!proc32) ret = 0;
    else switch (nrofargs)
    {
    case 0: ret = proc32();
            break;
    case 1: ret = proc32(args[0]);
            break;
    case 2: ret = proc32(args[0],args[1]);
            break;
    case 3: ret = proc32(args[0],args[1],args[2]);
            break;
    case 4: ret = proc32(args[0],args[1],args[2],args[3]);
            break;
    case 5: ret = proc32(args[0],args[1],args[2],args[3],args[4]);
            break;
    case 6: ret = proc32(args[0],args[1],args[2],args[3],args[4],args[5]);
            break;
    case 7: ret = proc32(args[0],args[1],args[2],args[3],args[4],args[5],args[6]);
            break;
    case 8: ret = proc32(args[0],args[1],args[2],args[3],args[4],args[5],args[6],args[7]);
            break;
    case 9: ret = proc32(args[0],args[1],args[2],args[3],args[4],args[5],args[6],args[7],args[8]);
            break;
    case 10:ret = proc32(args[0],args[1],args[2],args[3],args[4],args[5],args[6],args[7],args[8],args[9]);
            break;
    case 11:ret = proc32(args[0],args[1],args[2],args[3],args[4],args[5],args[6],args[7],args[8],args[9],args[10]);
            break;
    case 12:ret = proc32(args[0],args[1],args[2],args[3],args[4],args[5],args[6],args[7],args[8],args[9],args[10],args[11]);
            break;
    case 13:ret = proc32(args[0],args[1],args[2],args[3],args[4],args[5],args[6],args[7],args[8],args[9],args[10],args[11],args[12]);
            break;
    default:
            /* FIXME: should go up to 32  arguments */
            ERR("Unsupported number of arguments %ld, please report.\n",nrofargs);
            ret = 0;
            break;
    }

    /* POP nrofargs DWORD arguments and 3 DWORD parameters */
    if (!Ex) stack16_pop( (3 + nrofargs) * sizeof(DWORD) );

    TRACE("returns %08lx\n",ret);
    HeapFree( GetProcessHeap(), 0, args );

    RestoreThunkLock( mutex_count );
    return ret;
}

/**********************************************************************
 *           CallProc32W           (KERNEL.517)
 *
 * DWORD PASCAL CallProc32W( DWORD p1, ... , DWORD lpProcAddress,
 *                           DWORD fAddressConvert, DWORD cParams );
 */
DWORD WINAPI CallProc32W16( void )
{
    return WOW_CallProc32W16( FALSE );
}

/**********************************************************************
 *           _CallProcEx32W         (KERNEL.518)
 *
 * DWORD CallProcEx32W( DWORD cParams, DWORD fAddressConvert,
 *                      DWORD lpProcAddress, DWORD p1, ... );
 */
DWORD WINAPI CallProcEx32W16( void )
{
    return WOW_CallProc32W16( TRUE );
}


/**********************************************************************
 *           WOW16Call               (KERNEL.500)
 *
 * FIXME!!!
 *
 */
DWORD WINAPI WOW16Call(WORD x,WORD y,WORD z)
{
        int     i;
        DWORD   calladdr;
        VA_LIST16 args;
        FIXME("(0x%04x,0x%04x,%d),calling (",x,y,z);

        VA_START16(args);
        for (i=0;i<x/2;i++) {
                WORD    a = VA_ARG16(args,WORD);
                DPRINTF("%04x ",a);
        }
        calladdr = VA_ARG16(args,DWORD);
        VA_END16(args);
        stack16_pop( x + sizeof(DWORD) );
        DPRINTF(") calling address was 0x%08lx\n",calladdr);
        return 0;
}
