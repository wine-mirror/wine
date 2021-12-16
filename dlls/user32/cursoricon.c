/*
 * Cursor and icon support
 *
 * Copyright 1995 Alexandre Julliard
 * Copyright 1996 Martin Von Loewis
 * Copyright 1997 Alex Korobka
 * Copyright 1998 Turchanov Sergey
 * Copyright 2007 Henri Verbeet
 * Copyright 2009 Vincent Povirk for CodeWeavers
 * Copyright 2016 Dmitry Timoshkov
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
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <png.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winerror.h"
#include "winnls.h"
#include "wine/exception.h"
#include "wine/server.h"
#include "controls.h"
#include "win.h"
#include "user_private.h"
#include "wine/list.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(cursor);
WINE_DECLARE_DEBUG_CHANNEL(icon);
WINE_DECLARE_DEBUG_CHANNEL(resource);

#define RIFF_FOURCC( c0, c1, c2, c3 ) \
        ( (DWORD)(BYTE)(c0) | ( (DWORD)(BYTE)(c1) << 8 ) | \
        ( (DWORD)(BYTE)(c2) << 16 ) | ( (DWORD)(BYTE)(c3) << 24 ) )
#define PNG_SIGN RIFF_FOURCC(0x89,'P','N','G')

static struct list icon_cache = LIST_INIT( icon_cache );

/**********************************************************************
 * User objects management
 */

struct cursoricon_frame
{
    UINT               width;    /* frame-specific width */
    UINT               height;   /* frame-specific height */
    UINT               delay;    /* frame-specific delay between this frame and the next (in jiffies) */
    HBITMAP            color;    /* color bitmap */
    HBITMAP            alpha;    /* pre-multiplied alpha bitmap for 32-bpp icons */
    HBITMAP            mask;     /* mask bitmap (followed by color for 1-bpp icons) */
};

struct cursoricon_object
{
    struct user_object      obj;        /* object header */
    struct list             entry;      /* entry in shared icons list */
    ULONG_PTR               param;      /* opaque param used by 16-bit code */
    HMODULE                 module;     /* module for icons loaded from resources */
    LPWSTR                  resname;    /* resource name for icons loaded from resources */
    HRSRC                   rsrc;       /* resource for shared icons */
    BOOL                    is_shared;  /* whether this object is shared */
    BOOL                    is_icon;    /* whether icon or cursor */
    BOOL                    is_ani;     /* whether this object is a static cursor or an animated cursor */
    UINT                    delay;      /* delay between this frame and the next (in jiffies) */
    POINT                   hotspot;
};

struct static_cursoricon_object
{
    struct cursoricon_object shared;
    struct cursoricon_frame  frame;      /* frame-specific icon data */
};

struct animated_cursoricon_object
{
    struct cursoricon_object shared;
    UINT                     num_frames; /* number of frames in the icon/cursor */
    UINT                     num_steps;  /* number of sequence steps in the icon/cursor */
    HICON                    frames[1];  /* list of animated cursor frames */
};

static HBITMAP create_color_bitmap( int width, int height )
{
    HDC hdc = get_display_dc();
    HBITMAP ret = CreateCompatibleBitmap( hdc, width, height );
    release_display_dc( hdc );
    return ret;
}

static int get_display_bpp(void)
{
    HDC hdc = get_display_dc();
    int ret = GetDeviceCaps( hdc, BITSPIXEL );
    release_display_dc( hdc );
    return ret;
}

static HICON alloc_icon_handle( BOOL is_ani, UINT num_steps )
{
    struct cursoricon_object *obj;
    int icon_size;
    HICON handle;

    if (is_ani)
        icon_size = FIELD_OFFSET( struct animated_cursoricon_object, frames[num_steps] );
    else
        icon_size = sizeof( struct static_cursoricon_object );
    obj = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, icon_size );
    if (!obj) return NULL;

    obj->delay = 0;
    obj->is_ani = is_ani;
    if (is_ani)
    {
        struct animated_cursoricon_object *ani_icon_data = (struct animated_cursoricon_object *) obj;

        ani_icon_data->num_steps = num_steps;
        ani_icon_data->num_frames = num_steps; /* changed later for some animated cursors */
    }

    if (!(handle = alloc_user_handle( &obj->obj, USER_ICON )))
        HeapFree( GetProcessHeap(), 0, obj );
    return handle;
}

static struct cursoricon_object *get_icon_ptr( HICON handle )
{
    struct cursoricon_object *obj = get_user_handle_ptr( handle, USER_ICON );
    if (obj == OBJ_OTHER_PROCESS)
    {
        WARN( "icon handle %p from other process\n", handle );
        obj = NULL;
    }
    return obj;
}

static struct cursoricon_frame *get_icon_frame( struct cursoricon_object *obj, int istep )
{
    struct static_cursoricon_object *req_frame;

    if (obj->is_ani)
    {
        struct animated_cursoricon_object *ani_icon_data;
        struct cursoricon_object *frameobj;

        ani_icon_data = (struct animated_cursoricon_object *) obj;
        if (!(frameobj = get_icon_ptr( ani_icon_data->frames[istep] )))
            return 0;
        req_frame = (struct static_cursoricon_object *) frameobj;
    }
    else
        req_frame = (struct static_cursoricon_object *) obj;

    return &req_frame->frame;
}

static void release_icon_frame( struct cursoricon_object *obj, struct cursoricon_frame *frame )
{
    if (obj->is_ani)
    {
        struct cursoricon_object *frameobj;

        frameobj = (struct cursoricon_object *) (((char *)frame) - FIELD_OFFSET(struct static_cursoricon_object, frame));
        release_user_handle_ptr( frameobj );
    }
}

static UINT get_icon_steps( struct cursoricon_object *obj )
{
    if (obj->is_ani)
    {
        struct animated_cursoricon_object *ani_icon_data;

        ani_icon_data = (struct animated_cursoricon_object *) obj;
        return ani_icon_data->num_steps;
    }
    return 1;
}

static BOOL free_icon_handle( HICON handle )
{
    struct cursoricon_object *obj = free_user_handle( handle, USER_ICON );

    if (obj == OBJ_OTHER_PROCESS) WARN( "icon handle %p from other process\n", handle );
    else if (obj)
    {
        ULONG_PTR param = obj->param;
        UINT i;

        assert( !obj->rsrc );  /* shared icons can't be freed */

        if (!obj->is_ani)
        {
            struct cursoricon_frame *frame = get_icon_frame( obj, 0 );

            if (frame->alpha) DeleteObject( frame->alpha );
            if (frame->color) DeleteObject( frame->color );
            DeleteObject( frame->mask );
            release_icon_frame( obj, frame );
        }
        else
        {
            struct animated_cursoricon_object *ani_icon_data = (struct animated_cursoricon_object *) obj;

            for (i=0; i<ani_icon_data->num_steps; i++)
            {
                HICON hFrame = ani_icon_data->frames[i];

                if (hFrame)
                {
                    UINT j;

                    free_icon_handle( ani_icon_data->frames[i] );
                    for (j=0; j<ani_icon_data->num_steps; j++)
                    {
                        if (ani_icon_data->frames[j] == hFrame)
                            ani_icon_data->frames[j] = 0;
                    }
                }
            }
        }
        if (!IS_INTRESOURCE( obj->resname )) HeapFree( GetProcessHeap(), 0, obj->resname );
        HeapFree( GetProcessHeap(), 0, obj );
        if (wow_handlers.free_icon_param && param) wow_handlers.free_icon_param( param );
        USER_Driver->pDestroyCursorIcon( handle );
        return TRUE;
    }
    return FALSE;
}

ULONG_PTR get_icon_param( HICON handle )
{
    ULONG_PTR ret = 0;
    struct cursoricon_object *obj = get_user_handle_ptr( handle, USER_ICON );

    if (obj == OBJ_OTHER_PROCESS) WARN( "icon handle %p from other process\n", handle );
    else if (obj)
    {
        ret = obj->param;
        release_user_handle_ptr( obj );
    }
    return ret;
}

ULONG_PTR set_icon_param( HICON handle, ULONG_PTR param )
{
    ULONG_PTR ret = 0;
    struct cursoricon_object *obj = get_user_handle_ptr( handle, USER_ICON );

    if (obj == OBJ_OTHER_PROCESS) WARN( "icon handle %p from other process\n", handle );
    else if (obj)
    {
        ret = obj->param;
        obj->param = param;
        release_user_handle_ptr( obj );
    }
    return ret;
}


/***********************************************************************
 *             map_fileW
 *
 * Helper function to map a file to memory:
 *  name			-	file name
 *  [RETURN] ptr		-	pointer to mapped file
 *  [RETURN] filesize           -       pointer size of file to be stored if not NULL
 */
static const void *map_fileW( LPCWSTR name, LPDWORD filesize )
{
    HANDLE hFile, hMapping;
    LPVOID ptr = NULL;

    hFile = CreateFileW( name, GENERIC_READ, FILE_SHARE_READ, NULL,
                         OPEN_EXISTING, FILE_FLAG_RANDOM_ACCESS, 0 );
    if (hFile != INVALID_HANDLE_VALUE)
    {
        hMapping = CreateFileMappingW( hFile, NULL, PAGE_READONLY, 0, 0, NULL );
        if (hMapping)
        {
            ptr = MapViewOfFile( hMapping, FILE_MAP_READ, 0, 0, 0 );
            CloseHandle( hMapping );
            if (filesize)
                *filesize = GetFileSize( hFile, NULL );
        }
        CloseHandle( hFile );
    }
    return ptr;
}


/***********************************************************************
 *          get_dib_image_size
 *
 * Return the size of a DIB bitmap in bytes.
 */
static int get_dib_image_size( int width, int height, int depth )
{
    return (((width * depth + 31) / 8) & ~3) * abs( height );
}


/***********************************************************************
 *           bitmap_info_size
 *
 * Return the size of the bitmap info structure including color table.
 */
int bitmap_info_size( const BITMAPINFO * info, WORD coloruse )
{
    unsigned int colors, size, masks = 0;

    if (info->bmiHeader.biSize == sizeof(BITMAPCOREHEADER))
    {
        const BITMAPCOREHEADER *core = (const BITMAPCOREHEADER *)info;
        colors = (core->bcBitCount <= 8) ? 1 << core->bcBitCount : 0;
        return sizeof(BITMAPCOREHEADER) + colors *
             ((coloruse == DIB_RGB_COLORS) ? sizeof(RGBTRIPLE) : sizeof(WORD));
    }
    else  /* assume BITMAPINFOHEADER */
    {
        colors = info->bmiHeader.biClrUsed;
        if (colors > 256) /* buffer overflow otherwise */
                colors = 256;
        if (!colors && (info->bmiHeader.biBitCount <= 8))
            colors = 1 << info->bmiHeader.biBitCount;
        if (info->bmiHeader.biCompression == BI_BITFIELDS) masks = 3;
        size = max( info->bmiHeader.biSize, sizeof(BITMAPINFOHEADER) + masks * sizeof(DWORD) );
        return size + colors * ((coloruse == DIB_RGB_COLORS) ? sizeof(RGBQUAD) : sizeof(WORD));
    }
}


/***********************************************************************
 *             copy_bitmap
 *
 * Helper function to duplicate a bitmap.
 */
static HBITMAP copy_bitmap( HBITMAP bitmap )
{
    HDC src, dst = 0;
    HBITMAP new_bitmap = 0;
    BITMAP bmp;

    if (!bitmap) return 0;
    if (!GetObjectW( bitmap, sizeof(bmp), &bmp )) return 0;

    if ((src = CreateCompatibleDC( 0 )) && (dst = CreateCompatibleDC( 0 )))
    {
        SelectObject( src, bitmap );
        if ((new_bitmap = CreateCompatibleBitmap( src, bmp.bmWidth, bmp.bmHeight )))
        {
            SelectObject( dst, new_bitmap );
            BitBlt( dst, 0, 0, bmp.bmWidth, bmp.bmHeight, src, 0, 0, SRCCOPY );
        }
    }
    DeleteDC( dst );
    DeleteDC( src );
    return new_bitmap;
}


/***********************************************************************
 *          is_dib_monochrome
 *
 * Returns whether a DIB can be converted to a monochrome DDB.
 *
 * A DIB can be converted if its color table contains only black and
 * white. Black must be the first color in the color table.
 *
 * Note : If the first color in the color table is white followed by
 *        black, we can't convert it to a monochrome DDB with
 *        SetDIBits, because black and white would be inverted.
 */
static BOOL is_dib_monochrome( const BITMAPINFO* info )
{
    if (info->bmiHeader.biSize == sizeof(BITMAPCOREHEADER))
    {
        const RGBTRIPLE *rgb = ((const BITMAPCOREINFO*)info)->bmciColors;

        if (((const BITMAPCOREINFO*)info)->bmciHeader.bcBitCount != 1) return FALSE;

        /* Check if the first color is black */
        if ((rgb->rgbtRed == 0) && (rgb->rgbtGreen == 0) && (rgb->rgbtBlue == 0))
        {
            rgb++;

            /* Check if the second color is white */
            return ((rgb->rgbtRed == 0xff) && (rgb->rgbtGreen == 0xff)
                 && (rgb->rgbtBlue == 0xff));
        }
        else return FALSE;
    }
    else  /* assume BITMAPINFOHEADER */
    {
        const RGBQUAD *rgb = info->bmiColors;

        if (info->bmiHeader.biBitCount != 1) return FALSE;

        /* Check if the first color is black */
        if ((rgb->rgbRed == 0) && (rgb->rgbGreen == 0) &&
            (rgb->rgbBlue == 0) && (rgb->rgbReserved == 0))
        {
            rgb++;

            /* Check if the second color is white */
            return ((rgb->rgbRed == 0xff) && (rgb->rgbGreen == 0xff)
                 && (rgb->rgbBlue == 0xff) && (rgb->rgbReserved == 0));
        }
        else return FALSE;
    }
}

/***********************************************************************
 *           DIB_GetBitmapInfo
 *
 * Get the info from a bitmap header.
 * Return 1 for INFOHEADER, 0 for COREHEADER, -1 in case of failure.
 */
