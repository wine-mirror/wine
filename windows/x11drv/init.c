/*
 * X11 driver
 *
 * Copyright 1998 Patrik Stridvall
 */

#include "config.h"

#ifndef X_DISPLAY_MISSING

#include "clipboard.h"
#include "desktop.h"
#include "display.h"
#include "keyboard.h"
#include "message.h"
#include "monitor.h"
#include "win.h"
#include "x11drv.h"

CLIPBOARD_DRIVER X11DRV_CLIPBOARD_Driver =
{
  X11DRV_CLIPBOARD_EmptyClipboard,
  X11DRV_CLIPBOARD_SetClipboardData,
  X11DRV_CLIPBOARD_RequestSelection,
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
  X11DRV_EVENT_AddIO,
  X11DRV_EVENT_DeleteIO,
  X11DRV_EVENT_WaitNetEvent,
  X11DRV_EVENT_Synchronize,
  X11DRV_EVENT_CheckFocus,
  X11DRV_EVENT_QueryPointer,
  X11DRV_EVENT_DummyMotionNotify,
  X11DRV_EVENT_Pending,
  X11DRV_EVENT_IsUserIdle,
  X11DRV_EVENT_WakeUp
};

KEYBOARD_DRIVER X11DRV_KEYBOARD_Driver =
{
  X11DRV_KEYBOARD_Init,
  X11DRV_KEYBOARD_VkKeyScan,
  X11DRV_KEYBOARD_MapVirtualKey,
  X11DRV_KEYBOARD_GetKeyNameText,
  X11DRV_KEYBOARD_ToAscii
};

MONITOR_DRIVER X11DRV_MONITOR_Driver =
{
  X11DRV_MONITOR_Initialize,
  X11DRV_MONITOR_Finalize,
  X11DRV_MONITOR_GetWidth,
  X11DRV_MONITOR_GetHeight,
  X11DRV_MONITOR_GetDepth
};

MOUSE_DRIVER X11DRV_MOUSE_Driver =
{
  X11DRV_MOUSE_SetCursor,
  X11DRV_MOUSE_MoveCursor
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
  X11DRV_WND_ScrollWindow,
  X11DRV_WND_SetDrawable,
  X11DRV_WND_IsSelfClipping
};

#endif /* !defined(X_DISPLAY_MISSING) */


