/*
 * Compression API
 *
 * Copyright 2026 Chris Kadar
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

#include "compressapi.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(cabinet);

/***********************************************************************
 *		CreateCompressor (CABINET.30)
 */
BOOL WINAPI CreateCompressor( DWORD algorithm, COMPRESS_ALLOCATION_ROUTINES *routines, COMPRESSOR_HANDLE *handle )
{
    FIXME("algo (%lu) stub\n", algorithm);
    return TRUE;
}

/***********************************************************************
 *		Compress (CABINET.33)
 */
BOOL WINAPI Compress( COMPRESSOR_HANDLE handle, const VOID *data, SIZE_T data_size,
                      VOID *buffer, SIZE_T buffer_size, SIZE_T *compressed_data_size )
{
    FIXME("stub\n");

    *compressed_data_size = data_size;

    if( buffer_size < data_size )
    {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return FALSE;
    }
    memcpy(buffer, data, data_size);
    return TRUE;
}

/***********************************************************************
 *		CloseCompressor  (CABINET.35)
 */
BOOL WINAPI CloseCompressor( COMPRESSOR_HANDLE handle )
{
    FIXME("stub\n");
    return TRUE;
}

/***********************************************************************
 *		CreateDecompressor (CABINET.40)
 */
BOOL WINAPI CreateDecompressor( DWORD algorithm, COMPRESS_ALLOCATION_ROUTINES *routines, DECOMPRESSOR_HANDLE *handle )
{
    FIXME("algo (%lu) stub\n", algorithm);
    return TRUE;
}

/***********************************************************************
 *		Decompress (CABINET.43)
 */
BOOL WINAPI Decompress( DECOMPRESSOR_HANDLE handle, const VOID *data, SIZE_T data_size,
                        VOID *buffer, SIZE_T buffer_size, SIZE_T *decompressed_data_size)
{
    FIXME("stub\n");

    *decompressed_data_size = data_size;

    if( buffer_size < data_size )
    {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return FALSE;
    }
    memcpy(buffer, data, data_size);
    return TRUE;
}

/***********************************************************************
 *		CloseCompressor  (CABINET.45)
 */
BOOL WINAPI CloseDecompressor( DECOMPRESSOR_HANDLE handle )
{
    FIXME("stub\n");
    return TRUE;
}
