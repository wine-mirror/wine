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
static const WCHAR wszComa[] = { ',','\0' };
static const WCHAR wszSemicolon[] = { ';','\0' };
static const WCHAR wszNewLine[] = { '\n','\0' };
static const WCHAR wszOpenBrackets1[] = { '[','\0' };
static const WCHAR wszCloseBrackets1[] = { ']','\0' };
static const WCHAR wszOpenBrackets2[] = { '(','\0' };
static const WCHAR wszCloseBrackets2[] = { ')','\0' };
static const WCHAR wszOpenBrackets3[] = { '{','\0' };
static const WCHAR wszCloseBrackets3[] = { '}','\0' };
static const WCHAR wszInvertedComa[] = { '"','\0' };
static const WCHAR wszColon[] = { ':','\0' };

static const WCHAR wszUUID[] = { 'u','u','i','d','\0' };
static const WCHAR wszOdl[] = { 'o','d','l','\0' };

static const WCHAR wszVT_BOOL[]
    = { 'V','A','R','I','A','N','T','_','B','O','O','L','\0' };
static const WCHAR wszVT_UI1[]
    = { 'u','n','s','i','g','n','e','d',' ','c','h','a','r','\0' };
static const WCHAR wszVT_UI2[]
    = { 'u','n','s','i','g','n','e','d',' ','s','h','o','r','t','\0' };
static const WCHAR wszVT_UI4[]
    = { 'u','n','s','i','g','n','e','d',' ','l','o','n','g','\0' };
static const WCHAR wszVT_UI8[] = { 'u','i','n','t','6','4','\0' };
static const WCHAR wszVT_UINT[]
    = { 'u','n','s','i','g','n','e','d',' ','i','n','t','\0' };
static const WCHAR wszVT_I1[] = { 'c','h','a','r','\0' };
static const WCHAR wszVT_I2[] = { 's','h','o','r','t','\0' };
static const WCHAR wszVT_I4[] = { 'l','o','n','g','\0' };
static const WCHAR wszVT_I8[] = { 'i','n','t','6','4','\0' };
static const WCHAR wszVT_R4[] = { 's','i','n','g','l','e','\0' };
static const WCHAR wszVT_INT[] = { 'i','n','t','\0' };
static const WCHAR wszVT_BSTR[] = { 'B','S','T','R','\0' };
static const WCHAR wszVT_CY[] = { 'C','U','R','R','E','N','C','Y','\0' };
static const WCHAR wszVT_VARIANT[] = { 'V','A','R','I','A','N','T','\0' };
static const WCHAR wszVT_VOID[] = { 'v','o','i','d','\0' };
static const WCHAR wszVT_ERROR[] = { 'S','C','O','D','E','\0' };
static const WCHAR wszVT_LPSTR[] = { 'L','P','S','T','R','\0' };
static const WCHAR wszVT_LPWSTR[] = { 'L','P','W','S','T','R','\0' };

void AddToStrW(WCHAR *wszDest, const WCHAR *wszSource)
{
    lstrcpyW(&wszDest[lstrlenW(wszDest)], wszSource);
}

void AddToTLDataStrW(TYPELIB_DATA *pTLData, const WCHAR *wszSource)
{
    int SourceLen = lstrlenW(wszSource);

    pTLData->idl = HeapReAlloc(GetProcessHeap(), 0, pTLData->idl,
            sizeof(WCHAR)*(pTLData->idlLen+SourceLen+1));
    
    memcpy(&pTLData->idl[pTLData->idlLen], wszSource, sizeof(WCHAR)*(SourceLen+1));
    pTLData->idlLen += SourceLen;
}

LPARAM InitializeTLData(void)
{
    TYPELIB_DATA *pTLData;

    pTLData = HeapAlloc(GetProcessHeap(), 0, sizeof(TYPELIB_DATA));

    memset(pTLData, 0, sizeof(TYPELIB_DATA));
    pTLData->idl = HeapAlloc(GetProcessHeap(), 0, sizeof(WCHAR));
    pTLData->idl[0] = '\0';

    return (LPARAM)pTLData;
}

