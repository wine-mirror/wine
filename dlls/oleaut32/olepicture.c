/*
 * OLE Picture object
 *
 * Implementation of OLE IPicture and related interfaces
 *
 * Copyright 2000 Huw D M Davies for CodeWeavers.
 * Copyright 2001 Marcus Meissner
 * Copyright 2008 Kirill K. Smirnov
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
 * BUGS
 *
 * Support PICTYPE_BITMAP and PICTYPE_ICON, although only bitmaps very well..
 * Lots of methods are just stubs.
 *
 *
 * NOTES (or things that msdn doesn't tell you)
 *
 * The width and height properties are returned in HIMETRIC units (0.01mm)
 * IPicture::Render also uses these to select a region of the src picture.
 * A bitmap's size is converted into these units by using the screen resolution
 * thus an 8x8 bitmap on a 96dpi screen has a size of 212x212 (8/96 * 2540).
 *
 */

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define COBJMACROS
#include "winerror.h"
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "ole2.h"
#include "olectl.h"
#include "oleauto.h"
#include "connpt.h"
#include "urlmon.h"
#include "initguid.h"
#include "wincodec.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(olepicture);

#define BITMAP_FORMAT_BMP   0x4d42 /* "BM" */
#define BITMAP_FORMAT_JPEG  0xd8ff
#define BITMAP_FORMAT_GIF   0x4947
#define BITMAP_FORMAT_PNG   0x5089
#define BITMAP_FORMAT_APM   0xcdd7

#pragma pack(push,1)

/* Header for Aldus Placable Metafiles - a standard metafile follows */
typedef struct _APM_HEADER
{
    DWORD key;
    WORD handle;
    SHORT left;
    SHORT top;
    SHORT right;
    SHORT bottom;
    WORD inch;
    DWORD reserved;
    WORD checksum;
} APM_HEADER;

typedef struct {
    BYTE bWidth;
    BYTE bHeight;
    BYTE bColorCount;
    BYTE bReserved;
    WORD xHotspot;
    WORD yHotspot;
    DWORD dwDIBSize;
    DWORD dwDIBOffset;
} CURSORICONFILEDIRENTRY;

typedef struct
{
    WORD                idReserved;
    WORD                idType;
    WORD                idCount;
    CURSORICONFILEDIRENTRY  idEntries[1];
} CURSORICONFILEDIR;

#pragma pack(pop)

typedef struct
{
    const WICPixelFormatGUID *guid;
    UINT bpp;
    BOOL indexed;
} WICFORMAT;

const WICFORMAT wicformats[] =
{
    {&GUID_WICPixelFormat1bppIndexed, 1, TRUE},
    {&GUID_WICPixelFormat4bppIndexed, 4, TRUE},
    {&GUID_WICPixelFormat8bppIndexed, 8, TRUE},
    {&GUID_WICPixelFormat24bppBGR,    24},
    {&GUID_WICPixelFormat32bppBGR,    32},
};

/*************************************************************************
 *  Declaration of implementation class
 */

typedef struct OLEPictureImpl {

  /*
   * IPicture handles IUnknown
   */

    IPicture                  IPicture_iface;
    IDispatch                 IDispatch_iface;
    IPersistStream            IPersistStream_iface;
    IConnectionPointContainer IConnectionPointContainer_iface;

  /* Object reference count */
    LONG ref;

  /* We own the object and must destroy it ourselves */
    BOOL fOwn;

  /* Picture description */
    PICTDESC desc;

  /* These are the pixel size of a bitmap */
    DWORD origWidth;
    DWORD origHeight;

  /* And these are the size of the picture converted into HIMETRIC units */
    OLE_XSIZE_HIMETRIC himetricWidth;
    OLE_YSIZE_HIMETRIC himetricHeight;

    IConnectionPoint *pCP;

    BOOL keepOrigFormat;
    HDC	hDCCur;
    HBITMAP stock_bitmap;

  /* Bitmap transparency mask */
    HBITMAP hbmMask;
    HBITMAP hbmXor;
    COLORREF rgbTrans;

  /* data */
    void* data;
    int datalen;
    BOOL bIsDirty;                  /* Set to TRUE if picture has changed */
    unsigned int loadtime_magic;    /* If a length header was found, saves value */
    unsigned int loadtime_format;   /* for PICTYPE_BITMAP only, keeps track of image format (GIF/BMP/JPEG) */
} OLEPictureImpl;

static inline OLEPictureImpl *impl_from_IPicture(IPicture *iface)
{
    return CONTAINING_RECORD(iface, OLEPictureImpl, IPicture_iface);
}

static inline OLEPictureImpl *impl_from_IDispatch( IDispatch *iface )
{
    return CONTAINING_RECORD(iface, OLEPictureImpl, IDispatch_iface);
}

static inline OLEPictureImpl *impl_from_IPersistStream( IPersistStream *iface )
{
    return CONTAINING_RECORD(iface, OLEPictureImpl, IPersistStream_iface);
}

static inline OLEPictureImpl *impl_from_IConnectionPointContainer( IConnectionPointContainer *iface )
{
    return CONTAINING_RECORD(iface, OLEPictureImpl, IConnectionPointContainer_iface);
}

static inline int get_dib_stride(int width, int bpp)
{
    return ((width * bpp + 31) >> 3) & ~3;
}

/*
 * Predeclare VTables.  They get initialized at the end.
 */
static const IPictureVtbl OLEPictureImpl_VTable;
static const IDispatchVtbl OLEPictureImpl_IDispatch_VTable;
static const IPersistStreamVtbl OLEPictureImpl_IPersistStream_VTable;
static const IConnectionPointContainerVtbl OLEPictureImpl_IConnectionPointContainer_VTable;

/* pixels to HIMETRIC units conversion */
static inline OLE_XSIZE_HIMETRIC xpixels_to_himetric(INT pixels, HDC hdc)
{
    return MulDiv(pixels, 2540, GetDeviceCaps(hdc, LOGPIXELSX));
}

static inline OLE_YSIZE_HIMETRIC ypixels_to_himetric(INT pixels, HDC hdc)
{
    return MulDiv(pixels, 2540, GetDeviceCaps(hdc, LOGPIXELSY));
}

/***********************************************************************
 * Implementation of the OLEPictureImpl class.
 */

static void OLEPictureImpl_SetBitmap(OLEPictureImpl *This)
{
  BITMAP bm;
  HDC hdcRef;

  TRACE("bitmap handle %p\n", This->desc.bmp.hbitmap);
  if(GetObjectW(This->desc.bmp.hbitmap, sizeof(bm), &bm) != sizeof(bm)) {
    ERR("GetObject fails\n");
    return;
  }
  This->origWidth = bm.bmWidth;
  This->origHeight = bm.bmHeight;

  TRACE("width %d, height %d, bpp %d\n", bm.bmWidth, bm.bmHeight, bm.bmBitsPixel);

  /* The width and height are stored in HIMETRIC units (0.01 mm),
     so we take our pixel width divide by pixels per inch and
     multiply by 25.4 * 100 */
  /* Should we use GetBitmapDimension if available? */
  hdcRef = CreateCompatibleDC(0);

  This->himetricWidth  = xpixels_to_himetric(bm.bmWidth, hdcRef);
  This->himetricHeight = ypixels_to_himetric(bm.bmHeight, hdcRef);
  This->stock_bitmap = GetCurrentObject( hdcRef, OBJ_BITMAP );

  This->loadtime_format = BITMAP_FORMAT_BMP;

  DeleteDC(hdcRef);
}

static void OLEPictureImpl_SetIcon(OLEPictureImpl * This)
{
    ICONINFO infoIcon;

    TRACE("icon handle %p\n", This->desc.icon.hicon);
    if (GetIconInfo(This->desc.icon.hicon, &infoIcon)) {
        HDC hdcRef;
        BITMAP bm;

        TRACE("bitmap handle for icon is %p\n", infoIcon.hbmColor);
        if(GetObjectW(infoIcon.hbmColor ? infoIcon.hbmColor : infoIcon.hbmMask, sizeof(bm), &bm) != sizeof(bm)) {
            ERR("GetObject fails on icon bitmap\n");
            return;
        }

        This->origWidth = bm.bmWidth;
        This->origHeight = infoIcon.hbmColor ? bm.bmHeight : bm.bmHeight / 2;
        /* see comment on HIMETRIC on OLEPictureImpl_SetBitmap() */
        hdcRef = GetDC(0);

        This->himetricWidth  = xpixels_to_himetric(This->origWidth, hdcRef);
        This->himetricHeight = ypixels_to_himetric(This->origHeight, hdcRef);

        ReleaseDC(0, hdcRef);

        DeleteObject(infoIcon.hbmMask);
        if (infoIcon.hbmColor) DeleteObject(infoIcon.hbmColor);
    } else {
        ERR("GetIconInfo() fails on icon %p\n", This->desc.icon.hicon);
    }
}

static void OLEPictureImpl_SetEMF(OLEPictureImpl *This)
{
    ENHMETAHEADER emh;

    GetEnhMetaFileHeader(This->desc.emf.hemf, sizeof(emh), &emh);

    This->origWidth = 0;
    This->origHeight = 0;
    This->himetricWidth = emh.rclFrame.right - emh.rclFrame.left;
    This->himetricHeight = emh.rclFrame.bottom - emh.rclFrame.top;
}

/************************************************************************
 * OLEPictureImpl_Construct
 *
 * This method will construct a new instance of the OLEPictureImpl
 * class.
 *
 * The caller of this method must release the object when it's
 * done with it.
 */
static HRESULT OLEPictureImpl_Construct(LPPICTDESC pictDesc, BOOL fOwn, OLEPictureImpl **pict)
{
  OLEPictureImpl *newObject;
  HRESULT hr;

  if (pictDesc)
      TRACE("(%p) type = %d\n", pictDesc, pictDesc->picType);

  /*
   * Allocate space for the object.
   */
  newObject = calloc(1, sizeof(OLEPictureImpl));
  if (!newObject)
    return E_OUTOFMEMORY;

  /*
   * Initialize the virtual function table.
   */
  newObject->IPicture_iface.lpVtbl = &OLEPictureImpl_VTable;
  newObject->IDispatch_iface.lpVtbl = &OLEPictureImpl_IDispatch_VTable;
  newObject->IPersistStream_iface.lpVtbl = &OLEPictureImpl_IPersistStream_VTable;
  newObject->IConnectionPointContainer_iface.lpVtbl = &OLEPictureImpl_IConnectionPointContainer_VTable;

  newObject->pCP = NULL;
  hr = CreateConnectionPoint((IUnknown*)&newObject->IPicture_iface, &IID_IPropertyNotifySink,
                        &newObject->pCP);
  if (hr != S_OK)
  {
    free(newObject);
    return hr;
  }

  /*
   * Start with one reference count. The caller of this function
   * must release the interface pointer when it is done.
   */
  newObject->ref	= 1;
  newObject->hDCCur	= 0;

  newObject->fOwn	= fOwn;

  /* dunno about original value */
  newObject->keepOrigFormat = TRUE;

  newObject->hbmMask = NULL;
  newObject->hbmXor = NULL;
  newObject->loadtime_magic = 0xdeadbeef;
  newObject->loadtime_format = 0;
  newObject->bIsDirty = FALSE;

  if (pictDesc) {
      newObject->desc = *pictDesc;

      switch(pictDesc->picType) {
      case PICTYPE_BITMAP:
	OLEPictureImpl_SetBitmap(newObject);
	break;

      case PICTYPE_METAFILE:
	TRACE("metafile handle %p\n", pictDesc->wmf.hmeta);
	newObject->himetricWidth = pictDesc->wmf.xExt;
	newObject->himetricHeight = pictDesc->wmf.yExt;
	break;

      case PICTYPE_NONE:
	/* not sure what to do here */
	newObject->himetricWidth = newObject->himetricHeight = 0;
	break;

      case PICTYPE_ICON:
        OLEPictureImpl_SetIcon(newObject);
        break;

      case PICTYPE_ENHMETAFILE:
        OLEPictureImpl_SetEMF(newObject);
        break;

      default:
        WARN("Unsupported type %d\n", pictDesc->picType);
        IPicture_Release(&newObject->IPicture_iface);
        return E_UNEXPECTED;
      }
  } else {
      newObject->desc.picType = PICTYPE_UNINITIALIZED;
  }

  TRACE("returning %p\n", newObject);
  *pict = newObject;
  return S_OK;
}

/************************************************************************
 * OLEPictureImpl_Destroy
 *
 * This method is called by the Release method when the reference
 * count goes down to 0. It will free all resources used by
 * this object.  */
