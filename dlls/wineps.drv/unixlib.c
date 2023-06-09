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
#include <stdlib.h>
#include <math.h>

#include "windef.h"
#include "winbase.h"

#include "ntf.h"
#include "unixlib.h"
#include "ntgdi.h"
#include "ddk/winddi.h"
#include "wine/gdi_driver.h"
#include "wine/debug.h"
#include "wine/wingdi16.h"

WINE_DEFAULT_DEBUG_CHANNEL(psdrv);

static const WCHAR timesW[] = {'T','i','m','e','s',0};
static const WCHAR helveticaW[] = {'H','e','l','v','e','t','i','c','a',0};
static const WCHAR courierW[] = {'C','o','u','r','i','e','r',0};
static const WCHAR symbolW[] = {'S','y','m','b','o','l',0};
static const WCHAR arialW[] = {'A','r','i','a','l',0};
static const WCHAR systemW[] = {'S','y','s','t','e','m',0};
static const WCHAR times_new_romanW[] = {'T','i','m','e','s',' ','N','e','w',' ','R','o','m','a','n',0};
static const WCHAR courier_newW[] = {'C','o','u','r','i','e','r',' ','N','e','w',0};

static const struct gdi_dc_funcs psdrv_funcs;

struct glyph_info
{
    WCHAR wch;
    int width;
};

struct font_data
{
    struct list entry;

    char *name;
    IFIMETRICS *metrics;
    int glyph_count;
    struct glyph_info *glyphs;
    struct glyph_info def_glyph;
};

static struct list fonts = LIST_INIT(fonts);

struct printer_info
{
    struct list entry;
    const WCHAR *name;
    const PSDRV_DEVMODE *devmode;
};

static struct list printer_info_list = LIST_INIT(printer_info_list);

struct band_info
{
    BOOL graphics_flag;
    BOOL text_flag;
    RECT graphics_rect;
};

typedef struct
{
    struct gdi_physdev dev;
    PSDRV_DEVMODE *devmode;
    const struct printer_info *pi;

    /* builtin font info */
    BOOL builtin;
    SIZE size;
    const struct font_data *font;
    float scale;
    TEXTMETRICW tm;
    int escapement;

    SIZE page_size; /* Physical page size in device units */
    RECT imageable_area; /* Imageable area in device units */
    int horz_res; /* device caps */
    int vert_res;
    int horz_size;
    int vert_size;
    int log_pixels_x;
    int log_pixels_y;
} PSDRV_PDEVICE;

static inline PSDRV_PDEVICE *get_psdrv_dev(PHYSDEV dev)
{
    return (PSDRV_PDEVICE *)dev;
}

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

static INT get_device_caps(PHYSDEV dev, INT cap)
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
        return muldiv(pdev->horz_size, 100, pdev->devmode->dmPublic.dmScale);
    case VERTSIZE:
        return muldiv(pdev->vert_size, 100, pdev->devmode->dmPublic.dmScale);
    case HORZRES:
        return pdev->horz_res;
    case VERTRES:
        return pdev->vert_res;
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
        return pdev->log_pixels_x;
    case ASPECTY:
        return pdev->log_pixels_y;
    case LOGPIXELSX:
        return muldiv(pdev->log_pixels_x, pdev->devmode->dmPublic.dmScale, 100);
    case LOGPIXELSY:
        return muldiv(pdev->log_pixels_y, pdev->devmode->dmPublic.dmScale, 100);
    case NUMRESERVED:
        return 0;
    case COLORRES:
        return 0;
    case PHYSICALWIDTH:
        return (pdev->devmode->dmPublic.dmOrientation == DMORIENT_LANDSCAPE) ?
            pdev->page_size.cy : pdev->page_size.cx;
    case PHYSICALHEIGHT:
        return (pdev->devmode->dmPublic.dmOrientation == DMORIENT_LANDSCAPE) ?
            pdev->page_size.cx : pdev->page_size.cy;
    case PHYSICALOFFSETX:
        if (pdev->devmode->dmPublic.dmOrientation == DMORIENT_LANDSCAPE)
        {
            if (pdev->devmode->landscape_orientation == -90)
                return pdev->page_size.cy - pdev->imageable_area.top;
            else
                return pdev->imageable_area.bottom;
        }
        return pdev->imageable_area.left;

    case PHYSICALOFFSETY:
        if (pdev->devmode->dmPublic.dmOrientation == DMORIENT_LANDSCAPE)
        {
            if (pdev->devmode->landscape_orientation == -90)
                return pdev->page_size.cx - pdev->imageable_area.right;
            else
                return pdev->imageable_area.left;
        }
        return pdev->page_size.cy - pdev->imageable_area.top;

    default:
        dev = GET_NEXT_PHYSDEV(dev, pGetDeviceCaps);
        return dev->funcs->pGetDeviceCaps(dev, cap);
    }
}

static inline int paper_size_from_points(float size)
{
    return size * 254 / 72;
}

static const struct input_slot *find_slot(const struct printer_info *pi,
        const DEVMODEW *dm)
{
    const struct input_slot *slot = (const struct input_slot *)pi->devmode->data;
    int i;

    for (i = 0; i < pi->devmode->input_slots; i++)
    {
        if (slot[i].win_bin == dm->dmDefaultSource)
            return slot + i;
    }
    return NULL;
}

static const struct page_size *find_pagesize(const struct printer_info *pi,
        const DEVMODEW *dm)
{
    const struct page_size *page;
    int i;

    page = (const struct page_size *)(pi->devmode->data +
            pi->devmode->input_slots * sizeof(struct input_slot) +
            pi->devmode->resolutions * sizeof(struct resolution));
    for (i = 0; i < pi->devmode->page_sizes; i++)
    {
        if (page[i].win_page == dm->dmPaperSize)
            return page + i;
    }
    return NULL;
}

