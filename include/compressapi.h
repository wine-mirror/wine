/*
 * Copyright (C) 2026 Chris Kadar
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

#ifndef __WINE_COMPRESSAPI_H
#define __WINE_COMPRESSAPI_H

#include <windef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**********************************************************************/

DECLARE_HANDLE(COMPRESSOR_HANDLE);
DECLARE_HANDLE(DECOMPRESSOR_HANDLE);

/**********************************************************************/

typedef void* (CALLBACK *PFN_COMPRESS_ALLOCATE)(void *, SIZE_T);
typedef void (CALLBACK *PFN_COMPRESS_FREE)(void *, void *);

typedef struct
{
    PFN_COMPRESS_ALLOCATE Allocate;
    PFN_COMPRESS_FREE Free;
    void *UserContext;
} COMPRESS_ALLOCATION_ROUTINES;

/**********************************************************************/

#define COMPRESS_ALGORITHM_NULL         0
#define COMPRESS_ALGORITHM_INVALID      1
#define COMPRESS_ALGORITHM_MSZIP        2
#define COMPRESS_ALGORITHM_XPRESS       3
#define COMPRESS_ALGORITHM_XPRESS_HUFF  4
#define COMPRESS_ALGORITHM_LZMS         5

/**********************************************************************/

BOOL WINAPI CreateCompressor( DWORD, COMPRESS_ALLOCATION_ROUTINES*, COMPRESSOR_HANDLE* );
BOOL WINAPI Compress( COMPRESSOR_HANDLE, const VOID*, SIZE_T, VOID*, SIZE_T, SIZE_T* );
BOOL WINAPI CloseCompressor( COMPRESSOR_HANDLE );

BOOL WINAPI CreateDecompressor( DWORD, COMPRESS_ALLOCATION_ROUTINES*, DECOMPRESSOR_HANDLE* );
BOOL WINAPI Decompress( DECOMPRESSOR_HANDLE, const VOID*, SIZE_T, VOID*, SIZE_T, SIZE_T* );
BOOL WINAPI CloseDecompressor( DECOMPRESSOR_HANDLE );

#ifdef __cplusplus
}
#endif

#endif /* __WINE_COMPRESSAPI_H */
