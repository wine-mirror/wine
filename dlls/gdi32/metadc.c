/*
 * Metafile DC functions
 *
 * Copyright 1999 Huw D M Davies
 * Copyright 1993, 1994, 1996 Alexandre Julliard
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

#include <stdarg.h>

#include "ntgdi_private.h"
#include "winnls.h"
#include "mfdrv/metafiledrv.h"

#include "wine/debug.h"


WINE_DEFAULT_DEBUG_CHANNEL(metafile);

struct metadc *get_metadc_ptr( HDC hdc )
{
    struct metadc *metafile = get_gdi_client_ptr( hdc, NTGDI_OBJ_METADC );
    if (!metafile) SetLastError( ERROR_INVALID_HANDLE );
    return metafile;
}

BOOL metadc_write_record( struct metadc *metadc, METARECORD *mr, unsigned int rlen )
{
    DWORD len, size;
    METAHEADER *mh;

    len = metadc->mh->mtSize * sizeof(WORD) + rlen;
    size = HeapSize( GetProcessHeap(), 0, metadc->mh );
    if (len > size)
    {
        size += size / sizeof(WORD) + rlen;
        mh = HeapReAlloc( GetProcessHeap(), 0, metadc->mh, size );
        if (!mh) return FALSE;
        metadc->mh = mh;
    }
    memcpy( (WORD *)metadc->mh + metadc->mh->mtSize, mr, rlen );

    metadc->mh->mtSize += rlen / sizeof(WORD);
    metadc->mh->mtMaxRecord = max( metadc->mh->mtMaxRecord, rlen / sizeof(WORD) );
    return TRUE;
}

BOOL metadc_record( HDC hdc, METARECORD *mr, DWORD rlen )
{
    struct metadc *metadc;

    if (!(metadc = get_metadc_ptr( hdc ))) return FALSE;
    return metadc_write_record( metadc, mr, rlen );
}

BOOL metadc_param0( HDC hdc, short func )
{
    METARECORD mr;

    mr.rdSize = FIELD_OFFSET(METARECORD, rdParm[0]) / sizeof(WORD);
    mr.rdFunction = func;
    return metadc_record( hdc, &mr, mr.rdSize * sizeof(WORD) );
}

BOOL metadc_param1( HDC hdc, short func, short param )
{
    METARECORD mr;

    mr.rdSize = sizeof(mr) / sizeof(WORD);
    mr.rdFunction = func;
    mr.rdParm[0] = param;
    return metadc_record( hdc, &mr, mr.rdSize * sizeof(WORD) );
}

BOOL metadc_param2( HDC hdc, short func, short param1, short param2 )
{
    char buffer[FIELD_OFFSET(METARECORD, rdParm[2])];
    METARECORD *mr = (METARECORD *)&buffer;

    mr->rdSize = sizeof(buffer) / sizeof(WORD);
    mr->rdFunction = func;
    mr->rdParm[0] = param2;
    mr->rdParm[1] = param1;
    return metadc_record( hdc, mr, sizeof(buffer) );
}

BOOL metadc_param4( HDC hdc, short func, short param1, short param2,
                    short param3, short param4 )
{
    char buffer[FIELD_OFFSET(METARECORD, rdParm[4])];
    METARECORD *mr = (METARECORD *)&buffer;

    mr->rdSize = sizeof(buffer) / sizeof(WORD);
    mr->rdFunction = func;
    mr->rdParm[0] = param4;
    mr->rdParm[1] = param3;
    mr->rdParm[2] = param2;
    mr->rdParm[3] = param1;
    return metadc_record( hdc, mr, sizeof(buffer) );
}

BOOL metadc_param5( HDC hdc, short func, short param1, short param2,
                    short param3, short param4, short param5 )
{
    char buffer[FIELD_OFFSET(METARECORD, rdParm[5])];
    METARECORD *mr = (METARECORD *)&buffer;

    mr->rdSize = sizeof(buffer) / sizeof(WORD);
    mr->rdFunction = func;
    mr->rdParm[0] = param5;
    mr->rdParm[1] = param4;
    mr->rdParm[2] = param3;
    mr->rdParm[3] = param2;
    mr->rdParm[4] = param1;
    return metadc_record( hdc, mr, sizeof(buffer) );
}

BOOL metadc_param6( HDC hdc, short func, short param1, short param2,
                    short param3, short param4, short param5,
                    short param6 )
{
    char buffer[FIELD_OFFSET(METARECORD, rdParm[6])];
    METARECORD *mr = (METARECORD *)&buffer;

    mr->rdSize = sizeof(buffer) / sizeof(WORD);
    mr->rdFunction = func;
    mr->rdParm[0] = param6;
    mr->rdParm[1] = param5;
    mr->rdParm[2] = param4;
    mr->rdParm[3] = param3;
    mr->rdParm[4] = param2;
    mr->rdParm[5] = param1;
    return metadc_record( hdc, mr, sizeof(buffer) );
}

BOOL metadc_param8( HDC hdc, short func, short param1, short param2,
                    short param3, short param4, short param5,
                    short param6, short param7, short param8)
{
    char buffer[FIELD_OFFSET(METARECORD, rdParm[8])];
    METARECORD *mr = (METARECORD *)&buffer;

    mr->rdSize = sizeof(buffer) / sizeof(WORD);
    mr->rdFunction = func;
    mr->rdParm[0] = param8;
    mr->rdParm[1] = param7;
    mr->rdParm[2] = param6;
    mr->rdParm[3] = param5;
    mr->rdParm[4] = param4;
    mr->rdParm[5] = param3;
    mr->rdParm[6] = param2;
    mr->rdParm[7] = param1;
    return metadc_record( hdc, mr, sizeof(buffer) );
}

BOOL METADC_SaveDC( HDC hdc )
{
    return metadc_param0( hdc, META_SAVEDC );
}

BOOL METADC_RestoreDC( HDC hdc, INT level )
{
    return metadc_param1( hdc, META_RESTOREDC, level );
}

BOOL METADC_SetTextAlign( HDC hdc, UINT align )
{
    return metadc_param2( hdc, META_SETTEXTALIGN, HIWORD(align), LOWORD(align) );
}

BOOL METADC_SetBkMode( HDC hdc, INT mode )
{
    return metadc_param1( hdc, META_SETBKMODE, (WORD)mode );
}

BOOL METADC_SetBkColor( HDC hdc, COLORREF color )
{
    return metadc_param2( hdc, META_SETBKCOLOR, HIWORD(color), LOWORD(color) );
}

BOOL METADC_SetTextColor( HDC hdc, COLORREF color )
{
    return metadc_param2( hdc, META_SETTEXTCOLOR, HIWORD(color), LOWORD(color) );
}

BOOL METADC_SetROP2( HDC hdc, INT rop )
{
    return metadc_param1( hdc, META_SETROP2, (WORD)rop );
}

BOOL METADC_SetRelAbs( HDC hdc, INT mode )
{
    return metadc_param1( hdc, META_SETRELABS, (WORD)mode );
}

BOOL METADC_SetPolyFillMode( HDC hdc, INT mode )
{
    return metadc_param1( hdc, META_SETPOLYFILLMODE, mode );
}

BOOL METADC_SetStretchBltMode( HDC hdc, INT mode )
{
    return metadc_param1( hdc, META_SETSTRETCHBLTMODE, mode );
}

BOOL METADC_IntersectClipRect( HDC hdc, INT left, INT top, INT right, INT bottom )
{
    return metadc_param4( hdc, META_INTERSECTCLIPRECT, left, top, right, bottom );
}

BOOL METADC_ExcludeClipRect( HDC hdc, INT left, INT top, INT right, INT bottom )
{
    return metadc_param4( hdc, META_EXCLUDECLIPRECT, left, top, right, bottom );
}

BOOL METADC_OffsetClipRgn( HDC hdc, INT x, INT y )
{
    return metadc_param2( hdc, META_OFFSETCLIPRGN, x, y );
}

BOOL METADC_SetLayout( HDC hdc, DWORD layout )
{
    return metadc_param2( hdc, META_SETLAYOUT, HIWORD(layout), LOWORD(layout) );
}

BOOL METADC_SetMapMode( HDC hdc, INT mode )
{
    return metadc_param1( hdc, META_SETMAPMODE, mode );
}

BOOL METADC_SetViewportExtEx( HDC hdc, INT x, INT y )
{
    return metadc_param2( hdc, META_SETVIEWPORTEXT, x, y );
}

BOOL METADC_SetViewportOrgEx( HDC hdc, INT x, INT y )
{
    return metadc_param2( hdc, META_SETVIEWPORTORG, x, y );
}

BOOL METADC_SetWindowExtEx( HDC hdc, INT x, INT y )
{
    return metadc_param2( hdc, META_SETWINDOWEXT, x, y );
}

BOOL METADC_SetWindowOrgEx( HDC hdc, INT x, INT y )
{
    return metadc_param2( hdc, META_SETWINDOWORG, x, y );
}

BOOL METADC_OffsetViewportOrgEx( HDC hdc, INT x, INT y )
{
    return metadc_param2( hdc, META_OFFSETVIEWPORTORG, x, y );
}

BOOL METADC_OffsetWindowOrgEx( HDC hdc, INT x, INT y )
{
    return metadc_param2( hdc, META_OFFSETWINDOWORG, x, y );
}

BOOL METADC_ScaleViewportExtEx( HDC hdc, INT x_num, INT x_denom, INT y_num, INT y_denom )
{
    return metadc_param4( hdc, META_SCALEVIEWPORTEXT, x_num, x_denom, y_num, y_denom );
}

BOOL METADC_ScaleWindowExtEx( HDC hdc, INT x_num, INT x_denom, INT y_num, INT y_denom )
{
    return metadc_param4( hdc, META_SCALEWINDOWEXT, x_num, x_denom, y_num, y_denom );
}

BOOL METADC_SetTextJustification( HDC hdc, INT extra, INT breaks )
{
    return metadc_param2( hdc, META_SETTEXTJUSTIFICATION, extra, breaks );
}

BOOL METADC_SetTextCharacterExtra( HDC hdc, INT extra )
{
    return metadc_param1( hdc, META_SETTEXTCHAREXTRA, extra );
}

BOOL METADC_SetMapperFlags( HDC hdc, DWORD flags )
{
    return metadc_param2( hdc, META_SETMAPPERFLAGS, HIWORD(flags), LOWORD(flags) );
}

BOOL METADC_ExtEscape( HDC hdc, INT escape, INT input_size, const void *input,
                       INT output_size, void *output )
{
    METARECORD *mr;
    DWORD len;
    BOOL ret;

    if (output_size) return FALSE;  /* escapes that require output cannot work in metafiles */

    len = sizeof(*mr) + sizeof(WORD) + ((input_size + 1) & ~1);
    if (!(mr = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, len ))) return FALSE;
    mr->rdSize = len / sizeof(WORD);
    mr->rdFunction = META_ESCAPE;
    mr->rdParm[0] = escape;
    mr->rdParm[1] = input_size;
    memcpy( &mr->rdParm[2], input, input_size );
    ret = metadc_record( hdc, mr, len );
    HeapFree(GetProcessHeap(), 0, mr);
    return ret;
}

