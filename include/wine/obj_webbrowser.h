/*
 * Defines the COM interfaces and APIs related to the IE Web browser control
 *
 * Copyright 2001 John R. Sheets (for CodeWeavers)
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __WINE_WINE_OBJ_WEBBROWSER_H
#define __WINE_WINE_OBJ_WEBBROWSER_H

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */


/*****************************************************************************
 * Predeclare the interfaces and class IDs
 */
DEFINE_GUID(IID_IWebBrowser, 0xeab22ac1, 0x30c1, 0x11cf, 0xa7, 0xeb, 0x00, 0x00, 0xc0, 0x5b, 0xae, 0x0b);
typedef struct IWebBrowser IWebBrowser, *LPWEBBROWSER;

DEFINE_GUID(CLSID_WebBrowser, 0x8856f961, 0x340a, 0x11d0, 0xa9, 0x6b, 0x00, 0xc0, 0x4f, 0xd7, 0x05, 0xa2);

DEFINE_GUID(IID_IWebBrowserApp, 0x0002df05, 0x0000, 0x0000, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46);
typedef struct IWebBrowserApp IWebBrowserApp, *LPWEBBROWSERAPP;

/*****************************************************************************
 * IWebBrowser* related typedefs
 */

typedef enum BrowserNavConstants
{
    navOpenInNewWindow   = 0x1,
    navNoHistory         = 0x2,
    navNoReadFromCache   = 0x4,
    navNoWriteToCache    = 0x8,
    navAllowAutosearch   = 0x10,
    navBrowserBar        = 0x20,
    navHyperlink         = 0x40,
    navEnforceRestricted = 0x80
} BrowserNavConstants;

typedef enum RefreshConstants
{
    REFRESH_NORMAL     = 0,
    REFRESH_IFEXPIRED  = 1,
    REFRESH_COMPLETELY = 3
} RefreshConstants;

/*****************************************************************************
 * IWebBrowser interface
 */
#define INTERFACE IWebBrowser
#define IWebBrowser_METHODS \
	IDispatch_METHODS \
	STDMETHOD(GoBack)(THIS) PURE; \
	STDMETHOD(GoForward)(THIS) PURE; \
	STDMETHOD(GoHome)(THIS) PURE; \
	STDMETHOD(GoSearch)(THIS) PURE; \
	STDMETHOD(Navigate)(THIS_ BSTR URL, VARIANT *Flags, VARIANT *TargetFrameName, \
                            VARIANT *PostData, VARIANT *Headers) PURE; \
	STDMETHOD(Refresh)(THIS) PURE; \
	STDMETHOD(Refresh2)(THIS_ VARIANT *Level) PURE; \
	STDMETHOD(Stop)(THIS) PURE; \
	STDMETHOD(get_Application)(THIS_ void **ppDisp) PURE; \
	STDMETHOD(get_Parent)(THIS_ void **ppDisp) PURE; \
	STDMETHOD(get_Container)(THIS_ void **ppDisp) PURE; \
	STDMETHOD(get_Document)(THIS_ void **ppDisp) PURE; \
	STDMETHOD(get_TopLevelContainer)(THIS_ VARIANT *pBool) PURE; \
	STDMETHOD(get_Type)(THIS_ BSTR *Type) PURE; \
	STDMETHOD(get_Left)(THIS_ long *pl) PURE; \
	STDMETHOD(put_Left)(THIS_ long Left) PURE; \
	STDMETHOD(get_Top)(THIS_ long *pl) PURE; \
	STDMETHOD(put_Top)(THIS_ long Top) PURE; \
	STDMETHOD(get_Width)(THIS_ long *pl) PURE; \
	STDMETHOD(put_Width)(THIS_ long Width) PURE; \
	STDMETHOD(get_Height)(THIS_ long *pl) PURE; \
	STDMETHOD(put_Height)(THIS_ long Height) PURE; \
	STDMETHOD(get_LocationName)(THIS_ BSTR *LocationName) PURE; \
	STDMETHOD(get_LocationURL)(THIS_ BSTR *LocationURL) PURE; \
	STDMETHOD(get_Busy)(THIS_ VARIANT *pBool) PURE;
