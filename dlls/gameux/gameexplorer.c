/*
 *    Gameux library coclass GameExplorer implementation
 *
 * Copyright (C) 2010 Mariusz Pluciński
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
#define COBJMACROS

#include "config.h"

#include "ole2.h"
#include "sddl.h"
#include "xmldom.h"

#include "gameux.h"
#include "gameux_private.h"

#include "initguid.h"
#include "msxml2.h"

#include "wine/debug.h"
#include "winreg.h"

WINE_DEFAULT_DEBUG_CHANNEL(gameux);

/* function from Shell32, not defined in header */
extern BOOL WINAPI GUIDFromStringW(LPCWSTR psz, LPGUID pguid);

/*******************************************************************************
 * GameUX helper functions
 */
/*******************************************************************************
 * GAMEUX_initGameData
 *
 * Internal helper function. Description available in gameux_private.h file
 */
void GAMEUX_initGameData(struct GAMEUX_GAME_DATA *GameData)
{
    GameData->sGDFBinaryPath = NULL;
    GameData->sGameInstallDirectory = NULL;
}
/*******************************************************************************
 * GAMEUX_uninitGameData
 *
 * Internal helper function. Description available in gameux_private.h file
 */
void GAMEUX_uninitGameData(struct GAMEUX_GAME_DATA *GameData)
{
    HeapFree(GetProcessHeap(), 0, GameData->sGDFBinaryPath);
    HeapFree(GetProcessHeap(), 0, GameData->sGameInstallDirectory);
}
/*******************************************************************************
 * GAMEUX_buildGameRegistryPath
 *
 * Helper function, builds registry path to key, where game's data are stored
 *
 * Parameters:
 *  installScope                [I]     the scope which was used in AddGame/InstallGame call
 *  gameInstanceId              [I]     game instance GUID
 *  lpRegistryPath              [O]     pointer which will receive address to string
 *                                      containing expected registry path. Path
 *                                      is relative to HKLM registry key. It
 *                                      must be freed by calling HeapFree(GetProcessHeap(), 0, ...)
 *
 * Name of game's registry key always follows patterns below:
 *  When game is installed for current user only (installScope is GIS_CURRENT_USER):
 *      HKEY_LOCAL_MACHINE\Software\Microsoft\Windows\CurrentVersion\
 *          GameUX\[user's security ID]\[game instance ID]
 *
 *  When game is installed for all users (installScope is GIS_ALL_USERS):
 *      HKEY_LOCAL_MACHINE\Software\Microsoft\Windows\CurrentVersion\
 *          GameUX\Games\[game instance ID]
 *
 *
 */
