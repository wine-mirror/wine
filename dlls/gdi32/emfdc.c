/*
 * Enhanced MetaFile recording functions
 *
 * Copyright 1999 Huw D M Davies
 * Copyright 2016 Alexandre Julliard
 * Copyright 2021 Jacek Caban for CodeWeavers
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

#include <assert.h>
#include <stdlib.h>

#include "gdi_private.h"
#include "winnls.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(enhmetafile);

static inline UINT aligned_size(UINT size)
{
    return (size + 3) & ~3;
}

static inline void pad_record(void *record, UINT size)
{
    if (size & 3) memset((char *)record + size, 0, 4 - (size & 3));
}

struct emf
{
    ENHMETAHEADER  *emh;
    DC_ATTR *dc_attr;
    UINT     handles_size, cur_handles;
    HGDIOBJ *handles;
    HANDLE   file;
    HBRUSH   dc_brush;
    HPEN     dc_pen;
    BOOL     path;
    DWORD    palette_size;
    DWORD    palette_used;
    PALETTEENTRY *palette;
};

typedef enum
{
    EMRI_METAFILE = 1,
    EMRI_ENGINE_FONT,
    EMRI_DEVMODE,
    EMRI_TYPE1_FONT,
    EMRI_PRESTARTPAGE,
    EMRI_DESIGNVECTOR,
    EMRI_SUBSET_FONT,
    EMRI_DELTA_FONT,
    EMRI_FORM_METAFILE,
    EMRI_BW_METAFILE,
    EMRI_BW_FORM_METAFILE,
    EMRI_METAFILE_DATA,
    EMRI_METAFILE_EXT,
    EMRI_BW_METAFILE_EXT,
    EMRI_ENGINE_FONT_EXT,
    EMRI_TYPE1_FONT_EXT,
    EMRI_DESIGNVECTOR_EXT,
    EMRI_SUBSET_FONT_EXT,
    EMRI_DELTA_FONT_EXT,
    EMRI_PS_JOB_DATA,
    EMRI_EMBED_FONT_EXT,
} emfspool_record_type;

#define HANDLE_LIST_INC 20
static const RECTL empty_bounds = { 0, 0, -1, -1 };

static struct emf *get_dc_emf( DC_ATTR *dc_attr )
{
    return (struct emf *)(UINT_PTR)dc_attr->emf;
}

static HDC dc_attr_handle( DC_ATTR *dc_attr )
{
    return UlongToHandle( dc_attr->hdc );
}

static BOOL emfdc_record( struct emf *emf, EMR *emr )
{
    DWORD len, size;
    ENHMETAHEADER *emh;

    TRACE( "record %ld, size %ld\n", emr->iType, emr->nSize );

    assert( !(emr->nSize & 3) );

    emf->emh->nBytes += emr->nSize;
    emf->emh->nRecords++;

    size = HeapSize( GetProcessHeap(), 0, emf->emh );
    len = emf->emh->nBytes;
    if (len > size)
    {
        size += (size / 2) + emr->nSize;
        emh = HeapReAlloc( GetProcessHeap(), 0, emf->emh, size );
        if (!emh) return FALSE;
        emf->emh = emh;
    }
    memcpy( (char *)emf->emh + emf->emh->nBytes - emr->nSize, emr, emr->nSize );
    return TRUE;
}

static void emfdc_update_bounds( struct emf *emf, RECTL *rect )
{
    RECTL *bounds = &emf->dc_attr->emf_bounds;
    RECTL vport_rect = *rect;

    LPtoDP( dc_attr_handle( emf->dc_attr ), (POINT *)&vport_rect, 2 );

    /* The coordinate systems may be mirrored
       (LPtoDP handles points, not rectangles) */
    if (vport_rect.left > vport_rect.right)
    {
        LONG temp = vport_rect.right;
        vport_rect.right = vport_rect.left;
        vport_rect.left = temp;
    }
    if (vport_rect.top > vport_rect.bottom)
    {
        LONG temp = vport_rect.bottom;
        vport_rect.bottom = vport_rect.top;
        vport_rect.top = temp;
    }

    if (bounds->left > bounds->right)
    {
        /* first bounding rectangle */
        *bounds = vport_rect;
    }
    else
    {
        bounds->left   = min(bounds->left,   vport_rect.left);
        bounds->top    = min(bounds->top,    vport_rect.top);
        bounds->right  = max(bounds->right,  vport_rect.right);
        bounds->bottom = max(bounds->bottom, vport_rect.bottom);
    }
}

static UINT get_bitmap_info( HDC *hdc, HBITMAP *bitmap, BITMAPINFO *info )
{
    HBITMAP blit_bitmap;
    HDC blit_dc;
    UINT info_size, bpp;
    DIBSECTION dib;

    if (!(info_size = GetObjectW( *bitmap, sizeof(dib), &dib ))) return 0;

    if (info_size == sizeof(dib))
    {
        if (!GetDIBits( *hdc, *bitmap, 0, INT_MAX, NULL, info, DIB_RGB_COLORS ))
            return 0;
    }
    else
    {
        unsigned char dib_info_buffer[FIELD_OFFSET(BITMAPINFO, bmiColors[256])];
        BITMAPINFO *dib_info = (BITMAPINFO *)dib_info_buffer;
        BITMAP bmp = dib.dsBm;
        HPALETTE palette;
        void *bits;

        assert( info_size == sizeof(BITMAP) );

        dib_info->bmiHeader.biSize = sizeof(dib_info->bmiHeader);
        dib_info->bmiHeader.biWidth = bmp.bmWidth;
        dib_info->bmiHeader.biHeight = bmp.bmHeight;
        dib_info->bmiHeader.biPlanes = 1;
        dib_info->bmiHeader.biBitCount = bmp.bmBitsPixel;
        dib_info->bmiHeader.biCompression = BI_RGB;
        dib_info->bmiHeader.biSizeImage = 0;
        dib_info->bmiHeader.biXPelsPerMeter = 0;
        dib_info->bmiHeader.biYPelsPerMeter = 0;
        dib_info->bmiHeader.biClrUsed = 0;
        dib_info->bmiHeader.biClrImportant = 0;
        switch (dib_info->bmiHeader.biBitCount)
        {
        case 16:
            ((DWORD *)dib_info->bmiColors)[0] = 0xf800;
            ((DWORD *)dib_info->bmiColors)[1] = 0x07e0;
            ((DWORD *)dib_info->bmiColors)[2] = 0x001f;
            break;
        case 32:
            ((DWORD *)dib_info->bmiColors)[0] = 0xff0000;
            ((DWORD *)dib_info->bmiColors)[1] = 0x00ff00;
            ((DWORD *)dib_info->bmiColors)[2] = 0x0000ff;
            break;
        default:
            if (dib_info->bmiHeader.biBitCount > 8) break;
            if (!(palette = GetCurrentObject( *hdc, OBJ_PAL ))) return FALSE;
            if (!GetPaletteEntries( palette, 0, 256, (PALETTEENTRY *)dib_info->bmiColors ))
                return FALSE;
        }

        if (!(blit_dc = NtGdiCreateCompatibleDC( *hdc ))) return FALSE;
        if (!(blit_bitmap = CreateDIBSection( blit_dc, dib_info, DIB_RGB_COLORS, &bits, NULL, 0 )))
            goto err;
        if (!SelectObject( blit_dc, blit_bitmap )) goto err;
        if (!BitBlt( blit_dc, 0, 0, bmp.bmWidth, bmp.bmHeight, *hdc, 0, 0, SRCCOPY ))
            goto err;
        if (!GetDIBits( blit_dc, blit_bitmap, 0, INT_MAX, NULL, info, DIB_RGB_COLORS ))
            goto err;
        *hdc = blit_dc;
        *bitmap = blit_bitmap;
    }

    bpp = info->bmiHeader.biBitCount;
    if (bpp <= 8)
        return sizeof(BITMAPINFOHEADER) + (1 << bpp) * sizeof(RGBQUAD);
    else if (bpp == 16 || bpp == 32)
        return sizeof(BITMAPINFOHEADER) + 3 * sizeof(RGBQUAD);

    return sizeof(BITMAPINFOHEADER);

err:
    DeleteDC( blit_dc );
    if (blit_bitmap) DeleteObject( blit_bitmap );
    return 0;
}

/*******************************************************************************************
 * Verify that the DIB parameters are valid.
 */
static BOOL is_valid_dib_format( const BITMAPINFOHEADER *info, BOOL allow_compression )
{
    if (info->biWidth <= 0) return FALSE;
    if (info->biHeight == 0) return FALSE;

    if (allow_compression && (info->biCompression == BI_RLE4 || info->biCompression == BI_RLE8))
    {
        if (info->biHeight < 0) return FALSE;
        if (!info->biSizeImage) return FALSE;
        return info->biBitCount == (info->biCompression == BI_RLE4 ? 4 : 8);
    }

    if (!info->biPlanes) return FALSE;

    /* check for size overflow */
    if (!info->biBitCount) return FALSE;
    if (UINT_MAX / info->biBitCount < info->biWidth) return FALSE;
    if (UINT_MAX / get_dib_stride( info->biWidth, info->biBitCount ) < abs( info->biHeight )) return FALSE;

    switch (info->biBitCount)
    {
    case 1:
    case 4:
    case 8:
    case 24:
        return (info->biCompression == BI_RGB);
    case 16:
    case 32:
        return (info->biCompression == BI_BITFIELDS || info->biCompression == BI_RGB);
    default:
        return FALSE;
    }
}

static BOOL emf_parse_user_bitmapinfo( BITMAPINFOHEADER *dst, const BITMAPINFOHEADER *info,
                                       UINT coloruse, BOOL allow_compression,
                                       UINT *bmi_size, UINT *img_size )
{
    UINT colour_table_size;

    if (coloruse > DIB_PAL_COLORS + 1) return FALSE;  /* FIXME: handle DIB_PAL_COLORS+1 format */
    if (!info) return FALSE;

    memset(dst, 0, sizeof(*dst));

    if (info->biSize == sizeof(BITMAPCOREHEADER))
    {
        const BITMAPCOREHEADER *core = (const BITMAPCOREHEADER *)info;
        dst->biWidth         = core->bcWidth;
        dst->biHeight        = core->bcHeight;
        dst->biPlanes        = core->bcPlanes;
        dst->biBitCount      = core->bcBitCount;
        dst->biCompression   = BI_RGB;
        dst->biXPelsPerMeter = 0;
        dst->biYPelsPerMeter = 0;
        dst->biClrUsed       = 0;
        dst->biClrImportant  = 0;
    }
    else if (info->biSize >= sizeof(BITMAPINFOHEADER)) /* assume BITMAPINFOHEADER */
    {
        *dst = *info;
    }
    else
    {
        WARN( "(%lu): unknown/wrong size for header\n", info->biSize );
        return FALSE;
    }

    dst->biSize = sizeof(*dst);

    if (!is_valid_dib_format( dst, allow_compression )) return FALSE;

    colour_table_size = 0;
    if (dst->biCompression == BI_BITFIELDS)
    {
        colour_table_size = 3 * sizeof(DWORD);
    }
    else if (dst->biBitCount <= 8)
    {
        UINT elm_size = coloruse == DIB_PAL_COLORS ? sizeof(WORD) : sizeof(DWORD);
        UINT colours = dst->biClrUsed;

        if (!colours) colours = 1 << dst->biBitCount;

        /* Windows never truncates colour tables, even if they are
         * unnecessarily big (> 1<<bpp).  We emulate this behaviour. */

        if (colours > UINT_MAX / elm_size)
        {
            WARN( "too many colours in palette (%u > %u)\n",
                  colours, UINT_MAX / elm_size );
            return FALSE;
        }

        colour_table_size = colours * elm_size;
    }

    *bmi_size = sizeof(BITMAPINFOHEADER) + colour_table_size;
    if (*bmi_size < sizeof(BITMAPINFOHEADER))
        return FALSE;

    if (dst->biCompression == BI_RGB || dst->biCompression == BI_BITFIELDS)
        *img_size = get_dib_stride( dst->biWidth, dst->biBitCount ) * abs( dst->biHeight );
    else
        *img_size = dst->biSizeImage;

    return TRUE;
}

static void emf_copy_colours_from_user_bitmapinfo( BITMAPINFO *dst, const BITMAPINFO *info, UINT coloruse )
{
    if (dst->bmiHeader.biCompression == BI_BITFIELDS)
    {
        /* bitfields are always at bmiColors even in larger structures */
        memcpy( dst->bmiColors, info->bmiColors, 3 * sizeof(DWORD) );
    }
    else if (dst->bmiHeader.biBitCount <= 8)
    {
        void *src_colors = (char *)info + info->bmiHeader.biSize;
        unsigned int colors = dst->bmiHeader.biClrUsed;

        if (!colors) colors = 1 << dst->bmiHeader.biBitCount;

        if (coloruse == DIB_PAL_COLORS)
        {
            memcpy( dst->bmiColors, src_colors, colors * sizeof(WORD) );
        }
        else if (info->bmiHeader.biSize != sizeof(BITMAPCOREHEADER))
        {
            memcpy( dst->bmiColors, src_colors, colors * sizeof(RGBQUAD) );
        }
        else
        {
            unsigned int i;
            RGBTRIPLE *triple = (RGBTRIPLE *)src_colors;
            for (i = 0; i < colors; i++)
            {
                dst->bmiColors[i].rgbRed      = triple[i].rgbtRed;
                dst->bmiColors[i].rgbGreen    = triple[i].rgbtGreen;
                dst->bmiColors[i].rgbBlue     = triple[i].rgbtBlue;
                dst->bmiColors[i].rgbReserved = 0;
            }
        }
    }
}

static UINT emfdc_add_handle( struct emf *emf, HGDIOBJ obj )
{
    UINT index;

    for (index = 0; index < emf->handles_size; index++)
        if (emf->handles[index] == 0) break;

    if (index == emf->handles_size)
    {
        emf->handles_size += HANDLE_LIST_INC;
        emf->handles = HeapReAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                                    emf->handles,
                                    emf->handles_size * sizeof(emf->handles[0]) );
    }
    emf->handles[index] = obj;

    emf->cur_handles++;
    if (emf->cur_handles > emf->emh->nHandles)
        emf->emh->nHandles++;

    return index + 1; /* index 0 is reserved for the hmf, so we increment everything by 1 */
}

static UINT emfdc_find_object( struct emf *emf, HGDIOBJ obj )
{
    UINT index;

    for (index = 0; index < emf->handles_size; index++)
        if (emf->handles[index] == obj) return index + 1;

    return 0;
}

static void emfdc_delete_object( HDC hdc, HGDIOBJ obj )
{
    DC_ATTR *dc_attr = get_dc_attr( hdc );
    struct emf *emf = get_dc_emf( dc_attr );
    EMRDELETEOBJECT emr;
    UINT index;

    if(!(index = emfdc_find_object( emf, obj ))) return;

    emr.emr.iType = EMR_DELETEOBJECT;
    emr.emr.nSize = sizeof(emr);
    emr.ihObject = index;

    emfdc_record( emf, &emr.emr );

    emf->handles[index - 1] = 0;
    emf->cur_handles--;
}

