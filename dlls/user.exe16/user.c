/*
 * Misc 16-bit USER functions
 *
 * Copyright 1993, 1996 Alexandre Julliard
 * Copyright 2002 Patrik Stridvall
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

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define OEMRESOURCE

#include "wine/winuser16.h"
#include "windef.h"
#include "winbase.h"
#include "wownt32.h"
#include "user_private.h"
#include "ntuser.h"
#include "wine/list.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(user);

/* handle to handle 16 conversions */
#define HANDLE_16(h32)		(LOWORD(h32))
#define HGDIOBJ_16(h32)		(LOWORD(h32))

/* handle16 to handle conversions */
#define HANDLE_32(h16)		((HANDLE)(ULONG_PTR)(h16))
#define HGDIOBJ_32(h16)		((HGDIOBJ)(ULONG_PTR)(h16))

#define IS_MENU_STRING_ITEM(flags) \
    (((flags) & (MF_STRING | MF_BITMAP | MF_OWNERDRAW | MF_SEPARATOR)) == MF_STRING)

/* UserSeeUserDo parameters */
#define USUD_LOCALALLOC        0x0001
#define USUD_LOCALFREE         0x0002
#define USUD_LOCALCOMPACT      0x0003
#define USUD_LOCALHEAP         0x0004
#define USUD_FIRSTCLASS        0x0005

#define CID_RESOURCE  0x0001
#define CID_WIN32     0x0004
#define CID_NONSHARED 0x0008

WORD USER_HeapSel = 0;  /* USER heap selector */

static HINSTANCE16 gdi_inst;

struct gray_string_info
{
    GRAYSTRINGPROC16 proc;
    LPARAM           param;
    char             str[1];
};

/* callback for 16-bit gray string proc with opaque pointer */
static BOOL CALLBACK gray_string_callback( HDC hdc, LPARAM param, INT len )
{
    const struct gray_string_info *info = (struct gray_string_info *)param;
    WORD args[4];
    DWORD ret;

    args[3] = HDC_16(hdc);
    args[2] = HIWORD(info->param);
    args[1] = LOWORD(info->param);
    args[0] = len;
    WOWCallback16Ex( (DWORD)info->proc, WCB16_PASCAL, sizeof(args), args, &ret );
    return LOWORD(ret);
}

/* callback for 16-bit gray string proc with string pointer */
static BOOL CALLBACK gray_string_callback_ptr( HDC hdc, LPARAM param, INT len )
{
    const struct gray_string_info *info = CONTAINING_RECORD( (void *)param, struct gray_string_info, str );
    return gray_string_callback( hdc, (LPARAM)info, len );
}

struct draw_state_info
{
    DRAWSTATEPROC16 proc;
    LPARAM          param;
};

/* callback for 16-bit DrawState functions */
static BOOL CALLBACK draw_state_callback( HDC hdc, LPARAM lparam, WPARAM wparam, int cx, int cy )
{
    const struct draw_state_info *info = (struct draw_state_info *)lparam;
    WORD args[6];
    DWORD ret;

    args[5] = HDC_16(hdc);
    args[4] = HIWORD(info->param);
    args[3] = LOWORD(info->param);
    args[2] = wparam;
    args[1] = cx;
    args[0] = cy;
    WOWCallback16Ex( (DWORD)info->proc, WCB16_PASCAL, sizeof(args), args, &ret );
    return LOWORD(ret);
}

/* This function is a copy of the one in objects/font.c */
static void logfont_32_to_16( const LOGFONTA* font32, LPLOGFONT16 font16 )
{
    font16->lfHeight = font32->lfHeight;
    font16->lfWidth = font32->lfWidth;
    font16->lfEscapement = font32->lfEscapement;
    font16->lfOrientation = font32->lfOrientation;
    font16->lfWeight = font32->lfWeight;
    font16->lfItalic = font32->lfItalic;
    font16->lfUnderline = font32->lfUnderline;
    font16->lfStrikeOut = font32->lfStrikeOut;
    font16->lfCharSet = font32->lfCharSet;
    font16->lfOutPrecision = font32->lfOutPrecision;
    font16->lfClipPrecision = font32->lfClipPrecision;
    font16->lfQuality = font32->lfQuality;
    font16->lfPitchAndFamily = font32->lfPitchAndFamily;
    lstrcpynA( font16->lfFaceName, font32->lfFaceName, LF_FACESIZE );
}

static int get_bitmap_width_bytes( int width, int bpp )
{
    switch(bpp)
    {
    case 1:
        return 2 * ((width+15) / 16);
    case 4:
        return 2 * ((width+3) / 4);
    case 24:
        width *= 3;
        /* fall through */
    case 8:
        return width + (width & 1);
    case 16:
    case 15:
        return width * 2;
    case 32:
        return width * 4;
    default:
        WARN("Unknown depth %d, please report.\n", bpp );
    }
    return -1;
}

/***********************************************************************
 * Helper for wsprintf16
 */

#define WPRINTF_LEFTALIGN   0x0001  /* Align output on the left ('-' prefix) */
#define WPRINTF_PREFIX_HEX  0x0002  /* Prefix hex with 0x ('#' prefix) */
#define WPRINTF_ZEROPAD     0x0004  /* Pad with zeros ('0' prefix) */
#define WPRINTF_LONG        0x0008  /* Long arg ('l' prefix) */
#define WPRINTF_SHORT       0x0010  /* Short arg ('h' prefix) */
#define WPRINTF_UPPER_HEX   0x0020  /* Upper-case hex ('X' specifier) */

typedef enum
{
    WPR_UNKNOWN,
    WPR_CHAR,
    WPR_STRING,
    WPR_SIGNED,
    WPR_UNSIGNED,
    WPR_HEXA
} WPRINTF_TYPE;

typedef struct
{
    UINT         flags;
    UINT         width;
    UINT         precision;
    WPRINTF_TYPE type;
} WPRINTF_FORMAT;

static INT parse_format( LPCSTR format, WPRINTF_FORMAT *res )
{
    LPCSTR p = format;

    res->flags = 0;
    res->width = 0;
    res->precision = 0;
    if (*p == '-') { res->flags |= WPRINTF_LEFTALIGN; p++; }
    if (*p == '#') { res->flags |= WPRINTF_PREFIX_HEX; p++; }
    if (*p == '0') { res->flags |= WPRINTF_ZEROPAD; p++; }
    while ((*p >= '0') && (*p <= '9'))  /* width field */
    {
        res->width = res->width * 10 + *p - '0';
        p++;
    }
    if (*p == '.')  /* precision field */
    {
        p++;
        while ((*p >= '0') && (*p <= '9'))
        {
            res->precision = res->precision * 10 + *p - '0';
            p++;
        }
    }
    if (*p == 'l') { res->flags |= WPRINTF_LONG; p++; }
    else if (*p == 'h') { res->flags |= WPRINTF_SHORT; p++; }
    switch(*p)
    {
    case 'c':
    case 'C':  /* no Unicode in Win16 */
        res->type = WPR_CHAR;
        break;
    case 's':
    case 'S':
        res->type = WPR_STRING;
        break;
    case 'd':
    case 'i':
        res->type = WPR_SIGNED;
        break;
    case 'u':
        res->type = WPR_UNSIGNED;
        break;
    case 'p':
        res->width = 8;
        res->flags |= WPRINTF_ZEROPAD;
        /* fall through */
    case 'X':
        res->flags |= WPRINTF_UPPER_HEX;
        /* fall through */
    case 'x':
        res->type = WPR_HEXA;
        break;
    default: /* unknown format char */
        res->type = WPR_UNKNOWN;
        p--;  /* print format as normal char */
        break;
    }
    return (INT)(p - format) + 1;
}


/**********************************************************************
 * Management of the 16-bit cursors and icons
 */

struct cache_entry
{
    struct list entry;
    HINSTANCE16 inst;
    HRSRC16     rsrc;
    HRSRC16     group;
    HICON16     icon;
    INT         count;
};

static struct list icon_cache = LIST_INIT( icon_cache );

static const WORD ICON_HOTSPOT = 0x4242;

static HICON16 alloc_icon_handle( unsigned int size )
{
    HGLOBAL16 handle = GlobalAlloc16( GMEM_MOVEABLE, size + sizeof(ULONG_PTR) );
    char *ptr = GlobalLock16( handle );
    memset( ptr + size, 0, sizeof(ULONG_PTR) );
    GlobalUnlock16( handle );
    FarSetOwner16( handle, 0 );
    return handle;
}

static CURSORICONINFO *get_icon_ptr( HICON16 handle )
{
    return GlobalLock16( handle );
}

static void release_icon_ptr( HICON16 handle, CURSORICONINFO *ptr )
{
    GlobalUnlock16( handle );
}

static HICON store_icon_32( HICON16 icon16, HICON icon )
{
    HICON ret = 0;
    CURSORICONINFO *ptr = get_icon_ptr( icon16 );

    if (ptr)
    {
        unsigned int and_size = ptr->nHeight * get_bitmap_width_bytes( ptr->nWidth, 1 );
        unsigned int xor_size = ptr->nHeight * get_bitmap_width_bytes( ptr->nWidth, ptr->bBitsPerPixel );
        if (GlobalSize16( icon16 ) >= sizeof(*ptr) + sizeof(ULONG_PTR) + and_size + xor_size )
        {
            memcpy( &ret, (char *)(ptr + 1) + and_size + xor_size, sizeof(ret) );
            memcpy( (char *)(ptr + 1) + and_size + xor_size, &icon, sizeof(icon) );
            NtUserSetIconParam( icon, icon16 );
        }
        release_icon_ptr( icon16, ptr );
    }
    return ret;
}

static int free_icon_handle( HICON16 handle )
{
    HICON icon32;

    if ((icon32 = store_icon_32( handle, 0 ))) DestroyIcon( icon32 );
    return GlobalFree16( handle );
}

/* retrieve the 32-bit counterpart of a 16-bit icon, creating it if needed */
HICON get_icon_32( HICON16 icon16 )
{
    HICON ret = 0;
    CURSORICONINFO *ptr = get_icon_ptr( icon16 );

    if (ptr)
    {
        unsigned int and_size = ptr->nHeight * get_bitmap_width_bytes( ptr->nWidth, 1 );
        unsigned int xor_size = ptr->nHeight * get_bitmap_width_bytes( ptr->nWidth, ptr->bBitsPerPixel );
        if (GlobalSize16( icon16 ) >= sizeof(*ptr) + sizeof(ULONG_PTR) + xor_size + and_size )
        {
            memcpy( &ret, (char *)(ptr + 1) + xor_size + and_size, sizeof(ret) );
            if (!ret)
            {
                ICONINFO iinfo;

                iinfo.fIcon = (ptr->ptHotSpot.x == ICON_HOTSPOT) && (ptr->ptHotSpot.y == ICON_HOTSPOT);
                iinfo.xHotspot = ptr->ptHotSpot.x;
                iinfo.yHotspot = ptr->ptHotSpot.y;
                iinfo.hbmMask  = CreateBitmap( ptr->nWidth, ptr->nHeight, 1, 1, ptr + 1 );
                iinfo.hbmColor = CreateBitmap( ptr->nWidth, ptr->nHeight, ptr->bPlanes, ptr->bBitsPerPixel,
                                               (char *)(ptr + 1) + and_size );
                ret = CreateIconIndirect( &iinfo );
                DeleteObject( iinfo.hbmMask );
                DeleteObject( iinfo.hbmColor );
                memcpy( (char *)(ptr + 1) + xor_size + and_size, &ret, sizeof(ret) );
                NtUserSetIconParam( ret, icon16 );
            }
        }
        release_icon_ptr( icon16, ptr );
    }
    return ret;
}

/* retrieve the 16-bit counterpart of a 32-bit icon, creating it if needed */
HICON16 get_icon_16( HICON icon )
{
    HICON16 ret = NtUserGetIconParam( icon );

    if (!ret)
    {
        ICONINFO info;
        BITMAP bm;
        UINT and_size, xor_size;
        void *xor_bits = NULL, *and_bits;
        CURSORICONINFO cinfo;

        if (!(GetIconInfo( icon, &info ))) return 0;
        GetObjectW( info.hbmMask, sizeof(bm), &bm );
        and_size = bm.bmHeight * bm.bmWidthBytes;
        if (!(and_bits = HeapAlloc( GetProcessHeap(), 0, and_size ))) goto done;
        GetBitmapBits( info.hbmMask, and_size, and_bits );
        if (info.hbmColor)
        {
            GetObjectW( info.hbmColor, sizeof(bm), &bm );
            xor_size = bm.bmHeight * bm.bmWidthBytes;
            if (!(xor_bits = HeapAlloc( GetProcessHeap(), 0, xor_size ))) goto done;
            GetBitmapBits( info.hbmColor, xor_size, xor_bits );
        }
        else
        {
            bm.bmHeight /= 2;
            xor_bits = (char *)and_bits + and_size / 2;
        }
        if (!info.fIcon)
        {
            cinfo.ptHotSpot.x = info.xHotspot;
            cinfo.ptHotSpot.y = info.yHotspot;
        }
        else cinfo.ptHotSpot.x = cinfo.ptHotSpot.y = ICON_HOTSPOT;

        cinfo.nWidth        = bm.bmWidth;
        cinfo.nHeight       = bm.bmHeight;
        cinfo.nWidthBytes   = bm.bmWidthBytes;
        cinfo.bPlanes       = bm.bmPlanes;
        cinfo.bBitsPerPixel = bm.bmBitsPixel;

        if ((ret = CreateCursorIconIndirect16( 0, &cinfo, and_bits, xor_bits )))
            store_icon_32( ret, icon );

    done:
        if (info.hbmColor)
        {
            HeapFree( GetProcessHeap(), 0, xor_bits );
            DeleteObject( info.hbmColor );
        }
        HeapFree( GetProcessHeap(), 0, and_bits );
        DeleteObject( info.hbmMask );
    }
    return ret;
}

static void add_shared_icon( HINSTANCE16 inst, HRSRC16 rsrc, HRSRC16 group, HICON16 icon )
{
    struct cache_entry *cache = HeapAlloc( GetProcessHeap(), 0, sizeof(*cache) );

    if (!cache) return;
    cache->inst  = inst;
    cache->rsrc  = rsrc;
    cache->group = group;
    cache->icon  = icon;
    cache->count = 1;
    list_add_tail( &icon_cache, &cache->entry );
}