static int DIB_GetBitmapInfo( const BITMAPINFOHEADER *header, LONG *width,
                              LONG *height, WORD *bpp, DWORD *compr )
{
    if (header->biSize == sizeof(BITMAPCOREHEADER))
    {
        const BITMAPCOREHEADER *core = (const BITMAPCOREHEADER *)header;
        *width  = core->bcWidth;
        *height = core->bcHeight;
        *bpp    = core->bcBitCount;
        *compr  = 0;
        return 0;
    }
    else if (header->biSize == sizeof(BITMAPINFOHEADER) ||
             header->biSize == sizeof(BITMAPV4HEADER) ||
             header->biSize == sizeof(BITMAPV5HEADER))
    {
        *width  = header->biWidth;
        *height = header->biHeight;
        *bpp    = header->biBitCount;
        *compr  = header->biCompression;
        return 1;
    }
    WARN("unknown/wrong size (%u) for header\n", header->biSize);
    return -1;
}

/**********************************************************************
 *              get_icon_size
 */
BOOL get_icon_size( HICON handle, SIZE *size )
{
    struct cursoricon_object *info;
    struct cursoricon_frame *frame;

    if (!(info = get_icon_ptr( handle ))) return FALSE;
    frame = get_icon_frame( info, 0 );
    size->cx = frame->width;
    size->cy = frame->height;
    release_icon_frame( info, frame);
    release_user_handle_ptr( info );
    return TRUE;
}

struct png_wrapper
{
    const char *buffer;
    size_t size, pos;
};

static void user_read_data(png_structp png_ptr, png_bytep data, png_size_t length)
{
    struct png_wrapper *png = png_get_io_ptr(png_ptr);

    if (png->size - png->pos >= length)
    {
        memcpy(data, png->buffer + png->pos, length);
        png->pos += length;
    }
    else
    {
        png_error(png_ptr, "failed to read PNG data");
    }
}

static unsigned be_uint(unsigned val)
{
    union
    {
        unsigned val;
        unsigned char c[4];
    } u;

    u.val = val;
    return (u.c[0] << 24) | (u.c[1] << 16) | (u.c[2] << 8) | u.c[3];
}

static BOOL get_png_info(const void *png_data, DWORD size, int *width, int *height, int *bpp)
{
    static const char png_sig[8] = { 0x89,'P','N','G',0x0d,0x0a,0x1a,0x0a };
    static const char png_IHDR[8] = { 0,0,0,0x0d,'I','H','D','R' };
    const struct
    {
        char png_sig[8];
        char ihdr_sig[8];
        unsigned width, height;
        char bit_depth, color_type, compression, filter, interlace;
    } *png = png_data;

    if (size < sizeof(*png)) return FALSE;
    if (memcmp(png->png_sig, png_sig, sizeof(png_sig)) != 0) return FALSE;
    if (memcmp(png->ihdr_sig, png_IHDR, sizeof(png_IHDR)) != 0) return FALSE;

    *bpp = (png->color_type == PNG_COLOR_TYPE_RGB_ALPHA) ? 32 : 24;
    *width = be_uint(png->width);
    *height = be_uint(png->height);

    return TRUE;
}

static BITMAPINFO *load_png(const char *png_data, DWORD *size)
{
    struct png_wrapper png;
    png_structp png_ptr;
    png_infop info_ptr;
    png_bytep *row_pointers = NULL;
    int color_type, bit_depth, bpp, width, height;
    int rowbytes, image_size, mask_size = 0, i;
    BITMAPINFO *info = NULL;
    unsigned char *image_data;

    if (!get_png_info(png_data, *size, &width, &height, &bpp)) return NULL;

    png.buffer = png_data;
    png.size = *size;
    png.pos = 0;

    /* initialize libpng */
    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr) return NULL;

    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr)
    {
        png_destroy_read_struct(&png_ptr, NULL, NULL);
        return NULL;
    }

    /* set up setjmp/longjmp error handling */
    if (setjmp(png_jmpbuf(png_ptr)))
    {
        free(row_pointers);
        RtlFreeHeap(GetProcessHeap(), 0, info);
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        return NULL;
    }

    png_set_crc_action(png_ptr, PNG_CRC_QUIET_USE, PNG_CRC_QUIET_USE);

    /* set up custom i/o handling */
    png_set_read_fn(png_ptr, &png, user_read_data);

    /* read the header */
    png_read_info(png_ptr, info_ptr);

    color_type = png_get_color_type(png_ptr, info_ptr);
    bit_depth = png_get_bit_depth(png_ptr, info_ptr);

    /* expand grayscale image data to rgb */
    if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
        png_set_gray_to_rgb(png_ptr);

    /* expand palette image data to rgb */
    if (color_type == PNG_COLOR_TYPE_PALETTE || bit_depth < 8)
        png_set_expand(png_ptr);

    /* update color type information */
    png_read_update_info(png_ptr, info_ptr);

    color_type = png_get_color_type(png_ptr, info_ptr);
    bit_depth = png_get_bit_depth(png_ptr, info_ptr);

    bpp = 0;

    switch (color_type)
    {
    case PNG_COLOR_TYPE_RGB:
        if (bit_depth == 8)
            bpp = 24;
        break;

    case PNG_COLOR_TYPE_RGB_ALPHA:
        if (bit_depth == 8)
        {
            png_set_bgr(png_ptr);
            bpp = 32;
        }
        break;

    default:
        break;
    }

    if (!bpp)
    {
        FIXME("unsupported PNG color format %d, %d bpp\n", color_type, bit_depth);
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        return NULL;
    }

    width = png_get_image_width(png_ptr, info_ptr);
    height = png_get_image_height(png_ptr, info_ptr);

    rowbytes = (width * bpp + 7) / 8;
    image_size = height * rowbytes;
    if (bpp != 32) /* add a mask if there is no alpha */
        mask_size = (width + 7) / 8 * height;

    info = RtlAllocateHeap(GetProcessHeap(), 0, sizeof(BITMAPINFOHEADER) + image_size + mask_size);
    if (!info)
    {
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        return NULL;
    }

    image_data = (unsigned char *)info + sizeof(BITMAPINFOHEADER);
    memset(image_data + image_size, 0, mask_size);

    row_pointers = malloc(height * sizeof(png_bytep));
    if (!row_pointers)
    {
        RtlFreeHeap(GetProcessHeap(), 0, info);
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        return NULL;
    }

    /* upside down */
    for (i = 0; i < height; i++)
        row_pointers[i] = image_data + (height - i - 1) * rowbytes;

    png_read_image(png_ptr, row_pointers);
    free(row_pointers);
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);

    info->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info->bmiHeader.biWidth = width;
    info->bmiHeader.biHeight = height * 2;
    info->bmiHeader.biPlanes = 1;
    info->bmiHeader.biBitCount = bpp;
    info->bmiHeader.biCompression = BI_RGB;
    info->bmiHeader.biSizeImage = image_size;
    info->bmiHeader.biXPelsPerMeter = 0;
    info->bmiHeader.biYPelsPerMeter = 0;
    info->bmiHeader.biClrUsed = 0;
    info->bmiHeader.biClrImportant = 0;

    *size = sizeof(BITMAPINFOHEADER) + image_size + mask_size;
    return info;
}


/*
 *  The following macro functions account for the irregularities of
 *   accessing cursor and icon resources in files and resource entries.
 */
typedef BOOL (*fnGetCIEntry)( LPCVOID dir, DWORD size, int n,
                              int *width, int *height, int *bits );

/**********************************************************************
 *	    CURSORICON_FindBestIcon
 *
 * Find the icon closest to the requested size and bit depth.
 */
static int CURSORICON_FindBestIcon( LPCVOID dir, DWORD size, fnGetCIEntry get_entry,
                                    int width, int height, int depth, UINT loadflags )
{
    int i, cx, cy, bits, bestEntry = -1;
    UINT iTotalDiff, iXDiff=0, iYDiff=0, iColorDiff;
    UINT iTempXDiff, iTempYDiff, iTempColorDiff;

    /* Find Best Fit */
    iTotalDiff = 0xFFFFFFFF;
    iColorDiff = 0xFFFFFFFF;

    if (loadflags & LR_DEFAULTSIZE)
    {
        if (!width) width = GetSystemMetrics( SM_CXICON );
        if (!height) height = GetSystemMetrics( SM_CYICON );
    }
    else if (!width && !height)
    {
        /* use the size of the first entry */
        if (!get_entry( dir, size, 0, &width, &height, &bits )) return -1;
        iTotalDiff = 0;
    }

    for ( i = 0; iTotalDiff && get_entry( dir, size, i, &cx, &cy, &bits ); i++ )
    {
        iTempXDiff = abs(width - cx);
        iTempYDiff = abs(height - cy);

        if(iTotalDiff > (iTempXDiff + iTempYDiff))
        {
            iXDiff = iTempXDiff;
            iYDiff = iTempYDiff;
            iTotalDiff = iXDiff + iYDiff;
        }
    }

    /* Find Best Colors for Best Fit */
    for ( i = 0; get_entry( dir, size, i, &cx, &cy, &bits ); i++ )
    {
        TRACE("entry %d: %d x %d, %d bpp\n", i, cx, cy, bits);

        if(abs(width - cx) == iXDiff && abs(height - cy) == iYDiff)
        {
            iTempColorDiff = abs(depth - bits);
            if(iColorDiff > iTempColorDiff)
            {
                bestEntry = i;
                iColorDiff = iTempColorDiff;
            }
        }
    }

    return bestEntry;
}

static BOOL CURSORICON_GetResIconEntry( LPCVOID dir, DWORD size, int n,
                                        int *width, int *height, int *bits )
{
    const CURSORICONDIR *resdir = dir;
    const ICONRESDIR *icon;

    if ( resdir->idCount <= n )
        return FALSE;
    if ((const char *)&resdir->idEntries[n + 1] - (const char *)dir > size)
        return FALSE;
    icon = &resdir->idEntries[n].ResInfo.icon;
    *width = icon->bWidth;
    *height = icon->bHeight;
    *bits = resdir->idEntries[n].wBitCount;
    if (!*width && !*height) *width = *height = 256;
    return TRUE;
}

/**********************************************************************
 *	    CURSORICON_FindBestCursor
 *
 * Find the cursor closest to the requested size.
 *
 * FIXME: parameter 'color' ignored.
 */
static int CURSORICON_FindBestCursor( LPCVOID dir, DWORD size, fnGetCIEntry get_entry,
                                      int width, int height, int depth, UINT loadflags )
{
    int i, maxwidth, maxheight, maxbits, cx, cy, bits, bestEntry = -1;

    if (loadflags & LR_DEFAULTSIZE)
    {
        if (!width) width = GetSystemMetrics( SM_CXCURSOR );
        if (!height) height = GetSystemMetrics( SM_CYCURSOR );
    }
    else if (!width && !height)
    {
        /* use the first entry */
        if (!get_entry( dir, size, 0, &width, &height, &bits )) return -1;
        return 0;
    }

    /* First find the largest one smaller than or equal to the requested size*/

    maxwidth = maxheight = maxbits = 0;
    for ( i = 0; get_entry( dir, size, i, &cx, &cy, &bits ); i++ )
    {
        if (cx > width || cy > height) continue;
        if (cx < maxwidth || cy < maxheight) continue;
        if (cx == maxwidth && cy == maxheight)
        {
            if (loadflags & LR_MONOCHROME)
            {
                if (maxbits && bits >= maxbits) continue;
            }
            else if (bits <= maxbits) continue;
        }
        bestEntry = i;
        maxwidth  = cx;
        maxheight = cy;
        maxbits = bits;
    }
    if (bestEntry != -1) return bestEntry;

    /* Now find the smallest one larger than the requested size */

    maxwidth = maxheight = 255;
    for ( i = 0; get_entry( dir, size, i, &cx, &cy, &bits ); i++ )
    {
        if (cx > maxwidth || cy > maxheight) continue;
        if (cx == maxwidth && cy == maxheight)
        {
            if (loadflags & LR_MONOCHROME)
            {
                if (maxbits && bits >= maxbits) continue;
            }
            else if (bits <= maxbits) continue;
        }
        bestEntry = i;
        maxwidth  = cx;
        maxheight = cy;
        maxbits = bits;
    }
    if (bestEntry == -1) bestEntry = 0;

    return bestEntry;
}

static BOOL CURSORICON_GetResCursorEntry( LPCVOID dir, DWORD size, int n,
                                          int *width, int *height, int *bits )
{
    const CURSORICONDIR *resdir = dir;
    const CURSORDIR *cursor;

    if ( resdir->idCount <= n )
        return FALSE;
    if ((const char *)&resdir->idEntries[n + 1] - (const char *)dir > size)
        return FALSE;
    cursor = &resdir->idEntries[n].ResInfo.cursor;
    *width = cursor->wWidth;
    *height = cursor->wHeight;
    *bits = resdir->idEntries[n].wBitCount;
    if (*height == *width * 2) *height /= 2;
    return TRUE;
}

static const CURSORICONDIRENTRY *CURSORICON_FindBestIconRes( const CURSORICONDIR * dir, DWORD size,
                                                             int width, int height, int depth,
                                                             UINT loadflags )
{
    int n;

    n = CURSORICON_FindBestIcon( dir, size, CURSORICON_GetResIconEntry,
                                 width, height, depth, loadflags );
    if ( n < 0 )
        return NULL;
    return &dir->idEntries[n];
}

static const CURSORICONDIRENTRY *CURSORICON_FindBestCursorRes( const CURSORICONDIR *dir, DWORD size,
                                                               int width, int height, int depth,
                                                               UINT loadflags )
{
    int n = CURSORICON_FindBestCursor( dir, size, CURSORICON_GetResCursorEntry,
                                       width, height, depth, loadflags );
    if ( n < 0 )
        return NULL;
    return &dir->idEntries[n];
}