static BOOL emfdc_add_palette_entry( struct emf *emf, PALETTEENTRY *entry )
{
    int i;

    for (i = 0; i < emf->palette_used; i++)
    {
        if (emf->palette[i].peRed == entry->peRed &&
                emf->palette[i].peGreen == entry->peGreen &&
                emf->palette[i].peBlue == entry->peBlue) return TRUE;
    }

    if (emf->palette_size == emf->palette_used)
    {
        if (!emf->palette_size)
        {
            emf->palette = HeapAlloc( GetProcessHeap(), 0,
                    8 * sizeof(*emf->palette) );
            if (!emf->palette) return FALSE;
            emf->palette_size = 8;
        }
        else
        {
            void *new_palette = HeapReAlloc( GetProcessHeap(), 0, emf->palette,
                    2 * emf->palette_size * sizeof(*emf->palette) );
            if (!new_palette) return FALSE;
            emf->palette = new_palette;
            emf->palette_size *= 2;
        }
    }

    emf->palette[emf->palette_used++] = *entry;
    return TRUE;
}

static DWORD emfdc_create_brush( struct emf *emf, HBRUSH brush )
{
    DWORD index = 0;
    LOGBRUSH logbrush;

    if (!GetObjectA( brush, sizeof(logbrush), &logbrush )) return 0;

    switch (logbrush.lbStyle) {
    case BS_SOLID:
    case BS_HATCHED:
    case BS_NULL:
        {
            EMRCREATEBRUSHINDIRECT emr;
            emr.emr.iType = EMR_CREATEBRUSHINDIRECT;
            emr.emr.nSize = sizeof(emr);
            emr.ihBrush = index = emfdc_add_handle( emf, brush );
            emr.lb.lbStyle = logbrush.lbStyle;
            emr.lb.lbColor = logbrush.lbColor;
            emr.lb.lbHatch = logbrush.lbHatch;

            if(!emfdc_record( emf, &emr.emr ))
                index = 0;
        }
      break;
    case BS_PATTERN:
    case BS_DIBPATTERN:
        {
            EMRCREATEDIBPATTERNBRUSHPT *emr;
            char buffer[FIELD_OFFSET( BITMAPINFO, bmiColors[256] )];
            BITMAPINFO *info = (BITMAPINFO *)buffer;
            DWORD info_size;
            UINT usage;

            if (!NtGdiIcmBrushInfo( 0, brush, info, NULL, NULL, &usage, NULL, 0 )) break;
            info_size = get_dib_info_size( info, usage );

            emr = HeapAlloc( GetProcessHeap(), 0,
                             sizeof(EMRCREATEDIBPATTERNBRUSHPT) + sizeof(DWORD) +
                             info_size+info->bmiHeader.biSizeImage );
            if(!emr) break;

            /* FIXME: There is an extra DWORD written by native before the BMI.
             *        Not sure what it's meant to contain.
             */
            emr->offBmi = sizeof( EMRCREATEDIBPATTERNBRUSHPT ) + sizeof(DWORD);
            *(DWORD *)(emr + 1) = 0x20000000;

            if (logbrush.lbStyle == BS_PATTERN && info->bmiHeader.biBitCount == 1)
            {
                /* Presumably to reduce the size of the written EMF, MS supports an
                 * undocumented iUsage value of 2, indicating a mono bitmap without the
                 * 8 byte 2 entry black/white palette. Stupidly, they could have saved
                 * over 20 bytes more by also ignoring the BITMAPINFO fields that are
                 * irrelevant/constant for monochrome bitmaps.
                 * FIXME: It may be that the DIB functions themselves accept this value.
                 */
                emr->emr.iType = EMR_CREATEMONOBRUSH;
                usage = DIB_PAL_INDICES;
                emr->cbBmi = sizeof( BITMAPINFOHEADER );
            }
            else
            {
                emr->emr.iType = EMR_CREATEDIBPATTERNBRUSHPT;
                emr->cbBmi = info_size;
            }
            emr->ihBrush = index = emfdc_add_handle( emf, brush );
            emr->iUsage = usage;
            emr->offBits = emr->offBmi + emr->cbBmi;
            emr->cbBits = info->bmiHeader.biSizeImage;
            emr->emr.nSize = emr->offBits + emr->cbBits;

            if (info->bmiHeader.biClrUsed == 1 << info->bmiHeader.biBitCount)
                info->bmiHeader.biClrUsed = 0;
            memcpy( (BYTE *)emr + emr->offBmi, info, emr->cbBmi );
            NtGdiIcmBrushInfo( 0, brush, NULL, (char *)emr + emr->offBits, NULL, NULL, NULL, 0 );

            if (!emfdc_record( emf, &emr->emr )) index = 0;
            HeapFree( GetProcessHeap(), 0, emr );
        }
        break;

    default:
        FIXME("Unknown style %x\n", logbrush.lbStyle);
        break;
    }

    return index;
}

static BOOL emfdc_select_brush( DC_ATTR *dc_attr, HBRUSH brush )
{
    struct emf *emf = get_dc_emf( dc_attr );
    EMRSELECTOBJECT emr;
    DWORD index = 0;
    int i;

    /* If the object is a stock brush object, do not need to create it.
     * See definitions in  wingdi.h for range of stock brushes.
     * We do however have to handle setting the higher order bit to
     * designate that this is a stock object.
     */
    for (i = WHITE_BRUSH; i <= DC_BRUSH; i++)
    {
        if (brush == GetStockObject(i))
        {
            index = i | 0x80000000;
            break;
        }
    }

    if (!index && !(index = emfdc_find_object( emf, brush )))
    {
        if (!(index = emfdc_create_brush( emf, brush ))) return 0;
        GDI_hdc_using_object( brush, dc_attr_handle( dc_attr ), emfdc_delete_object );
    }

    emr.emr.iType = EMR_SELECTOBJECT;
    emr.emr.nSize = sizeof(emr);
    emr.ihObject = index;
    return emfdc_record( emf, &emr.emr );
}

static BOOL emfdc_create_font( struct emf *emf, HFONT font )
{
    DWORD index = 0;
    EMREXTCREATEFONTINDIRECTW emr;
    int i;

    if (!GetObjectW( font, sizeof(emr.elfw.elfLogFont), &emr.elfw.elfLogFont )) return FALSE;

    emr.emr.iType = EMR_EXTCREATEFONTINDIRECTW;
    emr.emr.nSize = (sizeof(emr) + 3) / 4 * 4;
    emr.ihFont = index = emfdc_add_handle( emf, font );
    emr.elfw.elfFullName[0] = '\0';
    emr.elfw.elfStyle[0]    = '\0';
    emr.elfw.elfVersion     = 0;
    emr.elfw.elfStyleSize   = 0;
    emr.elfw.elfMatch       = 0;
    emr.elfw.elfReserved    = 0;
    for (i = 0; i < ELF_VENDOR_SIZE; i++)
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

    return emfdc_record( emf, &emr.emr ) ? index : 0;
}

static BOOL emfdc_select_font( DC_ATTR *dc_attr, HFONT font )
{
    struct emf *emf = get_dc_emf( dc_attr );
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
        if (i != DEFAULT_PALETTE && font == GetStockObject(i))
        {
            index = i | 0x80000000;
            goto found;
        }
    }

    if (!(index = emfdc_find_object( emf, font )))
    {
        if (!(index = emfdc_create_font( emf, font ))) return FALSE;
        GDI_hdc_using_object( font, dc_attr_handle( dc_attr ), emfdc_delete_object );
    }

 found:
    emr.emr.iType = EMR_SELECTOBJECT;
    emr.emr.nSize = sizeof(emr);
    emr.ihObject = index;
    return emfdc_record( emf, &emr.emr );
}

static DWORD emfdc_ext_create_pen( struct emf *emf, HPEN pen )
{
    EMREXTCREATEPEN *emr = NULL;
    EXTLOGPEN *elp = NULL;
    int size, emr_size;
    unsigned int i;
    DWORD ret = 0;

    if (!(size = GetObjectW( pen, 0, NULL )))
        return 0;

    elp = malloc( size );
    if (!elp)
        return 0;

    /* Native adds an extra 4 bytes, presumably because someone wasn't careful about the
     * dynamic array [1] at the end of EXTLOGPEN. Also note that GetObject returns an
     * EXTLOGPEN with a pointer-sized elpHatch, whereas a metafile always has a 32 bit
     * sized emr->elp.elpHatch .*/
    emr_size = sizeof(*emr) - sizeof(*elp) + sizeof(emr->elp.elpStyleEntry) + size;
    emr = calloc( 1, emr_size );
    if (!emr)
        goto out;
    GetObjectW( pen, size, elp );

    if (elp->elpBrushStyle == BS_DIBPATTERN ||
            elp->elpBrushStyle == BS_DIBPATTERNPT ||
            elp->elpBrushStyle == BS_PATTERN)
    {
        FIXME( "elpBrushStyle = %d\n", elp->elpBrushStyle );
        goto out;
    }
    emr->offBmi = emr->offBits = emr_size;

    emr->elp.elpPenStyle = elp->elpPenStyle;
    emr->elp.elpWidth = elp->elpWidth;
    emr->elp.elpBrushStyle = elp->elpBrushStyle;
    emr->elp.elpColor = elp->elpColor;
    emr->elp.elpHatch = elp->elpHatch;
    emr->elp.elpNumEntries = elp->elpNumEntries;

    for (i = 0; i < elp->elpNumEntries; ++i)
        emr->elp.elpStyleEntry[i] = elp->elpStyleEntry[i];

    emr->emr.iType = EMR_EXTCREATEPEN;
    emr->emr.nSize = emr_size;
    emr->ihPen = ret = emfdc_add_handle( emf, pen );
    ret = emfdc_record( emf, &emr->emr ) ? ret : 0;

out:
    free( emr );
    free( elp );
    return ret;
}

static DWORD emfdc_create_pen( struct emf *emf, HPEN hPen )
{
    EMRCREATEPEN emr;
    DWORD index = 0;

    if (!GetObjectW( hPen, sizeof(emr.lopn), &emr.lopn ))
        return emfdc_ext_create_pen( emf, hPen );

    emr.emr.iType = EMR_CREATEPEN;
    emr.emr.nSize = sizeof(emr);
    emr.ihPen = index = emfdc_add_handle( emf, hPen );
    return emfdc_record( emf, &emr.emr ) ? index : 0;
}

static BOOL emfdc_select_pen( DC_ATTR *dc_attr, HPEN pen )
{
    struct emf *emf = get_dc_emf( dc_attr );
    EMRSELECTOBJECT emr;
    DWORD index = 0;
    int i;

    /* If the object is a stock pen object, do not need to create it.
     * See definitions in  wingdi.h for range of stock pens.
     * We do however have to handle setting the higher order bit to
     * designate that this is a stock object.
     */

    for (i = WHITE_PEN; i <= DC_PEN; i++)
    {
        if (pen == GetStockObject(i))
        {
            index = i | 0x80000000;
            break;
        }
    }
    if (!index && !(index = emfdc_find_object( emf, pen )))
    {
        if (!(index = emfdc_create_pen( emf, pen ))) return FALSE;
        GDI_hdc_using_object( pen, dc_attr_handle( dc_attr ), emfdc_delete_object );
    }

    emr.emr.iType = EMR_SELECTOBJECT;
    emr.emr.nSize = sizeof(emr);
    emr.ihObject = index;
    return emfdc_record( emf, &emr.emr );
}

static DWORD emfdc_create_palette( struct emf *emf, HPALETTE hPal )
{
    BYTE data[offsetof( EMRCREATEPALETTE, lgpl.palPalEntry[256] )];
    EMRCREATEPALETTE *hdr = (EMRCREATEPALETTE *)data;
    WORD i;

    memset( data, 0, sizeof(data) );

    hdr->lgpl.palVersion = 0x300;
    hdr->lgpl.palNumEntries = GetPaletteEntries( hPal, 0, 256, hdr->lgpl.palPalEntry );
    if (!hdr->lgpl.palNumEntries)
        return 0;

    for (i = 0; i < hdr->lgpl.palNumEntries; i++)
    {
        hdr->lgpl.palPalEntry[i].peFlags = 0;
        emfdc_add_palette_entry( emf, hdr->lgpl.palPalEntry + i );
    }

    hdr->emr.iType = EMR_CREATEPALETTE;
    hdr->emr.nSize = offsetof( EMRCREATEPALETTE, lgpl.palPalEntry[hdr->lgpl.palNumEntries] );
    hdr->ihPal = emfdc_add_handle( emf, hPal );

    if (!emfdc_record( emf, &hdr->emr ))
        hdr->ihPal = 0;
    return hdr->ihPal;
}

BOOL EMFDC_SelectPalette( DC_ATTR *dc_attr, HPALETTE palette )
{
    struct emf *emf = get_dc_emf( dc_attr );
    EMRSELECTPALETTE emr;
    DWORD index = 0;

    if (palette == GetStockObject( DEFAULT_PALETTE ))
    {
        index = DEFAULT_PALETTE | 0x80000000;
    }
    else if (!(index = emfdc_find_object( emf, palette )))
    {
        if (!(index = emfdc_create_palette( emf, palette ))) return 0;
        GDI_hdc_using_object( palette, dc_attr_handle( dc_attr ), emfdc_delete_object );
    }

    emr.emr.iType = EMR_SELECTPALETTE;
    emr.emr.nSize = sizeof(emr);
    emr.ihPal = index;
    return emfdc_record( emf, &emr.emr );
}

BOOL EMFDC_RealizePalette( DC_ATTR *dc_attr )
{
    HPALETTE palette = GetCurrentObject( dc_attr_handle( dc_attr ), OBJ_PAL );
    struct emf *emf = get_dc_emf( dc_attr );
    EMRREALIZEPALETTE emr;

    if (palette == GetStockObject( DEFAULT_PALETTE ))
        return TRUE;

    emr.emr.iType = EMR_REALIZEPALETTE;
    emr.emr.nSize = sizeof(emr);
    return emfdc_record( emf, &emr.emr );
}

BOOL EMFDC_SelectObject( DC_ATTR *dc_attr, HGDIOBJ obj )
{
    switch (gdi_handle_type( obj ))
    {
    case NTGDI_OBJ_BRUSH:
        return emfdc_select_brush( dc_attr, obj );
    case NTGDI_OBJ_FONT:
        return emfdc_select_font( dc_attr, obj );
    case NTGDI_OBJ_PEN:
    case NTGDI_OBJ_EXTPEN:
        return emfdc_select_pen( dc_attr, obj );
    default:
        return TRUE;
    }
}