static HICON16 find_shared_icon( HINSTANCE16 inst, HRSRC16 rsrc )
{
    struct cache_entry *cache;

    LIST_FOR_EACH_ENTRY( cache, &icon_cache, struct cache_entry, entry )
    {
        if (cache->inst != inst || cache->rsrc != rsrc) continue;
        cache->count++;
        return cache->icon;
    }
    return 0;
}

static int release_shared_icon( HICON16 icon )
{
    struct cache_entry *cache;

    LIST_FOR_EACH_ENTRY( cache, &icon_cache, struct cache_entry, entry )
    {
        if (cache->icon != icon) continue;
        if (!cache->count) return 0;
        return --cache->count;
    }
    return -1;
}

static void free_module_icons( HINSTANCE16 inst )
{
    struct cache_entry *cache, *next;

    LIST_FOR_EACH_ENTRY_SAFE( cache, next, &icon_cache, struct cache_entry, entry )
    {
        if (cache->inst != inst) continue;
        list_remove( &cache->entry );
        free_icon_handle( cache->icon );
        HeapFree( GetProcessHeap(), 0, cache );
    }
}

/**********************************************************************
 * Management of the 16-bit clipboard formats
 */

struct clipboard_format
{
    struct list entry;
    UINT        format;
    HANDLE16    data;
};

static struct list clipboard_formats = LIST_INIT( clipboard_formats );

static void set_clipboard_format( UINT format, HANDLE16 data )
{
    struct clipboard_format *fmt;

    /* replace it if it exists already */
    LIST_FOR_EACH_ENTRY( fmt, &clipboard_formats, struct clipboard_format, entry )
    {
        if (fmt->format != format) continue;
        GlobalFree16( fmt->data );
        fmt->data = data;
        return;
    }

    if ((fmt = HeapAlloc( GetProcessHeap(), 0, sizeof(*fmt) )))
    {
        fmt->format = format;
        fmt->data = data;
        list_add_tail( &clipboard_formats, &fmt->entry );
    }
}

static void free_clipboard_formats(void)
{
    struct list *head;

    while ((head = list_head( &clipboard_formats )))
    {
        struct clipboard_format *fmt = LIST_ENTRY( head, struct clipboard_format, entry );
        list_remove( &fmt->entry );
        GlobalFree16( fmt->data );
        HeapFree( GetProcessHeap(), 0, fmt );
    }
}


/***********************************************************************
 *		OldExitWindows (USER.2)
 */
void WINAPI OldExitWindows16(void)
{
    ExitWindows16(0, 0);
}


/**********************************************************************
 *		InitApp (USER.5)
 */
INT16 WINAPI InitApp16( HINSTANCE16 hInstance )
{
    /* Create task message queue */
    return (InitThreadInput16( 0, 0 ) != 0);
}


/***********************************************************************
 *		ExitWindows (USER.7)
 */
BOOL16 WINAPI ExitWindows16( DWORD dwReturnCode, UINT16 wReserved )
{
    return ExitWindowsEx( EWX_LOGOFF, 0xffffffff );
}


/***********************************************************************
 *		GetTimerResolution (USER.14)
 */
LONG WINAPI GetTimerResolution16(void)
{
    return (1000);
}


/***********************************************************************
 *		ClipCursor (USER.16)
 */
BOOL16 WINAPI ClipCursor16( const RECT16 *rect )
{
    RECT rect32;

    if (!rect) return ClipCursor( NULL );
    rect32.left   = rect->left;
    rect32.top    = rect->top;
    rect32.right  = rect->right;
    rect32.bottom = rect->bottom;
    return ClipCursor( &rect32 );
}


/***********************************************************************
 *		GetCursorPos (USER.17)
 */
BOOL16 WINAPI GetCursorPos16( POINT16 *pt )
{
    POINT pos;
    if (!pt) return FALSE;
    GetCursorPos(&pos);
    pt->x = pos.x;
    pt->y = pos.y;
    return TRUE;
}


/*******************************************************************
 *		AnyPopup (USER.52)
 */
BOOL16 WINAPI AnyPopup16(void)
{
    return AnyPopup();
}


/***********************************************************************
 *		SetCursor (USER.69)
 */
HCURSOR16 WINAPI SetCursor16(HCURSOR16 hCursor)
{
  return get_icon_16( SetCursor( get_icon_32(hCursor) ));
}


/***********************************************************************
 *		SetCursorPos (USER.70)
 */
void WINAPI SetCursorPos16( INT16 x, INT16 y )
{
    SetCursorPos( x, y );
}


/***********************************************************************
 *		ShowCursor (USER.71)
 */
INT16 WINAPI ShowCursor16(BOOL16 bShow)
{
  return ShowCursor(bShow);
}


/***********************************************************************
 *		SetRect (USER.72)
 */
void WINAPI SetRect16( LPRECT16 rect, INT16 left, INT16 top, INT16 right, INT16 bottom )
{
    rect->left   = left;
    rect->right  = right;
    rect->top    = top;
    rect->bottom = bottom;
}


/***********************************************************************
 *		SetRectEmpty (USER.73)
 */
void WINAPI SetRectEmpty16( LPRECT16 rect )
{
    rect->left = rect->right = rect->top = rect->bottom = 0;
}


/***********************************************************************
 *		CopyRect (USER.74)
 */
BOOL16 WINAPI CopyRect16( RECT16 *dest, const RECT16 *src )
{
    *dest = *src;
    return TRUE;
}


/***********************************************************************
 *		IsRectEmpty (USER.75)
 *
 * Bug compat: Windows checks for 0 or negative width/height.
 */
BOOL16 WINAPI IsRectEmpty16( const RECT16 *rect )
{
    return ((rect->left >= rect->right) || (rect->top >= rect->bottom));
}


/***********************************************************************
 *		PtInRect (USER.76)
 */
BOOL16 WINAPI PtInRect16( const RECT16 *rect, POINT16 pt )
{
    return ((pt.x >= rect->left) && (pt.x < rect->right) &&
            (pt.y >= rect->top) && (pt.y < rect->bottom));
}


/***********************************************************************
 *		OffsetRect (USER.77)
 */
void WINAPI OffsetRect16( LPRECT16 rect, INT16 x, INT16 y )
{
    rect->left   += x;
    rect->right  += x;
    rect->top    += y;
    rect->bottom += y;
}


/***********************************************************************
 *		InflateRect (USER.78)
 */
void WINAPI InflateRect16( LPRECT16 rect, INT16 x, INT16 y )
{
    rect->left   -= x;
    rect->top    -= y;
    rect->right  += x;
    rect->bottom += y;
}


/***********************************************************************
 *		IntersectRect (USER.79)
 */
BOOL16 WINAPI IntersectRect16( LPRECT16 dest, const RECT16 *src1,
                               const RECT16 *src2 )
{
    if (IsRectEmpty16(src1) || IsRectEmpty16(src2) ||
        (src1->left >= src2->right) || (src2->left >= src1->right) ||
        (src1->top >= src2->bottom) || (src2->top >= src1->bottom))
    {
        SetRectEmpty16( dest );
        return FALSE;
    }
    dest->left   = max( src1->left, src2->left );
    dest->right  = min( src1->right, src2->right );
    dest->top    = max( src1->top, src2->top );
    dest->bottom = min( src1->bottom, src2->bottom );
    return TRUE;
}


/***********************************************************************
 *		UnionRect (USER.80)
 */
BOOL16 WINAPI UnionRect16( LPRECT16 dest, const RECT16 *src1,
                           const RECT16 *src2 )
{
    if (IsRectEmpty16(src1))
    {
        if (IsRectEmpty16(src2))
        {
            SetRectEmpty16( dest );
            return FALSE;
        }
        else *dest = *src2;
    }
    else
    {
        if (IsRectEmpty16(src2)) *dest = *src1;
        else
        {
            dest->left   = min( src1->left, src2->left );
            dest->right  = max( src1->right, src2->right );
            dest->top    = min( src1->top, src2->top );
            dest->bottom = max( src1->bottom, src2->bottom );
        }
    }
    return TRUE;
}


/***********************************************************************
 *		FillRect (USER.81)
 * NOTE
 *   The Win16 variant doesn't support special color brushes like
 *   the Win32 one, despite the fact that Win16, as well as Win32,
 *   supports special background brushes for a window class.
 */
INT16 WINAPI FillRect16( HDC16 hdc, const RECT16 *rect, HBRUSH16 hbrush )
{
    HBRUSH prevBrush;

    /* coordinates are logical so we cannot fast-check 'rect',
     * it will be done later in the PatBlt().
     */

    if (!(prevBrush = SelectObject( HDC_32(hdc), HBRUSH_32(hbrush) ))) return 0;
    PatBlt( HDC_32(hdc), rect->left, rect->top,
              rect->right - rect->left, rect->bottom - rect->top, PATCOPY );
    SelectObject( HDC_32(hdc), prevBrush );
    return 1;
}


/***********************************************************************
 *		InvertRect (USER.82)
 */
void WINAPI InvertRect16( HDC16 hdc, const RECT16 *rect )
{
    PatBlt( HDC_32(hdc), rect->left, rect->top,
              rect->right - rect->left, rect->bottom - rect->top, DSTINVERT );
}


/***********************************************************************
 *		FrameRect (USER.83)
 */
INT16 WINAPI FrameRect16( HDC16 hdc, const RECT16 *rect16, HBRUSH16 hbrush )
{
    RECT rect;

    rect.left   = rect16->left;
    rect.top    = rect16->top;
    rect.right  = rect16->right;
    rect.bottom = rect16->bottom;
    return FrameRect( HDC_32(hdc), &rect, HBRUSH_32(hbrush) );
}


/***********************************************************************
 *		DrawIcon (USER.84)
 */
BOOL16 WINAPI DrawIcon16(HDC16 hdc, INT16 x, INT16 y, HICON16 hIcon)
{
  return DrawIcon(HDC_32(hdc), x, y, get_icon_32(hIcon) );
}


/***********************************************************************
 *           DrawText    (USER.85)
 */
INT16 WINAPI DrawText16( HDC16 hdc, LPCSTR str, INT16 count, LPRECT16 rect, UINT16 flags )
{
    INT16 ret;

    if (rect)
    {
        RECT rect32;

        rect32.left   = rect->left;
        rect32.top    = rect->top;
        rect32.right  = rect->right;
        rect32.bottom = rect->bottom;
        ret = DrawTextA( HDC_32(hdc), str, count, &rect32, flags );
        rect->left   = rect32.left;
        rect->top    = rect32.top;
        rect->right  = rect32.right;
        rect->bottom = rect32.bottom;
    }
    else ret = DrawTextA( HDC_32(hdc), str, count, NULL, flags);
    return ret;
}


/***********************************************************************
 *		IconSize (USER.86)
 *
 * See "Undocumented Windows". Used by W2.0 paint.exe.
 */
DWORD WINAPI IconSize16(void)
{
  return MAKELONG(GetSystemMetrics(SM_CYICON), GetSystemMetrics(SM_CXICON));
}


/***********************************************************************
 *		AdjustWindowRect (USER.102)
 */
BOOL16 WINAPI AdjustWindowRect16( LPRECT16 rect, DWORD style, BOOL16 menu )
{
    return AdjustWindowRectEx16( rect, style, menu, 0 );
}


/***********************************************************************
 *		MessageBeep (USER.104)
 */
void WINAPI MessageBeep16( UINT16 i )
{
    MessageBeep( i );
}


/**************************************************************************
 *		CloseClipboard (USER.138)
 */
BOOL16 WINAPI CloseClipboard16(void)
{
    BOOL ret = CloseClipboard();
    if (ret) free_clipboard_formats();
    return ret;
}


/**************************************************************************
 *		EmptyClipboard (USER.139)
 */
BOOL16 WINAPI EmptyClipboard16(void)
{
    BOOL ret = EmptyClipboard();
    if (ret) free_clipboard_formats();
    return ret;
}


/**************************************************************************
 *		SetClipboardData (USER.141)
 */
HANDLE16 WINAPI SetClipboardData16( UINT16 format, HANDLE16 data16 )
{
    HANDLE data32 = 0;

    switch (format)
    {
    case CF_BITMAP:
    case CF_PALETTE:
        data32 = HGDIOBJ_32( data16 );
        break;

    case CF_METAFILEPICT:
    {
        METAHEADER *header;
        METAFILEPICT *pict32;
        METAFILEPICT16 *pict16 = GlobalLock16( data16 );

        if (pict16)
        {
            if (!(data32 = GlobalAlloc( GMEM_MOVEABLE, sizeof(*pict32) ))) return 0;
            pict32 = GlobalLock( data32 );
            pict32->mm = pict16->mm;
            pict32->xExt = pict16->xExt;
            pict32->yExt = pict16->yExt;
            header = GlobalLock16( pict16->hMF );
            pict32->hMF = SetMetaFileBitsEx( header->mtSize * 2, (BYTE *)header );
            GlobalUnlock16( pict16->hMF );
            GlobalUnlock( data32 );
        }
        set_clipboard_format( format, data16 );
        break;
    }

    case CF_ENHMETAFILE:
        FIXME( "enhmetafile not supported in 16-bit\n" );
        return 0;

    default:
        if (format >= CF_GDIOBJFIRST && format <= CF_GDIOBJLAST)
            data32 = HGDIOBJ_32( data16 );
        else if (format >= CF_PRIVATEFIRST && format <= CF_PRIVATELAST)
            data32 = HANDLE_32( data16 );
        else
        {
            UINT size = GlobalSize16( data16 );
            void *ptr32, *ptr16 = GlobalLock16( data16 );
            if (ptr16)
            {
                if (!(data32 = GlobalAlloc( GMEM_MOVEABLE, size ))) return 0;
                ptr32 = GlobalLock( data32 );
                memcpy( ptr32, ptr16, size );
                GlobalUnlock( data32 );
            }
            set_clipboard_format( format, data16 );
        }
        break;
    }

    if (!SetClipboardData( format, data32 )) return 0;
    return data16;
}


/**************************************************************************
 *		GetClipboardData (USER.142)
 */
