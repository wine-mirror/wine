/*
 * TTY driver
 *
 * Copyright 1998-1999 Patrik Stridvall
 */

#include "clipboard.h"
#include "desktop.h"
#include "keyboard.h"
#include "message.h"
#include "monitor.h"
#include "mouse.h"
#include "user.h"
#include "win.h"
#include "ttydrv.h"

USER_DRIVER TTYDRV_USER_Driver =
{
  TTYDRV_USER_Initialize,
  TTYDRV_USER_Finalize,
  TTYDRV_USER_BeginDebugging,
  TTYDRV_USER_EndDebugging
};

CLIPBOARD_DRIVER TTYDRV_CLIPBOARD_Driver =
{
  TTYDRV_CLIPBOARD_Empty,
  TTYDRV_CLIPBOARD_SetData,
  TTYDRV_CLIPBOARD_GetData,
  TTYDRV_CLIPBOARD_ResetOwner
};

DESKTOP_DRIVER TTYDRV_DESKTOP_Driver =
{
  TTYDRV_DESKTOP_Initialize,
  TTYDRV_DESKTOP_Finalize
};

EVENT_DRIVER TTYDRV_EVENT_Driver = 
{
  TTYDRV_EVENT_Init,
  TTYDRV_EVENT_Synchronize,
  TTYDRV_EVENT_CheckFocus,
  TTYDRV_EVENT_QueryPointer,
  TTYDRV_EVENT_UserRepaintDisable
};

KEYBOARD_DRIVER TTYDRV_KEYBOARD_Driver =
{
  TTYDRV_KEYBOARD_Init,
  TTYDRV_KEYBOARD_VkKeyScan,
  TTYDRV_KEYBOARD_MapVirtualKey,
  TTYDRV_KEYBOARD_GetKeyNameText,
  TTYDRV_KEYBOARD_ToAscii,
  TTYDRV_KEYBOARD_GetBeepActive,
  TTYDRV_KEYBOARD_SetBeepActive,
  TTYDRV_KEYBOARD_Beep
};

MONITOR_DRIVER TTYDRV_MONITOR_Driver =
{
  TTYDRV_MONITOR_Initialize,
  TTYDRV_MONITOR_Finalize,
  TTYDRV_MONITOR_IsSingleWindow,
  TTYDRV_MONITOR_GetWidth,
  TTYDRV_MONITOR_GetHeight,
  TTYDRV_MONITOR_GetDepth,
  TTYDRV_MONITOR_GetScreenSaveActive,
  TTYDRV_MONITOR_SetScreenSaveActive,
  TTYDRV_MONITOR_GetScreenSaveTimeout,
  TTYDRV_MONITOR_SetScreenSaveTimeout
};

MOUSE_DRIVER TTYDRV_MOUSE_Driver =
{
  TTYDRV_MOUSE_SetCursor,
  TTYDRV_MOUSE_MoveCursor,
  TTYDRV_MOUSE_EnableWarpPointer
};

WND_DRIVER TTYDRV_WND_Driver =
{
  TTYDRV_WND_Initialize,
  TTYDRV_WND_Finalize,
  TTYDRV_WND_CreateDesktopWindow,
  TTYDRV_WND_CreateWindow,
  TTYDRV_WND_DestroyWindow,
  TTYDRV_WND_SetParent,
  TTYDRV_WND_ForceWindowRaise,
  TTYDRV_WND_SetWindowPos,
  TTYDRV_WND_SetText,
  TTYDRV_WND_SetFocus,
  TTYDRV_WND_PreSizeMove,
  TTYDRV_WND_PostSizeMove,
  TTYDRV_WND_ScrollWindow,
  TTYDRV_WND_SetDrawable,
  TTYDRV_WND_SetHostAttr,
  TTYDRV_WND_IsSelfClipping
};