/* determine if we can use 16-bit points to store all the input points */
static BOOL can_use_short_points( const POINT *pts, UINT count )
{
    UINT i;

    for (i = 0; i < count; i++)
        if (((pts[i].x + 0x8000) & ~0xffff) || ((pts[i].y + 0x8000) & ~0xffff))
            return FALSE;
    return TRUE;
}

/* store points in either long or short format; return a pointer to the end of the stored data */
static void *store_points( POINTL *dest, const POINT *pts, UINT count, BOOL short_points )
{
    if (short_points)
    {
        UINT i;
        POINTS *dest_short = (POINTS *)dest;

        for (i = 0; i < count; i++)
        {
            dest_short[i].x = pts[i].x;
            dest_short[i].y = pts[i].y;
        }
        return dest_short + count;
    }
    else
    {
        memcpy( dest, pts, count * sizeof(*dest) );
        return dest + count;
    }
}

/* compute the bounds of an array of points, optionally including the current position */
static void get_points_bounds( RECTL *bounds, const POINT *pts, UINT count, DC_ATTR *dc_attr )
{
    UINT i;

    if (dc_attr)
    {
        bounds->left = bounds->right = dc_attr->cur_pos.x;
        bounds->top = bounds->bottom = dc_attr->cur_pos.y;
    }
    else if (count)
    {
        bounds->left = bounds->right = pts[0].x;
        bounds->top = bounds->bottom = pts[0].y;
    }
    else *bounds = empty_bounds;

    for (i = 0; i < count; i++)
    {
        bounds->left   = min( bounds->left, pts[i].x );
        bounds->right  = max( bounds->right, pts[i].x );
        bounds->top    = min( bounds->top, pts[i].y );
        bounds->bottom = max( bounds->bottom, pts[i].y );
    }
}

/* helper for path stroke and fill functions */
static BOOL emfdrv_stroke_and_fill_path( struct emf *emf, INT type )
{
    EMRSTROKEANDFILLPATH emr;
    HRGN region;

    emr.emr.iType = type;
    emr.emr.nSize = sizeof(emr);
    emr.rclBounds = empty_bounds;

    if ((region = NtGdiPathToRegion( dc_attr_handle( emf->dc_attr ))))
    {
        NtGdiGetRgnBox( region, (RECT *)&emr.rclBounds );
        DeleteObject( region );
    }

    if (!emfdc_record( emf, &emr.emr )) return FALSE;
    if (!region) return FALSE;
    emfdc_update_bounds( emf, &emr.rclBounds );
    return TRUE;
}

BOOL EMFDC_MoveTo( DC_ATTR *dc_attr, INT x, INT y )
{
    struct emf *emf = get_dc_emf( dc_attr );
    EMRMOVETOEX emr;

    emr.emr.iType = EMR_MOVETOEX;
    emr.emr.nSize = sizeof(emr);
    emr.ptl.x = x;
    emr.ptl.y = y;
    return emfdc_record( emf, &emr.emr );
}

BOOL EMFDC_LineTo( DC_ATTR *dc_attr, INT x, INT y )
{
    EMRLINETO emr;

    emr.emr.iType = EMR_LINETO;
    emr.emr.nSize = sizeof(emr);
    emr.ptl.x = x;
    emr.ptl.y = y;
    return emfdc_record( get_dc_emf( dc_attr ), &emr.emr );
}

BOOL EMFDC_ArcChordPie( DC_ATTR *dc_attr, INT left, INT top, INT right, INT bottom,
                        INT xstart, INT ystart, INT xend, INT yend, DWORD type )
{
    struct emf *emf = get_dc_emf( dc_attr );
    EMRARC emr;
    INT temp;

    if (left == right || top == bottom) return FALSE;

    if (left > right) { temp = left; left = right; right = temp; }
    if (top > bottom) { temp = top; top = bottom; bottom = temp; }

    if (dc_attr->graphics_mode == GM_COMPATIBLE)
    {
        right--;
        bottom--;
    }

    emr.emr.iType     = type;
    emr.emr.nSize     = sizeof(emr);
    emr.rclBox.left   = left;
    emr.rclBox.top    = top;
    emr.rclBox.right  = right;
    emr.rclBox.bottom = bottom;
    emr.ptlStart.x    = xstart;
    emr.ptlStart.y    = ystart;
    emr.ptlEnd.x      = xend;
    emr.ptlEnd.y      = yend;
    return emfdc_record( emf, &emr.emr );
}

BOOL EMFDC_AngleArc( DC_ATTR *dc_attr, INT x, INT y, DWORD radius, FLOAT start, FLOAT sweep )
{
    EMRANGLEARC emr;

    emr.emr.iType   = EMR_ANGLEARC;
    emr.emr.nSize   = sizeof( emr );
    emr.ptlCenter.x = x;
    emr.ptlCenter.y = y;
    emr.nRadius     = radius;
    emr.eStartAngle = start;
    emr.eSweepAngle = sweep;
    return emfdc_record( get_dc_emf( dc_attr ), &emr.emr );
}

BOOL EMFDC_Ellipse( DC_ATTR *dc_attr, INT left, INT top, INT right, INT bottom )
{
    struct emf *emf = get_dc_emf( dc_attr );
    EMRELLIPSE emr;

    if (left == right || top == bottom) return FALSE;

    emr.emr.iType     = EMR_ELLIPSE;
    emr.emr.nSize     = sizeof(emr);
    emr.rclBox.left   = min( left, right );
    emr.rclBox.top    = min( top, bottom );
    emr.rclBox.right  = max( left, right );
    emr.rclBox.bottom = max( top, bottom );
    if (dc_attr->graphics_mode == GM_COMPATIBLE)
    {
        emr.rclBox.right--;
        emr.rclBox.bottom--;
    }
    return emfdc_record( emf, &emr.emr );
}

BOOL EMFDC_Rectangle( DC_ATTR *dc_attr, INT left, INT top, INT right, INT bottom )
{
    struct emf *emf = get_dc_emf( dc_attr );
    EMRRECTANGLE emr;

    if(left == right || top == bottom) return FALSE;

    emr.emr.iType     = EMR_RECTANGLE;
    emr.emr.nSize     = sizeof(emr);
    emr.rclBox.left   = min( left, right );
    emr.rclBox.top    = min( top, bottom );
    emr.rclBox.right  = max( left, right );
    emr.rclBox.bottom = max( top, bottom );
    if (dc_attr->graphics_mode == GM_COMPATIBLE)
    {
        emr.rclBox.right--;
        emr.rclBox.bottom--;
    }
    return emfdc_record( emf, &emr.emr );
}

BOOL EMFDC_RoundRect( DC_ATTR *dc_attr, INT left, INT top, INT right,
                      INT bottom, INT ell_width, INT ell_height )
{
    struct emf *emf = get_dc_emf( dc_attr );
    EMRROUNDRECT emr;

    if (left == right || top == bottom) return FALSE;

    emr.emr.iType     = EMR_ROUNDRECT;
    emr.emr.nSize     = sizeof(emr);
    emr.rclBox.left   = min( left, right );
    emr.rclBox.top    = min( top, bottom );
    emr.rclBox.right  = max( left, right );
    emr.rclBox.bottom = max( top, bottom );
    emr.szlCorner.cx  = ell_width;
    emr.szlCorner.cy  = ell_height;
    if (dc_attr->graphics_mode == GM_COMPATIBLE)
    {
        emr.rclBox.right--;
        emr.rclBox.bottom--;
    }
    return emfdc_record( emf, &emr.emr );
}

BOOL EMFDC_SetPixel( DC_ATTR *dc_attr, INT x, INT y, COLORREF color )
{
    EMRSETPIXELV emr;

    emr.emr.iType  = EMR_SETPIXELV;
    emr.emr.nSize  = sizeof(emr);
    emr.ptlPixel.x = x;
    emr.ptlPixel.y = y;
    emr.crColor = color;
    return emfdc_record( get_dc_emf( dc_attr ), &emr.emr );
}

static BOOL emfdc_polylinegon( DC_ATTR *dc_attr, const POINT *points, INT count, DWORD type )
{
    struct emf *emf = get_dc_emf( dc_attr );
    EMRPOLYLINE *emr;
    DWORD size;
    BOOL ret, use_small_emr = can_use_short_points( points, count );

    size = use_small_emr ? offsetof( EMRPOLYLINE16, apts[count] ) : offsetof( EMRPOLYLINE, aptl[count] );

    emr = HeapAlloc( GetProcessHeap(), 0, size );
    emr->emr.iType = use_small_emr ? type + EMR_POLYLINE16 - EMR_POLYLINE : type;
    emr->emr.nSize = size;
    emr->cptl = count;

    store_points( emr->aptl, points, count, use_small_emr );

    if (!emf->path)
        get_points_bounds( &emr->rclBounds, points, count,
                           (type == EMR_POLYBEZIERTO || type == EMR_POLYLINETO) ? dc_attr : 0 );
    else
        emr->rclBounds = empty_bounds;

    ret = emfdc_record( emf, &emr->emr );
    if (ret && !emf->path) emfdc_update_bounds( emf, &emr->rclBounds );
    HeapFree( GetProcessHeap(), 0, emr );
    return ret;
}

BOOL EMFDC_Polyline( DC_ATTR *dc_attr, const POINT *points, INT count )
{
    return emfdc_polylinegon( dc_attr, points, count, EMR_POLYLINE );
}

BOOL EMFDC_PolylineTo( DC_ATTR *dc_attr, const POINT *points, INT count )
{
    return emfdc_polylinegon( dc_attr, points, count, EMR_POLYLINETO );
}

BOOL EMFDC_Polygon( DC_ATTR *dc_attr, const POINT *pt, INT count )
{
    if(count < 2) return FALSE;
    return emfdc_polylinegon( dc_attr, pt, count, EMR_POLYGON );
}

BOOL EMFDC_PolyBezier( DC_ATTR *dc_attr, const POINT *pts, DWORD count )
{
    return emfdc_polylinegon( dc_attr, pts, count, EMR_POLYBEZIER );
}

BOOL EMFDC_PolyBezierTo( DC_ATTR *dc_attr, const POINT *pts, DWORD count )
{
    return emfdc_polylinegon( dc_attr, pts, count, EMR_POLYBEZIERTO );
}

static BOOL emfdc_poly_polylinegon( struct emf *emf, const POINT *pt, const INT *counts,
                                   UINT polys, DWORD type)
{
    EMRPOLYPOLYLINE *emr;
    DWORD cptl = 0, poly, size;
    BOOL ret, use_small_emr, bounds_valid = TRUE;

    for(poly = 0; poly < polys; poly++) {
        cptl += counts[poly];
        if(counts[poly] < 2) bounds_valid = FALSE;
    }
    if(!cptl) bounds_valid = FALSE;
    use_small_emr = can_use_short_points( pt, cptl );

    size = FIELD_OFFSET(EMRPOLYPOLYLINE, aPolyCounts[polys]);
    if(use_small_emr)
        size += cptl * sizeof(POINTS);
    else
        size += cptl * sizeof(POINTL);

    emr = HeapAlloc( GetProcessHeap(), 0, size );

    emr->emr.iType = type;
    if(use_small_emr) emr->emr.iType += EMR_POLYPOLYLINE16 - EMR_POLYPOLYLINE;

    emr->emr.nSize = size;
    if(bounds_valid && !emf->path)
        get_points_bounds( &emr->rclBounds, pt, cptl, 0 );
    else
        emr->rclBounds = empty_bounds;
    emr->nPolys = polys;
    emr->cptl = cptl;

    if(polys)
    {
        memcpy( emr->aPolyCounts, counts, polys * sizeof(DWORD) );
        store_points( (POINTL *)(emr->aPolyCounts + polys), pt, cptl, use_small_emr );
    }

    ret = emfdc_record( emf, &emr->emr );
    if(ret && !bounds_valid)
    {
        ret = FALSE;
        SetLastError( ERROR_INVALID_PARAMETER );
    }
    if(ret && !emf->path)
        emfdc_update_bounds( emf, &emr->rclBounds );
    HeapFree( GetProcessHeap(), 0, emr );
    return ret;
}

BOOL EMFDC_PolyPolyline( DC_ATTR *dc_attr, const POINT *pt, const DWORD *counts, DWORD polys)
{
    return emfdc_poly_polylinegon( get_dc_emf( dc_attr ), pt, (const INT *)counts, polys, EMR_POLYPOLYLINE );
}

BOOL EMFDC_PolyPolygon( DC_ATTR *dc_attr, const POINT *pt, const INT *counts, UINT polys )
{
    return emfdc_poly_polylinegon( get_dc_emf( dc_attr ), pt, counts, polys, EMR_POLYPOLYGON );
}

BOOL EMFDC_PolyDraw( DC_ATTR *dc_attr, const POINT *pts, const BYTE *types, DWORD count )
{
    struct emf *emf = get_dc_emf( dc_attr );
    EMRPOLYDRAW *emr;
    BOOL ret;
    BYTE *types_dest;
    BOOL use_small_emr = can_use_short_points( pts, count );
    DWORD size;

    size = use_small_emr ? offsetof( EMRPOLYDRAW16, apts[count] )
        : offsetof( EMRPOLYDRAW, aptl[count] );
    size += aligned_size(count);

    if (!(emr = HeapAlloc( GetProcessHeap(), 0, size ))) return FALSE;

    emr->emr.iType = use_small_emr ? EMR_POLYDRAW16 : EMR_POLYDRAW;
    emr->emr.nSize = size;
    emr->cptl = count;

    types_dest = store_points( emr->aptl, pts, count, use_small_emr );
    memcpy( types_dest, types, count );
    pad_record( types_dest, count );

    if (!emf->path)
        get_points_bounds( &emr->rclBounds, pts, count, 0 );
    else
        emr->rclBounds = empty_bounds;

    ret = emfdc_record( emf, &emr->emr );
    if (ret && !emf->path) emfdc_update_bounds( emf, &emr->rclBounds );
    HeapFree( GetProcessHeap(), 0, emr );
    return ret;
}

INT EMFDC_ExtEscape( DC_ATTR *dc_attr, INT escape, INT input_size, const char *input,
                      INT output_size, char *output)
{
    EMREXTESCAPE *emr;
    size_t size;

    if (escape == QUERYESCSUPPORT) return 0;

    size = aligned_size(FIELD_OFFSET( EMREXTESCAPE, EscData[input_size] ));
    if (!(emr = HeapAlloc( GetProcessHeap(), 0, size ))) return 0;

    emr->emr.iType = EMR_EXTESCAPE;
    emr->emr.nSize = size;
    emr->iEscape = escape;
    emr->cbEscData = input_size;
    memcpy(emr->EscData, input, input_size);
    pad_record(emr->EscData, input_size);
    emfdc_record( get_dc_emf( dc_attr ), &emr->emr );
    HeapFree( GetProcessHeap(), 0, emr );
    if (output_size && output) return 0;

    if (escape == PASSTHROUGH || escape == POSTSCRIPT_PASSTHROUGH)
        input_size -= sizeof(WORD);
    return input_size ? input_size : 1;
}