static void OLEPictureImpl_Destroy(OLEPictureImpl* Obj)
{
  TRACE("(%p)\n", Obj);

  if (Obj->pCP)
    IConnectionPoint_Release(Obj->pCP);

  if(Obj->fOwn) { /* We need to destroy the picture */
    switch(Obj->desc.picType) {
    case PICTYPE_BITMAP:
      DeleteObject(Obj->desc.bmp.hbitmap);
      if (Obj->hbmMask != NULL) DeleteObject(Obj->hbmMask);
      if (Obj->hbmXor != NULL) DeleteObject(Obj->hbmXor);
      break;
    case PICTYPE_METAFILE:
      DeleteMetaFile(Obj->desc.wmf.hmeta);
      break;
    case PICTYPE_ICON:
      DestroyIcon(Obj->desc.icon.hicon);
      break;
    case PICTYPE_ENHMETAFILE:
      DeleteEnhMetaFile(Obj->desc.emf.hemf);
      break;
    case PICTYPE_NONE:
    case PICTYPE_UNINITIALIZED:
      /* Nothing to do */
      break;
    default:
      FIXME("Unsupported type %d - unable to delete\n", Obj->desc.picType);
      break;
    }
  }
  free(Obj->data);
  free(Obj);
}

static ULONG WINAPI OLEPictureImpl_AddRef(
  IPicture* iface)
{
  OLEPictureImpl *This = impl_from_IPicture(iface);
  ULONG refCount = InterlockedIncrement(&This->ref);

  TRACE("%p, refcount %lu.\n", iface, refCount);

  return refCount;
}

static ULONG WINAPI OLEPictureImpl_Release(
      IPicture* iface)
{
  OLEPictureImpl *This = impl_from_IPicture(iface);
  ULONG refCount = InterlockedDecrement(&This->ref);

  TRACE("%p, refcount %lu.\n", iface, refCount);

  if (!refCount) OLEPictureImpl_Destroy(This);

  return refCount;
}

static HRESULT WINAPI OLEPictureImpl_QueryInterface(
  IPicture*  iface,
  REFIID  riid,
  void**  ppvObject)
{
  OLEPictureImpl *This = impl_from_IPicture(iface);

  TRACE("(%p)->(%s, %p)\n", This, debugstr_guid(riid), ppvObject);

  if (!ppvObject)
    return E_INVALIDARG;

  *ppvObject = 0;

  if (IsEqualIID(&IID_IUnknown, riid) || IsEqualIID(&IID_IPicture, riid))
    *ppvObject = &This->IPicture_iface;
  else if (IsEqualIID(&IID_IDispatch, riid))
    *ppvObject = &This->IDispatch_iface;
  else if (IsEqualIID(&IID_IPictureDisp, riid))
    *ppvObject = &This->IDispatch_iface;
  else if (IsEqualIID(&IID_IPersist, riid) || IsEqualIID(&IID_IPersistStream, riid))
    *ppvObject = &This->IPersistStream_iface;
  else if (IsEqualIID(&IID_IConnectionPointContainer, riid))
    *ppvObject = &This->IConnectionPointContainer_iface;

  if (!*ppvObject)
  {
    FIXME("() : asking for unsupported interface %s\n",debugstr_guid(riid));
    return E_NOINTERFACE;
  }

  IPicture_AddRef(iface);

  return S_OK;
}

/***********************************************************************
 *    OLEPicture_SendNotify (internal)
 *
 * Sends notification messages of changed properties to any interested
 * connections.
 */
static void OLEPicture_SendNotify(OLEPictureImpl* this, DISPID dispID)
{
  IEnumConnections *pEnum;
  CONNECTDATA CD;

  if (IConnectionPoint_EnumConnections(this->pCP, &pEnum) != S_OK)
      return;
  while(IEnumConnections_Next(pEnum, 1, &CD, NULL) == S_OK) {
    IPropertyNotifySink *sink;

    IUnknown_QueryInterface(CD.pUnk, &IID_IPropertyNotifySink, (LPVOID)&sink);
    IPropertyNotifySink_OnChanged(sink, dispID);
    IPropertyNotifySink_Release(sink);
    IUnknown_Release(CD.pUnk);
  }
  IEnumConnections_Release(pEnum);
}

/************************************************************************
 * OLEPictureImpl_get_Handle
 */
static HRESULT WINAPI OLEPictureImpl_get_Handle(IPicture *iface,
						OLE_HANDLE *phandle)
{
  OLEPictureImpl *This = impl_from_IPicture(iface);
  TRACE("(%p)->(%p)\n", This, phandle);

  if(!phandle)
    return E_POINTER;

  switch(This->desc.picType) {
  case PICTYPE_NONE:
  case PICTYPE_UNINITIALIZED:
    *phandle = 0;
    break;
  case PICTYPE_BITMAP:
    *phandle = HandleToUlong(This->desc.bmp.hbitmap);
    break;
  case PICTYPE_METAFILE:
    *phandle = HandleToUlong(This->desc.wmf.hmeta);
    break;
  case PICTYPE_ICON:
    *phandle = HandleToUlong(This->desc.icon.hicon);
    break;
  case PICTYPE_ENHMETAFILE:
    *phandle = HandleToUlong(This->desc.emf.hemf);
    break;
  default:
    FIXME("Unimplemented type %d\n", This->desc.picType);
    return E_NOTIMPL;
  }
  TRACE("returning handle %08x\n", *phandle);
  return S_OK;
}

/************************************************************************
 * OLEPictureImpl_get_hPal
 */
static HRESULT WINAPI OLEPictureImpl_get_hPal(IPicture *iface,
					      OLE_HANDLE *phandle)
{
    OLEPictureImpl *This = impl_from_IPicture(iface);

    TRACE("(%p)->(%p)\n", This, phandle);

    if (!phandle) return E_POINTER;

    if (This->desc.picType == PICTYPE_BITMAP)
    {
        *phandle = HandleToUlong(This->desc.bmp.hpal);
        return S_OK;
    }

    return E_FAIL;
}

/************************************************************************
 * OLEPictureImpl_get_Type
 */
static HRESULT WINAPI OLEPictureImpl_get_Type(IPicture *iface,
					      short *ptype)
{
  OLEPictureImpl *This = impl_from_IPicture(iface);
  TRACE("(%p)->(%p): type is %d\n", This, ptype, This->desc.picType);

  if(!ptype)
    return E_POINTER;

  *ptype = This->desc.picType;
  return S_OK;
}

static HRESULT WINAPI OLEPictureImpl_get_Width(IPicture *iface, OLE_XSIZE_HIMETRIC *pwidth)
{
  OLEPictureImpl *This = impl_from_IPicture(iface);
  TRACE("%p, %p.\n", iface, pwidth);
  *pwidth = This->himetricWidth;
  return S_OK;
}

static HRESULT WINAPI OLEPictureImpl_get_Height(IPicture *iface, OLE_YSIZE_HIMETRIC *pheight)
{
  OLEPictureImpl *This = impl_from_IPicture(iface);
  TRACE("%p, %p.\n", iface, pheight);
  *pheight = This->himetricHeight;
  return S_OK;
}

static void render_masked_bitmap(OLEPictureImpl *This, HDC hdc,
    LONG x, LONG y, LONG cx, LONG cy, OLE_XPOS_HIMETRIC xSrc, OLE_YPOS_HIMETRIC ySrc,
    OLE_XSIZE_HIMETRIC cxSrc, OLE_YSIZE_HIMETRIC cySrc, HBITMAP hbmMask, HBITMAP hbmXor)
{
    HDC hdcBmp;

    /* Set a mapping mode that maps bitmap pixels into HIMETRIC units.
     * NB y-axis gets flipped
     */

    hdcBmp = CreateCompatibleDC(0);
    SetMapMode(hdcBmp, MM_ANISOTROPIC);
    SetWindowOrgEx(hdcBmp, 0, 0, NULL);
    SetWindowExtEx(hdcBmp, This->himetricWidth, This->himetricHeight, NULL);
    SetViewportOrgEx(hdcBmp, 0, This->origHeight, NULL);
    SetViewportExtEx(hdcBmp, This->origWidth, -This->origHeight, NULL);

    if (hbmMask)
    {
        SetBkColor(hdc, RGB(255, 255, 255));
        SetTextColor(hdc, RGB(0, 0, 0));

        SelectObject(hdcBmp, hbmMask);
        StretchBlt(hdc, x, y, cx, cy, hdcBmp, xSrc, ySrc, cxSrc, cySrc, SRCAND);

        if (hbmXor)
        {
            SelectObject(hdcBmp, hbmXor);
            StretchBlt(hdc, x, y, cx, cy, hdcBmp, xSrc, ySrc, cxSrc, cySrc, SRCPAINT);
        }
        else StretchBlt(hdc, x, y, cx, cy, hdcBmp, xSrc, ySrc - This->himetricHeight,
                        cxSrc, cySrc, SRCPAINT);
    }
    else
    {
        SelectObject(hdcBmp, hbmXor);
        StretchBlt(hdc, x, y, cx, cy, hdcBmp, xSrc, ySrc, cxSrc, cySrc, SRCCOPY);
    }

    DeleteDC(hdcBmp);
}

/************************************************************************
 * OLEPictureImpl_Render
 */
static HRESULT WINAPI OLEPictureImpl_Render(IPicture *iface, HDC hdc,
					    LONG x, LONG y, LONG cx, LONG cy,
					    OLE_XPOS_HIMETRIC xSrc,
					    OLE_YPOS_HIMETRIC ySrc,
					    OLE_XSIZE_HIMETRIC cxSrc,
					    OLE_YSIZE_HIMETRIC cySrc,
					    LPCRECT prcWBounds)
{
  OLEPictureImpl *This = impl_from_IPicture(iface);
  TRACE("%p, %p, (%ld,%ld), (%ld,%ld), (%ld,%ld), (%ld,%ld), %p)\n", iface, hdc, x, y, cx, cy, xSrc, ySrc,
          cxSrc, cySrc, prcWBounds);
  if(prcWBounds)
  TRACE("prcWBounds %s\n", wine_dbgstr_rect(prcWBounds));

  if(cx == 0 || cy == 0 || cxSrc == 0 || cySrc == 0){
    return CTL_E_INVALIDPROPERTYVALUE;
  }

  /*
   * While the documentation suggests this to be here (or after rendering?)
   * it does cause an endless recursion in my sample app. -MM 20010804
  OLEPicture_SendNotify(This,DISPID_PICT_RENDER);
   */

  switch(This->desc.picType) {
  case PICTYPE_UNINITIALIZED:
  case PICTYPE_NONE:
    /* nothing to do */
    return S_OK;
  case PICTYPE_BITMAP:
  {
    HBITMAP hbmMask, hbmXor;

    if (This->hbmMask)
    {
        hbmMask = This->hbmMask;
        hbmXor = This->hbmXor;
    }
    else
    {
        hbmMask = 0;
        hbmXor = This->desc.bmp.hbitmap;
    }

    render_masked_bitmap(This, hdc, x, y, cx, cy, xSrc, ySrc, cxSrc, cySrc, hbmMask, hbmXor);
    break;
  }

  case PICTYPE_ICON:
  {
    ICONINFO info;

    if (!GetIconInfo(This->desc.icon.hicon, &info))
        return E_FAIL;

    render_masked_bitmap(This, hdc, x, y, cx, cy, xSrc, ySrc, cxSrc, cySrc, info.hbmMask, info.hbmColor);

    DeleteObject(info.hbmMask);
    if (info.hbmColor) DeleteObject(info.hbmColor);
    break;
  }

  case PICTYPE_METAFILE:
  {
    POINT prevOrg, prevWndOrg;
    SIZE prevExt, prevWndExt;
    int oldmode;

    /* Render the WMF to the appropriate location by setting the
       appropriate ratio between "device units" and "logical units" */
    oldmode = SetMapMode(hdc, MM_ANISOTROPIC);
    /* For the "source rectangle" the y-axis must be inverted */
    SetWindowOrgEx(hdc, xSrc, This->himetricHeight-ySrc, &prevWndOrg);
    SetWindowExtEx(hdc, cxSrc, -cySrc, &prevWndExt);
    /* For the "destination rectangle" no inversion is necessary */
    SetViewportOrgEx(hdc, x, y, &prevOrg);
    SetViewportExtEx(hdc, cx, cy, &prevExt);

    if (!PlayMetaFile(hdc, This->desc.wmf.hmeta))
        ERR("PlayMetaFile failed!\n");

    /* We're done, restore the DC to the previous settings for converting
       logical units to device units */
    SetWindowExtEx(hdc, prevWndExt.cx, prevWndExt.cy, NULL);
    SetWindowOrgEx(hdc, prevWndOrg.x, prevWndOrg.y, NULL);
    SetViewportExtEx(hdc, prevExt.cx, prevExt.cy, NULL);
    SetViewportOrgEx(hdc, prevOrg.x, prevOrg.y, NULL);
    SetMapMode(hdc, oldmode);
    break;
  }

  case PICTYPE_ENHMETAFILE:
  {
    RECT rc = { x, y, x + cx, y + cy };
    PlayEnhMetaFile(hdc, This->desc.emf.hemf, &rc);
    break;
  }

  default:
    FIXME("type %d not implemented\n", This->desc.picType);
    return E_NOTIMPL;
  }
  return S_OK;
}