ICOM_DEFINE(IWebBrowser,IDispatch)
#undef INTERFACE

#ifdef COBJMACROS
/*** IUnknown methods ***/
#define IWebBrowser_QueryInterface(p,a,b)      (p)->lpVtbl->QueryInterface(p,a,b)
#define IWebBrowser_AddRef(p)                  (p)->lpVtbl->AddRef(p)
#define IWebBrowser_Release(p)                 (p)->lpVtbl->Release(p)
/*** IDispatch methods ***/
#define IWebBrowser_GetTypeInfoCount(p,a)      (p)->lpVtbl->GetTypeInfoCount(p,a)
#define IWebBrowser_GetTypeInfo(p,a,b,c)       (p)->lpVtbl->GetTypeInfo(p,a,b,c)
#define IWebBrowser_GetIDsOfNames(p,a,b,c,d,e) (p)->lpVtbl->GetIDsOfNames(p,a,b,c,d,e)
#define IWebBrowser_Invoke(p,a,b,c,d,e,f,g,h)  (p)->lpVtbl->Invoke(p,a,b,c,d,e,f,g,h)
/*** IWebBrowser methods ***/
#define IWebBrowser_GoBack(p,a)      (p)->lpVtbl->GoBack(p,a)
#define IWebBrowser_GoForward(p,a)      (p)->lpVtbl->GoForward(p,a)
#define IWebBrowser_GoHome(p,a)      (p)->lpVtbl->GoHome(p,a)
#define IWebBrowser_GoSearch(p,a)      (p)->lpVtbl->GoSearch(p,a)
#define IWebBrowser_Navigate(p,a)      (p)->lpVtbl->Navigate(p,a)
#define IWebBrowser_Refresh(p,a)      (p)->lpVtbl->Refresh(p,a)
#define IWebBrowser_Refresh2(p,a)      (p)->lpVtbl->Refresh2(p,a)
#define IWebBrowser_Stop(p,a)      (p)->lpVtbl->Stop(p,a)
#define IWebBrowser_get_Application(p,a)      (p)->lpVtbl->get_Application(p,a)
#define IWebBrowser_get_Parent(p,a)      (p)->lpVtbl->get_Parent(p,a)
#define IWebBrowser_get_Container(p,a)      (p)->lpVtbl->get_Container(p,a)
#define IWebBrowser_get_Document(p,a)      (p)->lpVtbl->get_Document(p,a)
#define IWebBrowser_get_TopLevelContainer(p,a)      (p)->lpVtbl->get_TopLevelContainer(p,a)
#define IWebBrowser_get_Type(p,a)      (p)->lpVtbl->get_Type(p,a)
#define IWebBrowser_get_Left(p,a)      (p)->lpVtbl->get_Left(p,a)
#define IWebBrowser_put_Left(p,a)      (p)->lpVtbl->put_Left(p,a)
#define IWebBrowser_get_Top(p,a)      (p)->lpVtbl->get_Top(p,a)
#define IWebBrowser_put_Top(p,a)      (p)->lpVtbl->put_Top(p,a)
#define IWebBrowser_get_Width(p,a)      (p)->lpVtbl->get_Width(p,a)
#define IWebBrowser_put_Width(p,a)      (p)->lpVtbl->put_Width(p,a)
#define IWebBrowser_get_Height(p,a)      (p)->lpVtbl->get_Height(p,a)
#define IWebBrowser_put_Height(p,a)      (p)->lpVtbl->put_Height(p,a)
#define IWebBrowser_get_LocationName(p,a)      (p)->lpVtbl->get_LocationName(p,a)
#define IWebBrowser_get_LocationURL(p,a)      (p)->lpVtbl->get_LocationURL(p,a)
#define IWebBrowser_get_Busy(p,a)      (p)->lpVtbl->get_Busy(p,a)
#endif