BOOL EMFDC_ExtFloodFill( DC_ATTR *dc_attr, INT x, INT y, COLORREF color, UINT fill_type )
{
    EMREXTFLOODFILL emr;

    emr.emr.iType = EMR_EXTFLOODFILL;
    emr.emr.nSize = sizeof(emr);
    emr.ptlStart.x = x;
    emr.ptlStart.y = y;
    emr.crColor = color;
    emr.iMode = fill_type;
    return emfdc_record( get_dc_emf( dc_attr ), &emr.emr );
}

BOOL EMFDC_FillRgn( DC_ATTR *dc_attr, HRGN hrgn, HBRUSH hbrush )
{
    struct emf *emf = get_dc_emf( dc_attr );
    EMRFILLRGN *emr;
    DWORD size, rgnsize, index;
    BOOL ret;

    if (!(index = emfdc_create_brush( emf, hbrush ))) return FALSE;

    rgnsize = NtGdiGetRegionData( hrgn, 0, NULL );
    size = rgnsize + offsetof(EMRFILLRGN,RgnData);
    emr = HeapAlloc( GetProcessHeap(), 0, size );

    NtGdiGetRegionData( hrgn, rgnsize, (RGNDATA *)&emr->RgnData );

    emr->emr.iType = EMR_FILLRGN;
    emr->emr.nSize = size;
    emr->rclBounds.left   = ((RGNDATA *)&emr->RgnData)->rdh.rcBound.left;
    emr->rclBounds.top    = ((RGNDATA *)&emr->RgnData)->rdh.rcBound.top;
    emr->rclBounds.right  = ((RGNDATA *)&emr->RgnData)->rdh.rcBound.right - 1;
    emr->rclBounds.bottom = ((RGNDATA *)&emr->RgnData)->rdh.rcBound.bottom - 1;
    emr->cbRgnData = rgnsize;
    emr->ihBrush = index;

    ret = emfdc_record( emf, &emr->emr );
    if (ret) emfdc_update_bounds( emf, &emr->rclBounds );
    HeapFree( GetProcessHeap(), 0, emr );
    return ret;
}

BOOL EMFDC_FrameRgn( DC_ATTR *dc_attr, HRGN hrgn, HBRUSH hbrush, INT width, INT height )
{
    struct emf *emf = get_dc_emf( dc_attr );
    EMRFRAMERGN *emr;
    DWORD size, rgnsize, index;
    BOOL ret;

    index = emfdc_create_brush( emf, hbrush );
    if(!index) return FALSE;

    rgnsize = NtGdiGetRegionData( hrgn, 0, NULL );
    size = rgnsize + offsetof(EMRFRAMERGN,RgnData);
    emr = HeapAlloc( GetProcessHeap(), 0, size );

    NtGdiGetRegionData( hrgn, rgnsize, (RGNDATA *)&emr->RgnData );

    emr->emr.iType = EMR_FRAMERGN;
    emr->emr.nSize = size;
    emr->rclBounds.left   = ((RGNDATA *)&emr->RgnData)->rdh.rcBound.left;
    emr->rclBounds.top    = ((RGNDATA *)&emr->RgnData)->rdh.rcBound.top;
    emr->rclBounds.right  = ((RGNDATA *)&emr->RgnData)->rdh.rcBound.right - 1;
    emr->rclBounds.bottom = ((RGNDATA *)&emr->RgnData)->rdh.rcBound.bottom - 1;
    emr->cbRgnData = rgnsize;
    emr->ihBrush = index;
    emr->szlStroke.cx = width;
    emr->szlStroke.cy = height;

    ret = emfdc_record( emf, &emr->emr );
    if (ret) emfdc_update_bounds( emf, &emr->rclBounds );
    HeapFree( GetProcessHeap(), 0, emr );
    return ret;
}

static BOOL emfdc_paint_invert_region( struct emf *emf, HRGN hrgn, DWORD iType )
{
    EMRINVERTRGN *emr;
    DWORD size, rgnsize;
    BOOL ret;

    rgnsize = NtGdiGetRegionData( hrgn, 0, NULL );
    size = rgnsize + offsetof(EMRINVERTRGN,RgnData);
    emr = HeapAlloc( GetProcessHeap(), 0, size );

    NtGdiGetRegionData( hrgn, rgnsize, (RGNDATA *)&emr->RgnData );

    emr->emr.iType = iType;
    emr->emr.nSize = size;
    emr->rclBounds.left   = ((RGNDATA *)&emr->RgnData)->rdh.rcBound.left;
    emr->rclBounds.top    = ((RGNDATA *)&emr->RgnData)->rdh.rcBound.top;
    emr->rclBounds.right  = ((RGNDATA *)&emr->RgnData)->rdh.rcBound.right - 1;
    emr->rclBounds.bottom = ((RGNDATA *)&emr->RgnData)->rdh.rcBound.bottom - 1;
    emr->cbRgnData = rgnsize;

    ret = emfdc_record( emf, &emr->emr );
    if (ret) emfdc_update_bounds( emf, &emr->rclBounds );
    HeapFree( GetProcessHeap(), 0, emr );
    return ret;
}

BOOL EMFDC_PaintRgn( DC_ATTR *dc_attr, HRGN hrgn )
{
    return emfdc_paint_invert_region( get_dc_emf( dc_attr ), hrgn, EMR_PAINTRGN );
}

BOOL EMFDC_InvertRgn( DC_ATTR *dc_attr, HRGN hrgn )
{
    return emfdc_paint_invert_region( get_dc_emf( dc_attr ), hrgn, EMR_INVERTRGN );
}

BOOL EMFDC_ExtTextOut( DC_ATTR *dc_attr, INT x, INT y, UINT flags, const RECT *rect,
                       const WCHAR *str, UINT count, const INT *dx )
{
    struct emf *emf = get_dc_emf( dc_attr );
    HDC hdc = dc_attr_handle( dc_attr );
    FLOAT ex_scale, ey_scale;
    EMREXTTEXTOUTW *emr;
    int text_height = 0, top = y, bottom = y;
    int text_width = 0;
    TEXTMETRICW tm;
    DWORD dx_len, size;
    BOOL ret;

    if (!dx && flags & ETO_PDY) return FALSE;
    dx_len = flags & ETO_PDY ? count * 2 : count;
    size = sizeof(*emr) + ((count+1) & ~1) * sizeof(WCHAR) + dx_len * sizeof(INT);

    TRACE( "%s %s count %d size = %ld\n", debugstr_wn(str, count),
           wine_dbgstr_rect(rect), count, size );
    emr = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, size );

    if (dc_attr->graphics_mode == GM_COMPATIBLE)
    {
        HDC hdc = dc_attr_handle( dc_attr );
        const INT horzSize = GetDeviceCaps( hdc, HORZSIZE );
        const INT horzRes  = GetDeviceCaps( hdc, HORZRES );
        const INT vertSize = GetDeviceCaps( hdc, VERTSIZE );
        const INT vertRes  = GetDeviceCaps( hdc, VERTRES );
        SIZE wndext, vportext;

        GetViewportExtEx( hdc, &vportext );
        GetWindowExtEx( hdc, &wndext );
        ex_scale = 100.0 * ((FLOAT)horzSize  / (FLOAT)horzRes) /
            ((FLOAT)wndext.cx / (FLOAT)vportext.cx);
        ey_scale = 100.0 * ((FLOAT)vertSize  / (FLOAT)vertRes) /
            ((FLOAT)wndext.cy / (FLOAT)vportext.cy);
    }
    else
    {
        ex_scale = 0.0;
        ey_scale = 0.0;
    }

    emr->emr.iType = EMR_EXTTEXTOUTW;
    emr->emr.nSize = size;
    emr->iGraphicsMode = dc_attr->graphics_mode;
    emr->exScale = ex_scale;
    emr->eyScale = ey_scale;
    emr->emrtext.ptlReference.x = x;
    emr->emrtext.ptlReference.y = y;
    emr->emrtext.nChars = count;
    emr->emrtext.offString = sizeof(*emr);
    memcpy( (char*)emr + emr->emrtext.offString, str, count * sizeof(WCHAR) );
    emr->emrtext.fOptions = flags;
    if (!rect)
    {
        emr->emrtext.rcl.left = emr->emrtext.rcl.top = 0;
        emr->emrtext.rcl.right = emr->emrtext.rcl.bottom = -1;
    }
    else
    {
        emr->emrtext.rcl.left = rect->left;
        emr->emrtext.rcl.top = rect->top;
        emr->emrtext.rcl.right = rect->right;
        emr->emrtext.rcl.bottom = rect->bottom;
    }

    emr->emrtext.offDx = emr->emrtext.offString + ((count+1) & ~1) * sizeof(WCHAR);
    if (dx)
    {
        UINT i;
        SIZE str_size;
        memcpy( (char*)emr + emr->emrtext.offDx, dx, dx_len * sizeof(INT) );
        if (GetTextExtentPoint32W( hdc, str, count, &str_size ))
            text_height = str_size.cy;
        if (flags & ETO_PDY)
        {
            int cur_y = y;
            for (i = 0; i < count; i++)
            {
                top = min( top, cur_y );
                bottom = max( bottom, cur_y );
                text_width += dx[2 * i];
                cur_y -= dx[2 * i + 1];
            }
        }
        else
        {
            for (i = 0; i < count; i++) text_width += dx[i];
        }
    }
    else
    {
        UINT i;
        INT *emf_dx = (INT *)((char*)emr + emr->emrtext.offDx);
        SIZE charSize;
        for (i = 0; i < count; i++)
        {
            if (GetTextExtentPoint32W( hdc, str + i, 1, &charSize ))
            {
                emf_dx[i] = charSize.cx;
                text_width += charSize.cx;
                text_height = max( text_height, charSize.cy );
            }
        }
    }

    if (emf->path)
    {
        emr->rclBounds.left = emr->rclBounds.top = 0;
        emr->rclBounds.right = emr->rclBounds.bottom = -1;
        goto no_bounds;
    }

    /* FIXME: handle font escapement */
    switch (dc_attr->text_align & (TA_LEFT | TA_RIGHT | TA_CENTER))
    {
    case TA_CENTER:
        emr->rclBounds.left  = x - (text_width / 2) - 1;
        emr->rclBounds.right = x + (text_width / 2) + 1;
        break;

    case TA_RIGHT:
        emr->rclBounds.left  = x - text_width - 1;
        emr->rclBounds.right = x;
        break;

    default: /* TA_LEFT */
        emr->rclBounds.left  = x;
        emr->rclBounds.right = x + text_width + 1;
    }

    switch (dc_attr->text_align & (TA_TOP | TA_BOTTOM | TA_BASELINE))
    {
    case TA_BASELINE:
        if (!GetTextMetricsW( hdc, &tm )) tm.tmDescent = 0;
        /* Play safe here... it's better to have a bounding box */
        /* that is too big than too small. */
        emr->rclBounds.top    = top - text_height - 1;
        emr->rclBounds.bottom = bottom + tm.tmDescent + 1;
        break;

    case TA_BOTTOM:
        emr->rclBounds.top    = top - text_height - 1;
        emr->rclBounds.bottom = bottom;
        break;

    default: /* TA_TOP */
        emr->rclBounds.top    = top;
        emr->rclBounds.bottom = bottom + text_height + 1;
    }
    emfdc_update_bounds( emf, &emr->rclBounds );

no_bounds:
    ret = emfdc_record( emf, &emr->emr );
    HeapFree( GetProcessHeap(), 0, emr );
    return ret;
}

BOOL EMFDC_GradientFill( DC_ATTR *dc_attr, TRIVERTEX *vert_array, ULONG nvert,
                         void *grad_array, ULONG ngrad, ULONG mode )
{
    EMRGRADIENTFILL *emr;
    ULONG i, pt, size, num_pts = ngrad * (mode == GRADIENT_FILL_TRIANGLE ? 3 : 2);
    const ULONG *pts = (const ULONG *)grad_array;
    BOOL ret;

    size = FIELD_OFFSET(EMRGRADIENTFILL, Ver[nvert]) + num_pts * sizeof(pts[0]);

    emr = HeapAlloc( GetProcessHeap(), 0, size );
    if (!emr) return FALSE;

    for (i = 0; i < num_pts; i++)
    {
        pt = pts[i];

        if (i == 0)
        {
            emr->rclBounds.left = emr->rclBounds.right = vert_array[pt].x;
            emr->rclBounds.top = emr->rclBounds.bottom = vert_array[pt].y;
        }
        else
        {
            if (vert_array[pt].x < emr->rclBounds.left)
                emr->rclBounds.left = vert_array[pt].x;
            else if (vert_array[pt].x > emr->rclBounds.right)
                emr->rclBounds.right = vert_array[pt].x;
            if (vert_array[pt].y < emr->rclBounds.top)
                emr->rclBounds.top = vert_array[pt].y;
            else if (vert_array[pt].y > emr->rclBounds.bottom)
                emr->rclBounds.bottom = vert_array[pt].y;
        }
    }
    emr->rclBounds.right--;
    emr->rclBounds.bottom--;

    emr->emr.iType = EMR_GRADIENTFILL;
    emr->emr.nSize = size;
    emr->nVer = nvert;
    emr->nTri = ngrad;
    emr->ulMode = mode;
    memcpy( emr->Ver, vert_array, nvert * sizeof(vert_array[0]) );
    memcpy( emr->Ver + nvert, pts, num_pts * sizeof(pts[0]) );

    emfdc_update_bounds( get_dc_emf( dc_attr ), &emr->rclBounds );
    ret = emfdc_record( get_dc_emf( dc_attr ), &emr->emr );
    HeapFree( GetProcessHeap(), 0, emr );
    return ret;
}

BOOL EMFDC_FillPath( DC_ATTR *dc_attr )
{
    return emfdrv_stroke_and_fill_path( get_dc_emf( dc_attr ), EMR_FILLPATH );
}

BOOL EMFDC_StrokeAndFillPath( DC_ATTR *dc_attr )
{
    return emfdrv_stroke_and_fill_path( get_dc_emf( dc_attr ), EMR_STROKEANDFILLPATH );
}

BOOL EMFDC_StrokePath( DC_ATTR *dc_attr )
{
    return emfdrv_stroke_and_fill_path( get_dc_emf( dc_attr ), EMR_STROKEPATH );
}

