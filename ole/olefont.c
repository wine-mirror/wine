/*
 * OLE Font encapsulation implementation
 *
 * This file contains an implementation of the IFont
 * interface and the OleCreateFontIndirect API call.
 *
 * Copyright 1999 Francis Beaudet
 */
#include <assert.h>
#include <string.h>
#include "windows.h"
#include "winerror.h"
#include "oleauto.h"
#include "ocidl.h"
#include "olectl.h"
#include "debug.h"

/***********************************************************************
 * Declaration of the implemetation class for the IFont interface
 */
typedef struct OLEFontImpl OLEFontImpl;

struct OLEFontImpl
{
  /*
   * This class supports many interfaces. IUnknown, IFont, 
   * IDispatch and IDispFont. The first two are supported by
   * the first vtablem the other two are supported by the second
   * table.
   */
  ICOM_VTABLE(IFont)*     lpvtbl1;
  ICOM_VTABLE(IDispatch)* lpvtbl2;

  /*
   * Reference count for that instance of the class.
   */
  ULONG ref;

  /*
   * This structure contains the description of the class.
   */
  FONTDESC description;
};

/*
 * Here, I define utility macros to help with the casting of the 
 * "this" parameter.
 * There is a version to accomodate the first vtable and a version to
 * accomodate the second one.
 */
#define _ICOM_THIS(class,name) class* this = (class*)name;
#define _ICOM_THIS_From_IDispatch(class, name) class* this = (class*)(((void*)name)-sizeof(void*)); 

/***********************************************************************
 * Prototypes for the implementation functions for the IFont
 * interface
 */
static OLEFontImpl* OLEFontImpl_Construct(LPFONTDESC fontDesc);
static void         OLEFontImpl_Destroy(OLEFontImpl* fontDesc);
static HRESULT      WINAPI OLEFontImpl_QueryInterface(IFont* iface, REFIID riid, VOID** ppvoid);
static ULONG        WINAPI OLEFontImpl_AddRef(IFont* iface);
static ULONG        WINAPI OLEFontImpl_Release(IFont* iface);
static HRESULT      WINAPI OLEFontImpl_get_Name(IFont* iface, BSTR* pname);
static HRESULT      WINAPI OLEFontImpl_put_Name(IFont* iface, BSTR name);
static HRESULT      WINAPI OLEFontImpl_get_Size(IFont* iface, CY* psize);
static HRESULT      WINAPI OLEFontImpl_put_Size(IFont* iface, CY size);
static HRESULT      WINAPI OLEFontImpl_get_Bold(IFont* iface, BOOL* pbold);
static HRESULT      WINAPI OLEFontImpl_put_Bold(IFont* iface, BOOL bold);
static HRESULT      WINAPI OLEFontImpl_get_Italic(IFont* iface, BOOL* pitalic);
static HRESULT      WINAPI OLEFontImpl_put_Italic(IFont* iface, BOOL italic);
static HRESULT      WINAPI OLEFontImpl_get_Underline(IFont* iface, BOOL* punderline);
static HRESULT      WINAPI OLEFontImpl_put_Underline(IFont* iface, BOOL underline);
static HRESULT      WINAPI OLEFontImpl_get_Strikethrough(IFont* iface, BOOL* pstrikethrough);
static HRESULT      WINAPI OLEFontImpl_put_Strikethrough(IFont* iface, BOOL strikethrough);
static HRESULT      WINAPI OLEFontImpl_get_Weight(IFont* iface, short* pweight);
static HRESULT      WINAPI OLEFontImpl_put_Weight(IFont* iface, short weight);
static HRESULT      WINAPI OLEFontImpl_get_Charset(IFont* iface, short* pcharset);
static HRESULT      WINAPI OLEFontImpl_put_Charset(IFont* iface, short charset);
static HRESULT      WINAPI OLEFontImpl_get_hFont(IFont* iface, HFONT* phfont);
static HRESULT      WINAPI OLEFontImpl_put_hFont(IFont* iface, HFONT hfont);
static HRESULT      WINAPI OLEFontImpl_Clone(IFont* iface, IFont** ppfont);
static HRESULT      WINAPI OLEFontImpl_IsEqual(IFont* iface, IFont* pFontOther);
static HRESULT      WINAPI OLEFontImpl_SetRatio(IFont* iface, long cyLogical, long cyHimetric);
static HRESULT      WINAPI OLEFontImpl_QueryTextMetrics(IFont* iface, TEXTMETRICOLE* ptm);
static HRESULT      WINAPI OLEFontImpl_AddRefHfont(IFont* iface, HFONT hfont);
static HRESULT      WINAPI OLEFontImpl_ReleaseHfont(IFont* iface, HFONT hfont);
static HRESULT      WINAPI OLEFontImpl_SetHdc(IFont* iface, HDC hdc);

