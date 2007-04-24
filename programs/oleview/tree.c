/*
 * OleView (tree.c)
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

TREE tree;
static const WCHAR wszCLSID[] = { 'C','L','S','I','D','\\','\0' };
static const WCHAR wszInProcServer32[] = 
    { 'I','n','P','r','o','c','S','e','r','v','e','r','3','2','\0' };
static const WCHAR wszOle32dll[] = { 'o','l','e','3','2','.','d','l','l','\0' };
static const WCHAR wszOleAut32dll[] =
    { 'o','l','e','a','u','t','3','2','.','d','l','l','\0' };
static const WCHAR wszImplementedCategories[] = 
    { 'I','m','p','l','e','m','e','n','t','e','d',' ',
        'C','a','t','e','g','o','r','i','e','s','\0' };
static const WCHAR wszAppID[] = { 'A','p','p','I','D','\\','\0' };
static const WCHAR wszTypeLib[] = { 'T','y','p','e','L','i','b','\\','\0' };
static const WCHAR wszInterface[] = { 'I','n','t','e','r','f','a','c','e','\\','\0' };
static const WCHAR wszComponentCategories[] = { 'C','o','m','p','o','n','e','n','t',
    ' ','C','a','t','e','g','o','r','i','e','s','\\','\0' };
static const WCHAR wszGetPath[] = { '0','\\','w','i','n','3','2','\0' };

static LPARAM CreateITEM_INFO(INT flag, const WCHAR *info, const WCHAR *clsid, const WCHAR *path)
{
    ITEM_INFO *reg;

    reg = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(ITEM_INFO));

    reg->cFlag = flag;
    lstrcpyW(reg->info, info);
    if(clsid) lstrcpyW(reg->clsid, clsid);
    if(path) lstrcpyW(reg->path, path);

    return (LPARAM)reg;
}

void CreateInst(HTREEITEM item, WCHAR *wszMachineName)
{
    TVITEM tvi;
    HTREEITEM hCur;
    TVINSERTSTRUCT tvis;
    WCHAR wszTitle[MAX_LOAD_STRING];
    WCHAR wszMessage[MAX_LOAD_STRING];
    WCHAR wszFlagName[MAX_LOAD_STRING];
    WCHAR wszTreeName[MAX_LOAD_STRING];
    WCHAR wszRegPath[MAX_LOAD_STRING];
    const WCHAR wszFormat[] = { '\n','%','s',' ','(','$','%','x',')','\n','\0' };
    CLSID clsid;
    COSERVERINFO remoteInfo;
    MULTI_QI qi;
    IUnknown *obj, *unk;
    HRESULT hRes;

    memset(&tvi, 0, sizeof(TVITEM));
    tvi.mask = TVIF_TEXT;
    tvi.hItem = item;
    tvi.cchTextMax = MAX_LOAD_STRING;
    tvi.pszText = wszTreeName;

    memset(&tvis, 0, sizeof(TVINSERTSTRUCT));
    U(tvis).item.mask = TVIF_TEXT|TVIF_PARAM;
    U(tvis).item.cchTextMax = MAX_LOAD_STRING;
    tvis.hInsertAfter = (HTREEITEM)TVI_FIRST;
    U(tvis).item.pszText = tvi.pszText;
    tvis.hParent = item;
    tvis.hInsertAfter = TVI_LAST;

    SendMessage(globals.hTree, TVM_GETITEM, 0, (LPARAM)&tvi);

    if(!tvi.lParam || ((ITEM_INFO *)tvi.lParam)->loaded
                || !(((ITEM_INFO *)tvi.lParam)->cFlag&SHOWALL)) return;

    if(FAILED(CLSIDFromString(((ITEM_INFO *)tvi.lParam)->clsid, &clsid))) return;

    if(wszMachineName)
    {
        remoteInfo.dwReserved1 = 0;
        remoteInfo.dwReserved2 = 0;
        remoteInfo.pAuthInfo = NULL;
        remoteInfo.pwszName = wszMachineName;

        qi.pIID = &IID_IUnknown;

        CoCreateInstanceEx(&clsid, NULL, globals.dwClsCtx|CLSCTX_REMOTE_SERVER,
                &remoteInfo, 1, &qi);
        hRes = qi.hr;
        obj = qi.pItf;
    }
    else hRes = CoCreateInstance(&clsid, NULL, globals.dwClsCtx,
            &IID_IUnknown, (void **)&obj);

    if(FAILED(hRes))
    {
        LoadString(globals.hMainInst, IDS_CGCOFAIL, wszMessage,
                sizeof(WCHAR[MAX_LOAD_STRING]));
        LoadString(globals.hMainInst, IDS_ABOUT, wszTitle,
                sizeof(WCHAR[MAX_LOAD_STRING]));

#define CASE_ERR(i) case i: \
    MultiByteToWideChar(CP_ACP, 0, #i, -1, wszFlagName, MAX_LOAD_STRING); \
    break

        switch(hRes)
        {
            CASE_ERR(REGDB_E_CLASSNOTREG);
            CASE_ERR(E_NOINTERFACE);
            CASE_ERR(REGDB_E_READREGDB);
            CASE_ERR(REGDB_E_KEYMISSING);
            CASE_ERR(CO_E_DLLNOTFOUND);
            CASE_ERR(CO_E_APPNOTFOUND);
            CASE_ERR(E_ACCESSDENIED);
            CASE_ERR(CO_E_ERRORINDLL);
            CASE_ERR(CO_E_APPDIDNTREG);
            CASE_ERR(CLASS_E_CLASSNOTAVAILABLE);
            default:
                LoadString(globals.hMainInst, IDS_ERROR_UNKN, wszFlagName, MAX_LOAD_STRING);
        }

        wsprintfW(&wszMessage[lstrlenW(wszMessage)], wszFormat,
                wszFlagName, (unsigned)hRes);
        MessageBox(globals.hMainWnd, wszMessage, wszTitle, MB_OK|MB_ICONEXCLAMATION);
        return;
    }

    ((ITEM_INFO *)tvi.lParam)->loaded = 1;
    ((ITEM_INFO *)tvi.lParam)->pU = obj;

    tvi.mask = TVIF_STATE;
    tvi.state = TVIS_BOLD;
    tvi.stateMask = TVIS_BOLD;
    SendMessage(globals.hTree, TVM_SETITEM, 0, (LPARAM)&tvi);

    tvi.mask = TVIF_TEXT;
    hCur = TreeView_GetChild(globals.hTree, tree.hI);

    while(hCur)
    {
        tvi.hItem = hCur;
        SendMessage(globals.hTree, TVM_GETITEM, 0, (LPARAM)&tvi);

        if(!tvi.lParam)
        {
            hCur = TreeView_GetNextSibling(globals.hTree, hCur);
            continue;
        }

        CLSIDFromString(((ITEM_INFO *)tvi.lParam)->clsid, &clsid);
        hRes = IUnknown_QueryInterface(obj, &clsid, (void *)&unk);

        if(SUCCEEDED(hRes))
        {
            IUnknown_Release(unk);

            lstrcpyW(wszRegPath, wszInterface);
            lstrcpyW(&wszRegPath[lstrlenW(wszRegPath)], ((ITEM_INFO *)tvi.lParam)->clsid);
            U(tvis).item.lParam = CreateITEM_INFO(REGTOP|INTERFACE|REGPATH,
                    wszRegPath, ((ITEM_INFO *)tvi.lParam)->clsid, NULL);
            SendMessage(globals.hTree, TVM_INSERTITEM, 0, (LPARAM)&tvis);
        }
        hCur = TreeView_GetNextSibling(globals.hTree, hCur);
    }

    RefreshMenu(item);
    RefreshDetails(item);
}

void ReleaseInst(HTREEITEM item)
{
    TVITEM tvi;
    HTREEITEM cur;
    IUnknown *pU;

    memset(&tvi, 0, sizeof(TVITEM));
    tvi.hItem = item;
    SendMessage(globals.hTree, TVM_GETITEM, 0, (LPARAM)&tvi);

    if(!tvi.lParam) return;

    pU = ((ITEM_INFO *)tvi.lParam)->pU;

    if(pU) IUnknown_Release(pU);
    ((ITEM_INFO *)tvi.lParam)->loaded = 0;

    SendMessage(globals.hTree, TVM_EXPAND, TVE_COLLAPSE, (LPARAM)item);

    cur = TreeView_GetChild(globals.hTree, item);
    while(cur)
    {
        SendMessage(globals.hTree, TVM_DELETEITEM, 0, (LPARAM)cur);
        cur = TreeView_GetChild(globals.hTree, item);
    }

    tvi.mask = TVIF_CHILDREN|TVIF_STATE;
    tvi.state = 0;
    tvi.stateMask = TVIS_BOLD;
    tvi.cChildren = 1;
    SendMessage(globals.hTree, TVM_SETITEM, 0, (LPARAM)&tvi);
}

BOOL CreateRegPath(HTREEITEM item, WCHAR *buffer, int bufSize)
{
    TVITEM tvi;
    int bufLen;
    BOOL ret;

    memset(buffer, 0, sizeof(WCHAR[bufSize]));
    memset(&tvi, 0, sizeof(TVITEM));
    tvi.hItem = item;

    SendMessage(globals.hTree, TVM_GETITEM, 0, (LPARAM)&tvi);
    ret = (tvi.lParam && ((ITEM_INFO *)tvi.lParam)->cFlag & REGPATH);

    while(TRUE)
    {
        SendMessage(globals.hTree, TVM_GETITEM, 0, (LPARAM)&tvi);

        if(tvi.lParam && (((ITEM_INFO *)tvi.lParam)->cFlag & (REGPATH|REGTOP)))
        {
            bufLen = lstrlenW(((ITEM_INFO *)tvi.lParam)->info);
            memmove(&buffer[bufLen], buffer, sizeof(WCHAR[bufSize-bufLen]));
            memcpy(buffer, ((ITEM_INFO *)tvi.lParam)->info, sizeof(WCHAR[bufLen]));
        }

        if(tvi.lParam && ((ITEM_INFO *)tvi.lParam)->cFlag & REGTOP) break;

        if(!tvi.lParam) return FALSE;

        tvi.hItem = TreeView_GetParent(globals.hTree, tvi.hItem);
    }
    return ret;
}

static void AddCOMandAll(void)
{
    TVINSERTSTRUCT tvis;
    TVITEM tvi;
    HTREEITEM curSearch;
    HKEY hKey, hCurKey, hInfo;
    WCHAR valName[MAX_LOAD_STRING];
    WCHAR buffer[MAX_LOAD_STRING];
    WCHAR wszComp[MAX_LOAD_STRING];
    LONG lenBuffer;
    int i=-1;

    memset(&tvi, 0, sizeof(TVITEM));
    U(tvis).item.mask = TVIF_TEXT|TVIF_PARAM|TVIF_CHILDREN;
    U(tvis).item.cchTextMax = MAX_LOAD_STRING;
    U(tvis).item.cChildren = 1;
    tvis.hInsertAfter = (HTREEITEM)TVI_FIRST;

    if(RegOpenKey(HKEY_CLASSES_ROOT, wszCLSID, &hKey) != ERROR_SUCCESS) return;

    while(TRUE)
    {
        i++;

        if(RegEnumKey(hKey, i, valName, -1) != ERROR_SUCCESS) break;

        if(RegOpenKey(hKey, valName, &hCurKey) != ERROR_SUCCESS) continue;

        lenBuffer = sizeof(WCHAR[MAX_LOAD_STRING]);
        tvis.hParent = tree.hAO;

        if(RegOpenKey(hCurKey, wszInProcServer32, &hInfo) == ERROR_SUCCESS)
        {
            if(RegQueryValue(hInfo, NULL, buffer, &lenBuffer) == ERROR_SUCCESS
                    && *buffer)
                if(!memcmp(buffer, wszOle32dll, sizeof(WCHAR[9]))
                        ||!memcmp(buffer, wszOleAut32dll, sizeof(WCHAR[12])))
                    tvis.hParent = tree.hCLO;

            RegCloseKey(hInfo);
        }

        lenBuffer = sizeof(WCHAR[MAX_LOAD_STRING]);

        if(RegQueryValue(hCurKey, NULL, buffer, &lenBuffer) == ERROR_SUCCESS && *buffer)
            U(tvis).item.pszText = buffer;
        else U(tvis).item.pszText = valName;
    
        U(tvis).item.lParam = CreateITEM_INFO(REGPATH|SHOWALL, valName, valName, NULL);
        if(tvis.hParent) SendMessage(globals.hTree, TVM_INSERTITEM, 0, (LPARAM)&tvis);

        if(RegOpenKey(hCurKey, wszImplementedCategories, &hInfo) == ERROR_SUCCESS)
        {
            if(RegEnumKey(hInfo, 0, wszComp, -1) != ERROR_SUCCESS) break;

            RegCloseKey(hInfo);

            if(tree.hGBCC) curSearch = TreeView_GetChild(globals.hTree, tree.hGBCC);
            else curSearch = TreeView_GetChild(globals.hTree, TVI_ROOT);

            while(curSearch)
            {
                tvi.hItem = curSearch;
                SendMessage(globals.hTree, TVM_GETITEM, 0, (LPARAM)&tvi);

                if(tvi.lParam && !lstrcmpW(((ITEM_INFO *)tvi.lParam)->info, wszComp))
                {
                    tvis.hParent = curSearch;

                    memmove(&valName[6], valName, sizeof(WCHAR[MAX_LOAD_STRING-6]));
                    memmove(valName, wszCLSID, sizeof(WCHAR[6]));
                    U(tvis).item.lParam = CreateITEM_INFO(REGTOP|REGPATH|SHOWALL,
                            valName, &valName[6], NULL);

                    SendMessage(globals.hTree, TVM_INSERTITEM, 0, (LPARAM)&tvis);
                    break;
                }
                curSearch = TreeView_GetNextSibling(globals.hTree, curSearch);
            }
        }
        RegCloseKey(hCurKey);
    }
    RegCloseKey(hKey);

    SendMessage(globals.hTree, TVM_SORTCHILDREN, FALSE, (LPARAM)tree.hCLO);
    SendMessage(globals.hTree, TVM_SORTCHILDREN, FALSE, (LPARAM)tree.hAO);
}

static void AddApplicationID(void)
{
    TVINSERTSTRUCT tvis;
    HKEY hKey, hCurKey;
    WCHAR valName[MAX_LOAD_STRING];
    WCHAR buffer[MAX_LOAD_STRING];
    LONG lenBuffer;
    int i=-1;

    U(tvis).item.mask = TVIF_TEXT|TVIF_PARAM;
    U(tvis).item.cchTextMax = MAX_LOAD_STRING;
    tvis.hInsertAfter = (HTREEITEM)TVI_FIRST;
    tvis.hParent = tree.hAID;

    if(RegOpenKey(HKEY_CLASSES_ROOT, wszAppID, &hKey) != ERROR_SUCCESS) return;

    while(TRUE)
    {
        i++;

        if(RegEnumKey(hKey, i, valName, -1) != ERROR_SUCCESS) break;

        if(RegOpenKey(hKey, valName, &hCurKey) != ERROR_SUCCESS) continue;

        lenBuffer = sizeof(WCHAR[MAX_LOAD_STRING]);

        if(RegQueryValue(hCurKey, NULL, buffer, &lenBuffer) == ERROR_SUCCESS && *buffer)
            U(tvis).item.pszText = buffer;
        else U(tvis).item.pszText = valName;

        RegCloseKey(hCurKey);

        U(tvis).item.lParam = CreateITEM_INFO(REGPATH, valName, valName, NULL);
        SendMessage(globals.hTree, TVM_INSERTITEM, 0, (LPARAM)&tvis);
    }
    RegCloseKey(hKey);

    SendMessage(globals.hTree, TVM_SORTCHILDREN, FALSE, (LPARAM)tree.hAID);
}

static void AddTypeLib(void)
{
    TVINSERTSTRUCT tvis;
    HKEY hKey, hCurKey, hInfoKey, hPath;
    WCHAR valName[MAX_LOAD_STRING];
    WCHAR valParent[MAX_LOAD_STRING];
    WCHAR buffer[MAX_LOAD_STRING];
    WCHAR wszVer[MAX_LOAD_STRING];
    WCHAR wszPath[MAX_LOAD_STRING];
    const WCHAR wszFormat[] = { ' ','(','%','s',' ','%','s',')','\0' };
    const WCHAR wszFormat2[] = { '%','s','\\','%','s','\0' };
    LONG lenBuffer;
    int i=-1, j;

    U(tvis).item.mask = TVIF_TEXT|TVIF_PARAM;
    U(tvis).item.cchTextMax = MAX_LOAD_STRING;
    tvis.hInsertAfter = (HTREEITEM)TVI_FIRST;
    tvis.hParent = tree.hTL;

    if(RegOpenKey(HKEY_CLASSES_ROOT, wszTypeLib, &hKey) != ERROR_SUCCESS) return;

    while(TRUE)
    {
        i++;

        if(RegEnumKey(hKey, i, valParent, -1) != ERROR_SUCCESS) break;

        if(RegOpenKey(hKey, valParent, &hCurKey) != ERROR_SUCCESS) continue;

        j = -1;
        while(TRUE)
        {
            j++;

            if(RegEnumKey(hCurKey, j, valName, -1) != ERROR_SUCCESS) break;

            if(RegOpenKey(hCurKey, valName, &hInfoKey) != ERROR_SUCCESS) continue;

            lenBuffer = sizeof(WCHAR[MAX_LOAD_STRING]);

            if(RegQueryValue(hInfoKey, NULL, buffer, &lenBuffer) == ERROR_SUCCESS 
                    && *buffer)
            {
                LoadString(globals.hMainInst, IDS_TL_VER, wszVer,
                        sizeof(WCHAR[MAX_LOAD_STRING]));

                wsprintfW(&buffer[lstrlenW(buffer)], wszFormat, wszVer, valName);
                U(tvis).item.pszText = buffer;

                lenBuffer = MAX_LOAD_STRING;
                RegOpenKey(hInfoKey, wszGetPath, &hPath);
                RegQueryValue(hPath, NULL, wszPath, &lenBuffer);
                RegCloseKey(hPath);
            }
            else U(tvis).item.pszText = valName;

            RegCloseKey(hInfoKey);

            wsprintfW(wszVer, wszFormat2, valParent, valName);
            U(tvis).item.lParam = CreateITEM_INFO(REGPATH, wszVer, valParent, wszPath);

            SendMessage(globals.hTree, TVM_INSERTITEM, 0, (LPARAM)&tvis);
        }
        RegCloseKey(hCurKey);
    }

    RegCloseKey(hKey);

    SendMessage(globals.hTree, TVM_SORTCHILDREN, FALSE, (LPARAM)tree.hTL);
}

static void AddInterfaces(void)
{
    TVINSERTSTRUCT tvis;
    HKEY hKey, hCurKey;
    WCHAR valName[MAX_LOAD_STRING];
    WCHAR buffer[MAX_LOAD_STRING];
    LONG lenBuffer;
    int i=-1;

    U(tvis).item.mask = TVIF_TEXT|TVIF_PARAM;
    U(tvis).item.cchTextMax = MAX_LOAD_STRING;
    tvis.hInsertAfter = (HTREEITEM)TVI_FIRST;
    tvis.hParent = tree.hI;

    if(RegOpenKey(HKEY_CLASSES_ROOT, wszInterface, &hKey) != ERROR_SUCCESS) return;

    while(TRUE)
    {
        i++;

        if(RegEnumKey(hKey, i, valName, -1) != ERROR_SUCCESS) break;

        if(RegOpenKey(hKey, valName, &hCurKey) != ERROR_SUCCESS) continue;

        lenBuffer = sizeof(WCHAR[MAX_LOAD_STRING]);

        if(RegQueryValue(hCurKey, NULL, buffer, &lenBuffer) == ERROR_SUCCESS && *buffer)
            U(tvis).item.pszText = buffer;
        else U(tvis).item.pszText = valName;

        RegCloseKey(hCurKey);

        U(tvis).item.lParam = CreateITEM_INFO(REGPATH|INTERFACE, valName, valName, NULL);
        SendMessage(globals.hTree, TVM_INSERTITEM, 0, (LPARAM)&tvis);
    }

    RegCloseKey(hKey);

    SendMessage(globals.hTree, TVM_SORTCHILDREN, FALSE, (LPARAM)tree.hI);
}

static void AddComponentCategories(void)
{
    TVINSERTSTRUCT tvis;
    HKEY hKey, hCurKey;
    WCHAR valName[MAX_LOAD_STRING];
    WCHAR buffer[MAX_LOAD_STRING];
    LONG lenBuffer;
    DWORD lenBufferHlp;
    int i=-1;

    U(tvis).item.mask = TVIF_TEXT|TVIF_PARAM|TVIF_CHILDREN;
    U(tvis).item.cchTextMax = MAX_LOAD_STRING;
    tvis.hInsertAfter = (HTREEITEM)TVI_FIRST;
    if(tree.hGBCC) tvis.hParent = tree.hGBCC;
    else tvis.hParent = TVI_ROOT;
    U(tvis).item.cChildren = 1;

    if(RegOpenKey(HKEY_CLASSES_ROOT, wszComponentCategories, &hKey) != ERROR_SUCCESS)
        return;

    while(TRUE)
    {
        i++;

        if(RegEnumKey(hKey, i, valName, -1) != ERROR_SUCCESS) break;

        if(RegOpenKey(hKey, valName, &hCurKey) != ERROR_SUCCESS) continue;

        lenBuffer = sizeof(WCHAR[MAX_LOAD_STRING]);
        lenBufferHlp = sizeof(WCHAR[MAX_LOAD_STRING]);

        if(RegQueryValue(hCurKey, NULL, buffer, &lenBuffer) == ERROR_SUCCESS && *buffer)
            U(tvis).item.pszText = buffer;
        else if(RegEnumValue(hCurKey, 0, NULL, NULL, NULL, NULL,
                    (LPBYTE)buffer, &lenBufferHlp) == ERROR_SUCCESS && *buffer)
            U(tvis).item.pszText = buffer;
        else continue;

        RegCloseKey(hCurKey);

        U(tvis).item.lParam = CreateITEM_INFO(REGTOP, valName, valName, NULL);
        SendMessage(globals.hTree, TVM_INSERTITEM, 0, (LPARAM)&tvis);
    }

    RegCloseKey(hKey);

    SendMessage(globals.hTree, TVM_SORTCHILDREN, FALSE, (LPARAM)tree.hGBCC);
}

static void AddBaseEntries(void)
{
    TVINSERTSTRUCT tvis;
    WCHAR name[MAX_LOAD_STRING];

    U(tvis).item.mask = TVIF_TEXT|TVIF_CHILDREN|TVIF_PARAM;
    /* FIXME add TVIF_IMAGE */
    U(tvis).item.pszText = name;
    U(tvis).item.cchTextMax = MAX_LOAD_STRING;
    U(tvis).item.cChildren = 1;
    tvis.hInsertAfter = (HTREEITEM)TVI_FIRST;
    tvis.hParent = TVI_ROOT;

    LoadString(globals.hMainInst, IDS_TREE_I, U(tvis).item.pszText,
            sizeof(WCHAR[MAX_LOAD_STRING]));
    U(tvis).item.lParam = CreateITEM_INFO(REGTOP, wszInterface, NULL, NULL);
    tree.hI = TreeView_InsertItem(globals.hTree, &tvis);

    LoadString(globals.hMainInst, IDS_TREE_TL, U(tvis).item.pszText,
            sizeof(WCHAR[MAX_LOAD_STRING]));
    U(tvis).item.lParam = CreateITEM_INFO(REGTOP, wszTypeLib, NULL, NULL);
    tree.hTL = TreeView_InsertItem(globals.hTree, &tvis);

    LoadString(globals.hMainInst, IDS_TREE_AID, U(tvis).item.pszText,
            sizeof(WCHAR[MAX_LOAD_STRING]));
    U(tvis).item.lParam = CreateITEM_INFO(REGTOP|REGPATH, wszAppID, NULL, NULL);
    tree.hAID = TreeView_InsertItem(globals.hTree, &tvis);

    LoadString(globals.hMainInst, IDS_TREE_OC, U(tvis).item.pszText,
            sizeof(WCHAR[MAX_LOAD_STRING]));
    U(tvis).item.lParam = (LPARAM)NULL;
    tree.hOC = TreeView_InsertItem(globals.hTree, &tvis);


    tvis.hParent = tree.hOC;
    LoadString(globals.hMainInst, IDS_TREE_AO, U(tvis).item.pszText,
            sizeof(WCHAR[MAX_LOAD_STRING]));
    U(tvis).item.lParam = CreateITEM_INFO(REGTOP, wszCLSID, NULL, NULL);
    tree.hAO = TreeView_InsertItem(globals.hTree, &tvis);

    LoadString(globals.hMainInst, IDS_TREE_CLO, U(tvis).item.pszText,
            sizeof(WCHAR[MAX_LOAD_STRING]));
    tree.hCLO = TreeView_InsertItem(globals.hTree, &tvis);

    LoadString(globals.hMainInst, IDS_TREE_O1O, U(tvis).item.pszText,
            sizeof(WCHAR[MAX_LOAD_STRING]));
    U(tvis).item.lParam = (LPARAM)NULL;
    tree.hO1O = TreeView_InsertItem(globals.hTree, &tvis);

    LoadString(globals.hMainInst, IDS_TREE_GBCC, U(tvis).item.pszText,
            sizeof(WCHAR[MAX_LOAD_STRING]));
    U(tvis).item.lParam = CreateITEM_INFO(REGTOP|REGPATH,
            wszComponentCategories, NULL, NULL);
    tree.hGBCC = TreeView_InsertItem(globals.hTree, &tvis);

    SendMessage(globals.hTree, TVM_EXPAND, TVE_EXPAND, (LPARAM)tree.hOC);
}

