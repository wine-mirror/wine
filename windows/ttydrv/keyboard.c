/*
 * TTY keyboard driver
 *
 * Copyright 1998 Patrik Stridvall
 */

#include "dinput.h"
#include "keyboard.h"
#include "ttydrv.h"

/***********************************************************************
 *		TTYDRV_KEYBOARD_Init
 */
void TTYDRV_KEYBOARD_Init(void)
{
}

/***********************************************************************
 *		TTYDRV_KEYBOARD_VkKeyScan
 */
WORD TTYDRV_KEYBOARD_VkKeyScan(CHAR cChar)
{
  return 0;
}

/***********************************************************************
 *		TTYDRV_KEYBOARD_MapVirtualKey
 */
UINT16 TTYDRV_KEYBOARD_MapVirtualKey(UINT16 wCode, UINT16 wMapType)
{
  return 0;
}

/***********************************************************************
 *		TTYDRV_KEYBOARD_GetKeyNameText
 */
INT16 TTYDRV_KEYBOARD_GetKeyNameText(
  LONG lParam, LPSTR lpBuffer, INT16 nSize)
{  
  if(lpBuffer && nSize)
    {
      *lpBuffer = 0;
    }
  return 0;
}

/***********************************************************************
 *		TTYDRV_KEYBOARD_ToAscii
 */
INT16 TTYDRV_KEYBOARD_ToAscii(
   UINT16 virtKey, UINT16 scanCode,
   LPBYTE lpKeyState, LPVOID lpChar, UINT16 flags)
{
  return 0;
}

/***********************************************************************
 *		TTYDRV_KEYBOARD_GetBeepActive
 */
BOOL TTYDRV_KEYBOARD_GetBeepActive()
{
  return FALSE;
}

/***********************************************************************
 *		TTYDRV_KEYBOARD_SetBeepActive
 */
void TTYDRV_KEYBOARD_SetBeepActive(BOOL bActivate)
{
}

/***********************************************************************
 *		TTYDRV_KEYBOARD_Beep
 */
void TTYDRV_KEYBOARD_Beep()
{
}

/***********************************************************************
 *		TTYDRV_KEYBOARD_GetDIState
 */
BOOL TTYDRV_KEYBOARD_GetDIState(DWORD len, LPVOID ptr)
{
  return TRUE;
}

/***********************************************************************
 *		TTYDRV_KEYBOARD_GetDIData
 */
BOOL TTYDRV_KEYBOARD_GetDIData(
  BYTE *keystate,
  DWORD dodsize, LPDIDEVICEOBJECTDATA dod,
  LPDWORD entries, DWORD flags)
{
  return TRUE;
}

/***********************************************************************
 *		TTYDRV_KEYBOARD_GetKeyboardConfig
 */
void TTYDRV_KEYBOARD_GetKeyboardConfig(KEYBOARD_CONFIG *cfg) {

}

/***********************************************************************
 *		TTYDRV_KEYBOARD_SetKeyboardConfig
 */
extern void TTYDRV_KEYBOARD_SetKeyboardConfig(KEYBOARD_CONFIG *cfg, DWORD mask) {

}