static BOOL CURSORICON_GetFileEntry( LPCVOID dir, DWORD size, int n,
                                     int *width, int *height, int *bits )
{
    const CURSORICONFILEDIR *filedir = dir;
    const CURSORICONFILEDIRENTRY *entry;
    const BITMAPINFOHEADER *info;

    if ( filedir->idCount <= n )
        return FALSE;
    if ((const char *)&filedir->idEntries[n + 1] - (const char *)dir > size)
        return FALSE;
    entry = &filedir->idEntries[n];
    if (entry->dwDIBOffset > size - sizeof(info->biSize)) return FALSE;
    info = (const BITMAPINFOHEADER *)((const char *)dir + entry->dwDIBOffset);

    if (info->biSize == PNG_SIGN) return get_png_info(info, size, width, height, bits);

    if (info->biSize != sizeof(BITMAPCOREHEADER))
    {
        if ((const char *)(info + 1) - (const char *)dir > size) return FALSE;
        *bits = info->biBitCount;
    }
    else
    {
        const BITMAPCOREHEADER *coreinfo = (const BITMAPCOREHEADER *)((const char *)dir + entry->dwDIBOffset);
        if ((const char *)(coreinfo + 1) - (const char *)dir > size) return FALSE;
        *bits = coreinfo->bcBitCount;
    }
    *width = entry->bWidth;
    *height = entry->bHeight;
    return TRUE;
}

static const CURSORICONFILEDIRENTRY *CURSORICON_FindBestCursorFile( const CURSORICONFILEDIR *dir, DWORD size,
                                                                    int width, int height, int depth,
                                                                    UINT loadflags )
{
    int n = CURSORICON_FindBestCursor( dir, size, CURSORICON_GetFileEntry,
                                       width, height, depth, loadflags );
    if ( n < 0 )
        return NULL;
    return &dir->idEntries[n];
}

static const CURSORICONFILEDIRENTRY *CURSORICON_FindBestIconFile( const CURSORICONFILEDIR *dir, DWORD size,
                                                                  int width, int height, int depth,
                                                                  UINT loadflags )
{
    int n = CURSORICON_FindBestIcon( dir, size, CURSORICON_GetFileEntry,
                                     width, height, depth, loadflags );
    if ( n < 0 )
        return NULL;
    return &dir->idEntries[n];
}

/***********************************************************************
 *          bmi_has_alpha
 */
static BOOL bmi_has_alpha( const BITMAPINFO *info, const void *bits )
{
    int i;
    BOOL has_alpha = FALSE;
    const unsigned char *ptr = bits;

    if (info->bmiHeader.biBitCount != 32) return FALSE;
    for (i = 0; i < info->bmiHeader.biWidth * abs(info->bmiHeader.biHeight); i++, ptr += 4)
        if ((has_alpha = (ptr[3] != 0))) break;
    return has_alpha;
}

/***********************************************************************
 *          create_alpha_bitmap
 *
 * Create the alpha bitmap for a 32-bpp icon that has an alpha channel.
 */
static HBITMAP create_alpha_bitmap( HBITMAP color, const BITMAPINFO *src_info, const void *color_bits )
{
    HBITMAP alpha = 0;
    BITMAPINFO *info = NULL;
    BITMAP bm;
    HDC hdc;
    void *bits;
    unsigned char *ptr;
    int i;

    if (!GetObjectW( color, sizeof(bm), &bm )) return 0;
    if (bm.bmBitsPixel != 32) return 0;

    if (!(hdc = CreateCompatibleDC( 0 ))) return 0;
    if (!(info = HeapAlloc( GetProcessHeap(), 0, FIELD_OFFSET( BITMAPINFO, bmiColors[256] )))) goto done;
    info->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info->bmiHeader.biWidth = bm.bmWidth;
    info->bmiHeader.biHeight = -bm.bmHeight;
    info->bmiHeader.biPlanes = 1;
    info->bmiHeader.biBitCount = 32;
    info->bmiHeader.biCompression = BI_RGB;
    info->bmiHeader.biSizeImage = bm.bmWidth * bm.bmHeight * 4;
    info->bmiHeader.biXPelsPerMeter = 0;
    info->bmiHeader.biYPelsPerMeter = 0;
    info->bmiHeader.biClrUsed = 0;
    info->bmiHeader.biClrImportant = 0;
    if (!(alpha = CreateDIBSection( hdc, info, DIB_RGB_COLORS, &bits, NULL, 0 ))) goto done;

    if (src_info)
    {
        SelectObject( hdc, alpha );
        StretchDIBits( hdc, 0, 0, bm.bmWidth, bm.bmHeight,
                       0, 0, src_info->bmiHeader.biWidth, src_info->bmiHeader.biHeight,
                       color_bits, src_info, DIB_RGB_COLORS, SRCCOPY );

    }
    else
    {
        GetDIBits( hdc, color, 0, bm.bmHeight, bits, info, DIB_RGB_COLORS );
        if (!bmi_has_alpha( info, bits ))
        {
            DeleteObject( alpha );
            alpha = 0;
            goto done;
        }
    }

    /* pre-multiply by alpha */
    for (i = 0, ptr = bits; i < bm.bmWidth * bm.bmHeight; i++, ptr += 4)
    {
        unsigned int alpha = ptr[3];
        ptr[0] = ptr[0] * alpha / 255;
        ptr[1] = ptr[1] * alpha / 255;
        ptr[2] = ptr[2] * alpha / 255;
    }

done:
    DeleteDC( hdc );
    HeapFree( GetProcessHeap(), 0, info );
    return alpha;
}


/***********************************************************************
 *          create_icon_from_bmi
 *
 * Create an icon from its BITMAPINFO.
 */
static HICON create_icon_from_bmi( const BITMAPINFO *bmi, DWORD maxsize, HMODULE module, LPCWSTR resname,
                                   HRSRC rsrc, POINT hotspot, BOOL bIcon, INT width, INT height,
                                   UINT cFlag )
{
    DWORD size, color_size, mask_size;
    HBITMAP color = 0, mask = 0, alpha = 0;
    const void *color_bits, *mask_bits;
    void *alpha_mask_bits = NULL;
    BITMAPINFO *bmi_copy;
    BOOL ret = FALSE;
    BOOL do_stretch;
    HICON hObj = 0;
    HDC hdc = 0;
    LONG bmi_width, bmi_height;
    WORD bpp;
    DWORD compr;

    /* Check bitmap header */

    if (bmi->bmiHeader.biSize == PNG_SIGN)
    {
        BITMAPINFO *bmi_png = load_png( (const char *)bmi, &maxsize );

        if (bmi_png)
        {
            hObj = create_icon_from_bmi( bmi_png, maxsize, module, resname,
                                         rsrc, hotspot, bIcon, width, height, cFlag );
            HeapFree( GetProcessHeap(), 0, bmi_png );
            return hObj;
        }
        return 0;
    }

    if (maxsize < sizeof(BITMAPCOREHEADER))
    {
        WARN( "invalid size %u\n", maxsize );
        return 0;
    }
    if (maxsize < bmi->bmiHeader.biSize)
    {
        WARN( "invalid header size %u\n", bmi->bmiHeader.biSize );
        return 0;
    }
    if ( (bmi->bmiHeader.biSize != sizeof(BITMAPCOREHEADER)) &&
         (bmi->bmiHeader.biSize != sizeof(BITMAPINFOHEADER)  ||
         (bmi->bmiHeader.biCompression != BI_RGB &&
          bmi->bmiHeader.biCompression != BI_BITFIELDS)) )
    {
        WARN( "invalid bitmap header %u\n", bmi->bmiHeader.biSize );
        return 0;
    }

    size = bitmap_info_size( bmi, DIB_RGB_COLORS );
    DIB_GetBitmapInfo(&bmi->bmiHeader, &bmi_width, &bmi_height, &bpp, &compr);
    color_size = get_dib_image_size( bmi_width, bmi_height / 2,
                                     bpp );
    mask_size = get_dib_image_size( bmi_width, bmi_height / 2, 1 );
    if (size > maxsize || color_size > maxsize - size)
    {
        WARN( "truncated file %u < %u+%u+%u\n", maxsize, size, color_size, mask_size );
        return 0;
    }
    if (mask_size > maxsize - size - color_size) mask_size = 0;  /* no mask */

    if (cFlag & LR_DEFAULTSIZE)
    {
        if (!width) width = GetSystemMetrics( bIcon ? SM_CXICON : SM_CXCURSOR );
        if (!height) height = GetSystemMetrics( bIcon ? SM_CYICON : SM_CYCURSOR );
    }
    else
    {
        if (!width) width = bmi_width;
        if (!height) height = bmi_height/2;
    }
    do_stretch = (bmi_height/2 != height) ||
                 (bmi_width != width);

    /* Scale the hotspot */
    if (bIcon)
    {
        hotspot.x = width / 2;
        hotspot.y = height / 2;
    }
    else if (do_stretch)
    {
        hotspot.x = (hotspot.x * width) / bmi_width;
        hotspot.y = (hotspot.y * height) / (bmi_height / 2);
    }

    if (!(bmi_copy = HeapAlloc( GetProcessHeap(), 0, max( size, FIELD_OFFSET( BITMAPINFO, bmiColors[2] )))))
        return 0;
    if (!(hdc = CreateCompatibleDC( 0 ))) goto done;

    memcpy( bmi_copy, bmi, size );
    if (bmi_copy->bmiHeader.biSize != sizeof(BITMAPCOREHEADER))
        bmi_copy->bmiHeader.biHeight /= 2;
    else
        ((BITMAPCOREINFO *)bmi_copy)->bmciHeader.bcHeight /= 2;
    bmi_height /= 2;

    color_bits = (const char*)bmi + size;
    mask_bits = (const char*)color_bits + color_size;

    alpha = 0;
    if (is_dib_monochrome( bmi ))
    {
        if (!(mask = CreateBitmap( width, height * 2, 1, 1, NULL ))) goto done;
        color = 0;

        /* copy color data into second half of mask bitmap */
        SelectObject( hdc, mask );
        StretchDIBits( hdc, 0, height, width, height,
                       0, 0, bmi_width, bmi_height,
                       color_bits, bmi_copy, DIB_RGB_COLORS, SRCCOPY );
    }
    else
    {
        if (!(mask = CreateBitmap( width, height, 1, 1, NULL ))) goto done;
        if (!(color = create_color_bitmap( width, height )))
        {
            DeleteObject( mask );
            goto done;
        }
        SelectObject( hdc, color );
        StretchDIBits( hdc, 0, 0, width, height,
                       0, 0, bmi_width, bmi_height,
                       color_bits, bmi_copy, DIB_RGB_COLORS, SRCCOPY );

        if (bmi_has_alpha( bmi_copy, color_bits ))
        {
            alpha = create_alpha_bitmap( color, bmi_copy, color_bits );
            if (!mask_size)  /* generate mask from alpha */
            {
                LONG x, y, dst_stride = ((bmi_width + 31) / 8) & ~3;

                if ((alpha_mask_bits = heap_calloc( bmi_height, dst_stride )))
                {
                    static const unsigned char masks[] = { 0x80, 0x40, 0x20, 0x10, 0x8, 0x4, 0x2, 0x1 };
                    const DWORD *src = color_bits;
                    unsigned char *dst = alpha_mask_bits;

                    for (y = 0; y < bmi_height; y++, src += bmi_width, dst += dst_stride)
                        for (x = 0; x < bmi_width; x++)
                            if (src[x] >> 24 != 0xff) dst[x >> 3] |= masks[x & 7];

                    mask_bits = alpha_mask_bits;
                    mask_size = bmi_height * dst_stride;
                }
            }
        }

        /* convert info to monochrome to copy the mask */
        if (bmi_copy->bmiHeader.biSize != sizeof(BITMAPCOREHEADER))
        {
            RGBQUAD *rgb = bmi_copy->bmiColors;

            bmi_copy->bmiHeader.biBitCount = 1;
            bmi_copy->bmiHeader.biClrUsed = bmi_copy->bmiHeader.biClrImportant = 2;
            rgb[0].rgbBlue = rgb[0].rgbGreen = rgb[0].rgbRed = 0x00;
            rgb[1].rgbBlue = rgb[1].rgbGreen = rgb[1].rgbRed = 0xff;
            rgb[0].rgbReserved = rgb[1].rgbReserved = 0;
        }
        else
        {
            RGBTRIPLE *rgb = (RGBTRIPLE *)(((BITMAPCOREHEADER *)bmi_copy) + 1);

            ((BITMAPCOREINFO *)bmi_copy)->bmciHeader.bcBitCount = 1;
            rgb[0].rgbtBlue = rgb[0].rgbtGreen = rgb[0].rgbtRed = 0x00;
            rgb[1].rgbtBlue = rgb[1].rgbtGreen = rgb[1].rgbtRed = 0xff;
        }
    }

    if (mask_size)
    {
        SelectObject( hdc, mask );
        StretchDIBits( hdc, 0, 0, width, height,
                       0, 0, bmi_width, bmi_height,
                       mask_bits, bmi_copy, DIB_RGB_COLORS, SRCCOPY );
    }
    ret = TRUE;

done:
    DeleteDC( hdc );
    HeapFree( GetProcessHeap(), 0, bmi_copy );
    HeapFree( GetProcessHeap(), 0, alpha_mask_bits );

    if (ret)
        hObj = alloc_icon_handle( FALSE, 0 );
    if (hObj)
    {
        struct cursoricon_object *info = get_icon_ptr( hObj );
        struct cursoricon_frame *frame;

        info->is_icon = bIcon;
        info->module  = module;
        info->hotspot = hotspot;
        frame = get_icon_frame( info, 0 );
        frame->delay  = ~0;
        frame->width  = width;
        frame->height = height;
        frame->color  = color;
        frame->mask   = mask;
        frame->alpha  = alpha;
        release_icon_frame( info, frame );
        if (!IS_INTRESOURCE(resname))
        {
            info->resname = HeapAlloc( GetProcessHeap(), 0, (lstrlenW(resname) + 1) * sizeof(WCHAR) );
            if (info->resname) lstrcpyW( info->resname, resname );
        }
        else info->resname = MAKEINTRESOURCEW( LOWORD(resname) );

        if (cFlag & LR_SHARED)
        {
            info->is_shared = TRUE;
            if (module)
            {
                info->rsrc = rsrc;
                list_add_head( &icon_cache, &info->entry );
            }
        }
        release_user_handle_ptr( info );
    }
    else
    {
        DeleteObject( color );
        DeleteObject( alpha );
        DeleteObject( mask );
    }
    return hObj;
}


