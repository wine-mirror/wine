/*
 * TTY palette driver
 *
 * Copyright 1999 Patrik Stridvall
 */

#include "palette.h"
#include "ttydrv.h"

/**********************************************************************
 *	     TTYDRV_PALETTE_Initialize
 */
BOOL TTYDRV_PALETTE_Initialize(void)
{
  return TRUE;
}

/**********************************************************************
 *	     TTYDRV_PALETTE_Finalize
 *
 */
void TTYDRV_PALETTE_Finalize(void)
{
}

/***********************************************************************
 *           TTYDRV_PALETTE_SetMapping
 */
int TTYDRV_PALETTE_SetMapping(
  PALETTEOBJ *palPtr, UINT uStart, UINT uNum, BOOL mapOnly)
{
  return 0;
}

/***********************************************************************
 *           TTYDRV_PALETTE_UpdateMapping
 */
int TTYDRV_PALETTE_UpdateMapping(PALETTEOBJ *palPtr)
{
  return 0;
}

/***********************************************************************
 *           TTYDRV_PALETTE_IsDark
 */
int TTYDRV_PALETTE_IsDark(int pixel)
{
  return 0;
}
