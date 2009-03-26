/*
 *  OLE 2 clipboard support
 *
 *      Copyright 1999  Noel Borthwick <noel@macadamian.com>
 *      Copyright 2000  Abey George <abey@macadamian.com>
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
 *
 * NOTES:
 *    This file contains the implementation for the OLE Clipboard and its
 *    internal interfaces. The OLE clipboard interacts with an IDataObject
 *    interface via the OleSetClipboard, OleGetClipboard and
 *    OleIsCurrentClipboard API's. An internal IDataObject delegates
 *    to a client supplied IDataObject or the WIN32 clipboard API depending
 *    on whether OleSetClipboard has been invoked.
 *    Here are some operating scenarios:
 *
 *    1. OleSetClipboard called: In this case the internal IDataObject
 *       delegates to the client supplied IDataObject. Additionally OLE takes
 *       ownership of the Windows clipboard and any HGLOCBAL IDataObject
 *       items are placed on the Windows clipboard. This allows non OLE aware
 *       applications to access these. A local WinProc fields WM_RENDERFORMAT
 *       and WM_RENDERALLFORMATS messages in this case.
 *
 *    2. OleGetClipboard called without previous OleSetClipboard. Here the internal
 *       IDataObject functionality wraps around the WIN32 clipboard API.
 *
 *    3. OleGetClipboard called after previous OleSetClipboard. Here the internal
 *       IDataObject delegates to the source IDataObjects functionality directly,
 *       thereby bypassing the Windows clipboard.
 *
 *    Implementation references : Inside OLE 2'nd  edition by Kraig Brockschmidt
 *
 * TODO:
 *    - Support for pasting between different processes. OLE clipboard support
 *      currently works only for in process copy and paste. Since we internally
 *      store a pointer to the source's IDataObject and delegate to that, this
 *      will fail if the IDataObject client belongs to a different process.
 *    - IDataObject::GetDataHere is not implemented
 *    - OleFlushClipboard needs to additionally handle TYMED_IStorage media
 *      by copying the storage into global memory. Subsequently the default
 *      data object exposed through OleGetClipboard must convert this TYMED_HGLOBAL
 *      back to TYMED_IStorage.
 *    - OLE1 compatibility formats to be synthesized from OLE2 formats and put on
 *      clipboard in OleSetClipboard.
 *
 */

#include <assert.h>
#include <stdarg.h>
#include <string.h>

#define COBJMACROS
#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winerror.h"
#include "winnls.h"
#include "ole2.h"
#include "wine/debug.h"
#include "olestd.h"

#include "storage32.h"

#include "compobj_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

#define HANDLE_ERROR(err) do { hr = err; TRACE("(HRESULT=%x)\n", (HRESULT)err); goto CLEANUP; } while (0)


/****************************************************************************
 * ole_clipbrd
 */
struct ole_clipbrd
{
    const IDataObjectVtbl* lpvtbl;  /* Exposed IDataObject vtable */

    LONG ref;

    HWND hWndClipboard;              /* Hidden clipboard window */
    IDataObject* pIDataObjectSrc;    /* Source object passed to OleSetClipboard */
};

typedef struct ole_clipbrd ole_clipbrd;

static inline ole_clipbrd *impl_from_IDataObject(IDataObject *iface)
{
    return (ole_clipbrd*)((char*)iface - FIELD_OFFSET(ole_clipbrd, lpvtbl));
}

typedef struct PresentationDataHeader
{
  BYTE unknown1[28];
  DWORD dwObjectExtentX;
  DWORD dwObjectExtentY;
  DWORD dwSize;
} PresentationDataHeader;

/*
 * The one and only ole_clipbrd object which is created by OLEClipbrd_Initialize()
 */
static ole_clipbrd* theOleClipboard;


/*
 * Name of our registered OLE clipboard window class
 */
static const CHAR OLEClipbrd_WNDCLASS[] = "CLIPBRDWNDCLASS";

static UINT dataobject_clipboard_format;
static UINT ole_priv_data_clipboard_format;

/* Structure of 'Ole Private Data' clipboard format */
typedef struct
{
    FORMATETC fmtetc;
    DWORD first_use;  /* Has this cf been added to the list already */
    DWORD unk[2];
} ole_priv_data_entry;

typedef struct
{
    DWORD unk1;
    DWORD size; /* in bytes of the entire structure */
    DWORD unk2;
    DWORD count; /* no. of format entries */
    DWORD unk3[2];
    ole_priv_data_entry entries[1]; /* array of size count */
    /* then follows any DVTARGETDEVICE structures referenced in the FORMATETCs */
} ole_priv_data;

/*---------------------------------------------------------------------*
 *  Implementation of the internal IEnumFORMATETC interface returned by
 *  the OLE clipboard's IDataObject.
 *---------------------------------------------------------------------*/

typedef struct enum_fmtetc
{
    const IEnumFORMATETCVtbl *lpVtbl;
    LONG ref;

    UINT pos;    /* current enumerator position */
    UINT                         countFmt;  /* number of EnumFORMATETC's in array */
    LPFORMATETC                  pFmt;      /* array of EnumFORMATETC's */


    /*
     * IUnknown implementation of the parent data object.
     */
    IUnknown*                    pUnkDataObj;

} enum_fmtetc;

static inline enum_fmtetc *impl_from_IEnumFORMATETC(IEnumFORMATETC *iface)
{
    return (enum_fmtetc*)((char*)iface - FIELD_OFFSET(enum_fmtetc, lpVtbl));
}

/************************************************************************
 * OLEClipbrd_IEnumFORMATETC_QueryInterface (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static HRESULT WINAPI OLEClipbrd_IEnumFORMATETC_QueryInterface
  (LPENUMFORMATETC iface, REFIID riid, LPVOID* ppvObj)
{
  enum_fmtetc *This = impl_from_IEnumFORMATETC(iface);

  TRACE("(%p)->(IID: %s, %p)\n", This, debugstr_guid(riid), ppvObj);

  *ppvObj = NULL;

  if(IsEqualIID(riid, &IID_IUnknown) ||
     IsEqualIID(riid, &IID_IEnumFORMATETC))
  {
    *ppvObj = iface;
  }

  if(*ppvObj)
  {
    IEnumFORMATETC_AddRef((IEnumFORMATETC*)*ppvObj);
    TRACE("-- Interface: (%p)->(%p)\n",ppvObj,*ppvObj);
    return S_OK;
  }

  TRACE("-- Interface: E_NOINTERFACE\n");
  return E_NOINTERFACE;
}

/************************************************************************
 * OLEClipbrd_IEnumFORMATETC_AddRef (IUnknown)
 *
 * Since enumerating formats only makes sense when our data object is around,
 * we insure that it stays as long as we stay by calling our parents IUnknown
 * for AddRef and Release. But since we are not controlled by the lifetime of
 * the outer object, we still keep our own reference count in order to
 * free ourselves.
 */