static void merge_devmodes(PSDRV_DEVMODE *dm1, const DEVMODEW *dm2,
        const struct printer_info *pi)
{
    /* some sanity checks here on dm2 */

    if (dm2->dmFields & DM_ORIENTATION)
    {
        dm1->dmPublic.dmOrientation = dm2->dmOrientation;
        TRACE("Changing orientation to %d (%s)\n",
                dm1->dmPublic.dmOrientation,
                dm1->dmPublic.dmOrientation == DMORIENT_PORTRAIT ?
                "Portrait" :
                (dm1->dmPublic.dmOrientation == DMORIENT_LANDSCAPE ?
                 "Landscape" : "unknown"));
    }

    /* NB PaperWidth is always < PaperLength */
    if (dm2->dmFields & DM_PAPERSIZE)
    {
        const struct page_size *page = find_pagesize(pi, dm2);

        if (page)
        {
            dm1->dmPublic.dmPaperSize = dm2->dmPaperSize;
            dm1->dmPublic.dmPaperWidth  = paper_size_from_points(page->paper_dimension.x);
            dm1->dmPublic.dmPaperLength = paper_size_from_points(page->paper_dimension.y);
            dm1->dmPublic.dmFields |= DM_PAPERSIZE | DM_PAPERWIDTH | DM_PAPERLENGTH;
            TRACE("Changing page to %s %d x %d\n", debugstr_w(page->name),
                    dm1->dmPublic.dmPaperWidth,
                    dm1->dmPublic.dmPaperLength);

            if (dm1->dmPublic.dmSize >= FIELD_OFFSET(DEVMODEW, dmFormName) + CCHFORMNAME * sizeof(WCHAR))
            {
                memcpy(dm1->dmPublic.dmFormName, page->name, sizeof(page->name));
                dm1->dmPublic.dmFields |= DM_FORMNAME;
            }
        }
        else
            TRACE("Trying to change to unsupported pagesize %d\n", dm2->dmPaperSize);
    }

    else if ((dm2->dmFields & DM_PAPERLENGTH) &&
            (dm2->dmFields & DM_PAPERWIDTH))
    {
        dm1->dmPublic.dmPaperLength = dm2->dmPaperLength;
        dm1->dmPublic.dmPaperWidth = dm2->dmPaperWidth;
        TRACE("Changing PaperLength|Width to %dx%d\n",
                dm2->dmPaperLength,
                dm2->dmPaperWidth);
        dm1->dmPublic.dmFields &= ~DM_PAPERSIZE;
        dm1->dmPublic.dmFields |= (DM_PAPERLENGTH | DM_PAPERWIDTH);
    }
    else if (dm2->dmFields & (DM_PAPERLENGTH | DM_PAPERWIDTH))
    {
        /* You might think that this would be allowed if dm1 is in custom size
           mode, but apparently Windows reverts to standard paper mode even in
           this case */
        FIXME("Trying to change only paperlength or paperwidth\n");
        dm1->dmPublic.dmFields &= ~(DM_PAPERLENGTH | DM_PAPERWIDTH);
        dm1->dmPublic.dmFields |= DM_PAPERSIZE;
    }

    if (dm2->dmFields & DM_SCALE)
    {
        dm1->dmPublic.dmScale = dm2->dmScale;
        TRACE("Changing Scale to %d\n", dm2->dmScale);
    }

    if (dm2->dmFields & DM_COPIES)
    {
        dm1->dmPublic.dmCopies = dm2->dmCopies;
        TRACE("Changing Copies to %d\n", dm2->dmCopies);
    }

    if (dm2->dmFields & DM_DEFAULTSOURCE)
    {
        const struct input_slot *slot = find_slot(pi, dm2);

        if (slot)
            dm1->dmPublic.dmDefaultSource = dm2->dmDefaultSource;
        else
            TRACE("Trying to change to unsupported bin %d\n", dm2->dmDefaultSource);
    }

    if (dm2->dmFields & DM_DEFAULTSOURCE)
        dm1->dmPublic.dmDefaultSource = dm2->dmDefaultSource;
    if (dm2->dmFields & DM_PRINTQUALITY)
        dm1->dmPublic.dmPrintQuality = dm2->dmPrintQuality;
    if (dm2->dmFields & DM_COLOR)
        dm1->dmPublic.dmColor = dm2->dmColor;
    if (dm2->dmFields & DM_DUPLEX && pi->devmode->duplex)
        dm1->dmPublic.dmDuplex = dm2->dmDuplex;
    if (dm2->dmFields & DM_YRESOLUTION)
        dm1->dmPublic.dmYResolution = dm2->dmYResolution;
    if (dm2->dmFields & DM_TTOPTION)
        dm1->dmPublic.dmTTOption = dm2->dmTTOption;
    if (dm2->dmFields & DM_COLLATE)
        dm1->dmPublic.dmCollate = dm2->dmCollate;
    if (dm2->dmFields & DM_FORMNAME)
        lstrcpynW(dm1->dmPublic.dmFormName, dm2->dmFormName, CCHFORMNAME);
    if (dm2->dmFields & DM_BITSPERPEL)
        dm1->dmPublic.dmBitsPerPel = dm2->dmBitsPerPel;
    if (dm2->dmFields & DM_PELSWIDTH)
        dm1->dmPublic.dmPelsWidth = dm2->dmPelsWidth;
    if (dm2->dmFields & DM_PELSHEIGHT)
        dm1->dmPublic.dmPelsHeight = dm2->dmPelsHeight;
    if (dm2->dmFields & DM_DISPLAYFLAGS)
        dm1->dmPublic.dmDisplayFlags = dm2->dmDisplayFlags;
    if (dm2->dmFields & DM_DISPLAYFREQUENCY)
        dm1->dmPublic.dmDisplayFrequency = dm2->dmDisplayFrequency;
    if (dm2->dmFields & DM_POSITION)
        dm1->dmPublic.dmPosition = dm2->dmPosition;
    if (dm2->dmFields & DM_LOGPIXELS)
        dm1->dmPublic.dmLogPixels = dm2->dmLogPixels;
    if (dm2->dmFields & DM_ICMMETHOD)
        dm1->dmPublic.dmICMMethod = dm2->dmICMMethod;
    if (dm2->dmFields & DM_ICMINTENT)
        dm1->dmPublic.dmICMIntent = dm2->dmICMIntent;
    if (dm2->dmFields & DM_MEDIATYPE)
        dm1->dmPublic.dmMediaType = dm2->dmMediaType;
    if (dm2->dmFields & DM_DITHERTYPE)
        dm1->dmPublic.dmDitherType = dm2->dmDitherType;
    if (dm2->dmFields & DM_PANNINGWIDTH)
        dm1->dmPublic.dmPanningWidth = dm2->dmPanningWidth;
    if (dm2->dmFields & DM_PANNINGHEIGHT)
        dm1->dmPublic.dmPanningHeight = dm2->dmPanningHeight;
}

