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
#include "winerror.h"
#include "oleauto.h"
#include "ocidl.h"
#include "olectl.h"
#include "debugtools.h"
#include "heap.h"

DEFAULT_DEBUG_CHANNEL(ole)

/***********************************************************************
 * Declaration of constants used when serializing the font object.
 */
#define FONTPERSIST_ITALIC        0x02
#define FONTPERSIST_UNDERLINE     0x04
#define FONTPERSIST_STRIKETHROUGH 0x08

/***********************************************************************
 * Declaration of the implementation class for the IFont interface
 */
typedef struct OLEFontImpl OLEFontImpl;

struct OLEFontImpl
{
  /*
   * This class supports many interfaces. IUnknown, IFont, 
   * IDispatch, IDispFont and IPersistStream. The first two are
   * supported by the first vtablem the next two are supported by 
   * the second table and the last one has it's own.
   */
  ICOM_VTABLE(IFont)*     lpvtbl1;
  ICOM_VTABLE(IDispatch)* lpvtbl2;
  ICOM_VTABLE(IPersistStream)*            lpvtbl3;

  /*
   * Reference count for that instance of the class.
   */
  ULONG ref;

  /*
   * This structure contains the description of the class.
   */
  FONTDESC description;

  /*
   * Contain the font associated with this object.
   */
  HFONT gdiFont;

  /*
   * Font lock count.
   */
  DWORD fontLock;

  /*
   * Size ratio
   */
  long cyLogical;
  long cyHimetric;
};

/*
 * Here, I define utility macros to help with the casting of the 
 * "this" parameter.
 * There is a version to accomodate all of the VTables implemented
 * by this object.
 */
#define _ICOM_THIS(class,name) class* this = (class*)name;
#define _ICOM_THIS_From_IDispatch(class, name) class* this = (class*)(((char*)name)-sizeof(void*)); 
#define _ICOM_THIS_From_IPersistStream(class, name) class* this = (class*)(((char*)name)-2*sizeof(void*)); 

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

/***********************************************************************
 * Prototypes for the implementation functions for the IPersistStream
 * interface
 */
static HRESULT WINAPI OLEFontImpl_IPersistStream_QueryInterface(IPersistStream* iface, 
						    REFIID     riid, 
						    VOID**     ppvoid);
static ULONG   WINAPI OLEFontImpl_IPersistStream_AddRef(IPersistStream* iface);
static ULONG   WINAPI OLEFontImpl_IPersistStream_Release(IPersistStream* iface);
static HRESULT WINAPI OLEFontImpl_GetClassID(IPersistStream* iface, 
					     CLSID*                pClassID);
static HRESULT WINAPI OLEFontImpl_IsDirty(IPersistStream*  iface);
static HRESULT WINAPI OLEFontImpl_Load(IPersistStream*  iface,
				       IStream*         pLoadStream);
static HRESULT WINAPI OLEFontImpl_Save(IPersistStream*  iface,
				       IStream*         pOutStream,
				       BOOL             fClearDirty); 
static HRESULT WINAPI OLEFontImpl_GetSizeMax(IPersistStream*  iface,
					     ULARGE_INTEGER*  pcbSize); 

/*
 * Virtual function tables for the OLEFontImpl class.
 */
static ICOM_VTABLE(IFont) OLEFontImpl_VTable =
{
  ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
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
  ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
  OLEFontImpl_IDispatch_QueryInterface,
  OLEFontImpl_IDispatch_AddRef,
  OLEFontImpl_IDispatch_Release,
  OLEFontImpl_GetTypeInfoCount,
  OLEFontImpl_GetTypeInfo,
  OLEFontImpl_GetIDsOfNames,
  OLEFontImpl_Invoke
};

