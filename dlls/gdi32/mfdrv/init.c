/*
 * Metafile driver initialisation functions
 *
 * Copyright 1996 Alexandre Julliard
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
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "ntgdi_private.h"
#include "mfdrv/metafiledrv.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(metafile);


/**********************************************************************
 *           METADC_ExtEscape
 */
BOOL METADC_ExtEscape( HDC hdc, INT escape, INT input_size, const void *input,
                       INT output_size, void *output )
{
    METARECORD *mr;
    DWORD len;
    BOOL ret;

    if (output_size) return FALSE;  /* escapes that require output cannot work in metafiles */

    len = sizeof(*mr) + sizeof(WORD) + ((input_size + 1) & ~1);
    mr = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len);
    mr->rdSize = len / 2;
    mr->rdFunction = META_ESCAPE;
    mr->rdParm[0] = escape;
    mr->rdParm[1] = input_size;
    memcpy( &mr->rdParm[2], input, input_size );
    ret = metadc_record( hdc, mr, len );
    HeapFree(GetProcessHeap(), 0, mr);
    return ret;
}


/******************************************************************
 *         METADC_GetDeviceCaps
 *
 *A very simple implementation that returns DT_METAFILE
 */
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

/**********************************************************************
 *	     METADC_DeleteObject
 */
BOOL METADC_DeleteDC( HDC hdc )
{
    struct metadc *metadc;

    if (!(metadc = get_metadc_ptr( hdc ))) return FALSE;
    if (!NtGdiDeleteClientObj( hdc )) return FALSE;
    metadc_free( metadc );
    return TRUE;
}


/**********************************************************************
 *	     CreateMetaFileW   (GDI32.@)
 *
 *  Create a new DC and associate it with a metafile. Pass a filename
 *  to create a disk-based metafile, NULL to create a memory metafile.
 *
 * PARAMS
 *  filename [I] Filename of disk metafile
 *
 * RETURNS
 *  A handle to the metafile DC if successful, NULL on failure.
 */
HDC WINAPI CreateMetaFileW( LPCWSTR filename )
{
    struct metadc *metadc;
    HANDLE hdc;

    TRACE("%s\n", debugstr_w(filename) );

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

    TRACE("returning %p\n", hdc);
    return hdc;
}

/**********************************************************************
 *          CreateMetaFileA   (GDI32.@)
 *
 * See CreateMetaFileW.
 */
HDC WINAPI CreateMetaFileA(LPCSTR filename)
{
    LPWSTR filenameW;
    DWORD len;
    HDC hReturnDC;

    if (!filename) return CreateMetaFileW(NULL);

    len = MultiByteToWideChar( CP_ACP, 0, filename, -1, NULL, 0 );
    filenameW = HeapAlloc( GetProcessHeap(), 0, len*sizeof(WCHAR) );
    MultiByteToWideChar( CP_ACP, 0, filename, -1, filenameW, len );

    hReturnDC = CreateMetaFileW(filenameW);

    HeapFree( GetProcessHeap(), 0, filenameW );

    return hReturnDC;
}


/******************************************************************
 *	     CloseMetaFile   (GDI32.@)
 *
 *  Stop recording graphics operations in metafile associated with
 *  hdc and retrieve metafile.
 *
 * PARAMS
 *  hdc [I] Metafile DC to close 
 *
 * RETURNS
 *  Handle of newly created metafile on success, NULL on failure.
 */
HMETAFILE WINAPI CloseMetaFile(HDC hdc)
{
    struct metadc *metadc;
    DWORD bytes_written;
    HMETAFILE hmf;

    TRACE("(%p)\n", hdc );

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