static ULONG WINAPI OLEClipbrd_IEnumFORMATETC_AddRef(LPENUMFORMATETC iface)
{
  enum_fmtetc *This = impl_from_IEnumFORMATETC(iface);
  TRACE("(%p)->(count=%u)\n",This, This->ref);

  if (This->pUnkDataObj)
    IUnknown_AddRef(This->pUnkDataObj);

  return InterlockedIncrement(&This->ref);
}

/************************************************************************
 * OLEClipbrd_IEnumFORMATETC_Release (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static ULONG WINAPI OLEClipbrd_IEnumFORMATETC_Release(LPENUMFORMATETC iface)
{
  enum_fmtetc *This = impl_from_IEnumFORMATETC(iface);
  ULONG ref;

  TRACE("(%p)->(count=%u)\n",This, This->ref);

  if (This->pUnkDataObj)
    IUnknown_Release(This->pUnkDataObj);  /* Release parent data object */

  ref = InterlockedDecrement(&This->ref);
  if (!ref)
  {
    TRACE("() - destroying IEnumFORMATETC(%p)\n",This);
    HeapFree(GetProcessHeap(), 0, This->pFmt);
    HeapFree(GetProcessHeap(),0,This);
  }
  return ref;
}

/************************************************************************
 * OLEClipbrd_IEnumFORMATETC_Next (IEnumFORMATETC)
 *
 * Standard enumerator members for IEnumFORMATETC
 */
static HRESULT WINAPI OLEClipbrd_IEnumFORMATETC_Next
  (LPENUMFORMATETC iface, ULONG celt, FORMATETC *rgelt, ULONG *pceltFethed)
{
  enum_fmtetc *This = impl_from_IEnumFORMATETC(iface);
  UINT cfetch;
  HRESULT hres = S_FALSE;

  TRACE("(%p)->(pos=%u)\n", This, This->pos);

  if (This->pos < This->countFmt)
  {
    cfetch = This->countFmt - This->pos;
    if (cfetch >= celt)
    {
      cfetch = celt;
      hres = S_OK;
    }

    memcpy(rgelt, &This->pFmt[This->pos], cfetch * sizeof(FORMATETC));
    This->pos += cfetch;
  }
  else
  {
    cfetch = 0;
  }

  if (pceltFethed)
  {
    *pceltFethed = cfetch;
  }

  return hres;
}

/************************************************************************
 * OLEClipbrd_IEnumFORMATETC_Skip (IEnumFORMATETC)
 *
 * Standard enumerator members for IEnumFORMATETC
 */
static HRESULT WINAPI OLEClipbrd_IEnumFORMATETC_Skip(LPENUMFORMATETC iface, ULONG celt)
{
  enum_fmtetc *This = impl_from_IEnumFORMATETC(iface);
  TRACE("(%p)->(num=%u)\n", This, celt);

  This->pos += celt;
  if (This->pos > This->countFmt)
  {
    This->pos = This->countFmt;
    return S_FALSE;
  }
  return S_OK;
}

/************************************************************************
 * OLEClipbrd_IEnumFORMATETC_Reset (IEnumFORMATETC)
 *
 * Standard enumerator members for IEnumFORMATETC
 */
static HRESULT WINAPI OLEClipbrd_IEnumFORMATETC_Reset(LPENUMFORMATETC iface)
{
  enum_fmtetc *This = impl_from_IEnumFORMATETC(iface);
  TRACE("(%p)->()\n", This);

  This->pos = 0;
  return S_OK;
}

static LPENUMFORMATETC OLEClipbrd_IEnumFORMATETC_Construct(UINT cfmt, const FORMATETC afmt[],
                                                           LPUNKNOWN pUnkDataObj);

/************************************************************************
 * OLEClipbrd_IEnumFORMATETC_Clone (IEnumFORMATETC)
 *
 * Standard enumerator members for IEnumFORMATETC
 */
static HRESULT WINAPI OLEClipbrd_IEnumFORMATETC_Clone
  (LPENUMFORMATETC iface, LPENUMFORMATETC* ppenum)
{
  enum_fmtetc *This = impl_from_IEnumFORMATETC(iface);
  HRESULT hr = S_OK;

  TRACE("(%p)->(ppenum=%p)\n", This, ppenum);

  if ( !ppenum )
    return E_INVALIDARG;

  *ppenum = OLEClipbrd_IEnumFORMATETC_Construct(This->countFmt,
                                                This->pFmt,
                                                This->pUnkDataObj);

  /* FIXME: This is wrong! */
  if (FAILED( hr = IEnumFORMATETC_AddRef(*ppenum)))
    return ( hr );

  return (*ppenum) ? S_OK : E_OUTOFMEMORY;
}

static const IEnumFORMATETCVtbl efvt =
{
  OLEClipbrd_IEnumFORMATETC_QueryInterface,
  OLEClipbrd_IEnumFORMATETC_AddRef,
  OLEClipbrd_IEnumFORMATETC_Release,
  OLEClipbrd_IEnumFORMATETC_Next,
  OLEClipbrd_IEnumFORMATETC_Skip,
  OLEClipbrd_IEnumFORMATETC_Reset,
  OLEClipbrd_IEnumFORMATETC_Clone
};

/************************************************************************
 * OLEClipbrd_IEnumFORMATETC_Construct (UINT, const FORMATETC, LPUNKNOWN)
 *
 * Creates an IEnumFORMATETC enumerator from an array of FORMATETC
 * Structures. pUnkOuter is the outer unknown for reference counting only.
 * NOTE: this does not AddRef the interface.
 */

static LPENUMFORMATETC OLEClipbrd_IEnumFORMATETC_Construct(UINT cfmt, const FORMATETC afmt[],
                                                    LPUNKNOWN pUnkDataObj)
{
  enum_fmtetc* ef;
  DWORD size=cfmt * sizeof(FORMATETC);

  ef = HeapAlloc(GetProcessHeap(), 0, sizeof(*ef));
  if (!ef)
    return NULL;

  ef->ref = 0;
  ef->lpVtbl = &efvt;
  ef->pUnkDataObj = pUnkDataObj;

  ef->pos = 0;
  ef->countFmt = cfmt;
  ef->pFmt = HeapAlloc(GetProcessHeap(), 0, size);
  if (ef->pFmt)
    memcpy(ef->pFmt, afmt, size);
  else
  {
    HeapFree(GetProcessHeap(), 0, ef);
    return NULL;
  }

  TRACE("(%p)->()\n",ef);
  return (LPENUMFORMATETC)ef;
}

/***********************************************************************
 * OLEClipbrd_GlobalDupMem( HGLOBAL )
 * Helper method to duplicate an HGLOBAL chunk of memory
 */