HANDLE16 WINAPI GetClipboardData16( UINT16 format )
{
    HANDLE data32 = GetClipboardData( format );
    HANDLE16 data16 = 0;
    UINT size;
    void *ptr;

    if (!data32) return 0;

    switch (format)
    {
    case CF_BITMAP:
    case CF_PALETTE:
        data16 = HGDIOBJ_16( data32 );
        break;

    case CF_METAFILEPICT:
    {
        METAFILEPICT16 *pict16;
        METAFILEPICT *pict32 = GlobalLock( data32 );

        if (pict32)
        {
            if (!(data16 = GlobalAlloc16( GMEM_MOVEABLE, sizeof(*pict16) ))) return 0;
            pict16 = GlobalLock16( data16 );
            pict16->mm   = pict32->mm;
            pict16->xExt = pict32->xExt;
            pict16->yExt = pict32->yExt;
            size = GetMetaFileBitsEx( pict32->hMF, 0, NULL );
            pict16->hMF = GlobalAlloc16( GMEM_MOVEABLE, size );
            ptr = GlobalLock16( pict16->hMF );
            GetMetaFileBitsEx( pict32->hMF, size, ptr );
            GlobalUnlock16( pict16->hMF );
            GlobalUnlock16( data16 );
            set_clipboard_format( format, data16 );
        }
        break;
    }

    case CF_ENHMETAFILE:
        FIXME( "enhmetafile not supported in 16-bit\n" );
        return 0;

    default:
        if (format >= CF_GDIOBJFIRST && format <= CF_GDIOBJLAST)
            data16 = HGDIOBJ_16( data32 );
        else if (format >= CF_PRIVATEFIRST && format <= CF_PRIVATELAST)
            data16 = HANDLE_16( data32 );
        else
        {
            void *ptr16, *ptr32 = GlobalLock( data32 );
            if (ptr32)
            {
                size = GlobalSize( data32 );
                if (!(data16 = GlobalAlloc16( GMEM_MOVEABLE, size ))) return 0;
                ptr16 = GlobalLock16( data16 );
                memcpy( ptr16, ptr32, size );
                GlobalUnlock16( data16 );
                set_clipboard_format( format, data16 );
            }
        }
        break;
    }
    return data16;
}


/**************************************************************************
 *		CountClipboardFormats (USER.143)
 */
INT16 WINAPI CountClipboardFormats16(void)
{
    return CountClipboardFormats();
}


/**************************************************************************
 *		EnumClipboardFormats (USER.144)
 */
UINT16 WINAPI EnumClipboardFormats16( UINT16 id )
{
    return EnumClipboardFormats( id );
}


/**************************************************************************
 *		RegisterClipboardFormat (USER.145)
 */
UINT16 WINAPI RegisterClipboardFormat16( LPCSTR name )
{
    return RegisterClipboardFormatA( name );
}


/**************************************************************************
 *		GetClipboardFormatName (USER.146)
 */
INT16 WINAPI GetClipboardFormatName16( UINT16 id, LPSTR buffer, INT16 maxlen )
{
    return GetClipboardFormatNameA( id, buffer, maxlen );
}


/**********************************************************************
 *	    LoadMenu    (USER.150)
 */
HMENU16 WINAPI LoadMenu16( HINSTANCE16 instance, LPCSTR name )
{
    HRSRC16 hRsrc;
    HGLOBAL16 handle;
    HMENU16 hMenu;

    if (HIWORD(name) && name[0] == '#') name = ULongToPtr(atoi( name + 1 ));
    if (!name) return 0;

    instance = GetExePtr( instance );
    if (!(hRsrc = FindResource16( instance, name, (LPSTR)RT_MENU ))) return 0;
    if (!(handle = LoadResource16( instance, hRsrc ))) return 0;
    hMenu = LoadMenuIndirect16(LockResource16(handle));
    FreeResource16( handle );
    return hMenu;
}


/**********************************************************************
 *         CreateMenu    (USER.151)
 */
HMENU16 WINAPI CreateMenu16(void)
{
    return HMENU_16( CreateMenu() );
}


/**********************************************************************
 *         DestroyMenu    (USER.152)
 */
BOOL16 WINAPI DestroyMenu16( HMENU16 hMenu )
{
    return DestroyMenu( HMENU_32(hMenu) );
}


/*******************************************************************
 *         ChangeMenu    (USER.153)
 */
BOOL16 WINAPI ChangeMenu16( HMENU16 hMenu, UINT16 pos, SEGPTR data,
                            UINT16 id, UINT16 flags )
{
    if (flags & MF_APPEND) return AppendMenu16( hMenu, flags & ~MF_APPEND, id, data );

    /* FIXME: Word passes the item id in 'pos' and 0 or 0xffff as id */
    /* for MF_DELETE. We should check the parameters for all others */
    /* MF_* actions also (anybody got a doc on ChangeMenu?). */

    if (flags & MF_DELETE) return DeleteMenu16(hMenu, pos, flags & ~MF_DELETE);
    if (flags & MF_CHANGE) return ModifyMenu16(hMenu, pos, flags & ~MF_CHANGE, id, data );
    if (flags & MF_REMOVE) return RemoveMenu16(hMenu, flags & MF_BYPOSITION ? pos : id,
                                               flags & ~MF_REMOVE );
    /* Default: MF_INSERT */
    return InsertMenu16( hMenu, pos, flags, id, data );
}


/*******************************************************************
 *         CheckMenuItem    (USER.154)
 */
BOOL16 WINAPI CheckMenuItem16( HMENU16 hMenu, UINT16 id, UINT16 flags )
{
    return CheckMenuItem( HMENU_32(hMenu), id, flags );
}


/**********************************************************************
 *         EnableMenuItem    (USER.155)
 */
BOOL16 WINAPI EnableMenuItem16( HMENU16 hMenu, UINT16 wItemID, UINT16 wFlags )
{
    return EnableMenuItem( HMENU_32(hMenu), wItemID, wFlags );
}


/**********************************************************************
 *         GetSubMenu    (USER.159)
 */
HMENU16 WINAPI GetSubMenu16( HMENU16 hMenu, INT16 nPos )
{
    return HMENU_16( GetSubMenu( HMENU_32(hMenu), nPos ) );
}


/*******************************************************************
 *         GetMenuString    (USER.161)
 */
INT16 WINAPI GetMenuString16( HMENU16 hMenu, UINT16 wItemID,
                              LPSTR str, INT16 nMaxSiz, UINT16 wFlags )
{
    return GetMenuStringA( HMENU_32(hMenu), wItemID, str, nMaxSiz, wFlags );
}


/**********************************************************************
 *		WinHelp (USER.171)
 */
BOOL16 WINAPI WinHelp16( HWND16 hWnd, LPCSTR lpHelpFile, UINT16 wCommand,
                         DWORD dwData )
{
    BOOL ret;
    DWORD mutex_count;

    /* We might call WinExec() */
    ReleaseThunkLock(&mutex_count);

    ret = WinHelpA(WIN_Handle32(hWnd), lpHelpFile, wCommand, (DWORD)MapSL(dwData));

    RestoreThunkLock(mutex_count);
    return ret;
}


/***********************************************************************
 *		LoadCursor (USER.173)
 */
HCURSOR16 WINAPI LoadCursor16(HINSTANCE16 hInstance, LPCSTR name)
{
    return LoadImage16( hInstance, name, IMAGE_CURSOR, 0, 0, LR_SHARED | LR_DEFAULTSIZE );
}


/***********************************************************************
 *		LoadIcon (USER.174)
 */
HICON16 WINAPI LoadIcon16(HINSTANCE16 hInstance, LPCSTR name)
{
    return LoadImage16( hInstance, name, IMAGE_ICON, 0, 0, LR_SHARED | LR_DEFAULTSIZE );
}

/**********************************************************************
 *		LoadBitmap (USER.175)
 */
HBITMAP16 WINAPI LoadBitmap16(HINSTANCE16 hInstance, LPCSTR name)
{
    return LoadImage16( hInstance, name, IMAGE_BITMAP, 0, 0, 0 );
}

/**********************************************************************
 *     LoadString   (USER.176)
 */
INT16 WINAPI LoadString16( HINSTANCE16 instance, UINT16 resource_id, LPSTR buffer, INT16 buflen )
{
    HGLOBAL16 hmem;
    HRSRC16 hrsrc;
    unsigned char *p;
    int string_num;
    int ret;

    TRACE("inst=%04x id=%04x buff=%p len=%d\n", instance, resource_id, buffer, buflen);

    hrsrc = FindResource16( instance, MAKEINTRESOURCEA((resource_id>>4)+1), (LPSTR)RT_STRING );
    if (!hrsrc) return 0;
    hmem = LoadResource16( instance, hrsrc );
    if (!hmem) return 0;

    p = LockResource16(hmem);
    string_num = resource_id & 0x000f;
    while (string_num--) p += *p + 1;

    if (buffer == NULL) ret = *p;
    else
    {
        ret = min(buflen - 1, *p);
        if (ret > 0)
        {
            memcpy(buffer, p + 1, ret);
            buffer[ret] = '\0';
        }
        else if (buflen > 1)
        {
	    buffer[0] = '\0';
	    ret = 0;
	}
        TRACE( "%s loaded\n", debugstr_a(buffer));
    }
    FreeResource16( hmem );
    return ret;
}

/**********************************************************************
 *              LoadAccelerators  (USER.177)
 */
HACCEL16 WINAPI LoadAccelerators16(HINSTANCE16 instance, LPCSTR lpTableName)
{
    HRSRC16 hRsrc;
    HGLOBAL16 hMem;
    ACCEL16 *table16;
    HACCEL ret = 0;

    TRACE("%04x %s\n", instance, debugstr_a(lpTableName) );

    if (!(hRsrc = FindResource16( instance, lpTableName, (LPSTR)RT_ACCELERATOR )) ||
        !(hMem = LoadResource16(instance,hRsrc)))
    {
        WARN("couldn't find %04x %s\n", instance, debugstr_a(lpTableName));
        return 0;
    }
    if ((table16 = LockResource16( hMem )))
    {
        DWORD i, count = SizeofResource16( instance, hRsrc ) / sizeof(*table16);
        ACCEL *table = HeapAlloc( GetProcessHeap(), 0, count * sizeof(*table) );
        if (table)
        {
            for (i = 0; i < count; i++)
            {
                table[i].fVirt = table16[i].fVirt & 0x7f;
                table[i].key   = table16[i].key;
                table[i].cmd   = table16[i].cmd;
            }
            ret = CreateAcceleratorTableA( table, count );
            HeapFree( GetProcessHeap(), 0, table );
        }
    }
    FreeResource16( hMem );
    return HACCEL_16(ret);
}

/***********************************************************************
 *		GetSystemMetrics (USER.179)
 */
INT16 WINAPI GetSystemMetrics16( INT16 index )
{
    return GetSystemMetrics( index );
}


/*************************************************************************
 *		GetSysColor (USER.180)
 */
COLORREF WINAPI GetSysColor16( INT16 index )
{
    return GetSysColor( index );
}


/*************************************************************************
 *		SetSysColors (USER.181)
 */
VOID WINAPI SetSysColors16( INT16 count, const INT16 *list16, const COLORREF *values )
{
    INT i, *list;

    if ((list = HeapAlloc( GetProcessHeap(), 0, count * sizeof(*list) )))
    {
        for (i = 0; i < count; i++) list[i] = list16[i];
        SetSysColors( count, list, values );
        HeapFree( GetProcessHeap(), 0, list );
    }
}


/***********************************************************************
 *           GrayString   (USER.185)
 */
BOOL16 WINAPI GrayString16( HDC16 hdc, HBRUSH16 hbr, GRAYSTRINGPROC16 gsprc,
                            LPARAM lParam, INT16 cch, INT16 x, INT16 y,
                            INT16 cx, INT16 cy )
{
    BOOL ret;

    if (!gsprc) return GrayStringA( HDC_32(hdc), HBRUSH_32(hbr), NULL,
                                    (LPARAM)MapSL(lParam), cch, x, y, cx, cy );

    if (cch == -1 || (cch && cx && cy))
    {
        /* lParam can be treated as an opaque pointer */
        struct gray_string_info info;

        info.proc  = gsprc;
        info.param = lParam;
        ret = GrayStringA( HDC_32(hdc), HBRUSH_32(hbr), gray_string_callback,
                           (LPARAM)&info, cch, x, y, cx, cy );
    }
    else  /* here we need some string conversions */
    {
        char *str16 = MapSL(lParam);
        struct gray_string_info *info;

        if (!cch) cch = strlen(str16);
        info = HeapAlloc( GetProcessHeap(), 0, FIELD_OFFSET( struct gray_string_info, str[cch] ));
        if (!info) return FALSE;
        info->proc  = gsprc;
        info->param = lParam;
        memcpy( info->str, str16, cch );
        ret = GrayStringA( HDC_32(hdc), HBRUSH_32(hbr), gray_string_callback_ptr,
                           (LPARAM)info->str, cch, x, y, cx, cy );
        HeapFree( GetProcessHeap(), 0, info );
    }
    return ret;
}


/***********************************************************************
 *		SwapMouseButton (USER.186)
 */
BOOL16 WINAPI SwapMouseButton16( BOOL16 fSwap )
{
    return SwapMouseButton( fSwap );
}


/**************************************************************************
 *		IsClipboardFormatAvailable (USER.193)
 */
BOOL16 WINAPI IsClipboardFormatAvailable16( UINT16 wFormat )
{
    return IsClipboardFormatAvailable( wFormat );
}


/***********************************************************************
 *           TabbedTextOut    (USER.196)
 */
LONG WINAPI TabbedTextOut16( HDC16 hdc, INT16 x, INT16 y, LPCSTR lpstr,
                             INT16 count, INT16 nb_tabs, const INT16 *tabs16, INT16 tab_org )
{
    LONG ret;
    INT i, *tabs = HeapAlloc( GetProcessHeap(), 0, nb_tabs * sizeof(*tabs) );
    if (!tabs) return 0;
    for (i = 0; i < nb_tabs; i++) tabs[i] = tabs16[i];
    ret = TabbedTextOutA( HDC_32(hdc), x, y, lpstr, count, nb_tabs, tabs, tab_org );
    HeapFree( GetProcessHeap(), 0, tabs );
    return ret;
}