/**********************************************************************
 *          .ANI cursor support
 */
#define ANI_RIFF_ID RIFF_FOURCC('R', 'I', 'F', 'F')
#define ANI_LIST_ID RIFF_FOURCC('L', 'I', 'S', 'T')
#define ANI_ACON_ID RIFF_FOURCC('A', 'C', 'O', 'N')
#define ANI_anih_ID RIFF_FOURCC('a', 'n', 'i', 'h')
#define ANI_seq__ID RIFF_FOURCC('s', 'e', 'q', ' ')
#define ANI_fram_ID RIFF_FOURCC('f', 'r', 'a', 'm')
#define ANI_rate_ID RIFF_FOURCC('r', 'a', 't', 'e')

#define ANI_FLAG_ICON       0x1
#define ANI_FLAG_SEQUENCE   0x2

typedef struct {
    DWORD header_size;
    DWORD num_frames;
    DWORD num_steps;
    DWORD width;
    DWORD height;
    DWORD bpp;
    DWORD num_planes;
    DWORD display_rate;
    DWORD flags;
} ani_header;

typedef struct {
    DWORD           data_size;
    const unsigned char   *data;
} riff_chunk_t;

static void dump_ani_header( const ani_header *header )
{
    TRACE("     header size: %d\n", header->header_size);
    TRACE("          frames: %d\n", header->num_frames);
    TRACE("           steps: %d\n", header->num_steps);
    TRACE("           width: %d\n", header->width);
    TRACE("          height: %d\n", header->height);
    TRACE("             bpp: %d\n", header->bpp);
    TRACE("          planes: %d\n", header->num_planes);
    TRACE("    display rate: %d\n", header->display_rate);
    TRACE("           flags: 0x%08x\n", header->flags);
}


/*
 * RIFF:
 * DWORD "RIFF"
 * DWORD size
 * DWORD riff_id
 * BYTE[] data
 *
 * LIST:
 * DWORD "LIST"
 * DWORD size
 * DWORD list_id
 * BYTE[] data
 *
 * CHUNK:
 * DWORD chunk_id
 * DWORD size
 * BYTE[] data
 */
static void riff_find_chunk( DWORD chunk_id, DWORD chunk_type, const riff_chunk_t *parent_chunk, riff_chunk_t *chunk )
{
    const unsigned char *ptr = parent_chunk->data;
    const unsigned char *end = parent_chunk->data + (parent_chunk->data_size - (2 * sizeof(DWORD)));

    if (chunk_type == ANI_LIST_ID || chunk_type == ANI_RIFF_ID) end -= sizeof(DWORD);

    while (ptr < end)
    {
        if ((!chunk_type && *(const DWORD *)ptr == chunk_id )
                || (chunk_type && *(const DWORD *)ptr == chunk_type && *((const DWORD *)ptr + 2) == chunk_id ))
        {
            ptr += sizeof(DWORD);
            chunk->data_size = (*(const DWORD *)ptr + 1) & ~1;
            ptr += sizeof(DWORD);
            if (chunk_type == ANI_LIST_ID || chunk_type == ANI_RIFF_ID) ptr += sizeof(DWORD);
            chunk->data = ptr;

            return;
        }

        ptr += sizeof(DWORD);
        if (ptr >= end)
            break;
        ptr += (*(const DWORD *)ptr + 1) & ~1;
        ptr += sizeof(DWORD);
    }
}


/*
 * .ANI layout:
 *
 * RIFF:'ACON'                  RIFF chunk
 *     |- CHUNK:'anih'          Header
 *     |- CHUNK:'seq '          Sequence information (optional)
 *     \- LIST:'fram'           Frame list
 *            |- CHUNK:icon     Cursor frames
 *            |- CHUNK:icon
 *            |- ...
 *            \- CHUNK:icon
 */
static HCURSOR CURSORICON_CreateIconFromANI( const BYTE *bits, DWORD bits_size, INT width, INT height,
                                             INT depth, BOOL is_icon, UINT loadflags )
{
    struct animated_cursoricon_object *ani_icon_data;
    struct cursoricon_object *info;
    DWORD *frame_rates = NULL;
    DWORD *frame_seq = NULL;
    ani_header header;
    BOOL use_seq = FALSE;
    HCURSOR cursor;
    UINT i;
    BOOL error = FALSE;
    HICON *frames;

    riff_chunk_t root_chunk = { bits_size, bits };
    riff_chunk_t ACON_chunk = {0};
    riff_chunk_t anih_chunk = {0};
    riff_chunk_t fram_chunk = {0};
    riff_chunk_t rate_chunk = {0};
    riff_chunk_t seq_chunk = {0};
    const unsigned char *icon_chunk;
    const unsigned char *icon_data;

    TRACE("bits %p, bits_size %d\n", bits, bits_size);

    riff_find_chunk( ANI_ACON_ID, ANI_RIFF_ID, &root_chunk, &ACON_chunk );
    if (!ACON_chunk.data)
    {
        ERR("Failed to get root chunk.\n");
        return 0;
    }

    riff_find_chunk( ANI_anih_ID, 0, &ACON_chunk, &anih_chunk );
    if (!anih_chunk.data)
    {
        ERR("Failed to get 'anih' chunk.\n");
        return 0;
    }
    memcpy( &header, anih_chunk.data, sizeof(header) );
    dump_ani_header( &header );

    if (!(header.flags & ANI_FLAG_ICON))
    {
        FIXME("Raw animated icon/cursor data is not currently supported.\n");
        return 0;
    }

    if (header.flags & ANI_FLAG_SEQUENCE)
    {
        riff_find_chunk( ANI_seq__ID, 0, &ACON_chunk, &seq_chunk );
        if (seq_chunk.data)
        {
            frame_seq = (DWORD *) seq_chunk.data;
            use_seq = TRUE;
        }
        else
        {
            FIXME("Sequence data expected but not found, assuming steps == frames.\n");
            header.num_steps = header.num_frames;
        }
    }

    riff_find_chunk( ANI_rate_ID, 0, &ACON_chunk, &rate_chunk );
    if (rate_chunk.data)
        frame_rates = (DWORD *) rate_chunk.data;

    riff_find_chunk( ANI_fram_ID, ANI_LIST_ID, &ACON_chunk, &fram_chunk );
    if (!fram_chunk.data)
    {
        ERR("Failed to get icon list.\n");
        return 0;
    }

    cursor = alloc_icon_handle( TRUE, header.num_steps );
    if (!cursor) return 0;
    frames = HeapAlloc( GetProcessHeap(), 0, sizeof(*frames) * header.num_frames );
    if (!frames)
    {
        free_icon_handle( cursor );
        return 0;
    }

    info = get_icon_ptr( cursor );
    ani_icon_data = (struct animated_cursoricon_object *) info;
    info->is_icon = is_icon;
    ani_icon_data->num_frames = header.num_frames;

    /* The .ANI stores the display rate in jiffies (1/60s) */
    info->delay = header.display_rate;

    icon_chunk = fram_chunk.data;
    icon_data = fram_chunk.data + (2 * sizeof(DWORD));
    for (i=0; i<header.num_frames; i++)
    {
        const DWORD chunk_size = *(const DWORD *)(icon_chunk + sizeof(DWORD));
        const CURSORICONFILEDIRENTRY *entry;
        INT frameWidth, frameHeight;
        const BITMAPINFO *bmi;

        entry = CURSORICON_FindBestIconFile((const CURSORICONFILEDIR *) icon_data,
                                            bits + bits_size - icon_data,
                                            width, height, depth, loadflags );

        info->hotspot.x = entry->xHotspot;
        info->hotspot.y = entry->yHotspot;
        if (!header.width || !header.height)
        {
            frameWidth = entry->bWidth;
            frameHeight = entry->bHeight;
        }
        else
        {
            frameWidth = header.width;
            frameHeight = header.height;
        }

        frames[i] = NULL;
        if (entry->dwDIBOffset < bits + bits_size - icon_data)
        {
            bmi = (const BITMAPINFO *) (icon_data + entry->dwDIBOffset);
            /* Grab a frame from the animation */
            frames[i] = create_icon_from_bmi( bmi, bits + bits_size - (const BYTE *)bmi,
                                              NULL, NULL, NULL, info->hotspot,
                                              is_icon, frameWidth, frameHeight, loadflags );
        }

        if (!frames[i])
        {
            FIXME_(cursor)("failed to convert animated cursor frame.\n");
            error = TRUE;
            if (i == 0)
            {
                FIXME_(cursor)("Completely failed to create animated cursor!\n");
                ani_icon_data->num_frames = 0;
                release_user_handle_ptr( info );
                free_icon_handle( cursor );
                HeapFree( GetProcessHeap(), 0, frames );
                return 0;
            }
            break;
        }

        /* Advance to the next chunk */
        icon_chunk += chunk_size + (2 * sizeof(DWORD));
        icon_data = icon_chunk + (2 * sizeof(DWORD));
    }

    /* There was an error but we at least decoded the first frame, so just use that frame */
    if (error)
    {
        FIXME_(cursor)("Error creating animated cursor, only using first frame!\n");
        for (i=1; i<ani_icon_data->num_frames; i++)
            free_icon_handle( ani_icon_data->frames[i] );
        use_seq = FALSE;
        info->delay = 0;
        ani_icon_data->num_steps = 1;
        ani_icon_data->num_frames = 1;
    }

    /* Setup the animated frames in the correct sequence */
    for (i=0; i<ani_icon_data->num_steps; i++)
    {
        DWORD frame_id = use_seq ? frame_seq[i] : i;
        struct cursoricon_frame *frame;

        if (frame_id >= ani_icon_data->num_frames)
        {
            frame_id = ani_icon_data->num_frames-1;
            ERR_(cursor)("Sequence indicates frame past end of list, corrupt?\n");
        }
        ani_icon_data->frames[i] = frames[frame_id];
        frame = get_icon_frame( info, i );
        if (frame_rates)
            frame->delay = frame_rates[i];
        else
            frame->delay = ~0;
        release_icon_frame( info, frame );
    }

    HeapFree( GetProcessHeap(), 0, frames );
    release_user_handle_ptr( info );

    return cursor;
}


/**********************************************************************
 *		CreateIconFromResourceEx (USER32.@)
 *
 * FIXME: Convert to mono when cFlag is LR_MONOCHROME.
 */
HICON WINAPI CreateIconFromResourceEx( LPBYTE bits, UINT cbSize,
                                       BOOL bIcon, DWORD dwVersion,
                                       INT width, INT height,
                                       UINT cFlag )
{
    POINT hotspot;
    const BITMAPINFO *bmi;

    TRACE_(cursor)("%p (%u bytes), ver %08x, %ix%i %s %s\n",
                   bits, cbSize, dwVersion, width, height,
                   bIcon ? "icon" : "cursor", (cFlag & LR_MONOCHROME) ? "mono" : "" );

    if (!bits) return 0;

    if (dwVersion == 0x00020000)
    {
        FIXME_(cursor)("\t2.xx resources are not supported\n");
        return 0;
    }

    /* Check if the resource is an animated icon/cursor */
    if (!memcmp(bits, "RIFF", 4))
        return CURSORICON_CreateIconFromANI( bits, cbSize, width, height,
                                             0 /* default depth */, bIcon, cFlag );

    if (bIcon)
    {
        hotspot.x = width / 2;
        hotspot.y = height / 2;
        bmi = (BITMAPINFO *)bits;
    }
    else /* get the hotspot */
    {
        const SHORT *pt = (const SHORT *)bits;
        hotspot.x = pt[0];
        hotspot.y = pt[1];
        bmi = (const BITMAPINFO *)(pt + 2);
        cbSize -= 2 * sizeof(*pt);
    }

    return create_icon_from_bmi( bmi, cbSize, NULL, NULL, NULL, hotspot, bIcon, width, height, cFlag );
}


/**********************************************************************
 *		CreateIconFromResource (USER32.@)
 */
HICON WINAPI CreateIconFromResource( LPBYTE bits, UINT cbSize,
                                           BOOL bIcon, DWORD dwVersion)
{
    return CreateIconFromResourceEx( bits, cbSize, bIcon, dwVersion, 0, 0, LR_DEFAULTSIZE | LR_SHARED );
}


static HICON CURSORICON_LoadFromFile( LPCWSTR filename,
                             INT width, INT height, INT depth,
                             BOOL fCursor, UINT loadflags)
{
    const CURSORICONFILEDIRENTRY *entry;
    const CURSORICONFILEDIR *dir;
    DWORD filesize = 0;
    HICON hIcon = 0;
    const BYTE *bits;
    POINT hotspot;

    TRACE("loading %s\n", debugstr_w( filename ));

    bits = map_fileW( filename, &filesize );
    if (!bits)
        return hIcon;

    /* Check for .ani. */
    if (memcmp( bits, "RIFF", 4 ) == 0)
    {
        hIcon = CURSORICON_CreateIconFromANI( bits, filesize, width, height, depth, !fCursor, loadflags );
        goto end;
    }

    dir = (const CURSORICONFILEDIR*) bits;
    if ( filesize < FIELD_OFFSET( CURSORICONFILEDIR, idEntries[dir->idCount] ))
        goto end;

    if ( fCursor )
        entry = CURSORICON_FindBestCursorFile( dir, filesize, width, height, depth, loadflags );
    else
        entry = CURSORICON_FindBestIconFile( dir, filesize, width, height, depth, loadflags );

    if ( !entry )
        goto end;

    /* check that we don't run off the end of the file */
    if ( entry->dwDIBOffset > filesize )
        goto end;
    if ( entry->dwDIBOffset + entry->dwDIBSize > filesize )
        goto end;

    hotspot.x = entry->xHotspot;
    hotspot.y = entry->yHotspot;
    hIcon = create_icon_from_bmi( (const BITMAPINFO *)&bits[entry->dwDIBOffset], filesize - entry->dwDIBOffset,
                                  NULL, NULL, NULL, hotspot, !fCursor, width, height, loadflags );
end:
    TRACE("loaded %s -> %p\n", debugstr_w( filename ), hIcon );
    UnmapViewOfFile( bits );
    return hIcon;
}

