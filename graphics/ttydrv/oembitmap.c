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
 *		TTYDRV_DC_LoadOEMBitmap
 */
static HANDLE TTYDRV_DC_LoadOEMBitmap(WORD resid)
{
  HBITMAP hbitmap;

  TRACE("(%d)\n", resid);
 
  hbitmap = CreateBitmap(1, 1, 1, 1, NULL);
  TTYDRV_DC_CreateBitmap(hbitmap);

  return hbitmap;
}

/**********************************************************************
 *		TTYDRV_DC_LoadOEMCursorIcon
 */
static HANDLE TTYDRV_DC_LoadOEMCursorIcon(WORD resid, BOOL bCursor)
{
  return (HANDLE) NULL;
}

/**********************************************************************
 *		TTYDRV_DC_LoadOEMResource
 */
HANDLE TTYDRV_DC_LoadOEMResource(WORD resid, WORD type)
{
  switch(type)
  {
    case OEM_BITMAP:
      return TTYDRV_DC_LoadOEMBitmap(resid);
    case OEM_CURSOR:
      return TTYDRV_DC_LoadOEMCursorIcon(resid, TRUE);
    case OEM_ICON:
      return TTYDRV_DC_LoadOEMCursorIcon(resid, FALSE);
    default:
      ERR("unknown type (%d)\n", type);
  }

  return (HANDLE) NULL;
}


