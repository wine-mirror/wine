/*
 * OLE Picture object
 *
 * Implementation of OLE IPicture and related interfaces
 *
 * Copyright 2000 Huw D M Davies for CodeWeavers.
 *
 *
 * BUGS
 *
 * Only implements PICTYPE_BITMAP.
 * Most methods are just stubs.
 * Doesn't even expose IPersistStream, IConnectionPointContainer.
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

#include "winerror.h"
#include "olectl.h"
#include "wine/obj_picture.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(ole)

/*************************************************************************
 *  Declaration of implementation class
 */

typedef struct OLEPictureImpl {

  /*
   * IPicture handles IUnknown
   */

    ICOM_VTABLE(IPicture)       *lpvtbl1;
    ICOM_VTABLE(IDispatch)      *lpvtbl2;
    ICOM_VTABLE(IPersistStream) *lpvtbl3;

  /* Object referenece count */
    DWORD ref;

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

} OLEPictureImpl;

/*
 * Macros to retrieve pointer to IUnknown (IPicture) from the other VTables.
 */
#define ICOM_THIS_From_IDispatch(impl, name) \
    impl *This = (impl*)(((char*)name)-sizeof(void*));

#define ICOM_THIS_From_IPersistStream(impl, name) \
    impl *This = (impl*)(((char*)name)-2*sizeof(void*));

/*
 * Predeclare VTables.  They get initialized at the end.
 */
static ICOM_VTABLE(IPicture) OLEPictureImpl_VTable;
static ICOM_VTABLE(IDispatch) OLEPictureImpl_IDispatch_VTable;
static ICOM_VTABLE(IPersistStream) OLEPictureImpl_IPersistStream_VTable;

/***********************************************************************
 * Implementation of the OLEPictureImpl class.
 */

/************************************************************************
 * OLEPictureImpl_Construct
 *
 * This method will construct a new instance of the OLEPictureImpl
 * class.
 *
 * The caller of this method must release the object when it's
 * done with it.
 */
static OLEPictureImpl* OLEPictureImpl_Construct(LPPICTDESC pictDesc, BOOL fOwn)
{
  OLEPictureImpl* newObject = 0;
  TRACE("(%p) type = %d\n", pictDesc, pictDesc->picType);

  /*
   * Allocate space for the object.
   */
  newObject = HeapAlloc(GetProcessHeap(), 0, sizeof(OLEPictureImpl));

  if (newObject==0)
    return newObject;
  
  /*
   * Initialize the virtual function table.
   */
  newObject->lpvtbl1 = &OLEPictureImpl_VTable;
  newObject->lpvtbl2 = &OLEPictureImpl_IDispatch_VTable;
  newObject->lpvtbl3 = &OLEPictureImpl_IPersistStream_VTable;
  
  /*
   * Start with one reference count. The caller of this function 
   * must release the interface pointer when it is done.
   */
  newObject->ref = 1;

  newObject->fOwn = fOwn;

  if(pictDesc->cbSizeofstruct != sizeof(PICTDESC)) {
      FIXME("struct size = %d\n", pictDesc->cbSizeofstruct);
  }
  memcpy(&newObject->desc, pictDesc, sizeof(PICTDESC));

  switch(pictDesc->picType) {
  case PICTYPE_BITMAP:
    {
      BITMAP bm;
      HDC hdcRef;

      TRACE("bitmap handle %08x\n", pictDesc->u.bmp.hbitmap);
      if(GetObjectA(pictDesc->u.bmp.hbitmap, sizeof(bm), &bm) != sizeof(bm)) {
	ERR("GetObject fails\n");
	break;
      }

      newObject->origWidth = bm.bmWidth;
      newObject->origHeight = bm.bmHeight;

      /* The width and height are stored in HIMETRIC units (0.01 mm),
	 so we take our pixel width divide by pixels per inch and
	 multiply by 25.4 * 100 */

      /* Should we use GetBitmapDimension if available? */

      hdcRef = CreateCompatibleDC(0);

      newObject->himetricWidth = (bm.bmWidth * 2540) /
	GetDeviceCaps(hdcRef, LOGPIXELSX);
      newObject->himetricHeight = (bm.bmHeight * 2540) / 
	GetDeviceCaps(hdcRef, LOGPIXELSY);
      DeleteDC(hdcRef);
    }
    break;

  case PICTYPE_METAFILE:
    TRACE("metafile handle %08x\n", pictDesc->u.wmf.hmeta);
    newObject->himetricWidth = pictDesc->u.wmf.xExt;
    newObject->himetricHeight = pictDesc->u.wmf.yExt;
    break;

  case PICTYPE_ICON:
  case PICTYPE_ENHMETAFILE:
  default:
    FIXME("Unsupported type %d\n", pictDesc->picType);
    newObject->himetricWidth = newObject->himetricHeight = 0;
    break;
  }
    
  TRACE("returning %p\n", newObject);
  return newObject;
}