/**********************************************************************
 *          CURSORICON_Load
 *
 * Load a cursor or icon from resource or file.
 */
static HICON CURSORICON_Load(HINSTANCE hInstance, LPCWSTR name,
                             INT width, INT height, INT depth,
                             BOOL fCursor, UINT loadflags)
{
    HANDLE handle = 0;
    HICON hIcon = 0;
    HRSRC hRsrc;
    DWORD size;
    const CURSORICONDIR *dir;
    const CURSORICONDIRENTRY *dirEntry;
    const BYTE *bits;
    WORD wResId;
    POINT hotspot;

    TRACE("%p, %s, %dx%d, depth %d, fCursor %d, flags 0x%04x\n",
          hInstance, debugstr_w(name), width, height, depth, fCursor, loadflags);

    if ( loadflags & LR_LOADFROMFILE )    /* Load from file */
        return CURSORICON_LoadFromFile( name, width, height, depth, fCursor, loadflags );

    if (!hInstance) hInstance = user32_module;  /* Load OEM cursor/icon */

    /* don't cache 16-bit instances (FIXME: should never get 16-bit instances in the first place) */
    if ((ULONG_PTR)hInstance >> 16 == 0) loadflags &= ~LR_SHARED;

    /* Get directory resource ID */

    if (!(hRsrc = FindResourceW( hInstance, name,
                                 (LPWSTR)(fCursor ? RT_GROUP_CURSOR : RT_GROUP_ICON) )))
    {
        /* try animated resource */
        if (!(hRsrc = FindResourceW( hInstance, name,
                                    (LPWSTR)(fCursor ? RT_ANICURSOR : RT_ANIICON) ))) return 0;
        if (!(handle = LoadResource( hInstance, hRsrc ))) return 0;
        bits = LockResource( handle );
        return CURSORICON_CreateIconFromANI( bits, SizeofResource( hInstance, handle ),
                                             width, height, depth, !fCursor, loadflags );
    }

    /* Find the best entry in the directory */

    if (!(handle = LoadResource( hInstance, hRsrc ))) return 0;
    if (!(dir = LockResource( handle ))) return 0;
    size = SizeofResource( hInstance, hRsrc );
    if (fCursor)
        dirEntry = CURSORICON_FindBestCursorRes( dir, size, width, height, depth, loadflags );
    else
        dirEntry = CURSORICON_FindBestIconRes( dir, size, width, height, depth, loadflags );
    if (!dirEntry) return 0;
    wResId = dirEntry->wResId;
    FreeResource( handle );

    /* Load the resource */

    if (!(hRsrc = FindResourceW(hInstance,MAKEINTRESOURCEW(wResId),
                                (LPWSTR)(fCursor ? RT_CURSOR : RT_ICON) ))) return 0;

    /* If shared icon, check whether it was already loaded */
    if (loadflags & LR_SHARED)
    {
        struct cursoricon_object *ptr;

        USER_Lock();
        LIST_FOR_EACH_ENTRY( ptr, &icon_cache, struct cursoricon_object, entry )
        {
            if (ptr->module != hInstance) continue;
            if (ptr->rsrc != hRsrc) continue;
            hIcon = ptr->obj.handle;
            break;
        }
        USER_Unlock();
        if (hIcon) return hIcon;
    }

    if (!(handle = LoadResource( hInstance, hRsrc ))) return 0;
    size = SizeofResource( hInstance, hRsrc );
    bits = LockResource( handle );

    if (!fCursor)
    {
        hotspot.x = width / 2;
        hotspot.y = height / 2;
    }
    else /* get the hotspot */
    {
        const SHORT *pt = (const SHORT *)bits;
        hotspot.x = pt[0];
        hotspot.y = pt[1];
        bits += 2 * sizeof(SHORT);
        size -= 2 * sizeof(SHORT);
    }
    hIcon = create_icon_from_bmi( (const BITMAPINFO *)bits, size, hInstance, name, hRsrc,
                                  hotspot, !fCursor, width, height, loadflags );
    FreeResource( handle );
    return hIcon;
}


static HBITMAP create_masked_bitmap( int width, int height, const void *and, const void *xor )
{
    HBITMAP and_bitmap, xor_bitmap, bitmap;
    HDC src_dc, dst_dc;

    and_bitmap = CreateBitmap( width, height, 1, 1, and );
    xor_bitmap = CreateBitmap( width, height, 1, 1, xor );
    bitmap = CreateBitmap( width, height * 2, 1, 1, NULL );
    src_dc = CreateCompatibleDC( 0 );
    dst_dc = CreateCompatibleDC( 0 );

    SelectObject( dst_dc, bitmap );
    SelectObject( src_dc, and_bitmap );
    BitBlt( dst_dc, 0, 0, width, height, src_dc, 0, 0, SRCCOPY );
    SelectObject( src_dc, xor_bitmap );
    BitBlt( dst_dc, 0, height, width, height, src_dc, 0, 0, SRCCOPY );

    DeleteObject( and_bitmap );
    DeleteObject( xor_bitmap );
    DeleteDC( src_dc );
    DeleteDC( dst_dc );
    return bitmap;
}


/***********************************************************************
 *		CreateCursor (USER32.@)
 */
HCURSOR WINAPI CreateCursor( HINSTANCE instance, int hotspot_x, int hotspot_y,
                             int width, int height, const void *and, const void *xor )
{
    ICONINFO info;
    HCURSOR cursor;

    TRACE( "hotspot (%d,%d), size %dx%d\n", hotspot_x, hotspot_y, width, height );

    info.fIcon = FALSE;
    info.xHotspot = hotspot_x;
    info.yHotspot = hotspot_y;
    info.hbmColor = NULL;
    info.hbmMask = create_masked_bitmap( width, height, and, xor );
    cursor = CreateIconIndirect( &info );
    DeleteObject( info.hbmMask );
    return cursor;
}


/***********************************************************************
 *		CreateIcon (USER32.@)
 *
 *  Creates an icon based on the specified bitmaps. The bitmaps must be
 *  provided in a device dependent format and will be resized to
 *  (SM_CXICON,SM_CYICON) and depth converted to match the screen's color
 *  depth. The provided bitmaps must be top-down bitmaps.
 *  Although Windows does not support 15bpp(*) this API must support it
 *  for Winelib applications.
 *
 *  (*) Windows does not support 15bpp but it supports the 555 RGB 16bpp
 *      format!
 *
 * RETURNS
 *  Success: handle to an icon
 *  Failure: NULL
 *
 * FIXME: Do we need to resize the bitmaps?
 */
HICON WINAPI CreateIcon( HINSTANCE instance, int width, int height, BYTE planes,
                         BYTE depth, const void *and, const void *xor )
{
    ICONINFO info;
    HICON icon;

    TRACE_(icon)( "%dx%d, planes %d, depth %d\n", width, height, planes, depth );

    info.fIcon = TRUE;
    info.xHotspot = width / 2;
    info.yHotspot = height / 2;
    if (depth == 1)
    {
        info.hbmColor = NULL;
        info.hbmMask = create_masked_bitmap( width, height, and, xor );
    }
    else
    {
        info.hbmColor = CreateBitmap( width, height, planes, depth, xor );
        info.hbmMask = CreateBitmap( width, height, 1, 1, and );
    }

    icon = CreateIconIndirect( &info );

    DeleteObject( info.hbmMask );
    DeleteObject( info.hbmColor );

    return icon;
}


/***********************************************************************
 *		CopyIcon (USER32.@)
 */
HICON WINAPI CopyIcon( HICON icon )
{
    ICONINFOEXW info;
    HICON res;

    info.cbSize = sizeof(info);
    if (!GetIconInfoExW( icon, &info ))
        return NULL;

    res = CopyImage( icon, info.fIcon ? IMAGE_ICON : IMAGE_CURSOR, 0, 0, 0 );
    DeleteObject( info.hbmColor );
    DeleteObject( info.hbmMask );
    return res;
}


/***********************************************************************
 *		DestroyIcon (USER32.@)
 */
BOOL WINAPI DestroyIcon( HICON hIcon )
{
    BOOL ret = FALSE;
    struct cursoricon_object *obj = get_icon_ptr( hIcon );

    TRACE_(icon)("%p\n", hIcon );

    if (obj)
    {
        BOOL shared = obj->is_shared;
        release_user_handle_ptr( obj );
        ret = (NtUserGetCursor() != hIcon);
        if (!shared) free_icon_handle( hIcon );
    }
    return ret;
}


/***********************************************************************
 *		DestroyCursor (USER32.@)
 */
BOOL WINAPI DestroyCursor( HCURSOR hCursor )
{
    return DestroyIcon( hCursor );
}

/***********************************************************************
 *		DrawIcon (USER32.@)
 */
BOOL WINAPI DrawIcon( HDC hdc, INT x, INT y, HICON hIcon )
{
    return DrawIconEx( hdc, x, y, hIcon, 0, 0, 0, 0, DI_NORMAL | DI_COMPAT | DI_DEFAULTSIZE );
}

/***********************************************************************
 *		SetCursor (USER32.@)
 *
 * Set the cursor shape.
 *
 * RETURNS
 *	A handle to the previous cursor shape.
 */
HCURSOR WINAPI DECLSPEC_HOTPATCH SetCursor( HCURSOR hCursor /* [in] Handle of cursor to show */ )
{
    struct cursoricon_object *obj;
    HCURSOR hOldCursor;
    int show_count;
    BOOL ret;

    TRACE("%p\n", hCursor);

    SERVER_START_REQ( set_cursor )
    {
        req->flags = SET_CURSOR_HANDLE;
        req->handle = wine_server_user_handle( hCursor );
        if ((ret = !wine_server_call_err( req )))
        {
            hOldCursor = wine_server_ptr_handle( reply->prev_handle );
            show_count = reply->prev_count;
        }
    }
    SERVER_END_REQ;

    if (!ret) return 0;
    USER_Driver->pSetCursor( show_count >= 0 ? hCursor : 0 );

    if (!(obj = get_icon_ptr( hOldCursor ))) return 0;
    release_user_handle_ptr( obj );
    return hOldCursor;
}


/***********************************************************************
 *		ClipCursor (USER32.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH ClipCursor( const RECT *rect )
{
    UINT dpi;
    BOOL ret;
    RECT new_rect;

    TRACE( "Clipping to %s\n", wine_dbgstr_rect(rect) );

    if (rect)
    {
        if (rect->left > rect->right || rect->top > rect->bottom) return FALSE;
        if ((dpi = get_thread_dpi()))
        {
            new_rect = map_dpi_rect( *rect, dpi,
                                     get_monitor_dpi( MonitorFromRect( rect, MONITOR_DEFAULTTOPRIMARY )));
            rect = &new_rect;
        }
    }

    SERVER_START_REQ( set_cursor )
    {
        req->clip_msg = WM_WINE_CLIPCURSOR;
        if (rect)
        {
            req->flags       = SET_CURSOR_CLIP;
            req->clip.left   = rect->left;
            req->clip.top    = rect->top;
            req->clip.right  = rect->right;
            req->clip.bottom = rect->bottom;
        }
        else req->flags = SET_CURSOR_NOCLIP;

        if ((ret = !wine_server_call( req )))
        {
            new_rect.left   = reply->new_clip.left;
            new_rect.top    = reply->new_clip.top;
            new_rect.right  = reply->new_clip.right;
            new_rect.bottom = reply->new_clip.bottom;
        }
    }
    SERVER_END_REQ;
    if (ret) USER_Driver->pClipCursor( &new_rect );
    return ret;
}


/***********************************************************************
 *		GetClipCursor (USER32.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetClipCursor( RECT *rect )
{
    DPI_AWARENESS_CONTEXT context;
    UINT dpi;
    BOOL ret;

    if (!rect) return FALSE;

    SERVER_START_REQ( set_cursor )
    {
        req->flags = 0;
        if ((ret = !wine_server_call( req )))
        {
            rect->left   = reply->new_clip.left;
            rect->top    = reply->new_clip.top;
            rect->right  = reply->new_clip.right;
            rect->bottom = reply->new_clip.bottom;
        }
    }
    SERVER_END_REQ;

    if (ret && (dpi = get_thread_dpi()))
    {
        context = SetThreadDpiAwarenessContext( DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE );
        *rect = map_dpi_rect( *rect, get_monitor_dpi( MonitorFromRect( rect, MONITOR_DEFAULTTOPRIMARY )), dpi );
        SetThreadDpiAwarenessContext( context );
    }
    return ret;
}


/***********************************************************************
 *		SetSystemCursor (USER32.@)
 */
BOOL WINAPI SetSystemCursor(HCURSOR hcur, DWORD id)
{
    FIXME("(%p,%08x),stub!\n",  hcur, id);
    return TRUE;
}


/**********************************************************************
 *		LookupIconIdFromDirectoryEx (USER32.@)
 */
INT WINAPI LookupIconIdFromDirectoryEx( LPBYTE xdir, BOOL bIcon,
             INT width, INT height, UINT cFlag )
{
    const CURSORICONDIR *dir = (const CURSORICONDIR*)xdir;
    UINT retVal = 0;
    if( dir && !dir->idReserved && (dir->idType & 3) )
    {
        const CURSORICONDIRENTRY* entry;
        int depth = (cFlag & LR_MONOCHROME) ? 1 : get_display_bpp();

        if( bIcon )
            entry = CURSORICON_FindBestIconRes( dir, ~0u, width, height, depth, LR_DEFAULTSIZE );
        else
            entry = CURSORICON_FindBestCursorRes( dir, ~0u, width, height, depth, LR_DEFAULTSIZE );

        if( entry ) retVal = entry->wResId;
    }
    else WARN_(cursor)("invalid resource directory\n");
    return retVal;
}

/**********************************************************************
 *              LookupIconIdFromDirectory (USER32.@)
 */
INT WINAPI LookupIconIdFromDirectory( LPBYTE dir, BOOL bIcon )
{
    return LookupIconIdFromDirectoryEx( dir, bIcon, 0, 0, bIcon ? 0 : LR_MONOCHROME );
}

/***********************************************************************
 *              LoadCursorW (USER32.@)
 */