/***********************************************************************
 *           GetTabbedTextExtent    (USER.197)
 */
DWORD WINAPI GetTabbedTextExtent16( HDC16 hdc, LPCSTR lpstr, INT16 count,
                                    INT16 nb_tabs, const INT16 *tabs16 )
{
    LONG ret;
    INT i, *tabs = HeapAlloc( GetProcessHeap(), 0, nb_tabs * sizeof(*tabs) );
    if (!tabs) return 0;
    for (i = 0; i < nb_tabs; i++) tabs[i] = tabs16[i];
    ret = GetTabbedTextExtentA( HDC_32(hdc), lpstr, count, nb_tabs, tabs );
    HeapFree( GetProcessHeap(), 0, tabs );
    return ret;
}


/***********************************************************************
 *		UserSeeUserDo (USER.216)
 */
DWORD WINAPI UserSeeUserDo16(WORD wReqType, WORD wParam1, WORD wParam2, WORD wParam3)
{
    HANDLE16 oldDS = CURRENT_DS;
    DWORD ret = (DWORD)-1;

    CURRENT_DS = USER_HeapSel;
    switch (wReqType)
    {
    case USUD_LOCALALLOC:
        ret = LocalAlloc16(wParam1, wParam3);
        break;
    case USUD_LOCALFREE:
        ret = LocalFree16(wParam1);
        break;
    case USUD_LOCALCOMPACT:
        ret = LocalCompact16(wParam3);
        break;
    case USUD_LOCALHEAP:
        ret = USER_HeapSel;
        break;
    case USUD_FIRSTCLASS:
        FIXME("return a pointer to the first window class.\n");
        break;
    default:
        WARN("wReqType %04x (unknown)\n", wReqType);
    }
    CURRENT_DS = oldDS;
    return ret;
}


/***********************************************************************
 *           LookupMenuHandle   (USER.217)
 */
HMENU16 WINAPI LookupMenuHandle16( HMENU16 hmenu, INT16 id )
{
    FIXME( "%04x %04x: stub\n", hmenu, id );
    return hmenu;
}


static LPCSTR parse_menu_resource( LPCSTR res, HMENU hMenu, BOOL oldFormat )
{
    WORD flags, id = 0;
    LPCSTR str;
    BOOL end_flag;

    do
    {
        /* Windows 3.00 and later use a WORD for the flags, whereas 1.x and 2.x use a BYTE. */
        if (oldFormat)
        {
            flags = GET_BYTE(res);
            res += sizeof(BYTE);
        }
        else
        {
            flags = GET_WORD(res);
            res += sizeof(WORD);
        }

        end_flag = flags & MF_END;
        /* Remove MF_END because it has the same value as MF_HILITE */
        flags &= ~MF_END;
        if (!(flags & MF_POPUP))
        {
            id = GET_WORD(res);
            res += sizeof(WORD);
        }
        str = res;
        res += strlen(str) + 1;
        if (flags & MF_POPUP)
        {
            HMENU hSubMenu = CreatePopupMenu();
            if (!hSubMenu) return NULL;
            if (!(res = parse_menu_resource( res, hSubMenu, oldFormat ))) return NULL;
            AppendMenuA( hMenu, flags, (UINT_PTR)hSubMenu, str );
        }
        else  /* Not a popup */
        {
            AppendMenuA( hMenu, flags, id, *str ? str : NULL );
        }
    } while (!end_flag);
    return res;
}

/**********************************************************************
 *	    LoadMenuIndirect    (USER.220)
 */
HMENU16 WINAPI LoadMenuIndirect16( LPCVOID template )
{
    BOOL oldFormat;
    HMENU hMenu;
    WORD version, offset;
    LPCSTR p = template;

    TRACE("(%p)\n", template );

    /* Windows 1.x and 2.x menus have a slightly different menu format from 3.x menus */
    oldFormat = (GetExeVersion16() < 0x0300);

    /* Windows 3.00 and later menu items are preceded by a MENUITEMTEMPLATEHEADER structure */
    if (!oldFormat)
    {
        version = GET_WORD(p);
        p += sizeof(WORD);
        if (version)
        {
            WARN("version must be 0 for Win16 >= 3.00 applications\n" );
            return 0;
        }
        offset = GET_WORD(p);
        p += sizeof(WORD) + offset;
    }

    if (!(hMenu = CreateMenu())) return 0;
    if (!parse_menu_resource( p, hMenu, oldFormat ))
    {
        DestroyMenu( hMenu );
        return 0;
    }
    return HMENU_16(hMenu);
}


/*************************************************************************
 *		ScrollDC (USER.221)
 */
BOOL16 WINAPI ScrollDC16( HDC16 hdc, INT16 dx, INT16 dy, const RECT16 *rect,
                          const RECT16 *cliprc, HRGN16 hrgnUpdate,
                          LPRECT16 rcUpdate )
{
    RECT rect32, clipRect32, rcUpdate32;
    BOOL16 ret;

    if (rect)
    {
        rect32.left   = rect->left;
        rect32.top    = rect->top;
        rect32.right  = rect->right;
        rect32.bottom = rect->bottom;
    }
    if (cliprc)
    {
        clipRect32.left   = cliprc->left;
        clipRect32.top    = cliprc->top;
        clipRect32.right  = cliprc->right;
        clipRect32.bottom = cliprc->bottom;
    }
    ret = ScrollDC( HDC_32(hdc), dx, dy, rect ? &rect32 : NULL,
                    cliprc ? &clipRect32 : NULL, HRGN_32(hrgnUpdate),
                    &rcUpdate32 );
    if (rcUpdate)
    {
        rcUpdate->left   = rcUpdate32.left;
        rcUpdate->top    = rcUpdate32.top;
        rcUpdate->right  = rcUpdate32.right;
        rcUpdate->bottom = rcUpdate32.bottom;
    }
    return ret;
}


/***********************************************************************
 *		GetSystemDebugState (USER.231)
 */
WORD WINAPI GetSystemDebugState16(void)
{
    return 0;  /* FIXME */
}


/***********************************************************************
 *		EqualRect (USER.244)
 */
BOOL16 WINAPI EqualRect16( const RECT16* rect1, const RECT16* rect2 )
{
    return ((rect1->left == rect2->left) && (rect1->right == rect2->right) &&
            (rect1->top == rect2->top) && (rect1->bottom == rect2->bottom));
}


/***********************************************************************
 *		ExitWindowsExec (USER.246)
 */
BOOL16 WINAPI ExitWindowsExec16( LPCSTR lpszExe, LPCSTR lpszParams )
{
    TRACE("Should run the following in DOS-mode: \"%s %s\"\n",
          lpszExe, lpszParams);
    return ExitWindowsEx( EWX_LOGOFF, 0xffffffff );
}


/***********************************************************************
 *		GetCursor (USER.247)
 */
HCURSOR16 WINAPI GetCursor16(void)
{
  return get_icon_16( GetCursor() );
}


/**********************************************************************
 *		GetAsyncKeyState (USER.249)
 */
INT16 WINAPI GetAsyncKeyState16( INT16 key )
{
    return GetAsyncKeyState( key );
}


/**********************************************************************
 *         GetMenuState    (USER.250)
 */
UINT16 WINAPI GetMenuState16( HMENU16 hMenu, UINT16 wItemID, UINT16 wFlags )
{
    return GetMenuState( HMENU_32(hMenu), wItemID, wFlags );
}


/**************************************************************************
 *		SendDriverMessage (USER.251)
 */
LRESULT WINAPI SendDriverMessage16(HDRVR16 hDriver, UINT16 msg, LPARAM lParam1,
                                   LPARAM lParam2)
{
    FIXME("(%04x, %04x, %08Ix, %08Ix): stub\n", hDriver, msg, lParam1, lParam2);
    return 0;
}


/**************************************************************************
 *		OpenDriver (USER.252)
 */
HDRVR16 WINAPI OpenDriver16(LPCSTR lpDriverName, LPCSTR lpSectionName, LPARAM lParam2)
{
    FIXME( "(%s, %s, %08Ix): stub\n", debugstr_a(lpDriverName), debugstr_a(lpSectionName), lParam2);
    return 0;
}


/**************************************************************************
 *		CloseDriver (USER.253)
 */
LRESULT WINAPI CloseDriver16(HDRVR16 hDrvr, LPARAM lParam1, LPARAM lParam2)
{
    FIXME( "(%04x, %08Ix, %08Ix): stub\n", hDrvr, lParam1, lParam2);
    return FALSE;
}


/**************************************************************************
 *		GetDriverModuleHandle (USER.254)
 */
HMODULE16 WINAPI GetDriverModuleHandle16(HDRVR16 hDrvr)
{
    FIXME("(%04x): stub\n", hDrvr);
    return 0;
}


/**************************************************************************
 *		DefDriverProc (USER.255)
 */
LRESULT WINAPI DefDriverProc16(DWORD dwDevID, HDRVR16 hDriv, UINT16 wMsg,
                               LPARAM lParam1, LPARAM lParam2)
{
    FIXME( "devID=0x%08lx hDrv=0x%04x wMsg=%04x lP1=0x%08Ix lP2=0x%08Ix: stub\n",
	  dwDevID, hDriv, wMsg, lParam1, lParam2);
    return 0;
}


/**************************************************************************
 *		GetDriverInfo (USER.256)
 */
struct DRIVERINFOSTRUCT16;
BOOL16 WINAPI GetDriverInfo16(HDRVR16 hDrvr, struct DRIVERINFOSTRUCT16 *lpDrvInfo)
{
    FIXME( "(%04x, %p): stub\n", hDrvr, lpDrvInfo);
    return FALSE;
}


/**************************************************************************
 *		GetNextDriver (USER.257)
 */
HDRVR16 WINAPI GetNextDriver16(HDRVR16 hDrvr, DWORD dwFlags)
{
    FIXME( "(%04x, %08lx): stub\n", hDrvr, dwFlags);
    return 0;
}


/**********************************************************************
 *         GetMenuItemCount    (USER.263)
 */
INT16 WINAPI GetMenuItemCount16( HMENU16 hMenu )
{
    return GetMenuItemCount( HMENU_32(hMenu) );
}


/**********************************************************************
 *         GetMenuItemID    (USER.264)
 */
UINT16 WINAPI GetMenuItemID16( HMENU16 hMenu, INT16 nPos )
{
    return GetMenuItemID( HMENU_32(hMenu), nPos );
}


/***********************************************************************
 *		GlobalAddAtom (USER.268)
 */
ATOM WINAPI GlobalAddAtom16(LPCSTR lpString)
{
  return GlobalAddAtomA(lpString);
}

/***********************************************************************
 *		GlobalDeleteAtom (USER.269)
 */
ATOM WINAPI GlobalDeleteAtom16(ATOM nAtom)
{
  return GlobalDeleteAtom(nAtom);
}

/***********************************************************************
 *		GlobalFindAtom (USER.270)
 */
ATOM WINAPI GlobalFindAtom16(LPCSTR lpString)
{
  return GlobalFindAtomA(lpString);
}

/***********************************************************************
 *		GlobalGetAtomName (USER.271)
 */
UINT16 WINAPI GlobalGetAtomName16(ATOM nAtom, LPSTR lpBuffer, INT16 nSize)
{
  return GlobalGetAtomNameA(nAtom, lpBuffer, nSize);
}


/***********************************************************************
 *		ControlPanelInfo (USER.273)
 */
void WINAPI ControlPanelInfo16( INT16 nInfoType, WORD wData, LPSTR lpBuffer )
{
    FIXME("(%d, %04x, %p): stub.\n", nInfoType, wData, lpBuffer);
}


/***********************************************************************
 *           OldSetDeskPattern   (USER.279)
 */
BOOL16 WINAPI SetDeskPattern16(void)
{
    return SystemParametersInfoA( SPI_SETDESKPATTERN, -1, NULL, FALSE );
}


/***********************************************************************
 *		GetSysColorBrush (USER.281)
 */
HBRUSH16 WINAPI GetSysColorBrush16( INT16 index )
{
    return HBRUSH_16( GetSysColorBrush(index) );
}


/***********************************************************************
 *		SelectPalette (USER.282)
 */
HPALETTE16 WINAPI SelectPalette16( HDC16 hdc, HPALETTE16 hpal, BOOL16 bForceBackground )
{
    return HPALETTE_16( SelectPalette( HDC_32(hdc), HPALETTE_32(hpal), bForceBackground ));
}

/***********************************************************************
 *		RealizePalette (USER.283)
 */
UINT16 WINAPI RealizePalette16( HDC16 hdc )
{
    return UserRealizePalette( HDC_32(hdc) );
}


/***********************************************************************
 *		GetFreeSystemResources (USER.284)
 */
WORD WINAPI GetFreeSystemResources16( WORD resType )
{
    HANDLE16 oldDS = CURRENT_DS;
    int userPercent, gdiPercent;

    switch(resType)
    {
    case GFSR_USERRESOURCES:
        CURRENT_DS = USER_HeapSel;
        userPercent = (int)LocalCountFree16() * 100 / LocalHeapSize16();
        gdiPercent  = 100;
        CURRENT_DS = oldDS;
        break;

    case GFSR_GDIRESOURCES:
        CURRENT_DS = gdi_inst;
        gdiPercent  = (int)LocalCountFree16() * 100 / LocalHeapSize16();
        userPercent = 100;
        CURRENT_DS = oldDS;
        break;

    case GFSR_SYSTEMRESOURCES:
        CURRENT_DS = USER_HeapSel;
        userPercent = (int)LocalCountFree16() * 100 / LocalHeapSize16();
        CURRENT_DS = gdi_inst;
        gdiPercent  = (int)LocalCountFree16() * 100 / LocalHeapSize16();
        CURRENT_DS = oldDS;
        break;

    default:
        userPercent = gdiPercent = 0;
        break;
    }
    TRACE("<- userPercent %d, gdiPercent %d\n", userPercent, gdiPercent);
    return (WORD)min( userPercent, gdiPercent );
}


/***********************************************************************
 *           SetDeskWallpaper   (USER.285)
 */
BOOL16 WINAPI SetDeskWallpaper16( const char *filename )
{
    return SetDeskWallpaper( filename );
}


