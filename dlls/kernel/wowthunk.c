/*
 * Win32 WOW Generic Thunk API
 *
 * Copyright 1999 Ulrich Weigand 
 */

#include "wine/winbase16.h"
#include "winbase.h"
#include "wownt32.h"
#include "heap.h"
#include "miscemu.h"
#include "syslevel.h"
#include "stackframe.h"
#include "builtin16.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(thunk)

/*
 *  32-bit WOW routines (in WOW32, but actually forwarded to KERNEL32)
 */

/**********************************************************************
 *           WOWGetDescriptor        (WOW32.1) (KERNEL32.70)
 */
BOOL WINAPI WOWGetDescriptor( SEGPTR segptr, LPLDT_ENTRY ldtent )
{
    return GetThreadSelectorEntry( GetCurrentThread(), 
                                   segptr >> 16, ldtent );
}

/**********************************************************************
 *           WOWGetVDMPointer        (WOW32.5) (KERNEL32.56)
 */
LPVOID WINAPI WOWGetVDMPointer( DWORD vp, DWORD dwBytes, BOOL fProtectedMode )
{
    /* FIXME: add size check too */

    if ( fProtectedMode )
        return PTR_SEG_TO_LIN( vp );
    else
        return DOSMEM_MapRealToLinear( vp );
}

/**********************************************************************
 *           WOWGetVDMPointerFix     (WOW32.6) (KERNEL32.68)
 */
LPVOID WINAPI WOWGetVDMPointerFix( DWORD vp, DWORD dwBytes, BOOL fProtectedMode )
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

    return WOWGetVDMPointer( vp, dwBytes, fProtectedMode );
}

/**********************************************************************
 *           WOWGetVDMPointerUnFix   (WOW32.7) (KERNEL32.69)
 */
VOID WINAPI WOWGetVDMPointerUnfix( DWORD vp )
{
    /*
     * See above why we don't call:
     *
     * GlobalUnfix16( SELECTOROF(vp) );
     *
     */
}

/**********************************************************************
 *           WOWGlobalAlloc16        (WOW32.8) (KERNEL32.59)
 */
WORD WINAPI WOWGlobalAlloc16( WORD wFlags, DWORD cb )
{
    return (WORD)GlobalAlloc16( wFlags, cb );
}

/**********************************************************************
 *           WOWGlobalFree16         (WOW32.10) (KERNEL32.62)
 */
WORD WINAPI WOWGlobalFree16( WORD hMem )
{
    return (WORD)GlobalFree16( (HGLOBAL16)hMem );
}

/**********************************************************************
 *           WOWGlobalLock16         (WOW32.11) (KERNEL32.60)
 */
DWORD WINAPI WOWGlobalLock16( WORD hMem )
{
    return (DWORD)WIN16_GlobalLock16( (HGLOBAL16)hMem );
}

/**********************************************************************
 *           WOWGlobalUnlock16       (WOW32.13) (KERNEL32.61)
 */
BOOL WINAPI WOWGlobalUnlock16( WORD hMem )
{
    return (BOOL)GlobalUnlock16( (HGLOBAL16)hMem );
}

/**********************************************************************
 *           WOWGlobalAllocLock16    (WOW32.9) (KERNEL32.63)
 */
DWORD WINAPI WOWGlobalAllocLock16( WORD wFlags, DWORD cb, WORD *phMem )
{
    WORD hMem = WOWGlobalAlloc16( wFlags, cb );
    if (phMem) *phMem = hMem;

    return WOWGlobalLock16( hMem );
}

/**********************************************************************
 *           WOWGlobalLockSize16     (WOW32.12) (KERNEL32.65)
 */
DWORD WINAPI WOWGlobalLockSize16( WORD hMem, PDWORD pcb )
{
    if ( pcb ) 
        *pcb = GlobalSize16( (HGLOBAL16)hMem );

    return WOWGlobalLock16( hMem );
}

/**********************************************************************
 *           WOWGlobalUnlockFree16   (WOW32.14) (KERNEL32.64)
 */
WORD WINAPI WOWGlobalUnlockFree16( DWORD vpMem )
{
    if ( !WOWGlobalUnlock16( HIWORD(vpMem) ) )
        return FALSE;

    return WOWGlobalFree16( HIWORD(vpMem) );
}


/**********************************************************************
 *           WOWYield16              (WOW32.17) (KERNEL32.66)
 */
VOID WINAPI WOWYield16( void )
{
    /*
     * This does the right thing for both Win16 and Win32 tasks.  
     * More or less, at least :-/
     */
    Yield16();
}

/**********************************************************************
 *           WOWDirectedYield16       (WOW32.4) (KERNEL32.67)
 */
VOID WINAPI WOWDirectedYield16( WORD htask16 )
{
    /*
     * Argh.  Our scheduler doesn't like DirectedYield by Win32
     * tasks at all.  So we do hope that this routine is indeed 
     * only ever called by Win16 tasks that have thunked up ...
     */
    DirectedYield16( (HTASK16)htask16 );
}


/***********************************************************************
 *           WOWHandle32              (WOW32.16) (KERNEL32.57)
 */