/* Generate an EMRBITBLT, EMRSTRETCHBLT or EMRALPHABLEND record depending on the type parameter */
static BOOL emfdrv_stretchblt( struct emf *emf, INT x_dst, INT y_dst, INT width_dst, INT height_dst,
                               HDC hdc_src, INT x_src, INT y_src, INT width_src, INT height_src,
                               DWORD rop, DWORD type )
{
    BITMAPINFO src_info = {{ sizeof( src_info.bmiHeader ) }};
    UINT bmi_size, emr_size, size;
    HBITMAP bitmap, blit_bitmap = NULL;
    EMRBITBLT *emr = NULL;
    BITMAPINFO *bmi;
    HDC blit_dc;
    BOOL ret = FALSE;

    if (!(bitmap = GetCurrentObject( hdc_src, OBJ_BITMAP ))) return FALSE;

    blit_dc = hdc_src;
    blit_bitmap = bitmap;
    if (!(bmi_size = get_bitmap_info( &blit_dc, &blit_bitmap, &src_info ))) return FALSE;

    /* EMRSTRETCHBLT and EMRALPHABLEND have the same structure */
    emr_size = type == EMR_BITBLT ? sizeof(EMRBITBLT) : sizeof(EMRSTRETCHBLT);
    size = emr_size + bmi_size + src_info.bmiHeader.biSizeImage;

    if (!(emr = HeapAlloc(GetProcessHeap(), 0, size))) goto err;

    emr->emr.iType = type;
    emr->emr.nSize = size;
    emr->rclBounds.left = x_dst;
    emr->rclBounds.top = y_dst;
    emr->rclBounds.right = x_dst + width_dst - 1;
    emr->rclBounds.bottom = y_dst + height_dst - 1;
    emr->xDest = x_dst;
    emr->yDest = y_dst;
    emr->cxDest = width_dst;
    emr->cyDest = height_dst;
    emr->xSrc = x_src;
    emr->ySrc = y_src;
    if (type != EMR_BITBLT)
    {
        EMRSTRETCHBLT *emr_stretchblt = (EMRSTRETCHBLT *)emr;
        emr_stretchblt->cxSrc = width_src;
        emr_stretchblt->cySrc = height_src;
    }
    emr->dwRop = rop;
    NtGdiGetTransform( hdc_src, 0x204, &emr->xformSrc );
    emr->crBkColorSrc = GetBkColor( hdc_src );
    emr->iUsageSrc = DIB_RGB_COLORS;
    emr->offBmiSrc = emr_size;
    emr->cbBmiSrc = bmi_size;
    emr->offBitsSrc = emr_size + bmi_size;
    emr->cbBitsSrc = src_info.bmiHeader.biSizeImage;

    bmi = (BITMAPINFO *)((BYTE *)emr + emr->offBmiSrc);
    bmi->bmiHeader = src_info.bmiHeader;
    ret = GetDIBits( blit_dc, blit_bitmap, 0, src_info.bmiHeader.biHeight,
                     (BYTE *)emr + emr->offBitsSrc, bmi, DIB_RGB_COLORS );
    if (ret)
    {
        ret = emfdc_record( emf, (EMR *)emr );
        if (ret) emfdc_update_bounds( emf, &emr->rclBounds );
    }

err:
    HeapFree( GetProcessHeap(), 0, emr );
    if (blit_bitmap != bitmap) DeleteObject( blit_bitmap );
    if (blit_dc != hdc_src) DeleteDC( blit_dc );
    return ret;
}

BOOL EMFDC_AlphaBlend( DC_ATTR *dc_attr, INT x_dst, INT y_dst, INT width_dst, INT height_dst,
                       HDC hdc_src, INT x_src, INT y_src, INT width_src, INT height_src,
                       BLENDFUNCTION blend_function )
{
    return emfdrv_stretchblt( get_dc_emf( dc_attr ), x_dst, y_dst, width_dst, height_dst, hdc_src,
                              x_src, y_src, width_src, height_src, *(DWORD *)&blend_function,
                              EMR_ALPHABLEND );
}

BOOL EMFDC_PatBlt( DC_ATTR *dc_attr, INT left, INT top, INT width, INT height, DWORD rop )
{
    struct emf *emf = get_dc_emf( dc_attr );
    EMRBITBLT emr;
    BOOL ret;

    emr.emr.iType = EMR_BITBLT;
    emr.emr.nSize = sizeof(emr);
    emr.rclBounds.left = left;
    emr.rclBounds.top = top;
    emr.rclBounds.right = left + width - 1;
    emr.rclBounds.bottom = top + height - 1;
    emr.xDest = left;
    emr.yDest = top;
    emr.cxDest = width;
    emr.cyDest = height;
    emr.dwRop = rop;
    emr.xSrc = 0;
    emr.ySrc = 0;
    emr.xformSrc.eM11 = 1.0;
    emr.xformSrc.eM12 = 0.0;
    emr.xformSrc.eM21 = 0.0;
    emr.xformSrc.eM22 = 1.0;
    emr.xformSrc.eDx = 0.0;
    emr.xformSrc.eDy = 0.0;
    emr.crBkColorSrc = 0;
    emr.iUsageSrc = 0;
    emr.offBmiSrc = 0;
    emr.cbBmiSrc = 0;
    emr.offBitsSrc = 0;
    emr.cbBitsSrc = 0;

    ret = emfdc_record( emf, &emr.emr );
    if (ret) emfdc_update_bounds( emf, &emr.rclBounds );
    return ret;
}

static inline BOOL rop_uses_src( DWORD rop )
{
    return ((rop >> 2) & 0x330000) != (rop & 0x330000);
}

BOOL EMFDC_BitBlt( DC_ATTR *dc_attr, INT x_dst, INT y_dst, INT width, INT height,
                   HDC hdc_src, INT x_src, INT y_src, DWORD rop )
{
    if (!rop_uses_src( rop )) return EMFDC_PatBlt( dc_attr, x_dst, y_dst, width, height, rop );
    return emfdrv_stretchblt( get_dc_emf( dc_attr ), x_dst, y_dst, width, height,
                              hdc_src, x_src, y_src, width, height, rop, EMR_BITBLT );
}

BOOL EMFDC_StretchBlt( DC_ATTR *dc_attr, INT x_dst, INT y_dst, INT width_dst, INT height_dst,
                       HDC hdc_src, INT x_src, INT y_src, INT width_src, INT height_src,
                       DWORD rop )
{
    if (!rop_uses_src( rop )) return EMFDC_PatBlt( dc_attr, x_dst, y_dst, width_dst, height_dst, rop );
    return emfdrv_stretchblt( get_dc_emf( dc_attr ), x_dst, y_dst, width_dst, height_dst,
                              hdc_src, x_src, y_src, width_src,
                              height_src, rop, EMR_STRETCHBLT );
}

BOOL EMFDC_TransparentBlt( DC_ATTR *dc_attr, int x_dst, int y_dst, int width_dst, int height_dst,
                           HDC hdc_src, int x_src, int y_src, int width_src, int height_src,
                           UINT color )
{
    return emfdrv_stretchblt( get_dc_emf( dc_attr ), x_dst, y_dst, width_dst, height_dst,
                              hdc_src, x_src, y_src, width_src,
                              height_src, color, EMR_TRANSPARENTBLT );
}

BOOL EMFDC_MaskBlt( DC_ATTR *dc_attr, INT x_dst, INT y_dst, INT width_dst, INT height_dst,
                    HDC hdc_src, INT x_src, INT y_src, HBITMAP mask,
                    INT x_mask, INT y_mask, DWORD rop )
{
    unsigned char mask_info_buffer[FIELD_OFFSET(BITMAPINFO, bmiColors[256])];
    BITMAPINFO *mask_bits_info = (BITMAPINFO *)mask_info_buffer;
    struct emf *emf = get_dc_emf( dc_attr );
    BITMAPINFO mask_info = {{ sizeof( mask_info.bmiHeader ) }};
    BITMAPINFO src_info = {{ sizeof( src_info.bmiHeader ) }};
    HBITMAP bitmap, blit_bitmap = NULL, mask_bitmap = NULL;
    UINT bmi_size, size, mask_info_size = 0;
    EMRMASKBLT *emr = NULL;
    BITMAPINFO *bmi;
    HDC blit_dc, mask_dc = NULL;
    BOOL ret = FALSE;

    if (!rop_uses_src( rop ))
        return EMFDC_PatBlt( dc_attr, x_dst, y_dst, width_dst, height_dst, rop );

    if (!(bitmap = GetCurrentObject( hdc_src, OBJ_BITMAP ))) return FALSE;
    blit_dc = hdc_src;
    blit_bitmap = bitmap;
    if (!(bmi_size = get_bitmap_info( &blit_dc, &blit_bitmap, &src_info ))) return FALSE;

    if (mask)
    {
        mask_dc = hdc_src;
        mask_bitmap = mask;
        if (!(mask_info_size = get_bitmap_info( &mask_dc, &mask_bitmap, &mask_info ))) goto err;
        if (mask_info.bmiHeader.biBitCount == 1)
            mask_info_size = sizeof(BITMAPINFOHEADER); /* don't include colors */
    }
    else mask_info.bmiHeader.biSizeImage = 0;

    size = sizeof(*emr) + bmi_size + src_info.bmiHeader.biSizeImage +
        mask_info_size + mask_info.bmiHeader.biSizeImage;

    if (!(emr = HeapAlloc(GetProcessHeap(), 0, size))) goto err;

    emr->emr.iType = EMR_MASKBLT;
    emr->emr.nSize = size;
    emr->rclBounds.left = x_dst;
    emr->rclBounds.top = y_dst;
    emr->rclBounds.right = x_dst + width_dst - 1;
    emr->rclBounds.bottom = y_dst + height_dst - 1;
    emr->xDest = x_dst;
    emr->yDest = y_dst;
    emr->cxDest = width_dst;
    emr->cyDest = height_dst;
    emr->dwRop = rop;
    emr->xSrc = x_src;
    emr->ySrc = y_src;
    NtGdiGetTransform( hdc_src, 0x204, &emr->xformSrc );
    emr->crBkColorSrc = GetBkColor( hdc_src );
    emr->iUsageSrc = DIB_RGB_COLORS;
    emr->offBmiSrc = sizeof(*emr);
    emr->cbBmiSrc = bmi_size;
    emr->offBitsSrc = emr->offBmiSrc + bmi_size;
    emr->cbBitsSrc = src_info.bmiHeader.biSizeImage;
    emr->xMask = x_mask;
    emr->yMask = y_mask;
    emr->iUsageMask = DIB_PAL_INDICES;
    emr->offBmiMask = mask_info_size ? emr->offBitsSrc + emr->cbBitsSrc : 0;
    emr->cbBmiMask = mask_info_size;
    emr->offBitsMask = emr->offBmiMask + emr->cbBmiMask;
    emr->cbBitsMask = mask_info.bmiHeader.biSizeImage;

    bmi = (BITMAPINFO *)((char *)emr + emr->offBmiSrc);
    bmi->bmiHeader = src_info.bmiHeader;
    ret = GetDIBits( blit_dc, blit_bitmap, 0, src_info.bmiHeader.biHeight,
                     (char *)emr + emr->offBitsSrc, bmi, DIB_RGB_COLORS );
    if (!ret) goto err;

    if (mask_info_size)
    {
        mask_bits_info->bmiHeader = mask_info.bmiHeader;
        ret = GetDIBits( blit_dc, mask_bitmap, 0, mask_info.bmiHeader.biHeight,
                         (char *)emr + emr->offBitsMask, mask_bits_info, DIB_RGB_COLORS );
        if (ret) memcpy( (char *)emr + emr->offBmiMask, mask_bits_info, mask_info_size );
    }

    if (ret)
    {
        ret = emfdc_record( emf, (EMR *)emr );
        if (ret) emfdc_update_bounds( emf, &emr->rclBounds );
    }

err:
    HeapFree( GetProcessHeap(), 0, emr );
    if (mask_bitmap != mask) DeleteObject( mask_bitmap );
    if (mask_dc != hdc_src) DeleteObject( mask_dc );
    if (blit_bitmap != bitmap) DeleteObject( blit_bitmap );
    if (blit_dc != hdc_src) DeleteDC( blit_dc );
    return ret;
}

BOOL EMFDC_PlgBlt( DC_ATTR *dc_attr, const POINT *points, HDC hdc_src, INT x_src, INT y_src,
                   INT width, INT height, HBITMAP mask, INT x_mask, INT y_mask )
{
    unsigned char mask_info_buffer[FIELD_OFFSET(BITMAPINFO, bmiColors[256])];
    BITMAPINFO *mask_bits_info = (BITMAPINFO *)mask_info_buffer;
    struct emf *emf = get_dc_emf( dc_attr );
    BITMAPINFO mask_info = {{ sizeof( mask_info.bmiHeader ) }};
    BITMAPINFO src_info = {{ sizeof( src_info.bmiHeader ) }};
    HBITMAP bitmap, blit_bitmap = NULL, mask_bitmap = NULL;
    UINT bmi_size, size, mask_info_size = 0;
    EMRPLGBLT *emr = NULL;
    BITMAPINFO *bmi;
    HDC blit_dc, mask_dc = NULL;
    int x_min, y_min, x_max, y_max, i;
    BOOL ret = FALSE;

    if (!(bitmap = GetCurrentObject( hdc_src, OBJ_BITMAP ))) return FALSE;

    blit_dc = hdc_src;
    blit_bitmap = bitmap;
    if (!(bmi_size = get_bitmap_info( &blit_dc, &blit_bitmap, &src_info ))) return FALSE;

    if (mask)
    {
        mask_dc = hdc_src;
        mask_bitmap = mask;
        if (!(mask_info_size = get_bitmap_info( &mask_dc, &mask_bitmap, &mask_info ))) goto err;
        if (mask_info.bmiHeader.biBitCount == 1)
            mask_info_size = sizeof(BITMAPINFOHEADER); /* don't include colors */
    }
    else mask_info.bmiHeader.biSizeImage = 0;

    size = sizeof(*emr) + bmi_size + src_info.bmiHeader.biSizeImage +
        mask_info_size + mask_info.bmiHeader.biSizeImage;

    if (!(emr = HeapAlloc(GetProcessHeap(), 0, size))) goto err;

    emr->emr.iType = EMR_PLGBLT;
    emr->emr.nSize = size;

    /* FIXME: not exactly what native does */
    x_min = x_max = points[1].x + points[2].x - points[0].x;
    y_min = y_max = points[1].y + points[2].y - points[0].y;
    for (i = 0; i < ARRAYSIZE(emr->aptlDest); i++)
    {
        x_min = min( x_min, points[i].x );
        y_min = min( y_min, points[i].y );
        x_max = max( x_max, points[i].x );
        y_max = max( y_min, points[i].y );
    }
    emr->rclBounds.left = x_min;
    emr->rclBounds.top = y_min;
    emr->rclBounds.right = x_max;
    emr->rclBounds.bottom = y_max;
    memcpy( emr->aptlDest, points, sizeof(emr->aptlDest) );
    emr->xSrc = x_src;
    emr->ySrc = y_src;
    emr->cxSrc = width;
    emr->cySrc = height;
    NtGdiGetTransform( hdc_src, 0x204, &emr->xformSrc );
    emr->crBkColorSrc = GetBkColor( hdc_src );
    emr->iUsageSrc = DIB_RGB_COLORS;
    emr->offBmiSrc = sizeof(*emr);
    emr->cbBmiSrc = bmi_size;
    emr->offBitsSrc = emr->offBmiSrc + bmi_size;
    emr->cbBitsSrc = src_info.bmiHeader.biSizeImage;
    emr->xMask = x_mask;
    emr->yMask = y_mask;
    emr->iUsageMask = DIB_PAL_INDICES;
    emr->offBmiMask = mask_info_size ? emr->offBitsSrc + emr->cbBitsSrc : 0;
    emr->cbBmiMask = mask_info_size;
    emr->offBitsMask = emr->offBmiMask + emr->cbBmiMask;
    emr->cbBitsMask = mask_info.bmiHeader.biSizeImage;

    bmi = (BITMAPINFO *)((char *)emr + emr->offBmiSrc);
    bmi->bmiHeader = src_info.bmiHeader;
    ret = GetDIBits( blit_dc, blit_bitmap, 0, src_info.bmiHeader.biHeight,
                     (char *)emr + emr->offBitsSrc, bmi, DIB_RGB_COLORS );
    if (!ret) goto err;

    if (mask_info_size)
    {
        mask_bits_info->bmiHeader = mask_info.bmiHeader;
        ret = GetDIBits( blit_dc, mask_bitmap, 0, mask_info.bmiHeader.biHeight,
                         (char *)emr + emr->offBitsMask, mask_bits_info, DIB_RGB_COLORS );
        if (ret) memcpy( (char *)emr + emr->offBmiMask, mask_bits_info, mask_info_size );
    }

    if (ret)
    {
        ret = emfdc_record( emf, (EMR *)emr );
        if (ret) emfdc_update_bounds( emf, &emr->rclBounds );
    }

err:
    HeapFree( GetProcessHeap(), 0, emr );
    if (mask_bitmap != mask) DeleteObject( mask_bitmap );
    if (mask_dc != hdc_src) DeleteObject( mask_dc );
    if (blit_bitmap != bitmap) DeleteObject( blit_bitmap );
    if (blit_dc != hdc_src) DeleteDC( blit_dc );
    return ret;
}