static HRESULT GAMEUX_buildGameRegistryPath(GAME_INSTALL_SCOPE installScope,
        LPCGUID gameInstanceId,
        LPWSTR* lpRegistryPath)
{
    static const WCHAR sGameUxRegistryPath[] = {'S','O','F','T','W','A','R','E','\\',
            'M','i','c','r','o','s','o','f','t','\\','W','i','n','d','o','w','s','\\',
            'C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\','G','a','m','e','U','X',0};
    static const WCHAR sGames[] = {'G','a','m','e','s',0};
    static const WCHAR sBackslash[] = {'\\',0};

    HRESULT hr = S_OK;
    HANDLE hToken = NULL;
    PTOKEN_USER pTokenUser = NULL;
    DWORD dwLength;
    LPWSTR lpSID = NULL;
    WCHAR sInstanceId[40];
    WCHAR sRegistryPath[8192];

    TRACE("(0x%x, %s, %p)\n", installScope, debugstr_guid(gameInstanceId), lpRegistryPath);

    lstrcpyW(sRegistryPath, sGameUxRegistryPath);
    lstrcatW(sRegistryPath, sBackslash);

    if(installScope == GIS_CURRENT_USER)
    {
        /* build registry path containing user's SID */
        if(!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
            hr = HRESULT_FROM_WIN32(GetLastError());

        if(SUCCEEDED(hr))
        {
            if(!GetTokenInformation(hToken, TokenUser, NULL, 0, &dwLength) &&
                    GetLastError()!=ERROR_INSUFFICIENT_BUFFER)
                hr = HRESULT_FROM_WIN32(GetLastError());

            if(SUCCEEDED(hr))
            {
                pTokenUser = HeapAlloc(GetProcessHeap(), 0, dwLength);
                if(!pTokenUser)
                    hr = E_OUTOFMEMORY;
            }

            if(SUCCEEDED(hr))
                if(!GetTokenInformation(hToken, TokenUser, (LPVOID)pTokenUser, dwLength, &dwLength))
                    hr = HRESULT_FROM_WIN32(GetLastError());

            if(SUCCEEDED(hr))
                if(!ConvertSidToStringSidW(pTokenUser->User.Sid, &lpSID))
                    hr = HRESULT_FROM_WIN32(GetLastError());

            if(SUCCEEDED(hr))
            {
                lstrcatW(sRegistryPath, lpSID);
                LocalFree(lpSID);
            }

            HeapFree(GetProcessHeap(), 0, pTokenUser);
            CloseHandle(hToken);
        }
    }
    else if(installScope == GIS_ALL_USERS)
        /* build registry path without SID */
        lstrcatW(sRegistryPath, sGames);
    else
        hr = E_INVALIDARG;

    /* put game's instance id on the end of path */
    if(SUCCEEDED(hr))
        hr = (StringFromGUID2(gameInstanceId, sInstanceId, sizeof(sInstanceId)/sizeof(sInstanceId[0])) ? S_OK : E_FAIL);

    if(SUCCEEDED(hr))
    {
        lstrcatW(sRegistryPath, sBackslash);
        lstrcatW(sRegistryPath, sInstanceId);
    }

    if(SUCCEEDED(hr))
    {
        *lpRegistryPath = HeapAlloc(GetProcessHeap(), 0, (lstrlenW(sRegistryPath)+1)*sizeof(WCHAR));
        if(!*lpRegistryPath)
            hr = E_OUTOFMEMORY;
    }

    if(SUCCEEDED(hr))
        lstrcpyW(*lpRegistryPath, sRegistryPath);

    TRACE("result: 0x%x, path: %s\n", hr, debugstr_w(*lpRegistryPath));
    return hr;
}
/*******************************************************************************
 * GAMEUX_WriteRegistryRecord
 *
 * Helper function, writes data associated with game (stored in GAMEUX_GAME_DATA
 * structure) into expected place in registry.
 *
 * Parameters:
 *  GameData                            [I]     structure with data which will
 *                                              be written into registry.
 *                                              Proper values of fields installScope
 *                                              and guidInstanceId are required
 *                                              to create registry key.
 *
 * Schema of naming registry keys associated with games is available in
 * description of _buildGameRegistryPath internal function.
 *
 * List of registry keys associated with structure fields:
 *  Key                              Field in GAMEUX_GAME_DATA structure
 *   ApplicationId                    guidApplicationId
 *   ConfigApplicationPath            sGameInstallDirectory
 *   ConfigGDFBinaryPath              sGDFBinaryPath
 *
 */
static HRESULT GAMEUX_WriteRegistryRecord(struct GAMEUX_GAME_DATA *GameData)
{
    static const WCHAR sApplicationId[] =
            {'A','p','p','l','i','c','a','t','i','o','n','I','d',0};
    static const WCHAR sConfigApplicationPath[] =
            {'C','o','n','f','i','g','A','p','p','l','i','c','a','t','i','o','n','P','a','t','h',0};
    static const WCHAR sConfigGDFBinaryPath[] =
            {'C','o','n','f','i','g','G','D','F','B','i','n','a','r','y','P','a','t','h',0};

    HRESULT hr, hr2;
    LPWSTR lpRegistryKey;
    HKEY hKey;
    WCHAR sGameApplicationId[40];

    TRACE("(%p)\n", GameData);

    hr = GAMEUX_buildGameRegistryPath(GameData->installScope, &GameData->guidInstanceId, &lpRegistryKey);

    if(SUCCEEDED(hr))
        hr = (StringFromGUID2(&GameData->guidApplicationId, sGameApplicationId, sizeof(sGameApplicationId)/sizeof(sGameApplicationId[0])) ? S_OK : E_FAIL);

    if(SUCCEEDED(hr))
        hr = HRESULT_FROM_WIN32(RegCreateKeyExW(HKEY_LOCAL_MACHINE, lpRegistryKey,
                                                0, NULL, 0, KEY_ALL_ACCESS, NULL,
                                                &hKey, NULL));

    if(SUCCEEDED(hr))
    {
        /* write game data to registry key */
        hr = HRESULT_FROM_WIN32(RegSetValueExW(hKey, sConfigApplicationPath, 0,
                                               REG_SZ, (LPBYTE)(GameData->sGameInstallDirectory),
                                               (lstrlenW(GameData->sGameInstallDirectory)+1)*sizeof(WCHAR)));

        if(SUCCEEDED(hr))
            hr = HRESULT_FROM_WIN32(RegSetValueExW(hKey, sConfigGDFBinaryPath, 0,
                                                   REG_SZ, (LPBYTE)(GameData->sGDFBinaryPath),
                                                   (lstrlenW(GameData->sGDFBinaryPath)+1)*sizeof(WCHAR)));

        if(SUCCEEDED(hr))
            hr = HRESULT_FROM_WIN32(RegSetValueExW(hKey, sApplicationId, 0,
                                                   REG_SZ, (LPBYTE)(sGameApplicationId),
                                                   (lstrlenW(sGameApplicationId)+1)*sizeof(WCHAR)));

        RegCloseKey(hKey);

        if(FAILED(hr))
        {
            /* if something failed, remove whole key */
            hr2 = RegDeleteKeyExW(HKEY_LOCAL_MACHINE, lpRegistryKey, 0, 0);
            /* do not overwrite old failure code with new success code */
            if(FAILED(hr2))
                hr = hr2;
        }
    }

    HeapFree(GetProcessHeap(), 0, lpRegistryKey);
    TRACE("returning 0x%x\n", hr);
    return hr;
}
/*******************************************************************************
 * GAMEUX_ParseGameDefinition
 *
 * Helper function, loads data from given XML element into fields of GAME_DATA
 * structure
 *
 * Parameters:
 *  lpXMLGameDefinitionElement          [I]     Game Definition XML element
 *  GameData                            [O]     structure where data loaded from
 *                                              XML element will be stored in
 */
static HRESULT GAMEUX_ParseGameDefinition(
        IXMLDOMElement *gdElement,
        struct GAMEUX_GAME_DATA *GameData)
{
    static const WCHAR sGameId[] = {'g','a','m','e','I','D',0};

    HRESULT hr = S_OK;
    BSTR bstrAttribute;
    VARIANT variant;

    TRACE("(%p, %p)\n", gdElement, GameData);

    bstrAttribute = SysAllocString(sGameId);
    if(!bstrAttribute)
        hr = E_OUTOFMEMORY;

    hr = IXMLDOMElement_getAttribute(gdElement, bstrAttribute, &variant);

    if(SUCCEEDED(hr))
    {
        hr = ( GUIDFromStringW(V_BSTR(&variant), &GameData->guidApplicationId)==TRUE ? S_OK : E_FAIL);

        SysFreeString(V_BSTR(&variant));
    }

    SysFreeString(bstrAttribute);

    return hr;
}
/*******************************************************************************
 * GAMEUX_ParseGDFBinary
 *
 * Helper funtion, loads given binary and parses embed GDF if there's any.
 *
 * Parameters:
 *  GameData                [I/O]   Structure with game's data. Content of field
 *                                  sGDFBinaryPath defines path to binary, from
 *                                  which embed GDF will be loaded. Data from
 *                                  GDF will be stored in other fields of this
 *                                  structure.
 */
static HRESULT GAMEUX_ParseGDFBinary(struct GAMEUX_GAME_DATA *GameData)
{
    static const WCHAR sRes[] = {'r','e','s',':','/','/',0};
    static const WCHAR sDATA[] = {'D','A','T','A',0};
    static const WCHAR sSlash[] = {'/',0};

    HRESULT hr = S_OK;
    WCHAR sResourcePath[MAX_PATH];
    VARIANT variant;
    VARIANT_BOOL isSuccessful;
    IXMLDOMDocument *document;
    IXMLDOMNode *gdNode;
    IXMLDOMElement *root, *gdElement;

    TRACE("(%p)->sGDFBinaryPath = %s\n", GameData, debugstr_w(GameData->sGDFBinaryPath));

    /* prepare path to GDF, using res:// prefix */
    lstrcpyW(sResourcePath, sRes);
    lstrcatW(sResourcePath, GameData->sGDFBinaryPath);
    lstrcatW(sResourcePath, sSlash);
    lstrcatW(sResourcePath, sDATA);
    lstrcatW(sResourcePath, sSlash);
    lstrcatW(sResourcePath, ID_GDF_XML_STR);

    hr = CoCreateInstance(&CLSID_DOMDocument, NULL, CLSCTX_INPROC_SERVER,
            &IID_IXMLDOMDocument, (void**)&document);

    if(SUCCEEDED(hr))
    {
        /* load GDF into MSXML */
        V_VT(&variant) = VT_BSTR;
        V_BSTR(&variant) = SysAllocString(sResourcePath);
        if(!V_BSTR(&variant))
            hr = E_OUTOFMEMORY;

        if(SUCCEEDED(hr))
        {
            hr = IXMLDOMDocument_load(document, variant, &isSuccessful);
            if(hr == S_FALSE || isSuccessful == VARIANT_FALSE)
                hr = E_FAIL;
        }

        SysFreeString(V_BSTR(&variant));

        if(SUCCEEDED(hr))
        {
            hr = IXMLDOMDocument_get_documentElement(document, &root);
            if(hr == S_FALSE)
                hr = E_FAIL;
        }

        if(SUCCEEDED(hr))
        {
            hr = IXMLDOMElement_get_firstChild(root, &gdNode);
            if(hr == S_FALSE)
                hr = E_FAIL;

            if(SUCCEEDED(hr))
            {
                hr = IXMLDOMNode_QueryInterface(gdNode, &IID_IXMLDOMElement, (LPVOID*)&gdElement);
                if(SUCCEEDED(hr))
                {
                    hr = GAMEUX_ParseGameDefinition(gdElement, GameData);
                    IXMLDOMElement_Release(gdElement);
                }

                IXMLDOMNode_Release(gdNode);
            }

            IXMLDOMElement_Release(root);
        }

        IXMLDOMDocument_Release(document);
    }

    return hr;
}
/*******************************************************************************
 * GAMEUX_RegisterGame
 *
 * Internal helper function. Description available in gameux_private.h file
 */
HRESULT WINAPI GAMEUX_RegisterGame(LPCWSTR sGDFBinaryPath,
        LPCWSTR sGameInstallDirectory,
        GAME_INSTALL_SCOPE installScope,
        GUID *pInstanceID)
{
    HRESULT hr = S_OK;
    struct GAMEUX_GAME_DATA GameData;

    TRACE("(%s, %s, 0x%x, %s)\n", debugstr_w(sGDFBinaryPath), debugstr_w(sGameInstallDirectory), installScope, debugstr_guid(pInstanceID));

    GAMEUX_initGameData(&GameData);
    GameData.sGDFBinaryPath = HeapAlloc(GetProcessHeap(), 0, (lstrlenW(sGDFBinaryPath)+1)*sizeof(WCHAR));
    lstrcpyW(GameData.sGDFBinaryPath, sGDFBinaryPath);
    GameData.sGameInstallDirectory = HeapAlloc(GetProcessHeap(), 0, (lstrlenW(sGameInstallDirectory)+1)*sizeof(WCHAR));
    lstrcpyW(GameData.sGameInstallDirectory, sGameInstallDirectory);
    GameData.installScope = installScope;

    /* generate GUID if it was not provided by user */
    if(IsEqualGUID(pInstanceID, &GUID_NULL))
        hr = CoCreateGuid(pInstanceID);

    GameData.guidInstanceId = *pInstanceID;

    /* load data from GDF binary */
    if(SUCCEEDED(hr))
        hr = GAMEUX_ParseGDFBinary(&GameData);

    /* save data to registry */
    if(SUCCEEDED(hr))
        hr = GAMEUX_WriteRegistryRecord(&GameData);

    GAMEUX_uninitGameData(&GameData);
    TRACE("returing 0x%08x\n", hr);
    return hr;
}
/*******************************************************************************
 * GameExplorer implementation
 */

typedef struct _GameExplorerImpl
{
    const struct IGameExplorerVtbl *lpGameExplorerVtbl;
    const struct IGameExplorer2Vtbl *lpGameExplorer2Vtbl;
    LONG ref;
} GameExplorerImpl;

static inline GameExplorerImpl *impl_from_IGameExplorer(IGameExplorer *iface)
{
    return (GameExplorerImpl*)((char*)iface - FIELD_OFFSET(GameExplorerImpl, lpGameExplorerVtbl));
}

static inline IGameExplorer* IGameExplorer_from_impl(GameExplorerImpl* This)
{
    return (struct IGameExplorer*)&This->lpGameExplorerVtbl;
}

static inline GameExplorerImpl *impl_from_IGameExplorer2(IGameExplorer2 *iface)
{
    return (GameExplorerImpl*)((char*)iface - FIELD_OFFSET(GameExplorerImpl, lpGameExplorer2Vtbl));
}

static inline IGameExplorer2* IGameExplorer2_from_impl(GameExplorerImpl* This)
{
    return (struct IGameExplorer2*)&This->lpGameExplorer2Vtbl;
}

static HRESULT WINAPI GameExplorerImpl_QueryInterface(
        IGameExplorer *iface,
        REFIID riid,
        void **ppvObject)
{
    GameExplorerImpl *This = impl_from_IGameExplorer(iface);

    TRACE("(%p, %s, %p)\n", This, debugstr_guid(riid), ppvObject);

    *ppvObject = NULL;

    if(IsEqualGUID(riid, &IID_IUnknown) ||
       IsEqualGUID(riid, &IID_IGameExplorer))
    {
        *ppvObject = IGameExplorer_from_impl(This);
    }
    else if(IsEqualGUID(riid, &IID_IGameExplorer2))
    {
        *ppvObject = IGameExplorer2_from_impl(This);
    }
    else
    {
        FIXME("interface %s not implemented\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    IGameExplorer_AddRef(iface);
    return S_OK;
}

static ULONG WINAPI GameExplorerImpl_AddRef(IGameExplorer *iface)
{
    GameExplorerImpl *This = impl_from_IGameExplorer(iface);
    LONG ref;

    ref = InterlockedIncrement(&This->ref);

    TRACE("(%p): ref=%d\n", This, ref);
    return ref;
}

static ULONG WINAPI GameExplorerImpl_Release(IGameExplorer *iface)
{
    GameExplorerImpl *This = impl_from_IGameExplorer(iface);
    LONG ref;

    ref = InterlockedDecrement(&This->ref);
    TRACE("(%p): ref=%d\n", This, ref);

    if(ref == 0)
    {
        TRACE("freeing GameExplorer object\n");
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static HRESULT WINAPI GameExplorerImpl_AddGame(
        IGameExplorer *iface,
        BSTR bstrGDFBinaryPath,
        BSTR sGameInstallDirectory,
        GAME_INSTALL_SCOPE installScope,
        GUID *pInstanceID)
{
    GameExplorerImpl *This = impl_from_IGameExplorer(iface);
    TRACE("(%p, %s, %s, 0x%x, %s)\n", This, debugstr_w(bstrGDFBinaryPath), debugstr_w(sGameInstallDirectory), installScope, debugstr_guid(pInstanceID));
    return GAMEUX_RegisterGame(bstrGDFBinaryPath, sGameInstallDirectory, installScope, pInstanceID);
}

static HRESULT WINAPI GameExplorerImpl_RemoveGame(
        IGameExplorer *iface,
        GUID instanceID)
{
    GameExplorerImpl *This = impl_from_IGameExplorer(iface);

    TRACE("(%p, %s)\n", This, debugstr_guid(&instanceID));
    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI GameExplorerImpl_UpdateGame(
        IGameExplorer *iface,
        GUID instanceID)
{
    GameExplorerImpl *This = impl_from_IGameExplorer(iface);

    TRACE("(%p, %s)\n", This, debugstr_guid(&instanceID));
    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI GameExplorerImpl_VerifyAccess(
        IGameExplorer *iface,
        BSTR sGDFBinaryPath,
        BOOL *pHasAccess)
{
    GameExplorerImpl *This = impl_from_IGameExplorer(iface);

    TRACE("(%p, %s, %p)\n", This, debugstr_w(sGDFBinaryPath), pHasAccess);
    FIXME("stub\n");
    return E_NOTIMPL;
}

static const struct IGameExplorerVtbl GameExplorerImplVtbl =
{
    GameExplorerImpl_QueryInterface,
    GameExplorerImpl_AddRef,
    GameExplorerImpl_Release,
    GameExplorerImpl_AddGame,
    GameExplorerImpl_RemoveGame,
    GameExplorerImpl_UpdateGame,
    GameExplorerImpl_VerifyAccess
};


static HRESULT WINAPI GameExplorer2Impl_QueryInterface(
        IGameExplorer2 *iface,
        REFIID riid,
        void **ppvObject)
{
    GameExplorerImpl *This = impl_from_IGameExplorer2(iface);
    return GameExplorerImpl_QueryInterface(IGameExplorer_from_impl(This), riid, ppvObject);
}

static ULONG WINAPI GameExplorer2Impl_AddRef(IGameExplorer2 *iface)
{
    GameExplorerImpl *This = impl_from_IGameExplorer2(iface);
    return GameExplorerImpl_AddRef(IGameExplorer_from_impl(This));
}

static ULONG WINAPI GameExplorer2Impl_Release(IGameExplorer2 *iface)
{
    GameExplorerImpl *This = impl_from_IGameExplorer2(iface);
    return GameExplorerImpl_Release(IGameExplorer_from_impl(This));
}

static HRESULT WINAPI GameExplorer2Impl_CheckAccess(
        IGameExplorer2 *iface,
        LPCWSTR binaryGDFPath,
        BOOL *pHasAccess)
{
    GameExplorerImpl *This = impl_from_IGameExplorer2(iface);
    FIXME("stub (%p, %s, %p)\n", This, debugstr_w(binaryGDFPath), pHasAccess);
    return E_NOTIMPL;
}

static HRESULT WINAPI GameExplorer2Impl_InstallGame(
        IGameExplorer2 *iface,
        LPCWSTR binaryGDFPath,
        LPCWSTR installDirectory,
        GAME_INSTALL_SCOPE installScope)
{
    GameExplorerImpl *This = impl_from_IGameExplorer2(iface);
    FIXME("stub (%p, %s, %s, 0x%x)\n", This, debugstr_w(binaryGDFPath), debugstr_w(installDirectory), installScope);
    return E_NOTIMPL;
}

static HRESULT WINAPI GameExplorer2Impl_UninstallGame(
        IGameExplorer2 *iface,
        LPCWSTR binaryGDFPath)
{
    GameExplorerImpl *This = impl_from_IGameExplorer2(iface);
    FIXME("stub (%p, %s)\n", This, debugstr_w(binaryGDFPath));
    return E_NOTIMPL;
}

static const struct IGameExplorer2Vtbl GameExplorer2ImplVtbl =
{
    GameExplorer2Impl_QueryInterface,
    GameExplorer2Impl_AddRef,
    GameExplorer2Impl_Release,
    GameExplorer2Impl_InstallGame,
    GameExplorer2Impl_UninstallGame,
    GameExplorer2Impl_CheckAccess
};

/*
 * Construction routine
 */
HRESULT GameExplorer_create(
        IUnknown* pUnkOuter,
        IUnknown** ppObj)
{
    GameExplorerImpl *pGameExplorer;

    TRACE("(%p, %p)\n", pUnkOuter, ppObj);

    pGameExplorer = HeapAlloc(GetProcessHeap(), 0, sizeof(*pGameExplorer));

    if(!pGameExplorer)
        return E_OUTOFMEMORY;

    pGameExplorer->lpGameExplorerVtbl = &GameExplorerImplVtbl;
    pGameExplorer->lpGameExplorer2Vtbl = &GameExplorer2ImplVtbl;
    pGameExplorer->ref = 1;

    *ppObj = (IUnknown*)(&pGameExplorer->lpGameExplorerVtbl);

    TRACE("returning iface: %p\n", *ppObj);
    return S_OK;
}
