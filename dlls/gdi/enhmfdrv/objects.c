/*
 * Enhanced MetaFile objects
 *
 * Copyright 1999 Huw D M Davies
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "bitmap.h"
#include "enhmfdrv/enhmetafiledrv.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(enhmetafile);


/******************************************************************
 *         EMFDRV_AddHandle
 */
static UINT EMFDRV_AddHandle( PHYSDEV dev, HGDIOBJ obj )
{
    EMFDRV_PDEVICE *physDev = (EMFDRV_PDEVICE *)dev;
    UINT index;

    for(index = 0; index < physDev->handles_size; index++)
        if(physDev->handles[index] == 0) break;
    if(index == physDev->handles_size) {
        physDev->handles_size += HANDLE_LIST_INC;
	physDev->handles = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
				       physDev->handles,
				       physDev->handles_size * sizeof(physDev->handles[0]));
    }
    physDev->handles[index] = obj;

    physDev->cur_handles++;
    if(physDev->cur_handles > physDev->emh->nHandles)
        physDev->emh->nHandles++;

    return index + 1; /* index 0 is reserved for the hmf, so we increment everything by 1 */
}

/******************************************************************
 *         EMFDRV_FindObject
 */
static UINT EMFDRV_FindObject( PHYSDEV dev, HGDIOBJ obj )
{
    EMFDRV_PDEVICE *physDev = (EMFDRV_PDEVICE*) dev;
    UINT index;

    for(index = 0; index < physDev->handles_size; index++)
        if(physDev->handles[index] == obj) break;

    if(index == physDev->handles_size) return 0;

    return index + 1;
}


/******************************************************************
 *         EMFDRV_DeleteObject
 */
BOOL EMFDRV_DeleteObject( PHYSDEV dev, HGDIOBJ obj )
{
    EMRDELETEOBJECT emr;
    EMFDRV_PDEVICE *physDev = (EMFDRV_PDEVICE*) dev;
    UINT index;
    BOOL ret = TRUE;

    if(!(index = EMFDRV_FindObject(dev, obj))) return 0;

    emr.emr.iType = EMR_DELETEOBJECT;
    emr.emr.nSize = sizeof(emr);
    emr.ihObject = index;

    if(!EMFDRV_WriteRecord( dev, &emr.emr ))
        ret = FALSE;

    physDev->handles[index - 1] = 0;
    physDev->cur_handles--;
    return ret;
}

  
/***********************************************************************
 *           EMFDRV_SelectBitmap
 */
HBITMAP EMFDRV_SelectBitmap( PHYSDEV dev, HBITMAP hbitmap )
{
    return 0;
}


/***********************************************************************
 *           EMFDRV_CreateBrushIndirect
 */
DWORD EMFDRV_CreateBrushIndirect( PHYSDEV dev, HBRUSH hBrush )
{
    DWORD index = 0;
    LOGBRUSH logbrush;

    if (!GetObjectA( hBrush, sizeof(logbrush), &logbrush )) return 0;

    switch (logbrush.lbStyle) {
    case BS_SOLID:
    case BS_HATCHED:
    case BS_NULL:
      {
	EMRCREATEBRUSHINDIRECT emr;
	emr.emr.iType = EMR_CREATEBRUSHINDIRECT;
	emr.emr.nSize = sizeof(emr);
	emr.ihBrush = index = EMFDRV_AddHandle( dev, hBrush );
	emr.lb = logbrush;

	if(!EMFDRV_WriteRecord( dev, &emr.emr ))
	    index = 0;
      }
      break;
    case BS_DIBPATTERN:
      {
	EMRCREATEDIBPATTERNBRUSHPT *emr;
	DWORD bmSize, biSize, size;
	BITMAPINFO *info = GlobalLock16(logbrush.lbHatch);

	if (info->bmiHeader.biCompression)
            bmSize = info->bmiHeader.biSizeImage;
        else
	    bmSize = DIB_GetDIBImageBytes(info->bmiHeader.biWidth,
					  info->bmiHeader.biHeight,
					  info->bmiHeader.biBitCount);
	biSize = DIB_BitmapInfoSize(info, LOWORD(logbrush.lbColor));
	size = sizeof(EMRCREATEDIBPATTERNBRUSHPT) + biSize + bmSize;
	emr = HeapAlloc( GetProcessHeap(), 0, size );
	if(!emr) break;
	emr->emr.iType = EMR_CREATEDIBPATTERNBRUSHPT;
	emr->emr.nSize = size;
	emr->ihBrush = index = EMFDRV_AddHandle( dev, hBrush );
	emr->iUsage = LOWORD(logbrush.lbColor);
	emr->offBmi = sizeof(EMRCREATEDIBPATTERNBRUSHPT);
	emr->cbBmi = biSize;
	emr->offBits = sizeof(EMRCREATEDIBPATTERNBRUSHPT) + biSize;
	emr->cbBits = bmSize;
	memcpy((char *)emr + sizeof(EMRCREATEDIBPATTERNBRUSHPT), info,
	       biSize + bmSize );

	if(!EMFDRV_WriteRecord( dev, &emr->emr ))
	    index = 0;
	HeapFree( GetProcessHeap(), 0, emr );
	GlobalUnlock16(logbrush.lbHatch);
      }
      break;

    case BS_PATTERN:
        FIXME("Unsupported style %x\n",
	      logbrush.lbStyle);
        break;
    default:
        FIXME("Unknown style %x\n", logbrush.lbStyle);
	break;
    }
    return index;
}