static ICOM_VTABLE(IPersistStream) OLEFontImpl_IPersistStream_VTable =
{
  ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
  OLEFontImpl_IPersistStream_QueryInterface,
  OLEFontImpl_IPersistStream_AddRef,
  OLEFontImpl_IPersistStream_Release,
  OLEFontImpl_GetClassID,
  OLEFontImpl_IsDirty,
  OLEFontImpl_Load,
  OLEFontImpl_Save,
  OLEFontImpl_GetSizeMax
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
  newObject->lpvtbl3 = &OLEFontImpl_IPersistStream_VTable;
  
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
  newObject->description.fStrikethrough = fontDesc->fStrikethrough;

  /*
   * Initializing all the other members.
   */
  newObject->gdiFont  = 0;
  newObject->fontLock = 0;
  newObject->cyHimetric = 1;
  newObject->cyLogical  = 1;

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

  if (fontDesc->gdiFont!=0)
    DeleteObject(fontDesc->gdiFont);

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
  else if (memcmp(&IID_IPersistStream, riid, sizeof(IID_IPersistStream)) == 0) 
  {
    *ppvObject = (IPersistStream*)&(this->lpvtbl3);
  }
  
  /*
   * Check that we obtained an interface.
   */
  if ((*ppvObject)==0)
  {
    char clsid[50];

    WINE_StringFromCLSID((LPCLSID)riid,clsid);
    
    WARN(
	 "() : asking for un supported interface %s\n", 
	 clsid);

    return E_NOINTERFACE;
  }
  
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

  psize->u.Hi = 0;
  psize->u.Lo = this->description.cySize.u.Lo;

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

  this->description.cySize.u.Hi = 0;
  this->description.cySize.u.Lo = this->description.cySize.u.Lo;

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
  FIXME("():Stub\n");
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
  FIXME("():Stub\n");
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

  *pstrikethrough = this->description.fStrikethrough;

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

  this->description.fStrikethrough = strikethrough;

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
  _ICOM_THIS(OLEFontImpl, iface);

  if (phfont==NULL)
    return E_POINTER;

  /*
   * Realize the font if necessary
 */
  if (this->gdiFont==0)
{
    LOGFONTW logFont;
    INT      fontHeight;
    CY       cySize;
    
    /*
     * The height of the font returned by the get_Size property is the
     * height of the font in points multiplied by 10000... Using some
     * simple conversions and the ratio given by the application, it can
     * be converted to a height in pixels.
     */
    IFont_get_Size(iface, &cySize);

    fontHeight = MulDiv(cySize.u.Lo, 2540L, 72L);
    fontHeight = MulDiv(fontHeight, this->cyLogical,this->cyHimetric);

    memset(&logFont, 0, sizeof(LOGFONTW));

    logFont.lfHeight          = ((fontHeight%10000L)>5000L) ? (-fontHeight/10000L)-1 :
                                                              (-fontHeight/10000L);
    logFont.lfItalic          = this->description.fItalic;
    logFont.lfUnderline       = this->description.fUnderline;
    logFont.lfStrikeOut       = this->description.fStrikethrough;
    logFont.lfWeight          = this->description.sWeight;
    logFont.lfCharSet         = this->description.sCharset;
    logFont.lfOutPrecision    = OUT_CHARACTER_PRECIS;
    logFont.lfClipPrecision   = CLIP_DEFAULT_PRECIS;
    logFont.lfQuality         = DEFAULT_QUALITY;
    logFont.lfPitchAndFamily  = DEFAULT_PITCH;
    lstrcpyW(logFont.lfFaceName,this->description.lpstrName);

    this->gdiFont = CreateFontIndirectW(&logFont);
  }

  *phfont = this->gdiFont;

  return S_OK;
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
  OLEFontImpl* newObject = 0;

  _ICOM_THIS(OLEFontImpl, iface);

  if (ppfont == NULL)
    return E_POINTER;

  *ppfont = NULL;

  /*
   * Allocate space for the object.
   */
  newObject = HeapAlloc(GetProcessHeap(), 0, sizeof(OLEFontImpl));

  if (newObject==NULL)
    return E_OUTOFMEMORY;

  *newObject = *this;

  /*
   * That new object starts with a reference count of 1
   */
  newObject->ref          = 1;

  *ppfont = (IFont*)newObject;

  return S_OK;
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
  FIXME("():Stub\n");
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
  _ICOM_THIS(OLEFontImpl, iface);

  this->cyLogical  = cyLogical;
  this->cyHimetric = cyHimetric;

  return S_OK;
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
  FIXME("():Stub\n");
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
  _ICOM_THIS(OLEFontImpl, iface);

  if ( (hfont == 0) ||
       (hfont != this->gdiFont) )
    return E_INVALIDARG;

  this->fontLock++;

  return S_OK;
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
  _ICOM_THIS(OLEFontImpl, iface);

  if ( (hfont == 0) ||
       (hfont != this->gdiFont) )
    return E_INVALIDARG;

  this->fontLock--;

  /*
   * If we just released our last font reference, destroy it.
   */
  if (this->fontLock==0)
  {
    DeleteObject(this->gdiFont);
    this->gdiFont = 0; 
}

  return S_OK;
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
  FIXME("():Stub\n");
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
  FIXME("():Stub\n");

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
  FIXME("():Stub\n");

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
  FIXME("():Stub\n");

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
  FIXME("():Stub\n");

  return E_NOTIMPL;
}

