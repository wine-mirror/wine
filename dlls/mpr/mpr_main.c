/*
 * MPR undocumented functions
 */

#include "winbase.h"
#include "winnetwk.h"
#include "heap.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(mpr)

 /* 
  * FIXME: The following routines should use a private heap ...
  */

/*****************************************************************
 *  MPR_Alloc  [MPR.22]
 */
LPVOID WINAPI MPR_Alloc( DWORD dwSize )
{
    return HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, dwSize );
}

/*****************************************************************
 *  MPR_ReAlloc  [MPR.23]
 */
LPVOID WINAPI MPR_ReAlloc( LPVOID lpSrc, DWORD dwSize )
{
    if ( lpSrc )
        return HeapReAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, lpSrc, dwSize );
    else
        return HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, dwSize );
}

/*****************************************************************
 *  MPR_Free  [MPR.24]
 */
BOOL WINAPI MPR_Free( LPVOID lpMem )
{
    if ( lpMem )
        return HeapFree( GetProcessHeap(), 0, lpMem );
    else
        return FALSE;
}

/*****************************************************************
 *  [MPR.25]
 */
BOOL WINAPI _MPR_25( LPBYTE lpMem, INT len )
{
    FIXME( "(%p, %d): stub\n", lpMem, len );

    return FALSE;
}