INT METADC_GetDeviceCaps( HDC hdc, INT cap )
{
    if (!get_metadc_ptr( hdc )) return 0;

    switch(cap)
    {
    case TECHNOLOGY:
        return DT_METAFILE;
    case TEXTCAPS:
        return 0;
    default:
        TRACE(" unsupported capability %d, will return 0\n", cap );
    }
    return 0;
}

static void metadc_free( struct metadc *metadc )
{
    DWORD index;

    CloseHandle( metadc->hFile );
    HeapFree( GetProcessHeap(), 0, metadc->mh );
    for(index = 0; index < metadc->handles_size; index++)
        if(metadc->handles[index])
            GDI_hdc_not_using_object( metadc->handles[index], metadc->hdc );
    HeapFree( GetProcessHeap(), 0, metadc->handles );
    HeapFree( GetProcessHeap(), 0, metadc );
}

BOOL METADC_DeleteDC( HDC hdc )
{
    struct metadc *metadc;

    if (!(metadc = get_metadc_ptr( hdc ))) return FALSE;
    if (!NtGdiDeleteClientObj( hdc )) return FALSE;
    metadc_free( metadc );
    return TRUE;
}

/**********************************************************************
 *           CreateMetaFileW   (GDI32.@)
 *
 *  Create a new DC and associate it with a metafile. Pass a filename
 *  to create a disk-based metafile, NULL to create a memory metafile.
 */