/***********************************************************************
 *		keybd_event (USER.289)
 */
void WINAPI keybd_event16( CONTEXT *context )
{
    DWORD dwFlags = 0;

    if (HIBYTE(context->Eax) & 0x80) dwFlags |= KEYEVENTF_KEYUP;
    if (HIBYTE(context->Ebx) & 0x01) dwFlags |= KEYEVENTF_EXTENDEDKEY;

    keybd_event( LOBYTE(context->Eax), LOBYTE(context->Ebx),
                 dwFlags, MAKELONG(LOWORD(context->Esi), LOWORD(context->Edi)) );
}


/***********************************************************************
 *		mouse_event (USER.299)
 */
void WINAPI mouse_event16( CONTEXT *context )
{
    mouse_event( LOWORD(context->Eax), LOWORD(context->Ebx), LOWORD(context->Ecx),
                 LOWORD(context->Edx), MAKELONG(context->Esi, context->Edi) );
}


/***********************************************************************
 *		GetClipCursor (USER.309)
 */
void WINAPI GetClipCursor16( RECT16 *rect )
{
    if (rect)
    {
        RECT rect32;
        GetClipCursor( &rect32 );
        rect->left   = rect32.left;
        rect->top    = rect32.top;
        rect->right  = rect32.right;
        rect->bottom = rect32.bottom;
    }
}


/***********************************************************************
 *		SignalProc (USER.314)
 */
void WINAPI SignalProc16( HANDLE16 hModule, UINT16 code,
                          UINT16 uExitFn, HINSTANCE16 hInstance, HQUEUE16 hQueue )
{
    if (code == USIG16_DLL_UNLOAD)
    {
        hModule = GetExePtr(hModule);
        /* HOOK_FreeModuleHooks( hModule ); */
        free_module_classes( hModule );
        free_module_icons( hModule );
    }
}


/***********************************************************************
 *		SetEventHook (USER.321)
 *
 *	Used by Turbo Debugger for Windows
 */
FARPROC16 WINAPI SetEventHook16(FARPROC16 lpfnEventHook)
{
    FIXME("(lpfnEventHook=%p): stub\n", lpfnEventHook);
    return 0;
}


/**********************************************************************
 *		EnableHardwareInput (USER.331)
 */
BOOL16 WINAPI EnableHardwareInput16(BOOL16 bEnable)
{
    FIXME("(%d) - stub\n", bEnable);
    return TRUE;
}


/**********************************************************************
 *              LoadCursorIconHandler (USER.336)
 *
 * Supposed to load resources of Windows 2.x applications.
 */
HGLOBAL16 WINAPI LoadCursorIconHandler16( HGLOBAL16 hResource, HMODULE16 hModule, HRSRC16 hRsrc )
{
    FIXME("(%04x,%04x,%04x): old 2.x resources are not supported!\n", hResource, hModule, hRsrc);
    return 0;
}


/***********************************************************************
 *		GetMouseEventProc (USER.337)
 */
FARPROC16 WINAPI GetMouseEventProc16(void)
{
    HMODULE16 hmodule = GetModuleHandle16("USER");
    return GetProcAddress16( hmodule, "mouse_event" );
}


/***********************************************************************
 *		IsUserIdle (USER.333)
 */
BOOL16 WINAPI IsUserIdle16(void)
{
    if ( GetAsyncKeyState( VK_LBUTTON ) & 0x8000 )
        return FALSE;
    if ( GetAsyncKeyState( VK_RBUTTON ) & 0x8000 )
        return FALSE;
    if ( GetAsyncKeyState( VK_MBUTTON ) & 0x8000 )
        return FALSE;
    /* Should check for screen saver activation here ... */
    return TRUE;
}


/**********************************************************************
 *              LoadDIBIconHandler (USER.357)
 *
 * RT_ICON resource loader, installed by USER_SignalProc when module
 * is initialized.
 */
HGLOBAL16 WINAPI LoadDIBIconHandler16( HGLOBAL16 hMemObj, HMODULE16 hModule, HRSRC16 hRsrc )
{
    /* If hResource is zero we must allocate a new memory block, if it's
     * non-zero but GlobalLock() returns NULL then it was discarded and
     * we have to recommit some memory, otherwise we just need to check
     * the block size. See LoadProc() in 16-bit SDK for more.
     */
    FIXME( "%x %x %x: stub, not supported anymore\n", hMemObj, hModule, hRsrc );
    return 0;
}

/**********************************************************************
 *		LoadDIBCursorHandler (USER.356)
 *
 * RT_CURSOR resource loader. Same as above.
 */
HGLOBAL16 WINAPI LoadDIBCursorHandler16( HGLOBAL16 hMemObj, HMODULE16 hModule, HRSRC16 hRsrc )
{
    FIXME( "%x %x %x: stub, not supported anymore\n", hMemObj, hModule, hRsrc );
    return 0;
}


/**********************************************************************
 *		IsMenu    (USER.358)
 */
BOOL16 WINAPI IsMenu16( HMENU16 hmenu )
{
    return IsMenu( HMENU_32(hmenu) );
}


/***********************************************************************
 *		DCHook (USER.362)
 */
BOOL16 WINAPI DCHook16( HDC16 hdc, WORD code, DWORD data, LPARAM lParam )
{
    FIXME( "hDC = %x, %i: stub\n", hdc, code );
    return FALSE;
}


/**********************************************************************
 *		LookupIconIdFromDirectoryEx (USER.364)
 *
 * FIXME: exact parameter sizes
 */
INT16 WINAPI LookupIconIdFromDirectoryEx16( LPBYTE dir, BOOL16 bIcon,
                                            INT16 width, INT16 height, UINT16 cFlag )
{
    return LookupIconIdFromDirectoryEx( dir, bIcon, width, height, cFlag );
}


/***********************************************************************
 *		CopyIcon (USER.368)
 */
HICON16 WINAPI CopyIcon16( HINSTANCE16 hInstance, HICON16 hIcon )
{
    CURSORICONINFO *info = get_icon_ptr( hIcon );
    void *and_bits = info + 1;
    void *xor_bits = (BYTE *)and_bits + info->nHeight * get_bitmap_width_bytes( info->nWidth, 1 );
    HGLOBAL16 ret = CreateCursorIconIndirect16( hInstance, info, and_bits, xor_bits );
    release_icon_ptr( hIcon, info );
    return ret;
}


/***********************************************************************
 *		CopyCursor (USER.369)
 */
HCURSOR16 WINAPI CopyCursor16( HINSTANCE16 hInstance, HCURSOR16 hCursor )
{
    CURSORICONINFO *info = get_icon_ptr( hCursor );
    void *and_bits = info + 1;
    void *xor_bits = (BYTE *)and_bits + info->nHeight * get_bitmap_width_bytes( info->nWidth, 1 );
    HGLOBAL16 ret = CreateCursorIconIndirect16( hInstance, info, and_bits, xor_bits );
    release_icon_ptr( hCursor, info );
    return ret;
}


/***********************************************************************
 *		SubtractRect (USER.373)
 */
BOOL16 WINAPI SubtractRect16( LPRECT16 dest, const RECT16 *src1,
                              const RECT16 *src2 )
{
    RECT16 tmp;

    if (IsRectEmpty16( src1 ))
    {
        SetRectEmpty16( dest );
        return FALSE;
    }
    *dest = *src1;
    if (IntersectRect16( &tmp, src1, src2 ))
    {
        if (EqualRect16( &tmp, dest ))
        {
            SetRectEmpty16( dest );
            return FALSE;
        }
        if ((tmp.top == dest->top) && (tmp.bottom == dest->bottom))
        {
            if (tmp.left == dest->left) dest->left = tmp.right;
            else if (tmp.right == dest->right) dest->right = tmp.left;
        }
        else if ((tmp.left == dest->left) && (tmp.right == dest->right))
        {
            if (tmp.top == dest->top) dest->top = tmp.bottom;
            else if (tmp.bottom == dest->bottom) dest->bottom = tmp.top;
        }
    }
    return TRUE;
}


/**********************************************************************
 *		DllEntryPoint (USER.374)
 */
BOOL WINAPI DllEntryPoint( DWORD reason, HINSTANCE16 inst, WORD ds,
                           WORD heap, DWORD reserved1, WORD reserved2 )
{
    if (reason != DLL_PROCESS_ATTACH) return TRUE;
    if (USER_HeapSel) return TRUE;  /* already called */

    USER_HeapSel = ds;
    register_wow_handlers();
    gdi_inst = LoadLibrary16( "gdi.exe" );
    LoadLibrary16( "display.drv" );
    LoadLibrary16( "keyboard.drv" );
    LoadLibrary16( "mouse.drv" );
    LoadLibrary16( "user.exe" );  /* make sure it never gets unloaded */
    return TRUE;
}


/**********************************************************************
 *         SetMenuContextHelpId    (USER.384)
 */
BOOL16 WINAPI SetMenuContextHelpId16( HMENU16 hMenu, DWORD dwContextHelpID)
{
    return SetMenuContextHelpId( HMENU_32(hMenu), dwContextHelpID );
}


/**********************************************************************
 *         GetMenuContextHelpId    (USER.385)
 */
DWORD WINAPI GetMenuContextHelpId16( HMENU16 hMenu )
{
    return GetMenuContextHelpId( HMENU_32(hMenu) );
}


/***********************************************************************
 *		LoadImage (USER.389)
 */
HANDLE16 WINAPI LoadImage16(HINSTANCE16 hinst, LPCSTR name, UINT16 type, INT16 cx, INT16 cy, UINT16 flags)
{
    HGLOBAL16 handle;
    HRSRC16 hRsrc, hGroupRsrc;
    DWORD size;

    if (!hinst || (flags & LR_LOADFROMFILE))
    {
        if (type == IMAGE_BITMAP)
            return HBITMAP_16( LoadImageA( 0, name, type, cx, cy, flags ));
        else
            return get_icon_16( LoadImageA( 0, name, type, cx, cy, flags ));
    }

    hinst = GetExePtr( hinst );

    if (flags & LR_DEFAULTSIZE)
    {
        if (type == IMAGE_ICON)
        {
            if (!cx) cx = GetSystemMetrics(SM_CXICON);
            if (!cy) cy = GetSystemMetrics(SM_CYICON);
        }
        else if (type == IMAGE_CURSOR)
        {
            if (!cx) cx = GetSystemMetrics(SM_CXCURSOR);
            if (!cy) cy = GetSystemMetrics(SM_CYCURSOR);
        }
    }

    switch (type)
    {
    case IMAGE_BITMAP:
    {
        HBITMAP ret = 0;
        char *ptr;
        static const WCHAR prefixW[] = {'b','m','p',0};
        BITMAPFILEHEADER header;
        WCHAR path[MAX_PATH], filename[MAX_PATH];
        HANDLE file;

        filename[0] = 0;
        if (!(hRsrc = FindResource16( hinst, name, (LPCSTR)RT_BITMAP ))) return 0;
        if (!(handle = LoadResource16( hinst, hRsrc ))) return 0;
        if (!(ptr = LockResource16( handle ))) goto done;
        size = SizeofResource16( hinst, hRsrc );

        header.bfType = 0x4d42; /* 'BM' */
        header.bfReserved1 = 0;
        header.bfReserved2 = 0;
        header.bfSize = sizeof(header) + size;
        header.bfOffBits = 0;  /* not used by the 32-bit loading code */

        if (!GetTempPathW( MAX_PATH, path )) goto done;
        if (!GetTempFileNameW( path, prefixW, 0, filename )) goto done;

        file = CreateFileW( filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0 );
        if (file != INVALID_HANDLE_VALUE)
        {
            DWORD written;
            BOOL ok;
            ok = WriteFile( file, &header, sizeof(header), &written, NULL ) && (written == sizeof(header));
            if (ok) ok = WriteFile( file, ptr, size, &written, NULL ) && (written == size);
            CloseHandle( file );
            if (ok) ret = LoadImageW( 0, filename, IMAGE_BITMAP, cx, cy, flags | LR_LOADFROMFILE );
        }
    done:
        if (filename[0]) DeleteFileW( filename );
        FreeResource16( handle );
        return HBITMAP_16( ret );
    }

    case IMAGE_ICON:
    case IMAGE_CURSOR:
    {
        HICON16 hIcon = 0;
        BYTE *dir, *bits;
        INT id = 0;

        if (!(hRsrc = FindResource16( hinst, name,
                                      (LPCSTR)(type == IMAGE_ICON ? RT_GROUP_ICON : RT_GROUP_CURSOR ))))
            return 0;
        hGroupRsrc = hRsrc;

        if (!(handle = LoadResource16( hinst, hRsrc ))) return 0;
        if ((dir = LockResource16( handle ))) id = LookupIconIdFromDirectory( dir, type == IMAGE_ICON );
        FreeResource16( handle );
        if (!id) return 0;

        if (!(hRsrc = FindResource16( hinst, MAKEINTRESOURCEA(id),
                                      (LPCSTR)(type == IMAGE_ICON ? RT_ICON : RT_CURSOR) ))) return 0;

        if ((flags & LR_SHARED) && (hIcon = find_shared_icon( hinst, hRsrc ) ) != 0) return hIcon;

        if (!(handle = LoadResource16( hinst, hRsrc ))) return 0;
        bits = LockResource16( handle );
        size = SizeofResource16( hinst, hRsrc );
        hIcon = CreateIconFromResourceEx16( bits, size, type == IMAGE_ICON, 0x00030000, cx, cy, flags );
        FreeResource16( handle );

        if (hIcon && (flags & LR_SHARED)) add_shared_icon( hinst, hRsrc, hGroupRsrc, hIcon );
        return hIcon;
    }
    default:
        return 0;
    }
}

/******************************************************************************
 *		CopyImage (USER.390) Creates new image and copies attributes to it
 *
 */
HICON16 WINAPI CopyImage16(HANDLE16 hnd, UINT16 type, INT16 desiredx,
			   INT16 desiredy, UINT16 flags)
{
    if (flags & LR_COPYFROMRESOURCE) FIXME( "LR_COPYFROMRESOURCE not supported\n" );

    switch (type)
    {
    case IMAGE_BITMAP:
        return HBITMAP_16( CopyImage( HBITMAP_32(hnd), type, desiredx, desiredy, flags ));
    case IMAGE_ICON:
    case IMAGE_CURSOR:
        return CopyIcon16( FarGetOwner16(hnd), hnd );
    default:
        return 0;
    }
}

