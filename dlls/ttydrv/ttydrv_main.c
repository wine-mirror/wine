/*
 * TTYDRV initialization code
 */
#include <stdio.h>

#include "winbase.h"
#include "gdi.h"
#include "user.h"
#include "ttydrv.h"

/***********************************************************************
 *           TTYDRV initialisation routine
 */
BOOL WINAPI TTYDRV_Init( HINSTANCE hinst, DWORD reason, LPVOID reserved )
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        GDI_Driver = &TTYDRV_GDI_Driver;
        USER_Driver = &TTYDRV_USER_Driver;
    }
    return TRUE;
}
