/*
 * TTY clipboard driver
 *
 * Copyright 1998 Patrik Stridvall
 */

#include "config.h"

#include "heap.h"
#include "ttydrv.h"

/**********************************************************************/

char *TTYDRV_CLIPBOARD_szSelection = NULL;

/***********************************************************************
 *		TTYDRV_CLIPBOARD_EmptyClipboard
 */
void TTYDRV_CLIPBOARD_EmptyClipboard()
{
  if(TTYDRV_CLIPBOARD_szSelection)
    {
      HeapFree(SystemHeap, 0, TTYDRV_CLIPBOARD_szSelection);
      TTYDRV_CLIPBOARD_szSelection = NULL;
    }
}

/***********************************************************************
 *		TTYDRV_CLIPBOARD_SetClipboardData
 */
void TTYDRV_CLIPBOARD_SetClipboardData(UINT32 wFormat)
{
}

/***********************************************************************
 *		TTYDRV_CLIPBOARD_RequestSelection
 */
BOOL32 TTYDRV_CLIPBOARD_RequestSelection()
{
  return FALSE;
}

/***********************************************************************
 *		TTYDRV_CLIPBOARD_ResetOwner
 */
void TTYDRV_CLIPBOARD_ResetOwner(WND *pWnd, BOOL32 bFooBar)
{
}