/***********************************************************************
 *           EMFDRV_SelectBrush
 */
HBRUSH EMFDRV_SelectBrush(PHYSDEV dev, HBRUSH hBrush )
{
    EMFDRV_PDEVICE *physDev = (EMFDRV_PDEVICE*)dev;
    EMRSELECTOBJECT emr;
    DWORD index;
    int i;

    /* If the object is a stock brush object, do not need to create it.
     * See definitions in  wingdi.h for range of stock brushes.
     * We do however have to handle setting the higher order bit to
     * designate that this is a stock object.
     */
    for (i = WHITE_BRUSH; i <= NULL_BRUSH; i++)
    {
        if (hBrush == GetStockObject(i))
        {
            index = i | 0x80000000;
            goto found;
        }
    }
    if((index = EMFDRV_FindObject(dev, hBrush)) != 0)
        goto found;

    if (!(index = EMFDRV_CreateBrushIndirect(dev, hBrush ))) return 0;
    GDI_hdc_using_object(hBrush, physDev->hdc);

 found:
    emr.emr.iType = EMR_SELECTOBJECT;
    emr.emr.nSize = sizeof(emr);
    emr.ihObject = index;
    return EMFDRV_WriteRecord( dev, &emr.emr ) ? hBrush : 0;
}


/******************************************************************
 *         EMFDRV_CreateFontIndirect
 */
static BOOL EMFDRV_CreateFontIndirect(PHYSDEV dev, HFONT hFont )
{
    DWORD index = 0;
    EMREXTCREATEFONTINDIRECTW emr;
    int i;

    if (!GetObjectW( hFont, sizeof(emr.elfw.elfLogFont), &emr.elfw.elfLogFont )) return 0;

    emr.emr.iType = EMR_EXTCREATEFONTINDIRECTW;
    emr.emr.nSize = (sizeof(emr) + 3) / 4 * 4;
    emr.ihFont = index = EMFDRV_AddHandle( dev, hFont );
    emr.elfw.elfFullName[0] = '\0';
    emr.elfw.elfStyle[0]    = '\0';
    emr.elfw.elfVersion     = 0;
    emr.elfw.elfStyleSize   = 0;
    emr.elfw.elfMatch       = 0;
    emr.elfw.elfReserved    = 0;
    for(i = 0; i < ELF_VENDOR_SIZE; i++)
        emr.elfw.elfVendorId[i] = 0;
    emr.elfw.elfCulture                 = PAN_CULTURE_LATIN;
    emr.elfw.elfPanose.bFamilyType      = PAN_NO_FIT;
    emr.elfw.elfPanose.bSerifStyle      = PAN_NO_FIT;
    emr.elfw.elfPanose.bWeight          = PAN_NO_FIT;
    emr.elfw.elfPanose.bProportion      = PAN_NO_FIT;
    emr.elfw.elfPanose.bContrast        = PAN_NO_FIT;
    emr.elfw.elfPanose.bStrokeVariation = PAN_NO_FIT;
    emr.elfw.elfPanose.bArmStyle        = PAN_NO_FIT;
    emr.elfw.elfPanose.bLetterform      = PAN_NO_FIT;
    emr.elfw.elfPanose.bMidline         = PAN_NO_FIT;
    emr.elfw.elfPanose.bXHeight         = PAN_NO_FIT;

    if(!EMFDRV_WriteRecord( dev, &emr.emr ))
        index = 0;
    return index;
}


