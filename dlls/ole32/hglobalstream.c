/*
 * HGLOBAL Stream implementation
 *
 * This file contains the implementation of the stream interface
 * for streams contained supported by an HGLOBAL pointer.
 *
 * Copyright 1999 Francis Beaudet
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
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define COBJMACROS
#define NONAMELESSUNION

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "objbase.h"
#include "ole2.h"
#include "winerror.h"
#include "winternl.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(storage);

struct handle_wrapper
{
    LONG ref;
    HGLOBAL hglobal;
    ULONG size;
    BOOL delete_on_release;
};

static void handle_addref(struct handle_wrapper *handle)
{
    InterlockedIncrement(&handle->ref);
}

static void handle_release(struct handle_wrapper *handle)
{
    ULONG ref = InterlockedDecrement(&handle->ref);

    if (!ref)
    {
        if (handle->delete_on_release) GlobalFree(handle->hglobal);
        HeapFree(GetProcessHeap(), 0, handle);
    }
}

static struct handle_wrapper *handle_create(HGLOBAL hglobal, BOOL delete_on_release)
{
    struct handle_wrapper *handle;

    handle = HeapAlloc(GetProcessHeap(), 0, sizeof(*handle));
    if (!handle) return NULL;

    /* allocate a handle if one is not supplied */
    if (!hglobal) hglobal = GlobalAlloc(GMEM_MOVEABLE | GMEM_NODISCARD | GMEM_SHARE, 0);
    if (!hglobal)
    {
        HeapFree(GetProcessHeap(), 0, handle);
        return NULL;
    }
    handle->ref = 1;
    handle->hglobal = hglobal;
    handle->size = GlobalSize(hglobal);
    handle->delete_on_release = delete_on_release;

    return handle;
}

/****************************************************************************
 * HGLOBALStreamImpl definition.
 *
 * This class implements the IStream interface and represents a stream
 * supported by an HGLOBAL pointer.
 */
typedef struct
{
  IStream IStream_iface;
  LONG ref;

  struct handle_wrapper *handle;

  /* current position of the cursor */
  ULARGE_INTEGER currentPosition;
} HGLOBALStreamImpl;

static inline HGLOBALStreamImpl *impl_from_IStream(IStream *iface)
{
  return CONTAINING_RECORD(iface, HGLOBALStreamImpl, IStream_iface);
}

static const IStreamVtbl HGLOBALStreamImplVtbl;

static HGLOBALStreamImpl *hglobalstream_construct(void)
{
    HGLOBALStreamImpl *This = HeapAlloc(GetProcessHeap(), 0, sizeof(*This));

    if (This)
    {
        This->IStream_iface.lpVtbl = &HGLOBALStreamImplVtbl;
        This->ref = 1;
        This->handle = NULL;
        This->currentPosition.QuadPart = 0;
    }
    return This;
}

static HRESULT WINAPI HGLOBALStreamImpl_QueryInterface(
		  IStream*     iface,
		  REFIID         riid,	      /* [in] */
		  void**         ppvObject)   /* [iid_is][out] */
{
  HGLOBALStreamImpl* This = impl_from_IStream(iface);

  if (ppvObject==0)
    return E_INVALIDARG;

  *ppvObject = 0;

  if (IsEqualIID(&IID_IUnknown, riid) ||
      IsEqualIID(&IID_ISequentialStream, riid) ||
      IsEqualIID(&IID_IStream, riid))
  {
    *ppvObject = &This->IStream_iface;
  }

  if ((*ppvObject)==0)
    return E_NOINTERFACE;

  IStream_AddRef(iface);

  return S_OK;
}