static HGLOBAL OLEClipbrd_GlobalDupMem( HGLOBAL hGlobalSrc )
{
    HGLOBAL hGlobalDest;
    PVOID pGlobalSrc, pGlobalDest;
    DWORD cBytes;

    if ( !hGlobalSrc )
      return 0;

    cBytes = GlobalSize(hGlobalSrc);
    if ( 0 == cBytes )
      return 0;

    hGlobalDest = GlobalAlloc( GMEM_DDESHARE|GMEM_MOVEABLE,
                               cBytes );
    if ( !hGlobalDest )
      return 0;

    pGlobalSrc = GlobalLock(hGlobalSrc);
    pGlobalDest = GlobalLock(hGlobalDest);
    if ( !pGlobalSrc || !pGlobalDest )
    {
      GlobalFree(hGlobalDest);
      return 0;
    }

    memcpy(pGlobalDest, pGlobalSrc, cBytes);

    GlobalUnlock(hGlobalSrc);
    GlobalUnlock(hGlobalDest);

    return hGlobalDest;
}

#define MAX_CLIPFORMAT_NAME   80

/***********************************************************************
 * OLEClipbrd_RenderFormat(LPFORMATETC)
 * Render the clipboard data. Note that this call will delegate to the
 * source data object.
 * Note: This function assumes it is passed an HGLOBAL format to render.
 */
static HRESULT OLEClipbrd_RenderFormat(IDataObject *pIDataObject, LPFORMATETC pFormatetc)
{
  STGMEDIUM std;
  HGLOBAL hDup;
  HRESULT hr = S_OK;
  char szFmtName[MAX_CLIPFORMAT_NAME];
  ILockBytes *ptrILockBytes = 0;
  HGLOBAL hStorage = 0;

  if (!GetClipboardFormatNameA(pFormatetc->cfFormat, szFmtName, MAX_CLIPFORMAT_NAME))
      szFmtName[0] = '\0';

  /* If embed source */
  if (!strcmp(szFmtName, CF_EMBEDSOURCE))
  {
    memset(&std, 0, sizeof(STGMEDIUM));
    std.tymed = pFormatetc->tymed = TYMED_ISTORAGE;

    hStorage = GlobalAlloc(GMEM_SHARE|GMEM_MOVEABLE, 0);
    if (hStorage == NULL)
      HANDLE_ERROR( E_OUTOFMEMORY );
    hr = CreateILockBytesOnHGlobal(hStorage, FALSE, &ptrILockBytes);
    hr = StgCreateDocfileOnILockBytes(ptrILockBytes, STGM_SHARE_EXCLUSIVE|STGM_READWRITE, 0, &std.u.pstg);

    if (FAILED(hr = IDataObject_GetDataHere(theOleClipboard->pIDataObjectSrc, pFormatetc, &std)))
    {
      WARN("() : IDataObject_GetDataHere failed to render clipboard data! (%x)\n", hr);
      GlobalFree(hStorage);
      return hr;
    }

    if (1) /* check whether the presentation data is already -not- present */
    {
      FORMATETC fmt2;
      STGMEDIUM std2;
      METAFILEPICT *mfp = 0;

      fmt2.cfFormat = CF_METAFILEPICT;
      fmt2.ptd = 0;
      fmt2.dwAspect = DVASPECT_CONTENT;
      fmt2.lindex = -1;
      fmt2.tymed = TYMED_MFPICT;

      memset(&std2, 0, sizeof(STGMEDIUM));
      std2.tymed = TYMED_MFPICT;

      /* Get the metafile picture out of it */

      if (SUCCEEDED(hr = IDataObject_GetData(theOleClipboard->pIDataObjectSrc, &fmt2, &std2)))
      {
        mfp = GlobalLock(std2.u.hGlobal);
      }

      if (mfp)
      {
        OLECHAR name[]={ 2, 'O', 'l', 'e', 'P', 'r', 'e', 's', '0', '0', '0', 0};
        IStream *pStream = 0;
        void *mfBits;
        PresentationDataHeader pdh;
        INT nSize;
        CLSID clsID;
        LPOLESTR strProgID;
        CHAR strOleTypeName[51];
        BYTE OlePresStreamHeader [] =
        {
            0xFF, 0xFF, 0xFF, 0xFF, 0x03, 0x00, 0x00, 0x00,
            0x04, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
            0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00
        };

        nSize = GetMetaFileBitsEx(mfp->hMF, 0, NULL);

        memset(&pdh, 0, sizeof(PresentationDataHeader));
        memcpy(&pdh, OlePresStreamHeader, sizeof(OlePresStreamHeader));

        pdh.dwObjectExtentX = mfp->xExt;
        pdh.dwObjectExtentY = mfp->yExt;
        pdh.dwSize = nSize;

        hr = IStorage_CreateStream(std.u.pstg, name, STGM_CREATE|STGM_SHARE_EXCLUSIVE|STGM_READWRITE, 0, 0, &pStream);

        hr = IStream_Write(pStream, &pdh, sizeof(PresentationDataHeader), NULL);

        mfBits = HeapAlloc(GetProcessHeap(), 0, nSize);
        nSize = GetMetaFileBitsEx(mfp->hMF, nSize, mfBits);

        hr = IStream_Write(pStream, mfBits, nSize, NULL);

        IStream_Release(pStream);

        HeapFree(GetProcessHeap(), 0, mfBits);

        GlobalUnlock(std2.u.hGlobal);

        ReadClassStg(std.u.pstg, &clsID);
        ProgIDFromCLSID(&clsID, &strProgID);

        WideCharToMultiByte( CP_ACP, 0, strProgID, -1, strOleTypeName, sizeof(strOleTypeName), NULL, NULL );
        OLECONVERT_CreateOleStream(std.u.pstg);
        OLECONVERT_CreateCompObjStream(std.u.pstg, strOleTypeName);
      }
    }
  }
  else
  {
    if (FAILED(hr = IDataObject_GetData(pIDataObject, pFormatetc, &std)))
    {
        WARN("() : IDataObject_GetData failed to render clipboard data! (%x)\n", hr);
        GlobalFree(hStorage);
        return hr;
    }

    /* To put a copy back on the clipboard */

    hStorage = std.u.hGlobal;
  }

  /*
   *  Put a copy of the rendered data back on the clipboard
   */

  if ( !(hDup = OLEClipbrd_GlobalDupMem(hStorage)) )
    HANDLE_ERROR( E_OUTOFMEMORY );

  if ( !SetClipboardData( pFormatetc->cfFormat, hDup ) )
  {
    GlobalFree(hDup);
    WARN("() : Failed to set rendered clipboard data into clipboard!\n");
  }

CLEANUP:

  ReleaseStgMedium(&std);

  return hr;
}


