/*
 * Marshaling Routines
 *
 * Copyright 2005 Robert Shearman
 * Copyright 2009 Huw Davies
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

#define COBJMACROS
#define NONAMELESSUNION

#include "ole2.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

#define ALIGNED_LENGTH(_Len, _Align) (((_Len)+(_Align))&~(_Align))
#define ALIGNED_POINTER(_Ptr, _Align) ((LPVOID)ALIGNED_LENGTH((ULONG_PTR)(_Ptr), _Align))
#define ALIGN_LENGTH(_Len, _Align) _Len = ALIGNED_LENGTH(_Len, _Align)
#define ALIGN_POINTER(_Ptr, _Align) _Ptr = ALIGNED_POINTER(_Ptr, _Align)

static const char* debugstr_user_flags(ULONG *pFlags)
{
    char buf[12];
    const char* loword;
    switch (LOWORD(*pFlags))
    {
    case MSHCTX_LOCAL:
        loword = "MSHCTX_LOCAL";
        break;
    case MSHCTX_NOSHAREDMEM:
        loword = "MSHCTX_NOSHAREDMEM";
        break;
    case MSHCTX_DIFFERENTMACHINE:
        loword = "MSHCTX_DIFFERENTMACHINE";
        break;
    case MSHCTX_INPROC:
        loword = "MSHCTX_INPROC";
        break;
    default:
        sprintf(buf, "%d", LOWORD(*pFlags));
        loword=buf;
    }

    if (HIWORD(*pFlags) == NDR_LOCAL_DATA_REPRESENTATION)
        return wine_dbg_sprintf("MAKELONG(%s, NDR_LOCAL_DATA_REPRESENTATION)", loword);
    else
        return wine_dbg_sprintf("MAKELONG(%s, 0x%04x)", loword, HIWORD(*pFlags));
}

static ULONG handle_UserSize(ULONG *pFlags, ULONG StartingSize, HANDLE *handle)
{
    if (LOWORD(*pFlags) == MSHCTX_DIFFERENTMACHINE)
    {
        ERR("can't remote a local handle\n");
        RaiseException(RPC_S_INVALID_TAG, 0, 0, NULL);
        return StartingSize;
    }

    ALIGN_LENGTH(StartingSize, 3);
    return StartingSize + sizeof(RemotableHandle);
}

static unsigned char * handle_UserMarshal(ULONG *pFlags, unsigned char *pBuffer, HANDLE *handle)
{
    RemotableHandle *remhandle;
    if (LOWORD(*pFlags) == MSHCTX_DIFFERENTMACHINE)
    {
        ERR("can't remote a local handle\n");
        RaiseException(RPC_S_INVALID_TAG, 0, 0, NULL);
        return pBuffer;
    }

    ALIGN_POINTER(pBuffer, 3);
    remhandle = (RemotableHandle *)pBuffer;
    remhandle->fContext = WDT_INPROC_CALL;
    remhandle->u.hInproc = (LONG_PTR)*handle;
    return pBuffer + sizeof(RemotableHandle);
}

static unsigned char * handle_UserUnmarshal(ULONG *pFlags, unsigned char *pBuffer, HANDLE *handle)
{
    RemotableHandle *remhandle;

    ALIGN_POINTER(pBuffer, 3);
    remhandle = (RemotableHandle *)pBuffer;
    if (remhandle->fContext != WDT_INPROC_CALL)
        RaiseException(RPC_X_BAD_STUB_DATA, 0, 0, NULL);
    *handle = (HANDLE)(LONG_PTR)remhandle->u.hInproc;
    return pBuffer + sizeof(RemotableHandle);
}

static void handle_UserFree(ULONG *pFlags, HANDLE *handle)
{
    /* nothing to do */
}