static void update_dev_caps(PSDRV_PDEVICE *pdev)
{
    INT width = 0, height = 0, resx = 0, resy = 0;
    const struct resolution *res;
    const struct page_size *page;
    int i;

    dump_devmode(&pdev->devmode->dmPublic);

    if (pdev->devmode->dmPublic.dmFields & (DM_PRINTQUALITY | DM_YRESOLUTION | DM_LOGPIXELS))
    {
        if (pdev->devmode->dmPublic.dmFields & DM_PRINTQUALITY)
            resx = resy = pdev->devmode->dmPublic.dmPrintQuality;

        if (pdev->devmode->dmPublic.dmFields & DM_YRESOLUTION)
            resy = pdev->devmode->dmPublic.dmYResolution;

        if (pdev->devmode->dmPublic.dmFields & DM_LOGPIXELS)
            resx = resy = pdev->devmode->dmPublic.dmLogPixels;

        res = (const struct resolution *)(pdev->devmode->data
                + pdev->devmode->input_slots * sizeof(struct input_slot));
        for (i = 0; i < pdev->devmode->resolutions; i++)
        {
            if (res[i].x == resx && res[i].y == resy)
            {
                pdev->log_pixels_x = resx;
                pdev->log_pixels_y = resy;
                break;
            }
        }

        if (i == pdev->devmode->resolutions)
        {
            WARN("Requested resolution %dx%d is not supported by device\n", resx, resy);
            pdev->log_pixels_x = pdev->devmode->default_resolution;
            pdev->log_pixels_y = pdev->log_pixels_x;
        }
    }
    else
    {
        WARN("Using default device resolution %d\n", pdev->devmode->default_resolution);
        pdev->log_pixels_x = pdev->devmode->default_resolution;
        pdev->log_pixels_y = pdev->log_pixels_x;
    }

    if (pdev->devmode->dmPublic.dmFields & DM_PAPERSIZE) {
        page = find_pagesize(pdev->pi, &pdev->devmode->dmPublic);

        if (!page)
        {
            FIXME("Can't find page\n");
            SetRectEmpty(&pdev->imageable_area);
            pdev->page_size.cx = 0;
            pdev->page_size.cy = 0;
        }
        else
        {
            /* pdev sizes in device units; ppd sizes in 1/72" */
            SetRect(&pdev->imageable_area,
                    page->imageable_area.left * pdev->log_pixels_x / 72,
                    page->imageable_area.top * pdev->log_pixels_y / 72,
                    page->imageable_area.right * pdev->log_pixels_x / 72,
                    page->imageable_area.bottom * pdev->log_pixels_y / 72);
            pdev->page_size.cx = page->paper_dimension.x * pdev->log_pixels_x / 72;
            pdev->page_size.cy = page->paper_dimension.y * pdev->log_pixels_y / 72;
        }
    } else if ((pdev->devmode->dmPublic.dmFields & DM_PAPERLENGTH) &&
            (pdev->devmode->dmPublic.dmFields & DM_PAPERWIDTH)) {
        /* pdev sizes in device units; devmode sizes in 1/10 mm */
        pdev->imageable_area.left = pdev->imageable_area.bottom = 0;
        pdev->imageable_area.right = pdev->page_size.cx =
            pdev->devmode->dmPublic.dmPaperWidth * pdev->log_pixels_x / 254;
        pdev->imageable_area.top = pdev->page_size.cy =
            pdev->devmode->dmPublic.dmPaperLength * pdev->log_pixels_y / 254;
    } else {
        FIXME("Odd dmFields %x\n", (int)pdev->devmode->dmPublic.dmFields);
        SetRectEmpty(&pdev->imageable_area);
        pdev->page_size.cx = 0;
        pdev->page_size.cy = 0;
    }

    TRACE("imageable_area = %s: page_size = %dx%d\n", wine_dbgstr_rect(&pdev->imageable_area),
            (int)pdev->page_size.cx, (int)pdev->page_size.cy);

    /* these are in device units */
    width = pdev->imageable_area.right - pdev->imageable_area.left;
    height = pdev->imageable_area.top - pdev->imageable_area.bottom;

    if (pdev->devmode->dmPublic.dmOrientation == DMORIENT_PORTRAIT) {
        pdev->horz_res = width;
        pdev->vert_res = height;
    } else {
        pdev->horz_res = height;
        pdev->vert_res = width;
    }

    /* these are in mm */
    pdev->horz_size = (pdev->horz_res * 25.4) / pdev->log_pixels_x;
    pdev->vert_size = (pdev->vert_res * 25.4) / pdev->log_pixels_y;

    TRACE("devcaps: horz_size = %dmm, vert_size = %dmm, "
            "horz_res = %d, vert_res = %d\n",
            pdev->horz_size, pdev->vert_size,
            pdev->horz_res, pdev->vert_res);
}

static BOOL reset_dc(PHYSDEV dev, const DEVMODEW *devmode)
{
    PSDRV_PDEVICE *pdev = get_psdrv_dev(dev);

    if (devmode)
    {
        merge_devmodes(pdev->devmode, devmode, pdev->pi);
        update_dev_caps(pdev);
    }
    return TRUE;
}

static int cmp_glyph_info(const void *a, const void *b)
{
    return (int)((const struct glyph_info *)a)->wch -
        (int)((const struct glyph_info *)b)->wch;
}

const struct glyph_info *uv_metrics(WCHAR wch, const struct font_data *font)
{
    const struct glyph_info *needle;
    struct glyph_info key;

    /*
     *  Ugly work-around for symbol fonts.  Wine is sending characters which
     *  belong in the Unicode private use range (U+F020 - U+F0FF) as ASCII
     *  characters (U+0020 - U+00FF).
     */
    if ((font->glyphs->wch & 0xff00) == 0xf000 && wch < 0x100)
        wch |= 0xf000;

    key.wch = wch;
    needle = bsearch(&key, font->glyphs, font->glyph_count, sizeof(*font->glyphs), cmp_glyph_info);
    if (!needle)
    {
        WARN("No glyph for U+%.4X in '%s'\n", wch, font->name);
        needle = font->glyphs;
    }
    return needle;
}

static int ext_escape(PHYSDEV dev, int escape, int input_size, const void *input,
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
        struct band_info *ibi = (struct band_info *)input;
        struct band_info *obi = (struct band_info *)output;

        FIXME("BANDINFO(graphics %d, text %d, rect %s), stub!\n", ibi->graphics_flag,
                ibi->text_flag, wine_dbgstr_rect(&ibi->graphics_rect));
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

    case PSDRV_CHECK_WCHAR:
    {
        PSDRV_PDEVICE *pdev = get_psdrv_dev(dev);
        WCHAR *uv = (WCHAR *)input;
        WCHAR out = uv_metrics(*uv, pdev->font)->wch;

        if ((out & 0xff00) == 0xf000) out &= ~0xf000;
        *(WCHAR *)output = out;
        return 1;
    }

    case PSDRV_GET_BUILTIN_FONT_INFO:
    {
        PSDRV_PDEVICE *pdev = get_psdrv_dev(dev);
        struct font_info *font_info = (struct font_info *)output;

        if (!pdev->builtin)
            return 0;

        lstrcpynA(font_info->font_name, pdev->font->name, sizeof(font_info->font_name));
        font_info->size = pdev->size;
        font_info->escapement = pdev->escapement;
        return 1;
    }

    default:
        FIXME("Unimplemented code %d\n", escape);
        return 0;
    }
}

static inline float gdi_round(float f)
{
    return f > 0 ? f + 0.5 : f - 0.5;
}

