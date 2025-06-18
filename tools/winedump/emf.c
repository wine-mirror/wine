/*
 *  Dump an Enhanced Meta File
 *
 *  Copyright 2005 Mike McCormack
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"

#include <stdio.h>
#include <fcntl.h>
#include <stdarg.h>

#include "winedump.h"
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "gdiplusenums.h"

typedef struct
{
    WORD Type;
    WORD Flags;
    DWORD Size;
    DWORD DataSize;
} EmfPlusRecordHeader;

static const char *debugstr_wn(const WCHAR *wstr, unsigned int n)
{
    static char buf[80];
    char *p;
    unsigned int i;

    if (!wstr) return "(null)";

    i = 0;
    p = buf;
    *p++ = '\"';
    while (i < n && i < sizeof(buf) - 3 && wstr[i])
    {
        if (wstr[i] < 127) *p++ = wstr[i];
        else *p++ = '.';
        i++;
    }
    *p++ = '\"';
    *p = 0;
    return buf;
}

static const char *debugstr_rect(const RECTL *rect)
{
    return strmake( "%d,%d - %d,%d",
                    (UINT)rect->left, (UINT)rect->top, (UINT)rect->right, (UINT)rect->bottom );
}

static unsigned int read_int(const unsigned char *buffer)
{
    return buffer[0]
     + (buffer[1]<<8)
     + (buffer[2]<<16)
     + (buffer[3]<<24);
}

#define EMRCASE(x) case x: printf("%s%-20s %08x\n", pfx, #x, length); break
#define EMRPLUSCASE(x) case x: printf("%s    %-20s %04x %08x %08x\n", pfx, #x, \
        (UINT)header->Flags, (UINT)header->Size, (UINT)header->DataSize); break

unsigned long dump_emfrecord(const char *pfx, unsigned long offset)
{
    const unsigned char*        ptr;
    unsigned int type, length, i;

    ptr = PRD(offset, 8);
    if (!ptr) return 0;

    type = read_int(ptr);
    length = read_int(ptr + 4);

    switch(type)
    {
    case EMR_HEADER:
    {
        const ENHMETAHEADER *header = PRD(offset, sizeof(*header));

        printf("%s%-20s %08x\n", pfx, "EMR_HEADER", length);
        printf("%sbounds (%s) frame (%s) signature %#x version %#x bytes %#x records %#x\n"
               "%shandles %#x reserved %#x palette entries %#x px %dx%d mm %dx%d μm %dx%d opengl %d description %s\n",
               pfx, debugstr_rect( &header->rclBounds ), debugstr_rect( &header->rclFrame ),
               (UINT)header->dSignature, (UINT)header->nVersion, (UINT)header->nBytes,
               (UINT)header->nRecords, pfx, (UINT)header->nHandles, header->sReserved, (UINT)header->nPalEntries,
               (UINT)header->szlDevice.cx, (UINT)header->szlDevice.cy,
               (UINT)header->szlMillimeters.cx, (UINT)header->szlMillimeters.cy,
               (UINT)header->szlMicrometers.cx, (UINT)header->szlMicrometers.cy,
               (UINT)header->bOpenGL,
               debugstr_wn((LPCWSTR)((const BYTE *)header + header->offDescription), header->nDescription));
        break;
    }

    EMRCASE(EMR_POLYBEZIER);
    EMRCASE(EMR_POLYGON);
    EMRCASE(EMR_POLYLINE);
    EMRCASE(EMR_POLYBEZIERTO);
    EMRCASE(EMR_POLYLINETO);
    EMRCASE(EMR_POLYPOLYLINE);
    EMRCASE(EMR_POLYPOLYGON);
    EMRCASE(EMR_SETWINDOWEXTEX);
    EMRCASE(EMR_SETWINDOWORGEX);
    EMRCASE(EMR_SETVIEWPORTEXTEX);
    EMRCASE(EMR_SETVIEWPORTORGEX);
    EMRCASE(EMR_SETBRUSHORGEX);
    EMRCASE(EMR_EOF);
    EMRCASE(EMR_SETPIXELV);
    EMRCASE(EMR_SETMAPPERFLAGS);
    EMRCASE(EMR_SETMAPMODE);
    EMRCASE(EMR_SETBKMODE);
    EMRCASE(EMR_SETPOLYFILLMODE);
    EMRCASE(EMR_SETROP2);
    EMRCASE(EMR_SETSTRETCHBLTMODE);
    EMRCASE(EMR_SETTEXTALIGN);
    EMRCASE(EMR_SETCOLORADJUSTMENT);
    EMRCASE(EMR_SETTEXTCOLOR);
    EMRCASE(EMR_SETBKCOLOR);
    EMRCASE(EMR_OFFSETCLIPRGN);
    EMRCASE(EMR_MOVETOEX);
    EMRCASE(EMR_SETMETARGN);
    EMRCASE(EMR_EXCLUDECLIPRECT);

    case EMR_INTERSECTCLIPRECT:
    {
        const EMRINTERSECTCLIPRECT *clip = PRD(offset, sizeof(*clip));

        printf("%s%-20s %08x\n", pfx, "EMR_INTERSECTCLIPRECT", length);
        printf("%srect %s\n", pfx, debugstr_rect( &clip->rclClip ));
        break;
    }

    EMRCASE(EMR_SCALEVIEWPORTEXTEX);
    EMRCASE(EMR_SCALEWINDOWEXTEX);
    EMRCASE(EMR_SAVEDC);
    EMRCASE(EMR_RESTOREDC);
    EMRCASE(EMR_SETWORLDTRANSFORM);
    EMRCASE(EMR_MODIFYWORLDTRANSFORM);
    EMRCASE(EMR_SELECTOBJECT);
    EMRCASE(EMR_CREATEPEN);
    EMRCASE(EMR_CREATEBRUSHINDIRECT);
    EMRCASE(EMR_DELETEOBJECT);
    EMRCASE(EMR_ANGLEARC);
    EMRCASE(EMR_ELLIPSE);
    EMRCASE(EMR_RECTANGLE);
    EMRCASE(EMR_ROUNDRECT);
    EMRCASE(EMR_ARC);
    EMRCASE(EMR_CHORD);
    EMRCASE(EMR_PIE);
    EMRCASE(EMR_SELECTPALETTE);
    EMRCASE(EMR_CREATEPALETTE);
    EMRCASE(EMR_SETPALETTEENTRIES);
    EMRCASE(EMR_RESIZEPALETTE);
    EMRCASE(EMR_REALIZEPALETTE);
    EMRCASE(EMR_EXTFLOODFILL);
    EMRCASE(EMR_LINETO);
    EMRCASE(EMR_ARCTO);
    EMRCASE(EMR_POLYDRAW);
    EMRCASE(EMR_SETARCDIRECTION);
    EMRCASE(EMR_BEGINPATH);
    EMRCASE(EMR_ENDPATH);
    EMRCASE(EMR_CLOSEFIGURE);
    EMRCASE(EMR_FILLPATH);
    EMRCASE(EMR_STROKEANDFILLPATH);
    EMRCASE(EMR_STROKEPATH);
    EMRCASE(EMR_FLATTENPATH);
    EMRCASE(EMR_WIDENPATH);
    EMRCASE(EMR_SELECTCLIPPATH);
    EMRCASE(EMR_ABORTPATH);

    case EMR_SETMITERLIMIT:
    {
        const EMRSETMITERLIMIT *record = PRD(offset, sizeof(*record));

        printf("%s%-20s %08x\n", pfx, "EMR_SETMITERLIMIT", length);
        printf("%s miter limit %u\n", pfx, *(unsigned int *)&record->eMiterLimit);
        break;
    }

    case EMR_GDICOMMENT:
    {
        printf("%s%-20s %08x\n", pfx, "EMR_GDICOMMENT", length);

        /* Handle EMF+ records */
        if (length >= 16 && !memcmp((char*)PRD(offset + 12, sizeof(unsigned int)), "EMF+", 4))
        {
            const EmfPlusRecordHeader *header;
            const unsigned int *data_size;

            offset += 8;
            length -= 8;
            data_size = PRD(offset, sizeof(*data_size));
            printf("%sdata size = %x\n", pfx, *data_size);
            offset += 8;
            length -= 8;

            while (length >= sizeof(*header))
            {
                header = PRD(offset, sizeof(*header));
                switch(header->Type)
                {
                EMRPLUSCASE(EmfPlusRecordTypeInvalid);
                EMRPLUSCASE(EmfPlusRecordTypeHeader);
                EMRPLUSCASE(EmfPlusRecordTypeEndOfFile);
                EMRPLUSCASE(EmfPlusRecordTypeComment);
                EMRPLUSCASE(EmfPlusRecordTypeGetDC);
                EMRPLUSCASE(EmfPlusRecordTypeMultiFormatStart);
                EMRPLUSCASE(EmfPlusRecordTypeMultiFormatSection);
                EMRPLUSCASE(EmfPlusRecordTypeMultiFormatEnd);
                EMRPLUSCASE(EmfPlusRecordTypeObject);
                EMRPLUSCASE(EmfPlusRecordTypeClear);
                EMRPLUSCASE(EmfPlusRecordTypeFillRects);
                EMRPLUSCASE(EmfPlusRecordTypeDrawRects);
                EMRPLUSCASE(EmfPlusRecordTypeFillPolygon);
                EMRPLUSCASE(EmfPlusRecordTypeDrawLines);
                EMRPLUSCASE(EmfPlusRecordTypeFillEllipse);
                EMRPLUSCASE(EmfPlusRecordTypeDrawEllipse);
                EMRPLUSCASE(EmfPlusRecordTypeFillPie);
                EMRPLUSCASE(EmfPlusRecordTypeDrawPie);
                EMRPLUSCASE(EmfPlusRecordTypeDrawArc);
                EMRPLUSCASE(EmfPlusRecordTypeFillRegion);
                EMRPLUSCASE(EmfPlusRecordTypeFillPath);
                EMRPLUSCASE(EmfPlusRecordTypeDrawPath);
                EMRPLUSCASE(EmfPlusRecordTypeFillClosedCurve);
                EMRPLUSCASE(EmfPlusRecordTypeDrawClosedCurve);
                EMRPLUSCASE(EmfPlusRecordTypeDrawCurve);
                EMRPLUSCASE(EmfPlusRecordTypeDrawBeziers);
                EMRPLUSCASE(EmfPlusRecordTypeDrawImage);
                EMRPLUSCASE(EmfPlusRecordTypeDrawImagePoints);
                EMRPLUSCASE(EmfPlusRecordTypeDrawString);
                EMRPLUSCASE(EmfPlusRecordTypeSetRenderingOrigin);
                EMRPLUSCASE(EmfPlusRecordTypeSetAntiAliasMode);
                EMRPLUSCASE(EmfPlusRecordTypeSetTextRenderingHint);
                EMRPLUSCASE(EmfPlusRecordTypeSetTextContrast);
                EMRPLUSCASE(EmfPlusRecordTypeSetInterpolationMode);
                EMRPLUSCASE(EmfPlusRecordTypeSetPixelOffsetMode);
                EMRPLUSCASE(EmfPlusRecordTypeSetCompositingMode);
                EMRPLUSCASE(EmfPlusRecordTypeSetCompositingQuality);
                EMRPLUSCASE(EmfPlusRecordTypeSave);
                EMRPLUSCASE(EmfPlusRecordTypeRestore);
                EMRPLUSCASE(EmfPlusRecordTypeBeginContainer);
                EMRPLUSCASE(EmfPlusRecordTypeBeginContainerNoParams);
                EMRPLUSCASE(EmfPlusRecordTypeEndContainer);
                EMRPLUSCASE(EmfPlusRecordTypeSetWorldTransform);
                EMRPLUSCASE(EmfPlusRecordTypeResetWorldTransform);
                EMRPLUSCASE(EmfPlusRecordTypeMultiplyWorldTransform);
                EMRPLUSCASE(EmfPlusRecordTypeTranslateWorldTransform);
                EMRPLUSCASE(EmfPlusRecordTypeScaleWorldTransform);
                EMRPLUSCASE(EmfPlusRecordTypeRotateWorldTransform);
                EMRPLUSCASE(EmfPlusRecordTypeSetPageTransform);
                EMRPLUSCASE(EmfPlusRecordTypeResetClip);
                EMRPLUSCASE(EmfPlusRecordTypeSetClipRect);
                EMRPLUSCASE(EmfPlusRecordTypeSetClipPath);
                EMRPLUSCASE(EmfPlusRecordTypeSetClipRegion);
                EMRPLUSCASE(EmfPlusRecordTypeOffsetClip);
                EMRPLUSCASE(EmfPlusRecordTypeDrawDriverString);
                EMRPLUSCASE(EmfPlusRecordTypeStrokeFillPath);
                EMRPLUSCASE(EmfPlusRecordTypeSerializableObject);
                EMRPLUSCASE(EmfPlusRecordTypeSetTSGraphics);
                EMRPLUSCASE(EmfPlusRecordTypeSetTSClip);
                EMRPLUSCASE(EmfPlusRecordTotal);

                default:
                    printf("%s    unknown EMF+ record %x %04x %08x\n",
                           pfx, (UINT)header->Type, (UINT)header->Flags, (UINT)header->Size);
                    break;
                }

                if (length<sizeof(*header) || header->Size%4)
                    return 0;

                length -= sizeof(*header);
                offset += sizeof(*header);

                for (i=0; i<header->Size-sizeof(*header); i+=4)
                {
                    if (i%16 == 0)
                        printf("%s       ", pfx);
                    if (!(ptr = PRD(offset, 4))) return 0;
                    length -= 4;
                    offset += 4;
                    printf("%08x ", read_int(ptr));
                    if ((i % 16 == 12) || (i + 4 == header->Size - sizeof(*header)))
                        printf("\n");
                }
            }

            return offset;
        }

        break;
    }

    EMRCASE(EMR_FILLRGN);
    EMRCASE(EMR_FRAMERGN);
    EMRCASE(EMR_INVERTRGN);
    EMRCASE(EMR_PAINTRGN);

    case EMR_EXTSELECTCLIPRGN:
    {
        const EMREXTSELECTCLIPRGN *clip = PRD(offset, sizeof(*clip));
        const RGNDATA *data = (const RGNDATA *)clip->RgnData;
        UINT i, rc_count = 0;
        const RECTL *rc;

        if (length >= sizeof(*clip) + sizeof(*data))
            rc_count = data->rdh.nCount;

        printf("%s%-20s %08x\n", pfx, "EMR_EXTSELECTCLIPRGN", length);
        printf("%smode %d, rects %d\n", pfx, (UINT)clip->iMode, rc_count);
        for (i = 0, rc = (const RECTL *)data->Buffer; i < rc_count; i++)
            printf("%s (%s)", pfx, debugstr_rect( &rc[i] ));
        if (rc_count != 0) printf("\n");
        break;
    }

    EMRCASE(EMR_BITBLT);

    case EMR_STRETCHBLT:
    {
        const EMRSTRETCHBLT *blt = PRD(offset, sizeof(*blt));
        const BITMAPINFOHEADER *bmih = (const BITMAPINFOHEADER *)((const unsigned char *)blt + blt->offBmiSrc);

        printf("%s%-20s %08x\n", pfx, "EMR_STRETCHBLT", length);
        printf("%sbounds (%s) dst %d,%d %dx%d src %d,%d %dx%d rop %#x xform (%f, %f, %f, %f, %f, %f)\n"
               "%sbk_color %#x usage %#x bmi_offset %#x bmi_size %#x bits_offset %#x bits_size %#x\n",
               pfx, debugstr_rect( &blt->rclBounds ), (UINT)blt->xDest, (UINT)blt->yDest, (UINT)blt->cxDest, (UINT)blt->cyDest,
               (UINT)blt->xSrc, (UINT)blt->ySrc, (UINT)blt->cxSrc, (UINT)blt->cySrc, (UINT)blt->dwRop,
               blt->xformSrc.eM11, blt->xformSrc.eM12, blt->xformSrc.eM21,
               blt->xformSrc.eM22, blt->xformSrc.eDx, blt->xformSrc.eDy,
               pfx, (UINT)blt->crBkColorSrc, (UINT)blt->iUsageSrc, (UINT)blt->offBmiSrc, (UINT)blt->cbBmiSrc,
               (UINT)blt->offBitsSrc, (UINT)blt->cbBitsSrc);
        printf("%sBITMAPINFOHEADER biSize %#x biWidth %d biHeight %d biPlanes %d biBitCount %d biCompression %#x\n"
               "%sbiSizeImage %#x biXPelsPerMeter %d biYPelsPerMeter %d biClrUsed %#x biClrImportant %#x\n",
               pfx, (UINT)bmih->biSize, (UINT)bmih->biWidth, (UINT)bmih->biHeight, (UINT)bmih->biPlanes,
               (UINT)bmih->biBitCount, (UINT)bmih->biCompression, pfx, (UINT)bmih->biSizeImage,
               (UINT)bmih->biXPelsPerMeter, (UINT)bmih->biYPelsPerMeter, (UINT)bmih->biClrUsed,
               (UINT)bmih->biClrImportant);
        break;
    }

    EMRCASE(EMR_MASKBLT);
    EMRCASE(EMR_PLGBLT);
    EMRCASE(EMR_SETDIBITSTODEVICE);
    EMRCASE(EMR_STRETCHDIBITS);

    case EMR_EXTCREATEFONTINDIRECTW:
    {
        const EMREXTCREATEFONTINDIRECTW *pf = PRD(offset, sizeof(*pf));
        const LOGFONTW *plf = &pf->elfw.elfLogFont;

        printf("%s%-20s %08x\n", pfx, "EMR_EXTCREATEFONTINDIRECTW", length);
        printf("%s(%d %d %d %d %x out %d clip %x quality %d charset %d) %s %s %s %s\n",
               pfx, (UINT)plf->lfHeight, (UINT)plf->lfWidth, (UINT)plf->lfEscapement, (UINT)plf->lfOrientation,
               (UINT)plf->lfPitchAndFamily, (UINT)plf->lfOutPrecision, (UINT)plf->lfClipPrecision,
               plf->lfQuality, plf->lfCharSet,
               debugstr_wn(plf->lfFaceName, LF_FACESIZE),
               plf->lfWeight > 400 ? "Bold" : "",
               plf->lfItalic ? "Italic" : "",
               plf->lfUnderline ? "Underline" : "");
	break;
    }

    EMRCASE(EMR_EXTTEXTOUTA);

    case EMR_EXTTEXTOUTW:
    {
        const EMREXTTEXTOUTW *etoW = PRD(offset, sizeof(*etoW));
        const int *dx = (const int *)((const BYTE *)etoW + etoW->emrtext.offDx);
        int dx_size;

        printf("%s%-20s %08x\n", pfx, "EMR_EXTTEXTOUTW", length);
        printf("%sbounds (%s) mode %#x x_scale %f y_scale %f pt (%d,%d) rect (%s) flags %#x, %s\n",
               pfx, debugstr_rect( &etoW->rclBounds ), (UINT)etoW->iGraphicsMode, etoW->exScale, etoW->eyScale,
               (UINT)etoW->emrtext.ptlReference.x, (UINT)etoW->emrtext.ptlReference.y,
               debugstr_rect( &etoW->emrtext.rcl ), (UINT)etoW->emrtext.fOptions,
               debugstr_wn((LPCWSTR)((const BYTE *)etoW + etoW->emrtext.offString), etoW->emrtext.nChars));
        printf("%sdx_offset %u {", pfx, (UINT)etoW->emrtext.offDx);
        dx_size = etoW->emrtext.nChars;
        if (etoW->emrtext.fOptions & ETO_PDY)
            dx_size *= 2;
        for (i = 0; i < dx_size; ++i)
        {
            printf("%d", dx[i]);
            if (i != dx_size - 1)
                putchar(',');
        }
        printf("}\n");
	break;
    }

    EMRCASE(EMR_POLYBEZIER16);
    EMRCASE(EMR_POLYGON16);
    EMRCASE(EMR_POLYLINE16);
    EMRCASE(EMR_POLYBEZIERTO16);
    EMRCASE(EMR_POLYLINETO16);
    EMRCASE(EMR_POLYPOLYLINE16);
    EMRCASE(EMR_POLYPOLYGON16);
    EMRCASE(EMR_POLYDRAW16);
    EMRCASE(EMR_CREATEMONOBRUSH);
    EMRCASE(EMR_CREATEDIBPATTERNBRUSHPT);
    EMRCASE(EMR_EXTCREATEPEN);
    EMRCASE(EMR_POLYTEXTOUTA);
    EMRCASE(EMR_POLYTEXTOUTW);
    EMRCASE(EMR_SETICMMODE);
    EMRCASE(EMR_CREATECOLORSPACE);
    EMRCASE(EMR_SETCOLORSPACE);
    EMRCASE(EMR_DELETECOLORSPACE);
    EMRCASE(EMR_GLSRECORD);
    EMRCASE(EMR_GLSBOUNDEDRECORD);
    EMRCASE(EMR_PIXELFORMAT);
    EMRCASE(EMR_DRAWESCAPE);
    EMRCASE(EMR_EXTESCAPE);
    EMRCASE(EMR_STARTDOC);
    EMRCASE(EMR_SMALLTEXTOUT);
    EMRCASE(EMR_FORCEUFIMAPPING);
    EMRCASE(EMR_NAMEDESCAPE);
    EMRCASE(EMR_COLORCORRECTPALETTE);
    EMRCASE(EMR_SETICMPROFILEA);
    EMRCASE(EMR_SETICMPROFILEW);

    case EMR_ALPHABLEND:
    {
        const EMRALPHABLEND *blend = PRD(offset, sizeof(*blend));
        const BITMAPINFOHEADER *bmih = (const BITMAPINFOHEADER *)((const unsigned char *)blend + blend->offBmiSrc);

        printf("%s%-20s %08x\n", pfx, "EMR_ALPHABLEND", length);
        printf("%sbounds (%s) dst %d,%d %dx%d src %d,%d %dx%d rop %#x xform (%f, %f, %f, %f, %f, %f)\n"
               "%sbk_color %#x usage %#x bmi_offset %#x bmi_size %#x bits_offset %#x bits_size %#x\n",
               pfx, debugstr_rect( &blend->rclBounds ), (UINT)blend->xDest, (UINT)blend->yDest, (UINT)blend->cxDest, (UINT)blend->cyDest,
               (UINT)blend->xSrc, (UINT)blend->ySrc, (UINT)blend->cxSrc, (UINT)blend->cySrc,
               (UINT)blend->dwRop, blend->xformSrc.eM11, blend->xformSrc.eM12, blend->xformSrc.eM21,
               blend->xformSrc.eM22, blend->xformSrc.eDx, blend->xformSrc.eDy,
               pfx, (UINT)blend->crBkColorSrc, (UINT)blend->iUsageSrc, (UINT)blend->offBmiSrc,
               (UINT)blend->cbBmiSrc, (UINT)blend->offBitsSrc, (UINT)blend->cbBitsSrc);
        printf("%sBITMAPINFOHEADER biSize %#x biWidth %d biHeight %d biPlanes %d biBitCount %d biCompression %#x\n"
               "%sbiSizeImage %#x biXPelsPerMeter %d biYPelsPerMeter %d biClrUsed %#x biClrImportant %#x\n",
               pfx, (UINT)bmih->biSize, (UINT)bmih->biWidth, (UINT)bmih->biHeight, (UINT)bmih->biPlanes,
               (UINT)bmih->biBitCount, (UINT)bmih->biCompression, pfx, (UINT)bmih->biSizeImage,
               (UINT)bmih->biXPelsPerMeter, (UINT)bmih->biYPelsPerMeter, (UINT)bmih->biClrUsed,
               (UINT)bmih->biClrImportant);
        break;
    }

    EMRCASE(EMR_SETLAYOUT);
    EMRCASE(EMR_TRANSPARENTBLT);
    EMRCASE(EMR_RESERVED_117);
    EMRCASE(EMR_GRADIENTFILL);
    EMRCASE(EMR_SETLINKEDUFI);
    EMRCASE(EMR_SETTEXTJUSTIFICATION);
    EMRCASE(EMR_COLORMATCHTOTARGETW);
    EMRCASE(EMR_CREATECOLORSPACEW);

    default:
        printf("%s%u %08x\n", pfx, type, length);
        break;
    }

    if ( (length < 8) || (length % 4) )
        return 0;

    length -= 8;

    offset += 8;

    for(i=0; i<length; i+=4)
    {
         if (i%16 == 0)
             printf("%s   ", pfx);
         if (!(ptr = PRD(offset, 4))) return 0;
         offset += 4;
         printf("%08x ", read_int(ptr));
         if ( (i % 16 == 12) || (i + 4 == length))
             printf("\n");
    }

    return offset;
}

enum FileSig get_kind_emf(void)
{
    const ENHMETAHEADER*        hdr;

    hdr = PRD(0, sizeof(*hdr));
    if (hdr && hdr->iType == EMR_HEADER && hdr->dSignature == ENHMETA_SIGNATURE)
        return SIG_EMF;
    return SIG_UNKNOWN;
}

void emf_dump(void)
{
    unsigned long offset = 0;
    while ((offset = dump_emfrecord("", offset)));
}
