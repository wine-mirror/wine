/*
 * TTY keyboard driver
 *
 * Copyright 1998 Patrik Stridvall
 */

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