#define IMPL_WIREM_HANDLE(type) \
    ULONG __RPC_USER type##_UserSize(ULONG *pFlags, ULONG StartingSize, type *handle) \
    { \
        TRACE("(%s, %d, %p\n", debugstr_user_flags(pFlags), StartingSize, handle); \
        return handle_UserSize(pFlags, StartingSize, (HANDLE *)handle); \
    } \
    \
    unsigned char * __RPC_USER type##_UserMarshal(ULONG *pFlags, unsigned char *pBuffer, type *handle) \
    { \
        TRACE("(%s, %p, &%p\n", debugstr_user_flags(pFlags), pBuffer, *handle); \
        return handle_UserMarshal(pFlags, pBuffer, (HANDLE *)handle); \
    } \
    \
    unsigned char * __RPC_USER type##_UserUnmarshal(ULONG *pFlags, unsigned char *pBuffer, type *handle) \
    { \
        TRACE("(%s, %p, %p\n", debugstr_user_flags(pFlags), pBuffer, handle); \
        return handle_UserUnmarshal(pFlags, pBuffer, (HANDLE *)handle); \
    } \
    \
    void __RPC_USER type##_UserFree(ULONG *pFlags, type *handle) \
    { \
        TRACE("(%s, &%p\n", debugstr_user_flags(pFlags), *handle); \
        handle_UserFree(pFlags, (HANDLE *)handle); \
    }

IMPL_WIREM_HANDLE(HDC)
IMPL_WIREM_HANDLE(HICON)
IMPL_WIREM_HANDLE(HMENU)
IMPL_WIREM_HANDLE(HWND)

/******************************************************************************
 *           WdtpInterfacePointer_UserSize (combase.@)
 *
 * Calculates the buffer size required to marshal an interface pointer.
 *
 * PARAMS
 *  pFlags       [I] Flags. See notes.
 *  RealFlags    [I] The MSHCTX to use when marshaling the interface.
 *  punk         [I] Interface pointer to size.
 *  StartingSize [I] Starting size of the buffer. This value is added on to
 *                   the buffer size required for the clip format.
 *  riid         [I] ID of interface to size.
 *
 * RETURNS
 *  The buffer size required to marshal an interface pointer plus the starting size.
 *
 * NOTES
 *  Even though the function is documented to take a pointer to a ULONG in
 *  pFlags, it actually takes a pointer to a USER_MARSHAL_CB structure, of which
 *  the first parameter is a ULONG.
 */
ULONG __RPC_USER WdtpInterfacePointer_UserSize(ULONG *pFlags, ULONG RealFlags, ULONG StartingSize, IUnknown *punk, REFIID riid)
{
    DWORD marshal_size = 0;
    HRESULT hr;

    TRACE("%s, %#x, %u, %p, %s.\n", debugstr_user_flags(pFlags), RealFlags, StartingSize, punk, debugstr_guid(riid));

    hr = CoGetMarshalSizeMax(&marshal_size, riid, punk, LOWORD(RealFlags), NULL, MSHLFLAGS_NORMAL);
    if (FAILED(hr)) return StartingSize;

    ALIGN_LENGTH(StartingSize, 3);
    StartingSize += 2 * sizeof(DWORD);
    return StartingSize + marshal_size;
}

/******************************************************************************
 *           WdtpInterfacePointer_UserMarshal (combase.@)
 *
 * Marshals an interface pointer into a buffer.
 *
 * PARAMS
 *  pFlags    [I] Flags. See notes.
 *  RealFlags [I] The MSHCTX to use when marshaling the interface.
 *  pBuffer   [I] Buffer to marshal the clip format into.
 *  punk      [I] Interface pointer to marshal.
 *  riid      [I] ID of interface to marshal.
 *
 * RETURNS
 *  The end of the marshaled data in the buffer.
 *
 * NOTES
 *  Even though the function is documented to take a pointer to a ULONG in
 *  pFlags, it actually takes a pointer to a USER_MARSHAL_CB structure, of which
 *  the first parameter is a ULONG.
 */