HCURSOR WINAPI LoadCursorW(HINSTANCE hInstance, LPCWSTR name)
{
    TRACE("%p, %s\n", hInstance, debugstr_w(name));

    return LoadImageW( hInstance, name, IMAGE_CURSOR, 0, 0,
                       LR_SHARED | LR_DEFAULTSIZE );
}

/***********************************************************************
 *		LoadCursorA (USER32.@)
 */
HCURSOR WINAPI LoadCursorA(HINSTANCE hInstance, LPCSTR name)
{
    TRACE("%p, %s\n", hInstance, debugstr_a(name));

    return LoadImageA( hInstance, name, IMAGE_CURSOR, 0, 0,
                       LR_SHARED | LR_DEFAULTSIZE );
}

/***********************************************************************
 *		LoadCursorFromFileW (USER32.@)
 */
HCURSOR WINAPI LoadCursorFromFileW (LPCWSTR name)
{
    TRACE("%s\n", debugstr_w(name));

    return LoadImageW( 0, name, IMAGE_CURSOR, 0, 0,
                       LR_LOADFROMFILE | LR_DEFAULTSIZE );
}

/***********************************************************************
 *		LoadCursorFromFileA (USER32.@)
 */
HCURSOR WINAPI LoadCursorFromFileA (LPCSTR name)
{
    TRACE("%s\n", debugstr_a(name));

    return LoadImageA( 0, name, IMAGE_CURSOR, 0, 0,
                       LR_LOADFROMFILE | LR_DEFAULTSIZE );
}

/***********************************************************************
 *		LoadIconW (USER32.@)
 */
HICON WINAPI LoadIconW(HINSTANCE hInstance, LPCWSTR name)
{
    TRACE("%p, %s\n", hInstance, debugstr_w(name));

    return LoadImageW( hInstance, name, IMAGE_ICON, 0, 0,
                       LR_SHARED | LR_DEFAULTSIZE );
}

/***********************************************************************
 *              LoadIconA (USER32.@)
 */
HICON WINAPI LoadIconA(HINSTANCE hInstance, LPCSTR name)
{
    TRACE("%p, %s\n", hInstance, debugstr_a(name));

    return LoadImageA( hInstance, name, IMAGE_ICON, 0, 0,
                       LR_SHARED | LR_DEFAULTSIZE );
}

/**********************************************************************
 *              GetCursorFrameInfo (USER32.@)
 *
 * NOTES
 *    So far no use has been found for the second parameter, it is currently presumed
 *    that this parameter is reserved for future use.
 *
 * PARAMS
 *    hCursor      [I] Handle to cursor for which to retrieve information
 *    reserved     [I] No purpose has been found for this parameter (may be NULL)
 *    istep        [I] The step of the cursor for which to retrieve information
 *    rate_jiffies [O] Pointer to DWORD that receives the frame-specific delay (cannot be NULL)
 *    num_steps    [O] Pointer to DWORD that receives the number of steps in the cursor (cannot be NULL)
 *
 * RETURNS
 *    Success: Handle to a frame of the cursor (specified by istep)
 *    Failure: NULL cursor (0)
 */
HCURSOR WINAPI GetCursorFrameInfo(HCURSOR hCursor, DWORD reserved, DWORD istep, DWORD *rate_jiffies, DWORD *num_steps)
{
    struct cursoricon_object *ptr;
    HCURSOR ret = 0;
    UINT icon_steps;

    if (rate_jiffies == NULL || num_steps == NULL) return 0;

    if (!(ptr = get_icon_ptr( hCursor ))) return 0;

    TRACE("%p => %d %d %p %p\n", hCursor, reserved, istep, rate_jiffies, num_steps);
    if (reserved != 0)
        FIXME("Second parameter non-zero (%d), please report this!\n", reserved);

    icon_steps = get_icon_steps(ptr);
    if (istep < icon_steps || !ptr->is_ani)
    {
        struct animated_cursoricon_object *ani_icon_data = (struct animated_cursoricon_object *) ptr;
        UINT icon_frames = 1;

        if (ptr->is_ani)
            icon_frames = ani_icon_data->num_frames;
        if (ptr->is_ani && icon_frames > 1)
            ret = ani_icon_data->frames[istep];
        else
            ret = hCursor;
        if (icon_frames == 1)
        {
            *rate_jiffies = 0;
            *num_steps = 1;
        }
        else if (icon_steps == 1)
        {
            *num_steps = ~0;
            *rate_jiffies = ptr->delay;
        }
        else if (istep < icon_steps)
        {
            struct cursoricon_frame *frame;

            *num_steps = icon_steps;
            frame = get_icon_frame( ptr, istep );
            if (get_icon_steps(ptr) == 1)
                *num_steps = ~0;
            else
                *num_steps = get_icon_steps(ptr);
            /* If this specific frame does not have a delay then use the global delay */
            if (frame->delay == ~0)
                *rate_jiffies = ptr->delay;
            else
                *rate_jiffies = frame->delay;
            release_icon_frame( ptr, frame );
        }
    }

    release_user_handle_ptr( ptr );

    return ret;
}

/**********************************************************************
 *              GetIconInfo (USER32.@)
 */
BOOL WINAPI GetIconInfo(HICON hIcon, PICONINFO iconinfo)
{
    ICONINFOEXW infoW;

    infoW.cbSize = sizeof(infoW);
    if (!GetIconInfoExW( hIcon, &infoW )) return FALSE;
    iconinfo->fIcon    = infoW.fIcon;
    iconinfo->xHotspot = infoW.xHotspot;
    iconinfo->yHotspot = infoW.yHotspot;
    iconinfo->hbmColor = infoW.hbmColor;
    iconinfo->hbmMask  = infoW.hbmMask;
    return TRUE;
}

/**********************************************************************
 *              GetIconInfoExA (USER32.@)
 */
BOOL WINAPI GetIconInfoExA( HICON icon, ICONINFOEXA *info )
{
    ICONINFOEXW infoW;

    if (info->cbSize != sizeof(*info))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    infoW.cbSize = sizeof(infoW);
    if (!GetIconInfoExW( icon, &infoW )) return FALSE;
    info->fIcon    = infoW.fIcon;
    info->xHotspot = infoW.xHotspot;
    info->yHotspot = infoW.yHotspot;
    info->hbmColor = infoW.hbmColor;
    info->hbmMask  = infoW.hbmMask;
    info->wResID   = infoW.wResID;
    WideCharToMultiByte( CP_ACP, 0, infoW.szModName, -1, info->szModName, MAX_PATH, NULL, NULL );
    WideCharToMultiByte( CP_ACP, 0, infoW.szResName, -1, info->szResName, MAX_PATH, NULL, NULL );
    return TRUE;
}

/**********************************************************************
 *              GetIconInfoExW (USER32.@)
 */
BOOL WINAPI GetIconInfoExW( HICON icon, ICONINFOEXW *info )
{
    struct cursoricon_frame *frame;
    struct cursoricon_object *ptr;
    HMODULE module;
    BOOL ret = TRUE;

    if (info->cbSize != sizeof(*info))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    if (!(ptr = get_icon_ptr( icon )))
    {
        SetLastError( ERROR_INVALID_CURSOR_HANDLE );
        return FALSE;
    }

    frame = get_icon_frame( ptr, 0 );
    if (!frame)
    {
        release_user_handle_ptr( ptr );
        SetLastError( ERROR_INVALID_CURSOR_HANDLE );
        return FALSE;
    }

    TRACE("%p => %dx%d\n", icon, frame->width, frame->height);

    info->fIcon        = ptr->is_icon;
    info->xHotspot     = ptr->hotspot.x;
    info->yHotspot     = ptr->hotspot.y;
    info->hbmColor     = copy_bitmap( frame->color );
    info->hbmMask      = copy_bitmap( frame->mask );
    info->wResID       = 0;
    info->szModName[0] = 0;
    info->szResName[0] = 0;
    if (ptr->module)
    {
        if (IS_INTRESOURCE( ptr->resname )) info->wResID = LOWORD( ptr->resname );
        else lstrcpynW( info->szResName, ptr->resname, MAX_PATH );
    }
    if (!info->hbmMask || (!info->hbmColor && frame->color))
    {
        DeleteObject( info->hbmMask );
        DeleteObject( info->hbmColor );
        ret = FALSE;
    }
    module = ptr->module;
    release_icon_frame( ptr, frame );
    release_user_handle_ptr( ptr );
    if (ret && module) GetModuleFileNameW( module, info->szModName, MAX_PATH );
    return ret;
}

/* copy an icon bitmap, even when it can't be selected into a DC */
/* helper for CreateIconIndirect */
static void stretch_blt_icon( HDC hdc_dst, int dst_x, int dst_y, int dst_width, int dst_height,
                              HBITMAP src, int width, int height )
{
    HDC hdc = CreateCompatibleDC( 0 );

    if (!SelectObject( hdc, src ))  /* do it the hard way */
    {
        BITMAPINFO *info;
        void *bits;

        if (!(info = HeapAlloc( GetProcessHeap(), 0, FIELD_OFFSET( BITMAPINFO, bmiColors[256] )))) return;
        info->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        info->bmiHeader.biWidth = width;
        info->bmiHeader.biHeight = height;
        info->bmiHeader.biPlanes = GetDeviceCaps( hdc_dst, PLANES );
        info->bmiHeader.biBitCount = GetDeviceCaps( hdc_dst, BITSPIXEL );
        info->bmiHeader.biCompression = BI_RGB;
        info->bmiHeader.biSizeImage = get_dib_image_size( width, height, info->bmiHeader.biBitCount );
        info->bmiHeader.biXPelsPerMeter = 0;
        info->bmiHeader.biYPelsPerMeter = 0;
        info->bmiHeader.biClrUsed = 0;
        info->bmiHeader.biClrImportant = 0;
        bits = HeapAlloc( GetProcessHeap(), 0, info->bmiHeader.biSizeImage );
        if (bits && GetDIBits( hdc, src, 0, height, bits, info, DIB_RGB_COLORS ))
            StretchDIBits( hdc_dst, dst_x, dst_y, dst_width, dst_height,
                           0, 0, width, height, bits, info, DIB_RGB_COLORS, SRCCOPY );

        HeapFree( GetProcessHeap(), 0, bits );
        HeapFree( GetProcessHeap(), 0, info );
    }
    else StretchBlt( hdc_dst, dst_x, dst_y, dst_width, dst_height, hdc, 0, 0, width, height, SRCCOPY );

    DeleteDC( hdc );
}

/**********************************************************************
 *		CreateIconIndirect (USER32.@)
 */
HICON WINAPI CreateIconIndirect(PICONINFO iconinfo)
{
    BITMAP bmpXor, bmpAnd;
    HICON hObj;
    HBITMAP color = 0, mask;
    int width, height;
    HDC hdc;

    TRACE("color %p, mask %p, hotspot %ux%u, fIcon %d\n",
           iconinfo->hbmColor, iconinfo->hbmMask,
           iconinfo->xHotspot, iconinfo->yHotspot, iconinfo->fIcon);

    if (!iconinfo->hbmMask) return 0;

    GetObjectW( iconinfo->hbmMask, sizeof(bmpAnd), &bmpAnd );
    TRACE("mask: width %d, height %d, width bytes %d, planes %u, bpp %u\n",
           bmpAnd.bmWidth, bmpAnd.bmHeight, bmpAnd.bmWidthBytes,
           bmpAnd.bmPlanes, bmpAnd.bmBitsPixel);

    if (iconinfo->hbmColor)
    {
        GetObjectW( iconinfo->hbmColor, sizeof(bmpXor), &bmpXor );
        TRACE("color: width %d, height %d, width bytes %d, planes %u, bpp %u\n",
               bmpXor.bmWidth, bmpXor.bmHeight, bmpXor.bmWidthBytes,
               bmpXor.bmPlanes, bmpXor.bmBitsPixel);

        width = bmpXor.bmWidth;
        height = bmpXor.bmHeight;
        color = create_color_bitmap( width, height );
    }
    else
    {
        width = bmpAnd.bmWidth;
        height = bmpAnd.bmHeight;
    }
    mask = CreateBitmap( width, height, 1, 1, NULL );

    hdc = CreateCompatibleDC( 0 );
    SelectObject( hdc, mask );
    stretch_blt_icon( hdc, 0, 0, width, height, iconinfo->hbmMask, bmpAnd.bmWidth, bmpAnd.bmHeight );

    if (color)
    {
        SelectObject( hdc, color );
        stretch_blt_icon( hdc, 0, 0, width, height, iconinfo->hbmColor, width, height );
    }
    else height /= 2;

    DeleteDC( hdc );

    hObj = alloc_icon_handle( FALSE, 0 );
    if (hObj)
    {
        struct cursoricon_object *info = get_icon_ptr( hObj );
        struct cursoricon_frame *frame;

        info->is_icon = iconinfo->fIcon;
        frame = get_icon_frame( info, 0 );
        frame->delay  = ~0;
        frame->width  = width;
        frame->height = height;
        frame->color  = color;
        frame->mask   = mask;
        frame->alpha  = create_alpha_bitmap( iconinfo->hbmColor, NULL, NULL );
        release_icon_frame( info, frame );
        if (info->is_icon)
        {
            info->hotspot.x = width / 2;
            info->hotspot.y = height / 2;
        }
        else
        {
            info->hotspot.x = iconinfo->xHotspot;
            info->hotspot.y = iconinfo->yHotspot;
        }

        release_user_handle_ptr( info );
    }
    return hObj;
}