/************************************************************************
 * OLEFontImpl_IPersistStream_QueryInterface (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static HRESULT WINAPI OLEFontImpl_IPersistStream_QueryInterface(
  IPersistStream* iface,
  REFIID     riid,
  VOID**     ppvoid)
{
  _ICOM_THIS_From_IPersistStream(IFont, iface);

  return IFont_QueryInterface(this, riid, ppvoid);
}

/************************************************************************
 * OLEFontImpl_IPersistStream_Release (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static ULONG WINAPI OLEFontImpl_IPersistStream_Release(
  IPersistStream* iface)
{
  _ICOM_THIS_From_IPersistStream(IFont, iface);

  return IFont_Release(this);
}

/************************************************************************
 * OLEFontImpl_IPersistStream_AddRef (IUnknown)
 *
 * See Windows documentation for more details on IUnknown methods.
 */
static ULONG WINAPI OLEFontImpl_IPersistStream_AddRef(
  IPersistStream* iface)
{
  _ICOM_THIS_From_IPersistStream(IFont, iface);

  return IFont_AddRef(this);
}

/************************************************************************
 * OLEFontImpl_GetClassID (IPersistStream)
 *
 * See Windows documentation for more details on IPersistStream methods.
 */
static HRESULT WINAPI OLEFontImpl_GetClassID(
  IPersistStream* iface, 
  CLSID*                pClassID)
{
  if (pClassID==0)
    return E_POINTER;

  memcpy(pClassID, &CLSID_StdFont, sizeof(CLSID_StdFont));

  return S_OK;
}

/************************************************************************
 * OLEFontImpl_IsDirty (IPersistStream)
 *
 * See Windows documentation for more details on IPersistStream methods.
 */
static HRESULT WINAPI OLEFontImpl_IsDirty(
  IPersistStream*  iface)
{
  return S_OK;
}

/************************************************************************
 * OLEFontImpl_Load (IPersistStream)
 *
 * See Windows documentation for more details on IPersistStream methods.
 *
 * This is the format of the standard font serialization as far as I
 * know
 *
 * Offset   Type   Value           Comment
 * 0x0000   Byte   Unknown         Probably a version number, contains 0x01
 * 0x0001   Short  Charset         Charset value from the FONTDESC structure
 * 0x0003   Byte   Attributes      Flags defined as follows:
 *                                     00000010 - Italic
 *                                     00000100 - Underline
 *                                     00001000 - Strikethrough
 * 0x0004   Short  Weight          Weight value from FONTDESC structure
 * 0x0006   DWORD  size            "Low" portion of the cySize member of the FONTDESC
 *                                 structure/
 * 0x000A   Byte   name length     Length of the font name string (no null character)
 * 0x000B   String name            Name of the font (ASCII, no nul character)
 */
static HRESULT WINAPI OLEFontImpl_Load(
  IPersistStream*  iface,
  IStream*         pLoadStream)
{
  char  readBuffer[0x100];
  ULONG cbRead;
  BYTE  bVersion;
  BYTE  bAttributes;
  BYTE  bStringSize;

  _ICOM_THIS_From_IPersistStream(OLEFontImpl, iface);
  
  /*
   * Read the version byte
   */
  IStream_Read(pLoadStream, &bVersion, 1, &cbRead);

  if ( (cbRead!=1) ||
       (bVersion!=0x01) )
    return E_FAIL;

  /*
   * Charset
   */
  IStream_Read(pLoadStream, &this->description.sCharset, 2, &cbRead);

  if (cbRead!=2)
    return E_FAIL;

  /*
   * Attributes
   */
  IStream_Read(pLoadStream, &bAttributes, 1, &cbRead);

  if (cbRead!=1)
    return E_FAIL;

  this->description.fItalic        = (bAttributes & FONTPERSIST_ITALIC) != 0;
  this->description.fStrikethrough = (bAttributes & FONTPERSIST_STRIKETHROUGH) != 0;
  this->description.fUnderline     = (bAttributes & FONTPERSIST_UNDERLINE) != 0;
    
  /*
   * Weight
   */
  IStream_Read(pLoadStream, &this->description.sWeight, 2, &cbRead);

  if (cbRead!=2)
    return E_FAIL;

  /*
   * Size
   */
  IStream_Read(pLoadStream, &this->description.cySize.u.Lo, 4, &cbRead);

  if (cbRead!=4)
    return E_FAIL;

  this->description.cySize.u.Hi = 0;

  /*
   * FontName
   */
  IStream_Read(pLoadStream, &bStringSize, 1, &cbRead);

  if (cbRead!=1)
    return E_FAIL;

  memset(readBuffer, 0, 0x100);
  IStream_Read(pLoadStream, readBuffer, bStringSize, &cbRead);

  if (cbRead!=bStringSize)
    return E_FAIL;

  if (this->description.lpstrName!=0)
    HeapFree(GetProcessHeap(), 0, this->description.lpstrName);

  this->description.lpstrName = HEAP_strdupAtoW(GetProcessHeap(), 
						    HEAP_ZERO_MEMORY,
						    readBuffer);

  return S_OK;
}

