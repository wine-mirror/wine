/*
static char RCSId[] = "$Id: user.c,v 1.2 1993/07/04 04:04:21 root Exp root $";
static char Copyright[] = "Copyright  Robert J. Amstadt, 1993";
*/
#include <stdio.h>
#include <stdlib.h>
#include "atom.h"
#include "comm.h"
#include "gdi.h"
#include "desktop.h"
#include "dlls.h"
#include "dos_fs.h"
#include "sysmetrics.h"
#include "menu.h"
#include "dce.h"
#include "dialog.h"
#include "syscolor.h"
#include "win.h"
#include "windows.h"
#include "user.h"
#include "message.h"
#include "toolhelp.h"

#define USER_HEAP_SIZE          0x10000

#ifndef WINELIB
LPSTR USER_Heap = NULL;
WORD USER_HeapSel = 0;


/***********************************************************************
 *           GetFreeSystemResources   (USER.284)
 */
WORD GetFreeSystemResources( WORD resType )
{
    DWORD user, gdi;

    switch(resType)
    {
    case GFSR_USERRESOURCES:
        user = GetHeapSpaces( USER_HeapSel );
        gdi  = 0xffffffff;
        break;

    case GFSR_GDIRESOURCES:
        gdi  = GetHeapSpaces( GDI_HeapSel );
        user = 0xffffffff;
        break;

    case GFSR_SYSTEMRESOURCES:
        user = GetHeapSpaces( USER_HeapSel );
        gdi  = GetHeapSpaces( GDI_HeapSel );
        break;

    default:
        return 0;
    }
    if (user > gdi) return LOWORD(gdi) * 100 / HIWORD(gdi);
    else return LOWORD(user) * 100 / HIWORD(user);
}


/***********************************************************************
 *           SystemHeapInfo   (TOOLHELP.71)
 */
BOOL SystemHeapInfo( SYSHEAPINFO *pHeapInfo )
{
    pHeapInfo->wUserFreePercent = GetFreeSystemResources( GFSR_USERRESOURCES );
    pHeapInfo->wGDIFreePercent  = GetFreeSystemResources( GFSR_GDIRESOURCES );
    pHeapInfo->hUserSegment = USER_HeapSel;
    pHeapInfo->hGDISegment  = GDI_HeapSel;
    return TRUE;
}


/***********************************************************************
 *           USER_HeapInit
 */
BOOL USER_HeapInit(void)
{
    if (!(USER_HeapSel = GlobalAlloc(GMEM_FIXED,USER_HEAP_SIZE))) return FALSE;
    USER_Heap = GlobalLock( USER_HeapSel );
    LocalInit( USER_HeapSel, 0, USER_HEAP_SIZE-1 );
    return TRUE;
}
#endif


/**********************************************************************
 *					USER_InitApp
 */
int USER_InitApp(HINSTANCE hInstance)
{
    int queueSize;

      /* Create task message queue */
    queueSize = GetProfileInt( "windows", "DefaultQueueSize", 8 );
    if (!SetMessageQueue( queueSize )) return 0;

    return 1;
}