BOOL EMFDC_StretchDIBits( DC_ATTR *dc_attr, INT x_dst, INT y_dst, INT width_dst, INT height_dst,
                          INT x_src, INT y_src, INT width_src, INT height_src, const void *bits,
                          const BITMAPINFO *info, UINT usage, DWORD rop )
{
    EMRSTRETCHDIBITS *emr;
    BOOL ret;
    UINT bmi_size, img_size, payload_size, emr_size;
    BITMAPINFOHEADER bih;
    BITMAPINFO *bi;
    void *ptr;

    /* calculate the size of the colour table and the image */
    if (!emf_parse_user_bitmapinfo( &bih, &info->bmiHeader, usage, TRUE,
                                    &bmi_size, &img_size )) return 0;

    /* check for overflows */
    payload_size = aligned_size(bmi_size) + aligned_size(img_size);
    if (payload_size < bmi_size) return 0;

    emr_size = sizeof (EMRSTRETCHDIBITS) + payload_size;
    if (emr_size < sizeof (EMRSTRETCHDIBITS)) return 0;

    /* allocate record */
    if (!(emr = HeapAlloc(GetProcessHeap(), 0, emr_size ))) return 0;

    /* fill in the EMR header at the front of our piece of memory */
    emr->emr.iType = EMR_STRETCHDIBITS;
    emr->emr.nSize = emr_size;

    emr->xDest     = x_dst;
    emr->yDest     = y_dst;
    emr->cxDest    = width_dst;
    emr->cyDest    = height_dst;
    emr->dwRop     = rop;
    emr->xSrc      = x_src;
    emr->ySrc      = y_src;

    emr->iUsageSrc    = usage;
    emr->offBmiSrc    = sizeof (EMRSTRETCHDIBITS);
    emr->cbBmiSrc     = bmi_size;
    emr->offBitsSrc   = emr->offBmiSrc + aligned_size(bmi_size);
    emr->cbBitsSrc    = img_size;

    emr->cxSrc = width_src;
    emr->cySrc = height_src;

    emr->rclBounds.left   = x_dst;
    emr->rclBounds.top    = y_dst;
    emr->rclBounds.right  = x_dst + width_dst - 1;
    emr->rclBounds.bottom = y_dst + height_dst - 1;

    /* write a bitmap info header (with colours) to the record */
    bi = (BITMAPINFO *)((BYTE *)emr + emr->offBmiSrc);
    bi->bmiHeader = bih;
    emf_copy_colours_from_user_bitmapinfo( bi, info, usage );
    pad_record( bi, emr->cbBmiSrc );

    /* write bitmap bits to the record */
    ptr = memcpy ( (BYTE *)emr + emr->offBitsSrc, bits, img_size );
    pad_record( ptr, emr->cbBitsSrc );

    /* save the record we just created */
    ret = emfdc_record( get_dc_emf( dc_attr ), &emr->emr );
    if (ret) emfdc_update_bounds( get_dc_emf( dc_attr ), &emr->rclBounds );
    HeapFree( GetProcessHeap(), 0, emr );
    return ret;
}

BOOL EMFDC_SetDIBitsToDevice( DC_ATTR *dc_attr, INT x_dst, INT y_dst, DWORD width, DWORD height,
                              INT x_src, INT y_src, UINT startscan, UINT lines,
                              const void *bits, const BITMAPINFO *info, UINT usage )
{
    EMRSETDIBITSTODEVICE *emr;
    BOOL ret;
    UINT bmi_size, img_size, payload_size, emr_size;
    UINT src_height, stride;
    BITMAPINFOHEADER bih;
    BITMAPINFO *bi;
    void *ptr;

    /* calculate the size of the colour table and the image */
    if (!emf_parse_user_bitmapinfo( &bih, &info->bmiHeader, usage, TRUE,
                                    &bmi_size, &img_size )) return 0;

    if (bih.biHeight >= 0)
    {
        src_height = (UINT)bih.biHeight;
        if (src_height > y_src + height) src_height = y_src + height;

        if (src_height < startscan) lines = 0;
        else if (lines > src_height - startscan) lines = src_height - startscan;

        if (!lines) return 0;

        if (bih.biCompression == BI_RGB || bih.biCompression == BI_BITFIELDS)
        {
            /* truncate image and check for overflows */
            stride = get_dib_stride( bih.biWidth, bih.biBitCount );
            img_size = lines * stride;
            if (img_size / stride != lines) return 0;
        }
    }

    /* check for overflows */
    payload_size = aligned_size(bmi_size) + aligned_size(img_size);
    if (payload_size < bmi_size) return 0;

    emr_size = sizeof (EMRSETDIBITSTODEVICE) + payload_size;
    if (emr_size < sizeof (EMRSETDIBITSTODEVICE)) return 0;

    /* allocate record */
    if (!(emr = HeapAlloc( GetProcessHeap(), 0, emr_size ))) return FALSE;

    emr->emr.iType = EMR_SETDIBITSTODEVICE;
    emr->emr.nSize = emr_size;
    emr->rclBounds.left = x_dst;
    emr->rclBounds.top = y_dst;
    emr->rclBounds.right = x_dst + width - 1;
    emr->rclBounds.bottom = y_dst + height - 1;
    emr->xDest = x_dst;
    emr->yDest = y_dst;
    emr->xSrc = x_src;
    emr->ySrc = y_src;
    emr->cxSrc = width;
    emr->cySrc = height;
    emr->offBmiSrc = sizeof(EMRSETDIBITSTODEVICE);
    emr->cbBmiSrc = bmi_size;
    emr->offBitsSrc = sizeof(EMRSETDIBITSTODEVICE) + aligned_size(bmi_size);
    emr->cbBitsSrc = img_size;
    emr->iUsageSrc = usage;
    emr->iStartScan = startscan;
    emr->cScans = lines;

    /* write a bitmap info header (with colours) to the record */
    bi = (BITMAPINFO *)((BYTE *)emr + emr->offBmiSrc);
    bi->bmiHeader = bih;
    emf_copy_colours_from_user_bitmapinfo( bi, info, usage );
    pad_record( bi, bmi_size );

    /* write bitmap bits to the record */
    ptr = memcpy ( (BYTE *)emr + emr->offBitsSrc, bits, img_size );
    pad_record( ptr, img_size );

    if ((ret = emfdc_record( get_dc_emf( dc_attr ), (EMR*)emr )))
        emfdc_update_bounds( get_dc_emf( dc_attr ), &emr->rclBounds );

    HeapFree( GetProcessHeap(), 0, emr );
    return ret;
}

BOOL EMFDC_SetDCBrushColor( DC_ATTR *dc_attr, COLORREF color )
{
    struct emf *emf = get_dc_emf( dc_attr );
    HDC hdc = dc_attr_handle( dc_attr );
    EMRSELECTOBJECT emr;
    DWORD index;

    if (GetCurrentObject( hdc, OBJ_BRUSH ) != GetStockObject( DC_BRUSH )) return TRUE;

    if (emf->dc_brush) DeleteObject( emf->dc_brush );
    if (!(emf->dc_brush = CreateSolidBrush( color ))) return FALSE;
    if (!(index = emfdc_create_brush( emf, emf->dc_brush ))) return FALSE;
    GDI_hdc_using_object( emf->dc_brush, hdc, emfdc_delete_object );
    emr.emr.iType = EMR_SELECTOBJECT;
    emr.emr.nSize = sizeof(emr);
    emr.ihObject = index;
    return emfdc_record( emf, &emr.emr );
}

BOOL EMFDC_SetDCPenColor( DC_ATTR *dc_attr, COLORREF color )
{
    struct emf *emf = get_dc_emf( dc_attr );
    HDC hdc = dc_attr_handle( dc_attr );
    EMRSELECTOBJECT emr;
    DWORD index;
    LOGPEN logpen = { PS_SOLID, { 0, 0 }, color };

    if (GetCurrentObject( hdc, OBJ_PEN ) != GetStockObject( DC_PEN )) return TRUE;

    if (emf->dc_pen) DeleteObject( emf->dc_pen );
    if (!(emf->dc_pen = CreatePenIndirect( &logpen ))) return FALSE;
    if (!(index = emfdc_create_pen( emf, emf->dc_pen ))) return FALSE;
    GDI_hdc_using_object( emf->dc_pen, hdc, emfdc_delete_object );
    emr.emr.iType = EMR_SELECTOBJECT;
    emr.emr.nSize = sizeof(emr);
    emr.ihObject = index;
    return emfdc_record( emf, &emr.emr );
}

BOOL EMFDC_SaveDC( DC_ATTR *dc_attr )
{
    EMRSAVEDC emr;

    emr.emr.iType = EMR_SAVEDC;
    emr.emr.nSize = sizeof(emr);
    return emfdc_record( get_dc_emf( dc_attr ), &emr.emr );
}

BOOL EMFDC_RestoreDC( DC_ATTR *dc_attr, INT level )
{
    EMRRESTOREDC emr;

    if (abs(level) > dc_attr->save_level || level == 0) return FALSE;

    emr.emr.iType = EMR_RESTOREDC;
    emr.emr.nSize = sizeof(emr);
    if (level < 0)
        emr.iRelative = level;
    else
        emr.iRelative = level - dc_attr->save_level - 1;
    return emfdc_record( get_dc_emf( dc_attr ), &emr.emr );
}

BOOL EMFDC_SetTextAlign( DC_ATTR *dc_attr, UINT align )
{
    EMRSETTEXTALIGN emr;

    emr.emr.iType = EMR_SETTEXTALIGN;
    emr.emr.nSize = sizeof(emr);
    emr.iMode = align;
    return emfdc_record( get_dc_emf( dc_attr ), &emr.emr );
}

BOOL EMFDC_SetTextJustification( DC_ATTR *dc_attr, INT extra, INT breaks )
{
    EMRSETTEXTJUSTIFICATION emr;

    emr.emr.iType = EMR_SETTEXTJUSTIFICATION;
    emr.emr.nSize = sizeof(emr);
    emr.nBreakExtra = extra;
    emr.nBreakCount = breaks;
    return emfdc_record( get_dc_emf( dc_attr ), &emr.emr );
}

BOOL EMFDC_SetBkMode( DC_ATTR *dc_attr, INT mode )
{
    EMRSETBKMODE emr;

    emr.emr.iType = EMR_SETBKMODE;
    emr.emr.nSize = sizeof(emr);
    emr.iMode = mode;
    return emfdc_record( get_dc_emf( dc_attr ), &emr.emr );
}

BOOL EMFDC_SetBkColor( DC_ATTR *dc_attr, COLORREF color )
{
    EMRSETBKCOLOR emr;

    emr.emr.iType = EMR_SETBKCOLOR;
    emr.emr.nSize = sizeof(emr);
    emr.crColor = color;
    return emfdc_record( get_dc_emf( dc_attr ), &emr.emr );
}

BOOL EMFDC_SetBrushOrgEx( DC_ATTR *dc_attr, INT x, INT y )
{
    EMRSETBRUSHORGEX emr;

    emr.emr.iType = EMR_SETBRUSHORGEX;
    emr.emr.nSize = sizeof(emr);
    emr.ptlOrigin.x = x;
    emr.ptlOrigin.y = y;
    return emfdc_record( get_dc_emf( dc_attr ), &emr.emr );
}

BOOL EMFDC_SetTextColor( DC_ATTR *dc_attr, COLORREF color )
{
    EMRSETTEXTCOLOR emr;

    emr.emr.iType = EMR_SETTEXTCOLOR;
    emr.emr.nSize = sizeof(emr);
    emr.crColor = color;
    return emfdc_record( get_dc_emf( dc_attr ), &emr.emr );
}

BOOL EMFDC_SetROP2( DC_ATTR *dc_attr, INT rop )
{
    EMRSETROP2 emr;

    emr.emr.iType = EMR_SETROP2;
    emr.emr.nSize = sizeof(emr);
    emr.iMode = rop;
    return emfdc_record( get_dc_emf( dc_attr ), &emr.emr );
}

BOOL EMFDC_SetPolyFillMode( DC_ATTR *dc_attr, INT mode )
{
    EMRSETPOLYFILLMODE emr;

    emr.emr.iType = EMR_SETPOLYFILLMODE;
    emr.emr.nSize = sizeof(emr);
    emr.iMode = mode;
    return emfdc_record( get_dc_emf( dc_attr ), &emr.emr );
}

BOOL EMFDC_SetStretchBltMode( DC_ATTR *dc_attr, INT mode )
{
    EMRSETSTRETCHBLTMODE emr;

    emr.emr.iType = EMR_SETSTRETCHBLTMODE;
    emr.emr.nSize = sizeof(emr);
    emr.iMode = mode;
    return emfdc_record( get_dc_emf( dc_attr ), &emr.emr );
}

BOOL EMFDC_SetArcDirection( DC_ATTR *dc_attr, INT dir )
{
    EMRSETARCDIRECTION emr;

    emr.emr.iType = EMR_SETARCDIRECTION;
    emr.emr.nSize = sizeof(emr);
    emr.iArcDirection = dir;
    return emfdc_record( get_dc_emf( dc_attr ), &emr.emr );
}