/***********************************************************************
 * Prototypes for the implementation functions for the IDispatch
 * interface
 */
static HRESULT WINAPI OLEFontImpl_IDispatch_QueryInterface(IDispatch* iface, 
						    REFIID     riid, 
						    VOID**     ppvoid);
static ULONG   WINAPI OLEFontImpl_IDispatch_AddRef(IDispatch* iface);
static ULONG   WINAPI OLEFontImpl_IDispatch_Release(IDispatch* iface);
static HRESULT WINAPI OLEFontImpl_GetTypeInfoCount(IDispatch*    iface, 
					           unsigned int* pctinfo);
static HRESULT WINAPI OLEFontImpl_GetTypeInfo(IDispatch*  iface, 
				       	      UINT      iTInfo,
				              LCID        lcid, 
				              ITypeInfo** ppTInfo);
static HRESULT WINAPI OLEFontImpl_GetIDsOfNames(IDispatch*  iface,
					        REFIID      riid, 
					        LPOLESTR* rgszNames, 
					        UINT      cNames, 
					        LCID        lcid,
					        DISPID*     rgDispId);
static HRESULT WINAPI OLEFontImpl_Invoke(IDispatch*  iface,
				         DISPID      dispIdMember, 
				         REFIID      riid, 
				         LCID        lcid, 
				         WORD        wFlags,
				         DISPPARAMS* pDispParams,
				         VARIANT*    pVarResult, 
				         EXCEPINFO*  pExepInfo,
				         UINT*     puArgErr); 

/*
 * Virtual function tables for the OLEFontImpl class.
 */
static ICOM_VTABLE(IFont) OLEFontImpl_VTable =
{
  OLEFontImpl_QueryInterface,
  OLEFontImpl_AddRef,
  OLEFontImpl_Release,
  OLEFontImpl_get_Name,
  OLEFontImpl_put_Name,
  OLEFontImpl_get_Size,
  OLEFontImpl_put_Size,
  OLEFontImpl_get_Bold,
  OLEFontImpl_put_Bold,
  OLEFontImpl_get_Italic,
  OLEFontImpl_put_Italic,
  OLEFontImpl_get_Underline,
  OLEFontImpl_put_Underline,
  OLEFontImpl_get_Strikethrough,
  OLEFontImpl_put_Strikethrough,
  OLEFontImpl_get_Weight,
  OLEFontImpl_put_Weight,
  OLEFontImpl_get_Charset,
  OLEFontImpl_put_Charset,
  OLEFontImpl_get_hFont,
  OLEFontImpl_put_hFont,
  OLEFontImpl_Clone, 
  OLEFontImpl_IsEqual,
  OLEFontImpl_SetRatio,
  OLEFontImpl_QueryTextMetrics,
  OLEFontImpl_AddRefHfont,
  OLEFontImpl_ReleaseHfont,
  OLEFontImpl_SetHdc
};

static ICOM_VTABLE(IDispatch) OLEFontImpl_IDispatch_VTable =
{
  OLEFontImpl_IDispatch_QueryInterface,
  OLEFontImpl_IDispatch_AddRef,
  OLEFontImpl_IDispatch_Release,
  OLEFontImpl_GetTypeInfoCount,
  OLEFontImpl_GetTypeInfo,
  OLEFontImpl_GetIDsOfNames,
  OLEFontImpl_Invoke
};


/******************************************************************************
 *		OleCreateFontIndirect	[OLEAUT32.420]
 */
WINOLECTLAPI OleCreateFontIndirect(
  LPFONTDESC lpFontDesc,
  REFIID     riid,
  VOID**     ppvObj)
{
  OLEFontImpl* newFont = 0;
  HRESULT      hr      = S_OK;

  /*
   * Sanity check
   */
  if (ppvObj==0)
    return E_POINTER;

  *ppvObj = 0;

  /*
   * Try to construct a new instance of the class.
   */
  newFont = OLEFontImpl_Construct(lpFontDesc);

  if (newFont == 0)
    return E_OUTOFMEMORY;

  /*
   * Make sure it supports the interface required by the caller.
   */
  hr = IFont_QueryInterface((IFont*)newFont, riid, ppvObj);

  /*
   * Release the reference obtained in the constructor. If
   * the QueryInterface was unsuccessful, it will free the class.
   */
  IFont_Release((IFont*)newFont);

  return hr;
}


