/*
 * TTYDRV USER driver functions
 *
 * Copyright 1998 Patrik Stridvall
 */

#include "config.h"

#include "dinput.h"
#include "gdi.h"
#include "ttydrv.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(ttydrv);


/***********************************************************************
 *		VkKeyScan (TTYDRV.@)
 */
WORD TTYDRV_VkKeyScan(CHAR cChar)
{
  return 0;
}

/***********************************************************************
 *		MapVirtualKey (TTYDRV.@)
 */
UINT16 TTYDRV_MapVirtualKey(UINT16 wCode, UINT16 wMapType)
{
  return 0;
}

/***********************************************************************
 *		GetKeyNameText (TTYDRV.@)
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
 *		ToUnicode (TTYDRV.@)
 */
INT TTYDRV_ToUnicode( UINT virtKey, UINT scanCode, LPBYTE lpKeyState,
		      LPWSTR pwszBuff, int cchBuff, UINT flags )
{
  return 0;
}

/***********************************************************************
 *		Beep (TTYDRV.@)
 */
void TTYDRV_Beep(void)
{
}

/***********************************************************************
 *		SetCursor (TTYDRV.@)
 */
void TTYDRV_SetCursor( struct tagCURSORICONINFO *lpCursor )
{
}

/***********************************************************************
 *              GetScreenSaveActive (TTYDRV.@)
 *
 * Returns the active status of the screen saver
 */
BOOL TTYDRV_GetScreenSaveActive(void)
{
    return FALSE;
}

/***********************************************************************
 *              SetScreenSaveActive (TTYDRV.@)
 *
 * Activate/Deactivate the screen saver
 */
void TTYDRV_SetScreenSaveActive(BOOL bActivate)
{
    FIXME("(%d): stub\n", bActivate);
}

/**********************************************************************
 *		LoadOEMResource (TTYDRV.@)
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
 *		AcquireClipboard (TTYDRV.@)
 */
void TTYDRV_AcquireClipboard(void)
{
}

/***********************************************************************
 *		ReleaseClipboard (TTYDRV.@)
 */
void TTYDRV_ReleaseClipboard(void)
{
}

/***********************************************************************
 *		SetClipboardData (TTYDRV.@)
 */
void TTYDRV_SetClipboardData(UINT wFormat)
{
}

/***********************************************************************
 *		GetClipboardData (TTYDRV.@)
 */
BOOL TTYDRV_GetClipboardData(UINT wFormat)
{
  return FALSE;
}

/***********************************************************************
 *		IsClipboardFormatAvailable (TTYDRV.@)
 */
BOOL TTYDRV_IsClipboardFormatAvailable(UINT wFormat)
{
  return FALSE;
}

/**************************************************************************
 *		RegisterClipboardFormat (TTYDRV.@)
 *
 * Registers a custom clipboard format
 * Returns: TRUE - new format registered, FALSE - Format already registered
 */
BOOL TTYDRV_RegisterClipboardFormat( LPCSTR FormatName )
{
  return TRUE;
}

/**************************************************************************
 *		IsSelectionOwner (TTYDRV.@)
 *
 * Returns: TRUE - We(WINE) own the selection, FALSE - Selection not owned by us
 */
BOOL TTYDRV_IsSelectionOwner(void)
{
    return FALSE;
}
