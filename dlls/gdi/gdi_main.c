/*
 * GDI initialization code
 */

#include <string.h>
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
    return GDI_Init();
}


/***********************************************************************
 *           Copy   (GDI.250)
 */
void WINAPI Copy16( LPVOID src, LPVOID dst, WORD size )
{
    memcpy( dst, src, size );
}
