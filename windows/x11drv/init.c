/*
 * X11 driver
 *
 * Copyright 1998 Patrik Stridvall
 */

#include "config.h"

#ifndef X_DISPLAY_MISSING

#include "clipboard.h"
#include "monitor.h"
#include "user.h"
#include "win.h"
#include "x11drv.h"

CLIPBOARD_DRIVER X11DRV_CLIPBOARD_Driver =
{
  X11DRV_CLIPBOARD_Acquire,
  X11DRV_CLIPBOARD_Release,
  X11DRV_CLIPBOARD_SetData,
  X11DRV_CLIPBOARD_GetData,
  X11DRV_CLIPBOARD_IsFormatAvailable,
  X11DRV_CLIPBOARD_RegisterFormat,
  X11DRV_CLIPBOARD_IsSelectionowner,
  X11DRV_CLIPBOARD_ResetOwner
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


