
/*
 * Win32 GDI functions
 *
 * Copyright 1996 Thomas Sandford t.d.g.sandford@prds-grn.demon.co.uk
 */

#include <windows.h>
#include <gdi.h>
#include <pen.h>
#include <brush.h>
#include <bitmap.h>
#include <font.h>
#include <palette.h>
#include <debug.h>

int WIN32_GetObject( HANDLE handle, int count, LPVOID buffer )

/* largely a copy of GetObject, but with size mangling capabilities to
convert between win16 and win32 objects. Yeuch! */

{
    void *temp = alloca(count);
    GDIOBJHDR * ptr = NULL;

    dprintf_win32(stddeb, "WIN32_GetObject: %d %d %p\n", handle, count, buffer);

    if ((!count) || (temp == NULL))
	return 0;

    ptr = GDI_GetObjPtr(handle, MAGIC_DONTCARE);
    if (!ptr) return 0;

    /* FIXME: only bitmaps fixed so far */

    switch(ptr->wMagic)
    {
      case PEN_MAGIC:
          return PEN_GetObject( (PENOBJ *)ptr, count, buffer );
      case BRUSH_MAGIC:
          return BRUSH_GetObject( (BRUSHOBJ *)ptr, count, buffer );
      case BITMAP_MAGIC: {
	BITMAP *pbm = (BITMAP *)temp;
	int *pdest = (int *)buffer;

	if (buffer == NULL)
		return 28;

	BITMAP_GetObject( (BITMAPOBJ *)ptr, count, temp );
	if (count > 3)
		pdest[0] = pbm->bmType;
	if (count > 7)
		pdest[1] = pbm->bmWidth;
	if (count > 11)
		pdest[2] = pbm->bmHeight;
	if (count > 15)
		pdest[3] = pbm->bmWidthBytes;
	if (count > 19)
		pdest[4] = pbm->bmPlanes;
	if (count > 23)
		pdest[5] = pbm->bmBitsPixel;
	if (count > 27)
		pdest[6] = pbm->bmBits;
	
	return (count > 28) ? 28 : count - (count % 4);
      }
      case FONT_MAGIC:
          return FONT_GetObject( (FONTOBJ *)ptr, count, buffer );
      case PALETTE_MAGIC:
          return PALETTE_GetObject( (PALETTEOBJ *)ptr, count, buffer );
    }
    return 0;
}

