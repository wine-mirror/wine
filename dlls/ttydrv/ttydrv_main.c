/*
 * TTYDRV initialization code
 */
#include <stdio.h>

#include "winbase.h"
#include "clipboard.h"
#include "gdi.h"
#include "message.h"
#include "monitor.h"
#include "mouse.h"
#include "ttydrv.h"
#include "user.h"
#include "win.h"

static USER_DRIVER user_driver =
{
    /* event functions */
    TTYDRV_EVENT_Synchronize,
    TTYDRV_EVENT_CheckFocus,
    TTYDRV_EVENT_UserRepaintDisable,
    /* keyboard functions */
    TTYDRV_KEYBOARD_Init,
    TTYDRV_KEYBOARD_VkKeyScan,
    TTYDRV_KEYBOARD_MapVirtualKey,
    TTYDRV_KEYBOARD_GetKeyNameText,
    TTYDRV_KEYBOARD_ToAscii,
    TTYDRV_KEYBOARD_GetBeepActive,
    TTYDRV_KEYBOARD_SetBeepActive,
    TTYDRV_KEYBOARD_Beep,
    TTYDRV_KEYBOARD_GetDIState,
    TTYDRV_KEYBOARD_GetDIData,
    TTYDRV_KEYBOARD_GetKeyboardConfig,
    TTYDRV_KEYBOARD_SetKeyboardConfig,
    /* mouse functions */
    TTYDRV_MOUSE_Init,
    TTYDRV_MOUSE_SetCursor,
    TTYDRV_MOUSE_MoveCursor,
    TTYDRV_MOUSE_EnableWarpPointer
};


/***********************************************************************
 *           TTYDRV initialisation routine
 */
BOOL WINAPI TTYDRV_Init( HINSTANCE hinst, DWORD reason, LPVOID reserved )
{
    switch(reason)
    {
    case DLL_PROCESS_ATTACH:
        USER_Driver      = &user_driver;
        CLIPBOARD_Driver = &TTYDRV_CLIPBOARD_Driver;
        MONITOR_Driver   = &TTYDRV_MONITOR_Driver;
        WND_Driver       = &TTYDRV_WND_Driver;

        TTYDRV_MONITOR_Initialize( &MONITOR_PrimaryMonitor );
        TTYDRV_GDI_Initialize();
        break;

    case DLL_PROCESS_DETACH:
        TTYDRV_GDI_Finalize();
        TTYDRV_MONITOR_Finalize( &MONITOR_PrimaryMonitor );

        USER_Driver      = NULL;
        CLIPBOARD_Driver = NULL;
        MONITOR_Driver   = NULL;
        WND_Driver       = NULL;
        break;
    }
    return TRUE;
}
