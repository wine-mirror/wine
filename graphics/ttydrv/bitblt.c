/*
 * TTY DC bit blit
 *
 * Copyright 1999 Patrik Stridvall
 */

#include "dc.h"
#include "debugtools.h"
#include "pen.h"
#include "ttydrv.h"

DEFAULT_DEBUG_CHANNEL(ttydrv)

/***********************************************************************
 *		TTYDRV_DC_BitBlt
 */
BOOL TTYDRV_DC_BitBlt(DC *dcDst, INT xDst, INT yDst,
		      INT width, INT height, DC *dcSrc,
		      INT xSrc, INT ySrc, DWORD rop)
{
  FIXME("(%p, %d, %d, %d, %d, %p, %d, %d, %lu): stub\n",
	dcDst, xDst, yDst, width, height, 
        dcSrc, xSrc, ySrc, rop
  );

  return TRUE;
}

/***********************************************************************
 *		TTYDRV_DC_PatBlt
 */
BOOL TTYDRV_DC_PatBlt(DC *dc, INT left, INT top,
		      INT width, INT height, DWORD rop)
{
  FIXME("(%p, %d, %d, %d, %d, %lu): stub\n",
	dc, left, top, width, height, rop
  );


  return TRUE;
}

/***********************************************************************
 *		TTYDRV_DC_StretchBlt
 */
BOOL TTYDRV_DC_StretchBlt(DC *dcDst, INT xDst, INT yDst,
			  INT widthDst, INT heightDst,
			  DC *dcSrc, INT xSrc, INT ySrc,
			  INT widthSrc, INT heightSrc, DWORD rop)
{
  FIXME("(%p, %d, %d, %d, %d, %p, %d, %d, %d, %d, %lu): stub\n",
	dcDst, xDst, yDst, widthDst, heightDst,
	dcSrc, xSrc, ySrc, widthSrc, heightSrc, rop
  );

  return TRUE;
}

