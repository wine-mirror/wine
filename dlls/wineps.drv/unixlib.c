/*
 * Unix interface for wineps.drv
 *
 * Copyright 1998 Huw D M Davies
 * Copyright 2001 Marcus Meissner
 * Copyright 2023 Piotr Caban for CodeWeavers
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

#if 0
#pragma makedep unix
#endif

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"

#include "psdrv.h"
#include "unixlib.h"
#include "wine/gdi_driver.h"
#include "wine/debug.h"
#include "wine/wingdi16.h"

WINE_DEFAULT_DEBUG_CHANNEL(psdrv);

/* copied from kernelbase */
static int muldiv(int a, int b, int c)
{
    LONGLONG ret;

    if (!c) return -1;

    /* We want to deal with a positive divisor to simplify the logic. */
    if (c < 0)
    {
        a = -a;
        c = -c;
    }

    /* If the result is positive, we "add" to round. else, we subtract to round. */
    if ((a < 0 && b < 0) || (a >= 0 && b >= 0))
        ret = (((LONGLONG)a * b) + (c / 2)) / c;
    else
        ret = (((LONGLONG)a * b) - (c / 2)) / c;

    if (ret > 2147483647 || ret < -2147483647) return -1;
    return ret;
}

static void dump_fields(int fields)
{
    int add_space = 0;

#define CHECK_FIELD(flag) \
do \
{ \
    if (fields & flag) \
    { \
        if (add_space++) TRACE(" "); \
        TRACE(#flag); \
        fields &= ~flag; \
    } \
} \
while (0)

    CHECK_FIELD(DM_ORIENTATION);
    CHECK_FIELD(DM_PAPERSIZE);
    CHECK_FIELD(DM_PAPERLENGTH);
    CHECK_FIELD(DM_PAPERWIDTH);
    CHECK_FIELD(DM_SCALE);
    CHECK_FIELD(DM_POSITION);
    CHECK_FIELD(DM_NUP);
    CHECK_FIELD(DM_DISPLAYORIENTATION);
    CHECK_FIELD(DM_COPIES);
    CHECK_FIELD(DM_DEFAULTSOURCE);
    CHECK_FIELD(DM_PRINTQUALITY);
    CHECK_FIELD(DM_COLOR);
    CHECK_FIELD(DM_DUPLEX);
    CHECK_FIELD(DM_YRESOLUTION);
    CHECK_FIELD(DM_TTOPTION);
    CHECK_FIELD(DM_COLLATE);
    CHECK_FIELD(DM_FORMNAME);
    CHECK_FIELD(DM_LOGPIXELS);
    CHECK_FIELD(DM_BITSPERPEL);
    CHECK_FIELD(DM_PELSWIDTH);
    CHECK_FIELD(DM_PELSHEIGHT);
    CHECK_FIELD(DM_DISPLAYFLAGS);
    CHECK_FIELD(DM_DISPLAYFREQUENCY);
    CHECK_FIELD(DM_ICMMETHOD);
    CHECK_FIELD(DM_ICMINTENT);
    CHECK_FIELD(DM_MEDIATYPE);
    CHECK_FIELD(DM_DITHERTYPE);
    CHECK_FIELD(DM_PANNINGWIDTH);
    CHECK_FIELD(DM_PANNINGHEIGHT);
    if (fields) TRACE(" %#x", fields);
    TRACE("\n");
#undef CHECK_FIELD
}

static void dump_devmode(const DEVMODEW *dm)
{
    if (!TRACE_ON(psdrv)) return;

    TRACE("dmDeviceName: %s\n", debugstr_w(dm->dmDeviceName));
    TRACE("dmSpecVersion: 0x%04x\n", dm->dmSpecVersion);
    TRACE("dmDriverVersion: 0x%04x\n", dm->dmDriverVersion);
    TRACE("dmSize: 0x%04x\n", dm->dmSize);
    TRACE("dmDriverExtra: 0x%04x\n", dm->dmDriverExtra);
    TRACE("dmFields: 0x%04x\n", (int)dm->dmFields);
    dump_fields(dm->dmFields);
    TRACE("dmOrientation: %d\n", dm->dmOrientation);
    TRACE("dmPaperSize: %d\n", dm->dmPaperSize);
    TRACE("dmPaperLength: %d\n", dm->dmPaperLength);
    TRACE("dmPaperWidth: %d\n", dm->dmPaperWidth);
    TRACE("dmScale: %d\n", dm->dmScale);
    TRACE("dmCopies: %d\n", dm->dmCopies);
    TRACE("dmDefaultSource: %d\n", dm->dmDefaultSource);
    TRACE("dmPrintQuality: %d\n", dm->dmPrintQuality);
    TRACE("dmColor: %d\n", dm->dmColor);
    TRACE("dmDuplex: %d\n", dm->dmDuplex);
    TRACE("dmYResolution: %d\n", dm->dmYResolution);
    TRACE("dmTTOption: %d\n", dm->dmTTOption);
    TRACE("dmCollate: %d\n", dm->dmCollate);
    TRACE("dmFormName: %s\n", debugstr_w(dm->dmFormName));
    TRACE("dmLogPixels %u\n", dm->dmLogPixels);
    TRACE("dmBitsPerPel %u\n", (unsigned int)dm->dmBitsPerPel);
    TRACE("dmPelsWidth %u\n", (unsigned int)dm->dmPelsWidth);
    TRACE("dmPelsHeight %u\n", (unsigned int)dm->dmPelsHeight);
}

static INT CDECL get_device_caps(PHYSDEV dev, INT cap)
{
    PSDRV_PDEVICE *pdev = get_psdrv_dev(dev);

    TRACE("%p,%d\n", dev->hdc, cap);

    switch(cap)
    {
    case DRIVERVERSION:
        return 0;
    case TECHNOLOGY:
        return DT_RASPRINTER;
    case HORZSIZE:
        return muldiv(pdev->horzSize, 100, pdev->Devmode->dmPublic.dmScale);
    case VERTSIZE:
        return muldiv(pdev->vertSize, 100, pdev->Devmode->dmPublic.dmScale);
    case HORZRES:
        return pdev->horzRes;
    case VERTRES:
        return pdev->vertRes;
    case BITSPIXEL:
        /* Although Windows returns 1 for monochrome printers, we want
           CreateCompatibleBitmap to provide something other than 1 bpp */
        return 32;
    case NUMPENS:
        return 10;
    case NUMFONTS:
        return 39;
    case NUMCOLORS:
        return -1;
    case PDEVICESIZE:
        return sizeof(PSDRV_PDEVICE);
    case TEXTCAPS:
        return TC_CR_ANY | TC_VA_ABLE; /* psdrv 0x59f7 */
    case RASTERCAPS:
        return (RC_BITBLT | RC_BITMAP64 | RC_GDI20_OUTPUT | RC_DIBTODEV |
                RC_STRETCHBLT | RC_STRETCHDIB); /* psdrv 0x6e99 */
    case ASPECTX:
        return pdev->logPixelsX;
    case ASPECTY:
        return pdev->logPixelsY;
    case LOGPIXELSX:
        return muldiv(pdev->logPixelsX, pdev->Devmode->dmPublic.dmScale, 100);
    case LOGPIXELSY:
        return muldiv(pdev->logPixelsY, pdev->Devmode->dmPublic.dmScale, 100);
    case NUMRESERVED:
        return 0;
    case COLORRES:
        return 0;
    case PHYSICALWIDTH:
        return (pdev->Devmode->dmPublic.dmOrientation == DMORIENT_LANDSCAPE) ?
            pdev->PageSize.cy : pdev->PageSize.cx;
    case PHYSICALHEIGHT:
        return (pdev->Devmode->dmPublic.dmOrientation == DMORIENT_LANDSCAPE) ?
            pdev->PageSize.cx : pdev->PageSize.cy;
    case PHYSICALOFFSETX:
        if (pdev->Devmode->dmPublic.dmOrientation == DMORIENT_LANDSCAPE)
        {
            if (pdev->pi->ppd->LandscapeOrientation == -90)
                return pdev->PageSize.cy - pdev->ImageableArea.top;
            else
                return pdev->ImageableArea.bottom;
        }
        return pdev->ImageableArea.left;

    case PHYSICALOFFSETY:
        if (pdev->Devmode->dmPublic.dmOrientation == DMORIENT_LANDSCAPE)
        {
            if (pdev->pi->ppd->LandscapeOrientation == -90)
                return pdev->PageSize.cx - pdev->ImageableArea.right;
            else
                return pdev->ImageableArea.left;
        }
        return pdev->PageSize.cy - pdev->ImageableArea.top;

    default:
        dev = GET_NEXT_PHYSDEV(dev, pGetDeviceCaps);
        return dev->funcs->pGetDeviceCaps(dev, cap);
    }
}

static inline int paper_size_from_points(float size)
{
    return size * 254 / 72;
}

static INPUTSLOT *unix_find_slot(PPD *ppd, const PSDRV_DEVMODE *dm)
{
    INPUTSLOT *slot;

    LIST_FOR_EACH_ENTRY(slot, &ppd->InputSlots, INPUTSLOT, entry)
        if (slot->WinBin == dm->dmPublic.dmDefaultSource)
            return slot;

    return NULL;
}

static PAGESIZE *unix_find_pagesize(PPD *ppd, const PSDRV_DEVMODE *dm)
{
    PAGESIZE *page;

    LIST_FOR_EACH_ENTRY(page, &ppd->PageSizes, PAGESIZE, entry)
        if (page->WinPage == dm->dmPublic.dmPaperSize)
            return page;

    return NULL;
}

static void merge_devmodes(PSDRV_DEVMODE *dm1, const PSDRV_DEVMODE *dm2, PRINTERINFO *pi)
{
    /* some sanity checks here on dm2 */

    if (dm2->dmPublic.dmFields & DM_ORIENTATION)
    {
        dm1->dmPublic.dmOrientation = dm2->dmPublic.dmOrientation;
        TRACE("Changing orientation to %d (%s)\n",
                dm1->dmPublic.dmOrientation,
                dm1->dmPublic.dmOrientation == DMORIENT_PORTRAIT ?
                "Portrait" :
                (dm1->dmPublic.dmOrientation == DMORIENT_LANDSCAPE ?
                 "Landscape" : "unknown"));
    }

    /* NB PaperWidth is always < PaperLength */
    if (dm2->dmPublic.dmFields & DM_PAPERSIZE)
    {
        PAGESIZE *page = unix_find_pagesize(pi->ppd, dm2);

        if (page)
        {
            dm1->dmPublic.dmPaperSize = dm2->dmPublic.dmPaperSize;
            dm1->dmPublic.dmPaperWidth  = paper_size_from_points(page->PaperDimension->x);
            dm1->dmPublic.dmPaperLength = paper_size_from_points(page->PaperDimension->y);
            dm1->dmPublic.dmFields |= DM_PAPERSIZE | DM_PAPERWIDTH | DM_PAPERLENGTH;
            TRACE("Changing page to %s %d x %d\n", debugstr_w(page->FullName),
                    dm1->dmPublic.dmPaperWidth,
                    dm1->dmPublic.dmPaperLength);

            if (dm1->dmPublic.dmSize >= FIELD_OFFSET(DEVMODEW, dmFormName) + CCHFORMNAME * sizeof(WCHAR))
            {
                lstrcpynW(dm1->dmPublic.dmFormName, page->FullName, CCHFORMNAME);
                dm1->dmPublic.dmFields |= DM_FORMNAME;
            }
        }
        else
            TRACE("Trying to change to unsupported pagesize %d\n", dm2->dmPublic.dmPaperSize);
    }

    else if ((dm2->dmPublic.dmFields & DM_PAPERLENGTH) &&
            (dm2->dmPublic.dmFields & DM_PAPERWIDTH))
    {
        dm1->dmPublic.dmPaperLength = dm2->dmPublic.dmPaperLength;
        dm1->dmPublic.dmPaperWidth = dm2->dmPublic.dmPaperWidth;
        TRACE("Changing PaperLength|Width to %dx%d\n",
                dm2->dmPublic.dmPaperLength,
                dm2->dmPublic.dmPaperWidth);
        dm1->dmPublic.dmFields &= ~DM_PAPERSIZE;
        dm1->dmPublic.dmFields |= (DM_PAPERLENGTH | DM_PAPERWIDTH);
    }
    else if (dm2->dmPublic.dmFields & (DM_PAPERLENGTH | DM_PAPERWIDTH))
    {
        /* You might think that this would be allowed if dm1 is in custom size
           mode, but apparently Windows reverts to standard paper mode even in
           this case */
        FIXME("Trying to change only paperlength or paperwidth\n");
        dm1->dmPublic.dmFields &= ~(DM_PAPERLENGTH | DM_PAPERWIDTH);
        dm1->dmPublic.dmFields |= DM_PAPERSIZE;
    }

    if (dm2->dmPublic.dmFields & DM_SCALE)
    {
        dm1->dmPublic.dmScale = dm2->dmPublic.dmScale;
        TRACE("Changing Scale to %d\n", dm2->dmPublic.dmScale);
    }

    if (dm2->dmPublic.dmFields & DM_COPIES)
    {
        dm1->dmPublic.dmCopies = dm2->dmPublic.dmCopies;
        TRACE("Changing Copies to %d\n", dm2->dmPublic.dmCopies);
    }

    if (dm2->dmPublic.dmFields & DM_DEFAULTSOURCE)
    {
        INPUTSLOT *slot = unix_find_slot(pi->ppd, dm2);

        if (slot)
        {
            dm1->dmPublic.dmDefaultSource = dm2->dmPublic.dmDefaultSource;
            TRACE("Changing bin to '%s'\n", slot->FullName);
        }
        else
        {
            TRACE("Trying to change to unsupported bin %d\n", dm2->dmPublic.dmDefaultSource);
        }
    }

    if (dm2->dmPublic.dmFields & DM_DEFAULTSOURCE)
        dm1->dmPublic.dmDefaultSource = dm2->dmPublic.dmDefaultSource;
    if (dm2->dmPublic.dmFields & DM_PRINTQUALITY)
        dm1->dmPublic.dmPrintQuality = dm2->dmPublic.dmPrintQuality;
    if (dm2->dmPublic.dmFields & DM_COLOR)
        dm1->dmPublic.dmColor = dm2->dmPublic.dmColor;
    if (dm2->dmPublic.dmFields & DM_DUPLEX && pi->ppd->DefaultDuplex && pi->ppd->DefaultDuplex->WinDuplex != 0)
        dm1->dmPublic.dmDuplex = dm2->dmPublic.dmDuplex;
    if (dm2->dmPublic.dmFields & DM_YRESOLUTION)
        dm1->dmPublic.dmYResolution = dm2->dmPublic.dmYResolution;
    if (dm2->dmPublic.dmFields & DM_TTOPTION)
        dm1->dmPublic.dmTTOption = dm2->dmPublic.dmTTOption;
    if (dm2->dmPublic.dmFields & DM_COLLATE)
        dm1->dmPublic.dmCollate = dm2->dmPublic.dmCollate;
    if (dm2->dmPublic.dmFields & DM_FORMNAME)
        lstrcpynW(dm1->dmPublic.dmFormName, dm2->dmPublic.dmFormName, CCHFORMNAME);
    if (dm2->dmPublic.dmFields & DM_BITSPERPEL)
        dm1->dmPublic.dmBitsPerPel = dm2->dmPublic.dmBitsPerPel;
    if (dm2->dmPublic.dmFields & DM_PELSWIDTH)
        dm1->dmPublic.dmPelsWidth = dm2->dmPublic.dmPelsWidth;
    if (dm2->dmPublic.dmFields & DM_PELSHEIGHT)
        dm1->dmPublic.dmPelsHeight = dm2->dmPublic.dmPelsHeight;
    if (dm2->dmPublic.dmFields & DM_DISPLAYFLAGS)
        dm1->dmPublic.dmDisplayFlags = dm2->dmPublic.dmDisplayFlags;
    if (dm2->dmPublic.dmFields & DM_DISPLAYFREQUENCY)
        dm1->dmPublic.dmDisplayFrequency = dm2->dmPublic.dmDisplayFrequency;
    if (dm2->dmPublic.dmFields & DM_POSITION)
        dm1->dmPublic.dmPosition = dm2->dmPublic.dmPosition;
    if (dm2->dmPublic.dmFields & DM_LOGPIXELS)
        dm1->dmPublic.dmLogPixels = dm2->dmPublic.dmLogPixels;
    if (dm2->dmPublic.dmFields & DM_ICMMETHOD)
        dm1->dmPublic.dmICMMethod = dm2->dmPublic.dmICMMethod;
    if (dm2->dmPublic.dmFields & DM_ICMINTENT)
        dm1->dmPublic.dmICMIntent = dm2->dmPublic.dmICMIntent;
    if (dm2->dmPublic.dmFields & DM_MEDIATYPE)
        dm1->dmPublic.dmMediaType = dm2->dmPublic.dmMediaType;
    if (dm2->dmPublic.dmFields & DM_DITHERTYPE)
        dm1->dmPublic.dmDitherType = dm2->dmPublic.dmDitherType;
    if (dm2->dmPublic.dmFields & DM_PANNINGWIDTH)
        dm1->dmPublic.dmPanningWidth = dm2->dmPublic.dmPanningWidth;
    if (dm2->dmPublic.dmFields & DM_PANNINGHEIGHT)
        dm1->dmPublic.dmPanningHeight = dm2->dmPublic.dmPanningHeight;
}

static void update_dev_caps(PSDRV_PDEVICE *pdev)
{
    INT width = 0, height = 0, resx = 0, resy = 0;
    RESOLUTION *res;
    PAGESIZE *page;

    dump_devmode(&pdev->Devmode->dmPublic);

    if (pdev->Devmode->dmPublic.dmFields & (DM_PRINTQUALITY | DM_YRESOLUTION | DM_LOGPIXELS))
    {
        if (pdev->Devmode->dmPublic.dmFields & DM_PRINTQUALITY)
            resx = resy = pdev->Devmode->dmPublic.dmPrintQuality;

        if (pdev->Devmode->dmPublic.dmFields & DM_YRESOLUTION)
            resy = pdev->Devmode->dmPublic.dmYResolution;

        if (pdev->Devmode->dmPublic.dmFields & DM_LOGPIXELS)
            resx = resy = pdev->Devmode->dmPublic.dmLogPixels;

        LIST_FOR_EACH_ENTRY(res, &pdev->pi->ppd->Resolutions, RESOLUTION, entry)
        {
            if (res->resx == resx && res->resy == resy)
            {
                pdev->logPixelsX = resx;
                pdev->logPixelsY = resy;
                break;
            }
        }

        if (&res->entry == &pdev->pi->ppd->Resolutions)
        {
            WARN("Requested resolution %dx%d is not supported by device\n", resx, resy);
            pdev->logPixelsX = pdev->pi->ppd->DefaultResolution;
            pdev->logPixelsY = pdev->logPixelsX;
        }
    }
    else
    {
        WARN("Using default device resolution %d\n", pdev->pi->ppd->DefaultResolution);
        pdev->logPixelsX = pdev->pi->ppd->DefaultResolution;
        pdev->logPixelsY = pdev->logPixelsX;
    }

    if (pdev->Devmode->dmPublic.dmFields & DM_PAPERSIZE) {
        LIST_FOR_EACH_ENTRY(page, &pdev->pi->ppd->PageSizes, PAGESIZE, entry) {
            if (page->WinPage == pdev->Devmode->dmPublic.dmPaperSize)
                break;
        }

        if (&page->entry == &pdev->pi->ppd->PageSizes) {
            FIXME("Can't find page\n");
            SetRectEmpty(&pdev->ImageableArea);
            pdev->PageSize.cx = 0;
            pdev->PageSize.cy = 0;
        } else if (page->ImageableArea) {
            /* pdev sizes in device units; ppd sizes in 1/72" */
            SetRect(&pdev->ImageableArea, page->ImageableArea->llx * pdev->logPixelsX / 72,
                    page->ImageableArea->ury * pdev->logPixelsY / 72,
                    page->ImageableArea->urx * pdev->logPixelsX / 72,
                    page->ImageableArea->lly * pdev->logPixelsY / 72);
            pdev->PageSize.cx = page->PaperDimension->x *
                pdev->logPixelsX / 72;
            pdev->PageSize.cy = page->PaperDimension->y *
                pdev->logPixelsY / 72;
        } else {
            pdev->ImageableArea.left = pdev->ImageableArea.bottom = 0;
            pdev->ImageableArea.right = pdev->PageSize.cx =
                page->PaperDimension->x * pdev->logPixelsX / 72;
            pdev->ImageableArea.top = pdev->PageSize.cy =
                page->PaperDimension->y * pdev->logPixelsY / 72;
        }
    } else if ((pdev->Devmode->dmPublic.dmFields & DM_PAPERLENGTH) &&
            (pdev->Devmode->dmPublic.dmFields & DM_PAPERWIDTH)) {
        /* pdev sizes in device units; Devmode sizes in 1/10 mm */
        pdev->ImageableArea.left = pdev->ImageableArea.bottom = 0;
        pdev->ImageableArea.right = pdev->PageSize.cx =
            pdev->Devmode->dmPublic.dmPaperWidth * pdev->logPixelsX / 254;
        pdev->ImageableArea.top = pdev->PageSize.cy =
            pdev->Devmode->dmPublic.dmPaperLength * pdev->logPixelsY / 254;
    } else {
        FIXME("Odd dmFields %x\n", (int)pdev->Devmode->dmPublic.dmFields);
        SetRectEmpty(&pdev->ImageableArea);
        pdev->PageSize.cx = 0;
        pdev->PageSize.cy = 0;
    }

    TRACE("ImageableArea = %s: PageSize = %dx%d\n", wine_dbgstr_rect(&pdev->ImageableArea),
            (int)pdev->PageSize.cx, (int)pdev->PageSize.cy);

    /* these are in device units */
    width = pdev->ImageableArea.right - pdev->ImageableArea.left;
    height = pdev->ImageableArea.top - pdev->ImageableArea.bottom;

    if (pdev->Devmode->dmPublic.dmOrientation == DMORIENT_PORTRAIT) {
        pdev->horzRes = width;
        pdev->vertRes = height;
    } else {
        pdev->horzRes = height;
        pdev->vertRes = width;
    }

    /* these are in mm */
    pdev->horzSize = (pdev->horzRes * 25.4) / pdev->logPixelsX;
    pdev->vertSize = (pdev->vertRes * 25.4) / pdev->logPixelsY;

    TRACE("devcaps: horzSize = %dmm, vertSize = %dmm, "
            "horzRes = %d, vertRes = %d\n",
            pdev->horzSize, pdev->vertSize,
            pdev->horzRes, pdev->vertRes);
}

static BOOL CDECL reset_dc(PHYSDEV dev, const DEVMODEW *devmode)
{
    PSDRV_PDEVICE *pdev = get_psdrv_dev(dev);

    if (devmode)
    {
        merge_devmodes(pdev->Devmode, (const PSDRV_DEVMODE *)devmode, pdev->pi);
        update_dev_caps(pdev);
    }
    return TRUE;
}

static int CDECL ext_escape(PHYSDEV dev, int escape, int input_size, const void *input,
        int output_size, void *output)
{
    TRACE("%p,%d,%d,%p,%d,%p\n",
          dev->hdc, escape, input_size, input, output_size, output);

    switch (escape)
    {
    case QUERYESCSUPPORT:
        if (input_size < sizeof(SHORT))
        {
            WARN("input_size < sizeof(SHORT) (=%d) for QUERYESCSUPPORT\n", input_size);
            return 0;
        }
        else
        {
            DWORD num = (input_size < sizeof(DWORD)) ? *(const USHORT *)input : *(const DWORD *)input;
            TRACE("QUERYESCSUPPORT for %d\n", (int)num);

            switch (num)
            {
            case NEXTBAND:
            /*case BANDINFO:*/
            case SETCOPYCOUNT:
            case GETTECHNOLOGY:
            case SETLINECAP:
            case SETLINEJOIN:
            case SETMITERLIMIT:
            case SETCHARSET:
            case EXT_DEVICE_CAPS:
            case SET_BOUNDS:
            case EPSPRINTING:
            case POSTSCRIPT_DATA:
            case PASSTHROUGH:
            case POSTSCRIPT_PASSTHROUGH:
            case POSTSCRIPT_IGNORE:
            case BEGIN_PATH:
            case CLIP_TO_PATH:
            case END_PATH:
            /*case DRAWPATTERNRECT:*/

            /* PageMaker checks for it */
            case DOWNLOADHEADER:

            /* PageMaker doesn't check for DOWNLOADFACE and GETFACENAME but
             * uses them, they are supposed to be supported by any PS printer.
             */
            case DOWNLOADFACE:

            /* PageMaker checks for these as a part of process of detecting
             * a "fully compatible" PS printer, but doesn't actually use them.
             */
            case OPENCHANNEL:
            case CLOSECHANNEL:
                return TRUE;

            /* Windows PS driver reports 0, but still supports this escape */
            case GETFACENAME:
                return FALSE; /* suppress the FIXME below */

            default:
                FIXME("QUERYESCSUPPORT(%d) - not supported.\n", (int)num);
                return FALSE;
            }
        }

    case OPENCHANNEL:
        FIXME("OPENCHANNEL: stub\n");
        return 1;

    case CLOSECHANNEL:
        FIXME("CLOSECHANNEL: stub\n");
        return 1;

    case DOWNLOADHEADER:
        FIXME("DOWNLOADHEADER: stub\n");
        /* should return name of the downloaded procset */
        *(char *)output = 0;
        return 1;

    case GETFACENAME:
        FIXME("GETFACENAME: stub\n");
        lstrcpynA(output, "Courier", output_size);
        return 1;

    case DOWNLOADFACE:
        FIXME("DOWNLOADFACE: stub\n");
        return 1;

    case MFCOMMENT:
    {
        FIXME("MFCOMMENT(%p, %d)\n", input, input_size);
        return 1;
    }
    case DRAWPATTERNRECT:
    {
        DRAWPATRECT     *dpr = (DRAWPATRECT*)input;

        FIXME("DRAWPATTERNRECT(pos (%d,%d), size %dx%d, style %d, pattern %x), stub!\n",
                (int)dpr->ptPosition.x, (int)dpr->ptPosition.y,
                (int)dpr->ptSize.x, (int)dpr->ptSize.y,
                dpr->wStyle, dpr->wPattern);
        return 1;
    }
    case BANDINFO:
    {
        BANDINFOSTRUCT  *ibi = (BANDINFOSTRUCT*)input;
        BANDINFOSTRUCT  *obi = (BANDINFOSTRUCT*)output;

        FIXME("BANDINFO(graphics %d, text %d, rect %s), stub!\n", ibi->GraphicsFlag,
                ibi->TextFlag, wine_dbgstr_rect(&ibi->GraphicsRect));
        *obi = *ibi;
        return 1;
    }

    case SETCOPYCOUNT:
    {
        const INT *NumCopies = input;
        INT *ActualCopies = output;
        if (input_size != sizeof(INT))
        {
            WARN("input_size != sizeof(INT) (=%d) for SETCOPYCOUNT\n", input_size);
            return 0;
        }
        TRACE("SETCOPYCOUNT %d\n", *NumCopies);
        *ActualCopies = 1;
        return 1;
    }

    case GETTECHNOLOGY:
    {
        LPSTR p = output;
        strcpy(p, "PostScript");
        *(p + strlen(p) + 1) = '\0'; /* 2 '\0's at end of string */
        return 1;
    }

    case SETLINECAP:
    {
        INT newCap = *(const INT *)input;
        if (input_size != sizeof(INT))
        {
            WARN("input_size != sizeof(INT) (=%d) for SETLINECAP\n", input_size);
            return 0;
        }
        TRACE("SETLINECAP %d\n", newCap);
        return 0;
    }

    case SETLINEJOIN:
    {
        INT newJoin = *(const INT *)input;
        if (input_size != sizeof(INT))
        {
            WARN("input_size != sizeof(INT) (=%d) for SETLINEJOIN\n", input_size);
            return 0;
        }
        TRACE("SETLINEJOIN %d\n", newJoin);
        return 0;
    }

    case SETMITERLIMIT:
    {
        INT newLimit = *(const INT *)input;
        if (input_size != sizeof(INT))
        {
            WARN("input_size != sizeof(INT) (=%d) for SETMITERLIMIT\n", input_size);
            return 0;
        }
        TRACE("SETMITERLIMIT %d\n", newLimit);
        return 0;
    }

    case SETCHARSET:
    /* Undocumented escape used by winword6.
       Switches between ANSI and a special charset.
       If *lpInData == 1 we require that
       0x91 is quoteleft
       0x92 is quoteright
       0x93 is quotedblleft
       0x94 is quotedblright
       0x95 is bullet
       0x96 is endash
       0x97 is emdash
       0xa0 is non break space - yeah right.

       If *lpInData == 0 we get ANSI.
       Since there's nothing else there, let's just make these the default
       anyway and see what happens...
       */
    return 1;

    case EXT_DEVICE_CAPS:
    {
        UINT cap = *(const UINT *)input;
        if (input_size != sizeof(UINT))
        {
            WARN("input_size != sizeof(UINT) (=%d) for EXT_DEVICE_CAPS\n", input_size);
            return 0;
        }
        TRACE("EXT_DEVICE_CAPS %d\n", cap);
        return 0;
    }

    case SET_BOUNDS:
    {
        const RECT *r = input;
        if (input_size != sizeof(RECT))
        {
            WARN("input_size != sizeof(RECT) (=%d) for SET_BOUNDS\n", input_size);
            return 0;
        }
        TRACE("SET_BOUNDS %s\n", wine_dbgstr_rect(r));
        return 0;
    }

    case EPSPRINTING:
    {
        UINT epsprint = *(const UINT*)input;
        /* FIXME: In this mode we do not need to send page intros and page
         * ends according to the doc. But I just ignore that detail
         * for now.
         */
        TRACE("EPS Printing support %sable.\n",epsprint?"en":"dis");
        return 1;
    }

    case POSTSCRIPT_DATA:
    case PASSTHROUGH:
    case POSTSCRIPT_PASSTHROUGH:
        return 1;

    case POSTSCRIPT_IGNORE:
        return 1;

    case GETSETPRINTORIENT:
    {
        /* If lpInData is present, it is a 20 byte structure, first 32
         * bit LONG value is the orientation. if lpInData is NULL, it
         * returns the current orientation.
         */
        FIXME("GETSETPRINTORIENT not implemented (data %p)!\n",input);
        return 1;
    }
    case BEGIN_PATH:
        return 1;

    case END_PATH:
        return 1;

    case CLIP_TO_PATH:
        return 1;
    default:
        FIXME("Unimplemented code %d\n", escape);
        return 0;
    }
}

static NTSTATUS init_dc(void *arg)
{
    struct init_dc_params *params = arg;

    params->funcs->pGetDeviceCaps = get_device_caps;
    params->funcs->pResetDC = reset_dc;
    params->funcs->pExtEscape = ext_escape;
    return TRUE;
}

const unixlib_entry_t __wine_unix_call_funcs[] =
{
    init_dc,
};

C_ASSERT(ARRAYSIZE(__wine_unix_call_funcs) == unix_funcs_count);
