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
#define ICOM_INTERFACE IWebBrowser
#define IWebBrowser_METHODS \
	ICOM_METHOD(HRESULT,GoBack) \
	ICOM_METHOD(HRESULT,GoForward) \
	ICOM_METHOD(HRESULT,GoHome) \
	ICOM_METHOD(HRESULT,GoSearch) \
	ICOM_METHOD5(HRESULT,Navigate, BSTR,URL, VARIANT*,Flags, VARIANT*,TargetFrameName, \
                                       VARIANT*,PostData, VARIANT*,Headers) \
	ICOM_METHOD(HRESULT,Refresh) \
	ICOM_METHOD1(HRESULT,Refresh2, VARIANT*,Level) \
	ICOM_METHOD(HRESULT,Stop) \
	ICOM_METHOD1(HRESULT,get_Application, void**,ppDisp) \
	ICOM_METHOD1(HRESULT,get_Parent, void**,ppDisp) \
	ICOM_METHOD1(HRESULT,get_Container, void**,ppDisp) \
	ICOM_METHOD1(HRESULT,get_Document, void**,ppDisp) \
	ICOM_METHOD1(HRESULT,get_TopLevelContainer, VARIANT*,pBool) \
	ICOM_METHOD1(HRESULT,get_Type, BSTR*,Type) \
	ICOM_METHOD1(HRESULT,get_Left, long*,pl) \
	ICOM_METHOD1(HRESULT,put_Left, long,Left) \
	ICOM_METHOD1(HRESULT,get_Top, long*,pl) \
	ICOM_METHOD1(HRESULT,put_Top, long,Top) \
	ICOM_METHOD1(HRESULT,get_Width, long*,pl) \
	ICOM_METHOD1(HRESULT,put_Width, long,Width) \
	ICOM_METHOD1(HRESULT,get_Height, long*,pl) \
	ICOM_METHOD1(HRESULT,put_Height, long,Height) \
	ICOM_METHOD1(HRESULT,get_LocationName, BSTR*,LocationName) \
	ICOM_METHOD1(HRESULT,get_LocationURL, BSTR*,LocationURL) \
	ICOM_METHOD1(HRESULT,get_Busy, VARIANT*,pBool)
#define IWebBrowser_IMETHODS \
	IDispatch_METHODS \
	IWebBrowser_METHODS
ICOM_DEFINE(IWebBrowser,IDispatch)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define IWebBrowser_QueryInterface(p,a,b)      ICOM_CALL2(QueryInterface,p,a,b)
#define IWebBrowser_AddRef(p)                  ICOM_CALL (AddRef,p)
#define IWebBrowser_Release(p)                 ICOM_CALL (Release,p)
/*** IDispatch methods ***/
#define IWebBrowser_GetTypeInfoCount(p,a)      ICOM_CALL1 (GetTypeInfoCount,p,a)
#define IWebBrowser_GetTypeInfo(p,a,b,c)       ICOM_CALL3 (GetTypeInfo,p,a,b,c)
#define IWebBrowser_GetIDsOfNames(p,a,b,c,d,e) ICOM_CALL5 (GetIDsOfNames,p,a,b,c,d,e)
#define IWebBrowser_Invoke(p,a,b,c,d,e,f,g,h)  ICOM_CALL8 (Invoke,p,a,b,c,d,e,f,g,h)
/*** IWebBrowser methods ***/
#define IWebBrowser_GoBack(p,a)      ICOM_CALL1(GoBack,p,a)
#define IWebBrowser_GoForward(p,a)      ICOM_CALL1(GoForward,p,a)
#define IWebBrowser_GoHome(p,a)      ICOM_CALL1(GoHome,p,a)
#define IWebBrowser_GoSearch(p,a)      ICOM_CALL1(GoSearch,p,a)
#define IWebBrowser_Navigate(p,a)      ICOM_CALL1(Navigate,p,a)
#define IWebBrowser_Refresh(p,a)      ICOM_CALL1(Refresh,p,a)
#define IWebBrowser_Refresh2(p,a)      ICOM_CALL1(Refresh2,p,a)
#define IWebBrowser_Stop(p,a)      ICOM_CALL1(Stop,p,a)
#define IWebBrowser_get_Application(p,a)      ICOM_CALL1(get_Application,p,a)
#define IWebBrowser_get_Parent(p,a)      ICOM_CALL1(get_Parent,p,a)
#define IWebBrowser_get_Container(p,a)      ICOM_CALL1(get_Container,p,a)
#define IWebBrowser_get_Document(p,a)      ICOM_CALL1(get_Document,p,a)
#define IWebBrowser_get_TopLevelContainer(p,a)      ICOM_CALL1(get_TopLevelContainer,p,a)
#define IWebBrowser_get_Type(p,a)      ICOM_CALL1(get_Type,p,a)
#define IWebBrowser_get_Left(p,a)      ICOM_CALL1(get_Left,p,a)
#define IWebBrowser_put_Left(p,a)      ICOM_CALL1(put_Left,p,a)
#define IWebBrowser_get_Top(p,a)      ICOM_CALL1(get_Top,p,a)
#define IWebBrowser_put_Top(p,a)      ICOM_CALL1(put_Top,p,a)
#define IWebBrowser_get_Width(p,a)      ICOM_CALL1(get_Width,p,a)
#define IWebBrowser_put_Width(p,a)      ICOM_CALL1(put_Width,p,a)
#define IWebBrowser_get_Height(p,a)      ICOM_CALL1(get_Height,p,a)
#define IWebBrowser_put_Height(p,a)      ICOM_CALL1(put_Height,p,a)
#define IWebBrowser_get_LocationName(p,a)      ICOM_CALL1(get_LocationName,p,a)
#define IWebBrowser_get_LocationURL(p,a)      ICOM_CALL1(get_LocationURL,p,a)
#define IWebBrowser_get_Busy(p,a)      ICOM_CALL1(get_Busy,p,a)