/************************************************************************
 * OLEPictureImpl_Destroy
 *
 * This method is called by the Release method when the reference
 * count goes doen to 0. it will free all resources used by
 * this object.  */
static void OLEPictureImpl_Destroy(OLEPictureImpl* Obj)
{ 
  TRACE("(%p)\n", Obj);

  if(Obj->fOwn) { /* We need to destroy the picture */
    switch(Obj->desc.picType) {
    case PICTYPE_BITMAP:
      DeleteObject(Obj->desc.u.bmp.hbitmap);
      break;
    case PICTYPE_METAFILE:
      DeleteMetaFile(Obj->desc.u.wmf.hmeta);
      break;
    case PICTYPE_ICON:
      DestroyIcon(Obj->desc.u.icon.hicon);
      break;
    case PICTYPE_ENHMETAFILE:
      DeleteEnhMetaFile(Obj->desc.u.emf.hemf);
      break;
    default:
      FIXME("Unsupported type %d - unable to delete\n", Obj->desc.picType);
      break;
    }
  }
  HeapFree(GetProcessHeap(), 0, Obj);
}

static ULONG WINAPI OLEPictureImpl_AddRef(IPicture* iface);

/************************************************************************
 * OLEPictureImpl_QueryInterface (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static HRESULT WINAPI OLEPictureImpl_QueryInterface(
  IPicture*  iface,
  REFIID  riid,
  void**  ppvObject)
{
  ICOM_THIS(OLEPictureImpl, iface);
  TRACE("(%p)->(%s, %p)\n", This, debugstr_guid(riid), ppvObject);

  /*
   * Perform a sanity check on the parameters.
   */
  if ( (This==0) || (ppvObject==0) )
    return E_INVALIDARG;
  
  /*
   * Initialize the return parameter.
   */
  *ppvObject = 0;
  
  /*
   * Compare the riid with the interface IDs implemented by this object.
   */
  if (memcmp(&IID_IUnknown, riid, sizeof(IID_IUnknown)) == 0) 
  {
    *ppvObject = (IPicture*)This;
  }
  else if (memcmp(&IID_IPicture, riid, sizeof(IID_IPicture)) == 0) 
  {
    *ppvObject = (IPicture*)This;
  }
  else if (memcmp(&IID_IDispatch, riid, sizeof(IID_IDispatch)) == 0) 
  {
    *ppvObject = (IDispatch*)&(This->lpvtbl2);
  }
  else if (memcmp(&IID_IPictureDisp, riid, sizeof(IID_IPictureDisp)) == 0) 
  {
    *ppvObject = (IDispatch*)&(This->lpvtbl2);
  }
  /*  else if (memcmp(&IID_IPersistStream, riid, sizeof(IID_IPersistStream)) == 0) 
  {
  *ppvObject = (IPersistStream*)&(This->lpvtbl3);
  }*/
  
  /*
   * Check that we obtained an interface.
   */
  if ((*ppvObject)==0)
  {
    FIXME("() : asking for un supported interface %s\n",debugstr_guid(riid));
    return E_NOINTERFACE;
  }
  
  /*
   * Query Interface always increases the reference count by one when it is
   * successful
   */
  OLEPictureImpl_AddRef((IPicture*)This);

  return S_OK;;
}
        