static void scale_font(PSDRV_PDEVICE *pdev, const struct font_data *font, LONG height, TEXTMETRICW *tm)
{
    SHORT ascender, descender, line_gap, avg_char_width;
    USHORT units_per_em, win_ascent, win_descent;
    const IFIMETRICS *m = font->metrics;
    float scale;
    SIZE size;

    TRACE("'%s' %i\n", font->name, (int)height);

    if (height < 0) /* match em height */
        scale = -(height / (float)m->fwdUnitsPerEm);
    else /* match cell height */
        scale = height / (float)(m->fwdWinAscender + m->fwdWinDescender);

    size.cx = (INT)gdi_round(scale * (float)m->fwdUnitsPerEm);
    size.cy = -(INT)gdi_round(scale * (float)m->fwdUnitsPerEm);

    units_per_em = (USHORT)gdi_round((float)m->fwdUnitsPerEm * scale);
    ascender = (SHORT)gdi_round((float)m->fwdMacAscender * scale);
    descender = (SHORT)gdi_round((float)m->fwdMacDescender * scale);
    line_gap = (SHORT)gdi_round((float)m->fwdMacLineGap * scale);
    win_ascent = (USHORT)gdi_round((float)m->fwdWinAscender * scale);
    win_descent = (USHORT)gdi_round((float)m->fwdWinDescender * scale);
    avg_char_width = (SHORT)gdi_round((float)m->fwdAveCharWidth * scale);

    tm->tmAscent = (LONG)win_ascent;
    tm->tmDescent = (LONG)win_descent;
    tm->tmHeight = tm->tmAscent + tm->tmDescent;

    tm->tmInternalLeading = tm->tmHeight - (LONG)units_per_em;
    if (tm->tmInternalLeading < 0)
        tm->tmInternalLeading = 0;

    tm->tmExternalLeading =
            (LONG)(ascender - descender + line_gap) - tm->tmHeight;
    if (tm->tmExternalLeading < 0)
        tm->tmExternalLeading = 0;

    tm->tmAveCharWidth = (LONG)avg_char_width;

    tm->tmWeight = m->usWinWeight;
    tm->tmItalic = !!(m->fsSelection & FM_SEL_ITALIC);
    tm->tmUnderlined = !!(m->fsSelection & FM_SEL_UNDERSCORE);
    tm->tmStruckOut = !!(m->fsSelection & FM_SEL_STRIKEOUT);
    tm->tmFirstChar = font->glyphs[0].wch;
    tm->tmLastChar = font->glyphs[font->glyph_count - 1].wch;
    tm->tmDefaultChar = 0x001f; /* Win2K does this - FIXME? */
    tm->tmBreakChar = tm->tmFirstChar; /* should be 'space' */

    tm->tmPitchAndFamily = TMPF_DEVICE | TMPF_VECTOR;
    if (!(m->jWinPitchAndFamily & FIXED_PITCH))
        tm->tmPitchAndFamily |= TMPF_FIXED_PITCH; /* yes, it's backwards */
    if (m->fwdUnitsPerEm != 1000)
        tm->tmPitchAndFamily |= TMPF_TRUETYPE;

    tm->tmCharSet = ANSI_CHARSET; /* FIXME */
    tm->tmOverhang = 0;

    /*
     *  This is kludgy.  font->scale is used in several places in the driver
     *  to adjust PostScript-style metrics.  Since these metrics have been
     *  "normalized" to an em-square size of 1000, font->scale needs to be
     *  similarly adjusted..
     */

    scale *= (float)m->fwdUnitsPerEm / 1000.0;

    tm->tmMaxCharWidth = (LONG)gdi_round((m->rclFontBox.right - m->rclFontBox.left) * scale);

    if (pdev)
    {
        pdev->scale = scale;
        pdev->size = size;
    }

    TRACE("Selected PS font '%s' size %d weight %d.\n", font->name,
            (int)size.cx, (int)tm->tmWeight);
    TRACE("H = %d As = %d Des = %d IL = %d EL = %d\n", (int)tm->tmHeight,
            (int)tm->tmAscent, (int)tm->tmDescent, (int)tm->tmInternalLeading,
            (int)tm->tmExternalLeading);
}

static inline BOOL is_stock_font(HFONT font)
{
    int i;
    for (i = OEM_FIXED_FONT; i <= DEFAULT_GUI_FONT; i++)
    {
        if (i != DEFAULT_PALETTE && font == GetStockObject(i)) return TRUE;
    }
    return FALSE;
}

static struct font_data *find_font_data(const char *name)
{
    struct font_data *font;

    LIST_FOR_EACH_ENTRY(font, &fonts, struct font_data, entry)
    {
        if (!strcmp(font->name, name))
            return font;
    }
    return NULL;
}

static struct font_data *find_builtin_font(const PSDRV_DEVMODE *devmode,
        const WCHAR *facename, BOOL it, BOOL bd)
{
    struct installed_font *installed_font;
    BOOL best_it, best_bd, cur_it, cur_bd;
    struct font_data *best = NULL, *cur;
    const WCHAR *name;
    int i;

    installed_font = (struct installed_font *)(devmode->data +
            devmode->input_slots * sizeof(struct input_slot) +
            devmode->resolutions * sizeof(struct resolution) +
            devmode->page_sizes * sizeof(struct page_size) +
            devmode->font_subs * sizeof(struct font_sub));
    for (i = 0; i < devmode->installed_fonts; i++)
    {
        cur = find_font_data(installed_font[i].name);
        if (!cur) continue;

        name = (WCHAR *)((char *)cur->metrics + cur->metrics->dpwszFamilyName);
        cur_it = !!(cur->metrics->fsSelection & FM_SEL_ITALIC);
        cur_bd = !!(cur->metrics->fsSelection & FM_SEL_BOLD);

        if (!facename && it == cur_it && bd == cur_bd)
            return cur;
        if (facename && !wcscmp(facename, name) && it == cur_it && bd == cur_bd)
            return cur;
        if (facename && wcscmp(facename, name))
            continue;

        if (!best || (best_it != it && cur_it == it) ||
                (best_it != it && best_bd != bd && cur_bd == bd))
        {
            best = cur;
            best_it = cur_it;
            best_bd = cur_bd;
        }
    }

    return best;
}

