/*
 * TTY main driver
 *
 * Copyright 1998 Patrik Stridvall
 *
 */

#include "config.h"

#include "clipboard.h"
#include "desktop.h"
#include "message.h"
#include "keyboard.h"
#include "monitor.h"
#include "mouse.h"
#include "ttydrv.h"
#include "win.h"

/***********************************************************************
 *              TTYDRV_USER_Initialize
 */
BOOL TTYDRV_USER_Initialize(void)
{
  CLIPBOARD_Driver = &TTYDRV_CLIPBOARD_Driver;
  DESKTOP_Driver = &TTYDRV_DESKTOP_Driver;
  EVENT_Driver = &TTYDRV_EVENT_Driver;
  KEYBOARD_Driver = &TTYDRV_KEYBOARD_Driver;
  MONITOR_Driver = &TTYDRV_MONITOR_Driver;
  MOUSE_Driver = &TTYDRV_MOUSE_Driver;
  WND_Driver = &TTYDRV_WND_Driver;

  return TRUE;
}

/***********************************************************************
 *              TTYDRV_USER_Finalize
 */
void TTYDRV_USER_Finalize(void)
{
}

/**************************************************************************
 *		TTYDRV_USER_BeginDebugging
 */
void TTYDRV_USER_BeginDebugging(void)
{
}

/**************************************************************************
 *		TTYDRV_USER_EndDebugging
 */
void TTYDRV_USER_EndDebugging(void)
{
}