/************************************************************************
 * OLEPictureImpl_set_hPal
 */
static HRESULT WINAPI OLEPictureImpl_set_hPal(IPicture *iface,
					      OLE_HANDLE hpal)
{
    OLEPictureImpl *This = impl_from_IPicture(iface);

    TRACE("(%p)->(%08x)\n", This, hpal);

    if (This->desc.picType == PICTYPE_BITMAP)
    {
        This->desc.bmp.hpal = ULongToHandle(hpal);
        OLEPicture_SendNotify(This,DISPID_PICT_HPAL);
        return S_OK;
    }

    return E_FAIL;
}

/************************************************************************
 * OLEPictureImpl_get_CurDC
 */
static HRESULT WINAPI OLEPictureImpl_get_CurDC(IPicture *iface,
					       HDC *phdc)
{
  OLEPictureImpl *This = impl_from_IPicture(iface);
  TRACE("(%p), returning %p\n", This, This->hDCCur);
  if (phdc) *phdc = This->hDCCur;
  return S_OK;
}

/************************************************************************
 * OLEPictureImpl_SelectPicture
 */
static HRESULT WINAPI OLEPictureImpl_SelectPicture(IPicture *iface,
						   HDC hdcIn,
						   HDC *phdcOut,
						   OLE_HANDLE *phbmpOut)
{
  OLEPictureImpl *This = impl_from_IPicture(iface);
  TRACE("(%p)->(%p, %p, %p)\n", This, hdcIn, phdcOut, phbmpOut);
  if (This->desc.picType == PICTYPE_BITMAP) {
      if (phdcOut)
	  *phdcOut = This->hDCCur;
      if (This->hDCCur) SelectObject(This->hDCCur,This->stock_bitmap);
      if (hdcIn) SelectObject(hdcIn,This->desc.bmp.hbitmap);
      This->hDCCur = hdcIn;
      if (phbmpOut)
	  *phbmpOut = HandleToUlong(This->desc.bmp.hbitmap);
      return S_OK;
  } else {
      FIXME("Don't know how to select picture type %d\n",This->desc.picType);
      return E_FAIL;
  }
}

/************************************************************************
 * OLEPictureImpl_get_KeepOriginalFormat
 */
static HRESULT WINAPI OLEPictureImpl_get_KeepOriginalFormat(IPicture *iface,
							    BOOL *pfKeep)
{
  OLEPictureImpl *This = impl_from_IPicture(iface);
  TRACE("(%p)->(%p)\n", This, pfKeep);
  if (!pfKeep)
      return E_POINTER;
  *pfKeep = This->keepOrigFormat;
  return S_OK;
}

/************************************************************************
 * OLEPictureImpl_put_KeepOriginalFormat
 */
static HRESULT WINAPI OLEPictureImpl_put_KeepOriginalFormat(IPicture *iface,
							    BOOL keep)
{
  OLEPictureImpl *This = impl_from_IPicture(iface);
  TRACE("(%p)->(%d)\n", This, keep);
  This->keepOrigFormat = keep;
  /* FIXME: what DISPID notification here? */
  return S_OK;
}

/************************************************************************
 * OLEPictureImpl_PictureChanged
 */
static HRESULT WINAPI OLEPictureImpl_PictureChanged(IPicture *iface)
{
  OLEPictureImpl *This = impl_from_IPicture(iface);
  TRACE("(%p)->()\n", This);
  OLEPicture_SendNotify(This,DISPID_PICT_HANDLE);
  This->bIsDirty = TRUE;
  return S_OK;
}

/************************************************************************
 * OLEPictureImpl_get_Attributes
 */
static HRESULT WINAPI OLEPictureImpl_get_Attributes(IPicture *iface,
						    DWORD *pdwAttr)
{
  OLEPictureImpl *This = impl_from_IPicture(iface);
  TRACE("(%p)->(%p).\n", This, pdwAttr);

  if(!pdwAttr)
    return E_POINTER;

  *pdwAttr = 0;
  switch (This->desc.picType) {
  case PICTYPE_UNINITIALIZED:
  case PICTYPE_NONE: break;
  case PICTYPE_BITMAP: 	if (This->hbmMask) *pdwAttr = PICTURE_TRANSPARENT; break;	/* not 'truly' scalable, see MSDN. */
  case PICTYPE_ICON: *pdwAttr     = PICTURE_TRANSPARENT;break;
  case PICTYPE_ENHMETAFILE: /* fall through */
  case PICTYPE_METAFILE: *pdwAttr = PICTURE_TRANSPARENT|PICTURE_SCALABLE;break;
  default:FIXME("Unknown pictype %d\n",This->desc.picType);break;
  }
  return S_OK;
}


/************************************************************************
 *    IConnectionPointContainer
 */
static HRESULT WINAPI OLEPictureImpl_IConnectionPointContainer_QueryInterface(
  IConnectionPointContainer* iface,
  REFIID riid,
  VOID** ppvoid)
{
  OLEPictureImpl *This = impl_from_IConnectionPointContainer(iface);

  return IPicture_QueryInterface(&This->IPicture_iface,riid,ppvoid);
}

static ULONG WINAPI OLEPictureImpl_IConnectionPointContainer_AddRef(
  IConnectionPointContainer* iface)
{
  OLEPictureImpl *This = impl_from_IConnectionPointContainer(iface);

  return IPicture_AddRef(&This->IPicture_iface);
}

static ULONG WINAPI OLEPictureImpl_IConnectionPointContainer_Release(
  IConnectionPointContainer* iface)
{
  OLEPictureImpl *This = impl_from_IConnectionPointContainer(iface);

  return IPicture_Release(&This->IPicture_iface);
}

static HRESULT WINAPI OLEPictureImpl_EnumConnectionPoints(
  IConnectionPointContainer* iface,
  IEnumConnectionPoints** ppEnum)
{
  OLEPictureImpl *This = impl_from_IConnectionPointContainer(iface);

  FIXME("(%p,%p), stub!\n",This,ppEnum);
  return E_NOTIMPL;
}

static HRESULT WINAPI OLEPictureImpl_FindConnectionPoint(
  IConnectionPointContainer* iface,
  REFIID riid,
  IConnectionPoint **ppCP)
{
  OLEPictureImpl *This = impl_from_IConnectionPointContainer(iface);
  TRACE("(%p,%s,%p)\n",This,debugstr_guid(riid),ppCP);
  if (!ppCP)
      return E_POINTER;
  *ppCP = NULL;
  if (IsEqualGUID(riid,&IID_IPropertyNotifySink))
      return IConnectionPoint_QueryInterface(This->pCP, &IID_IConnectionPoint, (void**)ppCP);
  FIXME("no connection point for %s\n",debugstr_guid(riid));
  return CONNECT_E_NOCONNECTION;
}


/************************************************************************
 *    IPersistStream
 */

/************************************************************************
 * OLEPictureImpl_IPersistStream_QueryInterface (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static HRESULT WINAPI OLEPictureImpl_IPersistStream_QueryInterface(
  IPersistStream* iface,
  REFIID     riid,
  VOID**     ppvoid)
{
  OLEPictureImpl *This = impl_from_IPersistStream(iface);

  return IPicture_QueryInterface(&This->IPicture_iface, riid, ppvoid);
}

/************************************************************************
 * OLEPictureImpl_IPersistStream_AddRef (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static ULONG WINAPI OLEPictureImpl_IPersistStream_AddRef(
  IPersistStream* iface)
{
  OLEPictureImpl *This = impl_from_IPersistStream(iface);

  return IPicture_AddRef(&This->IPicture_iface);
}

/************************************************************************
 * OLEPictureImpl_IPersistStream_Release (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static ULONG WINAPI OLEPictureImpl_IPersistStream_Release(
  IPersistStream* iface)
{
  OLEPictureImpl *This = impl_from_IPersistStream(iface);

  return IPicture_Release(&This->IPicture_iface);
}

/************************************************************************
 * OLEPictureImpl_IPersistStream_GetClassID
 */
static HRESULT WINAPI OLEPictureImpl_GetClassID(
  IPersistStream* iface,CLSID* pClassID)
{
  TRACE("(%p)\n", pClassID);
  *pClassID = CLSID_StdPicture;
  return S_OK;
}

/************************************************************************
 * OLEPictureImpl_IPersistStream_IsDirty
 */
static HRESULT WINAPI OLEPictureImpl_IsDirty(
  IPersistStream* iface)
{
  OLEPictureImpl *This = impl_from_IPersistStream(iface);
  FIXME("(%p),stub!\n",This);
  return E_NOTIMPL;
}

HRESULT WINAPI WICCreateImagingFactory_Proxy(UINT, IWICImagingFactory**);

static HRESULT OLEPictureImpl_LoadWICSource(OLEPictureImpl *This, IWICImagingFactory *factory, IWICBitmapSource *src)
{
    HRESULT hr;
    BITMAPINFOHEADER bih;
    BITMAPINFO *info, *dyn_info = NULL;
    UINT width, height;
    UINT stride, buffersize;
    BYTE *bits, *mask = NULL;
    WICRect rc;
    WICPixelFormatGUID guid;
    IWICBitmapSource *real_source;
    IWICPalette *palette;
    UINT x, y, i;
    COLORREF white = RGB(255, 255, 255), black = RGB(0, 0, 0);
    BOOL has_alpha=FALSE, indexed=FALSE;

    hr = IWICBitmapSource_GetPixelFormat(src, &guid);
    if (FAILED(hr)) return hr;

    for (i = 0; i < ARRAY_SIZE(wicformats); i++)
    {
        if (IsEqualGUID(&guid, wicformats[i].guid))
        {
            bih.biBitCount = wicformats[i].bpp;
            indexed = wicformats[i].indexed;
            break;
        }
    }

    if (i == ARRAY_SIZE(wicformats))
    {
        memcpy(&guid, &GUID_WICPixelFormat32bppBGRA, sizeof(guid));
        bih.biBitCount = 32;
    }

    hr = WICConvertBitmapSource(&guid, src, &real_source);
    if (FAILED(hr)) return hr;

    hr = IWICBitmapSource_GetSize(real_source, &width, &height);
    if (FAILED(hr)) goto end;

    bih.biSize = sizeof(bih);
    bih.biWidth = width;
    bih.biHeight = -height;
    bih.biPlanes = 1;
    bih.biCompression = BI_RGB;
    bih.biSizeImage = 0;
    bih.biXPelsPerMeter = 4085; /* olepicture ignores the stored resolution */
    bih.biYPelsPerMeter = 4085;
    bih.biClrUsed = 0;
    bih.biClrImportant = 0;

    stride = get_dib_stride(width, bih.biBitCount);
    buffersize = stride * height;

    mask = malloc(buffersize);
    if (!mask)
    {
        hr = E_OUTOFMEMORY;
        goto end;
    }

    if (indexed)
    {
        UINT palette_size = 1 << bih.biBitCount, source_colors;

        info = dyn_info = malloc(FIELD_OFFSET(BITMAPINFO, bmiColors[palette_size]));
        if (!info)
        {
            hr = E_OUTOFMEMORY;
            goto end;
        }

        info->bmiHeader = bih;

        hr = IWICImagingFactory_CreatePalette(factory, &palette);

        if (SUCCEEDED(hr))
        {
            hr = IWICBitmapSource_CopyPalette(real_source, palette);

            if (SUCCEEDED(hr))
                hr = IWICPalette_GetColors(palette, palette_size, (WICColor*)&info->bmiColors[0], &source_colors);

            IWICPalette_Release(palette);
        }

        if (FAILED(hr))
            goto end;

        if (source_colors < palette_size)
            memset(&info->bmiColors[source_colors], 0, sizeof(RGBQUAD) * (palette_size - source_colors));
    }
    else
        info = (BITMAPINFO*)&bih;

    This->desc.bmp.hbitmap = CreateDIBSection(0, info, DIB_RGB_COLORS, (void **)&bits, NULL, 0);
    if (This->desc.bmp.hbitmap == 0)
    {
        hr = E_FAIL;
        goto end;
    }

    rc.X = 0;
    rc.Y = 0;
    rc.Width = width;
    rc.Height = height;
    hr = IWICBitmapSource_CopyPixels(real_source, &rc, stride, buffersize, bits);
    if (FAILED(hr))
    {
        DeleteObject(This->desc.bmp.hbitmap);
        goto end;
    }

    This->desc.picType = PICTYPE_BITMAP;
    OLEPictureImpl_SetBitmap(This);

    if (IsEqualGUID(&guid, &GUID_WICPixelFormat32bppBGRA))
    {
        /* set transparent pixels to black, all others to white */
        for(y = 0; y < height; y++){
            for(x = 0; x < width; x++){
                DWORD *pixel = (DWORD*)(bits + stride*y + 4*x);
                if((*pixel & 0x80000000) == 0)
                {
                    has_alpha = TRUE;
                    *(DWORD *)(mask + stride * y + 4 * x) = black;
                }
                else
                    *(DWORD *)(mask + stride * y + 4 * x) = white;
            }
        }
    }

    if (has_alpha)
    {
        HDC hdcref, hdcBmp, hdcXor, hdcMask;
        HBITMAP hbmoldBmp, hbmoldXor, hbmoldMask;

        hdcref = GetDC(0);

        This->hbmXor = CreateDIBitmap(
            hdcref,
            &bih,
            CBM_INIT,
            mask,
            (BITMAPINFO*)&bih,
            DIB_RGB_COLORS
        );

        This->hbmMask = CreateBitmap(width,-height,1,1,NULL);
        hdcBmp = CreateCompatibleDC(NULL);
        hdcXor = CreateCompatibleDC(NULL);
        hdcMask = CreateCompatibleDC(NULL);

        hbmoldBmp = SelectObject(hdcBmp,This->desc.bmp.hbitmap);
        hbmoldXor = SelectObject(hdcXor,This->hbmXor);
        hbmoldMask = SelectObject(hdcMask,This->hbmMask);

        SetBkColor(hdcXor,black);
        BitBlt(hdcMask,0,0,width,height,hdcXor,0,0,SRCCOPY);
        BitBlt(hdcXor,0,0,width,height,hdcBmp,0,0,SRCAND);

        SelectObject(hdcBmp,hbmoldBmp);
        SelectObject(hdcXor,hbmoldXor);
        SelectObject(hdcMask,hbmoldMask);

        DeleteDC(hdcBmp);
        DeleteDC(hdcXor);
        DeleteDC(hdcMask);
        ReleaseDC(0, hdcref);
    }

end:
    free(mask);
    free(dyn_info);
    IWICBitmapSource_Release(real_source);
    return hr;
}