void AddChildrenData(HTREEITEM hParent, TYPELIB_DATA *pData)
{
    HTREEITEM hCur;
    TVITEM tvi;

    memset(&tvi, 0, sizeof(&tvi));

    hCur = TreeView_GetChild(typelib.hTree, hParent);
    if(!hCur) return;

    do
    {
        tvi.hItem = hCur;
        SendMessage(typelib.hTree, TVM_GETITEM, 0, (LPARAM)&tvi);
        if(tvi.lParam && ((TYPELIB_DATA *)(tvi.lParam))->idlLen)
            AddToTLDataStrW(pData, ((TYPELIB_DATA *)(tvi.lParam))->idl);
    }while((hCur = TreeView_GetNextSibling(typelib.hTree, hCur)));

    AddToTLDataStrW(pData, pData->wszInsertAfter);
}

void CreateTypeInfo(WCHAR *wszAddTo, WCHAR *wszAddAfter, TYPEDESC tdesc, ITypeInfo *pTypeInfo)
{
    int i;
    BSTR bstrData;
    HRESULT hRes;
    ITypeInfo *pRefTypeInfo;
    WCHAR wszBuf[MAX_LOAD_STRING];
    WCHAR wszFormat[] = { '[','%','l','u',']','\0' };

    switch(tdesc.vt&VT_TYPEMASK)
    {
#define VTADDTOSTR(x) case x:\
        AddToStrW(wszAddTo, wsz##x);\
        break
        VTADDTOSTR(VT_BOOL);
        VTADDTOSTR(VT_UI1);
        VTADDTOSTR(VT_UI2);
        VTADDTOSTR(VT_UI4);
        VTADDTOSTR(VT_UI8);
        VTADDTOSTR(VT_UINT);
        VTADDTOSTR(VT_I1);
        VTADDTOSTR(VT_I2);
        VTADDTOSTR(VT_I4);
        VTADDTOSTR(VT_I8);
        VTADDTOSTR(VT_R4);
        VTADDTOSTR(VT_INT);
        VTADDTOSTR(VT_BSTR);
        VTADDTOSTR(VT_CY);
        VTADDTOSTR(VT_VARIANT);
        VTADDTOSTR(VT_VOID);
        VTADDTOSTR(VT_ERROR);
        VTADDTOSTR(VT_LPSTR);
        VTADDTOSTR(VT_LPWSTR);
        case VT_CARRAY:
        for(i=0; i<U(tdesc).lpadesc->cDims; i++)
        {
            wsprintfW(wszBuf, wszFormat, U(tdesc).lpadesc->rgbounds[i].cElements);
            AddToStrW(wszAddAfter, wszBuf);
        }
        CreateTypeInfo(wszAddTo, wszAddAfter, U(tdesc).lpadesc->tdescElem, pTypeInfo);
        break;
        case VT_PTR:
        CreateTypeInfo(wszAddTo, wszAddAfter, *U(tdesc).lptdesc, pTypeInfo);
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

int EnumVars(ITypeInfo *pTypeInfo, int cVars, HTREEITEM hParent)
{
    int i;
    TVINSERTSTRUCT tvis;
    VARDESC *pVarDesc;
    BSTR bstrName;
    WCHAR wszText[MAX_LOAD_STRING];
    WCHAR wszAfter[MAX_LOAD_STRING];

    U(tvis).item.mask = TVIF_TEXT|TVIF_PARAM;
    U(tvis).item.cchTextMax = MAX_LOAD_STRING;
    U(tvis).item.pszText = wszText;
    tvis.hInsertAfter = (HTREEITEM)TVI_LAST;
    tvis.hParent = hParent;

    for(i=0; i<cVars; i++)
    {
        if(FAILED(ITypeInfo_GetVarDesc(pTypeInfo, i, &pVarDesc))) continue;
        if(FAILED(ITypeInfo_GetDocumentation(pTypeInfo, pVarDesc->memid, &bstrName,
                NULL, NULL, NULL))) continue;

        memset(wszText, 0, sizeof(wszText));
        memset(wszAfter, 0, sizeof(wszAfter));
        CreateTypeInfo(wszText, wszAfter, pVarDesc->elemdescVar.tdesc, pTypeInfo);
        AddToStrW(wszText, wszSpace);
        AddToStrW(wszText, bstrName);
        AddToStrW(wszText, wszAfter);
        U(tvis).item.lParam = InitializeTLData();
        AddToTLDataStrW((TYPELIB_DATA*)(U(tvis).item.lParam), wszText);
        AddToTLDataStrW((TYPELIB_DATA*)(U(tvis).item.lParam), wszSemicolon);
        AddToTLDataStrW((TYPELIB_DATA*)(U(tvis).item.lParam), wszNewLine);

        SendMessage(typelib.hTree, TVM_INSERTITEM, 0, (LPARAM)&tvis);
        SysFreeString(bstrName);
        ITypeInfo_ReleaseVarDesc(pTypeInfo, pVarDesc);
    }

    return 0;
}

int EnumFuncs(ITypeInfo *pTypeInfo, int cFuncs, HTREEITEM hParent)
{
    int i;
    TVINSERTSTRUCT tvis;
    FUNCDESC *pFuncDesc;
    BSTR bstrName;

    U(tvis).item.mask = TVIF_TEXT;
    tvis.hInsertAfter = (HTREEITEM)TVI_LAST;
    tvis.hParent = hParent;

    for(i=0; i<cFuncs; i++)
    {
        if(FAILED(ITypeInfo_GetFuncDesc(pTypeInfo, i, &pFuncDesc))) continue;
        if(FAILED(ITypeInfo_GetDocumentation(pTypeInfo, pFuncDesc->memid, &bstrName,
                NULL, NULL, NULL))) continue;

        U(tvis).item.cchTextMax = SysStringLen(bstrName);
        U(tvis).item.pszText = bstrName;

        SendMessage(typelib.hTree, TVM_INSERTITEM, 0, (LPARAM)&tvis);
        SysFreeString(bstrName);
        ITypeInfo_ReleaseFuncDesc(pTypeInfo, pFuncDesc);
    }

    return 0;
}

int EnumImplTypes(ITypeInfo *pTypeInfo, int cImplTypes, HTREEITEM hParent)
{
    int i;
    TVINSERTSTRUCT tvis;
    ITypeInfo *pRefTypeInfo;
    HREFTYPE hRefType;
    TYPEATTR *pTypeAttr;
    BSTR bstrName;
    WCHAR wszInheritedInterfaces[MAX_LOAD_STRING];

    if(!cImplTypes) return 0;

    LoadString(globals.hMainInst, IDS_INHERITINTERFACES, wszInheritedInterfaces,
            sizeof(WCHAR[MAX_LOAD_STRING]));

    U(tvis).item.mask = TVIF_TEXT;
    U(tvis).item.cchTextMax = MAX_LOAD_STRING;
    U(tvis).item.pszText = wszInheritedInterfaces;
    tvis.hInsertAfter = (HTREEITEM)TVI_LAST;
    tvis.hParent = hParent;

    tvis.hParent = TreeView_InsertItem(typelib.hTree, &tvis);

    for(i=0; i<cImplTypes; i++)
    {
        if(FAILED(ITypeInfo_GetRefTypeOfImplType(pTypeInfo, i, &hRefType))) continue;
        if(FAILED(ITypeInfo_GetRefTypeInfo(pTypeInfo, hRefType, &pRefTypeInfo)))
            continue;
        if(FAILED(ITypeInfo_GetDocumentation(pRefTypeInfo, MEMBERID_NIL, &bstrName,
                NULL, NULL, NULL))) continue;
        if(FAILED(ITypeInfo_GetTypeAttr(pRefTypeInfo, &pTypeAttr))) continue;

        U(tvis).item.cchTextMax = SysStringLen(bstrName);
        U(tvis).item.pszText = bstrName;

        hParent = TreeView_InsertItem(typelib.hTree, &tvis);
        EnumVars(pRefTypeInfo, pTypeAttr->cVars, hParent);
        EnumFuncs(pRefTypeInfo, pTypeAttr->cFuncs, hParent);
        EnumImplTypes(pRefTypeInfo, pTypeAttr->cImplTypes, hParent);

        SysFreeString(bstrName);
        ITypeInfo_ReleaseTypeAttr(pRefTypeInfo, pTypeAttr);
        ITypeInfo_Release(pRefTypeInfo);
    }

    return 0;
}

void CreateInterfaceInfo(ITypeInfo *pTypeInfo, int cImplTypes, WCHAR *wszName,
        WCHAR *wszHelpString, TYPEATTR *pTypeAttr, TYPELIB_DATA *pTLData)
{
    ITypeInfo *pRefTypeInfo;
    HREFTYPE hRefType;
    BSTR bstrName;
    WCHAR wszGuid[MAX_LOAD_STRING];
    BOOL bFirst;

    const WCHAR wszInterface[] = { 'I','n','t','e','r','f','a','c','e',' ','\0' };
    const WCHAR wszHelpstring[] = { 'h','e','l','p','s','t','r','i','n','g','\0' };
    const WCHAR wszTYPEFLAG_FAPPOBJECT[] = { 'a','p','p','o','b','j','e','c','t','\0' };
    const WCHAR wszTYPEFLAG_FCANCREATE[] = { 'c','a','n','c','r','e','a','t','e','\0' };
    const WCHAR wszTYPEFLAG_FLICENSED[] = { 'l','i','c','e','n','s','e','d','\0' };
    const WCHAR wszTYPEFLAG_FPREDECLID[] = { 'p','r','e','d','e','c','l','i','d','\0' };
    const WCHAR wszTYPEFLAG_FHIDDEN[] = { 'h','i','d','d','e','n','\0' };
    const WCHAR wszTYPEFLAG_FCONTROL[] = { 'c','o','n','t','r','o','l','\0' };
    const WCHAR wszTYPEFLAG_FDUAL[] = { 'd','u','a','l','\0' };
    const WCHAR wszTYPEFLAG_FNONEXTENSIBLE[]
        = { 'n','o','n','e','x','t','e','n','s','i','b','l','e','\0' };
    const WCHAR wszTYPEFLAG_FOLEAUTOMATION[]
        = { 'o','l','e','a','u','t','o','m','a','t','i','o','n','\0' };
    const WCHAR wszTYPEFLAG_FRESTRICTED[]
        = { 'r','e','s','t','r','i','c','t','e','d','\0' };
    const WCHAR wszTYPEFLAG_FAGGREGATABLE[]
        = { 'a','g','g','r','e','g','a','t','a','b','l','e','\0' };
    const WCHAR wszTYPEFLAG_FREPLACEABLE[]
        = { 'r','e','p','l','a','c','a','b','l','e','\0' };
    const WCHAR wszTYPEFLAG_FDISPATCHABLE[]
        = { 'd','i','s','p','a','t','c','h','a','b','l','e','\0' };
    const WCHAR wszTYPEFLAG_FREVERSEBIND[]
        = { 'r','e','v','e','r','s','e','b','i','n','d','\0' };
    const WCHAR wszTYPEFLAG_FPROXY[] = { 'p','r','o','x','y','\0' };

    AddToTLDataStrW(pTLData, wszOpenBrackets1);
    AddToTLDataStrW(pTLData, wszNewLine);
    AddToTLDataStrW(pTLData, wszOdl);
    AddToTLDataStrW(pTLData, wszComa);
    AddToTLDataStrW(pTLData, wszNewLine);
    AddToTLDataStrW(pTLData, wszUUID);
    AddToTLDataStrW(pTLData, wszOpenBrackets2);
    StringFromGUID2(&(pTypeAttr->guid), wszGuid, MAX_LOAD_STRING);
    wszGuid[lstrlenW(wszGuid)-1] = '\0';
    AddToTLDataStrW(pTLData, &wszGuid[1]);
    AddToTLDataStrW(pTLData, wszCloseBrackets2);
    if(wszHelpString)
    {
        AddToTLDataStrW(pTLData, wszComa);
        AddToTLDataStrW(pTLData, wszNewLine);
        AddToTLDataStrW(pTLData, wszHelpstring);
        AddToTLDataStrW(pTLData, wszOpenBrackets2);
        AddToTLDataStrW(pTLData, wszInvertedComa);
        AddToTLDataStrW(pTLData, wszHelpString);
        AddToTLDataStrW(pTLData, wszInvertedComa);
        AddToTLDataStrW(pTLData, wszCloseBrackets2);
    }
    if(pTypeAttr->wTypeFlags)
    {
        bFirst = TRUE;
        AddToTLDataStrW(pTLData, wszComa);
        AddToTLDataStrW(pTLData, wszNewLine);
#define ENUM_FLAGS(x) if(pTypeAttr->wTypeFlags & x)\
        {\
            if(!bFirst)\
            {\
                AddToTLDataStrW(pTLData, wszComa);\
                AddToTLDataStrW(pTLData, wszSpace);\
            }\
            bFirst = FALSE;\
            AddToTLDataStrW(pTLData, wsz##x);\
        }
        ENUM_FLAGS(TYPEFLAG_FAPPOBJECT);
        ENUM_FLAGS(TYPEFLAG_FCANCREATE);
        ENUM_FLAGS(TYPEFLAG_FLICENSED);
        ENUM_FLAGS(TYPEFLAG_FPREDECLID);
        ENUM_FLAGS(TYPEFLAG_FHIDDEN);
        ENUM_FLAGS(TYPEFLAG_FCONTROL);
        ENUM_FLAGS(TYPEFLAG_FDUAL);
        ENUM_FLAGS(TYPEFLAG_FNONEXTENSIBLE);
        ENUM_FLAGS(TYPEFLAG_FOLEAUTOMATION);
        ENUM_FLAGS(TYPEFLAG_FRESTRICTED);
        ENUM_FLAGS(TYPEFLAG_FAGGREGATABLE);
        ENUM_FLAGS(TYPEFLAG_FREPLACEABLE);
        ENUM_FLAGS(TYPEFLAG_FDISPATCHABLE);
        ENUM_FLAGS(TYPEFLAG_FREVERSEBIND);
        ENUM_FLAGS(TYPEFLAG_FPROXY);
    }
    AddToTLDataStrW(pTLData, wszNewLine);
    AddToTLDataStrW(pTLData, wszCloseBrackets1);
    AddToTLDataStrW(pTLData, wszNewLine);
    AddToTLDataStrW(pTLData, wszInterface);
    AddToTLDataStrW(pTLData, wszName);
    AddToTLDataStrW(pTLData, wszSpace);
    if(cImplTypes)
    {
        AddToTLDataStrW(pTLData, wszColon);
        AddToTLDataStrW(pTLData, wszSpace);

        ITypeInfo_GetRefTypeOfImplType(pTypeInfo, 0, &hRefType);
        ITypeInfo_GetRefTypeInfo(pTypeInfo, hRefType, &pRefTypeInfo);
        ITypeInfo_GetDocumentation(pRefTypeInfo, MEMBERID_NIL, &bstrName,
                NULL, NULL, NULL);
        AddToTLDataStrW(pTLData, bstrName);
        AddToTLDataStrW(pTLData, wszSpace);

        SysFreeString(bstrName);
        ITypeInfo_Release(pRefTypeInfo);
    }
    AddToTLDataStrW(pTLData, wszOpenBrackets3);
    AddToTLDataStrW(pTLData, wszNewLine);

    AddToStrW(pTLData->wszInsertAfter, wszCloseBrackets3);
    AddToStrW(pTLData->wszInsertAfter, wszSemicolon);
    AddToStrW(pTLData->wszInsertAfter, wszNewLine);
}

int PopulateTree(void)
{
    TVINSERTSTRUCT tvis;
    ITypeLib *pTypeLib;
    TLIBATTR *pTLibAttr;
    ITypeInfo *pTypeInfo, *pRefTypeInfo;
    HREFTYPE hRefType;
    TYPEATTR *pTypeAttr;
    INT count, i;
    BSTR bstrName;
    BSTR bstrData;
    WCHAR wszText[MAX_LOAD_STRING];
    WCHAR wszAfter[MAX_LOAD_STRING];
    HRESULT hRes;
    HTREEITEM hParent;

    const WCHAR wszGeneratedInfo[] = { '/','/',' ','G','e','n','e','r','a','t','e','d',
        ' ','.','I','D','L',' ','f','i','l','e',' ','(','b','y',' ','t','h','e',' ',
        'O','L','E','/','C','O','M',' ','O','b','j','e','c','t',' ',
        'V','i','e','w','e','r',')','\n','/','/','\n','/','/',' ',
        't','y','p','e','l','i','b',' ','f','i','l','e','n','a','m','e',':',' ','\0'};

    const WCHAR wszFormat[] = { '%','s',' ','(','%','s',')','\0' };
    const WCHAR wszFormat2[] = { 'v','e','r','s','i','o','n',
        '(','%','l','d','.','%','l','d',')','\0' };

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

    const WCHAR wszHelpString[] = { 'h','e','l','p','s','t','r','i','n','g','\0' };
    const WCHAR wszLibrary[] = { 'l','i','b','r','a','r','y',' ','\0' };
    const WCHAR wszTag[] = { 't','a','g','\0' };

    U(tvis).item.mask = TVIF_TEXT|TVIF_PARAM;
    U(tvis).item.cchTextMax = MAX_LOAD_STRING;
    U(tvis).item.pszText = wszText;
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
    ITypeLib_GetLibAttr(pTypeLib, &pTLibAttr);
    
    U(tvis).item.lParam = InitializeTLData();
    AddToTLDataStrW((TYPELIB_DATA*)(U(tvis).item.lParam), wszGeneratedInfo);
    AddToTLDataStrW((TYPELIB_DATA*)(U(tvis).item.lParam), typelib.wszFileName);
    AddToTLDataStrW((TYPELIB_DATA*)(U(tvis).item.lParam), wszNewLine);
    AddToTLDataStrW((TYPELIB_DATA*)(U(tvis).item.lParam), wszNewLine);
    AddToTLDataStrW((TYPELIB_DATA*)(U(tvis).item.lParam), wszOpenBrackets1);
    AddToTLDataStrW((TYPELIB_DATA*)(U(tvis).item.lParam), wszNewLine);
    AddToTLDataStrW((TYPELIB_DATA*)(U(tvis).item.lParam), wszUUID);
    AddToTLDataStrW((TYPELIB_DATA*)(U(tvis).item.lParam), wszOpenBrackets2);
    StringFromGUID2(&(pTLibAttr->guid), wszText, MAX_LOAD_STRING);
    wszText[lstrlenW(wszText)-1] = '\0';
    AddToTLDataStrW((TYPELIB_DATA*)(U(tvis).item.lParam), &wszText[1]);
    AddToTLDataStrW((TYPELIB_DATA*)(U(tvis).item.lParam), wszCloseBrackets2);
    AddToTLDataStrW((TYPELIB_DATA*)(U(tvis).item.lParam), wszComa);
    AddToTLDataStrW((TYPELIB_DATA*)(U(tvis).item.lParam), wszNewLine);
    wsprintfW(wszText, wszFormat2, pTLibAttr->wMajorVerNum, pTLibAttr->wMinorVerNum);
    AddToTLDataStrW((TYPELIB_DATA*)(U(tvis).item.lParam), wszText);
    AddToTLDataStrW((TYPELIB_DATA*)(U(tvis).item.lParam), wszComa);
    AddToTLDataStrW((TYPELIB_DATA*)(U(tvis).item.lParam), wszNewLine);
    AddToTLDataStrW((TYPELIB_DATA*)(U(tvis).item.lParam), wszHelpString);
    AddToTLDataStrW((TYPELIB_DATA*)(U(tvis).item.lParam), wszOpenBrackets2);
    AddToTLDataStrW((TYPELIB_DATA*)(U(tvis).item.lParam), bstrData);
    AddToTLDataStrW((TYPELIB_DATA*)(U(tvis).item.lParam), wszCloseBrackets2);
    AddToTLDataStrW((TYPELIB_DATA*)(U(tvis).item.lParam), wszNewLine);
    AddToTLDataStrW((TYPELIB_DATA*)(U(tvis).item.lParam), wszCloseBrackets1);
    AddToTLDataStrW((TYPELIB_DATA*)(U(tvis).item.lParam), wszNewLine);
    AddToTLDataStrW((TYPELIB_DATA*)(U(tvis).item.lParam), wszLibrary);
    AddToTLDataStrW((TYPELIB_DATA*)(U(tvis).item.lParam), bstrName);
    AddToTLDataStrW((TYPELIB_DATA*)(U(tvis).item.lParam), wszNewLine);
    AddToTLDataStrW((TYPELIB_DATA*)(U(tvis).item.lParam), wszOpenBrackets3);
    AddToTLDataStrW((TYPELIB_DATA*)(U(tvis).item.lParam), wszNewLine);

    AddToStrW(((TYPELIB_DATA*)(U(tvis).item.lParam))->wszInsertAfter, wszCloseBrackets3);
    AddToStrW(((TYPELIB_DATA*)(U(tvis).item.lParam))->wszInsertAfter, wszSemicolon);

    wsprintfW(wszText, wszFormat, bstrName, bstrData);
    SysFreeString(bstrName);
    SysFreeString(bstrData);
    tvis.hParent = (HTREEITEM)SendMessage(typelib.hTree,
            TVM_INSERTITEM, 0, (LPARAM)&tvis);

    for(i=0; i<count; i++)
    {
        ITypeLib_GetTypeInfo(pTypeLib, i, &pTypeInfo);

        ITypeInfo_GetDocumentation(pTypeInfo, MEMBERID_NIL, &bstrName, &bstrData,
                NULL, NULL);
        ITypeInfo_GetTypeAttr(pTypeInfo, &pTypeAttr);

        memset(wszText, 0, sizeof(wszText));
        memset(wszAfter, 0, sizeof(wszAfter));
        U(tvis).item.lParam = InitializeTLData();
        switch(pTypeAttr->typekind)
        {
            case TKIND_ENUM:
                AddToStrW(wszText, wszTKIND_ENUM);
                AddToStrW(wszText, bstrName);
                break;
            case TKIND_RECORD:
                AddToTLDataStrW((TYPELIB_DATA*)(U(tvis).item.lParam), wszTKIND_RECORD);
                AddToTLDataStrW((TYPELIB_DATA*)(U(tvis).item.lParam), wszTag);
                AddToTLDataStrW((TYPELIB_DATA*)(U(tvis).item.lParam), bstrName);
                AddToTLDataStrW((TYPELIB_DATA*)(U(tvis).item.lParam), wszSpace);
                AddToTLDataStrW((TYPELIB_DATA*)(U(tvis).item.lParam), wszOpenBrackets3);
                AddToTLDataStrW((TYPELIB_DATA*)(U(tvis).item.lParam), wszNewLine);

                AddToStrW(((TYPELIB_DATA*)(U(tvis).item.lParam))->wszInsertAfter,
                        wszCloseBrackets3);
                AddToStrW(((TYPELIB_DATA*)(U(tvis).item.lParam))->wszInsertAfter,
                        wszSpace);
                AddToStrW(((TYPELIB_DATA*)(U(tvis).item.lParam))->wszInsertAfter,
                        bstrName);
                AddToStrW(((TYPELIB_DATA*)(U(tvis).item.lParam))->wszInsertAfter,
                        wszSemicolon);
                AddToStrW(((TYPELIB_DATA*)(U(tvis).item.lParam))->wszInsertAfter,
                        wszNewLine);

                AddToStrW(wszText, wszTKIND_RECORD);
                AddToStrW(wszText, bstrName);
                break;
            case TKIND_MODULE:
                AddToStrW(wszText, wszTKIND_MODULE);
                AddToStrW(wszText, bstrName);
                break;
            case TKIND_INTERFACE:
                CreateInterfaceInfo(pTypeInfo, pTypeAttr->cImplTypes, bstrName,
                        bstrData, pTypeAttr, (TYPELIB_DATA*)(U(tvis).item.lParam));

                AddToStrW(wszText, wszTKIND_INTERFACE);
                AddToStrW(wszText, bstrName);
                break;
            case TKIND_COCLASS:
                AddToStrW(wszText, wszTKIND_COCLASS);
                AddToStrW(wszText, bstrName);
                break;
            case TKIND_UNION:
                AddToStrW(wszText, wszTKIND_UNION);
                AddToStrW(wszText, bstrName);
                break;
            case TKIND_DISPATCH:
                AddToStrW(wszText, wszTKIND_DISPATCH);
                AddToStrW(wszText, bstrName);
                if(SUCCEEDED(ITypeInfo_GetRefTypeOfImplType(pTypeInfo, -1, &hRefType)))
                {
                    hParent = TreeView_InsertItem(typelib.hTree, &tvis);
                    EnumImplTypes(pTypeInfo, pTypeAttr->cImplTypes, hParent);
                    memset(wszText, 0, sizeof(wszText));
                    U(tvis).item.lParam = InitializeTLData();

                    ITypeInfo_GetRefTypeInfo(pTypeInfo, hRefType, &pRefTypeInfo);
                    ITypeInfo_GetDocumentation(pRefTypeInfo, MEMBERID_NIL, &bstrName,
                                &bstrData, NULL, NULL);

                    CreateInterfaceInfo(pTypeInfo, pTypeAttr->cImplTypes, bstrName,
                            bstrData, pTypeAttr, (TYPELIB_DATA*)(U(tvis).item.lParam));

                    AddToStrW(wszText, wszTKIND_INTERFACE);
                    AddToStrW(wszText, bstrName);
                    ITypeInfo_Release(pRefTypeInfo);
                }
                break;
            case TKIND_ALIAS:
                AddToStrW(wszText, wszTKIND_ALIAS);
                CreateTypeInfo(wszText, wszAfter, pTypeAttr->tdescAlias, pTypeInfo);
                AddToStrW(wszText, wszSpace);
                AddToStrW(wszText, bstrName);
                AddToStrW(wszText, wszAfter);
                break;
            default:
                lstrcpyW(wszText, bstrName);
                WINE_FIXME("pTypeAttr->typekind ==  %d\n not supported",
                        pTypeAttr->typekind);
        }
        hParent = TreeView_InsertItem(typelib.hTree, &tvis);

        EnumVars(pTypeInfo, pTypeAttr->cVars, hParent);
        EnumFuncs(pTypeInfo, pTypeAttr->cFuncs, hParent);
        EnumImplTypes(pTypeInfo, pTypeAttr->cImplTypes, hParent);

        AddChildrenData(hParent, (TYPELIB_DATA*)(U(tvis).item.lParam));

        ITypeInfo_ReleaseTypeAttr(pTypeInfo, pTypeAttr);
        ITypeInfo_Release(pTypeInfo);
        SysFreeString(bstrName);
        SysFreeString(bstrData);
    }
    SendMessage(typelib.hTree, TVM_EXPAND, TVE_EXPAND, (LPARAM)tvis.hParent);

    ITypeLib_Release(pTypeLib);
    return 0;
}

void UpdateData(HTREEITEM item)
{
    TVITEM tvi;

    memset(&tvi, 0, sizeof(TVITEM));
    tvi.mask = TVIF_PARAM;
    tvi.hItem = item;

    SendMessage(typelib.hTree, TVM_GETITEM, 0, (LPARAM)&tvi);
    if(!tvi.lParam)
    {
        SetWindowText(typelib.hEdit, wszSpace);
        return;
    }

    SetWindowText(typelib.hEdit, ((TYPELIB_DATA*)tvi.lParam)->idl);
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

void EmptyTLTree(void)
{
    HTREEITEM cur, del;
    TVITEM tvi;

    tvi.mask = TVIF_PARAM;
    cur = TreeView_GetChild(typelib.hTree, TVI_ROOT);

    while(TRUE)
    {
        del = cur;
        cur = TreeView_GetChild(typelib.hTree, del);

        if(!cur) cur = TreeView_GetNextSibling(typelib.hTree, del);
        if(!cur) cur = TreeView_GetParent(typelib.hTree, del);

        tvi.hItem = del;
        SendMessage(typelib.hTree, TVM_GETITEM, 0, (LPARAM)&tvi);
        if(tvi.lParam)
        {
            HeapFree(GetProcessHeap(), 0, ((TYPELIB_DATA *)tvi.lParam)->idl);
            HeapFree(GetProcessHeap(), 0, (TYPELIB_DATA *)tvi.lParam);
        }

        SendMessage(typelib.hTree, TVM_DELETEITEM, 0, (LPARAM)del);

        if(!cur) break;
    }
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
                    0, 0, 0, 0, typelib.hPaneWnd, (HMENU)TYPELIB_TREE,
                    globals.hMainInst, NULL);
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
            EmptyTLTree();
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
