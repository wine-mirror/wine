/*
 * TTY driver
 *
 * Copyright 1998 Patrik Stridvall
 */

#include "config.h"

#include "ttydrv.h"

#if 0
WND_DRIVER TTYDRV_WND_Driver =
{
  TTYDRV_WND_CreateDesktopWindow,
  TTYDRV_WND_CreateWindow,
  TTYDRV_WND_DestroyWindow,
  TTYDRV_WND_SetParent,
  TTYDRV_WND_ForceWindowRaise,
  TTYDRV_WND_SetWindowPos,
  TTYDRV_WND_SetText,
  TTYDRV_WND_SetFocus,
  TTYDRV_WND_PreSizeMove,
  TTYDRV_WND_PostSizeMove
};
#endif

CLIPBOARD_DRIVER TTYDRV_CLIPBOARD_Driver =
{
  TTYDRV_CLIPBOARD_EmptyClipboard,
  TTYDRV_CLIPBOARD_SetClipboardData,
  TTYDRV_CLIPBOARD_RequestSelection,
  TTYDRV_CLIPBOARD_ResetOwner
};

KEYBOARD_DRIVER TTYDRV_KEYBOARD_Driver =
{
  TTYDRV_KEYBOARD_Init,
  TTYDRV_KEYBOARD_VkKeyScan,
  TTYDRV_KEYBOARD_MapVirtualKey,
  TTYDRV_KEYBOARD_GetKeyNameText,
  TTYDRV_KEYBOARD_ToAscii
};

#if 0
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
#endif

#if 0
MOUSE_DRIVER TTYDRV_MOUSE_Driver =
{
};
#endif
