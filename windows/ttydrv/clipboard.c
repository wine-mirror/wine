/*
 * TTY clipboard driver
 *
 * Copyright 1998-1999 Patrik Stridvall
 */

#include "heap.h"
#include "ttydrv.h"
#include "win.h"

/**********************************************************************/

char *TTYDRV_CLIPBOARD_szSelection = NULL;

/***********************************************************************
 *		TTYDRV_CLIPBOARD_Acquire
 */
void TTYDRV_CLIPBOARD_Acquire()
{
}

/***********************************************************************
 *		TTYDRV_CLIPBOARD_Release
 */
void TTYDRV_CLIPBOARD_Release()
{
  if(TTYDRV_CLIPBOARD_szSelection)
    {
      HeapFree(SystemHeap, 0, TTYDRV_CLIPBOARD_szSelection);
      TTYDRV_CLIPBOARD_szSelection = NULL;
    }
}

/***********************************************************************
 *		TTYDRV_CLIPBOARD_SetData
 */
void TTYDRV_CLIPBOARD_SetData(UINT wFormat)
{
}

/***********************************************************************
 *		TTYDRV_CLIPBOARD_GetData
 */
BOOL TTYDRV_CLIPBOARD_GetData(UINT wFormat)
{
  return FALSE;
}

/***********************************************************************
 *		TTYDRV_CLIPBOARD_IsFormatAvailable
 */
BOOL TTYDRV_CLIPBOARD_IsFormatAvailable(UINT wFormat)
{
  return FALSE;
}

/**************************************************************************
 *		TTYDRV_CLIPBOARD_RegisterFormat
 *
 * Registers a custom clipboard format
 * Returns: TRUE - new format registered, FALSE - Format already registered
 */
BOOL TTYDRV_CLIPBOARD_RegisterFormat( LPCSTR FormatName )
{
  return TRUE;
}

/**************************************************************************
 *		X11DRV_CLIPBOARD_IsSelectionowner
 *
 * Returns: TRUE - We(WINE) own the selection, FALSE - Selection not owned by us
 */
BOOL TTYDRV_CLIPBOARD_IsSelectionowner()
{
    return FALSE;
}

/***********************************************************************
 *		TTYDRV_CLIPBOARD_ResetOwner
 */
void TTYDRV_CLIPBOARD_ResetOwner(WND *pWnd, BOOL bFooBar)
{
}