void EmptyTree(void)
{
    HTREEITEM cur, del;
    TVITEM tvi;

    tvi.mask = TVIF_PARAM;
    cur = TreeView_GetChild(globals.hTree, TVI_ROOT);

    while(TRUE)
    {
        del = cur;
        cur = TreeView_GetChild(globals.hTree, del);

        if(!cur) cur = TreeView_GetNextSibling(globals.hTree, del);
        if(!cur)
        {
            cur = TreeView_GetPrevSibling(globals.hTree, del);
            if(!cur) cur = TreeView_GetParent(globals.hTree, del);

            tvi.hItem = del;
            SendMessage(globals.hTree, TVM_GETITEM, 0, (LPARAM)&tvi);

            if(tvi.lParam)
            {
                if(((ITEM_INFO *)tvi.lParam)->loaded) ReleaseInst(del);
                HeapFree(GetProcessHeap(), 0, (ITEM_INFO *)tvi.lParam);
            }

            SendMessage(globals.hTree, TVM_DELETEITEM, 0, (LPARAM)del);

            if(!cur) break;
        }
    }
}

void AddTreeEx(void)
{
    AddBaseEntries();
    AddComponentCategories();
    AddCOMandAll();
    AddApplicationID();
    AddTypeLib();
    AddInterfaces();
}