/***********************************************************************
 * Implementation of the OLEFontImpl class.
 */

/************************************************************************
 * OLEFontImpl_Construct
 *
 * This method will construct a new instance of the OLEFontImpl
 * class.
 *
 * The caller of this method must release the object when it's
 * done with it.
 */
static OLEFontImpl* OLEFontImpl_Construct(LPFONTDESC fontDesc)
{
  OLEFontImpl* newObject = 0;

  /*
   * Allocate space for the object.
   */
  newObject = HeapAlloc(GetProcessHeap(), 0, sizeof(OLEFontImpl));

  if (newObject==0)
    return newObject;
  
  /*
   * Initialize the virtual function table.
   */
  newObject->lpvtbl1 = &OLEFontImpl_VTable;
  newObject->lpvtbl2 = &OLEFontImpl_IDispatch_VTable;
  
  /*
   * Start with one reference count. The caller of this function 
   * must release the interface pointer when it is done.
   */
  newObject->ref = 1;

  /*
   * Copy the description of the font in the object.
   */
  assert(fontDesc->cbSizeofstruct >= sizeof(FONTDESC));

  newObject->description.cbSizeofstruct = sizeof(FONTDESC);
  newObject->description.lpstrName = HeapAlloc(GetProcessHeap(),
					       0, 
					       (lstrlenW(fontDesc->lpstrName)+1) * sizeof(WCHAR));
  lstrcpyW(newObject->description.lpstrName, fontDesc->lpstrName);
  newObject->description.cySize         = fontDesc->cySize;
  newObject->description.sWeight        = fontDesc->sWeight;
  newObject->description.sCharset       = fontDesc->sCharset;
  newObject->description.fItalic        = fontDesc->fItalic;
  newObject->description.fUnderline     = fontDesc->fUnderline;
  newObject->description.fStrikeThrough = fontDesc->fStrikeThrough;

  return newObject;
}

/************************************************************************
 * OLEFontImpl_Construct
 *
 * This method is called by the Release method when the reference
 * count goes doen to 0. it will free all resources used by
 * this object.
 */
static void OLEFontImpl_Destroy(OLEFontImpl* fontDesc)
{
  if (fontDesc->description.lpstrName!=0)
    HeapFree(GetProcessHeap(), 0, fontDesc->description.lpstrName);

  HeapFree(GetProcessHeap(), 0, fontDesc);
}


/************************************************************************
 * OLEFontImpl_QueryInterface (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 */
HRESULT WINAPI OLEFontImpl_QueryInterface(
  IFont*  iface,
  REFIID  riid,
  void**  ppvObject)
{
  _ICOM_THIS(OLEFontImpl, iface);

  /*
   * Perform a sanity check on the parameters.
   */
  if ( (this==0) || (ppvObject==0) )
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
    *ppvObject = (IFont*)this;
  }
  else if (memcmp(&IID_IFont, riid, sizeof(IID_IFont)) == 0) 
  {
    *ppvObject = (IFont*)this;
  }
  else if (memcmp(&IID_IDispatch, riid, sizeof(IID_IDispatch)) == 0) 
  {
    *ppvObject = (IDispatch*)&(this->lpvtbl2);
  }
  else if (memcmp(&IID_IFontDisp, riid, sizeof(IID_IFontDisp)) == 0) 
  {
    *ppvObject = (IDispatch*)&(this->lpvtbl2);
  }
  
  /*
   * Check that we obtained an interface.
   */
  if ((*ppvObject)==0)
    return E_NOINTERFACE;
  
  /*
   * Query Interface always increases the reference count by one when it is
   * successful
   */
  OLEFontImpl_AddRef((IFont*)this);

  return S_OK;;
}
        
/************************************************************************
 * OLEFontImpl_AddRef (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 */
ULONG WINAPI OLEFontImpl_AddRef( 
  IFont* iface)
{
  _ICOM_THIS(OLEFontImpl, iface);

  this->ref++;

  return this->ref;
}
        
/************************************************************************
 * OLEFontImpl_Release (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 */
ULONG WINAPI OLEFontImpl_Release( 
      IFont* iface)
{
  _ICOM_THIS(OLEFontImpl, iface);

  /*
   * Decrease the reference count on this object.
   */
  this->ref--;

  /*
   * If the reference count goes down to 0, perform suicide.
   */
  if (this->ref==0)
  {
    OLEFontImpl_Destroy(this);

    return 0;
  }
  
  return this->ref;
}
        
