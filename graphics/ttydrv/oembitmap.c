/*
 * TTY DC OEM bitmap
 *
 * Copyright 1999 Patrik Stridvall
 */

#include "bitmap.h"
#include "ttydrv.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(ttydrv)

/**********************************************************************
 *		TTYDRV_LoadOEMBitmap
 */
static HANDLE TTYDRV_LoadOEMBitmap(WORD resid)
{
  HBITMAP hbitmap;

  TRACE("(%d)\n", resid);
 
  hbitmap = CreateBitmap(1, 1, 1, 1, NULL);
  TTYDRV_DC_CreateBitmap(hbitmap);

  return hbitmap;
}

/**********************************************************************
 *		TTYDRV_LoadOEMCursorIcon
 */
static HANDLE TTYDRV_LoadOEMCursorIcon(WORD resid, BOOL bCursor)
{
  return (HANDLE) NULL;
}

/**********************************************************************
 *		TTYDRV_LoadOEMResource
 */
HANDLE TTYDRV_LoadOEMResource(WORD resid, WORD type)
{
  switch(type)
  {
    case OEM_BITMAP:
      return TTYDRV_LoadOEMBitmap(resid);
    case OEM_CURSOR:
      return TTYDRV_LoadOEMCursorIcon(resid, TRUE);
    case OEM_ICON:
      return TTYDRV_LoadOEMCursorIcon(resid, FALSE);
    default:
      ERR("unknown type (%d)\n", type);
  }

  return (HANDLE) NULL;
}