/**********************************************************************
 *		DrawIconEx (USER.394)
 */
BOOL16 WINAPI DrawIconEx16(HDC16 hdc, INT16 xLeft, INT16 yTop, HICON16 hIcon,
			   INT16 cxWidth, INT16 cyWidth, UINT16 istep,
			   HBRUSH16 hbr, UINT16 flags)
{
  return DrawIconEx(HDC_32(hdc), xLeft, yTop, get_icon_32(hIcon), cxWidth, cyWidth,
		    istep, HBRUSH_32(hbr), flags);
}

/**********************************************************************
 *		GetIconInfo (USER.395)
 */
BOOL16 WINAPI GetIconInfo16(HICON16 hIcon, LPICONINFO16 iconinfo)
{
    CURSORICONINFO *info = get_icon_ptr( hIcon );
    INT height;

    if (!info) return FALSE;

    if ((info->ptHotSpot.x == ICON_HOTSPOT) && (info->ptHotSpot.y == ICON_HOTSPOT))
    {
        iconinfo->fIcon    = TRUE;
        iconinfo->xHotspot = info->nWidth / 2;
        iconinfo->yHotspot = info->nHeight / 2;
    }
    else
    {
        iconinfo->fIcon    = FALSE;
        iconinfo->xHotspot = info->ptHotSpot.x;
        iconinfo->yHotspot = info->ptHotSpot.y;
    }

    height = info->nHeight;

    if (info->bBitsPerPixel > 1)
    {
        iconinfo->hbmColor = HBITMAP_16( CreateBitmap( info->nWidth, info->nHeight,
                                                       info->bPlanes, info->bBitsPerPixel,
                                                       (char *)(info + 1)
                                                       + info->nHeight *
                                                       get_bitmap_width_bytes(info->nWidth,1) ));
    }
    else
    {
        iconinfo->hbmColor = 0;
        height *= 2;
    }

    iconinfo->hbmMask = HBITMAP_16( CreateBitmap( info->nWidth, height, 1, 1, info + 1 ));
    release_icon_ptr( hIcon, info );
    return TRUE;
}


/***********************************************************************
 *		FinalUserInit (USER.400)
 */
void WINAPI FinalUserInit16( void )
{
    /* FIXME: Should chain to FinalGdiInit */
}


/***********************************************************************
 *		CreateCursor (USER.406)
 */
HCURSOR16 WINAPI CreateCursor16(HINSTANCE16 hInstance,
				INT16 xHotSpot, INT16 yHotSpot,
				INT16 nWidth, INT16 nHeight,
				LPCVOID lpANDbits, LPCVOID lpXORbits)
{
  CURSORICONINFO info;

  info.ptHotSpot.x = xHotSpot;
  info.ptHotSpot.y = yHotSpot;
  info.nWidth = nWidth;
  info.nHeight = nHeight;
  info.nWidthBytes = 0;
  info.bPlanes = 1;
  info.bBitsPerPixel = 1;

  return CreateCursorIconIndirect16(hInstance, &info, lpANDbits, lpXORbits);
}


/***********************************************************************
 *		CreateIcon (USER.407)
 */
HICON16 WINAPI CreateIcon16( HINSTANCE16 hInstance, INT16 nWidth,
                             INT16 nHeight, BYTE bPlanes, BYTE bBitsPixel,
                             LPCVOID lpANDbits, LPCVOID lpXORbits )
{
    CURSORICONINFO info;

    info.ptHotSpot.x = ICON_HOTSPOT;
    info.ptHotSpot.y = ICON_HOTSPOT;
    info.nWidth = nWidth;
    info.nHeight = nHeight;
    info.nWidthBytes = 0;
    info.bPlanes = bPlanes;
    info.bBitsPerPixel = bBitsPixel;

    return CreateCursorIconIndirect16( hInstance, &info, lpANDbits, lpXORbits );
}


/***********************************************************************
 *		CreateCursorIconIndirect (USER.408)
 */
HGLOBAL16 WINAPI CreateCursorIconIndirect16( HINSTANCE16 hInstance,
                                           CURSORICONINFO *info,
                                           LPCVOID lpANDbits,
                                           LPCVOID lpXORbits )
{
    HICON16 handle;
    CURSORICONINFO *ptr;
    int sizeAnd, sizeXor;

    hInstance = GetExePtr( hInstance );  /* Make it a module handle */
    if (!lpXORbits || !lpANDbits || info->bPlanes != 1) return 0;
    info->nWidthBytes = get_bitmap_width_bytes(info->nWidth,info->bBitsPerPixel);
    sizeXor = info->nHeight * info->nWidthBytes;
    sizeAnd = info->nHeight * get_bitmap_width_bytes( info->nWidth, 1 );
    if (!(handle = alloc_icon_handle( sizeof(CURSORICONINFO) + sizeXor + sizeAnd )))
        return 0;
    FarSetOwner16( handle, hInstance );
    ptr = get_icon_ptr( handle );
    memcpy( ptr, info, sizeof(*info) );
    memcpy( ptr + 1, lpANDbits, sizeAnd );
    memcpy( (char *)(ptr + 1) + sizeAnd, lpXORbits, sizeXor );
    release_icon_ptr( handle, ptr );
    return handle;
}


/***********************************************************************
 *		InitThreadInput   (USER.409)
 */
HQUEUE16 WINAPI InitThreadInput16( WORD unknown, WORD flags )
{
    /* nothing to do here */
    return 0xbeef;
}


/*******************************************************************
 *         InsertMenu    (USER.410)
 */
BOOL16 WINAPI InsertMenu16( HMENU16 hMenu, UINT16 pos, UINT16 flags,
                            UINT16 id, SEGPTR data )
{
    UINT pos32 = (UINT)pos;
    if ((pos == (UINT16)-1) && (flags & MF_BYPOSITION)) pos32 = (UINT)-1;
    if (IS_MENU_STRING_ITEM(flags) && data)
        return InsertMenuA( HMENU_32(hMenu), pos32, flags, id, MapSL(data) );

    /* If "data" is an HBITMAP, the high WORD will contain the application's DGROUP selector if the
     * application cast (LPSTR)hBitmap rather than (LPSTR)(LONG)hBitmap. */
    if (flags & MF_BITMAP) data = (SEGPTR)HBITMAP_32(LOWORD(data));
    return InsertMenuA( HMENU_32(hMenu), pos32, flags, id, (LPSTR)data );
}


/*******************************************************************
 *         AppendMenu    (USER.411)
 */
BOOL16 WINAPI AppendMenu16(HMENU16 hMenu, UINT16 flags, UINT16 id, SEGPTR data)
{
    return InsertMenu16( hMenu, -1, flags | MF_BYPOSITION, id, data );
}


/**********************************************************************
 *         RemoveMenu   (USER.412)
 */
BOOL16 WINAPI RemoveMenu16( HMENU16 hMenu, UINT16 nPos, UINT16 wFlags )
{
    return RemoveMenu( HMENU_32(hMenu), nPos, wFlags );
}


/**********************************************************************
 *         DeleteMenu    (USER.413)
 */
BOOL16 WINAPI DeleteMenu16( HMENU16 hMenu, UINT16 nPos, UINT16 wFlags )
{
    return DeleteMenu( HMENU_32(hMenu), nPos, wFlags );
}


/*******************************************************************
 *         ModifyMenu    (USER.414)
 */
BOOL16 WINAPI ModifyMenu16( HMENU16 hMenu, UINT16 pos, UINT16 flags,
                            UINT16 id, SEGPTR data )
{
    if (IS_MENU_STRING_ITEM(flags))
        return ModifyMenuA( HMENU_32(hMenu), pos, flags, id, MapSL(data) );
    return ModifyMenuA( HMENU_32(hMenu), pos, flags, id, (LPSTR)data );
}


/**********************************************************************
 *         CreatePopupMenu    (USER.415)
 */
HMENU16 WINAPI CreatePopupMenu16(void)
{
    return HMENU_16( CreatePopupMenu() );
}


/**********************************************************************
 *         SetMenuItemBitmaps    (USER.418)
 */
BOOL16 WINAPI SetMenuItemBitmaps16( HMENU16 hMenu, UINT16 nPos, UINT16 wFlags,
                                    HBITMAP16 hNewUnCheck, HBITMAP16 hNewCheck)
{
    return SetMenuItemBitmaps( HMENU_32(hMenu), nPos, wFlags,
                               HBITMAP_32(hNewUnCheck), HBITMAP_32(hNewCheck) );
}


/***********************************************************************
 *           wvsprintf   (USER.421)
 */
INT16 WINAPI wvsprintf16( LPSTR buffer, LPCSTR spec, VA_LIST16 args )
{
    WPRINTF_FORMAT format;
    LPSTR p = buffer;
    UINT i, len, sign;
    CHAR number[20];
    CHAR char_view = 0;
    LPCSTR lpcstr_view = NULL;
    INT int_view;
    SEGPTR seg_str;

    while (*spec)
    {
        if (*spec != '%') { *p++ = *spec++; continue; }
        spec++;
        if (*spec == '%') { *p++ = *spec++; continue; }
        spec += parse_format( spec, &format );
        switch(format.type)
        {
        case WPR_CHAR:
            char_view = VA_ARG16( args, CHAR );
            len = format.precision = 1;
            break;
        case WPR_STRING:
            seg_str = VA_ARG16( args, SEGPTR );
            if (IsBadReadPtr16( seg_str, 1 )) lpcstr_view = "";
            else lpcstr_view = MapSL( seg_str );
            if (!lpcstr_view) lpcstr_view = "(null)";
            for (len = 0; !format.precision || (len < format.precision); len++)
                if (!lpcstr_view[len]) break;
            format.precision = len;
            break;
        case WPR_SIGNED:
            if (format.flags & WPRINTF_LONG) int_view = VA_ARG16( args, INT );
            else int_view = VA_ARG16( args, INT16 );
            len = sprintf( number, "%d", int_view );
            break;
        case WPR_UNSIGNED:
            if (format.flags & WPRINTF_LONG) int_view = VA_ARG16( args, UINT );
            else int_view = VA_ARG16( args, UINT16 );
            len = sprintf( number, "%u", int_view );
            break;
        case WPR_HEXA:
            if (format.flags & WPRINTF_LONG) int_view = VA_ARG16( args, UINT );
            else int_view = VA_ARG16( args, UINT16 );
            len = sprintf( number, (format.flags & WPRINTF_UPPER_HEX) ? "%X" : "%x", int_view);
            break;
        case WPR_UNKNOWN:
            continue;
        }
        if (format.precision < len) format.precision = len;
        if (format.flags & WPRINTF_LEFTALIGN) format.flags &= ~WPRINTF_ZEROPAD;
        if ((format.flags & WPRINTF_ZEROPAD) && (format.width > format.precision))
            format.precision = format.width;
        if (format.flags & WPRINTF_PREFIX_HEX) len += 2;

        sign = 0;
        if (!(format.flags & WPRINTF_LEFTALIGN))
            for (i = format.precision; i < format.width; i++) *p++ = ' ';
        switch(format.type)
        {
        case WPR_CHAR:
            *p = char_view;
            /* wsprintf16 ignores null characters */
            if (*p != '\0') p++;
            else if (format.width > 1) *p++ = ' ';
            break;
        case WPR_STRING:
            if (len) memcpy( p, lpcstr_view, len );
            p += len;
            break;
        case WPR_HEXA:
            if (format.flags & WPRINTF_PREFIX_HEX)
            {
                *p++ = '0';
                *p++ = (format.flags & WPRINTF_UPPER_HEX) ? 'X' : 'x';
                len -= 2;
            }
            /* fall through */
        case WPR_SIGNED:
            /* Transfer the sign now, just in case it will be zero-padded*/
            if (number[0] == '-')
            {
                *p++ = '-';
                sign = 1;
            }
            /* fall through */
        case WPR_UNSIGNED:
            for (i = len; i < format.precision; i++) *p++ = '0';
            if (len > sign) memcpy( p, number + sign, len - sign );
            p += len-sign;
            break;
        case WPR_UNKNOWN:
            continue;
        }
        if (format.flags & WPRINTF_LEFTALIGN)
            for (i = format.precision; i < format.width; i++) *p++ = ' ';
    }
    *p = 0;
    return p - buffer;
}


/***********************************************************************
 *           _wsprintf   (USER.420)
 */
INT16 WINAPIV wsprintf16( LPSTR buffer, LPCSTR spec, VA_LIST16 valist )
{
    return wvsprintf16( buffer, spec, valist );
}


/***********************************************************************
 *           lstrcmp   (USER.430)
 */
INT16 WINAPI lstrcmp16( LPCSTR str1, LPCSTR str2 )
{
    int ret;
    /* Looks too complicated, but in optimized strcpy we might get
    * a 32bit wide difference and would truncate it to 16 bit, so
     * erroneously returning equality. */
    ret = strcmp( str1, str2 );
    if (ret < 0) return -1;
    if (ret > 0) return  1;
    return 0;
}


/***********************************************************************
 *           AnsiUpper   (USER.431)
 */
SEGPTR WINAPI AnsiUpper16( SEGPTR strOrChar )
{
    /* uppercase only one char if strOrChar < 0x10000 */
    if (HIWORD(strOrChar))
    {
        CharUpperA( MapSL(strOrChar) );
        return strOrChar;
    }
    else return (SEGPTR)CharUpperA( (LPSTR)strOrChar );
}


/***********************************************************************
 *           AnsiLower   (USER.432)
 */
SEGPTR WINAPI AnsiLower16( SEGPTR strOrChar )
{
    /* lowercase only one char if strOrChar < 0x10000 */
    if (HIWORD(strOrChar))
    {
        CharLowerA( MapSL(strOrChar) );
        return strOrChar;
    }
    else return (SEGPTR)CharLowerA( (LPSTR)strOrChar );
}


/***********************************************************************
 *           AnsiUpperBuff   (USER.437)
 */
UINT16 WINAPI AnsiUpperBuff16( LPSTR str, UINT16 len )
{
    CharUpperBuffA( str, len ? len : 65536 );
    return len;
}


