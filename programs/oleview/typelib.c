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
static const WCHAR wszVT_BOOL[] = L"VARIANT_BOOL";
static const WCHAR wszVT_UI1[] = L"unsigned char";
static const WCHAR wszVT_UI2[] = L"unsigned short";
static const WCHAR wszVT_UI4[] = L"unsigned long";
static const WCHAR wszVT_UI8[] = L"uint64";
static const WCHAR wszVT_UINT[] = L"unsigned int";
static const WCHAR wszVT_I1[] = L"char";
static const WCHAR wszVT_I2[] = L"short";
static const WCHAR wszVT_I4[] = L"long";
static const WCHAR wszVT_I8[] = L"int64";
static const WCHAR wszVT_R4[] = L"single";
static const WCHAR wszVT_INT[] = L"int";
static const WCHAR wszVT_BSTR[] = L"BSTR";
static const WCHAR wszVT_CY[] = L"CURRENCY";
static const WCHAR wszVT_VARIANT[] = L"VARIANT";
static const WCHAR wszVT_VOID[] = L"void";
static const WCHAR wszVT_ERROR[] = L"SCODE";
static const WCHAR wszVT_LPSTR[] = L"LPSTR";
static const WCHAR wszVT_LPWSTR[] = L"LPWSTR";
static const WCHAR wszVT_HRESULT[] = L"HRESULT";
static const WCHAR wszVT_UNKNOWN[] = L"IUnknown";
static const WCHAR wszVT_DISPATCH[] = L"IDispatch";
static const WCHAR wszVT_DATE[] = L"DATE";
static const WCHAR wszVT_R8[] = L"double";
static const WCHAR wszVT_SAFEARRAY[] = L"SAFEARRAY";

static const WCHAR wszPARAMFLAG_FIN[] = L"in";
static const WCHAR wszPARAMFLAG_FOUT[] = L"out";
static const WCHAR wszPARAMFLAG_FLCID[] = L"cid";
static const WCHAR wszPARAMFLAG_FRETVAL[] = L"retval";
static const WCHAR wszPARAMFLAG_FOPT[] = L"optional";
static const WCHAR wszPARAMFLAG_FHASCUSTDATA[] = L"hascustdata";

static void ShowLastError(void)
{
    DWORD error = GetLastError();
    LPWSTR lpMsgBuf;
    WCHAR wszTitle[MAX_LOAD_STRING];

    LoadStringW(globals.hMainInst, IDS_TYPELIBTITLE, wszTitle, ARRAY_SIZE(wszTitle));
    FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
            NULL, error, 0, (LPWSTR)&lpMsgBuf, 0, NULL);
    MessageBoxW(NULL, lpMsgBuf, wszTitle, MB_OK | MB_ICONERROR);
    LocalFree(lpMsgBuf);
    return;
}

static void SaveIdl(WCHAR *wszFileName)
{
    HTREEITEM hIDL;
    TVITEMW tvi;
    HANDLE hFile;
    DWORD len, dwNumWrite;
    char *wszIdl;
    TYPELIB_DATA *data;

    hIDL = (HTREEITEM)SendMessageW(typelib.hTree, TVM_GETNEXTITEM,
            TVGN_CHILD, (LPARAM)TVI_ROOT);

    memset(&tvi, 0, sizeof(TVITEMW));
    tvi.hItem = hIDL;

    SendMessageW(typelib.hTree, TVM_GETITEMW, 0, (LPARAM)&tvi);
    data = (TYPELIB_DATA *)tvi.lParam;

    hFile = CreateFileW(wszFileName, GENERIC_WRITE, FILE_SHARE_WRITE,
                       NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if(hFile == INVALID_HANDLE_VALUE)
    {
        ShowLastError();
        return;
    }

    len = WideCharToMultiByte( CP_UTF8, 0, data->idl, data->idlLen, NULL, 0, NULL, NULL );
    wszIdl = malloc(len);
    WideCharToMultiByte( CP_UTF8, 0, data->idl, data->idlLen, wszIdl, len, NULL, NULL );

    if(!WriteFile(hFile, wszIdl, len, &dwNumWrite, NULL))
        ShowLastError();

    free(wszIdl);
    CloseHandle(hFile);
}

static void GetSaveIdlAsPath(void)
{
    OPENFILENAMEW saveidl;
    WCHAR *pFileName;
    WCHAR wszPath[MAX_LOAD_STRING];
    WCHAR wszDir[MAX_LOAD_STRING];

    memset(&saveidl, 0, sizeof(saveidl));

    lstrcpyW(wszDir, typelib.wszFileName);
    pFileName = wszDir + lstrlenW(wszDir);
    while(*pFileName != '.' && *pFileName != '\\' && *pFileName != '/'
            && pFileName > wszDir) pFileName -= 1;
    if(*pFileName == '.')
    {
        *pFileName = '\0';
        while(*pFileName != '\\' && *pFileName != '/' && pFileName > wszDir)
            pFileName -= 1;
    }
    if(*pFileName == '\\' || *pFileName == '/') pFileName += 1;
    lstrcpyW(wszPath, pFileName);

    GetCurrentDirectoryW(MAX_LOAD_STRING, wszDir);

    saveidl.lStructSize = sizeof(OPENFILENAMEW);
    saveidl.hwndOwner = globals.hTypeLibWnd;
    saveidl.hInstance = globals.hMainInst;
    saveidl.lpstrFilter = L"*.idl\0";
    saveidl.lpstrFile = wszPath;
    saveidl.nMaxFile = MAX_LOAD_STRING;
    saveidl.lpstrInitialDir = wszDir;
    saveidl.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY;
    saveidl.lpstrDefExt = L"idl";

    if (GetSaveFileNameW(&saveidl))
        SaveIdl(wszPath);
}

static void AddToStrW(WCHAR *wszDest, const WCHAR *wszSource)
{
    lstrcpyW(&wszDest[lstrlenW(wszDest)], wszSource);
}

static void AddToTLDataStrW(TYPELIB_DATA *pTLData, const WCHAR *wszSource)
{
    int SourceLen = lstrlenW(wszSource);

    pTLData->idl = realloc(pTLData->idl, sizeof(WCHAR) * (pTLData->idlLen + SourceLen + 1));

    memcpy(&pTLData->idl[pTLData->idlLen], wszSource, sizeof(WCHAR)*(SourceLen+1));
    pTLData->idlLen += SourceLen;
}

static void AddToTLDataStrWithTabsW(TYPELIB_DATA *pTLData, const WCHAR *wszSource)
{
    int lineLen = lstrlenW(wszSource);
    int newLinesNo = 0;
    const WCHAR *pSourcePos = wszSource;
    const WCHAR *pSourceBeg;

    if(!lineLen) return;
    while(*pSourcePos)
    {
        if(*pSourcePos == L'\n') newLinesNo++;
        pSourcePos += 1;
    }
    if(*(pSourcePos - 1) != L'\n') newLinesNo++;

    pTLData->idl = realloc(pTLData->idl,
            sizeof(WCHAR) * (pTLData->idlLen + lineLen + 4 * newLinesNo + 1));

    pSourcePos = wszSource;
    pSourceBeg = wszSource;
    while(newLinesNo)
    {
        if(*pSourcePos != L'\n' && *pSourcePos)
        {
            pSourcePos += 1;
            continue;
        }
        newLinesNo--;

        if(*pSourcePos) pSourcePos++;
        lineLen = pSourcePos - pSourceBeg;

        pTLData->idl[pTLData->idlLen] = L' ';
        pTLData->idl[pTLData->idlLen+1] = L' ';
        pTLData->idl[pTLData->idlLen+2] = L' ';
        pTLData->idl[pTLData->idlLen+3] = L' ';
        memcpy(&pTLData->idl[pTLData->idlLen+4], pSourceBeg, sizeof(WCHAR)*lineLen);
        pTLData->idlLen += lineLen + 4;
        pTLData->idl[pTLData->idlLen] = '\0';

        pSourceBeg = pSourcePos;
    }
}

static TYPELIB_DATA *InitializeTLData(void)
{
    TYPELIB_DATA *pTLData;

    pTLData = calloc(1, sizeof(TYPELIB_DATA));

    pTLData->idl = malloc(sizeof(WCHAR));
    pTLData->idl[0] = '\0';

    return pTLData;
}

static void AddSpaces(TYPELIB_DATA *pTLData, int tabSize)
{
    for(; tabSize>0; tabSize--)
        AddToTLDataStrW(pTLData, L" ");
}

static void AddChildrenData(HTREEITEM hParent, TYPELIB_DATA *pData)
{
    HTREEITEM hCur;
    TVITEMW tvi;

    memset(&tvi, 0, sizeof(tvi));

    hCur = (HTREEITEM)SendMessageW(typelib.hTree, TVM_GETNEXTITEM,
            TVGN_CHILD, (LPARAM)hParent);
    if(!hCur) return;

    do
    {
        tvi.hItem = hCur;
        SendMessageW(typelib.hTree, TVM_GETITEMW, 0, (LPARAM)&tvi);
        if(tvi.lParam && ((TYPELIB_DATA *)(tvi.lParam))->idlLen)
            AddToTLDataStrWithTabsW(pData, ((TYPELIB_DATA *)(tvi.lParam))->idl);
    } while((hCur = (HTREEITEM)SendMessageW(typelib.hTree, TVM_GETNEXTITEM,
            TVGN_NEXT, (LPARAM)hCur)));
}

static void CreateTypeInfo(WCHAR *wszAddTo, WCHAR *wszAddAfter, TYPEDESC tdesc, ITypeInfo *pTypeInfo)
{
    int i;
    BSTR bstrData;
    HRESULT hRes;
    ITypeInfo *pRefTypeInfo;
    WCHAR wszBuf[MAX_LOAD_STRING];

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
        VTADDTOSTR(VT_HRESULT);
        VTADDTOSTR(VT_UNKNOWN);
        VTADDTOSTR(VT_DISPATCH);
        VTADDTOSTR(VT_DATE);
        VTADDTOSTR(VT_R8);
        case VT_CARRAY:
        for(i=0; i<tdesc.lpadesc->cDims; i++)
        {
            wsprintfW(wszBuf, L"[%lu]", tdesc.lpadesc->rgbounds[i].cElements);
            AddToStrW(wszAddAfter, wszBuf);
        }
        CreateTypeInfo(wszAddTo, wszAddAfter, tdesc.lpadesc->tdescElem, pTypeInfo);
        break;
	case VT_SAFEARRAY:
	AddToStrW(wszAddTo, wszVT_SAFEARRAY);
        AddToStrW(wszAddTo, L"(");
        CreateTypeInfo(wszAddTo, wszAddAfter, *tdesc.lptdesc, pTypeInfo);
        AddToStrW(wszAddTo, L")");
	break;
        case VT_PTR:
        CreateTypeInfo(wszAddTo, wszAddAfter, *tdesc.lptdesc, pTypeInfo);
        AddToStrW(wszAddTo, L"*");
        break;
        case VT_USERDEFINED:
        hRes = ITypeInfo_GetRefTypeInfo(pTypeInfo,
                tdesc.hreftype, &pRefTypeInfo);
        if(SUCCEEDED(hRes))
        {
            ITypeInfo_GetDocumentation(pRefTypeInfo, MEMBERID_NIL,
                    &bstrData, NULL, NULL, NULL);
            AddToStrW(wszAddTo, bstrData);
            SysFreeString(bstrData);
            ITypeInfo_Release(pRefTypeInfo);
        }
        else AddToStrW(wszAddTo, L"<failed>");
        break;
        default:
        WINE_FIXME("tdesc.vt&VT_TYPEMASK == %d not supported\n",
                tdesc.vt&VT_TYPEMASK);
    }
}

