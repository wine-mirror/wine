/*
 * TTY driver
 *
 * Copyright 1998-1999 Patrik Stridvall
 */

#include "clipboard.h"
#include "desktop.h"
#include "display.h"
#include "keyboard.h"
#include "message.h"
#include "monitor.h"
#include "ttydrv.h"

CLIPBOARD_DRIVER TTYDRV_CLIPBOARD_Driver =
{
  TTYDRV_CLIPBOARD_EmptyClipboard,
  TTYDRV_CLIPBOARD_SetClipboardData,
  TTYDRV_CLIPBOARD_RequestSelection,
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
  TTYDRV_EVENT_AddIO,
  TTYDRV_EVENT_DeleteIO,
  TTYDRV_EVENT_WaitNetEvent,
  TTYDRV_EVENT_Synchronize,
  TTYDRV_EVENT_CheckFocus,
  TTYDRV_EVENT_QueryPointer,
  TTYDRV_EVENT_DummyMotionNotify,
  TTYDRV_EVENT_Pending,
  TTYDRV_EVENT_IsUserIdle
};

KEYBOARD_DRIVER TTYDRV_KEYBOARD_Driver =
{
  TTYDRV_KEYBOARD_Init,
  TTYDRV_KEYBOARD_VkKeyScan,
  TTYDRV_KEYBOARD_MapVirtualKey,
  TTYDRV_KEYBOARD_GetKeyNameText,
  TTYDRV_KEYBOARD_ToAscii
};

MONITOR_DRIVER TTYDRV_MONITOR_Driver =
{
  TTYDRV_MONITOR_Initialize,
  TTYDRV_MONITOR_Finalize,
  TTYDRV_MONITOR_GetWidth,
  TTYDRV_MONITOR_GetHeight,
  TTYDRV_MONITOR_GetDepth
};

MOUSE_DRIVER TTYDRV_MOUSE_Driver =
{
  TTYDRV_MOUSE_SetCursor,
  TTYDRV_MOUSE_MoveCursor
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
  TTYDRV_WND_IsSelfClipping
};


