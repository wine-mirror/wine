/******************************************************************************
 *
 * BigBlockFile
 *
 * This is the implementation of a file that consists of blocks of
 * a predetermined size.
 * This class is used in the Compound File implementation of the
 * IStorage and IStream interfaces. It provides the functionality
 * to read and write any blocks in the file as well as setting and
 * obtaining the size of the file.
 * The blocks are indexed sequentially from the start of the file
 * starting with -1.
 *
 * TODO:
 * - Support for a transacted mode
 *
 * Copyright 1999 Thuy Nguyen
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
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#define COBJMACROS
#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winerror.h"
#include "objbase.h"
#include "ole2.h"

#include "storage32.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(storage);

/***********************************************************
 * Data structures used internally by the BigBlockFile
 * class.
 */

struct BigBlockFile
{
    BOOL fileBased;
    ULARGE_INTEGER filesize;
    HANDLE hfile;
    DWORD flProtect;
    ILockBytes *pLkbyt;
};

/***********************************************************
 * Prototypes for private methods
 */

/* Note that this evaluates a and b multiple times, so don't
 * pass expressions with side effects. */
#define ROUND_UP(a, b) ((((a) + (b) - 1)/(b))*(b))

/******************************************************************************
 *      BIGBLOCKFILE_FileInit
 *
 * Initialize a big block object supported by a file.
 */
static BOOL BIGBLOCKFILE_FileInit(LPBIGBLOCKFILE This, HANDLE hFile)
{
  This->pLkbyt = NULL;
  This->hfile = hFile;

  if (This->hfile == INVALID_HANDLE_VALUE)
    return FALSE;

  This->filesize.u.LowPart = GetFileSize(This->hfile,
					 &This->filesize.u.HighPart);

  TRACE("file len %u\n", This->filesize.u.LowPart);

  return TRUE;
}

/******************************************************************************
 *      BIGBLOCKFILE_LockBytesInit
 *
 * Initialize a big block object supported by an ILockBytes.
 */
static BOOL BIGBLOCKFILE_LockBytesInit(LPBIGBLOCKFILE This, ILockBytes* plkbyt)
{
    This->hfile    = 0;
    This->pLkbyt   = plkbyt;
    ILockBytes_AddRef(This->pLkbyt);

    /* We'll get the size directly with ILockBytes_Stat */
    This->filesize.QuadPart = 0;

    TRACE("ILockBytes %p\n", This->pLkbyt);
    return TRUE;
}

/****************************************************************************
 *      BIGBLOCKFILE_GetProtectMode
 *
 * This function will return a protection mode flag for a file-mapping object
 * from the open flags of a file.
 */
static DWORD BIGBLOCKFILE_GetProtectMode(DWORD openFlags)
{
    switch(STGM_ACCESS_MODE(openFlags))
    {
    case STGM_WRITE:
    case STGM_READWRITE:
        return PAGE_READWRITE;
    }
    return PAGE_READONLY;
}


/* ILockByte Interfaces */

/******************************************************************************
 * This method is part of the ILockBytes interface.
 *
 * It reads a block of information from the byte array at the specified
 * offset.
 *
 * See the documentation of ILockBytes for more info.
 */
static HRESULT ImplBIGBLOCKFILE_ReadAt(
      BigBlockFile* const This,
      ULARGE_INTEGER ulOffset,  /* [in] */
      void*          pv,        /* [length_is][size_is][out] */
      ULONG          cb,        /* [in] */
      ULONG*         pcbRead)   /* [out] */
{
    ULONG bytes_left = cb;
    LPBYTE readPtr = pv;
    BOOL ret;
    LARGE_INTEGER offset;
    ULONG cbRead;

    TRACE("(%p)-> %i %p %i %p\n",This, ulOffset.u.LowPart, pv, cb, pcbRead);

    /* verify a sane environment */
    if (!This) return E_FAIL;

    if (pcbRead)
        *pcbRead = 0;

    offset.QuadPart = ulOffset.QuadPart;

    ret = SetFilePointerEx(This->hfile, offset, NULL, FILE_BEGIN);

    if (!ret)
        return STG_E_READFAULT;

    while (bytes_left)
    {
        ret = ReadFile(This->hfile, readPtr, bytes_left, &cbRead, NULL);

        if (!ret || cbRead == 0)
            return STG_E_READFAULT;

        if (pcbRead)
            *pcbRead += cbRead;

        bytes_left -= cbRead;
        readPtr += cbRead;
    }

    TRACE("finished\n");
    return S_OK;
}

/******************************************************************************
 * This method is part of the ILockBytes interface.
 *
 * It writes the specified bytes at the specified offset.
 * position. If the file is too small, it will be resized.
 *
 * See the documentation of ILockBytes for more info.
 */