/************************************************************************
 * OLEFontImpl_Save (IPersistStream)
 *
 * See Windows documentation for more details on IPersistStream methods.
 */
static HRESULT WINAPI OLEFontImpl_Save(
  IPersistStream*  iface,
  IStream*         pOutStream,
  BOOL             fClearDirty)
{
  char* writeBuffer = NULL;
  ULONG cbWritten;
  BYTE  bVersion = 0x01;
  BYTE  bAttributes;
  BYTE  bStringSize;
  
  _ICOM_THIS_From_IPersistStream(OLEFontImpl, iface);

  /*
   * Read the version byte
   */
  IStream_Write(pOutStream, &bVersion, 1, &cbWritten);

  if (cbWritten!=1)
    return E_FAIL;

  /*
   * Charset
   */
  IStream_Write(pOutStream, &this->description.sCharset, 2, &cbWritten);

  if (cbWritten!=2)
    return E_FAIL;

  /*
   * Attributes
   */
  bAttributes = 0;

  if (this->description.fItalic)
    bAttributes |= FONTPERSIST_ITALIC;

  if (this->description.fStrikethrough)
    bAttributes |= FONTPERSIST_STRIKETHROUGH;
  
  if (this->description.fUnderline)
    bAttributes |= FONTPERSIST_UNDERLINE;

  IStream_Write(pOutStream, &bAttributes, 1, &cbWritten);

  if (cbWritten!=1)
    return E_FAIL;
  
  /*
   * Weight
   */
  IStream_Write(pOutStream, &this->description.sWeight, 2, &cbWritten);

  if (cbWritten!=2)
    return E_FAIL;

  /*
   * Size
   */
  IStream_Write(pOutStream, &this->description.cySize.u.Lo, 4, &cbWritten);

  if (cbWritten!=4)
    return E_FAIL;

  /*
   * FontName
   */
  if (this->description.lpstrName!=0)
    bStringSize = lstrlenW(this->description.lpstrName);
  else
    bStringSize = 0;

  IStream_Write(pOutStream, &bStringSize, 1, &cbWritten);

  if (cbWritten!=1)
    return E_FAIL;

  if (bStringSize!=0)
  {
    writeBuffer = HEAP_strdupWtoA(GetProcessHeap(), 
				  HEAP_ZERO_MEMORY,
				  this->description.lpstrName);

    if (writeBuffer==0)
      return E_OUTOFMEMORY;

    IStream_Write(pOutStream, writeBuffer, bStringSize, &cbWritten);
    
    HeapFree(GetProcessHeap(), 0, writeBuffer);

    if (cbWritten!=bStringSize)
      return E_FAIL;
  }

  return S_OK;
}

/************************************************************************
 * OLEFontImpl_GetSizeMax (IPersistStream)
 *
 * See Windows documentation for more details on IPersistStream methods.
 */
static HRESULT WINAPI OLEFontImpl_GetSizeMax(
  IPersistStream*  iface,
  ULARGE_INTEGER*  pcbSize)
{
  _ICOM_THIS_From_IPersistStream(OLEFontImpl, iface);

  if (pcbSize==NULL)
    return E_POINTER;

  pcbSize->HighPart = 0;
  pcbSize->LowPart = 0;

  pcbSize->LowPart += sizeof(BYTE);  /* Version */
  pcbSize->LowPart += sizeof(WORD);  /* Lang code */
  pcbSize->LowPart += sizeof(BYTE);  /* Flags */
  pcbSize->LowPart += sizeof(WORD);  /* Weight */
  pcbSize->LowPart += sizeof(DWORD); /* Size */
  pcbSize->LowPart += sizeof(BYTE);  /* StrLength */

  if (this->description.lpstrName!=0)
    pcbSize->LowPart += lstrlenW(this->description.lpstrName);

  return S_OK;
}