/************************************************************************
 * OLEFontImpl_get_Name (IFont)
 *
 * See Windows documentation for more details on IFont methods.
 */
static HRESULT WINAPI OLEFontImpl_get_Name(
  IFont*  iface, 
  BSTR* pname)
{
  _ICOM_THIS(OLEFontImpl, iface);

  /*
   * Sanity check.
   */
  if (pname==0)
    return E_POINTER;

  if (this->description.lpstrName!=0)
    *pname = SysAllocString(this->description.lpstrName);
  else
    *pname = 0;

  return S_OK;
}

/************************************************************************
 * OLEFontImpl_put_Name (IFont)
 *
 * See Windows documentation for more details on IFont methods.
 */
static HRESULT WINAPI OLEFontImpl_put_Name(
  IFont* iface, 
  BSTR name)
{
  _ICOM_THIS(OLEFontImpl, iface);

  if (this->description.lpstrName==0)
  {
    this->description.lpstrName = HeapAlloc(GetProcessHeap(),
					    0, 
					    (lstrlenW(name)+1) * sizeof(WCHAR));
  }
  else
  {
    this->description.lpstrName = HeapReAlloc(GetProcessHeap(),
					      0, 
					      this->description.lpstrName,
					      (lstrlenW(name)+1) * sizeof(WCHAR));
  }

  if (this->description.lpstrName==0)
    return E_OUTOFMEMORY;

  lstrcpyW(this->description.lpstrName, name);

  return S_OK;
}

/************************************************************************
 * OLEFontImpl_get_Size (IFont)
 *
 * See Windows documentation for more details on IFont methods.
 */
static HRESULT WINAPI OLEFontImpl_get_Size(
  IFont* iface, 
  CY*    psize)
{
  _ICOM_THIS(OLEFontImpl, iface);

  /*
   * Sanity check
   */
  if (psize==0)
    return E_POINTER;

  *psize = this->description.cySize;

  return S_OK;
}

/************************************************************************
 * OLEFontImpl_put_Size (IFont)
 *
 * See Windows documentation for more details on IFont methods.
 */
static HRESULT WINAPI OLEFontImpl_put_Size(
  IFont* iface, 
  CY     size)
{
  _ICOM_THIS(OLEFontImpl, iface);

  this->description.cySize = size;

  return S_OK;
}

/************************************************************************
 * OLEFontImpl_get_Bold (IFont)
 *
 * See Windows documentation for more details on IFont methods.
 */
static HRESULT WINAPI OLEFontImpl_get_Bold(
  IFont*  iface, 
  BOOL* pbold)
{
  FIXME(ole,"():Stub\n");
  return E_NOTIMPL;
}

/************************************************************************
 * OLEFontImpl_put_Bold (IFont)
 *
 * See Windows documentation for more details on IFont methods.
 */
static HRESULT WINAPI OLEFontImpl_put_Bold(
  IFont* iface,
  BOOL bold)
{
  FIXME(ole,"():Stub\n");
  return E_NOTIMPL;
}

/************************************************************************
 * OLEFontImpl_get_Italic (IFont)
 *
 * See Windows documentation for more details on IFont methods.
 */
static HRESULT WINAPI OLEFontImpl_get_Italic(
  IFont*  iface, 
  BOOL* pitalic)
{
  _ICOM_THIS(OLEFontImpl, iface);

  /*
   * Sanity check
   */
  if (pitalic==0)
    return E_POINTER;

  *pitalic = this->description.fItalic;

  return S_OK;
}

/************************************************************************
 * OLEFontImpl_put_Italic (IFont)
 *
 * See Windows documentation for more details on IFont methods.
 */
static HRESULT WINAPI OLEFontImpl_put_Italic(
  IFont* iface, 
  BOOL italic)
{
  _ICOM_THIS(OLEFontImpl, iface);

  this->description.fItalic = italic;

  return S_OK;
}

/************************************************************************
 * OLEFontImpl_get_Underline (IFont)
 *
 * See Windows documentation for more details on IFont methods.
 */
static HRESULT WINAPI OLEFontImpl_get_Underline(
  IFont*  iface, 
  BOOL* punderline)
{
  _ICOM_THIS(OLEFontImpl, iface);

  /*
   * Sanity check
   */
  if (punderline==0)
    return E_POINTER;

  *punderline = this->description.fUnderline;

  return S_OK;
}