static HRESULT ImplBIGBLOCKFILE_WriteAt(
      BigBlockFile* const This,
      ULARGE_INTEGER ulOffset,    /* [in] */
      const void*    pv,          /* [size_is][in] */
      ULONG          cb,          /* [in] */
      ULONG*         pcbWritten)  /* [out] */
{
    ULONG size_needed = ulOffset.u.LowPart + cb;
    ULONG bytes_left = cb;
    const BYTE *writePtr = pv;
    BOOL ret;
    LARGE_INTEGER offset;
    ULONG cbWritten;

    TRACE("(%p)-> %i %p %i %p\n",This, ulOffset.u.LowPart, pv, cb, pcbWritten);

    /* verify a sane environment */
    if (!This) return E_FAIL;

    if (This->flProtect != PAGE_READWRITE)
        return STG_E_ACCESSDENIED;

    if (pcbWritten)
        *pcbWritten = 0;

    if (size_needed > This->filesize.u.LowPart)
    {
        ULARGE_INTEGER newSize;
        newSize.u.HighPart = 0;
        newSize.u.LowPart = size_needed;
        BIGBLOCKFILE_SetSize(This, newSize);
    }

    offset.QuadPart = ulOffset.QuadPart;

    ret = SetFilePointerEx(This->hfile, offset, NULL, FILE_BEGIN);

    if (!ret)
        return STG_E_READFAULT;

    while (bytes_left)
    {
        ret = WriteFile(This->hfile, writePtr, bytes_left, &cbWritten, NULL);

        if (!ret)
            return STG_E_READFAULT;

        if (pcbWritten)
            *pcbWritten += cbWritten;

        bytes_left -= cbWritten;
        writePtr += cbWritten;
    }

    TRACE("finished\n");
    return S_OK;
}

/******************************************************************************
 *      BIGBLOCKFILE_Construct
 *
 * Construct a big block file. Create the file mapping object.
 * Create the read only mapped pages list, the writable mapped page list
 * and the blocks in use list.
 */
BigBlockFile *BIGBLOCKFILE_Construct(HANDLE hFile, ILockBytes* pLkByt, DWORD openFlags,
                                     BOOL fileBased)
{
    BigBlockFile *This;

    This = HeapAlloc(GetProcessHeap(), 0, sizeof(BigBlockFile));

    if (This == NULL)
        return NULL;

    This->fileBased = fileBased;
    This->flProtect = BIGBLOCKFILE_GetProtectMode(openFlags);

    if (This->fileBased)
    {
        if (!BIGBLOCKFILE_FileInit(This, hFile))
        {
            HeapFree(GetProcessHeap(), 0, This);
            return NULL;
        }
    }
    else
    {
        if (!BIGBLOCKFILE_LockBytesInit(This, pLkByt))
        {
            HeapFree(GetProcessHeap(), 0, This);
            return NULL;
        }
    }

    return This;
}

/******************************************************************************
 *      BIGBLOCKFILE_Destructor
 *
 * Destructor. Clean up, free memory.
 */
void BIGBLOCKFILE_Destructor(BigBlockFile *This)
{
    if (This->fileBased)
    {
        CloseHandle(This->hfile);
    }
    else
    {
        ILockBytes_Release(This->pLkbyt);
    }

    HeapFree(GetProcessHeap(), 0, This);
}

/******************************************************************************
 *      BIGBLOCKFILE_ReadAt
 */
HRESULT BIGBLOCKFILE_ReadAt(BigBlockFile *This, ULARGE_INTEGER offset,
                            void* buffer, ULONG size, ULONG* bytesRead)
{
    if (This->fileBased)
        return ImplBIGBLOCKFILE_ReadAt(This,offset,buffer,size,bytesRead);
    else
        return ILockBytes_ReadAt(This->pLkbyt,offset,buffer,size,bytesRead);
}

/******************************************************************************
 *      BIGBLOCKFILE_WriteAt
 */
HRESULT BIGBLOCKFILE_WriteAt(BigBlockFile *This, ULARGE_INTEGER offset,
                             const void* buffer, ULONG size, ULONG* bytesRead)
{
    if (This->fileBased)
        return ImplBIGBLOCKFILE_WriteAt(This,offset,buffer,size,bytesRead);
    else
        return ILockBytes_WriteAt(This->pLkbyt,offset,buffer,size,bytesRead);
}

/******************************************************************************
 *      BIGBLOCKFILE_SetSize
 *
 * Sets the size of the file.
 *
 */
HRESULT BIGBLOCKFILE_SetSize(BigBlockFile *This, ULARGE_INTEGER newSize)
{
    HRESULT hr = S_OK;
    LARGE_INTEGER newpos;

    if (!This->fileBased)
        return ILockBytes_SetSize(This->pLkbyt, newSize);

    if (This->filesize.u.LowPart == newSize.u.LowPart)
        return hr;

    TRACE("from %u to %u\n", This->filesize.u.LowPart, newSize.u.LowPart);

    newpos.QuadPart = newSize.QuadPart;
    if (SetFilePointerEx(This->hfile, newpos, NULL, FILE_BEGIN))
    {
        SetEndOfFile(This->hfile);
    }

    This->filesize = newSize;
    return hr;
}

/******************************************************************************
 *      BIGBLOCKFILE_GetSize
 *
 * Gets the size of the file.
 *
 */
static HRESULT BIGBLOCKFILE_GetSize(BigBlockFile *This, ULARGE_INTEGER *size)
{
    HRESULT hr = S_OK;
    if(This->fileBased)
        *size = This->filesize;
    else
    {
        STATSTG stat;
        hr = ILockBytes_Stat(This->pLkbyt, &stat, STATFLAG_NONAME);
        if(SUCCEEDED(hr)) *size = stat.cbSize;
    }
    return hr;
}

/******************************************************************************
 *      BIGBLOCKFILE_Expand
 *
 * Grows the file to the specified size if necessary.
 */
HRESULT BIGBLOCKFILE_Expand(BigBlockFile *This, ULARGE_INTEGER newSize)
{
    ULARGE_INTEGER size;
    HRESULT hr;

    hr = BIGBLOCKFILE_GetSize(This, &size);
    if(FAILED(hr)) return hr;

    if (newSize.QuadPart > size.QuadPart)
        hr = BIGBLOCKFILE_SetSize(This, newSize);
    return hr;
}
