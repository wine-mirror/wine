/*
static char RCSId[] = "$Id: user.c,v 1.2 1993/07/04 04:04:21 root Exp root $";
static char Copyright[] = "Copyright  Robert J. Amstadt, 1993";
*/
#include <stdio.h>
#include <stdlib.h>
#include "atom.h"
#include "gdi.h"
#include "dlls.h"
#include "sysmetrics.h"
#include "menu.h"
#include "dce.h"
#include "dialog.h"
#include "syscolor.h"
#include "win.h"
#include "windows.h"
#include "prototypes.h"
#include "user.h"
#include "message.h"
#include "toolhelp.h"

#define USER_HEAP_SIZE          0x10000

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


#ifndef WINELIB
/***********************************************************************
 *           USER_HeapInit
 */
static BOOL USER_HeapInit(void)
{
    if (!(USER_HeapSel = GlobalAlloc(GMEM_FIXED,USER_HEAP_SIZE))) return FALSE;
    USER_Heap = GlobalLock( USER_HeapSel );
    LocalInit( USER_HeapSel, 0, USER_HEAP_SIZE-1 );
    return TRUE;
}
#endif

/**********************************************************************
 *					USER_InitApp
 *
 * Load necessary resources?
 */
int
USER_InitApp(int hInstance)
{
    int queueSize;

    SpyInit();

#ifndef WINELIB    
      /* Create USER heap */
    if (!USER_HeapInit()) return 0;
#endif
    
      /* Global atom table initialisation */
    if (!ATOM_Init()) return 0;
    
      /* GDI initialisation */
    if (!GDI_Init()) return 0;

      /* Initialize system colors and metrics*/
    SYSMETRICS_Init();
    SYSCOLOR_Init();

      /* Create the DCEs */
    DCE_Init();
    
      /* Initialize built-in window classes */
    if (!WIDGETS_Init()) return 0;

      /* Initialize dialog manager */
    if (!DIALOG_Init()) return 0;

      /* Initialize menus */
    if (!MENU_Init()) return 0;

      /* Create system message queue */
    queueSize = GetProfileInt( "windows", "TypeAhead", 120 );
    if (!MSG_CreateSysMsgQueue( queueSize )) return 0;

      /* Create task message queue */
    queueSize = GetProfileInt( "windows", "DefaultQueueSize", 8 );
    if (!SetMessageQueue( queueSize )) return 0;

      /* Create desktop window */
    if (!WIN_CreateDesktopWindow()) return 0;

#ifndef WINELIB
    /* Initialize DLLs */
    InitializeLoadedDLLs(NULL);
#endif
        
    return 1;
}