/***********************************************************************
 *           EMFDRV_SelectFont
 */
HFONT EMFDRV_SelectFont( PHYSDEV dev, HFONT hFont )
{
    EMFDRV_PDEVICE *physDev = (EMFDRV_PDEVICE*)dev;
    EMRSELECTOBJECT emr;
    DWORD index;
    int i;

    /* If the object is a stock font object, do not need to create it.
     * See definitions in  wingdi.h for range of stock fonts.
     * We do however have to handle setting the higher order bit to
     * designate that this is a stock object.
     */

    for (i = OEM_FIXED_FONT; i <= DEFAULT_GUI_FONT; i++)
    {
        if (i != DEFAULT_PALETTE && hFont == GetStockObject(i))
        {
            index = i | 0x80000000;
            goto found;
        }
    }

    if((index = EMFDRV_FindObject(dev, hFont)) != 0)
        goto found;

    if (!(index = EMFDRV_CreateFontIndirect(dev, hFont ))) return HGDI_ERROR;
    GDI_hdc_using_object(hFont, physDev->hdc);

 found:
    emr.emr.iType = EMR_SELECTOBJECT;
    emr.emr.nSize = sizeof(emr);
    emr.ihObject = index;
    if(!EMFDRV_WriteRecord( dev, &emr.emr ))
        return HGDI_ERROR;
    return 0;
}



/******************************************************************
 *         EMFDRV_CreatePenIndirect
 */
static HPEN EMFDRV_CreatePenIndirect(PHYSDEV dev, HPEN hPen )
{
    EMRCREATEPEN emr;
    DWORD index = 0;

    if (!GetObjectA( hPen, sizeof(emr.lopn), &emr.lopn )) return 0;

    emr.emr.iType = EMR_CREATEPEN;
    emr.emr.nSize = sizeof(emr);
    emr.ihPen = index = EMFDRV_AddHandle( dev, hPen );

    if(!EMFDRV_WriteRecord( dev, &emr.emr ))
        index = 0;
    return (HPEN)index;
}

/******************************************************************
 *         EMFDRV_SelectPen
 */
HPEN EMFDRV_SelectPen(PHYSDEV dev, HPEN hPen )
{
    EMFDRV_PDEVICE *physDev = (EMFDRV_PDEVICE*)dev;
    EMRSELECTOBJECT emr;
    DWORD index;
    int i;

    /* If the object is a stock pen object, do not need to create it.
     * See definitions in  wingdi.h for range of stock pens.
     * We do however have to handle setting the higher order bit to
     * designate that this is a stock object.
     */

    for (i = WHITE_PEN; i <= NULL_PEN; i++)
    {
        if (hPen == GetStockObject(i))
        {
            index = i | 0x80000000;
            goto found;
        }
    }
    if((index = EMFDRV_FindObject(dev, hPen)) != 0)
        goto found;

    if (!(index = (DWORD)EMFDRV_CreatePenIndirect(dev, hPen ))) return 0;
    GDI_hdc_using_object(hPen, physDev->hdc);

 found:
    emr.emr.iType = EMR_SELECTOBJECT;
    emr.emr.nSize = sizeof(emr);
    emr.ihObject = index;
    return EMFDRV_WriteRecord( dev, &emr.emr ) ? hPen : 0;
}


/******************************************************************
 *         EMFDRV_GdiComment
 */
BOOL EMFDRV_GdiComment(PHYSDEV dev, UINT bytes, CONST BYTE *buffer)
{
    EMRGDICOMMENT *emr;
    UINT total, rounded_size;
    BOOL ret;

    rounded_size = (bytes+3) & ~3;
    total = offsetof(EMRGDICOMMENT,Data) + rounded_size;

    emr = HeapAlloc(GetProcessHeap(), 0, total);
    emr->emr.iType = EMR_GDICOMMENT;
    emr->emr.nSize = total;
    emr->cbData = bytes;
    memset(&emr->Data[bytes], 0, rounded_size - bytes);
    memcpy(&emr->Data[0], buffer, bytes);

    ret = EMFDRV_WriteRecord( dev, &emr->emr );

    HeapFree(GetProcessHeap(), 0, emr);

    return ret;
}