/******************************************************************************
 *		DrawIconEx (USER32.@) Draws an icon or cursor on device context
 *
 * NOTES
 *    Why is this using SM_CXICON instead of SM_CXCURSOR?
 *
 * PARAMS
 *    hdc     [I] Handle to device context
 *    x0      [I] X coordinate of upper left corner
 *    y0      [I] Y coordinate of upper left corner
 *    hIcon   [I] Handle to icon to draw
 *    cxWidth [I] Width of icon
 *    cyWidth [I] Height of icon
 *    istep   [I] Index of frame in animated cursor
 *    hbr     [I] Handle to background brush
 *    flags   [I] Icon-drawing flags
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL WINAPI DrawIconEx( HDC hdc, INT x0, INT y0, HICON hIcon,
                            INT cxWidth, INT cyWidth, UINT istep,
                            HBRUSH hbr, UINT flags )
{
    struct cursoricon_frame *frame;
    struct cursoricon_object *ptr;
    HDC hdc_dest, hMemDC;
    BOOL result = FALSE, DoOffscreen;
    HBITMAP hB_off = 0;
    COLORREF oldFg, oldBg;
    INT x, y, nStretchMode;

    TRACE_(icon)("(hdc=%p,pos=%d.%d,hicon=%p,extend=%d.%d,istep=%d,br=%p,flags=0x%08x)\n",
                 hdc,x0,y0,hIcon,cxWidth,cyWidth,istep,hbr,flags );

    if (!(ptr = get_icon_ptr( hIcon ))) return FALSE;
    if (istep >= get_icon_steps( ptr ))
    {
        TRACE_(icon)("Stepped past end of animated frames=%d\n", istep);
        release_user_handle_ptr( ptr );
        return FALSE;
    }
    if (!(frame = get_icon_frame( ptr, istep )))
    {
        FIXME_(icon)("Error retrieving icon frame %d\n", istep);
        release_user_handle_ptr( ptr );
        return FALSE;
    }
    if (!(hMemDC = CreateCompatibleDC( hdc )))
    {
        release_icon_frame( ptr, frame );
        release_user_handle_ptr( ptr );
        return FALSE;
    }

    if (flags & DI_NOMIRROR)
        FIXME_(icon)("Ignoring flag DI_NOMIRROR\n");

    /* Calculate the size of the destination image.  */
    if (cxWidth == 0)
    {
        if (flags & DI_DEFAULTSIZE)
            cxWidth = GetSystemMetrics (SM_CXICON);
        else
            cxWidth = frame->width;
    }
    if (cyWidth == 0)
    {
        if (flags & DI_DEFAULTSIZE)
            cyWidth = GetSystemMetrics (SM_CYICON);
        else
            cyWidth = frame->height;
    }

    DoOffscreen = (GetObjectType( hbr ) == OBJ_BRUSH);

    if (DoOffscreen) {
        RECT r;

        SetRect(&r, 0, 0, cxWidth, cxWidth);

        if (!(hdc_dest = CreateCompatibleDC(hdc))) goto failed;
        if (!(hB_off = CreateCompatibleBitmap(hdc, cxWidth, cyWidth)))
        {
            DeleteDC( hdc_dest );
            goto failed;
        }
        SelectObject(hdc_dest, hB_off);
        FillRect(hdc_dest, &r, hbr);
        x = y = 0;
    }
    else
    {
        hdc_dest = hdc;
        x = x0;
        y = y0;
    }

    nStretchMode = SetStretchBltMode (hdc, STRETCH_DELETESCANS);

    oldFg = SetTextColor( hdc, RGB(0,0,0) );
    oldBg = SetBkColor( hdc, RGB(255,255,255) );

    if (frame->alpha && (flags & DI_IMAGE))
    {
        BOOL alpha_blend = TRUE;

        if (GetObjectType( hdc_dest ) == OBJ_MEMDC)
        {
            BITMAP bm;
            HBITMAP bmp = GetCurrentObject( hdc_dest, OBJ_BITMAP );
            alpha_blend = GetObjectW( bmp, sizeof(bm), &bm ) && bm.bmBitsPixel > 8;
        }
        if (alpha_blend)
        {
            BLENDFUNCTION pixelblend = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
            SelectObject( hMemDC, frame->alpha );
            if (GdiAlphaBlend( hdc_dest, x, y, cxWidth, cyWidth, hMemDC,
                               0, 0, frame->width, frame->height,
                               pixelblend )) goto done;
        }
    }

    if (flags & DI_MASK)
    {
        DWORD rop = (flags & DI_IMAGE) ? SRCAND : SRCCOPY;
        SelectObject( hMemDC, frame->mask );
        StretchBlt( hdc_dest, x, y, cxWidth, cyWidth,
                    hMemDC, 0, 0, frame->width, frame->height, rop );
    }

    if (flags & DI_IMAGE)
    {
        if (frame->color)
        {
            DWORD rop = (flags & DI_MASK) ? SRCINVERT : SRCCOPY;
            SelectObject( hMemDC, frame->color );
            StretchBlt( hdc_dest, x, y, cxWidth, cyWidth,
                        hMemDC, 0, 0, frame->width, frame->height, rop );
        }
        else
        {
            DWORD rop = (flags & DI_MASK) ? SRCINVERT : SRCCOPY;
            SelectObject( hMemDC, frame->mask );
            StretchBlt( hdc_dest, x, y, cxWidth, cyWidth,
                        hMemDC, 0, frame->height, frame->width,
                        frame->height, rop );
        }
    }

done:
    if (DoOffscreen) BitBlt( hdc, x0, y0, cxWidth, cyWidth, hdc_dest, 0, 0, SRCCOPY );

    SetTextColor( hdc, oldFg );
    SetBkColor( hdc, oldBg );
    SetStretchBltMode (hdc, nStretchMode);
    result = TRUE;
    if (hdc_dest != hdc) DeleteDC( hdc_dest );
    if (hB_off) DeleteObject(hB_off);
failed:
    DeleteDC( hMemDC );
    release_icon_frame( ptr, frame );
    release_user_handle_ptr( ptr );
    return result;
}

/***********************************************************************
 *           DIB_FixColorsToLoadflags
 *
 * Change color table entries when LR_LOADTRANSPARENT or LR_LOADMAP3DCOLORS
 * are in loadflags
 */
static void DIB_FixColorsToLoadflags(BITMAPINFO * bmi, UINT loadflags, BYTE pix)
{
    int colors;
    COLORREF c_W, c_S, c_F, c_L, c_C;
    int incr,i;
    RGBQUAD *ptr;
    int bitmap_type;
    LONG width;
    LONG height;
    WORD bpp;
    DWORD compr;

    if (((bitmap_type = DIB_GetBitmapInfo((BITMAPINFOHEADER*) bmi, &width, &height, &bpp, &compr)) == -1))
    {
        WARN_(resource)("Invalid bitmap\n");
        return;
    }

    if (bpp > 8) return;

    if (bitmap_type == 0) /* BITMAPCOREHEADER */
    {
        incr = 3;
        colors = 1 << bpp;
    }
    else
    {
        incr = 4;
        colors = bmi->bmiHeader.biClrUsed;
        if (colors > 256) colors = 256;
        if (!colors && (bpp <= 8)) colors = 1 << bpp;
    }

    c_W = GetSysColor(COLOR_WINDOW);
    c_S = GetSysColor(COLOR_3DSHADOW);
    c_F = GetSysColor(COLOR_3DFACE);
    c_L = GetSysColor(COLOR_3DLIGHT);

    if (loadflags & LR_LOADTRANSPARENT) {
        switch (bpp) {
        case 1: pix = pix >> 7; break;
        case 4: pix = pix >> 4; break;
        case 8: break;
        default:
            WARN_(resource)("(%d): Unsupported depth\n", bpp);
            return;
        }
        if (pix >= colors) {
            WARN_(resource)("pixel has color index greater than biClrUsed!\n");
            return;
        }
        if (loadflags & LR_LOADMAP3DCOLORS) c_W = c_F;
        ptr = (RGBQUAD*)((char*)bmi->bmiColors+pix*incr);
        ptr->rgbBlue = GetBValue(c_W);
        ptr->rgbGreen = GetGValue(c_W);
        ptr->rgbRed = GetRValue(c_W);
    }
    if (loadflags & LR_LOADMAP3DCOLORS)
        for (i=0; i<colors; i++) {
            ptr = (RGBQUAD*)((char*)bmi->bmiColors+i*incr);
            c_C = RGB(ptr->rgbRed, ptr->rgbGreen, ptr->rgbBlue);
            if (c_C == RGB(128, 128, 128)) {
                ptr->rgbRed = GetRValue(c_S);
                ptr->rgbGreen = GetGValue(c_S);
                ptr->rgbBlue = GetBValue(c_S);
            } else if (c_C == RGB(192, 192, 192)) {
                ptr->rgbRed = GetRValue(c_F);
                ptr->rgbGreen = GetGValue(c_F);
                ptr->rgbBlue = GetBValue(c_F);
            } else if (c_C == RGB(223, 223, 223)) {
                ptr->rgbRed = GetRValue(c_L);
                ptr->rgbGreen = GetGValue(c_L);
                ptr->rgbBlue = GetBValue(c_L);
            }
        }
}


/**********************************************************************
 *       BITMAP_Load
 */
static HBITMAP BITMAP_Load( HINSTANCE instance, LPCWSTR name,
                            INT desiredx, INT desiredy, UINT loadflags )
{
    HBITMAP hbitmap = 0, orig_bm;
    HRSRC hRsrc;
    HGLOBAL handle;
    const char *ptr = NULL;
    BITMAPINFO *info, *fix_info = NULL, *scaled_info = NULL;
    int size;
    BYTE pix;
    char *bits;
    LONG width, height, new_width, new_height;
    WORD bpp_dummy;
    DWORD compr_dummy, offbits = 0;
    INT bm_type;
    HDC screen_mem_dc = NULL;

    if (!(loadflags & LR_LOADFROMFILE))
    {
        if (!instance)
        {
            /* OEM bitmap: try to load the resource from user32.dll */
            instance = user32_module;
        }

        if (!(hRsrc = FindResourceW( instance, name, (LPWSTR)RT_BITMAP ))) return 0;
        if (!(handle = LoadResource( instance, hRsrc ))) return 0;

        if ((info = LockResource( handle )) == NULL) return 0;
    }
    else
    {
        BITMAPFILEHEADER * bmfh;

        if (!(ptr = map_fileW( name, NULL ))) return 0;
        info = (BITMAPINFO *)(ptr + sizeof(BITMAPFILEHEADER));
        bmfh = (BITMAPFILEHEADER *)ptr;
        if (bmfh->bfType != 0x4d42 /* 'BM' */)
        {
            WARN("Invalid/unsupported bitmap format!\n");
            goto end;
        }
        if (bmfh->bfOffBits) offbits = bmfh->bfOffBits - sizeof(BITMAPFILEHEADER);
    }

    bm_type = DIB_GetBitmapInfo( &info->bmiHeader, &width, &height,
                                 &bpp_dummy, &compr_dummy);
    if (bm_type == -1)
    {
        WARN("Invalid bitmap format!\n");
        goto end;
    }

    size = bitmap_info_size(info, DIB_RGB_COLORS);
    fix_info = HeapAlloc(GetProcessHeap(), 0, size);
    scaled_info = HeapAlloc(GetProcessHeap(), 0, size);

    if (!fix_info || !scaled_info) goto end;
    memcpy(fix_info, info, size);

    pix = *((LPBYTE)info + size);
    DIB_FixColorsToLoadflags(fix_info, loadflags, pix);

    memcpy(scaled_info, fix_info, size);

    if(desiredx != 0)
        new_width = desiredx;
    else
        new_width = width;

    if(desiredy != 0)
        new_height = height > 0 ? desiredy : -desiredy;
    else
        new_height = height;

    if(bm_type == 0)
    {
        BITMAPCOREHEADER *core = (BITMAPCOREHEADER *)&scaled_info->bmiHeader;
        core->bcWidth = new_width;
        core->bcHeight = new_height;
    }
    else
    {
        /* Some sanity checks for BITMAPINFO (not applicable to BITMAPCOREINFO) */
        if (info->bmiHeader.biHeight > 65535 || info->bmiHeader.biWidth > 65535) {
            WARN("Broken BitmapInfoHeader!\n");
            goto end;
        }

        scaled_info->bmiHeader.biWidth = new_width;
        scaled_info->bmiHeader.biHeight = new_height;
    }

    if (new_height < 0) new_height = -new_height;

    if (!(screen_mem_dc = CreateCompatibleDC( 0 ))) goto end;

    bits = (char *)info + (offbits ? offbits : size);

    if (loadflags & LR_CREATEDIBSECTION)
    {
        scaled_info->bmiHeader.biCompression = 0; /* DIBSection can't be compressed */
        hbitmap = CreateDIBSection(0, scaled_info, DIB_RGB_COLORS, NULL, 0, 0);
    }
    else
    {
        if (is_dib_monochrome(fix_info))
            hbitmap = CreateBitmap(new_width, new_height, 1, 1, NULL);
        else
            hbitmap = create_color_bitmap(new_width, new_height);
    }

    orig_bm = SelectObject(screen_mem_dc, hbitmap);
    if (info->bmiHeader.biBitCount > 1)
        SetStretchBltMode(screen_mem_dc, HALFTONE);
    StretchDIBits(screen_mem_dc, 0, 0, new_width, new_height, 0, 0, width, height, bits, fix_info, DIB_RGB_COLORS, SRCCOPY);
    SelectObject(screen_mem_dc, orig_bm);

end:
    if (screen_mem_dc) DeleteDC(screen_mem_dc);
    HeapFree(GetProcessHeap(), 0, scaled_info);
    HeapFree(GetProcessHeap(), 0, fix_info);
    if (loadflags & LR_LOADFROMFILE) UnmapViewOfFile( ptr );

    return hbitmap;
}

/**********************************************************************
 *		LoadImageA (USER32.@)
 *
 * See LoadImageW.
 */
HANDLE WINAPI LoadImageA( HINSTANCE hinst, LPCSTR name, UINT type,
                              INT desiredx, INT desiredy, UINT loadflags)
{
    HANDLE res;
    LPWSTR u_name;

    if (IS_INTRESOURCE(name))
        return LoadImageW(hinst, (LPCWSTR)name, type, desiredx, desiredy, loadflags);

    __TRY {
        DWORD len = MultiByteToWideChar( CP_ACP, 0, name, -1, NULL, 0 );
        u_name = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) );
        MultiByteToWideChar( CP_ACP, 0, name, -1, u_name, len );
    }
    __EXCEPT_PAGE_FAULT {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }
    __ENDTRY
    res = LoadImageW(hinst, u_name, type, desiredx, desiredy, loadflags);
    HeapFree(GetProcessHeap(), 0, u_name);
    return res;
}