INT EMFDC_ExcludeClipRect( DC_ATTR *dc_attr, INT left, INT top, INT right, INT bottom )
{
    EMREXCLUDECLIPRECT emr;

    emr.emr.iType      = EMR_EXCLUDECLIPRECT;
    emr.emr.nSize      = sizeof(emr);
    emr.rclClip.left   = left;
    emr.rclClip.top    = top;
    emr.rclClip.right  = right;
    emr.rclClip.bottom = bottom;
    return emfdc_record( get_dc_emf( dc_attr ), &emr.emr );
}

BOOL EMFDC_IntersectClipRect( DC_ATTR *dc_attr, INT left, INT top, INT right, INT bottom)
{
    EMRINTERSECTCLIPRECT emr;

    emr.emr.iType      = EMR_INTERSECTCLIPRECT;
    emr.emr.nSize      = sizeof(emr);
    emr.rclClip.left   = left;
    emr.rclClip.top    = top;
    emr.rclClip.right  = right;
    emr.rclClip.bottom = bottom;
    return emfdc_record( get_dc_emf( dc_attr ), &emr.emr );
}

BOOL EMFDC_OffsetClipRgn( DC_ATTR *dc_attr, INT x, INT y )
{
    EMROFFSETCLIPRGN emr;

    emr.emr.iType   = EMR_OFFSETCLIPRGN;
    emr.emr.nSize   = sizeof(emr);
    emr.ptlOffset.x = x;
    emr.ptlOffset.y = y;
    return emfdc_record( get_dc_emf( dc_attr ), &emr.emr );
}

BOOL EMFDC_ExtSelectClipRgn( DC_ATTR *dc_attr, HRGN hrgn, INT mode )
{
    EMREXTSELECTCLIPRGN *emr;
    DWORD size, rgnsize;
    BOOL ret;

    if (!hrgn)
    {
        if (mode != RGN_COPY) return ERROR;
        rgnsize = 0;
    }
    else rgnsize = NtGdiGetRegionData( hrgn, 0, NULL );

    size = rgnsize + offsetof(EMREXTSELECTCLIPRGN,RgnData);
    emr = HeapAlloc( GetProcessHeap(), 0, size );
    if (rgnsize) NtGdiGetRegionData( hrgn, rgnsize, (RGNDATA *)&emr->RgnData );

    emr->emr.iType = EMR_EXTSELECTCLIPRGN;
    emr->emr.nSize = size;
    emr->cbRgnData = rgnsize;
    emr->iMode     = mode;

    ret = emfdc_record( get_dc_emf( dc_attr ), &emr->emr );
    HeapFree( GetProcessHeap(), 0, emr );
    return ret;
}

BOOL EMFDC_SetMapMode( DC_ATTR *dc_attr, INT mode )
{
    EMRSETMAPMODE emr;

    emr.emr.iType = EMR_SETMAPMODE;
    emr.emr.nSize = sizeof(emr);
    emr.iMode = mode;
    return emfdc_record( get_dc_emf( dc_attr ), &emr.emr );
}

BOOL EMFDC_SetMetaRgn( DC_ATTR *dc_attr )
{
    EMRSETMETARGN emr;

    emr.emr.iType = EMR_SETMETARGN;
    emr.emr.nSize = sizeof(emr);
    return emfdc_record( get_dc_emf( dc_attr ), &emr.emr );
}

BOOL EMFDC_SetMiterLimit( DC_ATTR *dc_attr, FLOAT limit )
{
    struct emr_set_miter_limit emr;

    emr.emr.iType = EMR_SETMITERLIMIT;
    emr.emr.nSize = sizeof(emr);
    emr.eMiterLimit = limit;
    return emfdc_record( get_dc_emf( dc_attr ), &emr.emr );
}

BOOL EMFDC_SetViewportExtEx( DC_ATTR *dc_attr, INT cx, INT cy )
{
    EMRSETVIEWPORTEXTEX emr;

    emr.emr.iType = EMR_SETVIEWPORTEXTEX;
    emr.emr.nSize = sizeof(emr);
    emr.szlExtent.cx = cx;
    emr.szlExtent.cy = cy;
    return emfdc_record( get_dc_emf( dc_attr ), &emr.emr );
}

BOOL EMFDC_SetWindowExtEx( DC_ATTR *dc_attr, INT cx, INT cy )
{
    EMRSETWINDOWEXTEX emr;

    emr.emr.iType = EMR_SETWINDOWEXTEX;
    emr.emr.nSize = sizeof(emr);
    emr.szlExtent.cx = cx;
    emr.szlExtent.cy = cy;
    return emfdc_record( get_dc_emf( dc_attr ), &emr.emr );
}

BOOL EMFDC_SetViewportOrgEx( DC_ATTR *dc_attr, INT x, INT y )
{
    EMRSETVIEWPORTORGEX emr;

    emr.emr.iType = EMR_SETVIEWPORTORGEX;
    emr.emr.nSize = sizeof(emr);
    emr.ptlOrigin.x = x;
    emr.ptlOrigin.y = y;
    return emfdc_record( get_dc_emf( dc_attr ), &emr.emr );
}

BOOL EMFDC_SetWindowOrgEx( DC_ATTR *dc_attr, INT x, INT y )
{
    EMRSETWINDOWORGEX emr;

    emr.emr.iType = EMR_SETWINDOWORGEX;
    emr.emr.nSize = sizeof(emr);
    emr.ptlOrigin.x = x;
    emr.ptlOrigin.y = y;
    return emfdc_record( get_dc_emf( dc_attr ), &emr.emr );
}

BOOL EMFDC_ScaleViewportExtEx( DC_ATTR *dc_attr, INT x_num, INT x_denom, INT y_num, INT y_denom )
{
    EMRSCALEVIEWPORTEXTEX emr;

    emr.emr.iType = EMR_SCALEVIEWPORTEXTEX;
    emr.emr.nSize = sizeof(emr);
    emr.xNum      = x_num;
    emr.xDenom    = x_denom;
    emr.yNum      = y_num;
    emr.yDenom    = y_denom;
    return emfdc_record( get_dc_emf( dc_attr ), &emr.emr );
}

BOOL EMFDC_ScaleWindowExtEx( DC_ATTR *dc_attr, INT x_num, INT x_denom, INT y_num, INT y_denom )
{
    EMRSCALEWINDOWEXTEX emr;

    emr.emr.iType = EMR_SCALEWINDOWEXTEX;
    emr.emr.nSize = sizeof(emr);
    emr.xNum      = x_num;
    emr.xDenom    = x_denom;
    emr.yNum      = y_num;
    emr.yDenom    = y_denom;
    return emfdc_record( get_dc_emf( dc_attr ), &emr.emr );
}

BOOL EMFDC_SetLayout( DC_ATTR *dc_attr, DWORD layout )
{
    EMRSETLAYOUT emr;

    emr.emr.iType = EMR_SETLAYOUT;
    emr.emr.nSize = sizeof(emr);
    emr.iMode = layout;
    return emfdc_record( get_dc_emf( dc_attr ), &emr.emr );
}

BOOL EMFDC_SetWorldTransform( DC_ATTR *dc_attr, const XFORM *xform )
{
    EMRSETWORLDTRANSFORM emr;

    emr.emr.iType = EMR_SETWORLDTRANSFORM;
    emr.emr.nSize = sizeof(emr);
    emr.xform = *xform;
    return emfdc_record( get_dc_emf( dc_attr ), &emr.emr );
}

BOOL EMFDC_ModifyWorldTransform( DC_ATTR *dc_attr, const XFORM *xform, DWORD mode )
{
    EMRMODIFYWORLDTRANSFORM emr;

    emr.emr.iType = EMR_MODIFYWORLDTRANSFORM;
    emr.emr.nSize = sizeof(emr);
    if (mode == MWT_IDENTITY)
    {
        emr.xform.eM11 = 1.0f;
        emr.xform.eM12 = 0.0f;
        emr.xform.eM21 = 0.0f;
        emr.xform.eM22 = 1.0f;
        emr.xform.eDx = 0.0f;
        emr.xform.eDy = 0.0f;
    }
    else
    {
        emr.xform = *xform;
    }
    emr.iMode = mode;
    return emfdc_record( get_dc_emf( dc_attr ), &emr.emr );
}

BOOL EMFDC_SetMapperFlags( DC_ATTR *dc_attr, DWORD flags )
{
    EMRSETMAPPERFLAGS emr;

    emr.emr.iType = EMR_SETMAPPERFLAGS;
    emr.emr.nSize = sizeof(emr);
    emr.dwFlags   = flags;
    return emfdc_record( get_dc_emf( dc_attr ), &emr.emr );
}

BOOL EMFDC_AbortPath( DC_ATTR *dc_attr )
{
    struct emf *emf = get_dc_emf( dc_attr );
    EMRABORTPATH emr;

    emr.emr.iType = EMR_ABORTPATH;
    emr.emr.nSize = sizeof(emr);

    emf->path = FALSE;
    return emfdc_record( get_dc_emf( dc_attr ), &emr.emr );
}

BOOL EMFDC_BeginPath( DC_ATTR *dc_attr )
{
    struct emf *emf = get_dc_emf( dc_attr );
    EMRBEGINPATH emr;

    emr.emr.iType = EMR_BEGINPATH;
    emr.emr.nSize = sizeof(emr);
    if (!emfdc_record( emf, &emr.emr )) return FALSE;

    emf->path = TRUE;
    return TRUE;
}

BOOL EMFDC_CloseFigure( DC_ATTR *dc_attr )
{
    EMRCLOSEFIGURE emr;

    emr.emr.iType = EMR_CLOSEFIGURE;
    emr.emr.nSize = sizeof(emr);
    return emfdc_record( get_dc_emf( dc_attr ), &emr.emr );
}

BOOL EMFDC_EndPath( DC_ATTR *dc_attr )
{
    struct emf *emf = get_dc_emf( dc_attr );
    EMRENDPATH emr;

    emf->path = FALSE;

    emr.emr.iType = EMR_ENDPATH;
    emr.emr.nSize = sizeof(emr);
    return emfdc_record( emf, &emr.emr );
}

BOOL EMFDC_FlattenPath( DC_ATTR *dc_attr )
{
    EMRFLATTENPATH emr;

    emr.emr.iType = EMR_FLATTENPATH;
    emr.emr.nSize = sizeof(emr);
    return emfdc_record( get_dc_emf( dc_attr ), &emr.emr );
}

BOOL EMFDC_SelectClipPath( DC_ATTR *dc_attr, INT mode )
{
    EMRSELECTCLIPPATH emr;

    emr.emr.iType = EMR_SELECTCLIPPATH;
    emr.emr.nSize = sizeof(emr);
    emr.iMode = mode;
    return emfdc_record( get_dc_emf( dc_attr ), &emr.emr );
}

BOOL EMFDC_WidenPath( DC_ATTR *dc_attr )
{
    EMRWIDENPATH emr;

    emr.emr.iType = EMR_WIDENPATH;
    emr.emr.nSize = sizeof(emr);
    return emfdc_record( get_dc_emf( dc_attr ), &emr.emr );
}

void EMFDC_DeleteDC( DC_ATTR *dc_attr )
{
    struct emf *emf = get_dc_emf( dc_attr );
    UINT index;

    if (emf->dc_brush) DeleteObject( emf->dc_brush );
    if (emf->dc_pen) DeleteObject( emf->dc_pen );
    CloseHandle( emf->file );
    HeapFree( GetProcessHeap(), 0, emf->palette );
    HeapFree( GetProcessHeap(), 0, emf->emh );
    for (index = 0; index < emf->handles_size; index++)
        if (emf->handles[index])
            GDI_hdc_not_using_object( emf->handles[index], dc_attr_handle( emf->dc_attr ));
    HeapFree( GetProcessHeap(), 0, emf->handles );
    HeapFree( GetProcessHeap(), 0, emf );
    dc_attr->emf = 0;
}

/*******************************************************************
 *      GdiComment   (GDI32.@)
 */
BOOL WINAPI GdiComment( HDC hdc, UINT bytes, const BYTE *buffer )
{
    DC_ATTR *dc_attr;
    EMRGDICOMMENT *emr;
    UINT total;
    BOOL ret;

    if (!(dc_attr = get_dc_attr( hdc )) || !get_dc_emf( dc_attr )) return FALSE;

    total = offsetof(EMRGDICOMMENT,Data) + aligned_size(bytes);

    emr = HeapAlloc(GetProcessHeap(), 0, total);
    if (!emr)
        return FALSE;

    emr->emr.iType = EMR_GDICOMMENT;
    emr->emr.nSize = total;
    emr->cbData = bytes;
    memcpy(&emr->Data[0], buffer, bytes);
    pad_record(&emr->Data[0], bytes);

    ret = emfdc_record( get_dc_emf( dc_attr ), &emr->emr );

    HeapFree(GetProcessHeap(), 0, emr);

    return ret;
}

/**********************************************************************
 *           CreateEnhMetaFileA   (GDI32.@)
 */
HDC WINAPI CreateEnhMetaFileA( HDC hdc, const char *filename, const RECT *rect,
                               const char *description )
{
    WCHAR *filenameW = NULL;
    WCHAR *descriptionW = NULL;
    DWORD len1, len2, total;
    HDC ret;

    if (filename)
    {
        total = MultiByteToWideChar( CP_ACP, 0, filename, -1, NULL, 0 );
        filenameW = HeapAlloc( GetProcessHeap(), 0, total * sizeof(WCHAR) );
        MultiByteToWideChar( CP_ACP, 0, filename, -1, filenameW, total );
    }

    if(description)
    {
        len1 = strlen(description);
        len2 = strlen(description + len1 + 1);
        total = MultiByteToWideChar( CP_ACP, 0, description, len1 + len2 + 3, NULL, 0 );
        descriptionW = HeapAlloc( GetProcessHeap(), 0, total * sizeof(WCHAR) );
        MultiByteToWideChar( CP_ACP, 0, description, len1 + len2 + 3, descriptionW, total );
    }

    ret = CreateEnhMetaFileW( hdc, filenameW, rect, descriptionW );

    HeapFree( GetProcessHeap(), 0, filenameW );
    HeapFree( GetProcessHeap(), 0, descriptionW );
    return ret;
}

