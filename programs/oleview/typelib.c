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
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(oleview);

TYPELIB typelib;
static const WCHAR wszTypeLib[] = { 'T','Y','P','E','L','I','B','\0' };

static const WCHAR wszFailed[] = { '<','f','a','i','l','e','d','>','\0' };
static const WCHAR wszSpace[] = { ' ','\0' };
static const WCHAR wszAsterix[] = { '*','\0' };
static const WCHAR wszComa[] = { ',','\0' };
static const WCHAR wszEquals[] = { '=','\0' };
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
static const WCHAR wszVT_HRESULT[] = { 'H','R','E','S','U','L','T','\0' };
static const WCHAR wszVT_UNKNOWN[] = { 'I','U','n','k','n','o','w','n','\0' };
static const WCHAR wszVT_DISPATCH[] = { 'I','D','i','s','p','a','t','c','h','\0' };
static const WCHAR wszVT_DATE[] = { 'D','A','T','E','\0' };
static const WCHAR wszVT_R8[] = { 'd','o','u','b','l','e','\0' };
static const WCHAR wszVT_SAFEARRAY[] = { 'S','A','F','E','A','R','R','A','Y','\0' };

static const WCHAR wszFormat[] = { '0','x','%','.','8','l','x','\0' };
static const WCHAR wszStdCall[] = { '_','s','t','d','c','a','l','l','\0' };
static const WCHAR wszId[] = { 'i','d','\0' };
static const WCHAR wszHelpstring[] = { 'h','e','l','p','s','t','r','i','n','g','\0' };
static const WCHAR wszPropPut[] = { 'p','r','o','p','p','u','t','\0' };
static const WCHAR wszPropGet[] = { 'p','r','o','p','g','e','t','\0' };
static const WCHAR wszPropPutRef[] = { 'p','r','o','p','p','u','t','r','e','f','\0' };
static const WCHAR wszPARAMFLAG_FIN[] = { 'i','n','\0' };
static const WCHAR wszPARAMFLAG_FOUT[] = { 'o','u','t','\0' };
static const WCHAR wszPARAMFLAG_FLCID[] = { 'c','i','d','\0' };
static const WCHAR wszPARAMFLAG_FRETVAL[] = { 'r','e','t','v','a','l','\0' };
static const WCHAR wszPARAMFLAG_FOPT[] = { 'o','p','t','i','o','n','a','l','\0' };
static const WCHAR wszPARAMFLAG_FHASCUSTDATA[]
    = { 'h','a','s','c','u','s','t','d','a','t','a','\0' };
static const WCHAR wszDefaultValue[]
    = { 'd','e','f','a','u','l','t','v','a','l','u','e','\0' };

static const WCHAR wszReadOnly[] = { 'r','e','a','d','o','n','l','y','\0' };
static const WCHAR wszConst[] = { 'c','o','n','s','t','\0' };

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
    wszIdl = HeapAlloc(GetProcessHeap(), 0, len);
    WideCharToMultiByte( CP_UTF8, 0, data->idl, data->idlLen, wszIdl, len, NULL, NULL );

    if(!WriteFile(hFile, wszIdl, len, &dwNumWrite, NULL))
        ShowLastError();

    HeapFree(GetProcessHeap(), 0, wszIdl);
    CloseHandle(hFile);
}

static void GetSaveIdlAsPath(void)
{
    OPENFILENAMEW saveidl;
    WCHAR *pFileName;
    WCHAR wszPath[MAX_LOAD_STRING];
    WCHAR wszDir[MAX_LOAD_STRING];
    static const WCHAR wszDefaultExt[] = { 'i','d','l',0 };
    static const WCHAR wszIdlFiles[] = { '*','.','i','d','l','\0','\0' };

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
    saveidl.lpstrFilter = wszIdlFiles;
    saveidl.lpstrFile = wszPath;
    saveidl.nMaxFile = MAX_LOAD_STRING;
    saveidl.lpstrInitialDir = wszDir;
    saveidl.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY;
    saveidl.lpstrDefExt = wszDefaultExt;

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

    pTLData->idl = HeapReAlloc(GetProcessHeap(), 0, pTLData->idl,
            sizeof(WCHAR)*(pTLData->idlLen+SourceLen+1));

    memcpy(&pTLData->idl[pTLData->idlLen], wszSource, sizeof(WCHAR)*(SourceLen+1));
    pTLData->idlLen += SourceLen;
}

static void AddToTLDataStrWithTabsW(TYPELIB_DATA *pTLData, WCHAR *wszSource)
{
    int lineLen = lstrlenW(wszSource);
    int newLinesNo = 0;
    WCHAR *pSourcePos = wszSource;
    WCHAR *pSourceBeg;

    if(!lineLen) return;
    while(*pSourcePos)
    {
        if(*pSourcePos == *wszNewLine) newLinesNo++;
        pSourcePos += 1;
    }
    if(*(pSourcePos - 1) != *wszNewLine) newLinesNo++;

    pTLData->idl = HeapReAlloc(GetProcessHeap(), 0, pTLData->idl,
            sizeof(WCHAR)*(pTLData->idlLen+lineLen+4*newLinesNo+1));

    pSourcePos = wszSource;
    pSourceBeg = wszSource;
    while(newLinesNo)
    {
        if(*pSourcePos != *wszNewLine && *pSourcePos)
        {
            pSourcePos += 1;
            continue;
        }
        newLinesNo--;

        if(*pSourcePos)
        {
            *pSourcePos = '\0';
            lineLen = lstrlenW(pSourceBeg)+1;
            *pSourcePos = '\n';
            pSourcePos += 1;
        }
        else lineLen = lstrlenW(pSourceBeg);

        pTLData->idl[pTLData->idlLen] = *wszSpace;
        pTLData->idl[pTLData->idlLen+1] = *wszSpace;
        pTLData->idl[pTLData->idlLen+2] = *wszSpace;
        pTLData->idl[pTLData->idlLen+3] = *wszSpace;
        memcpy(&pTLData->idl[pTLData->idlLen+4], pSourceBeg, sizeof(WCHAR)*lineLen);
        pTLData->idlLen += lineLen + 4;
        pTLData->idl[pTLData->idlLen] = '\0';

        pSourceBeg = pSourcePos;
    }
}