/***********************************************************************
 * OLEClipbrd_WndProc(HWND, unsigned, WORD, LONG)
 * Processes messages sent to the OLE clipboard window.
 * Note that we will intercept messages in our WndProc only when data
 * has been placed in the clipboard via OleSetClipboard().
 * i.e. Only when OLE owns the windows clipboard.
 */
static LRESULT CALLBACK OLEClipbrd_WndProc
  (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  switch (message)
  {
    /*
     * WM_RENDERFORMAT
     * We receive this message to allow us to handle delayed rendering of
     * a specific clipboard format when an application requests data in
     * that format by calling GetClipboardData.
     * (Recall that in OleSetClipboard, we used SetClipboardData to
     * make all HGLOBAL formats supported by the source IDataObject
     * available using delayed rendering)
     * On receiving this message we must actually render the data in the
     * specified format and place it on the clipboard by calling the
     * SetClipboardData function.
     */
    case WM_RENDERFORMAT:
    {
      FORMATETC rgelt;

      ZeroMemory( &rgelt, sizeof(FORMATETC));

      /*
       * Initialize FORMATETC to a Windows clipboard friendly format
       */
      rgelt.cfFormat = (UINT) wParam;
      rgelt.dwAspect = DVASPECT_CONTENT;
      rgelt.lindex = -1;
      rgelt.tymed = TYMED_HGLOBAL;

      TRACE("(): WM_RENDERFORMAT(cfFormat=%d)\n", rgelt.cfFormat);

      /*
       * Render the clipboard data.
       * (We must have a source data object or we wouldn't be in this WndProc)
       */
      OLEClipbrd_RenderFormat( (IDataObject*)&(theOleClipboard->lpvtbl), &rgelt );

      break;
    }

    /*
     * WM_RENDERALLFORMATS
     * Sent before the clipboard owner window is destroyed.
     * We should receive this message only when OleUninitialize is called
     * while we have an IDataObject in the clipboard.
     * For the content of the clipboard to remain available to other
     * applications, we must render data in all the formats the source IDataObject
     * is capable of generating, and place the data on the clipboard by calling
     * SetClipboardData.
     */
    case WM_RENDERALLFORMATS:
    {
      IEnumFORMATETC* penumFormatetc = NULL;
      FORMATETC rgelt;

      TRACE("(): WM_RENDERALLFORMATS\n");

      /*
       * Render all HGLOBAL formats supported by the source into
       * the windows clipboard.
       */
      if ( FAILED( IDataObject_EnumFormatEtc( (IDataObject*)&(theOleClipboard->lpvtbl),
                                 DATADIR_GET, &penumFormatetc) ) )
      {
        WARN("(): WM_RENDERALLFORMATS failed to retrieve EnumFormatEtc!\n");
        return 0;
      }

      while ( S_OK == IEnumFORMATETC_Next(penumFormatetc, 1, &rgelt, NULL) )
      {
        if ( rgelt.tymed == TYMED_HGLOBAL )
        {
          /*
           * Render the clipboard data.
           */
          if ( FAILED(OLEClipbrd_RenderFormat( (IDataObject*)&(theOleClipboard->lpvtbl), &rgelt )) )
            continue;

          TRACE("(): WM_RENDERALLFORMATS(cfFormat=%d)\n", rgelt.cfFormat);
        }
      }

      IEnumFORMATETC_Release(penumFormatetc);

      break;
    }

    /*
     * WM_DESTROYCLIPBOARD
     * This is sent by EmptyClipboard before the clipboard is emptied.
     * We should release any IDataObject we are holding onto when we receive
     * this message, since it indicates that the OLE clipboard should be empty
     * from this point on.
     */
    case WM_DESTROYCLIPBOARD:
    {
      TRACE("(): WM_DESTROYCLIPBOARD\n");
      /*
       * Release the data object we are holding on to
       */
      if ( theOleClipboard->pIDataObjectSrc )
      {
        IDataObject_Release(theOleClipboard->pIDataObjectSrc);
        theOleClipboard->pIDataObjectSrc = NULL;
      }
      break;
    }

/*
    case WM_ASKCBFORMATNAME:
    case WM_CHANGECBCHAIN:
    case WM_DRAWCLIPBOARD:
    case WM_SIZECLIPBOARD:
    case WM_HSCROLLCLIPBOARD:
    case WM_VSCROLLCLIPBOARD:
    case WM_PAINTCLIPBOARD:
*/
    default:
      return DefWindowProcA(hWnd, message, wParam, lParam);
  }

  return 0;
}


/*---------------------------------------------------------------------*
 *  Implementation of the internal IDataObject interface exposed by
 *  the OLE clipboard.
 *---------------------------------------------------------------------*/


/************************************************************************
 * OLEClipbrd_IDataObject_QueryInterface (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static HRESULT WINAPI OLEClipbrd_IDataObject_QueryInterface(
            IDataObject*     iface,
            REFIID           riid,
            void**           ppvObject)
{
  ole_clipbrd *This = impl_from_IDataObject(iface);
  TRACE("(%p)->(IID:%s, %p)\n", This, debugstr_guid(riid), ppvObject);

  if ( (This==0) || (ppvObject==0) )
    return E_INVALIDARG;

  *ppvObject = 0;

  if (IsEqualIID(&IID_IUnknown, riid) ||
      IsEqualIID(&IID_IDataObject, riid))
  {
    *ppvObject = iface;
  }
  else
  {
    WARN( "() : asking for unsupported interface %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
  }

  IUnknown_AddRef((IUnknown*)*ppvObject);

  return S_OK;
}

/************************************************************************
 * OLEClipbrd_IDataObject_AddRef (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static ULONG WINAPI OLEClipbrd_IDataObject_AddRef(
            IDataObject*     iface)
{
  ole_clipbrd *This = impl_from_IDataObject(iface);

  TRACE("(%p)->(count=%u)\n",This, This->ref);

  return InterlockedIncrement(&This->ref);
}

/***********************************************************************
 * OLEClipbrd_DestroyWindow(HWND)
 * Destroy the clipboard window and unregister its class
 */
static void OLEClipbrd_DestroyWindow(HWND hwnd)
{
  DestroyWindow(hwnd);
  UnregisterClassA( OLEClipbrd_WNDCLASS, 0 );
}

static void OLEClipbrd_Destroy(ole_clipbrd* This)
{
    TRACE("()\n");

    if (!This) return;

    theOleClipboard = NULL;

    if ( This->hWndClipboard )
        OLEClipbrd_DestroyWindow(This->hWndClipboard);

    HeapFree(GetProcessHeap(), 0, This);
}