void AddTree(void)
{
    memset(&tree, 0, sizeof(TREE));
    AddComponentCategories();
    AddCOMandAll();
}

LRESULT CALLBACK TreeProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch(uMsg)
    {
        case WM_CREATE:
            globals.hTree = CreateWindow(WC_TREEVIEW, NULL,
                    WS_CHILD|WS_VISIBLE|TVS_HASLINES|TVS_HASBUTTONS|TVS_LINESATROOT,
                    0, 0, 0, 0, hWnd, (HMENU)TREE_WINDOW, globals.hMainInst, NULL);
            AddTreeEx();
            break;
        case WM_NOTIFY:
            if((int)wParam != TREE_WINDOW) break;
            switch(((LPNMHDR)lParam)->code)
            {
                case TVN_ITEMEXPANDING:
                    CreateInst(((NMTREEVIEW *)lParam)->itemNew.hItem, NULL);
                    break;
                case TVN_SELCHANGED:
                    RefreshMenu(((NMTREEVIEW *)lParam)->itemNew.hItem);
                    RefreshDetails(((NMTREEVIEW *)lParam)->itemNew.hItem);
                    break;
            }
            break;
        case WM_SIZE:
            MoveWindow(globals.hTree, 0, 0, LOWORD(lParam), HIWORD(lParam), TRUE);
            break;
        default:
            return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }
    return 0;
}

HWND CreateTreeWindow(HINSTANCE hInst)
{
    WNDCLASS wct;
    const WCHAR wszTreeClass[] = { 'T','R','E','E','\0' };

    memset(&wct, 0, sizeof(WNDCLASS));
    wct.lpfnWndProc = TreeProc;
    wct.lpszClassName = wszTreeClass;

    if(!RegisterClass(&wct)) return NULL;

    return CreateWindowEx(WS_EX_CLIENTEDGE, wszTreeClass, NULL, WS_CHILD|WS_VISIBLE,
            0, 0, 0, 0, globals.hPaneWnd, NULL, hInst, NULL);
}