#define ICOM_INTERFACE IWebBrowserApp
#define IWebBrowserApp_METHODS \
    ICOM_METHOD(HRESULT,Quit) \
    ICOM_METHOD2(HRESULT,ClientToWindow,int*,pcx,int*,pcy) \
    ICOM_METHOD2(HRESULT,PutProperty,BSTR,szProperty,VARIANT,vtValue) \
    ICOM_METHOD2(HRESULT,GetProperty,BSTR,szProperty,VARIANT*,pvtValue) \
    ICOM_METHOD1(HRESULT,get_Name,BSTR*,Name) \
    ICOM_METHOD1(HRESULT,get_HWND,long*,pHWND) \
    ICOM_METHOD1(HRESULT,get_FullName,BSTR*,FullName) \
    ICOM_METHOD1(HRESULT,get_Path,BSTR*,Path) \
    ICOM_METHOD1(HRESULT,get_Visible,VARIANT_BOOL*,pBool) \
    ICOM_METHOD1(HRESULT,put_Visible,VARIANT_BOOL,Value) \
    ICOM_METHOD1(HRESULT,get_StatusBar,VARIANT_BOOL*,pBool) \
    ICOM_METHOD1(HRESULT,put_StatusBar,VARIANT_BOOL,Value) \
    ICOM_METHOD1(HRESULT,get_StatusText,BSTR*,StatusText) \
    ICOM_METHOD1(HRESULT,put_StatusText,BSTR,StatusText) \
    ICOM_METHOD1(HRESULT,get_ToolBar,int*,Value) \
    ICOM_METHOD1(HRESULT,put_ToolBar,int,Value) \
    ICOM_METHOD1(HRESULT,get_MenuBar,VARIANT_BOOL*,Value) \
    ICOM_METHOD1(HRESULT,put_MenuBar,VARIANT_BOOL,Value) \
    ICOM_METHOD1(HRESULT,get_FullScreen,VARIANT_BOOL*,pbFullScreen) \
    ICOM_METHOD1(HRESULT,put_FullScreen,VARIANT_BOOL,bFullScreen)
#define IWebBrowserApp_IMETHODS \
    IDispatch_METHODS \
    IWebBrowser_METHODS \
    IWebBrowserApp_METHODS