#define INTERFACE IWebBrowserApp
#define IWebBrowserApp_METHODS \
    IWebBrowser_METHODS \
    STDMETHOD(Quit)(THIS) PURE; \
    STDMETHOD(ClientToWindow)(THIS_ int *pcx,int *pcy) PURE; \
    STDMETHOD(PutProperty)(THIS_ BSTR szProperty,VARIANT vtValue) PURE; \
    STDMETHOD(GetProperty)(THIS_ BSTR szProperty,VARIANT *pvtValue) PURE; \
    STDMETHOD(get_Name)(THIS_ BSTR *Name) PURE; \
    STDMETHOD(get_HWND)(THIS_ long *pHWND) PURE; \
    STDMETHOD(get_FullName)(THIS_ BSTR *FullName) PURE; \
    STDMETHOD(get_Path)(THIS_ BSTR *Path) PURE; \
    STDMETHOD(get_Visible)(THIS_ VARIANT_BOOL *pBool) PURE; \
    STDMETHOD(put_Visible)(THIS_ VARIANT_BOOL Value) PURE; \
    STDMETHOD(get_StatusBar)(THIS_ VARIANT_BOOL *pBool) PURE; \
    STDMETHOD(put_StatusBar)(THIS_ VARIANT_BOOL Value) PURE; \
    STDMETHOD(get_StatusText)(THIS_ BSTR *StatusText) PURE; \
    STDMETHOD(put_StatusText)(THIS_ BSTR StatusText) PURE; \
    STDMETHOD(get_ToolBar)(THIS_ int *Value) PURE; \
    STDMETHOD(put_ToolBar)(THIS_ int Value) PURE; \
    STDMETHOD(get_MenuBar)(THIS_ VARIANT_BOOL *Value) PURE; \
    STDMETHOD(put_MenuBar)(THIS_ VARIANT_BOOL Value) PURE; \
    STDMETHOD(get_FullScreen)(THIS_ VARIANT_BOOL *pbFullScreen) PURE; \
    STDMETHOD(put_FullScreen)(THIS_ VARIANT_BOOL bFullScreen) PURE;
ICOM_DEFINE(IWebBrowserApp,IWebBrowser)
#undef INTERFACE