static BOOL select_builtin_font(PSDRV_PDEVICE *pdev, HFONT hfont, LOGFONTW *plf)
{
    struct font_data *font_data;
    BOOL bd = FALSE, it = FALSE;
    LONG height;

    TRACE("Trying to find facename %s\n", debugstr_w(plf->lfFaceName));

    if (plf->lfItalic)
        it = TRUE;
    if (plf->lfWeight > 550)
        bd = TRUE;

    /* Look for a matching font family */
    font_data = find_builtin_font(pdev->devmode, plf->lfFaceName, it, bd);
    if (!font_data)
    {
        /* Fallback for Window's font families to common PostScript families */
        if (!wcscmp(plf->lfFaceName, arialW))
            wcscpy(plf->lfFaceName, helveticaW);
        else if (!wcscmp(plf->lfFaceName, systemW))
            wcscpy(plf->lfFaceName, helveticaW);
        else if (!wcscmp(plf->lfFaceName, times_new_romanW))
            wcscpy(plf->lfFaceName, timesW);
        else if (!wcscmp(plf->lfFaceName, courier_newW))
            wcscpy(plf->lfFaceName, courierW);

        font_data = find_builtin_font(pdev->devmode, plf->lfFaceName, it, bd);
    }
    /* If all else fails, use the first font defined for the printer */
    if (!font_data)
        font_data = find_builtin_font(pdev->devmode, NULL, it, bd);

    TRACE("Got family %s font '%s'\n", debugstr_w((WCHAR *)((char *)font_data->metrics +
                    font_data->metrics->dpwszFamilyName)), font_data->name);

    pdev->builtin = TRUE;
    pdev->font = NULL;
    pdev->font = font_data;

    height = plf->lfHeight;
    /* stock fonts ignore the mapping mode */
    if (!is_stock_font(hfont))
    {
        POINT pts[2];
        pts[0].x = pts[0].y = pts[1].x = 0;
        pts[1].y = height;
        NtGdiTransformPoints(pdev->dev.hdc, pts, pts, 2, NtGdiLPtoDP);
        height = pts[1].y - pts[0].y;
    }
    scale_font(pdev, font_data, height, &pdev->tm);

    /* Does anyone know if these are supposed to be reversed like this? */
    pdev->tm.tmDigitizedAspectX = pdev->log_pixels_y;
    pdev->tm.tmDigitizedAspectY = pdev->log_pixels_x;
    return TRUE;
}

static HFONT select_font(PHYSDEV dev, HFONT hfont, UINT *aa_flags)
{
    PSDRV_PDEVICE *pdev = get_psdrv_dev(dev);
    PHYSDEV next = GET_NEXT_PHYSDEV(dev, pSelectFont);
    const struct font_sub *font_sub;
    HFONT ret;
    LOGFONTW lf;
    BOOL subst = FALSE;
    int i;

    if (!NtGdiExtGetObjectW(hfont, sizeof(lf), &lf)) return 0;

    *aa_flags = GGO_BITMAP; /* no anti-aliasing on printer devices */

    TRACE("FaceName = %s Height = %d Italic = %d Weight = %d\n",
            debugstr_w(lf.lfFaceName), (int)lf.lfHeight, lf.lfItalic,
            (int)lf.lfWeight);

    if (lf.lfFaceName[0] == '\0')
    {
        switch(lf.lfPitchAndFamily & 0xf0)
        {
        case FF_DONTCARE:
            break;
        case FF_ROMAN:
        case FF_SCRIPT:
            wcscpy(lf.lfFaceName, timesW);
            break;
        case FF_SWISS:
            wcscpy(lf.lfFaceName, helveticaW);
            break;
        case FF_MODERN:
            wcscpy(lf.lfFaceName, courierW);
            break;
        case FF_DECORATIVE:
            wcscpy(lf.lfFaceName, symbolW);
            break;
        }
    }

    if (lf.lfFaceName[0] == '\0')
    {
        switch(lf.lfPitchAndFamily & 0x0f)
        {
        case VARIABLE_PITCH:
            wcscpy(lf.lfFaceName, timesW);
            break;
        default:
            wcscpy(lf.lfFaceName, courierW);
            break;
        }
    }

    font_sub = (const struct font_sub *)(pdev->devmode->data +
            pdev->devmode->input_slots * sizeof(struct input_slot) +
            pdev->devmode->resolutions * sizeof(struct resolution) +
            pdev->devmode->page_sizes * sizeof(struct page_size));
    for (i = 0; i < pdev->devmode->font_subs; i++)
    {
        if (!wcsicmp(lf.lfFaceName, font_sub[i].name))
        {
            TRACE("substituting facename %s for %s\n",
                    debugstr_w(font_sub[i].sub), debugstr_w(lf.lfFaceName));
            if (wcslen(font_sub[i].sub) < LF_FACESIZE)
            {
                wcscpy(lf.lfFaceName, font_sub[i].sub);
                subst = TRUE;
            }
            else
            {
                WARN("Facename %s is too long; ignoring substitution\n",
                        debugstr_w(font_sub[i].sub));
            }
            break;
        }
    }

    pdev->escapement = lf.lfEscapement;

    if (!subst && ((ret = next->funcs->pSelectFont(next, hfont, aa_flags))))
    {
        pdev->builtin = FALSE;
        return ret;
    }

    select_builtin_font(pdev, hfont, &lf);
    next->funcs->pSelectFont(next, 0, aa_flags);  /* tell next driver that we selected a device font */
    return hfont;
}

static UINT get_font_metric(const struct font_data *font,
        NEWTEXTMETRICEXW *ntmx, ENUMLOGFONTEXW *elfx)
{
    /* ntmx->ntmTm is NEWTEXTMETRICW; compatible w/ TEXTMETRICW per Win32 doc */
    TEXTMETRICW *tm = (TEXTMETRICW *)&(ntmx->ntmTm);
    LOGFONTW *lf = &(elfx->elfLogFont);

    memset(ntmx, 0, sizeof(*ntmx));
    memset(elfx, 0, sizeof(*elfx));

    scale_font(NULL, font, -(LONG)font->metrics->fwdUnitsPerEm, tm);

    lf->lfHeight = tm->tmHeight;
    lf->lfWidth = tm->tmAveCharWidth;
    lf->lfWeight = tm->tmWeight;
    lf->lfItalic = tm->tmItalic;
    lf->lfCharSet = tm->tmCharSet;

    lf->lfPitchAndFamily = font->metrics->jWinPitchAndFamily & FIXED_PITCH ? FIXED_PITCH : VARIABLE_PITCH;

    lstrcpynW(lf->lfFaceName, (WCHAR *)((char *)font->metrics + font->metrics->dpwszFamilyName), LF_FACESIZE);
    return DEVICE_FONTTYPE;
}