/************************************************************************
 * OLEClipbrd_IDataObject_Release (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static ULONG WINAPI OLEClipbrd_IDataObject_Release(
            IDataObject*     iface)
{
  ole_clipbrd *This = impl_from_IDataObject(iface);
  ULONG ref;

  TRACE("(%p)->(count=%u)\n",This, This->ref);

  ref = InterlockedDecrement(&This->ref);

  if (ref == 0)
  {
    OLEClipbrd_Destroy(This);
  }

  return ref;
}


/************************************************************************
 * OLEClipbrd_IDataObject_GetData (IDataObject)
 *
 * The OLE Clipboard's implementation of this method delegates to
 * a data source if there is one or wraps around the windows clipboard
 *
 * See Windows documentation for more details on IDataObject methods.
 */
static HRESULT WINAPI OLEClipbrd_IDataObject_GetData(
	    IDataObject*     iface,
	    LPFORMATETC      pformatetcIn,
	    STGMEDIUM*       pmedium)
{
  HANDLE      hData = 0;
  BOOL bClipboardOpen = FALSE;
  HRESULT hr = S_OK;
  LPVOID src;
  ole_clipbrd *This = impl_from_IDataObject(iface);

  TRACE("(%p,%p,%p)\n", iface, pformatetcIn, pmedium);

  if ( !pformatetcIn || !pmedium )
    return E_INVALIDARG;

  /*
   * If we have a data source placed on the clipboard (via OleSetClipboard)
   * simply delegate to the source object's QueryGetData
   * NOTE: This code assumes that the IDataObject is in the same address space!
   * We will need to add marshalling support when Wine handles multiple processes.
   */
  if ( This->pIDataObjectSrc )
  {
    return IDataObject_GetData(This->pIDataObjectSrc, pformatetcIn, pmedium);
  }

  if ( pformatetcIn->lindex != -1 )
    return DV_E_FORMATETC;

  if ( (pformatetcIn->tymed & TYMED_HGLOBAL) != TYMED_HGLOBAL )
    return DV_E_TYMED;
/*
   if ( pformatetcIn->dwAspect != DVASPECT_CONTENT )
     return DV_E_DVASPECT;
*/

  /*
   * Otherwise, get the data from the windows clipboard using GetClipboardData
   */
  if ( !(bClipboardOpen = OpenClipboard(theOleClipboard->hWndClipboard)) )
    HANDLE_ERROR( CLIPBRD_E_CANT_OPEN );

  hData = GetClipboardData(pformatetcIn->cfFormat);

  /* Must make a copy of global handle returned by GetClipboardData; it
   * is not valid after we call CloseClipboard
   * Application is responsible for freeing the memory (Forte Agent does this)
   */
  src = GlobalLock(hData);
  if(src) {
      LPVOID dest;
      ULONG  size;
      HANDLE hDest;

      size = GlobalSize(hData);
      hDest = GlobalAlloc(GHND, size);
      dest  = GlobalLock(hDest);
      memcpy(dest, src, size);
      GlobalUnlock(hDest);
      GlobalUnlock(hData);
      hData = hDest;
  }

  /*
   * Return the clipboard data in the storage medium structure
   */
  pmedium->tymed = (hData == 0) ? TYMED_NULL : TYMED_HGLOBAL;
  pmedium->u.hGlobal = hData;
  pmedium->pUnkForRelease = NULL;

  hr = S_OK;

CLEANUP:
  /*
   * Close Windows clipboard
   */
  if ( bClipboardOpen && !CloseClipboard() )
     hr = CLIPBRD_E_CANT_CLOSE;

  if ( FAILED(hr) )
      return hr;
  return (hData == 0) ? DV_E_FORMATETC : S_OK;
}

static HRESULT WINAPI OLEClipbrd_IDataObject_GetDataHere(
	    IDataObject*     iface,
	    LPFORMATETC      pformatetc,
	    STGMEDIUM*       pmedium)
{
  FIXME(": Stub\n");
  return E_NOTIMPL;
}

/************************************************************************
 * OLEClipbrd_IDataObject_QueryGetData (IDataObject)
 *
 * The OLE Clipboard's implementation of this method delegates to
 * a data source if there is one or wraps around the windows clipboard
 * function IsClipboardFormatAvailable() otherwise.
 *
 * See Windows documentation for more details on IDataObject methods.
 */
static HRESULT WINAPI OLEClipbrd_IDataObject_QueryGetData(
	    IDataObject*     iface,
	    LPFORMATETC      pformatetc)
{
  TRACE("(%p, %p)\n", iface, pformatetc);

  if (!pformatetc)
    return E_INVALIDARG;

  if ( pformatetc->dwAspect != DVASPECT_CONTENT )
    return DV_E_FORMATETC;

  if ( pformatetc->lindex != -1 )
    return DV_E_FORMATETC;

  /*
   * Delegate to the Windows clipboard function IsClipboardFormatAvailable
   */
  return (IsClipboardFormatAvailable(pformatetc->cfFormat)) ? S_OK : DV_E_CLIPFORMAT;
}

/************************************************************************
 * OLEClipbrd_IDataObject_GetCanonicalFormatEtc (IDataObject)
 *
 * See Windows documentation for more details on IDataObject methods.
 */
static HRESULT WINAPI OLEClipbrd_IDataObject_GetCanonicalFormatEtc(
	    IDataObject*     iface,
	    LPFORMATETC      pformatectIn,
	    LPFORMATETC      pformatetcOut)
{
  TRACE("(%p, %p, %p)\n", iface, pformatectIn, pformatetcOut);

  if ( !pformatectIn || !pformatetcOut )
    return E_INVALIDARG;

  *pformatetcOut = *pformatectIn;
  return DATA_S_SAMEFORMATETC;
}

/************************************************************************
 * OLEClipbrd_IDataObject_SetData (IDataObject)
 *
 * The OLE Clipboard's does not implement this method
 *
 * See Windows documentation for more details on IDataObject methods.
 */
static HRESULT WINAPI OLEClipbrd_IDataObject_SetData(
	    IDataObject*     iface,
	    LPFORMATETC      pformatetc,
	    STGMEDIUM*       pmedium,
	    BOOL             fRelease)
{
  TRACE("\n");
  return E_NOTIMPL;
}

/************************************************************************
 * OLEClipbrd_IDataObject_EnumFormatEtc (IDataObject)
 *
 * See Windows documentation for more details on IDataObject methods.
 */