/************************************************************************
 * OLEPictureImpl_AddRef (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static ULONG WINAPI OLEPictureImpl_AddRef( 
  IPicture* iface)
{
  ICOM_THIS(OLEPictureImpl, iface);
  TRACE("(%p)->(ref=%ld)\n", This, This->ref);
  This->ref++;

  return This->ref;
}
        
/************************************************************************
 * OLEPictureImpl_Release (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static ULONG WINAPI OLEPictureImpl_Release( 
      IPicture* iface)
{
  ICOM_THIS(OLEPictureImpl, iface);
  TRACE("(%p)->(ref=%ld)\n", This, This->ref);

  /*
   * Decrease the reference count on this object.
   */
  This->ref--;

  /*
   * If the reference count goes down to 0, perform suicide.
   */
  if (This->ref==0)
  {
    OLEPictureImpl_Destroy(This);

    return 0;
  }
  
  return This->ref;
}


/************************************************************************
 * OLEPictureImpl_get_Handle
 */ 
static HRESULT WINAPI OLEPictureImpl_get_Handle(IPicture *iface,
						OLE_HANDLE *phandle)
{
  ICOM_THIS(OLEPictureImpl, iface);
  TRACE("(%p)->(%p)\n", This, phandle);
  switch(This->desc.picType) {
  case PICTYPE_BITMAP:
    *phandle = This->desc.u.bmp.hbitmap;
    break;
  case PICTYPE_METAFILE:
    *phandle = This->desc.u.wmf.hmeta;
    break;
  case PICTYPE_ICON:
    *phandle = This->desc.u.icon.hicon;
    break;
  case PICTYPE_ENHMETAFILE:
    *phandle = This->desc.u.emf.hemf;
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
  ICOM_THIS(OLEPictureImpl, iface);
  FIXME("(%p)->(%p): stub\n", This, phandle);
  return E_NOTIMPL;
}

/************************************************************************
 * OLEPictureImpl_get_Type
 */ 
static HRESULT WINAPI OLEPictureImpl_get_Type(IPicture *iface,
					      short *ptype)
{
  ICOM_THIS(OLEPictureImpl, iface);
  TRACE("(%p)->(%p): type is %d\n", This, ptype, This->desc.picType);
  *ptype = This->desc.picType;
  return S_OK;
}

/************************************************************************
 * OLEPictureImpl_get_Width
 */ 
static HRESULT WINAPI OLEPictureImpl_get_Width(IPicture *iface,
					       OLE_XSIZE_HIMETRIC *pwidth)
{
  ICOM_THIS(OLEPictureImpl, iface);
  TRACE("(%p)->(%p): width is %ld\n", This, pwidth, This->himetricWidth);
  *pwidth = This->himetricWidth;
  return S_OK;
}

/************************************************************************
 * OLEPictureImpl_get_Height
 */ 
static HRESULT WINAPI OLEPictureImpl_get_Height(IPicture *iface,
						OLE_YSIZE_HIMETRIC *pheight)
{
  ICOM_THIS(OLEPictureImpl, iface);
  TRACE("(%p)->(%p): height is %ld\n", This, pheight, This->himetricHeight);
  *pheight = This->himetricHeight;
  return S_OK;
}

/************************************************************************
 * OLEPictureImpl_Render
 */ 
static HRESULT WINAPI OLEPictureImpl_Render(IPicture *iface, HDC hdc,
					    long x, long y, long cx, long cy,
					    OLE_XPOS_HIMETRIC xSrc,
					    OLE_YPOS_HIMETRIC ySrc,
					    OLE_XSIZE_HIMETRIC cxSrc,
					    OLE_YSIZE_HIMETRIC cySrc,
					    LPCRECT prcWBounds)
{
  ICOM_THIS(OLEPictureImpl, iface);
  TRACE("(%p)->(%08x, (%ld,%ld), (%ld,%ld) <- (%ld,%ld), (%ld,%ld), %p)\n",
	This, hdc, x, y, cx, cy, xSrc, ySrc, cxSrc, cySrc, prcWBounds);
  if(prcWBounds)
    TRACE("prcWBounds (%d,%d) - (%d,%d)\n", prcWBounds->left, prcWBounds->top,
	  prcWBounds->right, prcWBounds->bottom);

  switch(This->desc.picType) {
  case PICTYPE_BITMAP:
    {
      HBITMAP hbmpOld;
      HDC hdcBmp;

      /* Set a mapping mode that maps bitmap pixels into HIMETRIC units.
         NB y-axis gets flipped */

      hdcBmp = CreateCompatibleDC(0);
      SetMapMode(hdcBmp, MM_ANISOTROPIC);
      SetWindowOrgEx(hdcBmp, 0, 0, NULL);
      SetWindowExtEx(hdcBmp, This->himetricWidth, This->himetricHeight, NULL);
      SetViewportOrgEx(hdcBmp, 0, This->origHeight, NULL);
      SetViewportExtEx(hdcBmp, This->origWidth, -This->origHeight, NULL);

      hbmpOld = SelectObject(hdcBmp, This->desc.u.bmp.hbitmap);

      StretchBlt(hdc, x, y, cx, cy, hdcBmp, xSrc, ySrc, cxSrc, cySrc, SRCCOPY);

      SelectObject(hdcBmp, hbmpOld);
      DeleteDC(hdcBmp);
    }
    break;

  case PICTYPE_METAFILE:
  case PICTYPE_ICON:
  case PICTYPE_ENHMETAFILE:
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
  ICOM_THIS(OLEPictureImpl, iface);
  FIXME("(%p)->(%08x): stub\n", This, hpal);
  return E_NOTIMPL;
}

/************************************************************************
 * OLEPictureImpl_get_CurDC
 */ 
static HRESULT WINAPI OLEPictureImpl_get_CurDC(IPicture *iface,
					       HDC *phdc)
{
  ICOM_THIS(OLEPictureImpl, iface);
  FIXME("(%p)->(%p): stub\n", This, phdc);
  return E_NOTIMPL;
}

/************************************************************************
 * OLEPictureImpl_SelectPicture
 */ 
static HRESULT WINAPI OLEPictureImpl_SelectPicture(IPicture *iface,
						   HDC hdcIn,
						   HDC *phdcOut,
						   OLE_HANDLE *phbmpOut)
{
  ICOM_THIS(OLEPictureImpl, iface);
  FIXME("(%p)->(%08x, %p, %p): stub\n", This, hdcIn, phdcOut, phbmpOut);
  return E_NOTIMPL;
}

/************************************************************************
 * OLEPictureImpl_get_KeepOriginalFormat
 */ 
static HRESULT WINAPI OLEPictureImpl_get_KeepOriginalFormat(IPicture *iface,
							    BOOL *pfKeep)
{
  ICOM_THIS(OLEPictureImpl, iface);
  FIXME("(%p)->(%p): stub\n", This, pfKeep);
  return E_NOTIMPL;
}

/************************************************************************
 * OLEPictureImpl_put_KeepOriginalFormat
 */ 
static HRESULT WINAPI OLEPictureImpl_put_KeepOriginalFormat(IPicture *iface,
							    BOOL keep)
{
  ICOM_THIS(OLEPictureImpl, iface);
  FIXME("(%p)->(%d): stub\n", This, keep);
  return E_NOTIMPL;
}

/************************************************************************
 * OLEPictureImpl_PictureChanged
 */ 
static HRESULT WINAPI OLEPictureImpl_PictureChanged(IPicture *iface)
{
  ICOM_THIS(OLEPictureImpl, iface);
  FIXME("(%p)->(): stub\n", This);
  return E_NOTIMPL;
}

/************************************************************************
 * OLEPictureImpl_SaveAsFile
 */ 
static HRESULT WINAPI OLEPictureImpl_SaveAsFile(IPicture *iface,
						IStream *pstream,
						BOOL SaveMemCopy,
						LONG *pcbSize)
{
  ICOM_THIS(OLEPictureImpl, iface);
  FIXME("(%p)->(%p, %d, %p): stub\n", This, pstream, SaveMemCopy, pcbSize);
  return E_NOTIMPL;
}

/************************************************************************
 * OLEPictureImpl_get_Attributes
 */ 
static HRESULT WINAPI OLEPictureImpl_get_Attributes(IPicture *iface,
						    DWORD *pdwAttr)
{
  ICOM_THIS(OLEPictureImpl, iface);
  FIXME("(%p)->(%p): stub\n", This, pdwAttr);
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
  ICOM_THIS_From_IDispatch(IPicture, iface);

  return IPicture_QueryInterface(This, riid, ppvoid);
}

/************************************************************************
 * OLEPictureImpl_IDispatch_AddRef (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static ULONG WINAPI OLEPictureImpl_IDispatch_AddRef(
  IDispatch* iface)
{
  ICOM_THIS_From_IDispatch(IPicture, iface);

  return IPicture_AddRef(This);
}

/************************************************************************
 * OLEPictureImpl_IDispatch_Release (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static ULONG WINAPI OLEPictureImpl_IDispatch_Release(
  IDispatch* iface)
{
  ICOM_THIS_From_IDispatch(IPicture, iface);

  return IPicture_Release(This);
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
  FIXME("():Stub\n");

  return E_NOTIMPL;
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
  FIXME("():Stub\n");

  return E_NOTIMPL;
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
  FIXME("():Stub\n");

  return E_NOTIMPL;
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
  FIXME("():Stub\n");

  return E_NOTIMPL;
}


static ICOM_VTABLE(IPicture) OLEPictureImpl_VTable =
{
  ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
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

static ICOM_VTABLE(IDispatch) OLEPictureImpl_IDispatch_VTable =
{
  ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
  OLEPictureImpl_IDispatch_QueryInterface,
  OLEPictureImpl_IDispatch_AddRef,
  OLEPictureImpl_IDispatch_Release,
  OLEPictureImpl_GetTypeInfoCount,
  OLEPictureImpl_GetTypeInfo,
  OLEPictureImpl_GetIDsOfNames,
  OLEPictureImpl_Invoke
};

/***********************************************************************
 * OleCreatePictureIndirect
 */
HRESULT WINAPI OleCreatePictureIndirect(LPPICTDESC lpPictDesc, REFIID riid,
		            BOOL fOwn, LPVOID *ppvObj )
{
  OLEPictureImpl* newPict = NULL;
  HRESULT      hr         = S_OK;

  TRACE("(%p,%p,%d,%p)\n", lpPictDesc, riid, fOwn, ppvObj);

  /*
   * Sanity check
   */
  if (ppvObj==0)
    return E_POINTER;

  *ppvObj = NULL;

  /*
   * Try to construct a new instance of the class.
   */
  newPict = OLEPictureImpl_Construct(lpPictDesc, fOwn);

  if (newPict == NULL)
    return E_OUTOFMEMORY;

  /*
   * Make sure it supports the interface required by the caller.
   */
  hr = IPicture_QueryInterface((IPicture*)newPict, riid, ppvObj);

  /*
   * Release the reference obtained in the constructor. If
   * the QueryInterface was unsuccessful, it will free the class.
   */
  IPicture_Release((IPicture*)newPict);

  return hr;
}


/***********************************************************************
 * OleLoadPicture
 */
HRESULT WINAPI OleLoadPicture( LPSTREAM lpstream, LONG lSize, BOOL fRunmode,
		            REFIID reed, LPVOID *ppvObj )
{
  FIXME("(%p,%ld,%d,%p,%p), not implemented\n",
	lpstream, lSize, fRunmode, reed, ppvObj);
  return S_OK;
}
 