static HRESULT OLEPictureImpl_LoadWICDecoder(OLEPictureImpl *This, REFGUID format_guid, BYTE *xbuf, ULONG xread)
{
    HRESULT hr;
    IWICImagingFactory *factory;
    IWICBitmapDecoder *decoder;
    IWICBitmapFrameDecode *framedecode;
    IWICStream *stream;

    hr = WICCreateImagingFactory_Proxy(WINCODEC_SDK_VERSION, &factory);
    if (FAILED(hr))
        return hr;

    hr = IWICImagingFactory_CreateStream(factory, &stream);

    if (SUCCEEDED(hr)) /* created stream */
    {
        hr = IWICStream_InitializeFromMemory(stream, xbuf, xread);

        if (SUCCEEDED(hr)) /* initialized stream */
        {
            hr = IWICImagingFactory_CreateDecoder(factory, format_guid, &GUID_VendorMicrosoftBuiltIn, &decoder);
            if (SUCCEEDED(hr)) /* created decoder */
            {
                hr = IWICBitmapDecoder_Initialize(decoder, (IStream*)stream, WICDecodeMetadataCacheOnLoad);

                if (SUCCEEDED(hr)) /* initialized decoder */
                    hr = IWICBitmapDecoder_GetFrame(decoder, 0, &framedecode);

                IWICBitmapDecoder_Release(decoder);
            }
        }

        IWICStream_Release(stream);
    }

    if (SUCCEEDED(hr)) /* got framedecode */
    {
        hr = OLEPictureImpl_LoadWICSource(This, factory, (IWICBitmapSource*)framedecode);
        IWICBitmapFrameDecode_Release(framedecode);
    }

    IWICImagingFactory_Release(factory);

    return hr;
}

/*****************************************************
*   start of Icon-specific code
*/

static HRESULT OLEPictureImpl_LoadIcon(OLEPictureImpl *This, BYTE *xbuf, ULONG xread)
{
    HICON hicon;
    CURSORICONFILEDIR	*cifd = (CURSORICONFILEDIR*)xbuf;
    HDC hdcRef;
    int	i;

    TRACE("(this %p, xbuf %p, xread %lu)\n", This, xbuf, xread);

    /*
    FIXME("icon.idReserved=%d\n",cifd->idReserved);
    FIXME("icon.idType=%d\n",cifd->idType);
    FIXME("icon.idCount=%d\n",cifd->idCount);

    for (i=0;i<cifd->idCount;i++) {
	FIXME("[%d] width %d\n",i,cifd->idEntries[i].bWidth);
	FIXME("[%d] height %d\n",i,cifd->idEntries[i].bHeight);
	FIXME("[%d] bColorCount %d\n",i,cifd->idEntries[i].bColorCount);
	FIXME("[%d] bReserved %d\n",i,cifd->idEntries[i].bReserved);
	FIXME("[%d] xHotspot %d\n",i,cifd->idEntries[i].xHotspot);
	FIXME("[%d] yHotspot %d\n",i,cifd->idEntries[i].yHotspot);
	FIXME("[%d] dwDIBSize %d\n",i,cifd->idEntries[i].dwDIBSize);
	FIXME("[%d] dwDIBOffset %d\n",i,cifd->idEntries[i].dwDIBOffset);
    }
    */

    /* Need at least one icon to do something. */
    if (!cifd->idCount)
    {
        ERR("Invalid icon count of zero.\n");
        return E_FAIL;
    }
    i=0;
    /* If we have more than one icon, try to find the best.
     * this currently means '32 pixel wide'.
     */
    if (cifd->idCount!=1) {
	for (i=0;i<cifd->idCount;i++) {
	    if (cifd->idEntries[i].bWidth == 32)
		break;
	}
	if (i==cifd->idCount) i=0;
    }
    if (xread < cifd->idEntries[i].dwDIBOffset + cifd->idEntries[i].dwDIBSize)
    {
        ERR("Icon data address %lu is over %lu bytes available.\n",
            cifd->idEntries[i].dwDIBOffset + cifd->idEntries[i].dwDIBSize, xread);
        return E_FAIL;
    }
    if (cifd->idType == 2)
    {
        BYTE *buf = malloc(cifd->idEntries[i].dwDIBSize + 4);
        memcpy(buf, &cifd->idEntries[i].xHotspot, 4);
        memcpy(buf + 4, xbuf+cifd->idEntries[i].dwDIBOffset, cifd->idEntries[i].dwDIBSize);
        hicon = CreateIconFromResourceEx(
		    buf,
		    cifd->idEntries[i].dwDIBSize + 4,
		    FALSE, /* is cursor */
		    0x00030000,
		    cifd->idEntries[i].bWidth,
		    cifd->idEntries[i].bHeight,
		    0
	);
	free(buf);
    }
    else
    {
        hicon = CreateIconFromResourceEx(
		    xbuf+cifd->idEntries[i].dwDIBOffset,
		    cifd->idEntries[i].dwDIBSize,
		    TRUE, /* is icon */
		    0x00030000,
		    cifd->idEntries[i].bWidth,
		    cifd->idEntries[i].bHeight,
		    0
	);
    }
    if (!hicon) {
	ERR("CreateIcon failed.\n");
	return E_FAIL;
    } else {
	This->desc.picType = PICTYPE_ICON;
	This->desc.icon.hicon = hicon;
	This->origWidth = cifd->idEntries[i].bWidth;
	This->origHeight = cifd->idEntries[i].bHeight;
	hdcRef = CreateCompatibleDC(0);
	This->himetricWidth = xpixels_to_himetric(cifd->idEntries[i].bWidth, hdcRef);
	This->himetricHeight= ypixels_to_himetric(cifd->idEntries[i].bHeight, hdcRef);
	DeleteDC(hdcRef);
	return S_OK;
    }
}

static HRESULT OLEPictureImpl_LoadEnhMetafile(OLEPictureImpl *This,
                                              const BYTE *data, ULONG size)
{
    HENHMETAFILE hemf;
    ENHMETAHEADER hdr;

    hemf = SetEnhMetaFileBits(size, data);
    if (!hemf) return E_FAIL;

    GetEnhMetaFileHeader(hemf, sizeof(hdr), &hdr);

    This->desc.picType = PICTYPE_ENHMETAFILE;
    This->desc.emf.hemf = hemf;

    This->origWidth = 0;
    This->origHeight = 0;
    This->himetricWidth = hdr.rclFrame.right - hdr.rclFrame.left;
    This->himetricHeight = hdr.rclFrame.bottom - hdr.rclFrame.top;

    return S_OK;
}

static HRESULT OLEPictureImpl_LoadAPM(OLEPictureImpl *This,
                                      const BYTE *data, ULONG size)
{
    const APM_HEADER *header = (const APM_HEADER *)data;
    HMETAFILE hmf;

    if (size < sizeof(APM_HEADER))
        return E_FAIL;
    if (header->key != 0x9ac6cdd7)
        return E_FAIL;

    /* SetMetaFileBitsEx performs data check on its own */
    hmf = SetMetaFileBitsEx(size - sizeof(*header), data + sizeof(*header));
    if (!hmf) return E_FAIL;

    This->desc.picType = PICTYPE_METAFILE;
    This->desc.wmf.hmeta = hmf;
    This->desc.wmf.xExt = 0;
    This->desc.wmf.yExt = 0;

    This->origWidth = 0;
    This->origHeight = 0;
    This->himetricWidth = MulDiv((INT)header->right - header->left, 2540, header->inch);
    This->himetricHeight = MulDiv((INT)header->bottom - header->top, 2540, header->inch);
    return S_OK;
}

/************************************************************************
 * OLEPictureImpl_IPersistStream_Load (IUnknown)
 *
 * Loads the binary data from the IStream. Starts at current position.
 * There appears to be an 2 DWORD header:
 * 	DWORD magic;
 * 	DWORD len;
 *
 * Currently implemented: BITMAP, ICON, CURSOR, JPEG, GIF, WMF, EMF
 */
