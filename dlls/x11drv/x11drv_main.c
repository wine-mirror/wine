/*
 * X11DRV initialization code
 */
#include <stdio.h>

#include "winbase.h"
#include "gdi.h"
#include "user.h"
#include "x11drv.h"

/***********************************************************************
 *           X11DRV initialisation routine
 */
BOOL WINAPI X11DRV_Init( HINSTANCE hinst, DWORD reason, LPVOID reserved )
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        GDI_Driver = &X11DRV_GDI_Driver;
        USER_Driver = &X11DRV_USER_Driver;
    }
    return TRUE;
}
