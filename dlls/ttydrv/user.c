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
 *		TTYDRV_Synchronize
 */
void TTYDRV_Synchronize( void )
{
}

/***********************************************************************
 *		TTYDRV_CheckFocus
 */
BOOL TTYDRV_CheckFocus(void)
{
  return TRUE;
}

/***********************************************************************
 *		TTYDRV_UserRepaintDisable
 */
void TTYDRV_UserRepaintDisable( BOOL bDisable )
{
}


/***********************************************************************
 *		TTYDRV_InitKeyboard
 */
void TTYDRV_InitKeyboard(void)
{
}

/***********************************************************************
 *		TTYDRV_VkKeyScan
 */
WORD TTYDRV_VkKeyScan(CHAR cChar)
{
  return 0;
}

/***********************************************************************
 *		TTYDRV_MapVirtualKey
 */
UINT16 TTYDRV_MapVirtualKey(UINT16 wCode, UINT16 wMapType)
{
  return 0;
}

/***********************************************************************
 *		TTYDRV_GetKeyNameText
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
 *		TTYDRV_ToUnicode
 */
INT TTYDRV_ToUnicode( UINT virtKey, UINT scanCode, LPBYTE lpKeyState,
		      LPWSTR pwszBuff, int cchBuff, UINT flags )
{
  return 0;
}

/***********************************************************************
 *		TTYDRV_GetBeepActive
 */
BOOL TTYDRV_GetBeepActive(void)
{
  return FALSE;
}

/***********************************************************************
 *		TTYDRV_SetBeepActive
 */
void TTYDRV_SetBeepActive(BOOL bActivate)
{
}

/***********************************************************************
 *		TTYDRV_Beep
 */
void TTYDRV_Beep(void)
{
}

/***********************************************************************
 *		TTYDRV_GetDIState
 */
BOOL TTYDRV_GetDIState(DWORD len, LPVOID ptr)
{
  return TRUE;
}

/***********************************************************************
 *		TTYDRV_GetDIData
 */
BOOL TTYDRV_GetDIData( BYTE *keystate, DWORD dodsize, LPDIDEVICEOBJECTDATA dod,
                       LPDWORD entries, DWORD flags )
{
  return TRUE;
}

/***********************************************************************
 *		TTYDRV_GetKeyboardConfig
 */
void TTYDRV_GetKeyboardConfig(KEYBOARD_CONFIG *cfg)
{
}

/***********************************************************************
 *		TTYDRV_SetKeyboardConfig
 */
extern void TTYDRV_SetKeyboardConfig(KEYBOARD_CONFIG *cfg, DWORD mask)
{
}

/***********************************************************************
 *           TTYDRV_InitMouse
 */
void TTYDRV_InitMouse(LPMOUSE_EVENT_PROC proc)
{
}

/***********************************************************************
 *		TTYDRV_SetCursor
 */
void TTYDRV_SetCursor( struct tagCURSORICONINFO *lpCursor )
{
}

/***********************************************************************
 *		TTYDRV_MoveCursor
 */
void TTYDRV_MoveCursor(WORD wAbsX, WORD wAbsY)
{
}

/***********************************************************************
 *              TTYDRV_GetScreenSaveActive
 *
 * Returns the active status of the screen saver
 */
BOOL TTYDRV_GetScreenSaveActive(void)
{
    return FALSE;
}

/***********************************************************************
 *              TTYDRV_SetScreenSaveActive
 *
 * Activate/Deactivate the screen saver
 */
void TTYDRV_SetScreenSaveActive(BOOL bActivate)
{
    FIXME("(%d): stub\n", bActivate);
}

/***********************************************************************
 *              TTYDRV_GetScreenSaveTimeout
 *
 * Return the screen saver timeout
 */
int TTYDRV_GetScreenSaveTimeout(void)
{
    return 0;
}

/***********************************************************************
 *              TTYDRV_SetScreenSaveTimeout
 *
 * Set the screen saver timeout
 */
void TTYDRV_SetScreenSaveTimeout(int nTimeout)
{
    FIXME("(%d): stub\n", nTimeout);
}

/**********************************************************************
 *		TTYDRV_LoadOEMResource
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
 *              TTYDRV_IsSingleWindow
 */
BOOL TTYDRV_IsSingleWindow(void)
{
    return TRUE;
}

/***********************************************************************
 *		TTYDRV_AcquireClipboard
 */
void TTYDRV_AcquireClipboard(void)
{
}

/***********************************************************************
 *		TTYDRV_ReleaseClipboard
 */
void TTYDRV_ReleaseClipboard(void)
{
}

/***********************************************************************
 *		TTYDRV_SetClipboardData
 */
void TTYDRV_SetClipboardData(UINT wFormat)
{
}

/***********************************************************************
 *		TTYDRV_GetClipboardData
 */
BOOL TTYDRV_GetClipboardData(UINT wFormat)
{
  return FALSE;
}

/***********************************************************************
 *		TTYDRV_IsClipboardFormatAvailable
 */
BOOL TTYDRV_IsClipboardFormatAvailable(UINT wFormat)
{
  return FALSE;
}

/**************************************************************************
 *		TTYDRV_RegisterClipboardFormat
 *
 * Registers a custom clipboard format
 * Returns: TRUE - new format registered, FALSE - Format already registered
 */
BOOL TTYDRV_RegisterClipboardFormat( LPCSTR FormatName )
{
  return TRUE;
}

/**************************************************************************
 *		TTYDRV_IsSelectionOwner
 *
 * Returns: TRUE - We(WINE) own the selection, FALSE - Selection not owned by us
 */
BOOL TTYDRV_IsSelectionOwner(void)
{
    return FALSE;
}

/***********************************************************************
 *		TTYDRV_ResetSelectionOwner
 */
void TTYDRV_ResetSelectionOwner(struct tagWND *pWnd, BOOL bFooBar)
{
}
