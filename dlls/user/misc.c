/*
 * Misc USER functions
 *
 * Copyright 1995 Thomas Sandford
 * Copyright 1997 Marcus Meissner
 */

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winerror.h"

#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(win);

/**********************************************************************
 * SetLastErrorEx [USER32.485]  Sets the last-error code.
 *
 * RETURNS
 *    None.
 */
void WINAPI SetLastErrorEx(
    DWORD error, /* [in] Per-thread error code */
    DWORD type)  /* [in] Error type */
{
    TRACE("(0x%08lx, 0x%08lx)\n", error,type);
    switch(type) {
        case 0:
            break;
        case SLE_ERROR:
        case SLE_MINORERROR:
        case SLE_WARNING:
            /* Fall through for now */
        default:
            FIXME("(error=%08lx, type=%08lx): Unhandled type\n", error,type);
            break;
    }
    SetLastError( error );
}


/******************************************************************************
 * GetProcessWindowStation [USER32.280]  Returns handle of window station
 *
 * NOTES
 *    Docs say the return value is HWINSTA
 *
 * RETURNS
 *    Success: Handle to window station associated with calling process
 *    Failure: NULL
 */
DWORD WINAPI GetProcessWindowStation(void)
{
    FIXME("(void): stub\n");
    return 1;
}


/******************************************************************************
 * GetThreadDesktop [USER32.295]  Returns handle to desktop
 *
 * NOTES
 *    Docs say the return value is HDESK
 *
 * PARAMS
 *    dwThreadId [I] Thread identifier
 *
 * RETURNS
 *    Success: Handle to desktop associated with specified thread
 *    Failure: NULL
 */
DWORD WINAPI GetThreadDesktop( DWORD dwThreadId )
{
    FIXME("(%lx): stub\n",dwThreadId);
    return 1;
}


/******************************************************************************
 * SetDebugErrorLevel [USER32.475]
 * Sets the minimum error level for generating debugging events
 *
 * PARAMS
 *    dwLevel [I] Debugging error level
 */
VOID WINAPI SetDebugErrorLevel( DWORD dwLevel )
{
    FIXME("(%ld): stub\n", dwLevel);
}


/******************************************************************************
 *                    GetProcessDefaultLayout [USER32.802]
 *
 * Gets the default layout for parentless windows.
 * Right now, just returns 0 (left-to-right).
 *
 * RETURNS
 *    Success: Nonzero
 *    Failure: Zero
 *
 * BUGS
 *    No RTL
 */
BOOL WINAPI GetProcessDefaultLayout( DWORD *pdwDefaultLayout )
{
    if ( !pdwDefaultLayout ) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
     }
    FIXME( "( %p ): No BiDi\n", pdwDefaultLayout );
    *pdwDefaultLayout = 0;
    return TRUE;
}


/******************************************************************************
 *                    SetProcessDefaultLayout [USER32.803]
 *
 * Sets the default layout for parentless windows.
 * Right now, only accepts 0 (left-to-right).
 *
 * RETURNS
 *    Success: Nonzero
 *    Failure: Zero
 *
 * BUGS
 *    No RTL
 */
BOOL WINAPI SetProcessDefaultLayout( DWORD dwDefaultLayout )
{
    if ( dwDefaultLayout == 0 )
        return TRUE;
    FIXME( "( %08lx ): No BiDi\n", dwDefaultLayout );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}


/******************************************************************************
 * OpenDesktopA [USER32.408]
 *
 * NOTES
 *    Return type should be HDESK
 *
 *    Not supported on Win9x - returns NULL and calls SetLastError.
 */
HANDLE WINAPI OpenDesktopA( LPCSTR lpszDesktop, DWORD dwFlags, 
                                BOOL fInherit, DWORD dwDesiredAccess )
{
    FIXME("(%s,%lx,%i,%lx): stub\n",debugstr_a(lpszDesktop),dwFlags,
          fInherit,dwDesiredAccess);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


/******************************************************************************
 *		SetUserObjectInformationA   (USER32.512)
 */
BOOL WINAPI SetUserObjectInformationA( HANDLE hObj, INT nIndex, 
				       LPVOID pvInfo, DWORD nLength )
{
    FIXME("(%x,%d,%p,%lx): stub\n",hObj,nIndex,pvInfo,nLength);
    return TRUE;
}

/******************************************************************************
 *		SetThreadDesktop   (USER32.510)
 */
BOOL WINAPI SetThreadDesktop( HANDLE hDesktop )
{
    FIXME("(%x): stub\n",hDesktop);
    return TRUE;
}


/***********************************************************************
 *           RegisterShellHookWindow			[USER32.459]
 */
HRESULT WINAPI RegisterShellHookWindow ( DWORD u )
{
    FIXME("0x%08lx stub\n",u);
    return 0;
}


/***********************************************************************
 *           DeregisterShellHookWindow			[USER32.132]
 */
HRESULT WINAPI DeregisterShellHookWindow ( DWORD u )
{
    FIXME("0x%08lx stub\n",u);
    return 0;
	
}


/***********************************************************************
 *           RegisterTaskList   			[USER23.436]
 */
DWORD WINAPI RegisterTaskList (DWORD x)
{
    FIXME("0x%08lx\n",x);
    return TRUE;
}