/************************************************************************
 * OLEFontImpl_put_Underline (IFont)
 *
 * See Windows documentation for more details on IFont methods.
 */
static HRESULT WINAPI OLEFontImpl_put_Underline(
  IFont* iface,
  BOOL underline)
{
  _ICOM_THIS(OLEFontImpl, iface);

  this->description.fUnderline = underline;

  return S_OK;
}

/************************************************************************
 * OLEFontImpl_get_Strikethrough (IFont)
 *
 * See Windows documentation for more details on IFont methods.
 */
static HRESULT WINAPI OLEFontImpl_get_Strikethrough(
  IFont*  iface, 
  BOOL* pstrikethrough)
{
  _ICOM_THIS(OLEFontImpl, iface);

  /*
   * Sanity check
   */
  if (pstrikethrough==0)
    return E_POINTER;

  *pstrikethrough = this->description.fStrikeThrough;

  return S_OK;
}

/************************************************************************
 * OLEFontImpl_put_Strikethrough (IFont)
 *
 * See Windows documentation for more details on IFont methods.
 */
static HRESULT WINAPI OLEFontImpl_put_Strikethrough(
 IFont* iface, 
 BOOL strikethrough)
{
  _ICOM_THIS(OLEFontImpl, iface);

  this->description.fStrikeThrough = strikethrough;

  return S_OK;
}

/************************************************************************
 * OLEFontImpl_get_Weight (IFont)
 *
 * See Windows documentation for more details on IFont methods.
 */
static HRESULT WINAPI OLEFontImpl_get_Weight(
  IFont* iface, 
  short* pweight)
{
  _ICOM_THIS(OLEFontImpl, iface);

  /*
   * Sanity check
   */
  if (pweight==0)
    return E_POINTER;

  *pweight = this->description.sWeight;

  return S_OK;
}

/************************************************************************
 * OLEFontImpl_put_Weight (IFont)
 *
 * See Windows documentation for more details on IFont methods.
 */
static HRESULT WINAPI OLEFontImpl_put_Weight(
  IFont* iface, 
  short  weight)
{
  _ICOM_THIS(OLEFontImpl, iface);

  this->description.sWeight = weight;

  return S_OK;
}

/************************************************************************
 * OLEFontImpl_get_Charset (IFont)
 *
 * See Windows documentation for more details on IFont methods.
 */
static HRESULT WINAPI OLEFontImpl_get_Charset(
  IFont* iface, 
  short* pcharset)
{
  _ICOM_THIS(OLEFontImpl, iface);

  /*
   * Sanity check
   */
  if (pcharset==0)
    return E_POINTER;

  *pcharset = this->description.sCharset;

  return S_OK;
}

/************************************************************************
 * OLEFontImpl_put_Charset (IFont)
 *
 * See Windows documentation for more details on IFont methods.
 */
static HRESULT WINAPI OLEFontImpl_put_Charset(
  IFont* iface, 
  short charset)
{
  _ICOM_THIS(OLEFontImpl, iface);

  this->description.sCharset = charset;

  return S_OK;
}

/************************************************************************
 * OLEFontImpl_get_hFont (IFont)
 *
 * See Windows documentation for more details on IFont methods.
 */
static HRESULT WINAPI OLEFontImpl_get_hFont(
  IFont*   iface,
  HFONT* phfont)
{
  FIXME(ole,"():Stub\n");
  return E_NOTIMPL;
}

/************************************************************************
 * OLEFontImpl_put_hFont (IFont)
 *
 * See Windows documentation for more details on IFont methods.
 */
static HRESULT WINAPI OLEFontImpl_put_hFont(
  IFont*  iface,
  HFONT hfont)
{
  FIXME(ole,"():Stub\n");
  return E_NOTIMPL;
}

/************************************************************************
 * OLEFontImpl_Clone (IFont)
 *
 * See Windows documentation for more details on IFont methods.
 */
static HRESULT WINAPI OLEFontImpl_Clone(
  IFont*  iface,
  IFont** ppfont)
{
  FIXME(ole,"():Stub\n");
  return E_NOTIMPL;
}

/************************************************************************
 * OLEFontImpl_IsEqual (IFont)
 *
 * See Windows documentation for more details on IFont methods.
 */
static HRESULT WINAPI OLEFontImpl_IsEqual(
  IFont* iface, 
  IFont* pFontOther)
{
  FIXME(ole,"():Stub\n");
  return E_NOTIMPL;
}

