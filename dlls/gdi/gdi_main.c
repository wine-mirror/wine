/*
 * GDI initialization code
 */

#include "windef.h"
#include "wingdi.h"
#include "wine/winbase16.h"

#include "gdi.h"
#include "global.h"
#include "module.h"
#include "psdrv.h"
#include "tweak.h"
#include "win16drv.h"


/***********************************************************************
 *           GDI initialisation routine
 */
BOOL WINAPI MAIN_GdiInit(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    NE_MODULE *pModule;

    if ( GDI_HeapSel ) return TRUE;

    /* Create GDI heap */
    pModule = NE_GetPtr( GetModuleHandle16( "GDI" ) );
    if ( pModule )
    {
        GDI_HeapSel = GlobalHandleToSel16( (NE_SEG_TABLE( pModule ) + 
                                          pModule->dgroup - 1)->hSeg );
    }
    else
    {
        GDI_HeapSel = GlobalAlloc16( GMEM_FIXED, GDI_HEAP_SIZE );
        LocalInit16( GDI_HeapSel, 0, GDI_HEAP_SIZE-1 );
    }

    if (!TWEAK_Init()) return FALSE;

    /* GDI initialisation */
    if(!GDI_Init()) return FALSE;

    /* Create the Win16 printer driver */
    if (!WIN16DRV_Init()) return FALSE;

    /* PSDRV initialization */
    if(!PSDRV_Init()) return FALSE;

    return TRUE;
}