static BOOL enum_fonts(PHYSDEV dev, LPLOGFONTW plf, font_enum_proc proc, LPARAM lp)
{
    PSDRV_PDEVICE *pdev = get_psdrv_dev(dev);
    PHYSDEV next = GET_NEXT_PHYSDEV(dev, pEnumFonts);
    PSDRV_DEVMODE *devmode = pdev->devmode;
    struct installed_font *installed_font;
    struct font_data *cur;
    ENUMLOGFONTEXW lf;
    NEWTEXTMETRICEXW tm;
    const WCHAR *name;
    BOOL ret;
    UINT fm;
    int i;

    ret = next->funcs->pEnumFonts(next, plf, proc, lp);
    if (!ret) return FALSE;

    installed_font = (struct installed_font *)(devmode->data +
            devmode->input_slots * sizeof(struct input_slot) +
            devmode->resolutions * sizeof(struct resolution) +
            devmode->page_sizes * sizeof(struct page_size) +
            devmode->font_subs * sizeof(struct font_sub));
    if (plf && plf->lfFaceName[0])
    {
        TRACE("lfFaceName = %s\n", debugstr_w(plf->lfFaceName));

        for (i = 0; i < devmode->installed_fonts; i++)
        {
            cur = find_font_data(installed_font[i].name);
            if (!cur) continue;

            name = (WCHAR *)((char *)cur->metrics + cur->metrics->dpwszFamilyName);
            if (wcsncmp(plf->lfFaceName, name, wcslen(name)))
                continue;

            TRACE("Got '%s'\n", cur->name);
            fm = get_font_metric(cur, &tm, &lf);
            if (!(ret = (*proc)(&lf.elfLogFont, (TEXTMETRICW *)&tm, fm, lp)))
                break;
        }
    }
    else
    {
        TRACE("lfFaceName = NULL\n");

        for (i = 0; i < devmode->installed_fonts; i++)
        {
            cur = find_font_data(installed_font[i].name);
            if (!cur) continue;

            name = (WCHAR *)((char *)cur->metrics + cur->metrics->dpwszFamilyName);
            TRACE("Got '%s'\n", cur->name);
            fm = get_font_metric(cur, &tm, &lf);
            if (!(ret = (*proc)(&lf.elfLogFont, (TEXTMETRICW *)&tm, fm, lp)))
                break;
        }
    }
    return ret;
}

static BOOL get_char_width(PHYSDEV dev, UINT first, UINT count, const WCHAR *chars, INT *buffer)
{
    PSDRV_PDEVICE *pdev = get_psdrv_dev(dev);
    UINT i, c;

    if (!pdev->builtin)
    {
        dev = GET_NEXT_PHYSDEV(dev, pGetCharWidth);
        return dev->funcs->pGetCharWidth(dev, first, count, chars, buffer);
    }

    TRACE("U+%.4X +%u\n", first, count);

    for (i = 0; i < count; ++i)
    {
        c = chars ? chars[i] : first + i;

        if (c > 0xffff)
            return FALSE;

        *buffer = floor(uv_metrics(c, pdev->font)->width * pdev->scale + 0.5);
        TRACE("U+%.4X: %i\n", i, *buffer);
        ++buffer;
    }
    return TRUE;
}

static BOOL get_text_metrics(PHYSDEV dev, TEXTMETRICW *metrics)
{
    PSDRV_PDEVICE *pdev = get_psdrv_dev(dev);

    if (!pdev->builtin)
    {
        dev = GET_NEXT_PHYSDEV(dev, pGetTextMetrics);
        return dev->funcs->pGetTextMetrics(dev, metrics);
    }

    memcpy(metrics, &pdev->tm, sizeof(pdev->tm));
    return TRUE;
}

static BOOL get_text_extent_ex_point(PHYSDEV dev, const WCHAR *str, int count, int *dx)
{
    PSDRV_PDEVICE *pdev = get_psdrv_dev(dev);
    int             i;
    float           width = 0.0;

    if (!pdev->builtin)
    {
        dev = GET_NEXT_PHYSDEV(dev, pGetTextExtentExPoint);
        return dev->funcs->pGetTextExtentExPoint(dev, str, count, dx);
    }

    TRACE("%s %i\n", debugstr_wn(str, count), count);

    for (i = 0; i < count; ++i)
    {
        width += uv_metrics(str[i], pdev->font)->width;
        dx[i] = width * pdev->scale;
    }
    return TRUE;
}

static struct printer_info *find_printer_info(const WCHAR *name)
{
    struct printer_info *pi;

    LIST_FOR_EACH_ENTRY(pi, &printer_info_list, struct printer_info, entry)
    {
        if (!wcscmp(pi->name, name))
            return pi;
    }
    return NULL;
}

static PSDRV_PDEVICE *create_physdev(HDC hdc, const WCHAR *device,
        const DEVMODEW *devmode)
{
    struct printer_info *pi = find_printer_info(device);
    PSDRV_PDEVICE *pdev;

    if (!pi) return NULL;
    if (!find_builtin_font(pi->devmode, NULL, FALSE, FALSE))
    {
        RASTERIZER_STATUS status;
        if (!NtGdiGetRasterizerCaps(&status, sizeof(status)) ||
                !(status.wFlags & TT_AVAILABLE) ||
                !(status.wFlags & TT_ENABLED))
        {
            MESSAGE("Disabling printer %s since it has no builtin fonts and "
                    "there are no TrueType fonts available.\n", debugstr_w(device));
            return FALSE;
        }
    }

    pdev = malloc(sizeof(*pdev));
    if (!pdev) return NULL;

    pdev->devmode = malloc(pi->devmode->dmPublic.dmSize + pi->devmode->dmPublic.dmDriverExtra);
    if (!pdev->devmode)
    {
        free(pdev);
        return NULL;
    }

    memcpy(pdev->devmode, pi->devmode, pi->devmode->dmPublic.dmSize +
            pi->devmode->dmPublic.dmDriverExtra);
    pdev->pi = pi;
    pdev->log_pixels_x = pdev->devmode->default_resolution;
    pdev->log_pixels_y = pdev->devmode->default_resolution;

    if (devmode)
    {
        dump_devmode(devmode);
        merge_devmodes(pdev->devmode, devmode, pi);
    }

    update_dev_caps(pdev);
    NtGdiSelectFont(hdc, GetStockObject(DEVICE_DEFAULT_FONT));
    return pdev;
}

static BOOL create_dc(PHYSDEV *dev, const WCHAR *device,
        const WCHAR *output, const DEVMODEW *devmode)
{
    PSDRV_PDEVICE *pdev;

    TRACE("(%s %s %p)\n", debugstr_w(device), debugstr_w(output), devmode);

    if (!device) return FALSE;
    if (!(pdev = create_physdev((*dev)->hdc, device, devmode))) return FALSE;
    push_dc_driver(dev, &pdev->dev, &psdrv_funcs);
    return TRUE;
}

static BOOL create_compatible_dc(PHYSDEV orig, PHYSDEV *dev)
{
    PSDRV_PDEVICE *pdev, *orig_dev = get_psdrv_dev(orig);

    if (!(pdev = create_physdev((*dev)->hdc, orig_dev->pi->name,
                    &orig_dev->devmode->dmPublic))) return FALSE;
    push_dc_driver(dev, &pdev->dev, &psdrv_funcs);
    return TRUE;
}

static BOOL delete_dc(PHYSDEV dev)
{
    PSDRV_PDEVICE *pdev = get_psdrv_dev(dev);

    TRACE("\n");

    free(pdev->devmode);
    free(pdev);
    return TRUE;
}

