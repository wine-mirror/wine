/*
 * SHLWAPI IStream functions
 *
 * Copyright 2002 Jon Griffiths
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

#define COBJMACROS
#define NONAMELESSUNION

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winnls.h"
#define NO_SHLWAPI_REG
#define NO_SHLWAPI_PATH
#include "shlwapi.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

/*************************************************************************
 * @       [SHLWAPI.184]
 *
 * Call IStream_Read() on a stream.
 *
 * PARAMS
 *  lpStream [I] IStream object
 *  lpvDest  [O] Destination for data read
 *  ulSize   [I] Size of data to read
 *
 * RETURNS
 *  Success: S_OK. ulSize bytes have been read from the stream into lpvDest.
 *  Failure: An HRESULT error code, or E_FAIL if the read succeeded but the
 *           number of bytes read does not match.
 */
HRESULT WINAPI SHIStream_Read(IStream *lpStream, LPVOID lpvDest, ULONG ulSize)
{
  ULONG ulRead;
  HRESULT hRet;

  TRACE("(%p,%p,%u)\n", lpStream, lpvDest, ulSize);

  hRet = IStream_Read(lpStream, lpvDest, ulSize, &ulRead);

  if (SUCCEEDED(hRet) && ulRead != ulSize)
    hRet = E_FAIL;
  return hRet;
}

/*************************************************************************
 * @       [SHLWAPI.166]
 *
 * Determine if a stream has 0 length.
 *
 * PARAMS
 *  lpStream [I] IStream object
 *
 * RETURNS
 *  TRUE:  If the stream has 0 length
 *  FALSE: Otherwise.
 */
BOOL WINAPI SHIsEmptyStream(IStream *lpStream)
{
  STATSTG statstg;
  BOOL bRet = TRUE;

  TRACE("(%p)\n", lpStream);

  memset(&statstg, 0, sizeof(statstg));

  if(SUCCEEDED(IStream_Stat(lpStream, &statstg, 1)))
  {
    if(statstg.cbSize.QuadPart)
      bRet = FALSE; /* Non-Zero */
  }
  else
  {
    DWORD dwDummy;

    /* Try to read from the stream */
    if(SUCCEEDED(SHIStream_Read(lpStream, &dwDummy, sizeof(dwDummy))))
    {
      LARGE_INTEGER zero;
      zero.QuadPart = 0;

      IStream_Seek(lpStream, zero, 0, NULL);
      bRet = FALSE; /* Non-Zero */
    }
  }
  return bRet;
}

/*************************************************************************
 * @       [SHLWAPI.212]
 *
 * Call IStream_Write() on a stream.
 *
 * PARAMS
 *  lpStream [I] IStream object
 *  lpvSrc   [I] Source for data to write
 *  ulSize   [I] Size of data
 *
 * RETURNS
 *  Success: S_OK. ulSize bytes have been written to the stream from lpvSrc.
 *  Failure: An HRESULT error code, or E_FAIL if the write succeeded but the
 *           number of bytes written does not match.
 */
HRESULT WINAPI SHIStream_Write(IStream *lpStream, LPCVOID lpvSrc, ULONG ulSize)
{
  ULONG ulWritten;
  HRESULT hRet;

  TRACE("(%p,%p,%d)\n", lpStream, lpvSrc, ulSize);

  hRet = IStream_Write(lpStream, lpvSrc, ulSize, &ulWritten);

  if (SUCCEEDED(hRet) && ulWritten != ulSize)
    hRet = E_FAIL;

  return hRet;
}

/*************************************************************************
 * @       [SHLWAPI.213]
 *
 * Seek to the start of a stream.
 *
 * PARAMS
 *  lpStream [I] IStream object
 *
 * RETURNS
 *  Success: S_OK. The current position within the stream is updated
 *  Failure: An HRESULT error code.
 */
HRESULT WINAPI IStream_Reset(IStream *lpStream)
{
  LARGE_INTEGER zero;
  TRACE("(%p)\n", lpStream);
  zero.QuadPart = 0;
  return IStream_Seek(lpStream, zero, 0, NULL);
}

/*************************************************************************
 * @       [SHLWAPI.214]
 *
 * Get the size of a stream.
 *
 * PARAMS
 *  lpStream [I] IStream object
 *  lpulSize [O] Destination for size
 *
 * RETURNS
 *  Success: S_OK. lpulSize contains the size of the stream.
 *  Failure: An HRESULT error code.
 */
HRESULT WINAPI IStream_Size(IStream *lpStream, ULARGE_INTEGER* lpulSize)
{
  STATSTG statstg;
  HRESULT hRet;

  TRACE("(%p,%p)\n", lpStream, lpulSize);

  memset(&statstg, 0, sizeof(statstg));

  hRet = IStream_Stat(lpStream, &statstg, 1);

  if (SUCCEEDED(hRet) && lpulSize)
    *lpulSize = statstg.cbSize;
  return hRet;
}