static void emf_reset( DC_ATTR *dc_attr, const RECT *rect )
{
    struct emf *emf = get_dc_emf( dc_attr );
    HDC hdc = dc_attr_handle( dc_attr );
    int i;

    for (i = 0; i < emf->handles_size; i++)
        if (emf->handles[i])
            GDI_hdc_not_using_object( emf->handles[i], dc_attr_handle( emf->dc_attr ));
    memset( emf->handles, 0, emf->handles_size * sizeof(emf->handles[0]) );
    emf->cur_handles = 1;
    emf->file = 0;
    if (emf->dc_brush) DeleteObject( emf->dc_brush );
    emf->dc_brush = 0;
    if (emf->dc_pen) DeleteObject( emf->dc_pen );
    emf->dc_pen = 0;
    emf->path = FALSE;
    emf->palette_used = 0;

    dc_attr->emf_bounds.left = dc_attr->emf_bounds.top = 0;
    dc_attr->emf_bounds.right = dc_attr->emf_bounds.bottom = -1;

    if (rect)
    {
        emf->emh->rclFrame.left   = rect->left;
        emf->emh->rclFrame.top    = rect->top;
        emf->emh->rclFrame.right  = rect->right;
        emf->emh->rclFrame.bottom = rect->bottom;
    }
    else
    {
        /* Set this to {0,0 - -1,-1} and update it at the end */
        emf->emh->rclFrame.left = emf->emh->rclFrame.top = 0;
        emf->emh->rclFrame.right = emf->emh->rclFrame.bottom = -1;
    }

    emf->emh->dSignature = ENHMETA_SIGNATURE;
    emf->emh->nVersion = 0x10000;
    emf->emh->nBytes = emf->emh->nSize;
    emf->emh->nRecords = 1;
    emf->emh->nHandles = 1;
    emf->emh->sReserved = 0; /* According to docs, this is reserved and must be 0 */
    emf->emh->nPalEntries = 0;

    /* Size in pixels */
    emf->emh->szlDevice.cx = GetDeviceCaps( hdc, HORZRES );
    emf->emh->szlDevice.cy = GetDeviceCaps( hdc, VERTRES );

    /* Size in millimeters */
    emf->emh->szlMillimeters.cx = GetDeviceCaps( hdc, HORZSIZE );
    emf->emh->szlMillimeters.cy = GetDeviceCaps( hdc, VERTSIZE );

    emf->emh->cbPixelFormat = 0;
    emf->emh->offPixelFormat = 0;
    emf->emh->bOpenGL = 0;

    /* Size in micrometers */
    emf->emh->szlMicrometers.cx = emf->emh->szlMillimeters.cx * 1000;
    emf->emh->szlMicrometers.cy = emf->emh->szlMillimeters.cy * 1000;
}

static struct emf *emf_create( HDC hdc, const RECT *rect, const WCHAR *description )
{
    DWORD size = 0, length = 0;
    DC_ATTR *dc_attr;
    struct emf *emf;
    void *ptr;

    if (!(dc_attr = get_dc_attr( hdc )) || !(emf = HeapAlloc( GetProcessHeap(), 0, sizeof(*emf) )))
        return NULL;

    if (description) /* App name\0Title\0\0 */
    {
        length = lstrlenW( description );
        length += lstrlenW( description + length + 1 );
        length += 3;
        length *= 2;
    }
    size = sizeof(ENHMETAHEADER) + aligned_size(length);

    if (!(emf->emh = HeapAlloc( GetProcessHeap(), 0, size )) ||
        !(emf->handles = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                HANDLE_LIST_INC * sizeof(emf->handles[0]) )))
    {
        HeapFree( GetProcessHeap(), 0, emf->emh );
        HeapFree( GetProcessHeap(), 0, emf );
        return NULL;
    }

    emf->dc_attr = dc_attr;
    dc_attr->emf = (UINT_PTR)emf;

    emf->handles_size = HANDLE_LIST_INC;
    emf->dc_brush = 0;
    emf->dc_pen = 0;
    emf->palette_size = 0;
    emf->palette = NULL;

    emf->emh->iType = EMR_HEADER;
    emf->emh->nSize = size;
    emf->emh->nDescription = length / 2;
    emf->emh->offDescription = length ? sizeof(ENHMETAHEADER) : 0;

    ptr = memcpy( (char *)emf->emh + sizeof(ENHMETAHEADER), description, length );
    pad_record( ptr, length );

    emf_reset( dc_attr, rect );
    return emf;
}

/**********************************************************************
 *           CreateEnhMetaFileW   (GDI32.@)
 */
HDC WINAPI CreateEnhMetaFileW( HDC hdc, const WCHAR *filename, const RECT *rect,
                               const WCHAR *description )
{
    struct emf *emf;
    HANDLE file;
    HDC ret;

    TRACE( "(%p %s %s %s)\n", hdc, debugstr_w(filename), wine_dbgstr_rect(rect),
           debugstr_w(description) );

    if (!(ret = NtGdiCreateMetafileDC( hdc ))) return 0;

    emf = emf_create( ret, rect, description );
    if (!emf)
    {
        DeleteDC( ret );
        return 0;
    }

    if (filename)  /* disk based metafile */
    {
        if ((file = CreateFileW( filename, GENERIC_WRITE | GENERIC_READ, 0,
                                  NULL, CREATE_ALWAYS, 0, 0)) == INVALID_HANDLE_VALUE)
        {
            DeleteDC( ret );
            return 0;
        }
        emf->file = file;
    }

    TRACE( "returning %p\n", ret );
    return ret;
}

static BOOL emf_eof( DC_ATTR *dc_attr )
{
    struct emf *emf = get_dc_emf( dc_attr );
    UINT size, palette_size;
    EMREOF *emr;
    BOOL ret;

    palette_size = emf->palette_used * sizeof(*emf->palette);
    size = sizeof(*emr) + palette_size;
    if (!(emr = HeapAlloc( GetProcessHeap(), 0, size ))) return FALSE;

    emr->emr.iType = EMR_EOF;
    emr->emr.nSize = size;
    emr->nPalEntries = emf->palette_used;
    emr->offPalEntries = FIELD_OFFSET(EMREOF, nSizeLast);
    memcpy( (BYTE *)emr + emr->offPalEntries, emf->palette, palette_size );
    /* Set nSizeLast */
    ((DWORD *)((BYTE *)emr + size))[-1] = size;
    ret = emfdc_record( emf, &emr->emr );
    HeapFree( GetProcessHeap(), 0, emr );
    if (!ret) return FALSE;

    emf->emh->rclBounds = dc_attr->emf_bounds;

    /* Update rclFrame if not initialized in CreateEnhMetaFile */
    if (emf->emh->rclFrame.left > emf->emh->rclFrame.right)
    {
        emf->emh->rclFrame.left = emf->emh->rclBounds.left *
            emf->emh->szlMillimeters.cx * 100 / emf->emh->szlDevice.cx;
        emf->emh->rclFrame.top = emf->emh->rclBounds.top *
            emf->emh->szlMillimeters.cy * 100 / emf->emh->szlDevice.cy;
        emf->emh->rclFrame.right = emf->emh->rclBounds.right *
            emf->emh->szlMillimeters.cx * 100 / emf->emh->szlDevice.cx;
        emf->emh->rclFrame.bottom = emf->emh->rclBounds.bottom *
            emf->emh->szlMillimeters.cy * 100 / emf->emh->szlDevice.cy;
    }
    return TRUE;
}

/******************************************************************
 *           CloseEnhMetaFile (GDI32.@)
 */
HENHMETAFILE WINAPI CloseEnhMetaFile( HDC hdc )
{
    HENHMETAFILE hmf;
    struct emf *emf;
    DC_ATTR *dc_attr;
    HANDLE mapping = 0;

    TRACE("(%p)\n", hdc );

    if (!(dc_attr = get_dc_attr( hdc )) || !get_dc_emf( dc_attr )) return 0;
    emf = get_dc_emf( dc_attr );

    if (dc_attr->save_level)
        RestoreDC( hdc, 1 );

    if (emf->dc_brush) DeleteObject( emf->dc_brush );
    emf->dc_brush = 0;
    if (emf->dc_pen) DeleteObject( emf->dc_pen );
    emf->dc_pen = 0;

    if (!emf_eof( dc_attr )) return 0;

    if (emf->file)  /* disk based metafile */
    {
        if (!WriteFile( emf->file, emf->emh, emf->emh->nBytes, NULL, NULL ))
        {
            CloseHandle( emf->file );
            emf->file = 0;
            return 0;
        }
        HeapFree( GetProcessHeap(), 0, emf->emh );
        mapping = CreateFileMappingA( emf->file, NULL, PAGE_READONLY, 0, 0, NULL );
        TRACE( "mapping = %p\n", mapping );
        emf->emh = MapViewOfFile( mapping, FILE_MAP_READ, 0, 0, 0 );
        TRACE( "view = %p\n", emf->emh );
        CloseHandle( mapping );
        CloseHandle( emf->file );
    }

    hmf = EMF_Create_HENHMETAFILE( emf->emh, emf->emh->nBytes, emf->file != 0 );
    emf->file = 0;
    emf->emh = NULL;  /* So it won't be deleted */
    DeleteDC( hdc );
    return hmf;
}

BOOL spool_start_doc( DC_ATTR *dc_attr, HANDLE hspool, const DOCINFOW *doc_info )
{
    struct
    {
        unsigned int dwVersion;
        unsigned int cjSize;
        unsigned int dpszDocName;
        unsigned int dpszOutput;
    } *header;
    size_t size = sizeof(*header);
    struct emf *emf;
    DWORD written;
    WCHAR *p;

    TRACE( "(%p %p)\n", dc_attr, hspool );

    if (doc_info->lpszDocName)
        size += (wcslen( doc_info->lpszDocName ) + 1) * sizeof(WCHAR);
    if (doc_info->lpszOutput)
        size += (wcslen( doc_info->lpszOutput ) + 1) * sizeof(WCHAR);
    header = HeapAlloc( GetProcessHeap(), 0, size );
    if (!header) return FALSE;

    header->dwVersion = 0x10000;
    header->cjSize = size;
    p = (WCHAR *)(header + 1);
    if (doc_info->lpszDocName)
    {
        header->dpszDocName = (BYTE *)p - (BYTE *)header;
        wcscpy( p, doc_info->lpszDocName );
        p += wcslen( doc_info->lpszDocName ) + 1;
    }
    else
    {
        header->dpszDocName = 0;
    }
    if (doc_info->lpszOutput)
    {
        header->dpszOutput = (BYTE *)p - (BYTE *)header;
        wcscpy( p, doc_info->lpszOutput );
    }
    else
    {
        header->dpszOutput = 0;
    }
    if (!WritePrinter( hspool, header, size, &written )) written = 0;
    HeapFree( GetProcessHeap(), 0, header );
    if (written != size) return FALSE;

    emf = emf_create( dc_attr_handle(dc_attr), NULL, NULL );
    if (!emf) return FALSE;
    return TRUE;
}

int spool_start_page( DC_ATTR *dc_attr, HANDLE hspool )
{
    HDC hdc = dc_attr_handle( dc_attr );
    POINT pos = { 0 };
    XFORM xform;

    TRACE( "(%p)\n", dc_attr );

    /* Save current DC state to EMF */
    /* FIXME: SetTextJustification if needed */
    EMFDC_SelectObject( dc_attr, GetCurrentObject(hdc, OBJ_PEN) );
    EMFDC_SelectObject( dc_attr, GetCurrentObject(hdc, OBJ_BRUSH) );
    EMFDC_SelectObject( dc_attr, GetCurrentObject(hdc, OBJ_FONT) );
    if (GetBkColor( hdc ) != 0xffffff)
        EMFDC_SetBkColor( dc_attr, GetBkColor(hdc) );
    if (GetBkMode( hdc ) != OPAQUE)
        EMFDC_SetBkMode( dc_attr, GetBkMode(hdc) );
    GetCurrentPositionEx( hdc, &pos );
    if (pos.x || pos.y)
        EMFDC_MoveTo( dc_attr, pos.x, pos.y );
    if (GetMapMode( hdc ) != MM_TEXT)
        EMFDC_SetMapMode( dc_attr, GetMapMode(hdc) );
    if (GetPolyFillMode( hdc ) != ALTERNATE)
        EMFDC_SetPolyFillMode( dc_attr, GetPolyFillMode(hdc) );
    if (GetROP2( hdc ) != R2_COPYPEN)
        EMFDC_SetROP2( dc_attr, GetROP2(hdc) );
    if (GetStretchBltMode( hdc ) != BLACKONWHITE)
        EMFDC_SetStretchBltMode( dc_attr, GetStretchBltMode(hdc) );
    if (GetTextAlign( hdc ) != (TA_LEFT | TA_TOP))
        EMFDC_SetTextAlign( dc_attr, GetTextAlign(hdc) );
    if (GetTextColor( hdc ))
        EMFDC_SetTextColor( dc_attr, GetTextColor(hdc) );
    GetWorldTransform(hdc, &xform);
    if (xform.eM11 != 1 || xform.eM22 != 1 || xform.eM12 || xform.eM21 || xform.eDx || xform.eDy)
        EMFDC_SetWorldTransform( dc_attr, &xform );

    return StartPagePrinter( hspool );
}

int spool_end_page( DC_ATTR *dc_attr, HANDLE hspool, const DEVMODEW *devmode, BOOL write_devmode )
{
    struct record_hdr
    {
        unsigned int ulID;
        unsigned int cjSize;
    } record_hdr;
    struct
    {
        struct record_hdr hdr;
        LARGE_INTEGER pos;
    } metafile_ext;
    struct emf *emf = get_dc_emf( dc_attr );
    DWORD written;

    TRACE( "(%p %p)\n", dc_attr, hspool );

    if (!emf_eof( dc_attr )) return 0;

    record_hdr.ulID = EMRI_METAFILE_DATA;
    record_hdr.cjSize = emf->emh->nBytes;
    if (!WritePrinter( hspool, &record_hdr, sizeof(record_hdr), &written )) return 0;
    if (!WritePrinter( hspool, emf->emh, emf->emh->nBytes, &written )) return 0;

    if (write_devmode)
    {
        record_hdr.ulID = EMRI_DEVMODE;
        record_hdr.cjSize = devmode ? devmode->dmSize + devmode->dmDriverExtra : 0;
        if (!WritePrinter( hspool, &record_hdr, sizeof(record_hdr), &written )) return 0;
        if (devmode && !WritePrinter( hspool, (BYTE *)devmode,
                    record_hdr.cjSize, &written )) return 0;
    }

    metafile_ext.hdr.ulID = EMRI_METAFILE_EXT;
    metafile_ext.hdr.cjSize = sizeof(metafile_ext) - sizeof(struct record_hdr);
    metafile_ext.pos.QuadPart = emf->emh->nBytes + sizeof(record_hdr);
    if (write_devmode)
        metafile_ext.pos.QuadPart += record_hdr.cjSize + sizeof(record_hdr);
    if (!WritePrinter( hspool, &metafile_ext, sizeof(metafile_ext), &written )) return 0;

    emf_reset( dc_attr, NULL );
    return EndPagePrinter( hspool );
}

int spool_abort_doc( DC_ATTR *dc_attr, HANDLE hspool )
{
    TRACE( "(%p %p)\n", dc_attr, hspool );

    EMFDC_DeleteDC( dc_attr );
    return AbortPrinter( hspool );
}

int spool_end_doc( DC_ATTR *dc_attr, HANDLE hspool )
{
    TRACE( "(%p %p)\n", dc_attr, hspool );

    EMFDC_DeleteDC( dc_attr );
    return EndDocPrinter( hspool );
}