static HRESULT WINAPI OLEPictureImpl_Load(IPersistStream* iface, IStream *pStm) {
  HRESULT	hr;
  BOOL		headerisdata;
  BOOL		statfailed = FALSE;
  ULONG		xread, toread;
  ULONG 	headerread;
  BYTE 		*xbuf;
  DWORD		header[2];
  WORD		magic;
  STATSTG       statstg;
  OLEPictureImpl *This = impl_from_IPersistStream(iface);
  
  TRACE("(%p,%p)\n",This,pStm);

  /****************************************************************************************
   * Part 1: Load the data
   */
  /* Sometimes we have a header, sometimes we don't. Apply some guesses to find
   * out whether we do.
   *
   * UPDATE: the IStream can be mapped to a plain file instead of a stream in a
   * compound file. This may explain most, if not all, of the cases of "no
   * header", and the header validation should take this into account.
   * At least in Visual Basic 6, resource streams, valid headers are
   *    header[0] == "lt\0\0",
   *    header[1] == length_of_stream.
   *
   * Also handle streams where we do not have a working "Stat" method by
   * reading all data until the end of the stream.
   */
  hr = IStream_Stat(pStm,&statstg,STATFLAG_NONAME);
  if (hr != S_OK) {
      TRACE("stat failed with hres %#lx, proceeding to read all data.\n",hr);
      statfailed = TRUE;
      /* we will read at least 8 byte ... just right below */
      statstg.cbSize.QuadPart = 8;
  }

  toread = 0;
  headerread = 0;
  headerisdata = FALSE;
  do {
      hr = IStream_Read(pStm, header, 8, &xread);
      if (hr != S_OK || xread!=8) {
          ERR("Failure while reading picture header (hr is %#lx, nread is %ld).\n",hr,xread);
          return (hr?hr:E_FAIL);
      }
      headerread += xread;
      xread = 0;

      if (!memcmp(&(header[0]),"lt\0\0", 4) && (statfailed || (header[1] + headerread <= statstg.cbSize.QuadPart))) {
          if (toread != 0 && toread != header[1]) 
              FIXME("varying lengths of image data (prev=%lu curr=%lu), only last one will be used\n",
                  toread, header[1]);
          toread = header[1];
          if (statfailed)
          {
              statstg.cbSize.QuadPart = header[1] + 8;
              statfailed = FALSE;
          }
          if (toread == 0) break;
      } else {
          if (!memcmp(&(header[0]), "GIF8",     4) ||   /* GIF header */
              !memcmp(&(header[0]), "BM",       2) ||   /* BMP header */
              !memcmp(&(header[0]), "\xff\xd8", 2) ||   /* JPEG header */
              (header[0] == EMR_HEADER)            ||   /* EMF header */
              (header[0] == 0x10000)               ||   /* icon: idReserved 0, idType 1 */
              (header[0] == 0x20000)               ||   /* cursor: idReserved 0, idType 2 */
              (header[1] > statstg.cbSize.QuadPart)||   /* invalid size */
              (header[1]==0)
          ) {/* Found start of bitmap data */
              headerisdata = TRUE;
              if (toread == 0) 
              	  toread = statstg.cbSize.QuadPart-8;
              else toread -= 8;
              xread = 8;
          } else {
              FIXME("Unknown stream header magic: %#lx.\n", header[0]);
              toread = header[1];
          }
      }
  } while (!headerisdata);

  if (statfailed) { /* we don't know the size ... read all we get */
      unsigned int sizeinc = 4096;
      unsigned int origsize = sizeinc;
      ULONG nread = 42;

      TRACE("Reading all data from stream.\n");
      xbuf = calloc(1, origsize);
      if (headerisdata)
          memcpy (xbuf, header, 8);
      while (1) {
          while (xread < origsize) {
              hr = IStream_Read(pStm,xbuf+xread,origsize-xread,&nread);
              xread += nread;
              if (hr != S_OK || !nread)
                  break;
          }
          if (!nread || hr != S_OK) /* done, or error */
              break;
          if (xread == origsize) {
              sizeinc = 2*sizeinc; /* exponential increase */
              xbuf = realloc(xbuf, origsize + sizeinc);
              memset(xbuf + origsize, 0, sizeinc);
              origsize += sizeinc;
          }
      }
      if (hr != S_OK)
          TRACE("hr in no-stat loader case is %#lx.\n", hr);
      TRACE("loaded %ld bytes.\n", xread);
      This->datalen = xread;
      This->data    = xbuf;
  } else {
      This->datalen = toread+(headerisdata?8:0);
      xbuf = This->data = calloc(1, This->datalen);
      if (!xbuf)
          return E_OUTOFMEMORY;

      if (headerisdata)
          memcpy (xbuf, header, 8);

      while (xread < This->datalen) {
          ULONG nread;
          hr = IStream_Read(pStm,xbuf+xread,This->datalen-xread,&nread);
          xread += nread;
          if (hr != S_OK || !nread)
              break;
      }
      if (xread != This->datalen)
          ERR("Could only read %ld of %d bytes out of stream?\n", xread, This->datalen);
  }
  if (This->datalen == 0) { /* Marks the "NONE" picture */
      This->desc.picType = PICTYPE_NONE;
      return S_OK;
  }


  /****************************************************************************************
   * Part 2: Process the loaded data
   */

  magic = xbuf[0] + (xbuf[1]<<8);
  This->loadtime_format = magic;

  switch (magic) {
  case BITMAP_FORMAT_GIF: /* GIF */
    hr = OLEPictureImpl_LoadWICDecoder(This, &GUID_ContainerFormatGif, xbuf, xread);
    break;
  case BITMAP_FORMAT_JPEG: /* JPEG */
    hr = OLEPictureImpl_LoadWICDecoder(This, &GUID_ContainerFormatJpeg, xbuf, xread);
    break;
  case BITMAP_FORMAT_BMP: /* Bitmap */
    hr = OLEPictureImpl_LoadWICDecoder(This, &GUID_ContainerFormatBmp, xbuf, xread);
    break;
  case BITMAP_FORMAT_PNG: /* PNG */
    hr = OLEPictureImpl_LoadWICDecoder(This, &GUID_ContainerFormatPng, xbuf, xread);
    break;
  case BITMAP_FORMAT_APM: /* APM */
    hr = OLEPictureImpl_LoadAPM(This, xbuf, xread);
    break;
  case 0x0000: { /* ICON or CURSOR, first word is dwReserved */
    hr = OLEPictureImpl_LoadIcon(This, xbuf, xread);
    break;
  }
  default:
  {
    unsigned int i;

    /* let's see if it's a EMF */
    hr = OLEPictureImpl_LoadEnhMetafile(This, xbuf, xread);
    if (hr == S_OK) break;

    FIXME("Unknown magic %04x, %ld read bytes:\n", magic, xread);
    hr=E_FAIL;
    for (i=0;i<xread+8;i++) {
	if (i<8) FIXME("%02x ",((unsigned char*)header)[i]);
	else FIXME("%02x ",xbuf[i-8]);
        if (i % 10 == 9) FIXME("\n");
    }
    FIXME("\n");
    break;
  }
  }
  This->bIsDirty = FALSE;

  /* FIXME: this notify is not really documented */
  if (hr==S_OK)
      OLEPicture_SendNotify(This,DISPID_PICT_TYPE);
  return hr;
}

