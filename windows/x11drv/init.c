/*
 * X11 driver
 *
 * Copyright 1998 Patrik Stridvall
 */

#include "config.h"

#ifndef X_DISPLAY_MISSING

#include "clipboard.h"
#include "desktop.h"
#include "keyboard.h"
#include "message.h"
#include "monitor.h"
#include "mouse.h"
#include "user.h"
#include "win.h"
#include "x11drv.h"

USER_DRIVER X11DRV_USER_Driver =
{
  X11DRV_USER_Initialize,
  X11DRV_USER_Finalize,
  X11DRV_USER_BeginDebugging,
  X11DRV_USER_EndDebugging
};

CLIPBOARD_DRIVER X11DRV_CLIPBOARD_Driver =
{
  X11DRV_CLIPBOARD_Empty,
  X11DRV_CLIPBOARD_SetData,
  X11DRV_CLIPBOARD_GetData,
  X11DRV_CLIPBOARD_ResetOwner
};

DESKTOP_DRIVER X11DRV_DESKTOP_Driver =
{
  X11DRV_DESKTOP_Initialize,
  X11DRV_DESKTOP_Finalize
};

EVENT_DRIVER X11DRV_EVENT_Driver = 
{
  X11DRV_EVENT_Init,
  X11DRV_EVENT_Synchronize,
  X11DRV_EVENT_CheckFocus,
  X11DRV_EVENT_QueryPointer,
  X11DRV_EVENT_UserRepaintDisable
};

KEYBOARD_DRIVER X11DRV_KEYBOARD_Driver =
{
  X11DRV_KEYBOARD_Init,
  X11DRV_KEYBOARD_VkKeyScan,
  X11DRV_KEYBOARD_MapVirtualKey,
  X11DRV_KEYBOARD_GetKeyNameText,
  X11DRV_KEYBOARD_ToAscii,
  X11DRV_KEYBOARD_GetBeepActive,
  X11DRV_KEYBOARD_SetBeepActive,
  X11DRV_KEYBOARD_Beep
};

MONITOR_DRIVER X11DRV_MONITOR_Driver =
{
  X11DRV_MONITOR_Initialize,
  X11DRV_MONITOR_Finalize,
  X11DRV_MONITOR_IsSingleWindow,
  X11DRV_MONITOR_GetWidth,
  X11DRV_MONITOR_GetHeight,
  X11DRV_MONITOR_GetDepth,
  X11DRV_MONITOR_GetScreenSaveActive,
  X11DRV_MONITOR_SetScreenSaveActive,
  X11DRV_MONITOR_GetScreenSaveTimeout,
  X11DRV_MONITOR_SetScreenSaveTimeout
};

MOUSE_DRIVER X11DRV_MOUSE_Driver =
{
  X11DRV_MOUSE_SetCursor,
  X11DRV_MOUSE_MoveCursor,
  X11DRV_MOUSE_EnableWarpPointer
};

WND_DRIVER X11DRV_WND_Driver =
{
  X11DRV_WND_Initialize,
  X11DRV_WND_Finalize,
  X11DRV_WND_CreateDesktopWindow,
  X11DRV_WND_CreateWindow,
  X11DRV_WND_DestroyWindow,
  X11DRV_WND_SetParent,
  X11DRV_WND_ForceWindowRaise,
  X11DRV_WND_SetWindowPos,
  X11DRV_WND_SetText,
  X11DRV_WND_SetFocus,
  X11DRV_WND_PreSizeMove,
  X11DRV_WND_PostSizeMove,
  X11DRV_WND_SurfaceCopy,
  X11DRV_WND_SetDrawable,
  X11DRV_WND_SetHostAttr,
  X11DRV_WND_IsSelfClipping
};

#endif /* !defined(X_DISPLAY_MISSING) */