static int EnumVars(ITypeInfo *pTypeInfo, int cVars, HTREEITEM hParent)
{
    int i;
    TVINSERTSTRUCTW tvis;
    VARDESC *pVarDesc;
    BSTR bstrName;
    WCHAR wszText[MAX_LOAD_STRING];
    WCHAR wszAfter[MAX_LOAD_STRING];

    tvis.item.mask = TVIF_TEXT|TVIF_PARAM;
    tvis.item.cchTextMax = MAX_LOAD_STRING;
    tvis.item.pszText = wszText;
    tvis.hInsertAfter = TVI_LAST;
    tvis.hParent = hParent;

    for(i=0; i<cVars; i++)
    {
        TYPELIB_DATA *tld;

        if(FAILED(ITypeInfo_GetVarDesc(pTypeInfo, i, &pVarDesc))) continue;
        if(FAILED(ITypeInfo_GetDocumentation(pTypeInfo, pVarDesc->memid, &bstrName,
                NULL, NULL, NULL))) continue;

        tld = InitializeTLData();
        tvis.item.lParam = (LPARAM) tld;
        if(pVarDesc->memid < MIN_VAR_ID)
        {

            wsprintfW(wszText, L"[id(0x%.8lx)", pVarDesc->memid);
            AddToTLDataStrW(tld, wszText);

            if(pVarDesc->wVarFlags & VARFLAG_FREADONLY) AddToTLDataStrW(tld, L", readonly");
            AddToTLDataStrW(tld, L"]\n");
        }

        memset(wszText, 0, sizeof(wszText));
        memset(wszAfter, 0, sizeof(wszAfter));
        CreateTypeInfo(wszText, wszAfter, pVarDesc->elemdescVar.tdesc, pTypeInfo);
        AddToStrW(wszText, L" ");
        if (bstrName) AddToStrW(wszText, bstrName);
        AddToStrW(wszText, wszAfter);
        AddToTLDataStrW(tld, wszText);
        AddToTLDataStrW(tld, L";\n");

        SendMessageW(typelib.hTree, TVM_INSERTITEMW, 0, (LPARAM)&tvis);
        SysFreeString(bstrName);
        ITypeInfo_ReleaseVarDesc(pTypeInfo, pVarDesc);
    }

    return 0;
}