static HRESULT WINAPI OLEClipbrd_IDataObject_EnumFormatEtc(
	    IDataObject*     iface,
	    DWORD            dwDirection,
	    IEnumFORMATETC** ppenumFormatEtc)
{
  HRESULT hr = S_OK;
  FORMATETC *afmt = NULL;
  int cfmt, i;
  UINT format;
  BOOL bClipboardOpen;
  ole_clipbrd *This = impl_from_IDataObject(iface);

  TRACE("(%p, %x, %p)\n", iface, dwDirection, ppenumFormatEtc);

  /*
   * If we have a data source placed on the clipboard (via OleSetClipboard)
   * simply delegate to the source object's EnumFormatEtc
   */
  if ( This->pIDataObjectSrc )
  {
    return IDataObject_EnumFormatEtc(This->pIDataObjectSrc,
                                     dwDirection, ppenumFormatEtc);
  }

  /*
   * Otherwise we must provide our own enumerator which wraps around the
   * Windows clipboard function EnumClipboardFormats
   */
  if ( !ppenumFormatEtc )
    return E_INVALIDARG;

  if ( dwDirection != DATADIR_GET ) /* IDataObject_SetData not implemented */
    return E_NOTIMPL;

  /*
   * Store all current clipboard formats in an array of FORMATETC's,
   * and create an IEnumFORMATETC enumerator from this list.
   */
  cfmt = CountClipboardFormats();
  afmt = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                                sizeof(FORMATETC) * cfmt);
  /*
   * Open the Windows clipboard, associating it with our hidden window
   */
  if ( !(bClipboardOpen = OpenClipboard(This->hWndClipboard)) )
    HANDLE_ERROR( CLIPBRD_E_CANT_OPEN );

  /*
   * Store all current clipboard formats in an array of FORMATETC's
   * TODO: Handle TYMED_IStorage media which were put on the clipboard
   * by copying the storage into global memory. We must convert this
   * TYMED_HGLOBAL back to TYMED_IStorage.
   */
  for (i = 0, format = 0; i < cfmt; i++)
  {
    format = EnumClipboardFormats(format);
    if (!format)  /* Failed! */
    {
      ERR("EnumClipboardFormats failed to return format!\n");
      HANDLE_ERROR( E_FAIL );
    }

    /* Init the FORMATETC struct */
    afmt[i].cfFormat = format;
    afmt[i].ptd = NULL;
    afmt[i].dwAspect = DVASPECT_CONTENT;
    afmt[i].lindex = -1;
    afmt[i].tymed = TYMED_HGLOBAL;
  }

  /*
   * Create an EnumFORMATETC enumerator and return an
   * EnumFORMATETC after bumping up its ref count
   */
  *ppenumFormatEtc = OLEClipbrd_IEnumFORMATETC_Construct( cfmt, afmt, (LPUNKNOWN)iface);
  if (!(*ppenumFormatEtc))
    HANDLE_ERROR( E_OUTOFMEMORY );

  if (FAILED( hr = IEnumFORMATETC_AddRef(*ppenumFormatEtc)))
    HANDLE_ERROR( hr );

  hr = S_OK;

CLEANUP:
  /*
   * Free the array of FORMATETC's
   */
  HeapFree(GetProcessHeap(), 0, afmt);

  /*
   * Close Windows clipboard
   */
  if ( bClipboardOpen && !CloseClipboard() )
    hr = CLIPBRD_E_CANT_CLOSE;

  return hr;
}

/************************************************************************
 * OLEClipbrd_IDataObject_DAdvise (IDataObject)
 *
 * The OLE Clipboard's does not implement this method
 *
 * See Windows documentation for more details on IDataObject methods.
 */
static HRESULT WINAPI OLEClipbrd_IDataObject_DAdvise(
	    IDataObject*     iface,
	    FORMATETC*       pformatetc,
	    DWORD            advf,
	    IAdviseSink*     pAdvSink,
	    DWORD*           pdwConnection)
{
  TRACE("\n");
  return E_NOTIMPL;
}

/************************************************************************
 * OLEClipbrd_IDataObject_DUnadvise (IDataObject)
 *
 * The OLE Clipboard's does not implement this method
 *
 * See Windows documentation for more details on IDataObject methods.
 */
static HRESULT WINAPI OLEClipbrd_IDataObject_DUnadvise(
	    IDataObject*     iface,
	    DWORD            dwConnection)
{
  TRACE("\n");
  return E_NOTIMPL;
}

/************************************************************************
 * OLEClipbrd_IDataObject_EnumDAdvise (IDataObject)
 *
 * The OLE Clipboard does not implement this method
 *
 * See Windows documentation for more details on IDataObject methods.
 */
static HRESULT WINAPI OLEClipbrd_IDataObject_EnumDAdvise(
	    IDataObject*     iface,
	    IEnumSTATDATA**  ppenumAdvise)
{
  TRACE("\n");
  return E_NOTIMPL;
}

static const IDataObjectVtbl OLEClipbrd_IDataObject_VTable =
{
  OLEClipbrd_IDataObject_QueryInterface,
  OLEClipbrd_IDataObject_AddRef,
  OLEClipbrd_IDataObject_Release,
  OLEClipbrd_IDataObject_GetData,
  OLEClipbrd_IDataObject_GetDataHere,
  OLEClipbrd_IDataObject_QueryGetData,
  OLEClipbrd_IDataObject_GetCanonicalFormatEtc,
  OLEClipbrd_IDataObject_SetData,
  OLEClipbrd_IDataObject_EnumFormatEtc,
  OLEClipbrd_IDataObject_DAdvise,
  OLEClipbrd_IDataObject_DUnadvise,
  OLEClipbrd_IDataObject_EnumDAdvise
};

/*---------------------------------------------------------------------*
 *           Internal implementation methods for the OLE clipboard
 *---------------------------------------------------------------------*/

/*********************************************************
 * Construct the OLEClipbrd class.
 */
static ole_clipbrd* OLEClipbrd_Construct(void)
{
    ole_clipbrd* This;

    This = HeapAlloc( GetProcessHeap(), 0, sizeof(*This) );
    if (!This) return NULL;

    This->lpvtbl = &OLEClipbrd_IDataObject_VTable;
    This->ref = 1;

    This->hWndClipboard = NULL;
    This->pIDataObjectSrc = NULL;

    theOleClipboard = This;
    return This;
}

static void register_clipboard_formats(void)
{
    static const WCHAR DataObjectW[] = { 'D','a','t','a','O','b','j','e','c','t',0 };
    static const WCHAR OlePrivateDataW[] = { 'O','l','e',' ','P','r','i','v','a','t','e',' ','D','a','t','a',0 };

    if(!dataobject_clipboard_format)
        dataobject_clipboard_format = RegisterClipboardFormatW(DataObjectW);
    if(!ole_priv_data_clipboard_format)
        ole_priv_data_clipboard_format = RegisterClipboardFormatW(OlePrivateDataW);
}

/***********************************************************************
 * OLEClipbrd_Initialize()
 * Initializes the OLE clipboard.
 */
void OLEClipbrd_Initialize(void)
{
  register_clipboard_formats();

  if ( !theOleClipboard )
  {
    TRACE("()\n");
    theOleClipboard = OLEClipbrd_Construct();
  }
}


/***********************************************************************
 * OLEClipbrd_UnInitialize()
 * Un-Initializes the OLE clipboard
 */