HANDLE WINAPI WOWHandle32( WORD handle, WOW_HANDLE_TYPE type )
{
    switch ( type )
    {
    case WOW_TYPE_HWND:
    case WOW_TYPE_HMENU:
    case WOW_TYPE_HDWP:
    case WOW_TYPE_HDROP:
    case WOW_TYPE_HDC:
    case WOW_TYPE_HFONT:
    case WOW_TYPE_HMETAFILE:
    case WOW_TYPE_HRGN:
    case WOW_TYPE_HBITMAP:
    case WOW_TYPE_HBRUSH:
    case WOW_TYPE_HPALETTE:
    case WOW_TYPE_HPEN:
    case WOW_TYPE_HACCEL:
    case WOW_TYPE_HTASK:
    case WOW_TYPE_FULLHWND:
        return (HANDLE)handle;

    default:
        ERR( "handle 0x%04x of unknown type %d\n", handle, type );
        return (HANDLE)handle;
    }
}

/***********************************************************************
 *           WOWHandle16              (WOW32.15) (KERNEL32.58)
 */
WORD WINAPI WOWHandle16( HANDLE handle, WOW_HANDLE_TYPE type )
{
    if ( HIWORD(handle ) )
        ERR( "handle 0x%08x of type %d has non-zero HIWORD\n", handle, type );

    switch ( type )
    {
    case WOW_TYPE_HWND:
    case WOW_TYPE_HMENU:
    case WOW_TYPE_HDWP:
    case WOW_TYPE_HDROP:
    case WOW_TYPE_HDC:
    case WOW_TYPE_HFONT:
    case WOW_TYPE_HMETAFILE:
    case WOW_TYPE_HRGN:
    case WOW_TYPE_HBITMAP:
    case WOW_TYPE_HBRUSH:
    case WOW_TYPE_HPALETTE:
    case WOW_TYPE_HPEN:
    case WOW_TYPE_HACCEL:
    case WOW_TYPE_HTASK:
    case WOW_TYPE_FULLHWND:
        return LOWORD(handle);

    default:
        ERR( "handle 0x%08x of unknown type %d\n", handle, type );
        return LOWORD(handle);
    }
}

/**********************************************************************
 *           WOWCallback16            (WOW32.2) (KERNEL32.54)
 */
DWORD WINAPI WOWCallback16( DWORD vpfn16, DWORD dwParam )
{
    DWORD ret;

    if ( !WOWCallback16Ex( vpfn16, WCB16_PASCAL, 
                           sizeof(DWORD), &dwParam, &ret ) )
        ret = 0L;

    return ret;
}

/**********************************************************************
 *           WOWCallback16Ex         (WOW32.3) (KERNEL32.55)
 */
BOOL WINAPI WOWCallback16Ex( DWORD vpfn16, DWORD dwFlags,
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
     * its stack or not.  Fortunately, our CallTo16 core doesn't rely on 
     * the callee to do so; after the routine has returned, the 16-bit 
     * stack pointer is always reset to the position it had before. 
     */

    ret = CallTo16Long( (FARPROC16)vpfn16, cbArgs );

    if ( pdwRetCode )
        *pdwRetCode = ret;

    return TRUE;  /* success */
}



/*
 *  16-bit WOW routines (in KERNEL)
 */

/**********************************************************************
 *           GetVDMPointer32W16      (KERNEL.516)
 */
DWORD WINAPI GetVDMPointer32W16( SEGPTR vp, UINT16 fMode )
{
    return (DWORD)WOWGetVDMPointer( vp, 0, (DWORD)fMode );
}

/***********************************************************************
 *           LoadLibraryEx32W16      (KERNEL.513)
 */
DWORD WINAPI LoadLibraryEx32W16( LPCSTR lpszLibFile, DWORD hFile, DWORD dwFlags )
{
    HMODULE hModule;

    SYSLEVEL_ReleaseWin16Lock();
    hModule = LoadLibraryExA( lpszLibFile, (HANDLE)hFile, dwFlags );
    SYSLEVEL_RestoreWin16Lock();

    return (DWORD)hModule;
}

/***********************************************************************
 *           GetProcAddress32W16     (KERNEL.515)
 */
DWORD WINAPI GetProcAddress32W16( DWORD hModule, LPCSTR lpszProc )
{
    return (DWORD)GetProcAddress( (HMODULE)hModule, lpszProc );
}

/***********************************************************************
 *           FreeLibrary32W16        (KERNEL.514)
 */
DWORD WINAPI FreeLibrary32W16( DWORD hLibModule )
{
    BOOL retv;

    SYSLEVEL_ReleaseWin16Lock();
    retv = FreeLibrary( (HMODULE)hLibModule );
    SYSLEVEL_RestoreWin16Lock();

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
    VA_LIST16 valist;
    int i;
    int aix;

    SYSLEVEL_ReleaseWin16Lock();

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
                if (args) args[aix] = (DWORD)PTR_SEG_TO_LIN(ptr);
                if (TRACE_ON(thunk)) DPRINTF("%08lx(%p),",ptr,PTR_SEG_TO_LIN(ptr));
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

    SYSLEVEL_RestoreWin16Lock();

    return ret;
}

/**********************************************************************
 *           CallProc32W16           (KERNEL.517)
 *
 * DWORD PASCAL CallProc32W( DWORD p1, ... , DWORD lpProcAddress,
 *                           DWORD fAddressConvert, DWORD cParams );
 */
DWORD WINAPI CallProc32W16( void )
{
    return WOW_CallProc32W16( FALSE );
}

/**********************************************************************
 *           CallProcEx32W16         (KERNEL.518)
 *
 * DWORD CallProcEx32W( DWORD cParams, DWORD fAddressConvert, 
 *                      DWORD lpProcAddress, DWORD p1, ... );
 */
DWORD WINAPI CallProcEx32W16( void )
{
    return WOW_CallProc32W16( TRUE );
}


/**********************************************************************
 *           WOW16Call               (KERNEL.501)
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


