/*
 * TTYDRV initialization code
 */

#include "config.h"

#include <stdio.h>

#include "winbase.h"
#include "clipboard.h"
#include "gdi.h"
#include "message.h"
#include "monitor.h"
#include "mouse.h"
#include "user.h"
#include "win.h"
#include "debugtools.h"
#include "ttydrv.h"

DEFAULT_DEBUG_CHANNEL(ttydrv);

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
    TTYDRV_MOUSE_EnableWarpPointer,
    /* screen saver functions */
    TTYDRV_GetScreenSaveActive,
    TTYDRV_SetScreenSaveActive,
    TTYDRV_GetScreenSaveTimeout,
    TTYDRV_SetScreenSaveTimeout,
    /* windowing functions */
    TTYDRV_IsSingleWindow
};

int cell_width = 8;
int cell_height = 8;
WINDOW *root_window;


/***********************************************************************
 *           TTYDRV process initialisation routine
 */
static void process_attach(void)
{
    int rows, cols;

    USER_Driver      = &user_driver;
    CLIPBOARD_Driver = &TTYDRV_CLIPBOARD_Driver;
    WND_Driver       = &TTYDRV_WND_Driver;

#ifdef WINE_CURSES
    if ((root_window = initscr()))
    {
        werase(root_window);
        wrefresh(root_window);
    }
    getmaxyx(root_window, rows, cols);
#else  /* WINE_CURSES */
    rows = 60; /* FIXME: Hardcoded */
    cols = 80; /* FIXME: Hardcoded */
#endif  /* WINE_CURSES */

    MONITOR_PrimaryMonitor.rect.left   = 0;
    MONITOR_PrimaryMonitor.rect.top    = 0;
    MONITOR_PrimaryMonitor.rect.right  = cell_width * cols;
    MONITOR_PrimaryMonitor.rect.bottom = cell_height * rows;
    MONITOR_PrimaryMonitor.depth       = 1;

    TTYDRV_GDI_Initialize();

    /* load display.dll */
    LoadLibrary16( "display" );
}


/***********************************************************************
 *           TTYDRV process termination routine
 */
static void process_detach(void)
{
    TTYDRV_GDI_Finalize();

#ifdef WINE_CURSES
    if (root_window) endwin();
#endif  /* WINE_CURSES */

    USER_Driver      = NULL;
    CLIPBOARD_Driver = NULL;
    WND_Driver       = NULL;
}


/***********************************************************************
 *           TTYDRV initialisation routine
 */
BOOL WINAPI TTYDRV_Init( HINSTANCE hinst, DWORD reason, LPVOID reserved )
{
    static int process_count;

    switch(reason)
    {
    case DLL_PROCESS_ATTACH:
        if (!process_count++) process_attach();
        break;

    case DLL_PROCESS_DETACH:
        if (!--process_count) process_detach();
        break;
    }
    return TRUE;
}


/***********************************************************************
 *              TTYDRV_GetScreenSaveActive
 *
 * Returns the active status of the screen saver
 */
BOOL TTYDRV_GetScreenSaveActive(void)
{
    return FALSE;
}

/***********************************************************************
 *              TTYDRV_SetScreenSaveActive
 *
 * Activate/Deactivate the screen saver
 */
void TTYDRV_SetScreenSaveActive(BOOL bActivate)
{
    FIXME("(%d): stub\n", bActivate);
}

/***********************************************************************
 *              TTYDRV_GetScreenSaveTimeout
 *
 * Return the screen saver timeout
 */
int TTYDRV_GetScreenSaveTimeout(void)
{
    return 0;
}

/***********************************************************************
 *              TTYDRV_SetScreenSaveTimeout
 *
 * Set the screen saver timeout
 */
void TTYDRV_SetScreenSaveTimeout(int nTimeout)
{
    FIXME("(%d): stub\n", nTimeout);
}

/***********************************************************************
 *              TTYDRV_IsSingleWindow
 */
BOOL TTYDRV_IsSingleWindow(void)
{
    return TRUE;
}
