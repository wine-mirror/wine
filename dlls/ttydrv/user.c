/*
 * TTYDRV USER driver functions
 *
 * Copyright 1998 Patrik Stridvall
 */

#include "dinput.h"
#include "gdi.h"
#include "ttydrv.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(ttydrv);


/***********************************************************************
 *		TTYDRV_InitKeyboard (TTYDRV.@)
 */
void TTYDRV_InitKeyboard(void)
{
}

/***********************************************************************
 *		TTYDRV_VkKeyScan (TTYDRV.@)
 */
WORD TTYDRV_VkKeyScan(CHAR cChar)
{
  return 0;
}

/***********************************************************************
 *		TTYDRV_MapVirtualKey (TTYDRV.@)
 */
UINT16 TTYDRV_MapVirtualKey(UINT16 wCode, UINT16 wMapType)
{
  return 0;
}

/***********************************************************************
 *		TTYDRV_GetKeyNameText (TTYDRV.@)
 */
INT16 TTYDRV_GetKeyNameText( LONG lParam, LPSTR lpBuffer, INT16 nSize )
{  
  if(lpBuffer && nSize)
    {
      *lpBuffer = 0;
    }
  return 0;
}

/***********************************************************************
 *		TTYDRV_ToUnicode (TTYDRV.@)
 */
INT TTYDRV_ToUnicode( UINT virtKey, UINT scanCode, LPBYTE lpKeyState,
		      LPWSTR pwszBuff, int cchBuff, UINT flags )
{
  return 0;
}

/***********************************************************************
 *		TTYDRV_Beep (TTYDRV.@)
 */
void TTYDRV_Beep(void)
{
}

/***********************************************************************
 *		TTYDRV_GetDIState (TTYDRV.@)
 */
BOOL TTYDRV_GetDIState(DWORD len, LPVOID ptr)
{
  return TRUE;
}

/***********************************************************************
 *		TTYDRV_GetDIData (TTYDRV.@)
 */
BOOL TTYDRV_GetDIData( BYTE *keystate, DWORD dodsize, LPDIDEVICEOBJECTDATA dod,
                       LPDWORD entries, DWORD flags )
{
  return TRUE;
}

/***********************************************************************
 *           TTYDRV_InitMouse (TTYDRV.@)
 */
void TTYDRV_InitMouse(LPMOUSE_EVENT_PROC proc)
{
}

/***********************************************************************
 *		TTYDRV_SetCursor (TTYDRV.@)
 */
void TTYDRV_SetCursor( struct tagCURSORICONINFO *lpCursor )
{
}

/***********************************************************************
 *              TTYDRV_GetScreenSaveActive (TTYDRV.@)
 *
 * Returns the active status of the screen saver
 */
BOOL TTYDRV_GetScreenSaveActive(void)
{
    return FALSE;
}

/***********************************************************************
 *              TTYDRV_SetScreenSaveActive (TTYDRV.@)
 *
 * Activate/Deactivate the screen saver
 */
void TTYDRV_SetScreenSaveActive(BOOL bActivate)
{
    FIXME("(%d): stub\n", bActivate);
}

/***********************************************************************
 *              TTYDRV_GetScreenSaveTimeout (TTYDRV.@)
 *
 * Return the screen saver timeout
 */
int TTYDRV_GetScreenSaveTimeout(void)
{
    return 0;
}

/***********************************************************************
 *              TTYDRV_SetScreenSaveTimeout (TTYDRV.@)
 *
 * Set the screen saver timeout
 */
void TTYDRV_SetScreenSaveTimeout(int nTimeout)
{
    FIXME("(%d): stub\n", nTimeout);
}

/**********************************************************************
 *		TTYDRV_LoadOEMResource (TTYDRV.@)
 */
HANDLE TTYDRV_LoadOEMResource(WORD resid, WORD type)
{
  HBITMAP hbitmap;
  switch(type)
  {
    case OEM_BITMAP:
        hbitmap = CreateBitmap(1, 1, 1, 1, NULL);
        TTYDRV_DC_CreateBitmap(hbitmap);
        return hbitmap;
    case OEM_CURSOR:
    case OEM_ICON:
        break;
    default:
      ERR("unknown type (%d)\n", type);
  }
  return 0;
}

/***********************************************************************
 *		TTYDRV_AcquireClipboard (TTYDRV.@)
 */
void TTYDRV_AcquireClipboard(void)
{
}

/***********************************************************************
 *		TTYDRV_ReleaseClipboard (TTYDRV.@)
 */
void TTYDRV_ReleaseClipboard(void)
{
}

/***********************************************************************
 *		TTYDRV_SetClipboardData (TTYDRV.@)
 */
void TTYDRV_SetClipboardData(UINT wFormat)
{
}

/***********************************************************************
 *		TTYDRV_GetClipboardData (TTYDRV.@)
 */
BOOL TTYDRV_GetClipboardData(UINT wFormat)
{
  return FALSE;
}

/***********************************************************************
 *		TTYDRV_IsClipboardFormatAvailable (TTYDRV.@)
 */
BOOL TTYDRV_IsClipboardFormatAvailable(UINT wFormat)
{
  return FALSE;
}

/**************************************************************************
 *		TTYDRV_RegisterClipboardFormat (TTYDRV.@)
 *
 * Registers a custom clipboard format
 * Returns: TRUE - new format registered, FALSE - Format already registered
 */
BOOL TTYDRV_RegisterClipboardFormat( LPCSTR FormatName )
{
  return TRUE;
}

/**************************************************************************
 *		TTYDRV_IsSelectionOwner (TTYDRV.@)
 *
 * Returns: TRUE - We(WINE) own the selection, FALSE - Selection not owned by us
 */
BOOL TTYDRV_IsSelectionOwner(void)
{
    return FALSE;
}

/***********************************************************************
 *		TTYDRV_ResetSelectionOwner (TTYDRV.@)
 */
void TTYDRV_ResetSelectionOwner(struct tagWND *pWnd, BOOL bFooBar)
{
}