HDC WINAPI CreateMetaFileW( const WCHAR *filename )
{
    struct metadc *metadc;
    HANDLE hdc;

    TRACE( "%s\n", debugstr_w(filename) );

    if (!(hdc = NtGdiCreateClientObj( NTGDI_OBJ_METADC ))) return NULL;

    metadc = HeapAlloc( GetProcessHeap(), 0, sizeof(*metadc) );
    if (!metadc)
    {
        NtGdiDeleteClientObj( hdc );
        return NULL;
    }
    if (!(metadc->mh = HeapAlloc( GetProcessHeap(), 0, sizeof(*metadc->mh) )))
    {
        HeapFree( GetProcessHeap(), 0, metadc );
        NtGdiDeleteClientObj( hdc );
        return NULL;
    }

    metadc->hdc = hdc;
    set_gdi_client_ptr( hdc, metadc );

    metadc->handles = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                                 HANDLE_LIST_INC * sizeof(metadc->handles[0]) );
    metadc->handles_size = HANDLE_LIST_INC;
    metadc->cur_handles = 0;

    metadc->hFile = 0;

    metadc->mh->mtHeaderSize   = sizeof(METAHEADER) / sizeof(WORD);
    metadc->mh->mtVersion      = 0x0300;
    metadc->mh->mtSize         = metadc->mh->mtHeaderSize;
    metadc->mh->mtNoObjects    = 0;
    metadc->mh->mtMaxRecord    = 0;
    metadc->mh->mtNoParameters = 0;
    metadc->mh->mtType         = METAFILE_MEMORY;

    metadc->pen   = GetStockObject( BLACK_PEN );
    metadc->brush = GetStockObject( WHITE_BRUSH );
    metadc->font  = GetStockObject( DEVICE_DEFAULT_FONT );

    if (filename)  /* disk based metafile */
    {
        HANDLE file = CreateFileW( filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0 );
        if (file == INVALID_HANDLE_VALUE)
        {
            HeapFree( GetProcessHeap(), 0, metadc );
            NtGdiDeleteClientObj( hdc );
            return 0;
        }
        metadc->hFile = file;
    }

    TRACE( "returning %p\n", hdc );
    return hdc;
}