/************************************************************************
 * OLEFontImpl_SetRatio (IFont)
 *
 * See Windows documentation for more details on IFont methods.
 */
static HRESULT WINAPI OLEFontImpl_SetRatio(
  IFont* iface,
  long   cyLogical,
  long   cyHimetric)
{
  FIXME(ole,"():Stub\n");
  return E_NOTIMPL;
}

/************************************************************************
 * OLEFontImpl_QueryTextMetrics (IFont)
 *
 * See Windows documentation for more details on IFont methods.
 */
static HRESULT      WINAPI OLEFontImpl_QueryTextMetrics(
  IFont*         iface, 
  TEXTMETRICOLE* ptm)
{
  FIXME(ole,"():Stub\n");
  return E_NOTIMPL;
}

/************************************************************************
 * OLEFontImpl_AddRefHfont (IFont)
 *
 * See Windows documentation for more details on IFont methods.
 */
static HRESULT WINAPI OLEFontImpl_AddRefHfont(
  IFont*  iface, 
  HFONT hfont)
{
  FIXME(ole,"():Stub\n");
  return E_NOTIMPL;
}

/************************************************************************
 * OLEFontImpl_ReleaseHfont (IFont)
 *
 * See Windows documentation for more details on IFont methods.
 */
static HRESULT WINAPI OLEFontImpl_ReleaseHfont(
  IFont*  iface,
  HFONT hfont)
{
  FIXME(ole,"():Stub\n");
  return E_NOTIMPL;
}

/************************************************************************
 * OLEFontImpl_SetHdc (IFont)
 *
 * See Windows documentation for more details on IFont methods.
 */
static HRESULT WINAPI OLEFontImpl_SetHdc(
  IFont* iface,
  HDC  hdc)
{
  FIXME(ole,"():Stub\n");
  return E_NOTIMPL;
}

/************************************************************************
 * OLEFontImpl_IDispatch_QueryInterface (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static HRESULT WINAPI OLEFontImpl_IDispatch_QueryInterface(
  IDispatch* iface,
  REFIID     riid,
  VOID**     ppvoid)
{
  _ICOM_THIS_From_IDispatch(IFont, iface);

  return IFont_QueryInterface(this, riid, ppvoid);
}

/************************************************************************
 * OLEFontImpl_IDispatch_Release (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static ULONG WINAPI OLEFontImpl_IDispatch_Release(
  IDispatch* iface)
{
  _ICOM_THIS_From_IDispatch(IFont, iface);

  return IFont_Release(this);
}

/************************************************************************
 * OLEFontImpl_IDispatch_AddRef (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static ULONG WINAPI OLEFontImpl_IDispatch_AddRef(
  IDispatch* iface)
{
  _ICOM_THIS_From_IDispatch(IFont, iface);

  return IFont_AddRef(this);
}

/************************************************************************
 * OLEFontImpl_GetTypeInfoCount (IDispatch)
 *
 * See Windows documentation for more details on IDispatch methods.
 */
static HRESULT WINAPI OLEFontImpl_GetTypeInfoCount(
  IDispatch*    iface, 
  unsigned int* pctinfo)
{
  FIXME(ole,"():Stub\n");

  return E_NOTIMPL;
}

/************************************************************************
 * OLEFontImpl_GetTypeInfo (IDispatch)
 *
 * See Windows documentation for more details on IDispatch methods.
 */
static HRESULT WINAPI OLEFontImpl_GetTypeInfo(
  IDispatch*  iface, 
  UINT      iTInfo,
  LCID        lcid, 
  ITypeInfo** ppTInfo)
{
  FIXME(ole,"():Stub\n");

  return E_NOTIMPL;
}

/************************************************************************
 * OLEFontImpl_GetIDsOfNames (IDispatch)
 *
 * See Windows documentation for more details on IDispatch methods.
 */
static HRESULT WINAPI OLEFontImpl_GetIDsOfNames(
  IDispatch*  iface,
  REFIID      riid, 
  LPOLESTR* rgszNames, 
  UINT      cNames, 
  LCID        lcid,
  DISPID*     rgDispId)
{
  FIXME(ole,"():Stub\n");

  return E_NOTIMPL;
}

/************************************************************************
 * OLEFontImpl_Invoke (IDispatch)
 *
 * See Windows documentation for more details on IDispatch methods.
 */
static HRESULT WINAPI OLEFontImpl_Invoke(
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
  FIXME(ole,"():Stub\n");

  return E_NOTIMPL;
}