#ifdef COBJMACROS
/*** IUnknown methods ***/
#define IWebBrowserApp_QueryInterface(p,a,b)      (p)->lpVtbl->QueryInterface(p,a,b)
#define IWebBrowserApp_AddRef(p)                  (p)->lpVtbl->AddRef(p)
#define IWebBrowserApp_Release(p)                 (p)->lpVtbl->Release(p)
/*** IDispatch methods ***/
#define IWebBrowserApp_GetTypeInfoCount(p,a)      (p)->lpVtbl->GetTypeInfoCount(p,a)
#define IWebBrowserApp_GetTypeInfo(p,a,b,c)       (p)->lpVtbl->GetTypeInfo(p,a,b,c)
#define IWebBrowserApp_GetIDsOfNames(p,a,b,c,d,e) (p)->lpVtbl->GetIDsOfNames(p,a,b,c,d,e)
#define IWebBrowserApp_Invoke(p,a,b,c,d,e,f,g,h)  (p)->lpVtbl->Invoke(p,a,b,c,d,e,f,g,h)
/*** IWebBrowser methods ***/
#define IWebBrowserApp_GoBack(p,a)                (p)->lpVtbl->GoBack(p,a)
#define IWebBrowserApp_GoForward(p,a)             (p)->lpVtbl->GoForward(p,a)
#define IWebBrowserApp_GoHome(p,a)                (p)->lpVtbl->GoHome(p,a)
#define IWebBrowserApp_GoSearch(p,a)              (p)->lpVtbl->GoSearch(p,a)
#define IWebBrowserApp_Navigate(p,a)              (p)->lpVtbl->Navigate(p,a)
#define IWebBrowserApp_Refresh(p,a)               (p)->lpVtbl->Refresh(p,a)
#define IWebBrowserApp_Refresh2(p,a)              (p)->lpVtbl->Refresh2(p,a)
#define IWebBrowserApp_Stop(p,a)                  (p)->lpVtbl->Stop(p,a)
#define IWebBrowserApp_get_Application(p,a)       (p)->lpVtbl->get_Application(p,a)
#define IWebBrowserApp_get_Parent(p,a)            (p)->lpVtbl->get_Parent(p,a)
#define IWebBrowserApp_get_Container(p,a)         (p)->lpVtbl->get_Container(p,a)
#define IWebBrowserApp_get_Document(p,a)          (p)->lpVtbl->get_Document(p,a)
#define IWebBrowserApp_get_TopLevelContainer(p,a) (p)->lpVtbl->get_TopLevelContainer(p,a)
#define IWebBrowserApp_get_Type(p,a)              (p)->lpVtbl->get_Type(p,a)
#define IWebBrowserApp_get_Left(p,a)              (p)->lpVtbl->get_Left(p,a)
#define IWebBrowserApp_put_Left(p,a)              (p)->lpVtbl->put_Left(p,a)
#define IWebBrowserApp_get_Top(p,a)               (p)->lpVtbl->get_Top(p,a)
#define IWebBrowserApp_put_Top(p,a)               (p)->lpVtbl->put_Top(p,a)
#define IWebBrowserApp_get_Width(p,a)             (p)->lpVtbl->get_Width(p,a)
#define IWebBrowserApp_put_Width(p,a)             (p)->lpVtbl->put_Width(p,a)
#define IWebBrowserApp_get_Height(p,a)            (p)->lpVtbl->get_Height(p,a)
#define IWebBrowserApp_put_Height(p,a)            (p)->lpVtbl->put_Height(p,a)
#define IWebBrowserApp_get_LocationName(p,a)      (p)->lpVtbl->get_LocationName(p,a)
#define IWebBrowserApp_get_LocationURL(p,a)       (p)->lpVtbl->get_LocationURL(p,a)
#define IWebBrowserApp_get_Busy(p,a)              (p)->lpVtbl->get_Busy(p,a)
/*** IWebBrowserApp methods ***/
#define IWebBrowserApp_Quit(p)                    (p)->lpVtbl->Quit(p,a)
#define IWebBrowserApp_ClientToWindow(p,a,b)      (p)->lpVtbl->ClientToWindow(p,a,b)
#define IWebBrowserApp_PutProperty(p,a,b)         (p)->lpVtbl->PutProperty(p,a,b)
#define IWebBrowserApp_GetProperty(p,a,b)         (p)->lpVtbl->GetProperty(p,a,b)
#define IWebBrowserApp_get_Name(p,a)              (p)->lpVtbl->get_Name(p,a)
#define IWebBrowserApp_get_HWND(p,a)              (p)->lpVtbl->get_HWND(p,a)
#define IWebBrowserApp_get_FullName(p,a)          (p)->lpVtbl->get_FullName(p,a)
#define IWebBrowserApp_get_Path(p,a)              (p)->lpVtbl->get_Path(p,a)
#define IWebBrowserApp_get_Visible(p,a)           (p)->lpVtbl->get_Visible(p,a)
#define IWebBrowserApp_put_Visible(p,a)           (p)->lpVtbl->put_Visible(p,a)
#define IWebBrowserApp_get_StatusBar(p,a)         (p)->lpVtbl->get_StatusBar(p,a)
#define IWebBrowserApp_put_StatusBar(p,a)         (p)->lpVtbl->put_StatusBar(p,a)
#define IWebBrowserApp_get_StatusText(p,a)        (p)->lpVtbl->get_StatusText(p,a)
#define IWebBrowserApp_put_StatusText(p,a)        (p)->lpVtbl->put_StatusText(p,a)
#define IWebBrowserApp_get_ToolBar(p,a)           (p)->lpVtbl->get_ToolBar(p,a)
#define IWebBrowserApp_put_ToolBar(p,a)           (p)->lpVtbl->put_ToolBar(p,a)
#define IWebBrowserApp_get_MenuBar(p,a)           (p)->lpVtbl->get_MenuBar(p,a)
#define IWebBrowserApp_put_MenuBar(p,a)           (p)->lpVtbl->put_MenuBar(p,a)
#define IWebBrowserApp_get_FullScreen(p,a)        (p)->lpVtbl->get_FullScreen(p,a)
#define IWebBrowserApp_put_FullScreen(p,a)        (p)->lpVtbl->put_FullScreen(p,a)
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */

#endif /* __WINE_WINE_OBJ_WEBBROWSER_H */