void OLEClipbrd_UnInitialize(void)
{
  TRACE("()\n");
  /*
   * Destroy the clipboard if no one holds a reference to us.
   * Note that the clipboard was created with a reference count of 1.
   */
  if ( theOleClipboard && (theOleClipboard->ref <= 1) )
  {
    OLEClipbrd_Destroy( theOleClipboard );
  }
  else
  {
    WARN( "() : OLEClipbrd_UnInitialize called while client holds an IDataObject reference!\n");
  }
}

/***********************************************************************
 * OLEClipbrd_CreateWindow()
 * Create the clipboard window
 */
static HWND OLEClipbrd_CreateWindow(void)
{
  HWND hwnd = 0;
  WNDCLASSEXA wcex;

  /*
   * Register the clipboard window class if necessary
   */
    ZeroMemory( &wcex, sizeof(WNDCLASSEXA));

    wcex.cbSize         = sizeof(WNDCLASSEXA);
    /* Windows creates this class with a style mask of 0
     * We don't bother doing this since the FindClassByAtom code
     * would have to be changed to deal with this idiosyncrasy. */
    wcex.style          = CS_GLOBALCLASS;
    wcex.lpfnWndProc    = OLEClipbrd_WndProc;
    wcex.hInstance      = 0;
    wcex.lpszClassName  = OLEClipbrd_WNDCLASS;

    RegisterClassExA(&wcex);

  /*
   * Create a hidden window to receive OLE clipboard messages
   */

/*
 *  If we need to store state info we can store it here.
 *  For now we don't need this functionality.
 *   ClipboardWindowInfo clipboardInfo;
 *   ZeroMemory( &trackerInfo, sizeof(ClipboardWindowInfo));
 */

  hwnd = CreateWindowA(OLEClipbrd_WNDCLASS,
				    "ClipboardWindow",
				    WS_POPUP | WS_CLIPSIBLINGS | WS_OVERLAPPED,
				    CW_USEDEFAULT, CW_USEDEFAULT,
				    CW_USEDEFAULT, CW_USEDEFAULT,
				    0,
				    0,
				    0,
				    0 /*(LPVOID)&clipboardInfo */);

  return hwnd;
}

static inline BOOL is_format_in_list(ole_priv_data_entry *entries, DWORD num, UINT cf)
{
    DWORD i;
    for(i = 0; i < num; i++)
        if(entries[i].fmtetc.cfFormat == cf)
            return TRUE;

    return FALSE;
}

/*********************************************************************
 *          set_clipboard_formats
 *
 * Enumerate all HGLOBAL formats supported by the source and make
 * those formats available using delayed rendering using SetClipboardData.
 *
 * TODO: We need to additionally handle TYMED_IStorage and
 * TYMED_IStream data by copying into global memory.
 */
static HRESULT set_clipboard_formats(IDataObject *data)
{
    HRESULT hr;
    FORMATETC fmt;
    IEnumFORMATETC *enum_fmt;
    HGLOBAL priv_data_handle;
    DWORD target_offset;
    ole_priv_data *priv_data;
    DWORD count = 0, needed = sizeof(*priv_data), idx;

    hr = IDataObject_EnumFormatEtc(data, DATADIR_GET, &enum_fmt);
    if(FAILED(hr)) return hr;

    while(IEnumFORMATETC_Next(enum_fmt, 1, &fmt, NULL) == S_OK)
    {
        count++;
        needed += sizeof(priv_data->entries[0]);
        if(fmt.ptd)
        {
            needed += fmt.ptd->tdSize;
            CoTaskMemFree(fmt.ptd);
        }
    }

    /* Windows pads the list with two empty ole_priv_data_entries, one
     * after the entries array and one after the target device data.
     * Allocating with zero init to zero these pads. */

    needed += sizeof(priv_data->entries[0]); /* initialisation of needed includes one of these. */
    priv_data_handle = GlobalAlloc(GMEM_DDESHARE | GMEM_MOVEABLE | GMEM_ZEROINIT, needed);
    priv_data = GlobalLock(priv_data_handle);

    priv_data->unk1 = 0;
    priv_data->size = needed;
    priv_data->unk2 = 1;
    priv_data->count = count;
    priv_data->unk3[0] = 0;
    priv_data->unk3[1] = 0;

    IEnumFORMATETC_Reset(enum_fmt);

    idx = 0;
    target_offset = FIELD_OFFSET(ole_priv_data, entries[count + 1]); /* count entries + one pad. */

    while(IEnumFORMATETC_Next(enum_fmt, 1, &fmt, NULL) == S_OK)
    {
        if (fmt.tymed == TYMED_HGLOBAL)
        {
            char fmt_name[80];
            TRACE("(cfFormat=%d:%s)\n", fmt.cfFormat,
                  GetClipboardFormatNameA(fmt.cfFormat, fmt_name, sizeof(fmt_name)-1) ? fmt_name : "");

            SetClipboardData(fmt.cfFormat, NULL);
        }

        priv_data->entries[idx].fmtetc = fmt;
        if(fmt.ptd)
        {
            memcpy((char*)priv_data + target_offset, fmt.ptd, fmt.ptd->tdSize);
            priv_data->entries[idx].fmtetc.ptd = (DVTARGETDEVICE*)target_offset;
            target_offset += fmt.ptd->tdSize;
            CoTaskMemFree(fmt.ptd);
        }

        priv_data->entries[idx].first_use = !is_format_in_list(priv_data->entries, idx, fmt.cfFormat);
        priv_data->entries[idx].unk[0] = 0;
        priv_data->entries[idx].unk[1] = 0;

        idx++;
    }

    IEnumFORMATETC_Release(enum_fmt);

    GlobalUnlock(priv_data_handle);
    SetClipboardData(ole_priv_data_clipboard_format, priv_data_handle);

    return S_OK;
}

/*********************************************************************
 *          set_dataobject_format
 *
 * Windows creates a 'DataObject' clipboard format that contains the
 * clipboard window's HWND or NULL if the Ole clipboard has been flushed.
 */
static HRESULT set_dataobject_format(HWND hwnd)
{
    HGLOBAL h = GlobalAlloc(GMEM_DDESHARE | GMEM_MOVEABLE, sizeof(hwnd));
    HWND *data;

    if(!h) return E_OUTOFMEMORY;

    data = GlobalLock(h);
    *data = hwnd;
    GlobalUnlock(h);

    if(!SetClipboardData(dataobject_clipboard_format, h))
    {
        GlobalFree(h);
        return CLIPBRD_E_CANT_SET;
    }

    return S_OK;
}

/*---------------------------------------------------------------------*
 *           Win32 OLE clipboard API
 *---------------------------------------------------------------------*/