static const struct gdi_dc_funcs psdrv_funcs =
{
    .pCreateCompatibleDC = create_compatible_dc,
    .pCreateDC = create_dc,
    .pDeleteDC = delete_dc,
    .pEnumFonts = enum_fonts,
    .pExtEscape = ext_escape,
    .pGetCharWidth = get_char_width,
    .pGetDeviceCaps = get_device_caps,
    .pGetTextExtentExPoint = get_text_extent_ex_point,
    .pGetTextMetrics = get_text_metrics,
    .pResetDC = reset_dc,
    .pSelectFont = select_font,
    .priority = GDI_PRIORITY_GRAPHICS_DRV
};

static BOOL check_ntf_str(const char *data, UINT64 size, const char *str)
{
    if (str < data || str >= data + size)
        return FALSE;
    size -= str - data;
    while (*str && size)
    {
        size--;
        str++;
    }
    return size != 0;
}

static void free_font_data(struct font_data *font_data)
{
    free(font_data->name);
    free(font_data->metrics);
    free(font_data->glyphs);
    free(font_data);
}

static WCHAR convert_ntf_cp(unsigned short c, unsigned short cp)
{
    static const WCHAR map_fff1[256] = {
        0x0000, 0x02d8, 0x02c7, 0x02d9, 0x0131, 0xfb01, 0xfb02, 0x2044,
        0x02dd, 0x0141, 0x0142, 0x2212, 0x02db, 0x02da, 0x017d, 0x017e,
        0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
        0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
        0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027,
        0x0028, 0x0029, 0x002a, 0x002b, 0x002c, 0x002d, 0x002e, 0x002f,
        0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037,
        0x0038, 0x0029, 0x002a, 0x002b, 0x002c, 0x002d, 0x002e, 0x002f,
        0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047,
        0x0048, 0x0049, 0x004a, 0x004b, 0x004c, 0x004d, 0x004e, 0x004f,
        0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057,
        0x0058, 0x0059, 0x005a, 0x005b, 0x005c, 0x005d, 0x005e, 0x005f,
        0x0060, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067,
        0x0068, 0x0069, 0x006a, 0x006b, 0x006c, 0x006d, 0x006e, 0x006f,
        0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077,
        0x0078, 0x0079, 0x007a, 0x007b, 0x007c, 0x007d, 0x007e, 0x0000,
        0x20ac, 0x0000, 0x201a, 0x0192, 0x201e, 0x2026, 0x2020, 0x2021,
        0x02c6, 0x2030, 0x0160, 0x2039, 0x0152, 0x0000, 0x0000, 0x0000,
        0x0000, 0x2018, 0x2019, 0x201c, 0x201d, 0x2022, 0x2013, 0x2014,
        0x02dc, 0x2122, 0x0161, 0x203a, 0x0153, 0x0000, 0x0000, 0x0178,
        0x0000, 0x00a1, 0x00a2, 0x00a3, 0x00a4, 0x00a5, 0x00a6, 0x00a7,
        0x00a8, 0x00a9, 0x00aa, 0x00ab, 0x00ac, 0x0000, 0x00ae, 0x00af,
        0x00b0, 0x00b1, 0x00b2, 0x00b3, 0x00b4, 0x00b5, 0x00b6, 0x00b7,
        0x00b8, 0x00b9, 0x00ba, 0x00bb, 0x00bc, 0x00bd, 0x00be, 0x00bf,
        0x00c0, 0x00c1, 0x00c2, 0x00c3, 0x00c4, 0x00c5, 0x00c6, 0x00c7,
        0x00c8, 0x00c9, 0x00ca, 0x00cb, 0x00cc, 0x00cd, 0x00ce, 0x00cf,
        0x00d0, 0x00d1, 0x00d2, 0x00d3, 0x00d4, 0x00d5, 0x00d6, 0x00d7,
        0x00d8, 0x00d9, 0x00da, 0x00db, 0x00dc, 0x00dd, 0x00de, 0x00df,
        0x00e0, 0x00e1, 0x00e2, 0x00e3, 0x00e4, 0x00e5, 0x00e6, 0x00e7,
        0x00e8, 0x00e9, 0x00ea, 0x00eb, 0x00ec, 0x00ed, 0x00ee, 0x00ef,
        0x00f0, 0x00f1, 0x00f2, 0x00f3, 0x00f4, 0x00f5, 0x00f6, 0x00f7,
        0x00f8, 0x00f9, 0x00fa, 0x00fb, 0x00fc, 0x00fd, 0x00fe, 0x00ff,
    };
    WCHAR ret = 0;

    switch (cp)
    {
    case 0xfff1:
        ret = c < ARRAY_SIZE(map_fff1) ? map_fff1[c] : 0;
        break;
    case 0xffff: /* Wine extension */
        ret = c;
        break;
    }

    if (!ret && c)
        FIXME("unrecognized character %x in %x\n", c, cp);
    return ret;
}

static BOOL map_glyph_to_unicode(struct font_data *font_data,
        const char *data, UINT64 size, const char *name)
{
    const struct ntf_header *header = (const struct ntf_header *)data;
    const struct list_entry *list_elem;
    const struct glyph_set *glyph_set;
    const unsigned short *p;
    int i, j;

    list_elem = (const struct list_entry *)(data + header->glyph_set_off);
    for (i = 0; i < header->glyph_set_count; i++)
    {
        if (!check_ntf_str(data, size, data + list_elem->name_off))
            return FALSE;
        if (strcmp(data + list_elem->name_off, name))
        {
            list_elem++;
            continue;
        }
        if (list_elem->off + list_elem->size > size)
            return FALSE;

        glyph_set = (const struct glyph_set *)(data + list_elem->off);
        if (font_data->glyph_count > glyph_set->glyph_count)
            return FALSE;

        p = (const unsigned short *)((const char *)glyph_set + glyph_set->glyph_set_off);
        if (glyph_set->flags & GLYPH_SET_OMIT_CP)
        {
            const struct code_page *code_page = (const struct code_page *)((const char *)glyph_set + glyph_set->cp_off);
            unsigned short def_cp;

            if (glyph_set->cp_off + sizeof(*code_page) * glyph_set->cp_count > list_elem->size)
                return FALSE;
            if (glyph_set->cp_count != 1)
                return FALSE;
            if (glyph_set->glyph_set_off + sizeof(short) * font_data->glyph_count > list_elem->size)
                return FALSE;

            def_cp = code_page->cp;
            for (j = 0; j < font_data->glyph_count; j++)
            {
                font_data->glyphs[j].wch = convert_ntf_cp(p[0], def_cp);
                p++;
            }
        }
        else
        {
            if (glyph_set->glyph_set_off + sizeof(short[2]) * font_data->glyph_count > list_elem->size)
                return FALSE;

            for (j = 0; j < font_data->glyph_count; j++)
            {
                font_data->glyphs[j].wch = convert_ntf_cp(p[0], p[1]);
                p += 2;
            }
        }
        return TRUE;
    }

    return FALSE;
}

