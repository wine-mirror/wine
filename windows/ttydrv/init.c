/*
 * TTY driver
 *
 * Copyright 1998-1999 Patrik Stridvall
 */

#include "clipboard.h"
#include "monitor.h"
#include "user.h"
#include "win.h"
#include "ttydrv.h"

CLIPBOARD_DRIVER TTYDRV_CLIPBOARD_Driver =
{
  TTYDRV_CLIPBOARD_Acquire,
  TTYDRV_CLIPBOARD_Release,
  TTYDRV_CLIPBOARD_SetData,
  TTYDRV_CLIPBOARD_GetData,
  TTYDRV_CLIPBOARD_IsFormatAvailable,
  TTYDRV_CLIPBOARD_RegisterFormat,
  TTYDRV_CLIPBOARD_IsSelectionowner,
  TTYDRV_CLIPBOARD_ResetOwner
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