/***********************************************************************
 *           OleSetClipboard     [OLE32.@]
 *  Places a pointer to the specified data object onto the clipboard,
 *  making the data object accessible to the OleGetClipboard function.
 *
 * RETURNS
 *
 *    S_OK                  IDataObject pointer placed on the clipboard
 *    CLIPBRD_E_CANT_OPEN   OpenClipboard failed
 *    CLIPBRD_E_CANT_EMPTY  EmptyClipboard failed
 *    CLIPBRD_E_CANT_CLOSE  CloseClipboard failed
 *    CLIPBRD_E_CANT_SET    SetClipboard failed
 */

HRESULT WINAPI OleSetClipboard(IDataObject* pDataObj)
{
  HRESULT hr = S_OK;
  BOOL bClipboardOpen = FALSE;
  struct oletls *info = COM_CurrentInfo();

  TRACE("(%p)\n", pDataObj);

  if(!info)
    WARN("Could not allocate tls\n");
  else
    if(!info->ole_inits)
      return CO_E_NOTINITIALIZED;

  /*
   * Make sure we have a clipboard object
   */
  OLEClipbrd_Initialize();

  /*
   * If the Ole clipboard window hasn't been created yet, create it now.
   */
  if ( !theOleClipboard->hWndClipboard )
    theOleClipboard->hWndClipboard = OLEClipbrd_CreateWindow();

  if ( !theOleClipboard->hWndClipboard ) /* sanity check */
    HANDLE_ERROR( E_FAIL );

  /*
   * Open the Windows clipboard, associating it with our hidden window
   */
  if ( !(bClipboardOpen = OpenClipboard(theOleClipboard->hWndClipboard)) )
    HANDLE_ERROR( CLIPBRD_E_CANT_OPEN );

  /*
   * Empty the current clipboard and make our window the clipboard owner
   * NOTE: This will trigger a WM_DESTROYCLIPBOARD message
   */
  if ( !EmptyClipboard() )
    HANDLE_ERROR( CLIPBRD_E_CANT_EMPTY );

  /*
   * If we are already holding on to an IDataObject first release that.
   */
  if ( theOleClipboard->pIDataObjectSrc )
  {
    IDataObject_Release(theOleClipboard->pIDataObjectSrc);
    theOleClipboard->pIDataObjectSrc = NULL;
  }

  /* A NULL value indicates that the clipboard should be emptied. */
  theOleClipboard->pIDataObjectSrc = pDataObj;
  if ( pDataObj )
  {
    IDataObject_AddRef(theOleClipboard->pIDataObjectSrc);
    hr = set_clipboard_formats(pDataObj);
    if(FAILED(hr)) goto CLEANUP;
  }

  hr = set_dataobject_format(theOleClipboard->hWndClipboard);

CLEANUP:

  /*
   * Close Windows clipboard (It remains associated with our window)
   */
  if ( bClipboardOpen && !CloseClipboard() )
    hr = CLIPBRD_E_CANT_CLOSE;

  /*
   * Release the source IDataObject if something failed
   */
  if ( FAILED(hr) )
  {
    if (theOleClipboard->pIDataObjectSrc)
    {
      IDataObject_Release(theOleClipboard->pIDataObjectSrc);
      theOleClipboard->pIDataObjectSrc = NULL;
    }
  }

  return hr;
}


/***********************************************************************
 * OleGetClipboard [OLE32.@]
 * Returns a pointer to our internal IDataObject which represents the conceptual
 * state of the Windows clipboard. If the current clipboard already contains
 * an IDataObject, our internal IDataObject will delegate to this object.
 */
HRESULT WINAPI OleGetClipboard(IDataObject** ppDataObj)
{
  HRESULT hr = S_OK;
  TRACE("()\n");

  /*
   * Make sure we have a clipboard object
   */
  OLEClipbrd_Initialize();

  if (!theOleClipboard)
    return E_OUTOFMEMORY;

  /* Return a reference counted IDataObject */
  hr = IDataObject_QueryInterface( (IDataObject*)&(theOleClipboard->lpvtbl),
                                   &IID_IDataObject,  (void**)ppDataObj);
  return hr;
}

/******************************************************************************
 *              OleFlushClipboard        [OLE32.@]
 *  Renders the data from the source IDataObject into the windows clipboard
 *
 *  TODO: OleFlushClipboard needs to additionally handle TYMED_IStorage media
 *  by copying the storage into global memory. Subsequently the default
 *  data object exposed through OleGetClipboard must convert this TYMED_HGLOBAL
 *  back to TYMED_IStorage.
 */
HRESULT WINAPI OleFlushClipboard(void)
{
  IEnumFORMATETC* penumFormatetc = NULL;
  FORMATETC rgelt;
  HRESULT hr = S_OK;
  BOOL bClipboardOpen = FALSE;

  TRACE("()\n");

  OLEClipbrd_Initialize();

  /*
   * Already flushed or no source DataObject? Nothing to do.
   */
  if (!theOleClipboard->pIDataObjectSrc)
    return S_OK;

  if ( !(bClipboardOpen = OpenClipboard(theOleClipboard->hWndClipboard)) )
    HANDLE_ERROR( CLIPBRD_E_CANT_OPEN );

  /*
   * Render all HGLOBAL formats supported by the source into
   * the windows clipboard.
   */
  if ( FAILED( hr = IDataObject_EnumFormatEtc( theOleClipboard->pIDataObjectSrc,
                                               DATADIR_GET,
                                               &penumFormatetc) ))
  {
    HANDLE_ERROR( hr );
  }

  while ( S_OK == IEnumFORMATETC_Next(penumFormatetc, 1, &rgelt, NULL) )
  {
    if ( rgelt.tymed == TYMED_HGLOBAL )
    {
      CHAR szFmtName[80];
      TRACE("(cfFormat=%d:%s)\n", rgelt.cfFormat,
            GetClipboardFormatNameA(rgelt.cfFormat, szFmtName, sizeof(szFmtName)-1)
              ? szFmtName : "");

      if ( FAILED(OLEClipbrd_RenderFormat( theOleClipboard->pIDataObjectSrc, &rgelt )) )
        continue;
    }
  }

  IEnumFORMATETC_Release(penumFormatetc);

  hr = set_dataobject_format(NULL);

  IDataObject_Release(theOleClipboard->pIDataObjectSrc);
  theOleClipboard->pIDataObjectSrc = NULL;

CLEANUP:

  if ( bClipboardOpen && !CloseClipboard() )
    hr = CLIPBRD_E_CANT_CLOSE;

  return hr;
}


/***********************************************************************
 *           OleIsCurrentClipboard [OLE32.@]
 */
HRESULT WINAPI OleIsCurrentClipboard(IDataObject *pDataObject)
{
  TRACE("()\n");
  /*
   * Make sure we have a clipboard object
   */
  OLEClipbrd_Initialize();

  if (!theOleClipboard)
    return E_OUTOFMEMORY;

  if (pDataObject == NULL)
    return S_FALSE;

  return (pDataObject == theOleClipboard->pIDataObjectSrc) ? S_OK : S_FALSE;
}