static BOOL add_ntf_fonts(const char *data, int size)
{
    const struct ntf_header *header = (const struct ntf_header *)data;
    const struct width_range *width_range;
    const struct list_entry *list_elem;
    const struct font_mtx *font_mtx;
    struct font_data *font_data;
    const IFIMETRICS *metrics;
    const char *name;
    int i, j, k;

    if (size < sizeof(*header) ||
            size < header->glyph_set_off + header->glyph_set_count * sizeof(*list_elem) ||
            size < header->font_mtx_off + header->font_mtx_count * sizeof(*list_elem))
        return FALSE;

    list_elem = (const struct list_entry *)(data + header->font_mtx_off);
    for (i = 0; i < header->font_mtx_count; i++)
    {
        name = data + list_elem->name_off;
        if (!check_ntf_str(data, size, name))
            return FALSE;
        TRACE("adding %s font\n", name);

        if (list_elem->size + list_elem->off > size)
            return FALSE;
        font_mtx = (const struct font_mtx *)(data + list_elem->off);
        if (list_elem->off + font_mtx->metrics_off + FIELD_OFFSET(IFIMETRICS, panose) > size)
            return FALSE;
        metrics = (const IFIMETRICS *)((const char *)font_mtx + font_mtx->metrics_off);
        if (list_elem->off + font_mtx->metrics_off + metrics->cjThis > size)
            return FALSE;
        if (list_elem->off + font_mtx->width_off + sizeof(*width_range) * font_mtx->width_count > size)
            return FALSE;
        width_range = (const struct width_range *)((const char *)font_mtx + font_mtx->width_off);
        if (!check_ntf_str(data, size, (const char *)font_mtx + font_mtx->glyph_set_name_off))
            return FALSE;

        if (!font_mtx->glyph_count)
        {
            list_elem++;
            continue;
        }

        font_data = calloc(sizeof(*font_data), 1);
        if (!font_data)
            return FALSE;

        font_data->glyph_count = font_mtx->glyph_count;
        font_data->name = malloc(strlen(name) + 1);
        font_data->metrics = malloc(metrics->cjThis);
        font_data->glyphs = malloc(sizeof(*font_data->glyphs) * font_data->glyph_count);
        if (!font_data->name || !font_data->metrics || !font_data->glyphs)
        {
            free_font_data(font_data);
            return FALSE;
        }
        memcpy(font_data->name, name, strlen(name) + 1);
        memcpy(font_data->metrics, metrics, metrics->cjThis);

        for (j = 0; j < font_mtx->glyph_count; j++)
            font_data->glyphs[j].width = font_mtx->def_width;
        for (j = 0; j < font_mtx->width_count; j++)
        {
            /* Use default width */
            if (width_range[j].width == 0x80000008)
                continue;

            for (k = 0; k < width_range[j].count; k++)
            {
                if (width_range[j].first + k >= font_data->glyph_count)
                    break;
                font_data->glyphs[width_range[j].first + k].width = width_range[j].width;
            }
        }

        if (!map_glyph_to_unicode(font_data, data, size,
                    (const char *)font_mtx + font_mtx->glyph_set_name_off))
        {
            free_font_data(font_data);
            WARN("error loading %s font\n", name);
            list_elem++;
            continue;
        }
        font_data->def_glyph = font_data->glyphs[0];

        qsort(font_data->glyphs, font_data->glyph_count,
                sizeof(*font_data->glyphs), cmp_glyph_info);
        list_add_head(&fonts, &font_data->entry);
        list_elem++;
        TRACE("%s font added\n", name);
    }

    return TRUE;
}

static NTSTATUS import_ntf(void *arg)
{
    struct import_ntf_params *params = arg;

    return add_ntf_fonts(params->data, params->size);
}

static NTSTATUS open_dc(void *arg)
{
    UNICODE_STRING device_str, output_str;
    struct open_dc_params *params = arg;
    struct printer_info *pi;

    pi = find_printer_info(params->device);
    if (!pi)
    {
        pi = malloc(sizeof(*pi));
        if (!pi) return FALSE;

        pi->name = params->device;
        pi->devmode = params->def_devmode;
        list_add_head(&printer_info_list, &pi->entry);
    }

    device_str.Length = device_str.MaximumLength = lstrlenW(params->device) + 1;
    device_str.Buffer = (WCHAR *)params->device;
    if (params->output)
    {
        output_str.Length = output_str.MaximumLength = lstrlenW(params->output) + 1;
        output_str.Buffer = (WCHAR *)params->output;
    }
    params->hdc = NtGdiOpenDCW(&device_str, params->devmode, params->output ? &output_str : NULL,
            WINE_GDI_DRIVER_VERSION, 0, (HANDLE)&psdrv_funcs, NULL, NULL);
    return TRUE;
}

static NTSTATUS free_printer_info(void *arg)
{
    struct font_data *font, *font_next;
    struct printer_info *pi, *next;

    LIST_FOR_EACH_ENTRY_SAFE(pi, next, &printer_info_list, struct printer_info, entry)
    {
        free(pi);
    }

    LIST_FOR_EACH_ENTRY_SAFE(font, font_next, &fonts, struct font_data, entry)
    {
        free_font_data(font);
    }
    return 0;
}

const unixlib_entry_t __wine_unix_call_funcs[] =
{
    free_printer_info,
    import_ntf,
    open_dc,
};

C_ASSERT(ARRAYSIZE(__wine_unix_call_funcs) == unix_funcs_count);

#ifdef _WIN64

typedef ULONG PTR32;

static NTSTATUS wow64_import_ntf(void *args)
{
    struct
    {
        PTR32 data;
        int size;
    } const *params32 = args;
    struct import_ntf_params params = { ULongToPtr(params32->data), params32->size };

    return import_ntf(&params);
}

static NTSTATUS wow64_open_dc(void *args)
{
    struct
    {
        PTR32 device;
        PTR32 devmode;
        PTR32 output;
        PTR32 def_devmode;
        PTR32 hdc;
    } *params32 = args;
    struct open_dc_params params =
    {
        ULongToPtr(params32->device),
        ULongToPtr(params32->devmode),
        ULongToPtr(params32->output),
        ULongToPtr(params32->def_devmode),
        0
    };
    NTSTATUS ret;

    ret = open_dc(&params);
    params32->hdc = PtrToUlong(params.hdc);
    return ret;
}

const unixlib_entry_t __wine_unix_call_wow64_funcs[] =
{
    free_printer_info,
    wow64_import_ntf,
    wow64_open_dc,
};

C_ASSERT(ARRAYSIZE(__wine_unix_call_wow64_funcs) == unix_funcs_count);

#endif  /* _WIN64 */
