/*
 * GDI initialization code
 */

#include "windef.h"
#include "wingdi.h"
#include "wine/winbase16.h"

#include "gdi.h"
#include "win16drv.h"
#include "winbase.h"

/***********************************************************************
 *           GDI initialisation routine
 */
BOOL WINAPI MAIN_GdiInit(HINSTANCE hinstDLL, DWORD reason, LPVOID lpvReserved)
{
    if (reason != DLL_PROCESS_ATTACH) return TRUE;

    /* GDI initialisation */
    if(!GDI_Init()) return FALSE;

    /* Create the Win16 printer driver */
    if (!WIN16DRV_Init()) return FALSE;

    /* PSDRV initialization */
    if (!LoadLibraryA( "wineps" )) return FALSE;

    return TRUE;
}


/***********************************************************************
 *           Copy   (GDI.250)
 */
void WINAPI Copy16( LPVOID src, LPVOID dst, WORD size )
{
    memcpy( dst, src, size );
}