static TYPELIB_DATA *InitializeTLData(void)
{
    TYPELIB_DATA *pTLData;

    pTLData = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(TYPELIB_DATA));

    pTLData->idl = HeapAlloc(GetProcessHeap(), 0, sizeof(WCHAR));
    pTLData->idl[0] = '\0';

    return pTLData;
}

static void AddSpaces(TYPELIB_DATA *pTLData, int tabSize)
{
    for(; tabSize>0; tabSize--)
        AddToTLDataStrW(pTLData, wszSpace);
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
        VTADDTOSTR(VT_HRESULT);
        VTADDTOSTR(VT_UNKNOWN);
        VTADDTOSTR(VT_DISPATCH);
        VTADDTOSTR(VT_DATE);
        VTADDTOSTR(VT_R8);
        case VT_CARRAY:
        for(i=0; i<U(tdesc).lpadesc->cDims; i++)
        {
            wsprintfW(wszBuf, wszFormat, U(tdesc).lpadesc->rgbounds[i].cElements);
            AddToStrW(wszAddAfter, wszBuf);
        }
        CreateTypeInfo(wszAddTo, wszAddAfter, U(tdesc).lpadesc->tdescElem, pTypeInfo);
        break;
	case VT_SAFEARRAY:
	AddToStrW(wszAddTo, wszVT_SAFEARRAY);
	AddToStrW(wszAddTo, wszOpenBrackets2);
        CreateTypeInfo(wszAddTo, wszAddAfter, *U(tdesc).lptdesc, pTypeInfo);
        AddToStrW(wszAddTo, wszCloseBrackets2);
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

static int EnumVars(ITypeInfo *pTypeInfo, int cVars, HTREEITEM hParent)
{
    int i;
    TVINSERTSTRUCTW tvis;
    VARDESC *pVarDesc;
    BSTR bstrName;
    WCHAR wszText[MAX_LOAD_STRING];
    WCHAR wszAfter[MAX_LOAD_STRING];

    U(tvis).item.mask = TVIF_TEXT|TVIF_PARAM;
    U(tvis).item.cchTextMax = MAX_LOAD_STRING;
    U(tvis).item.pszText = wszText;
    tvis.hInsertAfter = TVI_LAST;
    tvis.hParent = hParent;

    for(i=0; i<cVars; i++)
    {
        TYPELIB_DATA *tld;

        if(FAILED(ITypeInfo_GetVarDesc(pTypeInfo, i, &pVarDesc))) continue;
        if(FAILED(ITypeInfo_GetDocumentation(pTypeInfo, pVarDesc->memid, &bstrName,
                NULL, NULL, NULL))) continue;

        tld = InitializeTLData();
        U(tvis).item.lParam = (LPARAM) tld;
        if(pVarDesc->memid < MIN_VAR_ID)
        {

            AddToTLDataStrW(tld, wszOpenBrackets1);
            AddToTLDataStrW(tld, wszId);
            AddToTLDataStrW(tld, wszOpenBrackets2);
            wsprintfW(wszText, wszFormat, pVarDesc->memid);
            AddToTLDataStrW(tld, wszText);
            memset(wszText, 0, sizeof(wszText));
            AddToTLDataStrW(tld, wszCloseBrackets2);

            if(pVarDesc->wVarFlags & VARFLAG_FREADONLY)
            {
                AddToTLDataStrW(tld, wszComa);
                AddToTLDataStrW(tld, wszSpace);
                AddToTLDataStrW(tld, wszReadOnly);
            }
            AddToTLDataStrW(tld, wszCloseBrackets1);
            AddToTLDataStrW(tld, wszNewLine);
        }

        memset(wszText, 0, sizeof(wszText));
        memset(wszAfter, 0, sizeof(wszAfter));
        CreateTypeInfo(wszText, wszAfter, pVarDesc->elemdescVar.tdesc, pTypeInfo);
        AddToStrW(wszText, wszSpace);
        if (bstrName) AddToStrW(wszText, bstrName);
        AddToStrW(wszText, wszAfter);
        AddToTLDataStrW(tld, wszText);
        AddToTLDataStrW(tld, wszSemicolon);
        AddToTLDataStrW(tld, wszNewLine);

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

    U(tvis).item.mask = TVIF_TEXT|TVIF_PARAM;
    U(tvis).item.cchTextMax = MAX_LOAD_STRING;
    U(tvis).item.pszText = wszText;
    tvis.hInsertAfter = TVI_LAST;
    tvis.hParent = hParent;

    for(i=0; i<cVars; i++)
    {
        TYPELIB_DATA *tld;

        if(FAILED(ITypeInfo_GetVarDesc(pTypeInfo, i, &pVarDesc))) continue;
        if(FAILED(ITypeInfo_GetDocumentation(pTypeInfo, pVarDesc->memid, &bstrName,
                NULL, NULL, NULL))) continue;

        tld = InitializeTLData();
        U(tvis).item.lParam = (LPARAM) tld;

        memset(wszText, 0, sizeof(wszText));
        memset(wszAfter, 0, sizeof(wszAfter));

        if (pVarDesc->varkind == VAR_CONST)
        {
            VARIANT var;
            VariantInit(&var);
            if (VariantChangeType(&var, U(*pVarDesc).lpvarValue, 0, VT_BSTR) == S_OK)
            {
                AddToStrW(wszText, wszConst);
                AddToStrW(wszText, wszSpace);
                AddToStrW(wszAfter, wszSpace);
                AddToStrW(wszAfter, wszEquals);
                AddToStrW(wszAfter, wszSpace);
                AddToStrW(wszAfter, V_BSTR(&var));
            }
        }

        CreateTypeInfo(wszText, wszAfter, pVarDesc->elemdescVar.tdesc, pTypeInfo);
        AddToStrW(wszText, wszSpace);
        AddToStrW(wszText, bstrName);
        AddToStrW(wszText, wszAfter);
        AddToTLDataStrW(tld, bstrName);
        AddToTLDataStrW(tld, wszAfter);
	if (i<cVars-1)
            AddToTLDataStrW(tld, wszComa);
        AddToTLDataStrW(tld, wszNewLine);

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
    WCHAR szRhs[] = {'r','h','s',0};    /* Right-hand side of a propput */
    BOOL bFirst;

    U(tvis).item.mask = TVIF_TEXT|TVIF_PARAM;
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
        U(tvis).item.cchTextMax = SysStringLen(bstrName);
        U(tvis).item.pszText = bstrName;
        U(tvis).item.lParam = (LPARAM) tld;
        bFirst = TRUE;
        if(pFuncDesc->memid < MIN_FUNC_ID || pTypeAttr->wTypeFlags & TYPEFLAG_FDUAL)
        {
            AddToTLDataStrW(tld, wszOpenBrackets1);
            bFirst = FALSE;
            AddToTLDataStrW(tld, wszId);
            AddToTLDataStrW(tld, wszOpenBrackets2);
            wsprintfW(wszText, wszFormat, pFuncDesc->memid);
            AddToTLDataStrW(tld, wszText);
            AddToTLDataStrW(tld, wszCloseBrackets2);
            memset(wszText, 0, sizeof(wszText));
        }

        CreateTypeInfo(wszText, wszAfter, pFuncDesc->elemdescFunc.tdesc, pTypeInfo);
        switch(pFuncDesc->invkind)
        {
            case INVOKE_PROPERTYGET:
                if(bFirst) AddToTLDataStrW(tld, wszOpenBrackets1);
                else
                {
                    AddToTLDataStrW(tld, wszComa);
                    AddToTLDataStrW(tld, wszSpace);
                }
                bFirst = FALSE;
                AddToTLDataStrW(tld, wszPropGet);
                break;
            case INVOKE_PROPERTYPUT:
                if(bFirst) AddToTLDataStrW(tld, wszOpenBrackets1);
                else
                {
                    AddToTLDataStrW(tld, wszComa);
                    AddToTLDataStrW(tld, wszSpace);
                }
                bFirst = FALSE;
                AddToTLDataStrW(tld, wszPropPut);
                break;
            case INVOKE_PROPERTYPUTREF:
                if(bFirst) AddToTLDataStrW(tld, wszOpenBrackets1);
                else
                {
                    AddToTLDataStrW(tld, wszComa);
                    AddToTLDataStrW(tld, wszSpace);
                }
                bFirst = FALSE;
                AddToTLDataStrW(tld, wszPropPutRef);
                break;
            default:;
        }
        if(SysStringLen(bstrHelpString))
        {
            if(bFirst) AddToTLDataStrW(tld, wszOpenBrackets1);
            else
            {
                AddToTLDataStrW(tld, wszComa);
                AddToTLDataStrW(tld, wszSpace);
            }
            bFirst = FALSE;
            AddToTLDataStrW(tld, wszHelpstring);
            AddToTLDataStrW(tld, wszOpenBrackets2);
            AddToTLDataStrW(tld, wszInvertedComa);
            AddToTLDataStrW(tld, bstrHelpString);
            AddToTLDataStrW(tld, wszInvertedComa);
            AddToTLDataStrW(tld, wszCloseBrackets2);
        }
        if(!bFirst)
        {
            AddToTLDataStrW(tld, wszCloseBrackets1);
            AddToTLDataStrW(tld, wszNewLine);
        }

        if(pTypeAttr->wTypeFlags & TYPEFLAG_FOLEAUTOMATION) {
            AddToTLDataStrW(tld, wszVT_HRESULT);
            if(strcmpW(wszText, wszVT_VOID)) pFuncDesc->cParams++;
        }
        else {
            AddToTLDataStrW(tld, wszText);
            AddToTLDataStrW(tld, wszAfter);
        }

        bstrParamNames = HeapAlloc(GetProcessHeap(), 0,
                sizeof(BSTR)*(pFuncDesc->cParams+1));
        if(FAILED(ITypeInfo_GetNames(pTypeInfo, pFuncDesc->memid, bstrParamNames,
                pFuncDesc->cParams+1, &namesNo)))
        {
            HeapFree(GetProcessHeap(), 0, bstrParamNames);
            continue;
        }
        SysFreeString(bstrParamNames[0]);

        AddToTLDataStrW(tld, wszSpace);
        if(pFuncDesc->memid >= MIN_FUNC_ID)
        {
            AddToTLDataStrW(tld, wszStdCall);
            AddToTLDataStrW(tld, wszSpace);
        }
        if (bstrName) AddToTLDataStrW(tld, bstrName);
        AddToTLDataStrW(tld, wszOpenBrackets2);

        for(j=0; j<pFuncDesc->cParams; j++)
        {
            if(j != 0) AddToTLDataStrW(tld, wszComa);
            if(pFuncDesc->cParams != 1)
            {
                AddToTLDataStrW(tld, wszNewLine);
                AddSpaces(tld, TAB_SIZE);
            }
            bFirst = TRUE;
#define ENUM_PARAM_FLAG(x)\
            if(U(pFuncDesc->lprgelemdescParam[j]).paramdesc.wParamFlags & x) \
            {\
                if(bFirst) AddToTLDataStrW(tld,\
                        wszOpenBrackets1);\
                else\
                {\
                    AddToTLDataStrW(tld, wszComa);\
                    AddToTLDataStrW(tld, wszSpace);\
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

            if(U(pFuncDesc->lprgelemdescParam[j]).paramdesc.wParamFlags & PARAMFLAG_FHASDEFAULT)
            {
		VARIANT var, *param=&U(pFuncDesc->lprgelemdescParam[j]).paramdesc.pparamdescex->varDefaultValue;
		VariantInit(&var);
                if(bFirst) AddToTLDataStrW(tld,
                        wszOpenBrackets1);
                else
                {
                    AddToTLDataStrW(tld, wszComa);
                    AddToTLDataStrW(tld, wszSpace);
                }
                bFirst = FALSE;
                AddToTLDataStrW(tld, wszDefaultValue);
                AddToTLDataStrW(tld, wszOpenBrackets2);
		if (V_VT(param) == VT_BSTR)
		{
		    AddToTLDataStrW(tld, wszInvertedComa);
		    AddToTLDataStrW(tld, V_BSTR(param));
		    AddToTLDataStrW(tld, wszInvertedComa);
		} else if (VariantChangeType(&var, param, 0, VT_BSTR) == S_OK)
		    AddToTLDataStrW(tld, V_BSTR(&var));
                AddToTLDataStrW(tld, wszCloseBrackets2);
            }

            if(!bFirst)
            {
                AddToTLDataStrW(tld, wszCloseBrackets1);
                AddToTLDataStrW(tld, wszSpace);
            }

            memset(wszText, 0, sizeof(wszText));
            memset(wszAfter, 0, sizeof(wszAfter));
            CreateTypeInfo(wszText, wszAfter, pFuncDesc->lprgelemdescParam[j].tdesc,
                    pTypeInfo);
            AddToTLDataStrW(tld, wszText);
            AddToTLDataStrW(tld, wszAfter);
            AddToTLDataStrW(tld, wszSpace);
            if (j+1 < namesNo) {
                if (bstrParamNames[j+1])
                {
                    AddToTLDataStrW(tld, bstrParamNames[j+1]);
                    SysFreeString(bstrParamNames[j+1]);
                }
            } else {
                AddToTLDataStrW(tld, szRhs);
            }
        }
        AddToTLDataStrW(tld, wszCloseBrackets2);
        AddToTLDataStrW(tld, wszSemicolon);
        AddToTLDataStrW(tld, wszNewLine);

        SendMessageW(typelib.hTree, TVM_INSERTITEMW, 0, (LPARAM)&tvis);
        HeapFree(GetProcessHeap(), 0, bstrParamNames);
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

    U(tvis).item.mask = TVIF_TEXT;
    U(tvis).item.cchTextMax = MAX_LOAD_STRING;
    U(tvis).item.pszText = wszInheritedInterfaces;
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

        U(tvis).item.cchTextMax = SysStringLen(bstrName);
        U(tvis).item.pszText = bstrName;

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
    const WCHAR wszTKIND_INTERFACE[] = { 'i','n','t','e','r','f','a','c','e',' ','\0' };
    const WCHAR wszTKIND_DISPATCH[]
        = { 'd','i','s','p','i','n','t','e','r','f','a','c','e',' ','\0' };
    const WCHAR wszIMPLTYPEFLAG_FDEFAULT[]
        = { 'd','e','f','a','u','l','t','\0' };
    const WCHAR wszIMPLTYPEFLAG_FSOURCE[]
        = { 's','o','u','r','c','e','\0' };
    const WCHAR wszIMPLTYPEFLAG_FRESTRICTED[]
        = { 'r','e','s','t','r','i','c','t','e','d','\0' };

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
            if(bFirst) AddToTLDataStrW(pTLData,\
                    wszOpenBrackets1);\
            else\
            {\
                AddToTLDataStrW(pTLData, wszComa);\
                AddToTLDataStrW(pTLData, wszSpace);\
            }\
            bFirst = FALSE;\
            AddToTLDataStrW(pTLData, wsz##x);\
        }
        ENUM_IMPLTYPEFLAG(IMPLTYPEFLAG_FDEFAULT);
        ENUM_IMPLTYPEFLAG(IMPLTYPEFLAG_FSOURCE);
        ENUM_IMPLTYPEFLAG(IMPLTYPEFLAG_FRESTRICTED);
        if(!bFirst)
        {
            AddToTLDataStrW(pTLData, wszCloseBrackets1);
            AddToTLDataStrW(pTLData, wszSpace);
        }

        if(pTypeAttr->typekind == TKIND_INTERFACE ||
                (pTypeAttr->wTypeFlags & TYPEFLAG_FDUAL))
            AddToTLDataStrW(pTLData, wszTKIND_INTERFACE);
        else if(pTypeAttr->typekind == TKIND_DISPATCH)
            AddToTLDataStrW(pTLData, wszTKIND_DISPATCH);
        AddToTLDataStrW(pTLData, wszSpace);

        AddToTLDataStrW(pTLData, bstrName);
        AddToTLDataStrW(pTLData, wszSemicolon);
        AddToTLDataStrW(pTLData, wszNewLine);

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
            AddToTLDataStrW(pTLData, wszNewLine);
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
    WCHAR wszPredefinition[] = { '/','/',' ','T','L','i','b',' ',':','\n',
        '/','/',' ','F','o','r','w','a','r','d',' ','d','e','c','l','a','r','e',' ',
        'a','l','l',' ','t','y','p','e','s',' ','d','e','f','i','n','e','d',' ',
        'i','n',' ','t','h','i','s',' ','t','y','p','e','l','i','b','\0' };

    hFirst = (HTREEITEM)SendMessageW(typelib.hTree, TVM_GETNEXTITEM,
            TVGN_CHILD, (LPARAM)hFirst);

    AddToTLDataStrWithTabsW(pTLData, wszPredefinition);
    AddToTLDataStrW(pTLData, wszNewLine);

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
            AddToStrW(wszText, wszSemicolon);
            AddToTLDataStrWithTabsW(pTLData, wszText);
            AddToTLDataStrW(pTLData, wszNewLine);
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

    const WCHAR wszInterface[] = { 'i','n','t','e','r','f','a','c','e',' ','\0' };
    const WCHAR wszDispinterface[]
        = { 'd','i','s','p','i','n','t','e','r','f','a','c','e',' ','\0' };
    const WCHAR wszHelpcontext[] = { 'h','e','l','p','c','o','n','t','e','x','t','\0' };
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
    const WCHAR wszTYPEFLAG_FREVERSEBIND[]
        = { 'r','e','v','e','r','s','e','b','i','n','d','\0' };
    const WCHAR wszTYPEFLAG_FPROXY[] = { 'p','r','o','x','y','\0' };

    AddToTLDataStrW(pTLData, wszOpenBrackets1);
    AddToTLDataStrW(pTLData, wszNewLine);
    if(pTypeAttr->typekind != TKIND_DISPATCH)
    {
        AddSpaces(pTLData, TAB_SIZE);
        AddToTLDataStrW(pTLData, wszOdl);
        AddToTLDataStrW(pTLData, wszComa);
        AddToTLDataStrW(pTLData, wszNewLine);
    }
    AddSpaces(pTLData, TAB_SIZE);
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
        AddSpaces(pTLData, TAB_SIZE);
        AddToTLDataStrW(pTLData, wszHelpstring);
        AddToTLDataStrW(pTLData, wszOpenBrackets2);
        AddToTLDataStrW(pTLData, wszInvertedComa);
        AddToTLDataStrW(pTLData, wszHelpString);
        AddToTLDataStrW(pTLData, wszInvertedComa);
        AddToTLDataStrW(pTLData, wszCloseBrackets2);
    }
    if(ulHelpContext)
    {
        AddToTLDataStrW(pTLData, wszComa);
        AddToTLDataStrW(pTLData, wszNewLine);
        AddSpaces(pTLData, TAB_SIZE);
        AddToTLDataStrW(pTLData, wszHelpcontext);
        AddToTLDataStrW(pTLData, wszOpenBrackets2);
        wsprintfW(wszHelpContext, wszFormat, ulHelpContext);
        AddToTLDataStrW(pTLData, wszHelpContext);
        AddToTLDataStrW(pTLData, wszCloseBrackets2);
    }
    if(pTypeAttr->wTypeFlags)
    {
#define ENUM_FLAGS(x) if(pTypeAttr->wTypeFlags & x)\
        {\
            AddToTLDataStrW(pTLData, wszComa);\
            AddToTLDataStrW(pTLData, wszNewLine);\
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
    AddToTLDataStrW(pTLData, wszNewLine);
    AddToTLDataStrW(pTLData, wszCloseBrackets1);
    AddToTLDataStrW(pTLData, wszNewLine);
    if(pTypeAttr->typekind != TKIND_DISPATCH) AddToTLDataStrW(pTLData, wszInterface);
    else AddToTLDataStrW(pTLData, wszDispinterface);
    AddToTLDataStrW(pTLData, wszName);
    AddToTLDataStrW(pTLData, wszSpace);
    if(cImplTypes && pTypeAttr->typekind != TKIND_DISPATCH)
    {
        AddToTLDataStrW(pTLData, wszColon);
        AddToTLDataStrW(pTLData, wszSpace);

        ITypeInfo_GetRefTypeOfImplType(pTypeInfo, 0, &hRefType);
        if (SUCCEEDED(ITypeInfo_GetRefTypeInfo(pTypeInfo, hRefType, &pRefTypeInfo)))
        {
            ITypeInfo_GetDocumentation(pRefTypeInfo, MEMBERID_NIL, &bstrName,
                NULL, NULL, NULL);
            AddToTLDataStrW(pTLData, bstrName);
            AddToTLDataStrW(pTLData, wszSpace);

            SysFreeString(bstrName);
            ITypeInfo_Release(pRefTypeInfo);
        }
        else
            AddToTLDataStrW(pTLData, wszFailed);
    }
    AddToTLDataStrW(pTLData, wszOpenBrackets3);
    AddToTLDataStrW(pTLData, wszNewLine);

    AddToStrW(pTLData->wszInsertAfter, wszCloseBrackets3);
    AddToStrW(pTLData->wszInsertAfter, wszSemicolon);
    AddToStrW(pTLData->wszInsertAfter, wszNewLine);
}

static void CreateTypedefHeader(ITypeInfo *pTypeInfo,
        TYPEATTR *pTypeAttr, TYPELIB_DATA *pTLData)
{
    BOOL bFirst = TRUE;
    WCHAR wszGuid[MAX_LOAD_STRING];
    const WCHAR wszTypedef[] = { 't','y','p','e','d','e','f',' ','\0' };
    const WCHAR wszPublic[] = { 'p','u','b','l','i','c','\0' };

    AddToTLDataStrW(pTLData, wszTypedef);
    if(memcmp(&pTypeAttr->guid, &GUID_NULL, sizeof(GUID)))
    {
        AddToTLDataStrW(pTLData, wszOpenBrackets1);
        bFirst = FALSE;
        AddToTLDataStrW(pTLData, wszUUID);
        AddToTLDataStrW(pTLData, wszOpenBrackets2);
        StringFromGUID2(&(pTypeAttr->guid), wszGuid, MAX_LOAD_STRING);
        wszGuid[lstrlenW(wszGuid)-1] = '\0';
        AddToTLDataStrW(pTLData, &wszGuid[1]);
        AddToTLDataStrW(pTLData, wszCloseBrackets2);
    }
    if(pTypeAttr->typekind == TKIND_ALIAS)
    {
        if(bFirst) AddToTLDataStrW(pTLData, wszOpenBrackets1);
        else
        {
            AddToTLDataStrW(pTLData, wszComa);
            AddToTLDataStrW(pTLData, wszSpace);
        }
        bFirst = FALSE;
        AddToTLDataStrW(pTLData, wszPublic);
    }
    if(!bFirst)
    {
        AddToTLDataStrW(pTLData, wszCloseBrackets1);
        AddToTLDataStrW(pTLData, wszNewLine);
    }
}

static void CreateCoclassHeader(ITypeInfo *pTypeInfo,
        TYPEATTR *pTypeAttr, TYPELIB_DATA *pTLData)
{
    WCHAR wszGuid[MAX_LOAD_STRING];
    BSTR bstrHelpString;
    const WCHAR wszNoncreatable[]
        = { 'n','o','n','c','r','e','a','t','a','b','l','e','\0' };

    AddToTLDataStrW(pTLData, wszOpenBrackets1);
    AddToTLDataStrW(pTLData, wszNewLine);

    AddSpaces(pTLData, TAB_SIZE);
    AddToTLDataStrW(pTLData, wszUUID);
    AddToTLDataStrW(pTLData, wszOpenBrackets2);
    StringFromGUID2(&(pTypeAttr->guid), wszGuid, MAX_LOAD_STRING);
    wszGuid[lstrlenW(wszGuid)-1] = '\0';
    AddToTLDataStrW(pTLData, &wszGuid[1]);
    AddToTLDataStrW(pTLData, wszCloseBrackets2);

    if(SUCCEEDED(ITypeInfo_GetDocumentation(pTypeInfo, MEMBERID_NIL, NULL,
            &bstrHelpString, NULL, NULL)))
    {
        if(SysStringLen(bstrHelpString))
        {
            AddToTLDataStrW(pTLData, wszComa);
            AddToTLDataStrW(pTLData, wszNewLine);
            AddSpaces(pTLData, TAB_SIZE);
            AddToTLDataStrW(pTLData, wszHelpstring);
            AddToTLDataStrW(pTLData, wszOpenBrackets2);
            AddToTLDataStrW(pTLData, wszInvertedComa);
            AddToTLDataStrW(pTLData, bstrHelpString);
            AddToTLDataStrW(pTLData, wszInvertedComa);
            AddToTLDataStrW(pTLData, wszCloseBrackets2);
        }
        SysFreeString(bstrHelpString);
    }

    if(!(pTypeAttr->wTypeFlags & TYPEFLAG_FCANCREATE))
    {
        AddToTLDataStrW(pTLData, wszComa);
        AddToTLDataStrW(pTLData, wszNewLine);
        AddSpaces(pTLData, TAB_SIZE);
        AddToTLDataStrW(pTLData, wszNoncreatable);
    }

    AddToTLDataStrW(pTLData, wszNewLine);
    AddToTLDataStrW(pTLData, wszCloseBrackets1);
    AddToTLDataStrW(pTLData, wszNewLine);
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

    const WCHAR wszLibrary[] = { 'l','i','b','r','a','r','y',' ','\0' };
    const WCHAR wszTag[] = { 't','a','g','\0' };

    WCHAR wszProperties[] = { 'p','r','o','p','e','r','t','i','e','s','\0' };
    WCHAR wszMethods[] = { 'm','e','t','h','o','d','s','\0' };

    U(tvis).item.mask = TVIF_TEXT|TVIF_PARAM;
    U(tvis).item.cchTextMax = MAX_LOAD_STRING;
    U(tvis).item.pszText = wszText;
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
                       wszError, 0, 0, wszMessage, ARRAY_SIZE(wszMessage), (__ms_va_list*)args);
        MessageBoxW(globals.hMainWnd, wszMessage, NULL, MB_OK|MB_ICONEXCLAMATION);
        return 1;
    }
    count = ITypeLib_GetTypeInfoCount(pTypeLib);

    ITypeLib_GetDocumentation(pTypeLib, -1, &bstrName, &bstrData, NULL, NULL);
    ITypeLib_GetLibAttr(pTypeLib, &pTLibAttr);

    tld = InitializeTLData();
    U(tvis).item.lParam = (LPARAM) tld;
    AddToTLDataStrW(tld, wszGeneratedInfo);
    AddToTLDataStrW(tld, typelib.wszFileName);
    AddToTLDataStrW(tld, wszNewLine);
    AddToTLDataStrW(tld, wszNewLine);
    AddToTLDataStrW(tld, wszOpenBrackets1);
    AddToTLDataStrW(tld, wszNewLine);
    AddSpaces(tld, TAB_SIZE);
    AddToTLDataStrW(tld, wszUUID);
    AddToTLDataStrW(tld, wszOpenBrackets2);
    StringFromGUID2(&(pTLibAttr->guid), wszText, MAX_LOAD_STRING);
    wszText[lstrlenW(wszText)-1] = '\0';
    AddToTLDataStrW(tld, &wszText[1]);
    AddToTLDataStrW(tld, wszCloseBrackets2);
    AddToTLDataStrW(tld, wszComa);
    AddToTLDataStrW(tld, wszNewLine);
    AddSpaces(tld, TAB_SIZE);
    wsprintfW(wszText, wszFormat2, pTLibAttr->wMajorVerNum, pTLibAttr->wMinorVerNum);
    AddToTLDataStrW(tld, wszText);

    if (bstrData)
    {
        /* helpstring is optional */
        AddToTLDataStrW(tld, wszComa);
        AddToTLDataStrW(tld, wszNewLine);
        AddSpaces(tld, TAB_SIZE);
        AddToTLDataStrW(tld, wszHelpstring);
        AddToTLDataStrW(tld, wszOpenBrackets2);
        AddToTLDataStrW(tld, wszInvertedComa);
        AddToTLDataStrW(tld, bstrData);
        AddToTLDataStrW(tld, wszInvertedComa);
        AddToTLDataStrW(tld, wszCloseBrackets2);
    }

    AddToTLDataStrW(tld, wszNewLine);
    AddToTLDataStrW(tld, wszCloseBrackets1);
    AddToTLDataStrW(tld, wszNewLine);
    AddToTLDataStrW(tld, wszLibrary);
    if (bstrName) AddToTLDataStrW(tld, bstrName);
    AddToTLDataStrW(tld, wszNewLine);
    AddToTLDataStrW(tld, wszOpenBrackets3);
    AddToTLDataStrW(tld, wszNewLine);

    AddToStrW(tld->wszInsertAfter, wszCloseBrackets3);
    AddToStrW(tld->wszInsertAfter, wszSemicolon);

    wsprintfW(wszText, wszFormat, bstrName, bstrData);
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
        U(tvis).item.lParam = (LPARAM)tld;
        switch(pTypeAttr->typekind)
        {
            case TKIND_ENUM:
                AddToStrW(wszText, wszTKIND_ENUM);
                AddToStrW(wszText, bstrName);

                CreateTypedefHeader(pTypeInfo, pTypeAttr, tld);
                AddToTLDataStrW(tld, &wszTKIND_ENUM[lstrlenW(wszTKIND_ALIAS)]);
                AddToTLDataStrW(tld, wszOpenBrackets3);
                AddToTLDataStrW(tld,wszNewLine);
                AddToStrW(tld->wszInsertAfter, wszCloseBrackets3);
                AddToStrW(tld->wszInsertAfter, wszSpace);
                AddToStrW(tld->wszInsertAfter, bstrName);
                AddToStrW(tld->wszInsertAfter, wszSemicolon);
                AddToStrW(tld->wszInsertAfter, wszNewLine);

                bInsert = FALSE;
                hParent = TreeView_InsertItemW(typelib.hTree, &tvis);
                EnumEnums(pTypeInfo, pTypeAttr->cVars, hParent);
                AddChildrenData(hParent, tld);
                AddToTLDataStrW(tld, tld->wszInsertAfter);
                break;
            case TKIND_RECORD:
                AddToTLDataStrW(tld, wszTKIND_RECORD);
                AddToTLDataStrW(tld, wszTag);
                AddToTLDataStrW(tld, bstrName);
                AddToTLDataStrW(tld, wszSpace);
                AddToTLDataStrW(tld, wszOpenBrackets3);
                AddToTLDataStrW(tld, wszNewLine);

                AddToStrW(tld->wszInsertAfter, wszCloseBrackets3);
                AddToStrW(tld->wszInsertAfter, wszSpace);
                AddToStrW(tld->wszInsertAfter, bstrName);
                AddToStrW(tld->wszInsertAfter, wszSemicolon);
                AddToStrW(tld->wszInsertAfter, wszNewLine);

                AddToStrW(wszText, wszTKIND_RECORD);
                AddToStrW(wszText, bstrName);
                break;
            case TKIND_MODULE:
                AddToStrW(wszText, wszTKIND_MODULE);
                AddToStrW(wszText, bstrName);
                break;
            case TKIND_INTERFACE:
                CreateInterfaceInfo(pTypeInfo, pTypeAttr->cImplTypes, bstrName,
                        bstrData, ulHelpContext, pTypeAttr, tld);
                tld->bPredefine = TRUE;

                AddToStrW(wszText, wszTKIND_INTERFACE);
                AddToStrW(wszText, bstrName);
                break;
            case TKIND_COCLASS:
                AddToStrW(wszText, wszTKIND_COCLASS);
                AddToStrW(wszText, bstrName);

                CreateCoclassHeader(pTypeInfo, pTypeAttr, tld);
                AddToTLDataStrW(tld, wszTKIND_COCLASS);
                AddToTLDataStrW(tld, bstrName);
                AddToTLDataStrW(tld, wszSpace);
                AddToTLDataStrW(tld, wszOpenBrackets3);
                AddToTLDataStrW(tld, wszNewLine);

                EnumCoclassImplTypes(pTypeInfo, pTypeAttr->cImplTypes, tld);

                AddToStrW(tld->wszInsertAfter, wszCloseBrackets3);
                AddToStrW(tld->wszInsertAfter, wszSemicolon);
                AddToStrW(tld->wszInsertAfter, wszNewLine);

                bInsert = FALSE;
                hParent = TreeView_InsertItemW(typelib.hTree, &tvis);
                AddToTLDataStrW(tld, tld->wszInsertAfter);
                break;
            case TKIND_UNION:
                AddToStrW(wszText, wszTKIND_UNION);
                AddToStrW(wszText, bstrName);
                break;
            case TKIND_DISPATCH:
                CreateInterfaceInfo(pTypeInfo, pTypeAttr->cImplTypes, bstrName,
                        bstrData, ulHelpContext, pTypeAttr, tld);
                tld->bPredefine = TRUE;
                if(pTypeAttr->wTypeFlags & TYPEFLAG_FDUAL)
                    tld->bHide = TRUE;
                AddToStrW(wszText, wszTKIND_DISPATCH);
                AddToStrW(wszText, bstrName);

                hParent = TreeView_InsertItemW(typelib.hTree, &tvis);
                hMain = tvis.hParent;
                tldDispatch = tld;

                lstrcpyW(wszText, wszProperties);
                tvis.hParent = hParent;
                tld = InitializeTLData();
                U(tvis).item.lParam = (LPARAM) tld;
                AddToTLDataStrW(tld, wszProperties);
                AddToTLDataStrW(tld, wszColon);
                AddToTLDataStrW(tld, wszNewLine);
                tvis.hParent = TreeView_InsertItemW(typelib.hTree, &tvis);
                EnumVars(pTypeInfo, pTypeAttr->cVars, tvis.hParent);
                AddChildrenData(tvis.hParent, tld);

                lstrcpyW(wszText, wszMethods);
                tvis.hParent = hParent;
                tld = InitializeTLData();
                U(tvis).item.lParam = (LPARAM) tld;
                AddToTLDataStrW(tld, wszMethods);
                AddToTLDataStrW(tld, wszColon);
                AddToTLDataStrW(tld, wszNewLine);
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
                    U(tvis).item.lParam = (LPARAM) tld;

                    ITypeInfo_GetRefTypeInfo(pTypeInfo, hRefType, &pRefTypeInfo);
                    ITypeInfo_GetDocumentation(pRefTypeInfo, MEMBERID_NIL, &bstrName,
                                &bstrData, &ulHelpContext, NULL);
                    ITypeInfo_GetTypeAttr(pRefTypeInfo, &pTypeAttr);

                    CreateInterfaceInfo(pTypeInfo, pTypeAttr->cImplTypes, bstrName,
                            bstrData, ulHelpContext, pTypeAttr, tld);
                    tld->bPredefine = TRUE;

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

                CreateTypedefHeader(pTypeInfo, pTypeAttr, tld);
                AddToTLDataStrW(tld, &wszText[lstrlenW(wszTKIND_ALIAS)]);
                AddToTLDataStrW(tld, wszSemicolon);
                AddToTLDataStrW(tld, wszNewLine);
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
        SetWindowTextW(typelib.hEdit, wszSpace);
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
            HeapFree(GetProcessHeap(), 0, ((TYPELIB_DATA *)tvi.lParam)->idl);
            HeapFree(GetProcessHeap(), 0, (TYPELIB_DATA *)tvi.lParam);
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
    wcc.lpszClassName = wszTypeLib;

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

    globals.hTypeLibWnd = CreateWindowW(wszTypeLib, wszTitle,
            WS_OVERLAPPEDWINDOW|WS_VISIBLE,
            CW_USEDEFAULT, CW_USEDEFAULT, 800, 600, NULL, NULL, hInst, NULL);
    if(!globals.hTypeLibWnd) return FALSE;

    typelib.hStatusBar = CreateStatusWindowW(WS_VISIBLE|WS_CHILD,
            wszTitle, globals.hTypeLibWnd, 0);

    TypeLibResizeChild();
    return TRUE;
}