/**********************************************************************
 *           CreateMetaFileA   (GDI32.@)
 */
HDC WINAPI CreateMetaFileA( const char *filename )
{
    LPWSTR filenameW;
    DWORD len;
    HDC hdc;

    if (!filename) return CreateMetaFileW( NULL );

    len = MultiByteToWideChar( CP_ACP, 0, filename, -1, NULL, 0 );
    filenameW = HeapAlloc( GetProcessHeap(), 0, len*sizeof(WCHAR) );
    MultiByteToWideChar( CP_ACP, 0, filename, -1, filenameW, len );

    hdc = CreateMetaFileW( filenameW );

    HeapFree( GetProcessHeap(), 0, filenameW );
    return hdc;
}

/******************************************************************
 *           CloseMetaFile   (GDI32.@)
 *
 *  Stop recording graphics operations in metafile associated with
 *  hdc and retrieve metafile.
 */
HMETAFILE WINAPI CloseMetaFile( HDC hdc )
{
    struct metadc *metadc;
    DWORD bytes_written;
    HMETAFILE hmf;

    TRACE( "(%p)\n", hdc );

    if (!(metadc = get_metadc_ptr( hdc ))) return FALSE;

    /* Construct the end of metafile record - this is documented
     * in SDK Knowledgebase Q99334.
     */
    if (!metadc_param0( hdc, META_EOF )) return FALSE;
    if (!NtGdiDeleteClientObj( hdc )) return FALSE;

    if (metadc->hFile && !WriteFile( metadc->hFile, metadc->mh, metadc->mh->mtSize * sizeof(WORD),
                                     &bytes_written, NULL ))
    {
        metadc_free( metadc );
        return FALSE;
    }

    /* Now allocate a global handle for the metafile */
    hmf = MF_Create_HMETAFILE( metadc->mh );
    if (hmf) metadc->mh = NULL;  /* So it won't be deleted */
    metadc_free( metadc );
    return hmf;
}