/* pass NULL buffer to fetch size */
static HRESULT serializeBMP(HBITMAP hbmp, void **buffer, unsigned int *length)
{
    char infobuf[sizeof(BITMAPINFOHEADER) + 256 * sizeof(RGBQUAD)] = { 0 };
    BITMAPINFO *info = (BITMAPINFO *)infobuf;
    BITMAP bm;
    HBITMAP hdib = NULL;
    BITMAPINFO *bitmap;
    BITMAPFILEHEADER *filehdr;
    int numentries;
    HDC hdc;
    HRESULT hr = S_OK;
    unsigned char *data = NULL;

    hdc = GetDC(0);
    if (!GetObjectW(hbmp, sizeof(bm), &bm)) {
        hr = E_INVALIDARG;
        goto done;
    }

    /* Convert 16bpp/32bpp bitmap to 24bpp */
    if (bm.bmBitsPixel == 16 || bm.bmBitsPixel == 32) {
        void *bits;
        info->bmiHeader.biSize = sizeof(info->bmiHeader);
        info->bmiHeader.biWidth = bm.bmWidth;
        info->bmiHeader.biHeight = bm.bmHeight;
        info->bmiHeader.biPlanes = 1;
        info->bmiHeader.biBitCount = 24;
        info->bmiHeader.biCompression = BI_RGB;
        hdib = CreateDIBSection(hdc, info, DIB_RGB_COLORS, &bits, NULL, 0);
        if (!hdib) {
            hr = E_OUTOFMEMORY;
            goto done;
        }
        if (buffer) {
            HDC src, dst;
            src = CreateCompatibleDC(hdc);
            dst = CreateCompatibleDC(hdc);
            SelectObject(src, hbmp);
            SelectObject(dst, hdib);
            if (!BitBlt(dst, 0, 0, bm.bmWidth, bm.bmHeight, src, 0, 0, SRCCOPY))
                hr = E_INVALIDARG;
            DeleteDC(src);
            DeleteDC(dst);
            if (FAILED(hr))
                goto done;
        }
        hbmp = hdib;
    }

    /* Find out bitmap size and padded length */
    info->bmiHeader.biSize = sizeof(info->bmiHeader);
    if (!GetDIBits(hdc, hbmp, 0, 0, NULL, info, DIB_RGB_COLORS)) {
        hr = E_INVALIDARG;
        goto done;
    }

    /* Calculate the total length required for the BMP data */
    if (info->bmiHeader.biClrUsed != 0) {
        numentries = info->bmiHeader.biClrUsed;
    if (numentries > 256)
        numentries = 256;
    } else {
        if (info->bmiHeader.biBitCount <= 8)
            numentries = 1 << info->bmiHeader.biBitCount;
        else
            numentries = 0;
    }

    *length =
        sizeof(BITMAPFILEHEADER) +
        sizeof(BITMAPINFOHEADER) +
        numentries * sizeof(RGBQUAD) +
        info->bmiHeader.biSizeImage;

    if (!buffer)
        goto done;

    /* Fetch bitmap palette & pixel data */
    if (!(data = malloc(info->bmiHeader.biSizeImage))) {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    if (!GetDIBits(hdc, hbmp, 0, info->bmiHeader.biHeight, data, info, DIB_RGB_COLORS)) {
        hr = E_INVALIDARG;
        goto done;
    }

    if (!(*buffer = malloc(*length))) {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    /* Fill the BITMAPFILEHEADER */
    filehdr = *buffer;
    filehdr->bfType = BITMAP_FORMAT_BMP;
    filehdr->bfSize = *length;
    filehdr->bfReserved1 = 0;
    filehdr->bfReserved2 = 0;
    filehdr->bfOffBits =
        sizeof(BITMAPFILEHEADER) +
        sizeof(BITMAPINFOHEADER) +
        numentries * sizeof(RGBQUAD);

    /* Fill the BITMAPINFOHEADER and the palette data */
    bitmap = (BITMAPINFO *)((unsigned char *)(*buffer) + sizeof(BITMAPFILEHEADER));
    memcpy(bitmap, info, sizeof(BITMAPINFOHEADER) + numentries * sizeof(RGBQUAD));
    memcpy((unsigned char *)(*buffer) + filehdr->bfOffBits, data, bitmap->bmiHeader.biSizeImage);

done:
    free(data);
    ReleaseDC(0, hdc);
    if (hdib) DeleteObject(hdib);
    return hr;
}

static BOOL serializeIcon(HICON hIcon, void ** ppBuffer, unsigned int * pLength)
{
	ICONINFO infoIcon;
        BOOL success = FALSE;

	*ppBuffer = NULL; *pLength = 0;
	if (GetIconInfo(hIcon, &infoIcon)) {
		HDC hDC;
		BITMAPINFO * pInfoBitmap;
		unsigned char * pIconData = NULL;
		unsigned int iDataSize = 0;

        pInfoBitmap = calloc(1, sizeof(BITMAPINFOHEADER) + 256 * sizeof(RGBQUAD));

		/* Find out icon size */
		hDC = GetDC(0);
		pInfoBitmap->bmiHeader.biSize = sizeof(pInfoBitmap->bmiHeader);
		GetDIBits(hDC, infoIcon.hbmColor, 0, 0, NULL, pInfoBitmap, DIB_RGB_COLORS);
		if (1) {
			/* Auxiliary pointers */
			CURSORICONFILEDIR * pIconDir;
			CURSORICONFILEDIRENTRY * pIconEntry;
			BITMAPINFOHEADER * pIconBitmapHeader;
			unsigned int iOffsetPalette;
			unsigned int iOffsetColorData;
			unsigned int iOffsetMaskData;

			unsigned int iLengthScanLineMask;
			unsigned int iNumEntriesPalette;

			iLengthScanLineMask = ((pInfoBitmap->bmiHeader.biWidth + 31) >> 5) << 2;
/*
			FIXME("DEBUG: bitmap size is %d x %d\n",
				pInfoBitmap->bmiHeader.biWidth,
				pInfoBitmap->bmiHeader.biHeight);
			FIXME("DEBUG: bitmap bpp is %d\n",
				pInfoBitmap->bmiHeader.biBitCount);
			FIXME("DEBUG: bitmap nplanes is %d\n",
				pInfoBitmap->bmiHeader.biPlanes);
			FIXME("DEBUG: bitmap biSizeImage is %u\n",
				pInfoBitmap->bmiHeader.biSizeImage);
*/
			/* Let's start with one CURSORICONFILEDIR and one CURSORICONFILEDIRENTRY */
			iDataSize += 3 * sizeof(WORD) + sizeof(CURSORICONFILEDIRENTRY) + sizeof(BITMAPINFOHEADER);
			pIconData = calloc(1, iDataSize);

			/* Fill out the CURSORICONFILEDIR */
			pIconDir = (CURSORICONFILEDIR *)pIconData;
			pIconDir->idType = 1;
			pIconDir->idCount = 1;
			pIconDir->idReserved = 0;

			/* Fill out the CURSORICONFILEDIRENTRY */
			pIconEntry = (CURSORICONFILEDIRENTRY *)(pIconData + 3 * sizeof(WORD));
			pIconEntry->bWidth = (unsigned char)pInfoBitmap->bmiHeader.biWidth;
			pIconEntry->bHeight = (unsigned char)pInfoBitmap->bmiHeader.biHeight;
			pIconEntry->bColorCount =
				(pInfoBitmap->bmiHeader.biBitCount < 8)
				? 1 << pInfoBitmap->bmiHeader.biBitCount
				: 0;
			pIconEntry->xHotspot = pInfoBitmap->bmiHeader.biPlanes;
			pIconEntry->yHotspot = pInfoBitmap->bmiHeader.biBitCount;
			pIconEntry->dwDIBSize = 0;
			pIconEntry->dwDIBOffset = 3 * sizeof(WORD) + sizeof(CURSORICONFILEDIRENTRY);

			/* Fill out the BITMAPINFOHEADER */
			pIconBitmapHeader = (BITMAPINFOHEADER *)(pIconData + 3 * sizeof(WORD) + sizeof(CURSORICONFILEDIRENTRY));
			*pIconBitmapHeader = pInfoBitmap->bmiHeader;

			/*	Find out whether a palette exists for the bitmap */
			if (	(pInfoBitmap->bmiHeader.biBitCount == 16 && pInfoBitmap->bmiHeader.biCompression == BI_RGB)
				||	(pInfoBitmap->bmiHeader.biBitCount == 24)
				||	(pInfoBitmap->bmiHeader.biBitCount == 32 && pInfoBitmap->bmiHeader.biCompression == BI_RGB)) {
				iNumEntriesPalette = pInfoBitmap->bmiHeader.biClrUsed;
				if (iNumEntriesPalette > 256) iNumEntriesPalette = 256; 
			} else if ((pInfoBitmap->bmiHeader.biBitCount == 16 || pInfoBitmap->bmiHeader.biBitCount == 32)
				&& pInfoBitmap->bmiHeader.biCompression == BI_BITFIELDS) {
				iNumEntriesPalette = 3;
			} else if (pInfoBitmap->bmiHeader.biBitCount <= 8) {
				iNumEntriesPalette = 1 << pInfoBitmap->bmiHeader.biBitCount;
			} else {
				iNumEntriesPalette = 0;
			}

			/*  Add bitmap size and header size to icon data size. */
			iOffsetPalette = iDataSize;
			iDataSize += iNumEntriesPalette * sizeof(DWORD);
			iOffsetColorData = iDataSize;
			iDataSize += pIconBitmapHeader->biSizeImage;
			iOffsetMaskData = iDataSize;
			iDataSize += pIconBitmapHeader->biHeight * iLengthScanLineMask;
			pIconBitmapHeader->biSizeImage += pIconBitmapHeader->biHeight * iLengthScanLineMask;
			pIconBitmapHeader->biHeight *= 2;
			pIconData = realloc(pIconData, iDataSize);
			pIconEntry = (CURSORICONFILEDIRENTRY *)(pIconData + 3 * sizeof(WORD));
			pIconBitmapHeader = (BITMAPINFOHEADER *)(pIconData + 3 * sizeof(WORD) + sizeof(CURSORICONFILEDIRENTRY));
			pIconEntry->dwDIBSize = iDataSize - (3 * sizeof(WORD) + sizeof(CURSORICONFILEDIRENTRY));

			/* Get the actual bitmap data from the icon bitmap */
			GetDIBits(hDC, infoIcon.hbmColor, 0, pInfoBitmap->bmiHeader.biHeight,
				pIconData + iOffsetColorData, pInfoBitmap, DIB_RGB_COLORS);
			if (iNumEntriesPalette > 0) {
				memcpy(pIconData + iOffsetPalette, pInfoBitmap->bmiColors,
					iNumEntriesPalette * sizeof(RGBQUAD));
			}

			/* Reset all values so that GetDIBits call succeeds */
			memset(pIconData + iOffsetMaskData, 0, iDataSize - iOffsetMaskData);
			memset(pInfoBitmap, 0, sizeof(BITMAPINFOHEADER) + 256 * sizeof(RGBQUAD));
			pInfoBitmap->bmiHeader.biSize = sizeof(pInfoBitmap->bmiHeader);
/*
            if (!(GetDIBits(hDC, infoIcon.hbmMask, 0, 0, NULL, pInfoBitmap, DIB_RGB_COLORS)
				&& GetDIBits(hDC, infoIcon.hbmMask, 0, pIconEntry->bHeight,
					pIconData + iOffsetMaskData, pInfoBitmap, DIB_RGB_COLORS))) {

                printf("ERROR: unable to get bitmap mask (error %u)\n",
					GetLastError());

			}
*/
            GetDIBits(hDC, infoIcon.hbmMask, 0, 0, NULL, pInfoBitmap, DIB_RGB_COLORS);
            GetDIBits(hDC, infoIcon.hbmMask, 0, pIconEntry->bHeight, pIconData + iOffsetMaskData, pInfoBitmap, DIB_RGB_COLORS);

			/* Write out everything produced so far to the stream */
			*ppBuffer = pIconData; *pLength = iDataSize;
                        success = TRUE;
		} else {
/*
			printf("ERROR: unable to get bitmap information via GetDIBits() (error %u)\n",
				GetLastError());
*/
		}
		/*
			Remarks (from MSDN entry on GetIconInfo):

			GetIconInfo creates bitmaps for the hbmMask and hbmColor
			members of ICONINFO. The calling application must manage
			these bitmaps and delete them when they are no longer
			necessary.
		 */
		if (hDC) ReleaseDC(0, hDC);
		DeleteObject(infoIcon.hbmMask);
		if (infoIcon.hbmColor) DeleteObject(infoIcon.hbmColor);
		free(pInfoBitmap);
	} else {
		ERR("Unable to get icon information (error %lu)\n", GetLastError());
	}
        return success;
}

static HRESULT serializeEMF(HENHMETAFILE hemf, void **buf, unsigned *size)
{
    if (!(*size = GetEnhMetaFileBits(hemf, 0, NULL)))
        return E_FAIL;

    if (!(*buf = HeapAlloc(GetProcessHeap(), 0, *size)))
        return E_OUTOFMEMORY;

    if (!GetEnhMetaFileBits(hemf, *size, *buf))
    {
        HeapFree(GetProcessHeap(), 0, *buf);
        return E_FAIL;
    }
    return S_OK;
}

static HRESULT WINAPI OLEPictureImpl_Save(
  IPersistStream* iface,IStream*pStm,BOOL fClearDirty)
{
    HRESULT hResult = E_NOTIMPL;
    void * pIconData;
    unsigned int iDataSize;
    DWORD header[2];
    ULONG dummy;
    OLEPictureImpl *This = impl_from_IPersistStream(iface);

    TRACE("%p %p %d\n", This, pStm, fClearDirty);

    switch (This->desc.picType) {
    case PICTYPE_NONE:
        header[0] = 0x0000746c;
        header[1] = 0;
        hResult = IStream_Write(pStm, header, 2 * sizeof(DWORD), &dummy);
        break;

    case PICTYPE_ICON:
        if (This->bIsDirty || !This->data) {
            if (!serializeIcon(This->desc.icon.hicon, &pIconData, &iDataSize)) {
                ERR("(%p,%p,%d), serializeIcon() failed\n", This, pStm, fClearDirty);
                hResult = E_FAIL;
                break;
            }
            free(This->data);
            This->data = pIconData;
            This->datalen = iDataSize;
        }

        header[0] = (This->loadtime_magic != 0xdeadbeef) ? This->loadtime_magic : 0x0000746c;
        header[1] = This->datalen;
        IStream_Write(pStm, header, 2 * sizeof(DWORD), &dummy);
        IStream_Write(pStm, This->data, This->datalen, &dummy);
        hResult = S_OK;
        break;
    case PICTYPE_BITMAP:
        if (This->bIsDirty || !This->data) {
            switch (This->keepOrigFormat ? This->loadtime_format : BITMAP_FORMAT_BMP) {
            case BITMAP_FORMAT_BMP:
                hResult = serializeBMP(This->desc.bmp.hbitmap, &pIconData, &iDataSize);
                break;
            case BITMAP_FORMAT_JPEG:
                FIXME("(%p,%p,%d), PICTYPE_BITMAP (format JPEG) not implemented!\n",This,pStm,fClearDirty);
                break;
            case BITMAP_FORMAT_GIF:
                FIXME("(%p,%p,%d), PICTYPE_BITMAP (format GIF) not implemented!\n",This,pStm,fClearDirty);
                break;
            case BITMAP_FORMAT_PNG:
                FIXME("(%p,%p,%d), PICTYPE_BITMAP (format PNG) not implemented!\n",This,pStm,fClearDirty);
                break;
            default:
                FIXME("(%p,%p,%d), PICTYPE_BITMAP (format UNKNOWN, using BMP?) not implemented!\n",This,pStm,fClearDirty);
                break;
            }

            if (hResult != S_OK)
                break;

            free(This->data);
            This->data = pIconData;
            This->datalen = iDataSize;
        }

        header[0] = (This->loadtime_magic != 0xdeadbeef) ? This->loadtime_magic : 0x0000746c;
        header[1] = This->datalen;
        IStream_Write(pStm, header, 2 * sizeof(DWORD), &dummy);
        IStream_Write(pStm, This->data, This->datalen, &dummy);
        hResult = S_OK;
        break;

    case PICTYPE_ENHMETAFILE:
        if (This->bIsDirty || !This->data)
        {
            hResult = serializeEMF(This->desc.emf.hemf, &pIconData, &iDataSize);
            if (hResult != S_OK)
                break;

            HeapFree(GetProcessHeap(), 0, This->data);
            This->data = pIconData;
            This->datalen = iDataSize;
        }
        header[0] = 0x0000746c;
        header[1] = This->datalen;
        IStream_Write(pStm, header, 2 * sizeof(DWORD), &dummy);
        IStream_Write(pStm, This->data, This->datalen, &dummy);
        hResult = S_OK;
        break;

    case PICTYPE_METAFILE:
        FIXME("(%p,%p,%d), PICTYPE_METAFILE not implemented!\n",This,pStm,fClearDirty);
        break;
    default:
        FIXME("(%p,%p,%d), [unknown type] not implemented!\n",This,pStm,fClearDirty);
        break;
    }
    if (hResult == S_OK && fClearDirty) This->bIsDirty = FALSE;
    return hResult;
}

static HRESULT WINAPI OLEPictureImpl_GetSizeMax(IPersistStream *iface, ULARGE_INTEGER *size)
{
    HRESULT hr = E_NOTIMPL;
    OLEPictureImpl *This = impl_from_IPersistStream(iface);
    unsigned int datasize = This->datalen;

    FIXME("(%p,%p), partial stub!\n", This, size);

    if (!size)
        return E_INVALIDARG;

    switch (This->desc.picType) {
    case PICTYPE_NONE:
        hr = S_OK;
        break;
    case PICTYPE_ICON:
        FIXME("(%p), PICTYPE_ICON not implemented!\n",This);
        break;
    case PICTYPE_BITMAP:
        if (This->bIsDirty || !This->data) {
            switch (This->keepOrigFormat ? This->loadtime_format : BITMAP_FORMAT_BMP) {
            case BITMAP_FORMAT_BMP:
                hr = serializeBMP(This->desc.bmp.hbitmap, NULL, &datasize);
                break;
            case BITMAP_FORMAT_JPEG:
                FIXME("(%p), PICTYPE_BITMAP (format JPEG) not implemented!\n",This);
                break;
            case BITMAP_FORMAT_GIF:
                FIXME("(%p), PICTYPE_BITMAP (format GIF) not implemented!\n",This);
                break;
            case BITMAP_FORMAT_PNG:
                FIXME("(%p), PICTYPE_BITMAP (format PNG) not implemented!\n",This);
                break;
            default:
                FIXME("(%p), PICTYPE_BITMAP (format UNKNOWN, using BMP?) not implemented!\n",This);
                break;
            }
        } else hr = S_OK;

        break;
    case PICTYPE_METAFILE:
        FIXME("(%p), PICTYPE_METAFILE not implemented!\n",This);
        break;
    case PICTYPE_ENHMETAFILE:
        FIXME("(%p), PICTYPE_ENHMETAFILE not implemented!\n",This);
        break;
    default:
        FIXME("(%p), [unknown type] not implemented!\n",This);
        break;
    }

    size->HighPart = 0;
    size->LowPart = datasize + 8;
    return hr;
}

/************************************************************************
 * OLEPictureImpl_SaveAsFile
 */
static HRESULT WINAPI OLEPictureImpl_SaveAsFile(IPicture *iface,
    IStream *stream, BOOL mem_copy, LONG *size)
{
    OLEPictureImpl *This = impl_from_IPicture(iface);
    void *data;
    unsigned int data_size;
    ULONG written;
    HRESULT hr;

    FIXME("(%p)->(%p,%d,%p): semi-stub\n", This, stream, mem_copy, size);

    switch (This->desc.picType)
    {
    case PICTYPE_NONE:
        return S_OK;

    case PICTYPE_ICON:
        if (!mem_copy) return E_FAIL;

        if (This->bIsDirty || !This->data)
        {
            if (!serializeIcon(This->desc.icon.hicon, &data, &data_size))
                return E_FAIL;
            HeapFree(GetProcessHeap(), 0, This->data);
            This->data = data;
            This->datalen = data_size;
        }
        hr = IStream_Write(stream, This->data, This->datalen, &written);
        if (hr == S_OK && size) *size = written;
        return hr;

    case PICTYPE_BITMAP:
        if (!mem_copy) return E_FAIL;

        if (This->bIsDirty || !This->data)
        {
            switch (This->keepOrigFormat ? This->loadtime_format : BITMAP_FORMAT_BMP)
            {
            case BITMAP_FORMAT_BMP:
                hr = serializeBMP(This->desc.bmp.hbitmap, &data, &data_size);
                if (hr != S_OK) return hr;
                break;
            case BITMAP_FORMAT_JPEG:
                FIXME("BITMAP_FORMAT_JPEG is not implemented\n");
                return E_NOTIMPL;
            case BITMAP_FORMAT_GIF:
                FIXME("BITMAP_FORMAT_GIF is not implemented\n");
                return E_NOTIMPL;
            case BITMAP_FORMAT_PNG:
                FIXME("BITMAP_FORMAT_PNG is not implemented\n");
                return E_NOTIMPL;
            default:
                FIXME("PICTYPE_BITMAP/%#x is not implemented\n", This->loadtime_format);
                return E_NOTIMPL;
            }

            HeapFree(GetProcessHeap(), 0, This->data);
            This->data = data;
            This->datalen = data_size;
        }
        hr = IStream_Write(stream, This->data, This->datalen, &written);
        if (hr == S_OK && size) *size = written;
        return hr;

    case PICTYPE_METAFILE:
        FIXME("PICTYPE_METAFILE is not implemented\n");
        return E_NOTIMPL;

    case PICTYPE_ENHMETAFILE:
        if (!mem_copy) return E_FAIL;

        if (This->bIsDirty || !This->data)
        {
            hr = serializeEMF(This->desc.emf.hemf, &data, &data_size);
            if (hr != S_OK) return hr;
            HeapFree(GetProcessHeap(), 0, This->data);
            This->data = data;
            This->datalen = data_size;
        }
        hr = IStream_Write(stream, This->data, This->datalen, &written);
        if (hr == S_OK && size) *size = written;
        return hr;

    default:
        FIXME("%#x is not implemented\n", This->desc.picType);
        break;
    }
    return E_NOTIMPL;
}

/************************************************************************
 *    IDispatch
 */

/************************************************************************
 * OLEPictureImpl_IDispatch_QueryInterface (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static HRESULT WINAPI OLEPictureImpl_IDispatch_QueryInterface(
  IDispatch* iface,
  REFIID     riid,
  VOID**     ppvoid)
{
  OLEPictureImpl *This = impl_from_IDispatch(iface);

  return IPicture_QueryInterface(&This->IPicture_iface, riid, ppvoid);
}

/************************************************************************
 * OLEPictureImpl_IDispatch_AddRef (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static ULONG WINAPI OLEPictureImpl_IDispatch_AddRef(
  IDispatch* iface)
{
  OLEPictureImpl *This = impl_from_IDispatch(iface);

  return IPicture_AddRef(&This->IPicture_iface);
}

/************************************************************************
 * OLEPictureImpl_IDispatch_Release (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static ULONG WINAPI OLEPictureImpl_IDispatch_Release(
  IDispatch* iface)
{
  OLEPictureImpl *This = impl_from_IDispatch(iface);

  return IPicture_Release(&This->IPicture_iface);
}

/************************************************************************
 * OLEPictureImpl_GetTypeInfoCount (IDispatch)
 *
 * See Windows documentation for more details on IDispatch methods.
 */
static HRESULT WINAPI OLEPictureImpl_GetTypeInfoCount(
  IDispatch*    iface,
  unsigned int* pctinfo)
{
  TRACE("(%p)\n", pctinfo);

  *pctinfo = 1;

  return S_OK;
}

/************************************************************************
 * OLEPictureImpl_GetTypeInfo (IDispatch)
 *
 * See Windows documentation for more details on IDispatch methods.
 */
static HRESULT WINAPI OLEPictureImpl_GetTypeInfo(
  IDispatch*  iface,
  UINT      iTInfo,
  LCID        lcid,
  ITypeInfo** ppTInfo)
{
  ITypeLib *tl;
  HRESULT hres;

  TRACE("(iTInfo=%d, lcid=%04x, %p)\n", iTInfo, (int)lcid, ppTInfo);

  if (iTInfo != 0)
    return E_FAIL;

  hres = LoadTypeLib(L"stdole2.tlb", &tl);
  if (FAILED(hres))
  {
    ERR("Could not load stdole2.tlb\n");
    return hres;
  }

  hres = ITypeLib_GetTypeInfoOfGuid(tl, &IID_IPictureDisp, ppTInfo);
  if (FAILED(hres))
    ERR("Did not get IPictureDisp typeinfo from typelib, hres %#lx.\n", hres);

  return hres;
}

/************************************************************************
 * OLEPictureImpl_GetIDsOfNames (IDispatch)
 *
 * See Windows documentation for more details on IDispatch methods.
 */
static HRESULT WINAPI OLEPictureImpl_GetIDsOfNames(
  IDispatch*  iface,
  REFIID      riid,
  LPOLESTR* rgszNames,
  UINT      cNames,
  LCID        lcid,
  DISPID*     rgDispId)
{
  ITypeInfo * pTInfo;
  HRESULT hres;

  TRACE("(%p,%s,%p,cNames=%d,lcid=%04x,%p)\n", iface, debugstr_guid(riid),
        rgszNames, cNames, (int)lcid, rgDispId);

  if (cNames == 0)
  {
    return E_INVALIDARG;
  }
  else
  {
    /* retrieve type information */
    hres = OLEPictureImpl_GetTypeInfo(iface, 0, lcid, &pTInfo);

    if (FAILED(hres))
    {
      ERR("GetTypeInfo failed.\n");
      return hres;
    }

    /* convert names to DISPIDs */
    hres = DispGetIDsOfNames (pTInfo, rgszNames, cNames, rgDispId);
    ITypeInfo_Release(pTInfo);

    return hres;
  }
}

/************************************************************************
 * OLEPictureImpl_Invoke (IDispatch)
 *
 * See Windows documentation for more details on IDispatch methods.
 */
static HRESULT WINAPI OLEPictureImpl_Invoke(
  IDispatch*  iface,
  DISPID      dispIdMember,
  REFIID      riid,
  LCID        lcid,
  WORD        wFlags,
  DISPPARAMS* pDispParams,
  VARIANT*    pVarResult,
  EXCEPINFO*  pExepInfo,
  UINT*     puArgErr)
{
  OLEPictureImpl *This = impl_from_IDispatch(iface);
  HRESULT hr;

  /* validate parameters */

  if (!IsEqualIID(riid, &IID_NULL))
  {
    ERR("riid was %s instead of IID_NULL\n", debugstr_guid(riid));
    return DISP_E_UNKNOWNNAME;
  }

  if (!pDispParams)
  {
    ERR("null pDispParams not allowed\n");
    return DISP_E_PARAMNOTOPTIONAL;
  }

  if (wFlags & DISPATCH_PROPERTYGET)
  {
    if (pDispParams->cArgs != 0)
    {
      ERR("param count for DISPATCH_PROPERTYGET was %d instead of 0\n", pDispParams->cArgs);
      return DISP_E_BADPARAMCOUNT;
    }
    if (!pVarResult)
    {
      ERR("null pVarResult not allowed when DISPATCH_PROPERTYGET specified\n");
      return DISP_E_PARAMNOTOPTIONAL;
    }
  }
  else if (wFlags & DISPATCH_PROPERTYPUT)
  {
    if (pDispParams->cArgs != 1)
    {
      ERR("param count for DISPATCH_PROPERTYPUT was %d instead of 1\n", pDispParams->cArgs);
      return DISP_E_BADPARAMCOUNT;
    }
  }

  switch (dispIdMember)
  {
  case DISPID_PICT_HANDLE:
    if (wFlags & DISPATCH_PROPERTYGET)
    {
      TRACE("DISPID_PICT_HANDLE\n");
      V_VT(pVarResult) = VT_I4;
      return IPicture_get_Handle(&This->IPicture_iface, &V_UINT(pVarResult));
    }
    break;
  case DISPID_PICT_HPAL:
    if (wFlags & DISPATCH_PROPERTYGET)
    {
      TRACE("DISPID_PICT_HPAL\n");
      V_VT(pVarResult) = VT_I4;
      return IPicture_get_hPal(&This->IPicture_iface, &V_UINT(pVarResult));
    }
    else if (wFlags & DISPATCH_PROPERTYPUT)
    {
      VARIANTARG vararg;

      TRACE("DISPID_PICT_HPAL\n");

      VariantInit(&vararg);
      hr = VariantChangeTypeEx(&vararg, &pDispParams->rgvarg[0], lcid, 0, VT_I4);
      if (FAILED(hr))
        return hr;

      hr = IPicture_set_hPal(&This->IPicture_iface, V_I4(&vararg));

      VariantClear(&vararg);
      return hr;
    }
    break;
  case DISPID_PICT_TYPE:
    if (wFlags & DISPATCH_PROPERTYGET)
    {
      TRACE("DISPID_PICT_TYPE\n");
      V_VT(pVarResult) = VT_I2;
      return OLEPictureImpl_get_Type(&This->IPicture_iface, &V_I2(pVarResult));
    }
    break;
  case DISPID_PICT_WIDTH:
    if (wFlags & DISPATCH_PROPERTYGET)
    {
      TRACE("DISPID_PICT_WIDTH\n");
      V_VT(pVarResult) = VT_I4;
      return IPicture_get_Width(&This->IPicture_iface, &V_I4(pVarResult));
    }
    break;
  case DISPID_PICT_HEIGHT:
    if (wFlags & DISPATCH_PROPERTYGET)
    {
      TRACE("DISPID_PICT_HEIGHT\n");
      V_VT(pVarResult) = VT_I4;
      return IPicture_get_Height(&This->IPicture_iface, &V_I4(pVarResult));
    }
    break;
  case DISPID_PICT_RENDER:
    if (wFlags & DISPATCH_METHOD)
    {
      VARIANTARG *args = pDispParams->rgvarg;
      int i;

      TRACE("DISPID_PICT_RENDER\n");

      if (pDispParams->cArgs != 10)
        return DISP_E_BADPARAMCOUNT;

      /* All parameters are supposed to be VT_I4 (on 64 bits too). */
      for (i = 0; i < pDispParams->cArgs; i++)
        if (V_VT(&args[i]) != VT_I4)
        {
          ERR("DISPID_PICT_RENDER: wrong argument type %d:%d\n", i, V_VT(&args[i]));
          return DISP_E_TYPEMISMATCH;
        }

      /* FIXME: rectangle pointer argument handling seems broken on 64 bits,
                currently Render() doesn't use it at all so for now NULL is passed. */
      return IPicture_Render(&This->IPicture_iface,
                LongToHandle(V_I4(&args[9])),
                             V_I4(&args[8]),
                             V_I4(&args[7]),
                             V_I4(&args[6]),
                             V_I4(&args[5]),
                             V_I4(&args[4]),
                             V_I4(&args[3]),
                             V_I4(&args[2]),
                             V_I4(&args[1]),
                                      NULL);
    }
    break;
  }

  ERR("invalid dispid %#lx or wFlags 0x%x\n", dispIdMember, wFlags);
  return DISP_E_MEMBERNOTFOUND;
}


static const IPictureVtbl OLEPictureImpl_VTable =
{
  OLEPictureImpl_QueryInterface,
  OLEPictureImpl_AddRef,
  OLEPictureImpl_Release,
  OLEPictureImpl_get_Handle,
  OLEPictureImpl_get_hPal,
  OLEPictureImpl_get_Type,
  OLEPictureImpl_get_Width,
  OLEPictureImpl_get_Height,
  OLEPictureImpl_Render,
  OLEPictureImpl_set_hPal,
  OLEPictureImpl_get_CurDC,
  OLEPictureImpl_SelectPicture,
  OLEPictureImpl_get_KeepOriginalFormat,
  OLEPictureImpl_put_KeepOriginalFormat,
  OLEPictureImpl_PictureChanged,
  OLEPictureImpl_SaveAsFile,
  OLEPictureImpl_get_Attributes
};

static const IDispatchVtbl OLEPictureImpl_IDispatch_VTable =
{
  OLEPictureImpl_IDispatch_QueryInterface,
  OLEPictureImpl_IDispatch_AddRef,
  OLEPictureImpl_IDispatch_Release,
  OLEPictureImpl_GetTypeInfoCount,
  OLEPictureImpl_GetTypeInfo,
  OLEPictureImpl_GetIDsOfNames,
  OLEPictureImpl_Invoke
};

static const IPersistStreamVtbl OLEPictureImpl_IPersistStream_VTable =
{
  OLEPictureImpl_IPersistStream_QueryInterface,
  OLEPictureImpl_IPersistStream_AddRef,
  OLEPictureImpl_IPersistStream_Release,
  OLEPictureImpl_GetClassID,
  OLEPictureImpl_IsDirty,
  OLEPictureImpl_Load,
  OLEPictureImpl_Save,
  OLEPictureImpl_GetSizeMax
};

static const IConnectionPointContainerVtbl OLEPictureImpl_IConnectionPointContainer_VTable =
{
  OLEPictureImpl_IConnectionPointContainer_QueryInterface,
  OLEPictureImpl_IConnectionPointContainer_AddRef,
  OLEPictureImpl_IConnectionPointContainer_Release,
  OLEPictureImpl_EnumConnectionPoints,
  OLEPictureImpl_FindConnectionPoint
};

/***********************************************************************
 * OleCreatePictureIndirect (OLEAUT32.419)
 */
HRESULT WINAPI OleCreatePictureIndirect(LPPICTDESC lpPictDesc, REFIID riid,
		            BOOL Own, void **ppvObj )
{
  OLEPictureImpl* newPict;
  HRESULT hr;

  TRACE("(%p,%s,%d,%p)\n", lpPictDesc, debugstr_guid(riid), Own, ppvObj);

  *ppvObj = NULL;

  hr = OLEPictureImpl_Construct(lpPictDesc, Own, &newPict);
  if (hr != S_OK) return hr;

  /*
   * Make sure it supports the interface required by the caller.
   */
  hr = IPicture_QueryInterface(&newPict->IPicture_iface, riid, ppvObj);

  /*
   * Release the reference obtained in the constructor. If
   * the QueryInterface was unsuccessful, it will free the class.
   */
  IPicture_Release(&newPict->IPicture_iface);

  return hr;
}


/***********************************************************************
 * OleLoadPicture (OLEAUT32.418)
 */
HRESULT WINAPI OleLoadPicture( LPSTREAM lpstream, LONG lSize, BOOL fRunmode,
		            REFIID riid, LPVOID *ppvObj )
{
  LPPERSISTSTREAM ps;
  IPicture	*newpic;
  HRESULT hr;

  TRACE("%p, %ld, %d, %s, %p), partially implemented.\n",
	lpstream, lSize, fRunmode, debugstr_guid(riid), ppvObj);

  hr = OleCreatePictureIndirect(NULL,riid,!fRunmode,(LPVOID*)&newpic);
  if (hr != S_OK)
    return hr;
  hr = IPicture_QueryInterface(newpic,&IID_IPersistStream, (LPVOID*)&ps);
  if (hr != S_OK) {
      ERR("Could not get IPersistStream iface from Ole Picture?\n");
      IPicture_Release(newpic);
      *ppvObj = NULL;
      return hr;
  }
  hr = IPersistStream_Load(ps,lpstream);
  IPersistStream_Release(ps);
  if (FAILED(hr))
  {
      ERR("IPersistStream_Load failed\n");
      IPicture_Release(newpic);
      *ppvObj = NULL;
      return hr;
  }
  hr = IPicture_QueryInterface(newpic,riid,ppvObj);
  if (hr != S_OK)
      ERR("Failed to get interface %s from IPicture.\n",debugstr_guid(riid));
  IPicture_Release(newpic);
  return hr;
}

/***********************************************************************
 * OleLoadPictureEx (OLEAUT32.401)
 */
HRESULT WINAPI OleLoadPictureEx( LPSTREAM lpstream, LONG lSize, BOOL fRunmode,
		            REFIID riid, DWORD xsiz, DWORD ysiz, DWORD flags, LPVOID *ppvObj )
{
  LPPERSISTSTREAM ps;
  IPicture	*newpic;
  HRESULT hr;

  FIXME("%p, %ld, %d, %s, %lu, %lu, %#lx, %p, partially implemented.\n",
	lpstream, lSize, fRunmode, debugstr_guid(riid), xsiz, ysiz, flags, ppvObj);

  hr = OleCreatePictureIndirect(NULL,riid,!fRunmode,(LPVOID*)&newpic);
  if (hr != S_OK)
    return hr;
  hr = IPicture_QueryInterface(newpic,&IID_IPersistStream, (LPVOID*)&ps);
  if (hr != S_OK) {
      ERR("Could not get IPersistStream iface from Ole Picture?\n");
      IPicture_Release(newpic);
      *ppvObj = NULL;
      return hr;
  }
  hr = IPersistStream_Load(ps,lpstream);
  IPersistStream_Release(ps);
  if (FAILED(hr))
  {
      ERR("IPersistStream_Load failed\n");
      IPicture_Release(newpic);
      *ppvObj = NULL;
      return hr;
  }
  hr = IPicture_QueryInterface(newpic,riid,ppvObj);
  if (hr != S_OK)
      ERR("Failed to get interface %s from IPicture.\n",debugstr_guid(riid));
  IPicture_Release(newpic);
  return hr;
}

static HRESULT create_stream(const WCHAR *filename, IStream **stream)
{
    HANDLE hFile;
    DWORD dwFileSize;
    HGLOBAL hGlobal = NULL;
    DWORD dwBytesRead;
    HRESULT hr = S_OK;

    hFile = CreateFileW(filename, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
        return HRESULT_FROM_WIN32(GetLastError());

    dwFileSize = GetFileSize(hFile, NULL);
    if (dwFileSize != INVALID_FILE_SIZE)
    {
        hGlobal = GlobalAlloc(GMEM_FIXED, dwFileSize);
        if (!hGlobal)
            hr = E_OUTOFMEMORY;
        else
        {
            if (!ReadFile(hFile, hGlobal, dwFileSize, &dwBytesRead, NULL))
            {
                GlobalFree(hGlobal);
                hr = HRESULT_FROM_WIN32(GetLastError());
            }
        }
    }

    CloseHandle(hFile);

    if (FAILED(hr)) return hr;

    hr = CreateStreamOnHGlobal(hGlobal, TRUE, stream);
    if (FAILED(hr))
        GlobalFree(hGlobal);

    return hr;
}

/***********************************************************************
 * OleLoadPictureFile (OLEAUT32.422)
 */
HRESULT WINAPI OleLoadPictureFile(VARIANT filename, IDispatch **picture)
{
    IStream *stream;
    HRESULT hr;

    TRACE("(%s,%p)\n", wine_dbgstr_variant(&filename), picture);

    if (V_VT(&filename) != VT_BSTR)
        return CTL_E_FILENOTFOUND;

    hr = create_stream(V_BSTR(&filename), &stream);
    if (hr != S_OK)
    {
        if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
            return CTL_E_FILENOTFOUND;

        return CTL_E_PATHFILEACCESSERROR;
    }

    hr = OleLoadPicture(stream, 0, FALSE, &IID_IDispatch, (void **)picture);
    IStream_Release(stream);
    return hr;
}

/***********************************************************************
 * OleSavePictureFile (OLEAUT32.423)
 */
HRESULT WINAPI OleSavePictureFile(IDispatch *picture, BSTR filename)
{
  FIXME("(%p %s): stub\n", picture, debugstr_w(filename));
  return CTL_E_FILENOTFOUND;
}

/***********************************************************************
 * OleLoadPicturePath (OLEAUT32.424)
 */
HRESULT WINAPI OleLoadPicturePath( LPOLESTR szURLorPath, LPUNKNOWN punkCaller,
		DWORD dwReserved, OLE_COLOR clrReserved, REFIID riid,
		LPVOID *ppvRet )
{
  IStream *stream;
  HRESULT hRes;
  WCHAR *file_candidate;
  WCHAR path_buf[MAX_PATH];

  TRACE("%s, %p, %ld, %#lx, %s, %p.\n", debugstr_w(szURLorPath), punkCaller, dwReserved,
        clrReserved, debugstr_guid(riid), ppvRet);

  if (!szURLorPath || !ppvRet)
      return E_INVALIDARG;

  *ppvRet = NULL;

  /* Convert file URLs to DOS paths. */
  if (wcsncmp(szURLorPath, L"file:", 5) == 0) {
      DWORD size;
      hRes = CoInternetParseUrl(szURLorPath, PARSE_PATH_FROM_URL, 0, path_buf,
                                ARRAY_SIZE(path_buf), &size, 0);
      if (FAILED(hRes))
          return hRes;

      file_candidate = path_buf;
  }
  else
      file_candidate = szURLorPath;

  /* Handle candidate DOS paths separately. */
  if (file_candidate[0] && file_candidate[1] == ':') {
      hRes = create_stream(file_candidate, &stream);
      if (FAILED(hRes))
	  return INET_E_RESOURCE_NOT_FOUND;
  } else {
      IMoniker *pmnk;
      IBindCtx *pbc;

      hRes = CreateBindCtx(0, &pbc);
      if (SUCCEEDED(hRes)) 
      {
	  hRes = CreateURLMoniker(NULL, szURLorPath, &pmnk);
	  if (SUCCEEDED(hRes))
	  {	         
	      hRes = IMoniker_BindToStorage(pmnk, pbc, NULL, &IID_IStream, (LPVOID*)&stream);
	      IMoniker_Release(pmnk);
	  }
	  IBindCtx_Release(pbc);
      }
      if (FAILED(hRes))
	  return hRes;
  }

  hRes = OleLoadPicture(stream, 0, FALSE, riid, ppvRet);

  IStream_Release(stream);

  return hRes;
}

/*******************************************************************************
 * StdPic ClassFactory
 */
typedef struct
{
    /* IUnknown fields */
    IClassFactory IClassFactory_iface;
    LONG          ref;
} IClassFactoryImpl;

static inline IClassFactoryImpl *impl_from_IClassFactory(IClassFactory *iface)
{
       return CONTAINING_RECORD(iface, IClassFactoryImpl, IClassFactory_iface);
}

static HRESULT WINAPI
SPCF_QueryInterface(LPCLASSFACTORY iface,REFIID riid,LPVOID *ppobj) {
	IClassFactoryImpl *This = impl_from_IClassFactory(iface);

	FIXME("(%p)->(%s,%p),stub!\n",This,debugstr_guid(riid),ppobj);
	return E_NOINTERFACE;
}

static ULONG WINAPI
SPCF_AddRef(LPCLASSFACTORY iface) {
	IClassFactoryImpl *This = impl_from_IClassFactory(iface);
	return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI SPCF_Release(LPCLASSFACTORY iface) {
	IClassFactoryImpl *This = impl_from_IClassFactory(iface);
	/* static class, won't be  freed */
	return InterlockedDecrement(&This->ref);
}

static HRESULT WINAPI SPCF_CreateInstance(
	LPCLASSFACTORY iface,LPUNKNOWN pOuter,REFIID riid,LPVOID *ppobj
) {
    /* Creates an uninitialized picture */
    return OleCreatePictureIndirect(NULL,riid,TRUE,ppobj);

}

static HRESULT WINAPI SPCF_LockServer(LPCLASSFACTORY iface,BOOL dolock) {
	IClassFactoryImpl *This = impl_from_IClassFactory(iface);
	FIXME("(%p)->(%d),stub!\n",This,dolock);
	return S_OK;
}

static const IClassFactoryVtbl SPCF_Vtbl = {
	SPCF_QueryInterface,
	SPCF_AddRef,
	SPCF_Release,
	SPCF_CreateInstance,
	SPCF_LockServer
};
static IClassFactoryImpl STDPIC_CF = {{&SPCF_Vtbl}, 1 };

void _get_STDPIC_CF(LPVOID *ppv) { *ppv = &STDPIC_CF; }
