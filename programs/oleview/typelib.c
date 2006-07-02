/*
 * OleView (typelib.c)
 *
 * Copyright 2006 Piotr Caban
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

#include "main.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(oleview);

TYPELIB typelib;
static const WCHAR wszTypeLib[] = { 'T','Y','P','E','L','I','B','\0' };

static const WCHAR wszFailed[] = { '<','f','a','i','l','e','d','>','\0' };
static const WCHAR wszSpace[] = { ' ','\0' };
static const WCHAR wszAsterix[] = { '*','\0' };

static const WCHAR wszVT_BOOL[]
    = { 'V','A','R','I','A','N','T','_','B','O','O','L','\0' };
static const WCHAR wszVT_UI1[]
    = { 'u','n','s','i','g','n','e','d',' ','c','h','a','r','\0' };
static const WCHAR wszVT_UI4[]
    = { 'u','n','s','i','g','n','e','d',' ','l','o','n','g','\0' };
static const WCHAR wszVT_I4[] = { 'l','o','n','g','\0' };
static const WCHAR wszVT_R4[] = { 's','i','n','g','l','e','\0' };
static const WCHAR wszVT_INT[] = { 'i','n','t','\0' };
static const WCHAR wszVT_BSTR[] = { 'B','S','T','R','\0' };
static const WCHAR wszVT_CY[] = { 'C','U','R','R','E','N','C','Y','\0' };

void AddToStrW(WCHAR *wszDest, const WCHAR *wszSource)
{
    lstrcpyW(&wszDest[lstrlenW(wszDest)], wszSource);
}

void CreateTypeInfo(WCHAR *wszAddTo, TYPEDESC tdesc, ITypeInfo *pTypeInfo)
{
    BSTR bstrData;
    HRESULT hRes;
    ITypeInfo *pRefTypeInfo;

    switch(tdesc.vt&VT_TYPEMASK)
    {
#define VTADDTOSTR(x) case x:\
        AddToStrW(wszAddTo, wsz##x);\
        break
        VTADDTOSTR(VT_BOOL);
        VTADDTOSTR(VT_UI1);
        VTADDTOSTR(VT_UI4);
        VTADDTOSTR(VT_I4);
        VTADDTOSTR(VT_R4);
        VTADDTOSTR(VT_INT);
        VTADDTOSTR(VT_BSTR);
        VTADDTOSTR(VT_CY);
        case VT_PTR:
        CreateTypeInfo(wszAddTo, *U(tdesc).lptdesc, pTypeInfo);
        AddToStrW(wszAddTo, wszAsterix);
        break;
        case VT_USERDEFINED:
        hRes = ITypeInfo_GetRefTypeInfo(pTypeInfo,
                U(tdesc).hreftype, &pRefTypeInfo);
        if(SUCCEEDED(hRes))
        {
            ITypeInfo_GetDocumentation(pRefTypeInfo, MEMBERID_NIL,
                    &bstrData, NULL, NULL, NULL);
            AddToStrW(wszAddTo, bstrData);
            SysFreeString(bstrData);
            ITypeInfo_Release(pRefTypeInfo);
        }
        else AddToStrW(wszAddTo, wszFailed);
        break;
        default:
        WINE_FIXME("tdesc.vt&VT_TYPEMASK == %d not supported\n",
                tdesc.vt&VT_TYPEMASK);
    }
}

int PopulateTree(void)
{
    TVINSERTSTRUCT tvis;
    ITypeLib *pTypeLib;
    ITypeInfo *pTypeInfo;
    TYPEATTR *pTypeAttr;
    INT count, i;
    BSTR bstrName;
    BSTR bstrData;
    WCHAR wszText[MAX_LOAD_STRING];
    HRESULT hRes;

    const WCHAR wszFormat[] = { '%','s',' ','(','%','s',')','\0' };

    const WCHAR wszTKIND_ENUM[] = { 't','y','p','e','d','e','f',' ','e','n','u','m',' ','\0' };
    const WCHAR wszTKIND_RECORD[]
        = { 't','y','p','e','d','e','f',' ','s','t','r','u','c','t',' ','\0' };
    const WCHAR wszTKIND_MODULE[] = { 'm','o','d','u','l','e',' ','\0' };
    const WCHAR wszTKIND_INTERFACE[] = { 'i','n','t','e','r','f','a','c','e',' ','\0' };
    const WCHAR wszTKIND_DISPATCH[]
        = { 'd','i','s','p','i','n','t','e','r','f','a','c','e',' ','\0' };
    const WCHAR wszTKIND_COCLASS[] = { 'c','o','c','l','a','s','s',' ','\0' };
    const WCHAR wszTKIND_ALIAS[] = { 't','y','p','e','d','e','f',' ','\0' };
    const WCHAR wszTKIND_UNION[]
        = { 't','y','p','e','d','e','f',' ','u','n','i','o','n',' ','\0' };

    U(tvis).item.mask = TVIF_TEXT|TVIF_CHILDREN;
    U(tvis).item.cchTextMax = MAX_LOAD_STRING;
    U(tvis).item.pszText = wszText;
    U(tvis).item.cChildren = 1;
    tvis.hInsertAfter = (HTREEITEM)TVI_LAST;
    tvis.hParent = TVI_ROOT;

    if(FAILED((hRes = LoadTypeLib(typelib.wszFileName, &pTypeLib))))
    {
        WCHAR wszMessage[MAX_LOAD_STRING];
        WCHAR wszError[MAX_LOAD_STRING];

        LoadString(globals.hMainInst, IDS_ERROR_LOADTYPELIB,
                wszError, sizeof(WCHAR[MAX_LOAD_STRING]));
        wsprintfW(wszMessage, wszError, typelib.wszFileName, hRes);
        MessageBox(globals.hMainWnd, wszMessage, NULL, MB_OK|MB_ICONEXCLAMATION);
        return 1;
    }
    count = ITypeLib_GetTypeInfoCount(pTypeLib);

    ITypeLib_GetDocumentation(pTypeLib, -1, &bstrName, &bstrData, NULL, NULL);
    wsprintfW(wszText, wszFormat, bstrName, bstrData);
    SysFreeString(bstrName);
    SysFreeString(bstrData);
    tvis.hParent = (HTREEITEM)SendMessage(typelib.hTree,
            TVM_INSERTITEM, 0, (LPARAM)&tvis);

    for(i=0; i<count; i++)
    {
        ITypeLib_GetTypeInfo(pTypeLib, i, &pTypeInfo);

        ITypeInfo_GetDocumentation(pTypeInfo, MEMBERID_NIL, &bstrName, NULL, NULL, NULL);
        ITypeInfo_GetTypeAttr(pTypeInfo, &pTypeAttr);

        memset(wszText, 0, sizeof(wszText));
        switch(pTypeAttr->typekind)
        {
#define TKINDADDTOSTR(x) case x:\
    AddToStrW(wszText, wsz##x);\
    AddToStrW(wszText, bstrName);\
    break
            TKINDADDTOSTR(TKIND_ENUM);
            TKINDADDTOSTR(TKIND_RECORD);
            TKINDADDTOSTR(TKIND_MODULE);
            TKINDADDTOSTR(TKIND_INTERFACE);
            TKINDADDTOSTR(TKIND_DISPATCH);
            TKINDADDTOSTR(TKIND_COCLASS);
            TKINDADDTOSTR(TKIND_UNION);
            case TKIND_ALIAS:
                AddToStrW(wszText, wszTKIND_ALIAS);
                CreateTypeInfo(wszText, pTypeAttr->tdescAlias, pTypeInfo);
                AddToStrW(wszText, wszSpace);
                AddToStrW(wszText, bstrName);
                break;
            default:
                lstrcpyW(wszText, bstrName);
                WINE_FIXME("pTypeAttr->typekind ==  %d\n not supported",
                        pTypeAttr->typekind);
        }
        SendMessage(typelib.hTree, TVM_INSERTITEM, 0, (LPARAM)&tvis);
        ITypeInfo_ReleaseTypeAttr(pTypeInfo, pTypeAttr);
        ITypeInfo_Release(pTypeInfo);
        SysFreeString(bstrName);
    }
    SendMessage(typelib.hTree, TVM_EXPAND, TVE_EXPAND, (LPARAM)tvis.hParent);

    ITypeLib_Release(pTypeLib);
    return 0;
}

void TypeLibResizeChild(void)
{
    RECT client, stat;

    MoveWindow(typelib.hStatusBar, 0, 0, 0, 0, TRUE);

    if(IsWindowVisible(typelib.hStatusBar))
        GetClientRect(typelib.hStatusBar, &stat);
    else stat.bottom = 0;

    GetClientRect(globals.hTypeLibWnd, &client);
    MoveWindow(typelib.hPaneWnd, 0, 0,
            client.right, client.bottom-stat.bottom, TRUE);
}

void TypeLibMenuCommand(WPARAM wParam, HWND hWnd)
{
    BOOL vis;

    switch(wParam)
    {
        case IDM_STATUSBAR:
            vis = IsWindowVisible(typelib.hStatusBar);
            ShowWindow(typelib.hStatusBar, vis ? SW_HIDE : SW_SHOW);
            CheckMenuItem(GetMenu(hWnd), LOWORD(wParam),
                    vis ? MF_UNCHECKED : MF_CHECKED);
            TypeLibResizeChild();
            break;
        case IDM_CLOSE:
            DestroyWindow(hWnd);
            break;
    }
}

void UpdateTypeLibStatusBar(int itemID)
{
    WCHAR info[MAX_LOAD_STRING];

    if(!LoadString(globals.hMainInst, itemID, info, sizeof(WCHAR[MAX_LOAD_STRING])))
        LoadString(globals.hMainInst, IDS_READY, info, sizeof(WCHAR[MAX_LOAD_STRING]));

    SendMessage(typelib.hStatusBar, SB_SETTEXT, 0, (LPARAM)info);
}

LRESULT CALLBACK TypeLibProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch(uMsg)
    {
        case WM_CREATE:
        {
            if(!CreatePanedWindow(hWnd, &typelib.hPaneWnd, globals.hMainInst))
                DestroyWindow(hWnd);
            typelib.hTree = CreateWindowEx(WS_EX_CLIENTEDGE, WC_TREEVIEW, NULL,
                    WS_CHILD|WS_VISIBLE|TVS_HASLINES|TVS_HASBUTTONS|TVS_LINESATROOT,
                    0, 0, 0, 0, typelib.hPaneWnd, NULL, globals.hMainInst, NULL);
            typelib.hEdit = CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, NULL,
                    WS_CHILD|WS_VISIBLE|ES_MULTILINE|ES_READONLY|WS_HSCROLL|WS_VSCROLL,
                    0, 0, 0, 0, typelib.hPaneWnd, NULL, globals.hMainInst, NULL);

            SetLeft(typelib.hPaneWnd, typelib.hTree);
            SetRight(typelib.hPaneWnd, typelib.hEdit);

            if(PopulateTree()) DestroyWindow(hWnd);
            else SetFocus(typelib.hTree);
            break;
        }
        case WM_COMMAND:
            TypeLibMenuCommand(LOWORD(wParam), hWnd);
        case WM_MENUSELECT:
            UpdateTypeLibStatusBar(LOWORD(wParam));
            break;
        case WM_SETFOCUS:
            SetFocus(typelib.hTree);
            break;
        case WM_SIZE:
            if(wParam == SIZE_MINIMIZED) break;
            TypeLibResizeChild();
            break;
        case WM_DESTROY:
            break;
        default:
            return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }
    return 0;
}

BOOL TypeLibRegisterClass(void)
{
    WNDCLASS wcc;

    memset(&wcc, 0, sizeof(WNDCLASS));
    wcc.lpfnWndProc = TypeLibProc;
    wcc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wcc.lpszMenuName = MAKEINTRESOURCE(IDM_TYPELIB);
    wcc.lpszClassName = wszTypeLib;

    if(!RegisterClass(&wcc))
        return FALSE;

    return TRUE;
}

BOOL CreateTypeLibWindow(HINSTANCE hInst, WCHAR *wszFileName)
{
    WCHAR wszTitle[MAX_LOAD_STRING];
    LoadString(hInst, IDS_TYPELIBTITLE, wszTitle, sizeof(WCHAR[MAX_LOAD_STRING]));

    if(wszFileName) lstrcpyW(typelib.wszFileName, wszFileName);
    else
    {
        TVITEM tvi;

        memset(&tvi, 0, sizeof(TVITEM));
        tvi.hItem = TreeView_GetSelection(globals.hTree);

        SendMessage(globals.hTree, TVM_GETITEM, 0, (LPARAM)&tvi);
        lstrcpyW(typelib.wszFileName, ((ITEM_INFO*)tvi.lParam)->path);
    }

    globals.hTypeLibWnd = CreateWindow(wszTypeLib, wszTitle,
            WS_OVERLAPPEDWINDOW|WS_VISIBLE,
            CW_USEDEFAULT, CW_USEDEFAULT, 800, 600, NULL, NULL, hInst, NULL);
    if(!globals.hTypeLibWnd) return FALSE;

    typelib.hStatusBar = CreateStatusWindow(WS_VISIBLE|WS_CHILD,
            (LPWSTR)wszTitle, globals.hTypeLibWnd, 0);

    TypeLibResizeChild();
    return TRUE;
}