ICOM_DEFINE(IWebBrowserApp,IWebBrowser)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define IWebBrowserApp_QueryInterface(p,a,b)      ICOM_CALL2(QueryInterface,p,a,b)
#define IWebBrowserApp_AddRef(p)                  ICOM_CALL (AddRef,p)
#define IWebBrowserApp_Release(p)                 ICOM_CALL (Release,p)
/*** IDispatch methods ***/
#define IWebBrowserApp_GetTypeInfoCount(p,a)      ICOM_CALL1 (GetTypeInfoCount,p,a)
#define IWebBrowserApp_GetTypeInfo(p,a,b,c)       ICOM_CALL3 (GetTypeInfo,p,a,b,c)
#define IWebBrowserApp_GetIDsOfNames(p,a,b,c,d,e) ICOM_CALL5 (GetIDsOfNames,p,a,b,c,d,e)
#define IWebBrowserApp_Invoke(p,a,b,c,d,e,f,g,h)  ICOM_CALL8 (Invoke,p,a,b,c,d,e,f,g,h)
/*** IWebBrowser methods ***/
#define IWebBrowserApp_GoBack(p,a)                ICOM_CALL1(GoBack,p,a)
#define IWebBrowserApp_GoForward(p,a)             ICOM_CALL1(GoForward,p,a)
#define IWebBrowserApp_GoHome(p,a)                ICOM_CALL1(GoHome,p,a)
#define IWebBrowserApp_GoSearch(p,a)              ICOM_CALL1(GoSearch,p,a)
#define IWebBrowserApp_Navigate(p,a)              ICOM_CALL1(Navigate,p,a)
#define IWebBrowserApp_Refresh(p,a)               ICOM_CALL1(Refresh,p,a)
#define IWebBrowserApp_Refresh2(p,a)              ICOM_CALL1(Refresh2,p,a)
#define IWebBrowserApp_Stop(p,a)                  ICOM_CALL1(Stop,p,a)
#define IWebBrowserApp_get_Application(p,a)       ICOM_CALL1(get_Application,p,a)
#define IWebBrowserApp_get_Parent(p,a)            ICOM_CALL1(get_Parent,p,a)
#define IWebBrowserApp_get_Container(p,a)         ICOM_CALL1(get_Container,p,a)
#define IWebBrowserApp_get_Document(p,a)          ICOM_CALL1(get_Document,p,a)
#define IWebBrowserApp_get_TopLevelContainer(p,a) ICOM_CALL1(get_TopLevelContainer,p,a)
#define IWebBrowserApp_get_Type(p,a)              ICOM_CALL1(get_Type,p,a)
#define IWebBrowserApp_get_Left(p,a)              ICOM_CALL1(get_Left,p,a)
#define IWebBrowserApp_put_Left(p,a)              ICOM_CALL1(put_Left,p,a)
#define IWebBrowserApp_get_Top(p,a)               ICOM_CALL1(get_Top,p,a)
#define IWebBrowserApp_put_Top(p,a)               ICOM_CALL1(put_Top,p,a)
#define IWebBrowserApp_get_Width(p,a)             ICOM_CALL1(get_Width,p,a)
#define IWebBrowserApp_put_Width(p,a)             ICOM_CALL1(put_Width,p,a)
#define IWebBrowserApp_get_Height(p,a)            ICOM_CALL1(get_Height,p,a)
#define IWebBrowserApp_put_Height(p,a)            ICOM_CALL1(put_Height,p,a)
#define IWebBrowserApp_get_LocationName(p,a)      ICOM_CALL1(get_LocationName,p,a)
#define IWebBrowserApp_get_LocationURL(p,a)       ICOM_CALL1(get_LocationURL,p,a)
#define IWebBrowserApp_get_Busy(p,a)              ICOM_CALL1(get_Busy,p,a)
/*** IWebBrowserApp methods ***/
#define IWebBrowserApp_Quit(p)                    ICOM_CALL(Quit,p,a)
#define IWebBrowserApp_ClientToWindow(p,a,b)      ICOM_CALL2(ClientToWindow,p,a,b)
#define IWebBrowserApp_PutProperty(p,a,b)         ICOM_CALL2(PutProperty,p,a,b)
#define IWebBrowserApp_GetProperty(p,a,b)         ICOM_CALL2(GetProperty,p,a,b)
#define IWebBrowserApp_get_Name(p,a)              ICOM_CALL1(get_Name,p,a)
#define IWebBrowserApp_get_HWND(p,a)              ICOM_CALL1(get_HWND,p,a)
#define IWebBrowserApp_get_FullName(p,a)          ICOM_CALL1(get_FullName,p,a)
#define IWebBrowserApp_get_Path(p,a)              ICOM_CALL1(get_Path,p,a)
#define IWebBrowserApp_get_Visible(p,a)           ICOM_CALL1(get_Visible,p,a)
#define IWebBrowserApp_put_Visible(p,a)           ICOM_CALL1(put_Visible,p,a)
#define IWebBrowserApp_get_StatusBar(p,a)         ICOM_CALL1(get_StatusBar,p,a)
#define IWebBrowserApp_put_StatusBar(p,a)         ICOM_CALL1(put_StatusBar,p,a)
#define IWebBrowserApp_get_StatusText(p,a)        ICOM_CALL1(get_StatusText,p,a)
#define IWebBrowserApp_put_StatusText(p,a)        ICOM_CALL1(put_StatusText,p,a)
#define IWebBrowserApp_get_ToolBar(p,a)           ICOM_CALL1(get_ToolBar,p,a)
#define IWebBrowserApp_put_ToolBar(p,a)           ICOM_CALL1(put_ToolBar,p,a)
#define IWebBrowserApp_get_MenuBar(p,a)           ICOM_CALL1(get_MenuBar,p,a)
#define IWebBrowserApp_put_MenuBar(p,a)           ICOM_CALL1(put_MenuBar,p,a)
#define IWebBrowserApp_get_FullScreen(p,a)        ICOM_CALL1(get_FullScreen,p,a)
#define IWebBrowserApp_put_FullScreen(p,a)        ICOM_CALL1(put_FullScreen,p,a)

#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */

#endif /* __WINE_WINE_OBJ_WEBBROWSER_H */
