/*
 * TTY window driver
 *
 * Copyright 1998,1999 Patrik Stridvall
 */

#include "class.h"
#include "dc.h"
#include "ttydrv.h"
#include "win.h"

/**********************************************************************
 *		TTYDRV_WND_Initialize
 */
void TTYDRV_WND_Initialize(WND *wndPtr)
{
}

/**********************************************************************
 *		TTYDRV_WND_Finalize
 */
void TTYDRV_WND_Finalize(WND *wndPtr)
{
}

/**********************************************************************
 *		TTYDRV_WND_CreateDesktopWindow
 */
BOOL32 TTYDRV_WND_CreateDesktopWindow(WND *wndPtr, CLASS *classPtr, BOOL32 bUnicode)
{
  return FALSE;
}

/**********************************************************************
 *		TTYDRV_WND_CreateWindow
 */
BOOL32 TTYDRV_WND_CreateWindow(WND *wndPtr, CLASS *classPtr, CREATESTRUCT32A *cs, BOOL32 bUnicode)
{
  return FALSE;
}

/***********************************************************************
 *		TTYDRV_WND_DestroyWindow
 */
BOOL32 TTYDRV_WND_DestroyWindow(WND *wndPtr)
{
  return FALSE;
}

/*****************************************************************
 *		X11DRV_WND_SetParent
 */
WND *TTYDRV_WND_SetParent(WND *wndPtr, WND *pWndParent)
{
  return NULL;
}

/***********************************************************************
 *		TTYDRV_WND_ForceWindowRaise
 */
void TTYDRV_WND_ForceWindowRaise(WND *wndPtr)
{
}

/***********************************************************************
 *           WINPOS_SetXWindowPos
 *
 * SetWindowPos() for an X window. Used by the real SetWindowPos().
 */
void TTYDRV_WND_SetWindowPos(WND *wndPtr, const WINDOWPOS32 *winpos, BOOL32 bSMC_SETXPOS)
{
}

/*****************************************************************
 *		TTYDRV_WND_SetText
 */
void TTYDRV_WND_SetText(WND *wndPtr, LPCSTR text)
{   
}

/*****************************************************************
 *		TTYDRV_WND_SetFocus
 */
void TTYDRV_WND_SetFocus(WND *wndPtr)
{
}

/*****************************************************************
 *		TTYDRV_WND_PreSizeMove
 */
void TTYDRV_WND_PreSizeMove(WND *wndPtr)
{
}

/*****************************************************************
 *		 TTYDRV_WND_PostSizeMove
 */
void TTYDRV_WND_PostSizeMove(WND *wndPtr)
{
}


/*****************************************************************
 *		 TTYDRV_WND_ScrollWindow
 */
void TTYDRV_WND_ScrollWindow(
  WND *wndPtr, DC *dcPtr, INT32 dx, INT32 dy, 
  const RECT32 *clipRect, BOOL32 bUpdate)
{
}

/***********************************************************************
 *		TTYDRV_WND_SetDrawable
 */
void TTYDRV_WND_SetDrawable(WND *wndPtr, DC *dc, WORD flags, BOOL32 bSetClipOrigin)
{
}

/***********************************************************************
 *		TTYDRV_WND_IsSelfClipping
 */
BOOL32 TTYDRV_WND_IsSelfClipping(WND *wndPtr)
{
  return FALSE;
}