unsigned char * WINAPI WdtpInterfacePointer_UserMarshal(ULONG *pFlags, ULONG RealFlags, unsigned char *pBuffer, IUnknown *punk, REFIID riid)
{
    HGLOBAL h = GlobalAlloc(GMEM_MOVEABLE, 0);
    IStream *stm;
    DWORD size;
    void *ptr;

    TRACE("%s, %#x, %p, &%p, %s.\n", debugstr_user_flags(pFlags), RealFlags, pBuffer, punk, debugstr_guid(riid));

    if (!h) return NULL;
    if (CreateStreamOnHGlobal(h, TRUE, &stm) != S_OK)
    {
        GlobalFree(h);
        return NULL;
    }

    if (CoMarshalInterface(stm, riid, punk, LOWORD(RealFlags), NULL, MSHLFLAGS_NORMAL) != S_OK)
    {
        IStream_Release(stm);
        return pBuffer;
    }

    ALIGN_POINTER(pBuffer, 3);
    size = GlobalSize(h);

    *(DWORD *)pBuffer = size;
    pBuffer += sizeof(DWORD);
    *(DWORD *)pBuffer = size;
    pBuffer += sizeof(DWORD);

    ptr = GlobalLock(h);
    memcpy(pBuffer, ptr, size);
    GlobalUnlock(h);

    IStream_Release(stm);
    return pBuffer + size;
}

/******************************************************************************
 *           WdtpInterfacePointer_UserUnmarshal (combase.@)
 *
 * Unmarshals an interface pointer from a buffer.
 *
 * PARAMS
 *  pFlags   [I] Flags. See notes.
 *  pBuffer  [I] Buffer to marshal the clip format from.
 *  ppunk    [I/O] Address that receives the unmarshaled interface pointer.
 *  riid     [I] ID of interface to unmarshal.
 *
 * RETURNS
 *  The end of the marshaled data in the buffer.
 *
 * NOTES
 *  Even though the function is documented to take a pointer to an ULONG in
 *  pFlags, it actually takes a pointer to a USER_MARSHAL_CB structure, of which
 *  the first parameter is an ULONG.
 */
unsigned char * WINAPI WdtpInterfacePointer_UserUnmarshal(ULONG *pFlags, unsigned char *pBuffer, IUnknown **ppunk, REFIID riid)
{
    HRESULT hr;
    HGLOBAL h;
    IStream *stm;
    DWORD size;
    void *ptr;
    IUnknown *orig;

    TRACE("%s, %p, %p, %s.\n", debugstr_user_flags(pFlags), pBuffer, ppunk, debugstr_guid(riid));

    ALIGN_POINTER(pBuffer, 3);

    size = *(DWORD *)pBuffer;
    pBuffer += sizeof(DWORD);
    if (size != *(DWORD *)pBuffer)
        RaiseException(RPC_X_BAD_STUB_DATA, 0, 0, NULL);

    pBuffer += sizeof(DWORD);

    /* FIXME: sanity check on size */

    h = GlobalAlloc(GMEM_MOVEABLE, size);
    if (!h) RaiseException(RPC_X_NO_MEMORY, 0, 0, NULL);

    if (CreateStreamOnHGlobal(h, TRUE, &stm) != S_OK)
    {
        GlobalFree(h);
        RaiseException(RPC_X_NO_MEMORY, 0, 0, NULL);
    }

    ptr = GlobalLock(h);
    memcpy(ptr, pBuffer, size);
    GlobalUnlock(h);

    orig = *ppunk;
    hr = CoUnmarshalInterface(stm, riid, (void**)ppunk);
    IStream_Release(stm);

    if (hr != S_OK) RaiseException(hr, 0, 0, NULL);

    if (orig) IUnknown_Release(orig);

    return pBuffer + size;
}

/******************************************************************************
 *           WdtpInterfacePointer_UserFree (combase.@)
 */
void WINAPI WdtpInterfacePointer_UserFree(IUnknown *punk)
{
    TRACE("%p.\n", punk);
    if (punk) IUnknown_Release(punk);
}