/***********************************************************************
 *           AnsiLowerBuff   (USER.438)
 */
UINT16 WINAPI AnsiLowerBuff16( LPSTR str, UINT16 len )
{
    CharLowerBuffA( str, len ? len : 65536 );
    return len;
}


/*******************************************************************
 *              InsertMenuItem   (USER.441)
 *
 * FIXME: untested
 */
BOOL16 WINAPI InsertMenuItem16( HMENU16 hmenu, UINT16 pos, BOOL16 byposition,
                                const MENUITEMINFO16 *mii )
{
    MENUITEMINFOA miia;

    miia.cbSize        = sizeof(miia);
    miia.fMask         = mii->fMask;
    miia.dwTypeData    = (LPSTR)mii->dwTypeData;
    miia.fType         = mii->fType;
    miia.fState        = mii->fState;
    miia.wID           = mii->wID;
    miia.hSubMenu      = HMENU_32(mii->hSubMenu);
    miia.hbmpChecked   = HBITMAP_32(mii->hbmpChecked);
    miia.hbmpUnchecked = HBITMAP_32(mii->hbmpUnchecked);
    miia.dwItemData    = mii->dwItemData;
    miia.cch           = mii->cch;
    if (IS_MENU_STRING_ITEM(miia.fType))
        miia.dwTypeData = MapSL(mii->dwTypeData);
    return InsertMenuItemA( HMENU_32(hmenu), pos, byposition, &miia );
}


/**********************************************************************
 *	     DrawState    (USER.449)
 */
BOOL16 WINAPI DrawState16( HDC16 hdc, HBRUSH16 hbr, DRAWSTATEPROC16 func, LPARAM ldata,
                           WPARAM16 wdata, INT16 x, INT16 y, INT16 cx, INT16 cy, UINT16 flags )
{
    struct draw_state_info info;
    UINT opcode = flags & 0xf;

    if (opcode == DST_TEXT || opcode == DST_PREFIXTEXT)
    {
        /* make sure DrawStateA doesn't try to use ldata as a pointer */
        if (!wdata) wdata = strlen( MapSL(ldata) );
        if (!cx || !cy)
        {
            SIZE s;
            if (!GetTextExtentPoint32A( HDC_32(hdc), MapSL(ldata), wdata, &s )) return FALSE;
            if (!cx) cx = s.cx;
            if (!cy) cy = s.cy;
        }
    }
    info.proc  = func;
    info.param = ldata;
    return DrawStateA( HDC_32(hdc), HBRUSH_32(hbr), draw_state_callback,
                       (LPARAM)&info, wdata, x, y, cx, cy, flags );
}


/**********************************************************************
 *		CreateIconFromResourceEx (USER.450)
 *
 * FIXME: not sure about exact parameter types
 */
HICON16 WINAPI CreateIconFromResourceEx16(LPBYTE bits, UINT16 cbSize,
					  BOOL16 bIcon, DWORD dwVersion,
					  INT16 width, INT16 height,
					  UINT16 cFlag)
{
    return get_icon_16( CreateIconFromResourceEx( bits, cbSize, bIcon, dwVersion, width, height, cFlag ));
}


/***********************************************************************
 *		AdjustWindowRectEx (USER.454)
 */
BOOL16 WINAPI AdjustWindowRectEx16( LPRECT16 rect, DWORD style, BOOL16 menu, DWORD exStyle )
{
    RECT rect32;
    BOOL ret;

    rect32.left   = rect->left;
    rect32.top    = rect->top;
    rect32.right  = rect->right;
    rect32.bottom = rect->bottom;
    ret = AdjustWindowRectEx( &rect32, style, menu, exStyle );
    rect->left   = rect32.left;
    rect->top    = rect32.top;
    rect->right  = rect32.right;
    rect->bottom = rect32.bottom;
    return ret;
}


/**********************************************************************
 *              GetIconID (USER.455)
 */
WORD WINAPI GetIconID16( HGLOBAL16 hResource, DWORD resType )
{
    BYTE *dir = GlobalLock16(hResource);

    switch (resType)
    {
    case RT_CURSOR:
        return LookupIconIdFromDirectoryEx16( dir, FALSE, GetSystemMetrics(SM_CXCURSOR),
                                              GetSystemMetrics(SM_CYCURSOR), LR_MONOCHROME );
    case RT_ICON:
        return LookupIconIdFromDirectoryEx16( dir, TRUE, GetSystemMetrics(SM_CXICON),
                                              GetSystemMetrics(SM_CYICON), 0 );
    }
    return 0;
}


/**********************************************************************
 *              LoadIconHandler (USER.456)
 */
HICON16 WINAPI LoadIconHandler16( HGLOBAL16 hResource, BOOL16 bNew )
{
    return CreateIconFromResourceEx16( LockResource16( hResource ), 0xffff, TRUE,
                                       bNew ? 0x00030000 : 0x00020000, 0, 0, LR_DEFAULTCOLOR );
}


/***********************************************************************
 *		DestroyIcon (USER.457)
 */
BOOL16 WINAPI DestroyIcon16(HICON16 hIcon)
{
    int count;

    TRACE("%04x\n", hIcon );

    count = release_shared_icon( hIcon );
    if (count != -1) return !count;
    /* assume non-shared */
    free_icon_handle( hIcon );
    return TRUE;
}

/***********************************************************************
 *		DestroyCursor (USER.458)
 */
BOOL16 WINAPI DestroyCursor16(HCURSOR16 hCursor)
{
    return DestroyIcon16( hCursor );
}


/***********************************************************************
 *		DumpIcon (USER.459)
 */
DWORD WINAPI DumpIcon16( SEGPTR pInfo, WORD *lpLen,
                       SEGPTR *lpXorBits, SEGPTR *lpAndBits )
{
    CURSORICONINFO *info = MapSL( pInfo );
    int sizeAnd, sizeXor;

    if (!info) return 0;
    sizeXor = info->nHeight * info->nWidthBytes;
    sizeAnd = info->nHeight * get_bitmap_width_bytes( info->nWidth, 1 );
    if (lpAndBits) *lpAndBits = pInfo + sizeof(CURSORICONINFO);
    if (lpXorBits) *lpXorBits = pInfo + sizeof(CURSORICONINFO) + sizeAnd;
    if (lpLen) *lpLen = sizeof(CURSORICONINFO) + sizeAnd + sizeXor;
    return MAKELONG( sizeXor, sizeXor );
}


/*******************************************************************
 *			DRAG_QueryUpdate16
 *
 * Recursively find a child that contains spDragInfo->pt point
 * and send WM_QUERYDROPOBJECT. Helper for DragObject16.
 */
static BOOL DRAG_QueryUpdate16( HWND hQueryWnd, SEGPTR spDragInfo )
{
    BOOL bResult;
    WPARAM wParam;
    POINT pt, old_pt;
    LPDRAGINFO16 ptrDragInfo = MapSL(spDragInfo);
    RECT tempRect;
    HWND child;

    if (!IsWindowEnabled(hQueryWnd)) return FALSE;

    old_pt.x = ptrDragInfo->pt.x;
    old_pt.y = ptrDragInfo->pt.y;
    pt = old_pt;
    ScreenToClient( hQueryWnd, &pt );
    child = ChildWindowFromPointEx( hQueryWnd, pt, CWP_SKIPINVISIBLE );
    if (!child) return FALSE;

    if (child != hQueryWnd)
    {
        wParam = 0;
        if (DRAG_QueryUpdate16( child, spDragInfo )) return TRUE;
    }
    else
    {
        GetClientRect( hQueryWnd, &tempRect );
        wParam = !PtInRect( &tempRect, pt );
    }

    ptrDragInfo->pt.x = pt.x;
    ptrDragInfo->pt.y = pt.y;
    ptrDragInfo->hScope = HWND_16(hQueryWnd);

    bResult = SendMessage16( HWND_16(hQueryWnd), WM_QUERYDROPOBJECT, wParam, spDragInfo );

    if (!bResult)
    {
        ptrDragInfo->pt.x = old_pt.x;
        ptrDragInfo->pt.y = old_pt.y;
    }
    return bResult;
}


/******************************************************************************
 *		DragObject (USER.464)
 */
DWORD WINAPI DragObject16( HWND16 hwndScope, HWND16 hWnd, UINT16 wObj,
                           HANDLE16 hOfStruct, WORD szList, HCURSOR16 hCursor )
{
    MSG	msg;
    LPDRAGINFO16 lpDragInfo;
    SEGPTR	spDragInfo;
    HCURSOR 	hOldCursor=0, hBummer=0, hCursor32;
    HGLOBAL16	hDragInfo  = GlobalAlloc16( GMEM_SHARE | GMEM_ZEROINIT, 2*sizeof(DRAGINFO16));
    HCURSOR	hCurrentCursor = 0;
    HWND16	hCurrentWnd = 0;

    lpDragInfo = (LPDRAGINFO16) GlobalLock16(hDragInfo);
    spDragInfo = WOWGlobalLock16(hDragInfo);

    if( !lpDragInfo || !spDragInfo ) return 0L;

    if (!(hBummer = LoadCursorA(0, MAKEINTRESOURCEA(OCR_NO))))
    {
        GlobalFree16(hDragInfo);
        return 0L;
    }

    if ((hCursor32 = get_icon_32( hCursor ))) SetCursor( hCursor32 );

    lpDragInfo->hWnd   = hWnd;
    lpDragInfo->hScope = 0;
    lpDragInfo->wFlags = wObj;
    lpDragInfo->hList  = szList; /* near pointer! */
    lpDragInfo->hOfStruct = hOfStruct;
    lpDragInfo->l = 0L;

    SetCapture( HWND_32(hWnd) );
    ShowCursor( TRUE );

    do
    {
        GetMessageW( &msg, 0, WM_MOUSEFIRST, WM_MOUSELAST );

       *(lpDragInfo+1) = *lpDragInfo;

	lpDragInfo->pt.x = msg.pt.x;
	lpDragInfo->pt.y = msg.pt.y;

	/* update DRAGINFO struct */
	if( DRAG_QueryUpdate16(WIN_Handle32(hwndScope), spDragInfo) > 0 )
	    hCurrentCursor = hCursor32;
	else
        {
            hCurrentCursor = hBummer;
            lpDragInfo->hScope = 0;
	}
	if( hCurrentCursor )
	    SetCursor(hCurrentCursor);

	/* send WM_DRAGLOOP */
        SendMessage16( hWnd, WM_DRAGLOOP, hCurrentCursor != hBummer, spDragInfo );
	/* send WM_DRAGSELECT or WM_DRAGMOVE */
	if( hCurrentWnd != lpDragInfo->hScope )
	{
	    if( hCurrentWnd )
	        SendMessage16( hCurrentWnd, WM_DRAGSELECT, 0,
                        MAKELPARAM(LOWORD(spDragInfo)+sizeof(DRAGINFO16),
                                   HIWORD(spDragInfo)) );
	    hCurrentWnd = lpDragInfo->hScope;
	    if( hCurrentWnd )
                SendMessage16( hCurrentWnd, WM_DRAGSELECT, 1, spDragInfo);
	}
	else
	    if( hCurrentWnd )
                SendMessage16( hCurrentWnd, WM_DRAGMOVE, 0, spDragInfo);

    } while( msg.message != WM_LBUTTONUP && msg.message != WM_NCLBUTTONUP );

    ReleaseCapture();
    ShowCursor( FALSE );

    if( hCursor ) SetCursor(hOldCursor);

    if( hCurrentCursor != hBummer )
	msg.lParam = SendMessage16( lpDragInfo->hScope, WM_DROPOBJECT,
                                    hWnd, spDragInfo );
    else
        msg.lParam = 0;
    GlobalFree16(hDragInfo);

    return (DWORD)(msg.lParam);
}


/***********************************************************************
 *		DrawFocusRect (USER.466)
 */
void WINAPI DrawFocusRect16( HDC16 hdc, const RECT16* rc )
{
    RECT rect32;

    rect32.left   = rc->left;
    rect32.top    = rc->top;
    rect32.right  = rc->right;
    rect32.bottom = rc->bottom;
    DrawFocusRect( HDC_32(hdc), &rect32 );
}


/***********************************************************************
 *           AnsiNext   (USER.472)
 */
SEGPTR WINAPI AnsiNext16(SEGPTR current)
{
    char *ptr = MapSL(current);
    return current + (CharNextA(ptr) - ptr);
}


/***********************************************************************
 *           AnsiPrev   (USER.473)
 */
SEGPTR WINAPI AnsiPrev16( LPCSTR start, SEGPTR current )
{
    char *ptr = MapSL(current);
    return current - (ptr - CharPrevA( start, ptr ));
}


/****************************************************************************
 *		GetKeyboardLayoutName (USER.477)
 */
INT16 WINAPI GetKeyboardLayoutName16( LPSTR name )
{
    return GetKeyboardLayoutNameA( name );
}


/***********************************************************************
 *		SystemParametersInfo (USER.483)
 */