static ULONG WINAPI HGLOBALStreamImpl_AddRef(IStream* iface)
{
  HGLOBALStreamImpl* This = impl_from_IStream(iface);
  return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI HGLOBALStreamImpl_Release(
		IStream* iface)
{
  HGLOBALStreamImpl* This= impl_from_IStream(iface);
  ULONG ref = InterlockedDecrement(&This->ref);

  if (!ref)
  {
    handle_release(This->handle);
    HeapFree(GetProcessHeap(), 0, This);
  }

  return ref;
}

/***
 * This method is part of the ISequentialStream interface.
 *
 * If reads a block of information from the stream at the current
 * position. It then moves the current position at the end of the
 * read block
 *
 * See the documentation of ISequentialStream for more info.
 */
static HRESULT WINAPI HGLOBALStreamImpl_Read(
		  IStream*     iface,
		  void*          pv,        /* [length_is][size_is][out] */
		  ULONG          cb,        /* [in] */
		  ULONG*         pcbRead)   /* [out] */
{
  HGLOBALStreamImpl* This = impl_from_IStream(iface);

  void* supportBuffer;
  ULONG bytesReadBuffer;
  ULONG bytesToReadFromBuffer;

  TRACE("(%p, %p, %d, %p)\n", iface,
	pv, cb, pcbRead);

  /*
   * If the caller is not interested in the number of bytes read,
   * we use another buffer to avoid "if" statements in the code.
   */
  if (pcbRead==0)
    pcbRead = &bytesReadBuffer;

  /*
   * Using the known size of the stream, calculate the number of bytes
   * to read from the block chain
   */
  bytesToReadFromBuffer = min( This->handle->size - This->currentPosition.u.LowPart, cb);

  /*
   * Lock the buffer in position and copy the data.
   */
  supportBuffer = GlobalLock(This->handle->hglobal);
  if (!supportBuffer)
  {
      WARN("read from invalid hglobal %p\n", This->handle->hglobal);
      *pcbRead = 0;
      return S_OK;
  }

  memcpy(pv, (char *) supportBuffer+This->currentPosition.u.LowPart, bytesToReadFromBuffer);

  /*
   * Move the current position to the new position
   */
  This->currentPosition.u.LowPart+=bytesToReadFromBuffer;

  /*
   * Return the number of bytes read.
   */
  *pcbRead = bytesToReadFromBuffer;

  /*
   * Cleanup
   */
  GlobalUnlock(This->handle->hglobal);

  /*
   * Always returns S_OK even if the end of the stream is reached before the
   * buffer is filled
   */

  return S_OK;
}

/***
 * This method is part of the ISequentialStream interface.
 *
 * It writes a block of information to the stream at the current
 * position. It then moves the current position at the end of the
 * written block. If the stream is too small to fit the block,
 * the stream is grown to fit.
 *
 * See the documentation of ISequentialStream for more info.
 */
static HRESULT WINAPI HGLOBALStreamImpl_Write(
	          IStream*     iface,
		  const void*    pv,          /* [size_is][in] */
		  ULONG          cb,          /* [in] */
		  ULONG*         pcbWritten)  /* [out] */
{
  HGLOBALStreamImpl* This = impl_from_IStream(iface);

  void*          supportBuffer;
  ULARGE_INTEGER newSize;
  ULONG          bytesWritten = 0;

  TRACE("(%p, %p, %d, %p)\n", iface, pv, cb, pcbWritten);

  /*
   * If the caller is not interested in the number of bytes written,
   * we use another buffer to avoid "if" statements in the code.
   */
  if (pcbWritten == 0)
    pcbWritten = &bytesWritten;

  if (cb == 0)
    goto out;

  *pcbWritten = 0;

  newSize.u.HighPart = 0;
  newSize.u.LowPart = This->currentPosition.u.LowPart + cb;

  /*
   * Verify if we need to grow the stream
   */
  if (newSize.u.LowPart > This->handle->size)
  {
    /* grow stream */
    HRESULT hr = IStream_SetSize(iface, newSize);
    if (FAILED(hr))
    {
      ERR("IStream_SetSize failed with error 0x%08x\n", hr);
      return hr;
    }
  }

  /*
   * Lock the buffer in position and copy the data.
   */
  supportBuffer = GlobalLock(This->handle->hglobal);
  if (!supportBuffer)
  {
      WARN("write to invalid hglobal %p\n", This->handle->hglobal);
      return S_OK;
  }

  memcpy((char *) supportBuffer+This->currentPosition.u.LowPart, pv, cb);

  /*
   * Move the current position to the new position
   */
  This->currentPosition.u.LowPart+=cb;

  /*
   * Cleanup
   */
  GlobalUnlock(This->handle->hglobal);

out:
  /*
   * Return the number of bytes read.
   */
  *pcbWritten = cb;

  return S_OK;
}

/***
 * This method is part of the IStream interface.
 *
 * It will move the current stream pointer according to the parameters
 * given.
 *
 * See the documentation of IStream for more info.
 */
static HRESULT WINAPI HGLOBALStreamImpl_Seek(
		  IStream*      iface,
		  LARGE_INTEGER   dlibMove,         /* [in] */
		  DWORD           dwOrigin,         /* [in] */
		  ULARGE_INTEGER* plibNewPosition) /* [out] */
{
  HGLOBALStreamImpl* This = impl_from_IStream(iface);

  ULARGE_INTEGER newPosition = This->currentPosition;
  HRESULT hr = S_OK;

  TRACE("(%p, %x%08x, %d, %p)\n", iface, dlibMove.u.HighPart,
	dlibMove.u.LowPart, dwOrigin, plibNewPosition);

  /*
   * The file pointer is moved depending on the given "function"
   * parameter.
   */
  switch (dwOrigin)
  {
    case STREAM_SEEK_SET:
      newPosition.u.HighPart = 0;
      newPosition.u.LowPart = 0;
      break;
    case STREAM_SEEK_CUR:
      break;
    case STREAM_SEEK_END:
      newPosition.QuadPart = This->handle->size;
      break;
    default:
      hr = STG_E_SEEKERROR;
      goto end;
  }

  /*
   * Move the actual file pointer
   * If the file pointer ends-up after the end of the stream, the next Write operation will
   * make the file larger. This is how it is documented.
   */
  newPosition.u.HighPart = 0;
  newPosition.u.LowPart += dlibMove.QuadPart;

  if (dlibMove.u.LowPart >= 0x80000000 &&
      newPosition.u.LowPart >= dlibMove.u.LowPart)
  {
    /* We tried to seek backwards and went past the start. */
    hr = STG_E_SEEKERROR;
    goto end;
  }

  This->currentPosition = newPosition;

end:
  if (plibNewPosition) *plibNewPosition = This->currentPosition;

  return hr;
}

/***
 * This method is part of the IStream interface.
 *
 * It will change the size of a stream.
 *
 * TODO: Switch from small blocks to big blocks and vice versa.
 *
 * See the documentation of IStream for more info.
 */
static HRESULT WINAPI HGLOBALStreamImpl_SetSize(
				     IStream*      iface,
				     ULARGE_INTEGER  libNewSize)   /* [in] */
{
  HGLOBALStreamImpl* This = impl_from_IStream(iface);
  HGLOBAL supportHandle;

  TRACE("(%p, %d)\n", iface, libNewSize.u.LowPart);

  /*
   * HighPart is ignored as shown in tests
   */

  if (This->handle->size == libNewSize.u.LowPart)
    return S_OK;

  /*
   * Re allocate the HGlobal to fit the new size of the stream.
   */
  supportHandle = GlobalReAlloc(This->handle->hglobal, libNewSize.u.LowPart, GMEM_MOVEABLE);

  if (supportHandle == 0)
    return E_OUTOFMEMORY;

  This->handle->hglobal = supportHandle;
  This->handle->size = libNewSize.u.LowPart;

  return S_OK;
}

/***
 * This method is part of the IStream interface.
 *
 * It will copy the 'cb' Bytes to 'pstm' IStream.
 *
 * See the documentation of IStream for more info.
 */
static HRESULT WINAPI HGLOBALStreamImpl_CopyTo(
				    IStream*      iface,
				    IStream*      pstm,         /* [unique][in] */
				    ULARGE_INTEGER  cb,           /* [in] */
				    ULARGE_INTEGER* pcbRead,      /* [out] */
				    ULARGE_INTEGER* pcbWritten)   /* [out] */
{
  HRESULT        hr = S_OK;
  BYTE           tmpBuffer[128];
  ULONG          bytesRead, bytesWritten, copySize;
  ULARGE_INTEGER totalBytesRead;
  ULARGE_INTEGER totalBytesWritten;

  TRACE("(%p, %p, %d, %p, %p)\n", iface, pstm,
	cb.u.LowPart, pcbRead, pcbWritten);

  if ( pstm == 0 )
    return STG_E_INVALIDPOINTER;

  totalBytesRead.QuadPart = 0;
  totalBytesWritten.QuadPart = 0;

  while ( cb.QuadPart > 0 )
  {
    if ( cb.QuadPart >= sizeof(tmpBuffer) )
      copySize = sizeof(tmpBuffer);
    else
      copySize = cb.u.LowPart;

    hr = IStream_Read(iface, tmpBuffer, copySize, &bytesRead);
    if (FAILED(hr))
        break;

    totalBytesRead.QuadPart += bytesRead;

    if (bytesRead)
    {
        hr = IStream_Write(pstm, tmpBuffer, bytesRead, &bytesWritten);
        if (FAILED(hr))
            break;

        totalBytesWritten.QuadPart += bytesWritten;
    }

    if (bytesRead!=copySize)
      cb.QuadPart = 0;
    else
      cb.QuadPart -= bytesRead;
  }

  if (pcbRead) pcbRead->QuadPart = totalBytesRead.QuadPart;
  if (pcbWritten) pcbWritten->QuadPart = totalBytesWritten.QuadPart;

  return hr;
}

/***
 * This method is part of the IStream interface.
 *
 * For streams supported by HGLOBALS, this function does nothing.
 * This is what the documentation tells us.
 *
 * See the documentation of IStream for more info.
 */
static HRESULT WINAPI HGLOBALStreamImpl_Commit(
		  IStream*      iface,
		  DWORD         grfCommitFlags)  /* [in] */
{
  return S_OK;
}

/***
 * This method is part of the IStream interface.
 *
 * For streams supported by HGLOBALS, this function does nothing.
 * This is what the documentation tells us.
 *
 * See the documentation of IStream for more info.
 */
static HRESULT WINAPI HGLOBALStreamImpl_Revert(
		  IStream* iface)
{
  return S_OK;
}

/***
 * This method is part of the IStream interface.
 *
 * For streams supported by HGLOBALS, this function does nothing.
 * This is what the documentation tells us.
 *
 * See the documentation of IStream for more info.
 */
static HRESULT WINAPI HGLOBALStreamImpl_LockRegion(
		  IStream*       iface,
		  ULARGE_INTEGER libOffset,   /* [in] */
		  ULARGE_INTEGER cb,          /* [in] */
		  DWORD          dwLockType)  /* [in] */
{
  return STG_E_INVALIDFUNCTION;
}

/*
 * This method is part of the IStream interface.
 *
 * For streams supported by HGLOBALS, this function does nothing.
 * This is what the documentation tells us.
 *
 * See the documentation of IStream for more info.
 */
static HRESULT WINAPI HGLOBALStreamImpl_UnlockRegion(
		  IStream*       iface,
		  ULARGE_INTEGER libOffset,   /* [in] */
		  ULARGE_INTEGER cb,          /* [in] */
		  DWORD          dwLockType)  /* [in] */
{
  return S_OK;
}

/***
 * This method is part of the IStream interface.
 *
 * This method returns information about the current
 * stream.
 *
 * See the documentation of IStream for more info.
 */
static HRESULT WINAPI HGLOBALStreamImpl_Stat(
		  IStream*     iface,
		  STATSTG*     pstatstg,     /* [out] */
		  DWORD        grfStatFlag)  /* [in] */
{
  HGLOBALStreamImpl* This = impl_from_IStream(iface);

  memset(pstatstg, 0, sizeof(STATSTG));

  pstatstg->pwcsName = NULL;
  pstatstg->type     = STGTY_STREAM;
  pstatstg->cbSize.QuadPart = This->handle->size;

  return S_OK;
}

static HRESULT WINAPI HGLOBALStreamImpl_Clone(
		  IStream*     iface,
		  IStream**    ppstm) /* [out] */
{
  HGLOBALStreamImpl* This = impl_from_IStream(iface), *clone;
  ULARGE_INTEGER dummy;
  LARGE_INTEGER offset;

  TRACE(" Cloning %p (deleteOnRelease=%d seek position=%ld)\n",iface,This->handle->delete_on_release,(long)This->currentPosition.QuadPart);

  *ppstm = NULL;

  clone = hglobalstream_construct();
  if (!clone) return E_OUTOFMEMORY;

  *ppstm = &clone->IStream_iface;
  handle_addref(This->handle);
  clone->handle = This->handle;

  offset.QuadPart = (LONGLONG)This->currentPosition.QuadPart;
  IStream_Seek(*ppstm, offset, STREAM_SEEK_SET, &dummy);
  return S_OK;
}

static const IStreamVtbl HGLOBALStreamImplVtbl =
{
    HGLOBALStreamImpl_QueryInterface,
    HGLOBALStreamImpl_AddRef,
    HGLOBALStreamImpl_Release,
    HGLOBALStreamImpl_Read,
    HGLOBALStreamImpl_Write,
    HGLOBALStreamImpl_Seek,
    HGLOBALStreamImpl_SetSize,
    HGLOBALStreamImpl_CopyTo,
    HGLOBALStreamImpl_Commit,
    HGLOBALStreamImpl_Revert,
    HGLOBALStreamImpl_LockRegion,
    HGLOBALStreamImpl_UnlockRegion,
    HGLOBALStreamImpl_Stat,
    HGLOBALStreamImpl_Clone
};

/***********************************************************************
 *           CreateStreamOnHGlobal     [OLE32.@]
 */
HRESULT WINAPI CreateStreamOnHGlobal(
		HGLOBAL   hGlobal,
		BOOL      fDeleteOnRelease,
		LPSTREAM* ppstm)
{
  HGLOBALStreamImpl* This;

  if (!ppstm)
    return E_INVALIDARG;

  This = hglobalstream_construct();
  if (!This) return E_OUTOFMEMORY;

  This->handle = handle_create(hGlobal, fDeleteOnRelease);
  if (!This->handle)
  {
      HeapFree(GetProcessHeap(), 0, This);
      return E_OUTOFMEMORY;
  }

  *ppstm = &This->IStream_iface;

  return S_OK;
}

/***********************************************************************
 *           GetHGlobalFromStream     [OLE32.@]
 */
HRESULT WINAPI GetHGlobalFromStream(IStream* pstm, HGLOBAL* phglobal)
{
  HGLOBALStreamImpl* pStream;

  if (!pstm || !phglobal)
    return E_INVALIDARG;

  pStream = impl_from_IStream(pstm);

  /*
   * Verify that the stream object was created with CreateStreamOnHGlobal.
   */
  if (pStream->IStream_iface.lpVtbl == &HGLOBALStreamImplVtbl)
    *phglobal = pStream->handle->hglobal;
  else
  {
    *phglobal = 0;
    return E_INVALIDARG;
  }

  return S_OK;
}