static int EnumEnums(ITypeInfo *pTypeInfo, int cVars, HTREEITEM hParent)
{
    int i;
    TVINSERTSTRUCTW tvis;
    VARDESC *pVarDesc;
    BSTR bstrName;
    WCHAR wszText[MAX_LOAD_STRING];
    WCHAR wszAfter[MAX_LOAD_STRING];

    tvis.item.mask = TVIF_TEXT|TVIF_PARAM;
    tvis.item.cchTextMax = MAX_LOAD_STRING;
    tvis.item.pszText = wszText;
    tvis.hInsertAfter = TVI_LAST;
    tvis.hParent = hParent;

    for(i=0; i<cVars; i++)
    {
        TYPELIB_DATA *tld;

        if(FAILED(ITypeInfo_GetVarDesc(pTypeInfo, i, &pVarDesc))) continue;
        if(FAILED(ITypeInfo_GetDocumentation(pTypeInfo, pVarDesc->memid, &bstrName,
                NULL, NULL, NULL))) continue;

        tld = InitializeTLData();
        tvis.item.lParam = (LPARAM) tld;

        memset(wszText, 0, sizeof(wszText));
        memset(wszAfter, 0, sizeof(wszAfter));

        if (pVarDesc->varkind == VAR_CONST)
        {
            VARIANT var;
            VariantInit(&var);
            if (VariantChangeType(&var, pVarDesc->lpvarValue, 0, VT_BSTR) == S_OK)
            {
                AddToStrW(wszText, L"const ");
                AddToStrW(wszAfter, L" = ");
                AddToStrW(wszAfter, V_BSTR(&var));
            }
        }

        CreateTypeInfo(wszText, wszAfter, pVarDesc->elemdescVar.tdesc, pTypeInfo);
        AddToStrW(wszText, L" ");
        AddToStrW(wszText, bstrName);
        AddToStrW(wszText, wszAfter);
        AddToTLDataStrW(tld, bstrName);
        AddToTLDataStrW(tld, wszAfter);
	if (i<cVars-1)
            AddToTLDataStrW(tld, L",");
        AddToTLDataStrW(tld, L"\n");

        SendMessageW(typelib.hTree, TVM_INSERTITEMW, 0, (LPARAM)&tvis);
        SysFreeString(bstrName);
        ITypeInfo_ReleaseVarDesc(pTypeInfo, pVarDesc);
    }

    return 0;
}