BOOL16 WINAPI SystemParametersInfo16( UINT16 uAction, UINT16 uParam,
                                      LPVOID lpvParam, UINT16 fuWinIni )
{
    BOOL16 ret;

    TRACE("(%u, %u, %p, %u)\n", uAction, uParam, lpvParam, fuWinIni);

    switch (uAction)
    {
    case SPI_GETBEEP:
    case SPI_GETSCREENSAVEACTIVE:
    case SPI_GETICONTITLEWRAP:
    case SPI_GETMENUDROPALIGNMENT:
    case SPI_GETFASTTASKSWITCH:
    case SPI_GETDRAGFULLWINDOWS:
    {
	BOOL tmp;
	ret = SystemParametersInfoA( uAction, uParam, lpvParam ? &tmp : NULL, fuWinIni );
	if (ret && lpvParam) *(BOOL16 *)lpvParam = tmp;
	break;
    }

    case SPI_GETBORDER:
    case SPI_ICONHORIZONTALSPACING:
    case SPI_GETSCREENSAVETIMEOUT:
    case SPI_GETGRIDGRANULARITY:
    case SPI_GETKEYBOARDDELAY:
    case SPI_ICONVERTICALSPACING:
    {
	INT tmp;
	ret = SystemParametersInfoA( uAction, uParam, lpvParam ? &tmp : NULL, fuWinIni );
	if (ret && lpvParam) *(INT16 *)lpvParam = tmp;
	break;
    }

    case SPI_GETKEYBOARDSPEED:
    case SPI_GETMOUSEHOVERWIDTH:
    case SPI_GETMOUSEHOVERHEIGHT:
    case SPI_GETMOUSEHOVERTIME:
    {
	DWORD tmp;
	ret = SystemParametersInfoA( uAction, uParam, lpvParam ? &tmp : NULL, fuWinIni );
	if (ret && lpvParam) *(WORD *)lpvParam = tmp;
	break;
    }

    case SPI_GETICONTITLELOGFONT:
    {
	LOGFONTA tmp;
	ret = SystemParametersInfoA( uAction, uParam, lpvParam ? &tmp : NULL, fuWinIni );
	if (ret && lpvParam) logfont_32_to_16( &tmp, (LPLOGFONT16)lpvParam );
	break;
    }

    case SPI_GETNONCLIENTMETRICS:
    {
	NONCLIENTMETRICSA tmp;
	LPNONCLIENTMETRICS16 lpnm16 = (LPNONCLIENTMETRICS16)lpvParam;
	if (lpnm16 && lpnm16->cbSize == sizeof(NONCLIENTMETRICS16))
	{
	    tmp.cbSize = sizeof(NONCLIENTMETRICSA);
	    ret = SystemParametersInfoA( uAction, uParam, &tmp, fuWinIni );
	    if (ret)
            {
                lpnm16->iBorderWidth	 = tmp.iBorderWidth;
                lpnm16->iScrollWidth	 = tmp.iScrollWidth;
                lpnm16->iScrollHeight	 = tmp.iScrollHeight;
                lpnm16->iCaptionWidth	 = tmp.iCaptionWidth;
                lpnm16->iCaptionHeight	 = tmp.iCaptionHeight;
                lpnm16->iSmCaptionWidth	 = tmp.iSmCaptionWidth;
                lpnm16->iSmCaptionHeight = tmp.iSmCaptionHeight;
                lpnm16->iMenuWidth       = tmp.iMenuWidth;
                lpnm16->iMenuHeight      = tmp.iMenuHeight;
                logfont_32_to_16( &tmp.lfCaptionFont,	&lpnm16->lfCaptionFont );
                logfont_32_to_16( &tmp.lfSmCaptionFont,	&lpnm16->lfSmCaptionFont );
                logfont_32_to_16( &tmp.lfMenuFont,	&lpnm16->lfMenuFont );
                logfont_32_to_16( &tmp.lfStatusFont,	&lpnm16->lfStatusFont );
                logfont_32_to_16( &tmp.lfMessageFont,	&lpnm16->lfMessageFont );
            }
	}
	else /* winfile 95 sets cbSize to 340 */
	    ret = SystemParametersInfoA( uAction, uParam, lpvParam, fuWinIni );
	break;
    }

    case SPI_GETWORKAREA:
    {
	RECT tmp;
	ret = SystemParametersInfoA( uAction, uParam, lpvParam ? &tmp : NULL, fuWinIni );
	if (ret && lpvParam)
        {
            RECT16 *r16 = lpvParam;
            r16->left   = tmp.left;
            r16->top    = tmp.top;
            r16->right  = tmp.right;
            r16->bottom = tmp.bottom;
        }
	break;
    }

    default:
	ret = SystemParametersInfoA( uAction, uParam, lpvParam, fuWinIni );
        break;
    }

    return ret;
}


/***********************************************************************
 *		USER_489 (USER.489)
 */
LONG WINAPI stub_USER_489(void)
{
    FIXME("stub\n");
    return 0;
}


/***********************************************************************
 *		USER_490 (USER.490)
 */
LONG WINAPI stub_USER_490(void)
{
    FIXME("stub\n");
    return 0;
}


/***********************************************************************
 *		USER_492 (USER.492)
 */
LONG WINAPI stub_USER_492(void)
{
    FIXME("stub\n");
    return 0;
}


/***********************************************************************
 *		USER_496 (USER.496)
 */
LONG WINAPI stub_USER_496(void)
{
    FIXME("stub\n");
    return 0;
}


/***********************************************************************
 *           FormatMessage   (USER.606)
 */
DWORD WINAPI FormatMessage16(
    DWORD   dwFlags,
    SEGPTR lpSource,     /* [in] NOTE: not always a valid pointer */
    WORD   dwMessageId,
    WORD   dwLanguageId,
    LPSTR  lpBuffer,     /* [out] NOTE: *((HLOCAL16*)) for FORMAT_MESSAGE_ALLOCATE_BUFFER*/
    WORD   nSize,
    LPDWORD args )       /* [in] NOTE: va_list *args */
{
/* This implementation is completely dependent on the format of the va_list on x86 CPUs */
    LPSTR       target,t;
    DWORD       talloced;
    LPSTR       from,f;
    DWORD       width = dwFlags & FORMAT_MESSAGE_MAX_WIDTH_MASK;
    BOOL        eos = FALSE;
    LPSTR       allocstring = NULL;

    TRACE("(0x%lx,%lx,%d,0x%x,%p,%d,%p)\n",
          dwFlags,lpSource,dwMessageId,dwLanguageId,lpBuffer,nSize,args);
        if ((dwFlags & FORMAT_MESSAGE_FROM_SYSTEM)
                && (dwFlags & FORMAT_MESSAGE_FROM_HMODULE)) return 0;
        if ((dwFlags & FORMAT_MESSAGE_FROM_STRING)
                &&((dwFlags & FORMAT_MESSAGE_FROM_SYSTEM)
                        || (dwFlags & FORMAT_MESSAGE_FROM_HMODULE))) return 0;

    if (width && width != FORMAT_MESSAGE_MAX_WIDTH_MASK)
        FIXME("line wrapping (%lu) not supported.\n", width);
    from = NULL;
    if (dwFlags & FORMAT_MESSAGE_FROM_STRING)
    {
        char *source = MapSL(lpSource);
        from = HeapAlloc( GetProcessHeap(), 0, strlen(source)+1 );
        strcpy( from, source );
    }
    else if (dwFlags & FORMAT_MESSAGE_FROM_SYSTEM) {
        from = HeapAlloc( GetProcessHeap(),0,200 );
        sprintf(from,"Systemmessage, messageid = 0x%08x\n",dwMessageId);
    }
    else if (dwFlags & FORMAT_MESSAGE_FROM_HMODULE) {
        INT16   bufsize;
        HINSTANCE16 hinst16 = ((HINSTANCE16)lpSource & 0xffff);

        dwMessageId &= 0xFFFF;
        bufsize=LoadString16(hinst16,dwMessageId,NULL,0);
        if (bufsize) {
            from = HeapAlloc( GetProcessHeap(), 0, bufsize +1);
            LoadString16(hinst16,dwMessageId,from,bufsize+1);
        }
    }
    target      = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, 100);
    t   = target;
    talloced= 100;

#define ADD_TO_T(c) \
        do { \
            *t++=c;\
            if (t-target == talloced) {\
                target  = HeapReAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,target,talloced*2);\
                t       = target+talloced;\
                talloced*=2;\
            } \
        } while(0)

    if (from) {
        f=from;
        while (*f && !eos) {
            if (*f=='%') {
                int     insertnr;
                char    *fmtstr,*x,*lastf;
                DWORD   *argliststart;

                fmtstr = NULL;
                lastf = f;
                f++;
                if (!*f) {
                    ADD_TO_T('%');
                    continue;
                }
                switch (*f) {
                case '1':case '2':case '3':case '4':case '5':
                case '6':case '7':case '8':case '9':
                    insertnr=*f-'0';
                    switch (f[1]) {
                    case '0':case '1':case '2':case '3':
                    case '4':case '5':case '6':case '7':
                    case '8':case '9':
                        f++;
                        insertnr=insertnr*10+*f-'0';
                        f++;
                        break;
                    default:
                        f++;
                        break;
                    }
                    if (*f=='!') {
                        f++;
                        if (NULL!=(x=strchr(f,'!'))) {
                            *x='\0';
                            fmtstr=HeapAlloc(GetProcessHeap(),0,strlen(f)+2);
                            sprintf(fmtstr,"%%%s",f);
                            f=x+1;
                        } else {
                            fmtstr=HeapAlloc(GetProcessHeap(),0,strlen(f)+2);
                            sprintf(fmtstr,"%%%s",f);
                            f+=strlen(f); /*at \0*/
                        }
                    }
                    else
                    {
                        if(!args) break;
                        fmtstr=HeapAlloc( GetProcessHeap(), 0, 3 );
                        strcpy( fmtstr, "%s" );
                    }
                    if (args) {
                        int     ret;
                        int     sz;
                        LPSTR   b = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sz = 100);

                        argliststart=args+insertnr-1;

                        /* CMF - This makes a BIG assumption about va_list */
                        while ((ret = vsnprintf(b, sz, fmtstr, (va_list) argliststart)) < 0 || ret >= sz) {
                            LPSTR new_b;
                            sz = (ret == -1 ? sz + 100 : ret + 1);
                            new_b = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, b, sz);
                            if (!new_b) break;
                            b = new_b;
                        }
                        for (x=b; *x; x++) ADD_TO_T(*x);
                        HeapFree(GetProcessHeap(), 0, b);
                    } else {
                        /* NULL args - copy formatstr
                         * (probably wrong)
                         */
                        while ((lastf<f)&&(*lastf)) {
                            ADD_TO_T(*lastf++);
                        }
                    }
                    HeapFree(GetProcessHeap(),0,fmtstr);
                    break;
                case '0': /* Just stop processing format string */
                    eos = TRUE;
                    f++;
                    break;
                case 'n': /* 16 bit version just outputs 'n' */
                default:
                    ADD_TO_T(*f++);
                    break;
                }
            } else { /* '\n' or '\r' gets mapped to "\r\n" */
                if(*f == '\n' || *f == '\r') {
                    if (width == 0) {
                        ADD_TO_T('\r');
                        ADD_TO_T('\n');
                        if(*f++ == '\r' && *f == '\n')
                            f++;
                    }
                } else {
                    ADD_TO_T(*f++);
                }
            }
        }
        *t='\0';
    }
    talloced = strlen(target)+1;
    TRACE("-- %s\n",debugstr_a(target));
    if (dwFlags & FORMAT_MESSAGE_ALLOCATE_BUFFER) {
        /* nSize is the MINIMUM size */
        HLOCAL16 h = LocalAlloc16(LPTR,talloced);
        SEGPTR ptr = LocalLock16(h);
        allocstring = MapSL( ptr );
        memcpy( allocstring,target,talloced);
        LocalUnlock16( h );
        *((HLOCAL16*)lpBuffer) = h;
    } else
        lstrcpynA(lpBuffer,target,nSize);
    HeapFree(GetProcessHeap(),0,target);
    HeapFree(GetProcessHeap(),0,from);
    return (dwFlags & FORMAT_MESSAGE_ALLOCATE_BUFFER) ?
        strlen(allocstring):
        strlen(lpBuffer);
}
#undef ADD_TO_T


/**********************************************************************
 *		DestroyIcon32 (USER.610)
 *
 * This routine is actually exported from Win95 USER under the name
 * DestroyIcon32 ...  The behaviour implemented here should mimic
 * the Win95 one exactly, especially the return values, which
 * depend on the setting of various flags.
 */
WORD WINAPI DestroyIcon32( HGLOBAL16 handle, UINT16 flags )
{
    WORD retv;

    /* Check whether destroying active cursor */

    if (GetCursor16() == handle)
    {
        WARN("Destroying active cursor!\n" );
        return FALSE;
    }

    /* Try shared cursor/icon first */

    if (!(flags & CID_NONSHARED))
    {
        INT count = release_shared_icon( handle );
        if (count != -1)
            return (flags & CID_WIN32) ? TRUE : (count == 0);
    }

    /* Now assume non-shared cursor/icon */

    retv = free_icon_handle( handle );
    return (flags & CID_RESOURCE)? retv : TRUE;
}


/***********************************************************************
 *		ChangeDisplaySettings (USER.620)
 */
LONG WINAPI ChangeDisplaySettings16( LPDEVMODEA devmode, DWORD flags )
{
    return ChangeDisplaySettingsA( devmode, flags );
}


/***********************************************************************
 *		EnumDisplaySettings (USER.621)
 */
BOOL16 WINAPI EnumDisplaySettings16( LPCSTR name, DWORD n, LPDEVMODEA devmode )
{
    return EnumDisplaySettingsA( name, n, devmode );
}

/**********************************************************************
 *          DrawFrameControl  (USER.656)
 */
BOOL16 WINAPI DrawFrameControl16( HDC16 hdc, LPRECT16 rc, UINT16 uType, UINT16 uState )
{
    RECT rect32;
    BOOL ret;

    rect32.left   = rc->left;
    rect32.top    = rc->top;
    rect32.right  = rc->right;
    rect32.bottom = rc->bottom;
    ret = DrawFrameControl( HDC_32(hdc), &rect32, uType, uState );
    rc->left   = rect32.left;
    rc->top    = rect32.top;
    rc->right  = rect32.right;
    rc->bottom = rect32.bottom;
    return ret;
}

/**********************************************************************
 *          DrawEdge   (USER.659)
 */
BOOL16 WINAPI DrawEdge16( HDC16 hdc, LPRECT16 rc, UINT16 edge, UINT16 flags )
{
    RECT rect32;
    BOOL ret;

    rect32.left   = rc->left;
    rect32.top    = rc->top;
    rect32.right  = rc->right;
    rect32.bottom = rc->bottom;
    ret = DrawEdge( HDC_32(hdc), &rect32, edge, flags );
    rc->left   = rect32.left;
    rc->top    = rect32.top;
    rc->right  = rect32.right;
    rc->bottom = rect32.bottom;
    return ret;
}

/**********************************************************************
 *		CheckMenuRadioItem (USER.666)
 */
BOOL16 WINAPI CheckMenuRadioItem16(HMENU16 hMenu, UINT16 first, UINT16 last,
                                   UINT16 check, BOOL16 bypos)
{
     return CheckMenuRadioItem( HMENU_32(hMenu), first, last, check, bypos );
}