/******************************************************************************
 *		LoadImageW (USER32.@) Loads an icon, cursor, or bitmap
 *
 * PARAMS
 *    hinst     [I] Handle of instance that contains image
 *    name      [I] Name of image
 *    type      [I] Type of image
 *    desiredx  [I] Desired width
 *    desiredy  [I] Desired height
 *    loadflags [I] Load flags
 *
 * RETURNS
 *    Success: Handle to newly loaded image
 *    Failure: NULL
 *
 * FIXME: Implementation lacks some features, see LR_ defines in winuser.h
 */
HANDLE WINAPI LoadImageW( HINSTANCE hinst, LPCWSTR name, UINT type,
                INT desiredx, INT desiredy, UINT loadflags )
{
    int depth;
    WCHAR path[MAX_PATH];

    TRACE_(resource)("(%p,%s,%d,%d,%d,0x%08x)\n",
                     hinst,debugstr_w(name),type,desiredx,desiredy,loadflags);

    if (loadflags & LR_LOADFROMFILE)
    {
        loadflags &= ~LR_SHARED;
        /* relative paths are not only relative to the current working directory */
        if (SearchPathW(NULL, name, NULL, ARRAY_SIZE(path), path, NULL)) name = path;
    }
    switch (type) {
    case IMAGE_BITMAP:
        return BITMAP_Load( hinst, name, desiredx, desiredy, loadflags );

    case IMAGE_ICON:
    case IMAGE_CURSOR:
        depth = 1;
        if (!(loadflags & LR_MONOCHROME)) depth = get_display_bpp();
        return CURSORICON_Load(hinst, name, desiredx, desiredy, depth, (type == IMAGE_CURSOR), loadflags);
    }
    return 0;
}


/* StretchBlt from src to dest; helper for CopyImage(). */
static void stretch_bitmap( HBITMAP dst, HBITMAP src, int dst_width, int dst_height, int src_width, int src_height )
{
    HDC src_dc = CreateCompatibleDC( 0 ), dst_dc = CreateCompatibleDC( 0 );

    SelectObject( src_dc, src );
    SelectObject( dst_dc, dst );
    StretchBlt( dst_dc, 0, 0, dst_width, dst_height, src_dc, 0, 0, src_width, src_height, SRCCOPY );

    DeleteDC( src_dc );
    DeleteDC( dst_dc );
}


/******************************************************************************
 *		CopyImage (USER32.@) Creates new image and copies attributes to it
 *
 * PARAMS
 *    hnd      [I] Handle to image to copy
 *    type     [I] Type of image to copy
 *    desiredx [I] Desired width of new image
 *    desiredy [I] Desired height of new image
 *    flags    [I] Copy flags
 *
 * RETURNS
 *    Success: Handle to newly created image
 *    Failure: NULL
 *
 * BUGS
 *    Only Windows NT 4.0 supports the LR_COPYRETURNORG flag for bitmaps,
 *    all other versions (95/2000/XP have been tested) ignore it.
 *
 * NOTES
 *    If LR_CREATEDIBSECTION is absent, the copy will be monochrome for
 *    a monochrome source bitmap or if LR_MONOCHROME is present, otherwise
 *    the copy will have the same depth as the screen.
 *    The content of the image will only be copied if the bit depth of the
 *    original image is compatible with the bit depth of the screen, or
 *    if the source is a DIB section.
 *    The LR_MONOCHROME flag is ignored if LR_CREATEDIBSECTION is present.
 */
HANDLE WINAPI CopyImage( HANDLE hnd, UINT type, INT desiredx,
                             INT desiredy, UINT flags )
{
    TRACE("hnd=%p, type=%u, desiredx=%d, desiredy=%d, flags=%x\n",
          hnd, type, desiredx, desiredy, flags);

    switch (type)
    {
        case IMAGE_BITMAP:
        {
            HBITMAP res = NULL;
            DIBSECTION ds;
            int objSize;
            BITMAPINFO * bi;

            objSize = GetObjectW( hnd, sizeof(ds), &ds );
            if (!objSize) return 0;
            if ((desiredx < 0) || (desiredy < 0)) return 0;

            if (flags & LR_COPYFROMRESOURCE)
            {
                FIXME("The flag LR_COPYFROMRESOURCE is not implemented for bitmaps\n");
            }

            if (desiredx == 0) desiredx = ds.dsBm.bmWidth;
            if (desiredy == 0) desiredy = ds.dsBm.bmHeight;

            /* Allocate memory for a BITMAPINFOHEADER structure and a
               color table. The maximum number of colors in a color table
               is 256 which corresponds to a bitmap with depth 8.
               Bitmaps with higher depths don't have color tables. */
            bi = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(BITMAPINFOHEADER) + 256 * sizeof(RGBQUAD));
            if (!bi) return 0;

            bi->bmiHeader.biSize        = sizeof(bi->bmiHeader);
            bi->bmiHeader.biPlanes      = ds.dsBm.bmPlanes;
            bi->bmiHeader.biBitCount    = ds.dsBm.bmBitsPixel;
            bi->bmiHeader.biCompression = BI_RGB;

            if (flags & LR_CREATEDIBSECTION)
            {
                /* Create a DIB section. LR_MONOCHROME is ignored */
                void * bits;
                HDC dc = CreateCompatibleDC(NULL);

                if (objSize == sizeof(DIBSECTION))
                {
                    /* The source bitmap is a DIB.
                       Get its attributes to create an exact copy */
                    memcpy(bi, &ds.dsBmih, sizeof(BITMAPINFOHEADER));
                }

                bi->bmiHeader.biWidth  = desiredx;
                bi->bmiHeader.biHeight = desiredy;

                /* Get the color table or the color masks */
                GetDIBits(dc, hnd, 0, ds.dsBm.bmHeight, NULL, bi, DIB_RGB_COLORS);

                res = CreateDIBSection(dc, bi, DIB_RGB_COLORS, &bits, NULL, 0);
                DeleteDC(dc);
            }
            else
            {
                /* Create a device-dependent bitmap */

                BOOL monochrome = (flags & LR_MONOCHROME);

                if (objSize == sizeof(DIBSECTION))
                {
                    /* The source bitmap is a DIB section.
                       Get its attributes */
                    HDC dc = CreateCompatibleDC(NULL);
                    bi->bmiHeader.biWidth  = ds.dsBm.bmWidth;
                    bi->bmiHeader.biHeight = ds.dsBm.bmHeight;
                    GetDIBits(dc, hnd, 0, ds.dsBm.bmHeight, NULL, bi, DIB_RGB_COLORS);
                    DeleteDC(dc);

                    if (!monochrome && ds.dsBm.bmBitsPixel == 1)
                    {
                        /* Look if the colors of the DIB are black and white */

                        monochrome = 
                              (bi->bmiColors[0].rgbRed == 0xff
                            && bi->bmiColors[0].rgbGreen == 0xff
                            && bi->bmiColors[0].rgbBlue == 0xff
                            && bi->bmiColors[0].rgbReserved == 0
                            && bi->bmiColors[1].rgbRed == 0
                            && bi->bmiColors[1].rgbGreen == 0
                            && bi->bmiColors[1].rgbBlue == 0
                            && bi->bmiColors[1].rgbReserved == 0)
                            ||
                              (bi->bmiColors[0].rgbRed == 0
                            && bi->bmiColors[0].rgbGreen == 0
                            && bi->bmiColors[0].rgbBlue == 0
                            && bi->bmiColors[0].rgbReserved == 0
                            && bi->bmiColors[1].rgbRed == 0xff
                            && bi->bmiColors[1].rgbGreen == 0xff
                            && bi->bmiColors[1].rgbBlue == 0xff
                            && bi->bmiColors[1].rgbReserved == 0);
                    }
                }
                else if (!monochrome)
                {
                    monochrome = ds.dsBm.bmBitsPixel == 1;
                }

                if (monochrome)
                    res = CreateBitmap(desiredx, desiredy, 1, 1, NULL);
                else
                    res = create_color_bitmap(desiredx, desiredy);
            }

            if (res)
            {
                /* Only copy the bitmap if it's a DIB section or if it's
                   compatible to the screen */
                if (objSize == sizeof(DIBSECTION) ||
                    ds.dsBm.bmBitsPixel == 1 ||
                    ds.dsBm.bmBitsPixel == get_display_bpp())
                {
                    /* The source bitmap may already be selected in a device context,
                       use GetDIBits/StretchDIBits and not StretchBlt  */

                    HDC dc;
                    void * bits;

                    dc = CreateCompatibleDC(NULL);
                    if (ds.dsBm.bmBitsPixel > 1)
                        SetStretchBltMode(dc, HALFTONE);

                    bi->bmiHeader.biWidth = ds.dsBm.bmWidth;
                    bi->bmiHeader.biHeight = ds.dsBm.bmHeight;
                    bi->bmiHeader.biSizeImage = 0;
                    bi->bmiHeader.biClrUsed = 0;
                    bi->bmiHeader.biClrImportant = 0;

                    /* Fill in biSizeImage */
                    GetDIBits(dc, hnd, 0, ds.dsBm.bmHeight, NULL, bi, DIB_RGB_COLORS);
                    bits = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, bi->bmiHeader.biSizeImage);

                    if (bits)
                    {
                        HBITMAP oldBmp;

                        /* Get the image bits of the source bitmap */
                        GetDIBits(dc, hnd, 0, ds.dsBm.bmHeight, bits, bi, DIB_RGB_COLORS);

                        /* Copy it to the destination bitmap */
                        oldBmp = SelectObject(dc, res);
                        StretchDIBits(dc, 0, 0, desiredx, desiredy,
                                      0, 0, ds.dsBm.bmWidth, ds.dsBm.bmHeight,
                                      bits, bi, DIB_RGB_COLORS, SRCCOPY);
                        SelectObject(dc, oldBmp);

                        HeapFree(GetProcessHeap(), 0, bits);
                    }

                    DeleteDC(dc);
                }

                if (flags & LR_COPYDELETEORG)
                {
                    DeleteObject(hnd);
                }
            }
            HeapFree(GetProcessHeap(), 0, bi);
            return res;
        }
        case IMAGE_ICON:
        case IMAGE_CURSOR:
        {
            struct cursoricon_frame *frame;
            struct cursoricon_object *icon;
            int depth = (flags & LR_MONOCHROME) ? 1 : get_display_bpp();
            HICON resource_icon = NULL;
            ICONINFO info;
            HICON res;

            if (!(icon = get_icon_ptr( hnd ))) return 0;

            if (icon->rsrc && (flags & LR_COPYFROMRESOURCE))
            {
                resource_icon = CURSORICON_Load( icon->module, icon->resname, desiredx,
                                                 desiredy, depth, !icon->is_icon, flags );
                release_user_handle_ptr( icon );
                if (!(icon = get_icon_ptr( resource_icon )))
                {
                    if (resource_icon) DestroyIcon( resource_icon );
                    return 0;
                }
            }
            frame = get_icon_frame( icon, 0 );

            if (flags & LR_DEFAULTSIZE)
            {
                if (!desiredx) desiredx = GetSystemMetrics( type == IMAGE_ICON ? SM_CXICON : SM_CXCURSOR );
                if (!desiredy) desiredy = GetSystemMetrics( type == IMAGE_ICON ? SM_CYICON : SM_CYCURSOR );
            }
            else
            {
                if (!desiredx) desiredx = frame->width;
                if (!desiredy) desiredy = frame->height;
            }

            info.fIcon = icon->is_icon;
            info.xHotspot = icon->hotspot.x;
            info.yHotspot = icon->hotspot.y;

            if (desiredx == frame->width && desiredy == frame->height)
            {
                info.hbmColor = frame->color;
                info.hbmMask = frame->mask;
                res = CreateIconIndirect( &info );
            }
            else
            {
                if (frame->color)
                {
                    if (!(info.hbmColor = create_color_bitmap( desiredx, desiredy )))
                    {
                        release_icon_frame( icon, frame );
                        release_user_handle_ptr( icon );
                        if (resource_icon) DestroyIcon( resource_icon );
                        return 0;
                    }
                    stretch_bitmap( info.hbmColor, frame->color, desiredx, desiredy,
                                    frame->width, frame->height );

                    if (!(info.hbmMask = CreateBitmap( desiredx, desiredy, 1, 1, NULL )))
                    {
                        DeleteObject( info.hbmColor );
                        release_icon_frame( icon, frame );
                        release_user_handle_ptr( icon );
                        if (resource_icon) DestroyIcon( resource_icon );
                        return 0;
                    }
                    stretch_bitmap( info.hbmMask, frame->mask, desiredx, desiredy,
                                    frame->width, frame->height );
                }
                else
                {
                    info.hbmColor = NULL;

                    if (!(info.hbmMask = CreateBitmap( desiredx, desiredy * 2, 1, 1, NULL )))
                    {
                        release_user_handle_ptr( icon );
                        if (resource_icon) DestroyIcon( resource_icon );
                        return 0;
                    }
                    stretch_bitmap( info.hbmMask, frame->mask, desiredx, desiredy * 2,
                                    frame->width, frame->height * 2 );
                }

                res = CreateIconIndirect( &info );

                DeleteObject( info.hbmColor );
                DeleteObject( info.hbmMask );
            }

            release_icon_frame( icon, frame );
            release_user_handle_ptr( icon );

            if (res && (flags & LR_COPYDELETEORG)) DestroyIcon( hnd );
            if (resource_icon) DestroyIcon( resource_icon );
            return res;
        }
    }
    return 0;
}


/******************************************************************************
 *		LoadBitmapW (USER32.@) Loads bitmap from the executable file
 *
 * RETURNS
 *    Success: Handle to specified bitmap
 *    Failure: NULL
 */
HBITMAP WINAPI LoadBitmapW(
    HINSTANCE instance, /* [in] Handle to application instance */
    LPCWSTR name)         /* [in] Address of bitmap resource name */
{
    return LoadImageW( instance, name, IMAGE_BITMAP, 0, 0, 0 );
}

/**********************************************************************
 *		LoadBitmapA (USER32.@)
 *
 * See LoadBitmapW.
 */
HBITMAP WINAPI LoadBitmapA( HINSTANCE instance, LPCSTR name )
{
    return LoadImageA( instance, name, IMAGE_BITMAP, 0, 0, 0 );
}