static int EnumFuncs(ITypeInfo *pTypeInfo, TYPEATTR *pTypeAttr, HTREEITEM hParent)
{
    int i, j;
    int cFuncs;
    unsigned namesNo;
    TVINSERTSTRUCTW tvis;
    FUNCDESC *pFuncDesc;
    BSTR bstrName, bstrHelpString, *bstrParamNames;
    WCHAR wszText[MAX_LOAD_STRING];
    WCHAR wszAfter[MAX_LOAD_STRING];
    BOOL bFirst;

    tvis.item.mask = TVIF_TEXT|TVIF_PARAM;
    tvis.hInsertAfter = TVI_LAST;
    tvis.hParent = hParent;

    cFuncs = pTypeAttr->cFuncs;

    i = 0;
    if(pTypeAttr->wTypeFlags & TYPEFLAG_FDUAL) { /* skip 7 members of IDispatch */
        cFuncs += 7;
        i += 7;
    }

    for(; i<cFuncs; i++)
    {
        TYPELIB_DATA *tld;

        if(FAILED(ITypeInfo_GetFuncDesc(pTypeInfo, i, &pFuncDesc))) continue;

        if(FAILED(ITypeInfo_GetDocumentation(pTypeInfo, pFuncDesc->memid, &bstrName,
                &bstrHelpString, NULL, NULL))) continue;

        memset(wszText, 0, sizeof(wszText));
        memset(wszAfter, 0, sizeof(wszAfter));
        tld = InitializeTLData();
        tvis.item.cchTextMax = SysStringLen(bstrName);
        tvis.item.pszText = bstrName;
        tvis.item.lParam = (LPARAM) tld;
        bFirst = TRUE;
        if(pFuncDesc->memid < MIN_FUNC_ID || pTypeAttr->wTypeFlags & TYPEFLAG_FDUAL)
        {
            bFirst = FALSE;
            wsprintfW(wszText, L"[id(0x%.8lx)", pFuncDesc->memid);
            AddToTLDataStrW(tld, wszText);
            memset(wszText, 0, sizeof(wszText));
        }

        CreateTypeInfo(wszText, wszAfter, pFuncDesc->elemdescFunc.tdesc, pTypeInfo);
        switch(pFuncDesc->invkind)
        {
            case INVOKE_PROPERTYGET:
                if(bFirst) AddToTLDataStrW(tld, L"[");
                else AddToTLDataStrW(tld, L", ");
                bFirst = FALSE;
                AddToTLDataStrW(tld, L"propget");
                break;
            case INVOKE_PROPERTYPUT:
                if(bFirst) AddToTLDataStrW(tld, L"[");
                else AddToTLDataStrW(tld, L", ");
                bFirst = FALSE;
                AddToTLDataStrW(tld, L"propput");
                break;
            case INVOKE_PROPERTYPUTREF:
                if(bFirst) AddToTLDataStrW(tld, L"[");
                else AddToTLDataStrW(tld, L", ");
                bFirst = FALSE;
                AddToTLDataStrW(tld, L"propputref");
                break;
            default:;
        }
        if(SysStringLen(bstrHelpString))
        {
            if(bFirst) AddToTLDataStrW(tld, L"[");
            else AddToTLDataStrW(tld, L", ");
            bFirst = FALSE;
            AddToTLDataStrW(tld, L"helpstring(\"");
            AddToTLDataStrW(tld, bstrHelpString);
            AddToTLDataStrW(tld, L"\")");
        }
        if(!bFirst) AddToTLDataStrW(tld, L"]\n");

        if(pTypeAttr->wTypeFlags & TYPEFLAG_FOLEAUTOMATION) {
            AddToTLDataStrW(tld, wszVT_HRESULT);
            if(lstrcmpW(wszText, wszVT_VOID)) pFuncDesc->cParams++;
        }
        else {
            AddToTLDataStrW(tld, wszText);
            AddToTLDataStrW(tld, wszAfter);
        }

        bstrParamNames = malloc(sizeof(BSTR) * (pFuncDesc->cParams + 1));
        if(FAILED(ITypeInfo_GetNames(pTypeInfo, pFuncDesc->memid, bstrParamNames,
                pFuncDesc->cParams+1, &namesNo)))
        {
            free(bstrParamNames);
            continue;
        }
        SysFreeString(bstrParamNames[0]);

        AddToTLDataStrW(tld, L" ");
        if(pFuncDesc->memid >= MIN_FUNC_ID)
        {
            AddToTLDataStrW(tld, L"_stdcall ");
        }
        if (bstrName) AddToTLDataStrW(tld, bstrName);
        AddToTLDataStrW(tld, L"(");

        for(j=0; j<pFuncDesc->cParams; j++)
        {
            if(j != 0) AddToTLDataStrW(tld, L",");
            if(pFuncDesc->cParams != 1)
            {
                AddToTLDataStrW(tld, L"\n");
                AddSpaces(tld, TAB_SIZE);
            }
            bFirst = TRUE;
#define ENUM_PARAM_FLAG(x)\
            if(pFuncDesc->lprgelemdescParam[j].paramdesc.wParamFlags & x) \
            {\
                if(bFirst)\
                    AddToTLDataStrW(tld, L"[");\
                else\
                {\
                    AddToTLDataStrW(tld, L", ");\
                }\
                bFirst = FALSE;\
                AddToTLDataStrW(tld, wsz##x);\
            }
            ENUM_PARAM_FLAG(PARAMFLAG_FIN);
            ENUM_PARAM_FLAG(PARAMFLAG_FOUT);
            ENUM_PARAM_FLAG(PARAMFLAG_FLCID);
            ENUM_PARAM_FLAG(PARAMFLAG_FRETVAL);
            ENUM_PARAM_FLAG(PARAMFLAG_FOPT);
            ENUM_PARAM_FLAG(PARAMFLAG_FHASCUSTDATA);

            if(pFuncDesc->lprgelemdescParam[j].paramdesc.wParamFlags & PARAMFLAG_FHASDEFAULT)
            {
		VARIANT var, *param=&pFuncDesc->lprgelemdescParam[j].paramdesc.pparamdescex->varDefaultValue;
		VariantInit(&var);
                if(bFirst) AddToTLDataStrW(tld, L"[");
                else AddToTLDataStrW(tld, L", ");
                bFirst = FALSE;
                AddToTLDataStrW(tld, L"defaultvalue(");
		if (V_VT(param) == VT_BSTR)
		{
                    AddToTLDataStrW(tld, L"\"");
		    AddToTLDataStrW(tld, V_BSTR(param));
                    AddToTLDataStrW(tld, L"\"");
		} else if (VariantChangeType(&var, param, 0, VT_BSTR) == S_OK)
		    AddToTLDataStrW(tld, V_BSTR(&var));
                AddToTLDataStrW(tld, L")");
            }

            if(!bFirst) AddToTLDataStrW(tld, L"] ");

            memset(wszText, 0, sizeof(wszText));
            memset(wszAfter, 0, sizeof(wszAfter));
            CreateTypeInfo(wszText, wszAfter, pFuncDesc->lprgelemdescParam[j].tdesc,
                    pTypeInfo);
            AddToTLDataStrW(tld, wszText);
            AddToTLDataStrW(tld, wszAfter);
            AddToTLDataStrW(tld, L" ");
            if (j+1 < namesNo) {
                if (bstrParamNames[j+1])
                {
                    AddToTLDataStrW(tld, bstrParamNames[j+1]);
                    SysFreeString(bstrParamNames[j+1]);
                }
            } else {
                AddToTLDataStrW(tld, L"rhs");
            }
        }
        AddToTLDataStrW(tld, L");\n");

        SendMessageW(typelib.hTree, TVM_INSERTITEMW, 0, (LPARAM)&tvis);
        free(bstrParamNames);
        SysFreeString(bstrName);
        SysFreeString(bstrHelpString);
        ITypeInfo_ReleaseFuncDesc(pTypeInfo, pFuncDesc);
    }

    return 0;
}

static int EnumImplTypes(ITypeInfo *pTypeInfo, int cImplTypes, HTREEITEM hParent)
{
    int i;
    TVINSERTSTRUCTW tvis;
    ITypeInfo *pRefTypeInfo;
    HREFTYPE hRefType;
    TYPEATTR *pTypeAttr;
    BSTR bstrName;
    WCHAR wszInheritedInterfaces[MAX_LOAD_STRING];

    if(!cImplTypes) return 0;

    LoadStringW(globals.hMainInst, IDS_INHERITINTERFACES, wszInheritedInterfaces,
                ARRAY_SIZE(wszInheritedInterfaces));

    tvis.item.mask = TVIF_TEXT;
    tvis.item.cchTextMax = MAX_LOAD_STRING;
    tvis.item.pszText = wszInheritedInterfaces;
    tvis.hInsertAfter = TVI_LAST;
    tvis.hParent = hParent;

    tvis.hParent = TreeView_InsertItemW(typelib.hTree, &tvis);

    for(i=0; i<cImplTypes; i++)
    {
        if(FAILED(ITypeInfo_GetRefTypeOfImplType(pTypeInfo, i, &hRefType))) continue;
        if(FAILED(ITypeInfo_GetRefTypeInfo(pTypeInfo, hRefType, &pRefTypeInfo)))
            continue;
        if(FAILED(ITypeInfo_GetDocumentation(pRefTypeInfo, MEMBERID_NIL, &bstrName,
                NULL, NULL, NULL)))
        {
            ITypeInfo_Release(pRefTypeInfo);
            continue;
        }
        if(FAILED(ITypeInfo_GetTypeAttr(pRefTypeInfo, &pTypeAttr)))
        {
            ITypeInfo_Release(pRefTypeInfo);
            continue;
        }

        tvis.item.cchTextMax = SysStringLen(bstrName);
        tvis.item.pszText = bstrName;

        hParent = TreeView_InsertItemW(typelib.hTree, &tvis);
        EnumVars(pRefTypeInfo, pTypeAttr->cVars, hParent);
        EnumFuncs(pRefTypeInfo, pTypeAttr, hParent);
        EnumImplTypes(pRefTypeInfo, pTypeAttr->cImplTypes, hParent);

        SysFreeString(bstrName);
        ITypeInfo_ReleaseTypeAttr(pRefTypeInfo, pTypeAttr);
        ITypeInfo_Release(pRefTypeInfo);
    }

    return 0;
}

static void EnumCoclassImplTypes(ITypeInfo *pTypeInfo,
        int cImplTypes, TYPELIB_DATA *pTLData)
{
    int i;
    ITypeInfo *pRefTypeInfo;
    HREFTYPE hRefType;
    TYPEATTR *pTypeAttr;
    BSTR bstrName;
    BOOL bFirst;
    INT flags;
    static const WCHAR wszIMPLTYPEFLAG_FDEFAULT[] = L"default";
    static const WCHAR wszIMPLTYPEFLAG_FSOURCE[] = L"source";
    static const WCHAR wszIMPLTYPEFLAG_FRESTRICTED[] = L"restricted";

    for(i=0; i<cImplTypes; i++)
    {
        if(FAILED(ITypeInfo_GetRefTypeOfImplType(pTypeInfo, i, &hRefType))) continue;
        if(FAILED(ITypeInfo_GetRefTypeInfo(pTypeInfo, hRefType, &pRefTypeInfo)))
            continue;
        if(FAILED(ITypeInfo_GetDocumentation(pRefTypeInfo, MEMBERID_NIL, &bstrName,
                        NULL, NULL, NULL)))
        {
            ITypeInfo_Release(pRefTypeInfo);
            continue;
        }
        if(FAILED(ITypeInfo_GetTypeAttr(pRefTypeInfo, &pTypeAttr)))
        {
            ITypeInfo_Release(pRefTypeInfo);
            continue;
        }

        AddSpaces(pTLData, TAB_SIZE);
        ITypeInfo_GetImplTypeFlags(pTypeInfo, i, &flags);
        bFirst = TRUE;
#define ENUM_IMPLTYPEFLAG(x)\
        if(flags & x) \
        {\
            if(bFirst)\
                AddToTLDataStrW(pTLData, L"[");\
            else\
            {\
                AddToTLDataStrW(pTLData, L", ");\
            }\
            bFirst = FALSE;\
            AddToTLDataStrW(pTLData, wsz##x);\
        }
        ENUM_IMPLTYPEFLAG(IMPLTYPEFLAG_FDEFAULT);
        ENUM_IMPLTYPEFLAG(IMPLTYPEFLAG_FSOURCE);
        ENUM_IMPLTYPEFLAG(IMPLTYPEFLAG_FRESTRICTED);
        if(!bFirst)
            AddToTLDataStrW(pTLData, L"] ");

        if(pTypeAttr->typekind == TKIND_INTERFACE ||
                (pTypeAttr->wTypeFlags & TYPEFLAG_FDUAL))
            AddToTLDataStrW(pTLData, L"interface ");
        else if(pTypeAttr->typekind == TKIND_DISPATCH)
            AddToTLDataStrW(pTLData, L"dispinterface ");
        AddToTLDataStrW(pTLData, L" ");

        AddToTLDataStrW(pTLData, bstrName);
        AddToTLDataStrW(pTLData, L";\n");

        SysFreeString(bstrName);
        ITypeInfo_ReleaseTypeAttr(pRefTypeInfo, pTypeAttr);
        ITypeInfo_Release(pRefTypeInfo);
    }
}

static void AddIdlData(HTREEITEM hCur, TYPELIB_DATA *pTLData)
{
    TVITEMW tvi;

    hCur = (HTREEITEM)SendMessageW(typelib.hTree, TVM_GETNEXTITEM,
            TVGN_CHILD, (LPARAM)hCur);
    memset(&tvi, 0, sizeof(TVITEMW));
    tvi.mask = TVIF_PARAM;

    while(hCur)
    {
        tvi.hItem = hCur;
        SendMessageW(typelib.hTree, TVM_GETITEMW, 0, (LPARAM)&tvi);
        if(!((TYPELIB_DATA*)(tvi.lParam))->bHide) {
            AddToTLDataStrW(pTLData, L"\n");
            AddToTLDataStrWithTabsW(pTLData, ((TYPELIB_DATA*)(tvi.lParam))->idl);
        }
        hCur = (HTREEITEM)SendMessageW(typelib.hTree, TVM_GETNEXTITEM,
                TVGN_NEXT, (LPARAM)hCur);
    }
}

static void AddPredefinitions(HTREEITEM hFirst, TYPELIB_DATA *pTLData)
{
    HTREEITEM hCur;
    TVITEMW tvi;
    WCHAR wszText[MAX_LOAD_STRING];

    hFirst = (HTREEITEM)SendMessageW(typelib.hTree, TVM_GETNEXTITEM,
            TVGN_CHILD, (LPARAM)hFirst);

    AddToTLDataStrWithTabsW(pTLData, L"// TLib :\n// Forward declare all types defined in this typelib");
    AddToTLDataStrW(pTLData, L"\n");

    hCur = hFirst;
    memset(&tvi, 0, sizeof(TVITEMW));
    tvi.mask = TVIF_TEXT|TVIF_PARAM;
    tvi.cchTextMax = MAX_LOAD_STRING;
    tvi.pszText = wszText;
    while(hCur)
    {
        tvi.hItem = hCur;
        SendMessageW(typelib.hTree, TVM_GETITEMW, 0, (LPARAM)&tvi);
        if(((TYPELIB_DATA*)(tvi.lParam))->bPredefine &&
                !((TYPELIB_DATA*)(tvi.lParam))->bHide)
        {
            AddToStrW(wszText, L";");
            AddToTLDataStrWithTabsW(pTLData, wszText);
            AddToTLDataStrW(pTLData, L"\n");
        }
        hCur = (HTREEITEM)SendMessageW(typelib.hTree, TVM_GETNEXTITEM,
                TVGN_NEXT, (LPARAM)hCur);
    }
}

static void CreateInterfaceInfo(ITypeInfo *pTypeInfo, int cImplTypes, WCHAR *wszName,
        WCHAR *wszHelpString, ULONG ulHelpContext, TYPEATTR *pTypeAttr,
        TYPELIB_DATA *pTLData)
{
    ITypeInfo *pRefTypeInfo;
    HREFTYPE hRefType;
    BSTR bstrName;
    WCHAR wszGuid[MAX_LOAD_STRING];
    WCHAR wszHelpContext[MAX_LOAD_STRING];

    static const WCHAR wszTYPEFLAG_FAPPOBJECT[] = L"appobject";
    static const WCHAR wszTYPEFLAG_FCANCREATE[] = L"cancreate";
    static const WCHAR wszTYPEFLAG_FLICENSED[] = L"licensed";
    static const WCHAR wszTYPEFLAG_FPREDECLID[] = L"predeclid";
    static const WCHAR wszTYPEFLAG_FHIDDEN[] = L"hidden";
    static const WCHAR wszTYPEFLAG_FCONTROL[] = L"control";
    static const WCHAR wszTYPEFLAG_FDUAL[] = L"dual";
    static const WCHAR wszTYPEFLAG_FNONEXTENSIBLE[] = L"nonextensible";
    static const WCHAR wszTYPEFLAG_FOLEAUTOMATION[] = L"oleautomation";
    static const WCHAR wszTYPEFLAG_FRESTRICTED[] = L"restricted";
    static const WCHAR wszTYPEFLAG_FAGGREGATABLE[] = L"aggregatable";
    static const WCHAR wszTYPEFLAG_FREPLACEABLE[] = L"replacable";
    static const WCHAR wszTYPEFLAG_FREVERSEBIND[] = L"reversebind";
    static const WCHAR wszTYPEFLAG_FPROXY[] = L"proxy";

    AddToTLDataStrW(pTLData, L"[\n");
    if(pTypeAttr->typekind != TKIND_DISPATCH)
    {
        AddSpaces(pTLData, TAB_SIZE);
        AddToTLDataStrW(pTLData, L"odl,\n");
    }
    AddSpaces(pTLData, TAB_SIZE);
    AddToTLDataStrW(pTLData, L"uuid(");
    StringFromGUID2(&(pTypeAttr->guid), wszGuid, MAX_LOAD_STRING);
    wszGuid[lstrlenW(wszGuid)-1] = '\0';
    AddToTLDataStrW(pTLData, &wszGuid[1]);
    AddToTLDataStrW(pTLData, L")");
    if(wszHelpString)
    {
        AddToTLDataStrW(pTLData, L",\n");
        AddSpaces(pTLData, TAB_SIZE);
        AddToTLDataStrW(pTLData, L"helpstring(\"");
        AddToTLDataStrW(pTLData, wszHelpString);
        AddToTLDataStrW(pTLData, L"\")");
    }
    if(ulHelpContext)
    {
        AddToTLDataStrW(pTLData, L",\n");
        AddSpaces(pTLData, TAB_SIZE);
        wsprintfW(wszHelpContext, L"helpcontext(0x%.8lx)", ulHelpContext);
        AddToTLDataStrW(pTLData, wszHelpContext);
    }
    if(pTypeAttr->wTypeFlags)
    {
#define ENUM_FLAGS(x) if(pTypeAttr->wTypeFlags & x)\
        {\
            AddToTLDataStrW(pTLData, L",\n");\
            AddSpaces(pTLData, TAB_SIZE);\
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
        ENUM_FLAGS(TYPEFLAG_FREVERSEBIND);
        ENUM_FLAGS(TYPEFLAG_FPROXY);
    }
    AddToTLDataStrW(pTLData, L"\n]\n");
    if(pTypeAttr->typekind != TKIND_DISPATCH) AddToTLDataStrW(pTLData, L"interface ");
    else AddToTLDataStrW(pTLData, L"dispinterface ");
    AddToTLDataStrW(pTLData, wszName);
    AddToTLDataStrW(pTLData, L" ");
    if(cImplTypes && pTypeAttr->typekind != TKIND_DISPATCH)
    {
        AddToTLDataStrW(pTLData, L": ");

        ITypeInfo_GetRefTypeOfImplType(pTypeInfo, 0, &hRefType);
        if (SUCCEEDED(ITypeInfo_GetRefTypeInfo(pTypeInfo, hRefType, &pRefTypeInfo)))
        {
            ITypeInfo_GetDocumentation(pRefTypeInfo, MEMBERID_NIL, &bstrName,
                NULL, NULL, NULL);
            AddToTLDataStrW(pTLData, bstrName);
            AddToTLDataStrW(pTLData, L" ");

            SysFreeString(bstrName);
            ITypeInfo_Release(pRefTypeInfo);
        }
        else
            AddToTLDataStrW(pTLData, L"<failed>");
    }
    AddToTLDataStrW(pTLData, L"{\n");

    AddToStrW(pTLData->wszInsertAfter, L"};\n");
}

static void CreateTypedefHeader(ITypeInfo *pTypeInfo,
        TYPEATTR *pTypeAttr, TYPELIB_DATA *pTLData)
{
    BOOL bFirst = TRUE;
    WCHAR wszGuid[MAX_LOAD_STRING];

    AddToTLDataStrW(pTLData, L"typedef ");
    if(memcmp(&pTypeAttr->guid, &GUID_NULL, sizeof(GUID)))
    {
        bFirst = FALSE;
        AddToTLDataStrW(pTLData, L"[uuid(");
        StringFromGUID2(&(pTypeAttr->guid), wszGuid, MAX_LOAD_STRING);
        wszGuid[lstrlenW(wszGuid)-1] = '\0';
        AddToTLDataStrW(pTLData, &wszGuid[1]);
        AddToTLDataStrW(pTLData, L")");
    }
    if(pTypeAttr->typekind == TKIND_ALIAS)
    {
        if(bFirst) AddToTLDataStrW(pTLData, L"[");
        else AddToTLDataStrW(pTLData, L", ");
        bFirst = FALSE;
        AddToTLDataStrW(pTLData, L"public");
    }
    if(!bFirst) AddToTLDataStrW(pTLData, L"]\n");
}

static void CreateCoclassHeader(ITypeInfo *pTypeInfo,
        TYPEATTR *pTypeAttr, TYPELIB_DATA *pTLData)
{
    WCHAR wszGuid[MAX_LOAD_STRING];
    BSTR bstrHelpString;

    AddToTLDataStrW(pTLData, L"[\n");

    AddSpaces(pTLData, TAB_SIZE);
    AddToTLDataStrW(pTLData, L"uuid(");
    StringFromGUID2(&(pTypeAttr->guid), wszGuid, MAX_LOAD_STRING);
    wszGuid[lstrlenW(wszGuid)-1] = '\0';
    AddToTLDataStrW(pTLData, &wszGuid[1]);
    AddToTLDataStrW(pTLData, L")");

    if(SUCCEEDED(ITypeInfo_GetDocumentation(pTypeInfo, MEMBERID_NIL, NULL,
            &bstrHelpString, NULL, NULL)))
    {
        if(SysStringLen(bstrHelpString))
        {
            AddToTLDataStrW(pTLData, L",\n");
            AddSpaces(pTLData, TAB_SIZE);
            AddToTLDataStrW(pTLData, L"helpstring(\"");
            AddToTLDataStrW(pTLData, bstrHelpString);
            AddToTLDataStrW(pTLData, L"\")");
        }
        SysFreeString(bstrHelpString);
    }

    if(!(pTypeAttr->wTypeFlags & TYPEFLAG_FCANCREATE))
    {
        AddToTLDataStrW(pTLData, L",\n");
        AddSpaces(pTLData, TAB_SIZE);
        AddToTLDataStrW(pTLData, L"noncreatable");
    }

    AddToTLDataStrW(pTLData, L"\n]\n");
}

static int PopulateTree(void)
{
    TVINSERTSTRUCTW tvis;
    TVITEMW tvi;
    ITypeLib *pTypeLib;
    TLIBATTR *pTLibAttr;
    ITypeInfo *pTypeInfo, *pRefTypeInfo;
    HREFTYPE hRefType;
    TYPEATTR *pTypeAttr;
    INT count, i;
    ULONG ulHelpContext;
    BSTR bstrName;
    BSTR bstrData;
    WCHAR wszText[MAX_LOAD_STRING];
    WCHAR wszAfter[MAX_LOAD_STRING];
    HRESULT hRes;
    HTREEITEM hParent;
    HTREEITEM hMain;
    BOOL bInsert;
    TYPELIB_DATA *tldDispatch;
    TYPELIB_DATA *tld;

    tvis.item.mask = TVIF_TEXT|TVIF_PARAM;
    tvis.item.cchTextMax = MAX_LOAD_STRING;
    tvis.item.pszText = wszText;
    tvis.hInsertAfter = TVI_LAST;
    tvis.hParent = TVI_ROOT;

    if(FAILED((hRes = LoadTypeLib(typelib.wszFileName, &pTypeLib))))
    {
        WCHAR wszMessage[MAX_LOAD_STRING];
        WCHAR wszError[MAX_LOAD_STRING];
        DWORD_PTR args[2];

        LoadStringW(globals.hMainInst, IDS_ERROR_LOADTYPELIB, wszError, ARRAY_SIZE(wszError));
        args[0] = (DWORD_PTR)typelib.wszFileName;
        args[1] = hRes;
        FormatMessageW(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ARGUMENT_ARRAY,
                       wszError, 0, 0, wszMessage, ARRAY_SIZE(wszMessage), (va_list *)args);
        MessageBoxW(globals.hMainWnd, wszMessage, NULL, MB_OK|MB_ICONEXCLAMATION);
        return 1;
    }
    count = ITypeLib_GetTypeInfoCount(pTypeLib);

    ITypeLib_GetDocumentation(pTypeLib, -1, &bstrName, &bstrData, NULL, NULL);
    ITypeLib_GetLibAttr(pTypeLib, &pTLibAttr);

    tld = InitializeTLData();
    tvis.item.lParam = (LPARAM) tld;
    AddToTLDataStrW(tld, L"// Generated .IDL file (by the OLE/COM Object Viewer)\n//\n// typelib filename: ");
    AddToTLDataStrW(tld, typelib.wszFileName);
    AddToTLDataStrW(tld, L"\n\n[\n");
    AddSpaces(tld, TAB_SIZE);
    AddToTLDataStrW(tld, L"uuid(");
    StringFromGUID2(&(pTLibAttr->guid), wszText, MAX_LOAD_STRING);
    wszText[lstrlenW(wszText)-1] = '\0';
    AddToTLDataStrW(tld, &wszText[1]);
    AddToTLDataStrW(tld, L"),\n");
    AddSpaces(tld, TAB_SIZE);
    wsprintfW(wszText, L"version(%ld.%ld)", pTLibAttr->wMajorVerNum, pTLibAttr->wMinorVerNum);
    AddToTLDataStrW(tld, wszText);

    if (bstrData)
    {
        /* helpstring is optional */
        AddToTLDataStrW(tld, L",\n");
        AddSpaces(tld, TAB_SIZE);
        AddToTLDataStrW(tld, L"helpstring(\"");
        AddToTLDataStrW(tld, bstrData);
        AddToTLDataStrW(tld, L"\")");
    }

    AddToTLDataStrW(tld, L"\n]\n");
    AddToTLDataStrW(tld, L"library ");
    if (bstrName) AddToTLDataStrW(tld, bstrName);
    AddToTLDataStrW(tld, L"\n{\n");

    AddToStrW(tld->wszInsertAfter, L"};");

    wsprintfW(wszText, L"%s (%s)", bstrName, bstrData);
    SysFreeString(bstrName);
    SysFreeString(bstrData);
    tvis.hParent = (HTREEITEM)SendMessageW(typelib.hTree,
            TVM_INSERTITEMW, 0, (LPARAM)&tvis);

    for(i=0; i<count; i++)
    {
        bInsert = TRUE;
        ITypeLib_GetTypeInfo(pTypeLib, i, &pTypeInfo);

        ITypeInfo_GetDocumentation(pTypeInfo, MEMBERID_NIL, &bstrName, &bstrData,
                &ulHelpContext, NULL);
        ITypeInfo_GetTypeAttr(pTypeInfo, &pTypeAttr);

        memset(wszText, 0, sizeof(wszText));
        memset(wszAfter, 0, sizeof(wszAfter));
        tld = InitializeTLData();
        tvis.item.lParam = (LPARAM)tld;
        switch(pTypeAttr->typekind)
        {
            case TKIND_ENUM:
                AddToStrW(wszText, L"typedef enum ");
                AddToStrW(wszText, bstrName);

                CreateTypedefHeader(pTypeInfo, pTypeAttr, tld);
                AddToTLDataStrW(tld, L"enum {\n");
                AddToStrW(tld->wszInsertAfter, L"} ");
                AddToStrW(tld->wszInsertAfter, bstrName);
                AddToStrW(tld->wszInsertAfter, L";\n");

                bInsert = FALSE;
                hParent = TreeView_InsertItemW(typelib.hTree, &tvis);
                EnumEnums(pTypeInfo, pTypeAttr->cVars, hParent);
                AddChildrenData(hParent, tld);
                AddToTLDataStrW(tld, tld->wszInsertAfter);
                break;
            case TKIND_RECORD:
                AddToTLDataStrW(tld, L"typedef struct tag");
                AddToTLDataStrW(tld, bstrName);
                AddToTLDataStrW(tld, L" {\n");

                AddToStrW(tld->wszInsertAfter, L"} ");
                AddToStrW(tld->wszInsertAfter, bstrName);
                AddToStrW(tld->wszInsertAfter, L";\n");

                AddToStrW(wszText, L"typedef struct ");
                AddToStrW(wszText, bstrName);
                break;
            case TKIND_MODULE:
                AddToStrW(wszText, L"module ");
                AddToStrW(wszText, bstrName);
                break;
            case TKIND_INTERFACE:
                CreateInterfaceInfo(pTypeInfo, pTypeAttr->cImplTypes, bstrName,
                        bstrData, ulHelpContext, pTypeAttr, tld);
                tld->bPredefine = TRUE;

                AddToStrW(wszText, L"interface ");
                AddToStrW(wszText, bstrName);
                break;
            case TKIND_COCLASS:
                AddToStrW(wszText, L"coclass ");
                AddToStrW(wszText, bstrName);

                CreateCoclassHeader(pTypeInfo, pTypeAttr, tld);
                AddToTLDataStrW(tld, L"coclass ");
                AddToTLDataStrW(tld, bstrName);
                AddToTLDataStrW(tld, L" {\n");

                EnumCoclassImplTypes(pTypeInfo, pTypeAttr->cImplTypes, tld);

                AddToStrW(tld->wszInsertAfter, L"};\n");

                bInsert = FALSE;
                hParent = TreeView_InsertItemW(typelib.hTree, &tvis);
                AddToTLDataStrW(tld, tld->wszInsertAfter);
                break;
            case TKIND_UNION:
                AddToStrW(wszText, L"typedef union ");
                AddToStrW(wszText, bstrName);
                break;
            case TKIND_DISPATCH:
                CreateInterfaceInfo(pTypeInfo, pTypeAttr->cImplTypes, bstrName,
                        bstrData, ulHelpContext, pTypeAttr, tld);
                tld->bPredefine = TRUE;
                if(pTypeAttr->wTypeFlags & TYPEFLAG_FDUAL)
                    tld->bHide = TRUE;
                AddToStrW(wszText, L"dispinterface ");
                AddToStrW(wszText, bstrName);

                hParent = TreeView_InsertItemW(typelib.hTree, &tvis);
                hMain = tvis.hParent;
                tldDispatch = tld;

                lstrcpyW(wszText, L"properties");
                tvis.hParent = hParent;
                tld = InitializeTLData();
                tvis.item.lParam = (LPARAM) tld;
                AddToTLDataStrW(tld, L"properties:\n");
                tvis.hParent = TreeView_InsertItemW(typelib.hTree, &tvis);
                EnumVars(pTypeInfo, pTypeAttr->cVars, tvis.hParent);
                AddChildrenData(tvis.hParent, tld);

                lstrcpyW(wszText, L"methods");
                tvis.hParent = hParent;
                tld = InitializeTLData();
                tvis.item.lParam = (LPARAM) tld;
                AddToTLDataStrW(tld, L"methods:\n");
                tvis.hParent = TreeView_InsertItemW(typelib.hTree, &tvis);
                EnumFuncs(pTypeInfo, pTypeAttr, tvis.hParent);
                AddChildrenData(tvis.hParent, tld);

                EnumImplTypes(pTypeInfo, pTypeAttr->cImplTypes, hParent);
                AddChildrenData(hParent, tldDispatch);
                AddToTLDataStrW(tldDispatch, tldDispatch->wszInsertAfter);

                bInsert = FALSE;
                tvis.hParent = hMain;

                if(SUCCEEDED(ITypeInfo_GetRefTypeOfImplType(pTypeInfo, -1, &hRefType)))
                {
                    bInsert = TRUE;

                    ITypeInfo_ReleaseTypeAttr(pTypeInfo, pTypeAttr);
                    SysFreeString(bstrName);
                    SysFreeString(bstrData);

                    memset(wszText, 0, sizeof(wszText));
                    tld = InitializeTLData();
                    tvis.item.lParam = (LPARAM) tld;

                    ITypeInfo_GetRefTypeInfo(pTypeInfo, hRefType, &pRefTypeInfo);
                    ITypeInfo_GetDocumentation(pRefTypeInfo, MEMBERID_NIL, &bstrName,
                                &bstrData, &ulHelpContext, NULL);
                    ITypeInfo_GetTypeAttr(pRefTypeInfo, &pTypeAttr);

                    CreateInterfaceInfo(pTypeInfo, pTypeAttr->cImplTypes, bstrName,
                            bstrData, ulHelpContext, pTypeAttr, tld);
                    tld->bPredefine = TRUE;

                    AddToStrW(wszText, L"interface ");
                    AddToStrW(wszText, bstrName);
                    ITypeInfo_Release(pRefTypeInfo);
                }
                break;
            case TKIND_ALIAS:
                AddToStrW(wszText, L"typedef ");
                CreateTypeInfo(wszText, wszAfter, pTypeAttr->tdescAlias, pTypeInfo);
                AddToStrW(wszText, L" ");
                AddToStrW(wszText, bstrName);
                AddToStrW(wszText, wszAfter);

                CreateTypedefHeader(pTypeInfo, pTypeAttr, tld);
                AddToTLDataStrW(tld, &wszText[lstrlenW(L"typedef ")]);
                AddToTLDataStrW(tld, L";\n");
                break;
            default:
                lstrcpyW(wszText, bstrName);
                WINE_FIXME("pTypeAttr->typekind == %d not supported\n",
                        pTypeAttr->typekind);
        }

        if(bInsert)
        {
            hParent = TreeView_InsertItemW(typelib.hTree, &tvis);

            EnumVars(pTypeInfo, pTypeAttr->cVars, hParent);
            EnumFuncs(pTypeInfo, pTypeAttr, hParent);
            EnumImplTypes(pTypeInfo, pTypeAttr->cImplTypes, hParent);

            if(memcmp(bstrName, wszVT_UNKNOWN, sizeof(wszVT_UNKNOWN)))
                AddChildrenData(hParent, tld);
            AddToTLDataStrW(tld, tld->wszInsertAfter);
        }

        ITypeInfo_ReleaseTypeAttr(pTypeInfo, pTypeAttr);
        ITypeInfo_Release(pTypeInfo);
        SysFreeString(bstrName);
        SysFreeString(bstrData);
    }
    SendMessageW(typelib.hTree, TVM_EXPAND, TVE_EXPAND, (LPARAM)tvis.hParent);

    memset(&tvi, 0, sizeof(TVITEMW));
    tvi.mask = TVIF_PARAM;
    tvi.hItem = tvis.hParent;

    SendMessageW(typelib.hTree, TVM_GETITEMW, 0, (LPARAM)&tvi);
    AddPredefinitions(tvi.hItem, (TYPELIB_DATA*)(tvi.lParam));
    AddIdlData(tvi.hItem, (TYPELIB_DATA*)(tvi.lParam));
    AddToTLDataStrW((TYPELIB_DATA*)(tvi.lParam),
            ((TYPELIB_DATA*)(tvi.lParam))->wszInsertAfter);

    ITypeLib_Release(pTypeLib);
    return 0;
}

void UpdateData(HTREEITEM item)
{
    TVITEMW tvi;

    memset(&tvi, 0, sizeof(TVITEMW));
    tvi.mask = TVIF_PARAM;
    tvi.hItem = item;

    SendMessageW(typelib.hTree, TVM_GETITEMW, 0, (LPARAM)&tvi);
    if(!tvi.lParam)
    {
        SetWindowTextW(typelib.hEdit, L" ");
        return;
    }

    SetWindowTextW(typelib.hEdit, ((TYPELIB_DATA*)tvi.lParam)->idl);
}

static void TypeLibResizeChild(void)
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

static void TypeLibMenuCommand(WPARAM wParam, HWND hWnd)
{
    BOOL vis;

    switch(wParam)
    {
        case IDM_SAVEAS:
            GetSaveIdlAsPath();
            break;
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

static void UpdateTypeLibStatusBar(int itemID)
{
    WCHAR info[MAX_LOAD_STRING];

    if(!LoadStringW(globals.hMainInst, itemID, info, ARRAY_SIZE(info)))
        LoadStringW(globals.hMainInst, IDS_READY, info, ARRAY_SIZE(info));

    SendMessageW(typelib.hStatusBar, SB_SETTEXTW, 0, (LPARAM)info);
}

static void EmptyTLTree(void)
{
    HTREEITEM cur, del;
    TVITEMW tvi;

    tvi.mask = TVIF_PARAM;
    cur = (HTREEITEM)SendMessageW(typelib.hTree, TVM_GETNEXTITEM,
            TVGN_CHILD, (LPARAM)TVI_ROOT);

    while(TRUE)
    {
        del = cur;
        cur = (HTREEITEM)SendMessageW(typelib.hTree, TVM_GETNEXTITEM,
                TVGN_CHILD, (LPARAM)del);

        if(!cur) cur = (HTREEITEM)SendMessageW(typelib.hTree,
                TVM_GETNEXTITEM, TVGN_NEXT, (LPARAM)del);
        if(!cur) cur = (HTREEITEM)SendMessageW(typelib.hTree,
                TVM_GETNEXTITEM, TVGN_PARENT, (LPARAM)del);

        tvi.hItem = del;
        SendMessageW(typelib.hTree, TVM_GETITEMW, 0, (LPARAM)&tvi);
        if(tvi.lParam)
        {
            free(((TYPELIB_DATA*)tvi.lParam)->idl);
            free((TYPELIB_DATA*)tvi.lParam);
        }

        SendMessageW(typelib.hTree, TVM_DELETEITEM, 0, (LPARAM)del);

        if(!cur) break;
    }
}

static LRESULT CALLBACK TypeLibProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch(uMsg)
    {
        case WM_CREATE:
        {
            if(!CreatePanedWindow(hWnd, &typelib.hPaneWnd, globals.hMainInst))
                DestroyWindow(hWnd);
            typelib.hTree = CreateWindowExW(WS_EX_CLIENTEDGE, WC_TREEVIEWW, NULL,
                    WS_CHILD|WS_VISIBLE|TVS_HASLINES|TVS_HASBUTTONS|TVS_LINESATROOT,
                    0, 0, 0, 0, typelib.hPaneWnd, (HMENU)TYPELIB_TREE,
                    globals.hMainInst, NULL);
            typelib.hEdit = CreateWindowExW(WS_EX_CLIENTEDGE, WC_EDITW, NULL,
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
            break;
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
            return DefWindowProcW(hWnd, uMsg, wParam, lParam);
    }
    return 0;
}

BOOL TypeLibRegisterClassW(void)
{
    WNDCLASSW wcc;

    memset(&wcc, 0, sizeof(WNDCLASSW));
    wcc.lpfnWndProc = TypeLibProc;
    wcc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wcc.hCursor       = LoadCursorW(0, (LPCWSTR)IDC_ARROW);
    wcc.lpszMenuName = MAKEINTRESOURCEW(IDM_TYPELIB);
    wcc.lpszClassName = L"TYPELIB";

    if(!RegisterClassW(&wcc))
        return FALSE;

    return TRUE;
}

BOOL CreateTypeLibWindow(HINSTANCE hInst, WCHAR *wszFileName)
{
    WCHAR wszTitle[MAX_LOAD_STRING];
    LoadStringW(hInst, IDS_TYPELIBTITLE, wszTitle, ARRAY_SIZE(wszTitle));

    if(wszFileName) lstrcpyW(typelib.wszFileName, wszFileName);
    else
    {
        TVITEMW tvi;

        memset(&tvi, 0, sizeof(TVITEMW));
        tvi.hItem = (HTREEITEM)SendMessageW(globals.hTree, TVM_GETNEXTITEM,
                TVGN_CARET, 0);

        SendMessageW(globals.hTree, TVM_GETITEMW, 0, (LPARAM)&tvi);
        lstrcpyW(typelib.wszFileName, ((ITEM_INFO*)tvi.lParam)->path);
    }

    globals.hTypeLibWnd = CreateWindowW(L"TYPELIB", wszTitle,
            WS_OVERLAPPEDWINDOW|WS_VISIBLE,
            CW_USEDEFAULT, CW_USEDEFAULT, 800, 600, NULL, NULL, hInst, NULL);
    if(!globals.hTypeLibWnd) return FALSE;

    typelib.hStatusBar = CreateStatusWindowW(WS_VISIBLE|WS_CHILD,
            wszTitle, globals.hTypeLibWnd, 0);

    TypeLibResizeChild();
    return TRUE;
}
