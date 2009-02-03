/*
 *  ITfInputProcessorProfiles implementation
 *
 *  Copyright 2009 Aric Stewart, CodeWeavers
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

#include "config.h"

#include <stdarg.h>

#define COBJMACROS

#include "wine/debug.h"
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winuser.h"
#include "shlwapi.h"
#include "winerror.h"
#include "objbase.h"

#include "wine/unicode.h"

#include "msctf.h"
#include "msctf_internal.h"

WINE_DEFAULT_DEBUG_CHANNEL(msctf);

typedef struct tagInputProcessorProfiles {
    const ITfInputProcessorProfilesVtbl *InputProcessorProfilesVtbl;
    LONG refCount;

    LANGID  currentLanguage;
} InputProcessorProfiles;

static void InputProcessorProfiles_Destructor(InputProcessorProfiles *This)
{
    TRACE("destroying %p\n", This);
    HeapFree(GetProcessHeap(),0,This);
}

static HRESULT WINAPI InputProcessorProfiles_QueryInterface(ITfInputProcessorProfiles *iface, REFIID iid, LPVOID *ppvOut)
{
    InputProcessorProfiles *This = (InputProcessorProfiles *)iface;
    *ppvOut = NULL;

    if (IsEqualIID(iid, &IID_IUnknown) || IsEqualIID(iid, &IID_ITfInputProcessorProfiles))
    {
        *ppvOut = This;
    }

    if (*ppvOut)
    {
        IUnknown_AddRef(iface);
        return S_OK;
    }

    WARN("unsupported interface: %s\n", debugstr_guid(iid));
    return E_NOINTERFACE;
}

static ULONG WINAPI InputProcessorProfiles_AddRef(ITfInputProcessorProfiles *iface)
{
    InputProcessorProfiles *This = (InputProcessorProfiles *)iface;
    return InterlockedIncrement(&This->refCount);
}

static ULONG WINAPI InputProcessorProfiles_Release(ITfInputProcessorProfiles *iface)
{
    InputProcessorProfiles *This = (InputProcessorProfiles *)iface;
    ULONG ret;

    ret = InterlockedDecrement(&This->refCount);
    if (ret == 0)
        InputProcessorProfiles_Destructor(This);
    return ret;
}

/*****************************************************
 * ITfInputProcessorProfiles functions
 *****************************************************/
static HRESULT WINAPI InputProcessorProfiles_Register(
        ITfInputProcessorProfiles *iface, REFCLSID rclsid)
{
    InputProcessorProfiles *This = (InputProcessorProfiles*)iface;
    HKEY tipkey;
    WCHAR buf[39];
    static const WCHAR fmt[] = {'%','s','\\','%','s',0};
    WCHAR fullkey[68];

    TRACE("(%p) %s\n",This,debugstr_guid(rclsid));

    StringFromGUID2(rclsid, buf, 39);
    sprintfW(fullkey,fmt,szwSystemTIPKey,buf);

    if (RegCreateKeyExW(HKEY_LOCAL_MACHINE,fullkey, 0, NULL, 0,
                    KEY_READ | KEY_WRITE, NULL, &tipkey, NULL) != ERROR_SUCCESS)
        return E_FAIL;

    RegCloseKey(tipkey);

    return S_OK;
}

static HRESULT WINAPI InputProcessorProfiles_Unregister(
        ITfInputProcessorProfiles *iface, REFCLSID rclsid)
{
    InputProcessorProfiles *This = (InputProcessorProfiles*)iface;
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI InputProcessorProfiles_AddLanguageProfile(
        ITfInputProcessorProfiles *iface, REFCLSID rclsid,
        LANGID langid, REFGUID guidProfile, const WCHAR *pchDesc,
        ULONG cchDesc, const WCHAR *pchIconFile, ULONG cchFile,
        ULONG uIconIndex)
{
    HKEY tipkey,fmtkey;
    WCHAR buf[39];
    WCHAR fullkey[100];
    ULONG res;

    static const WCHAR fmt[] = {'%','s','\\','%','s',0};
    static const WCHAR fmt2[] = {'%','s','\\','0','x','%','0','8','x','\\','%','s',0};
    static const WCHAR lngp[] = {'L','a','n','g','u','a','g','e','P','r','o','f','i','l','e',0};
    static const WCHAR desc[] = {'D','e','s','c','r','i','p','t','i','o','n',0};
    static const WCHAR icnf[] = {'I','c','o','n','F','i','l','e',0};
    static const WCHAR icni[] = {'I','c','o','n','I','n','d','e','x',0};

    InputProcessorProfiles *This = (InputProcessorProfiles*)iface;

    TRACE("(%p) %s %x %s %s %s %i\n",This,debugstr_guid(rclsid), langid,
            debugstr_guid(guidProfile), debugstr_wn(pchDesc,cchDesc),
            debugstr_wn(pchIconFile,cchFile),uIconIndex);

    StringFromGUID2(rclsid, buf, 39);
    sprintfW(fullkey,fmt,szwSystemTIPKey,buf);

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,fullkey, 0, KEY_READ | KEY_WRITE,
                &tipkey ) != ERROR_SUCCESS)
        return E_FAIL;

    StringFromGUID2(guidProfile, buf, 39);
    sprintfW(fullkey,fmt2,lngp,langid,buf);

    res = RegCreateKeyExW(tipkey,fullkey, 0, NULL, 0, KEY_READ | KEY_WRITE,
            NULL, &fmtkey, NULL);

    if (!res)
    {
        RegSetValueExW(fmtkey, desc, 0, REG_SZ, (LPBYTE)pchDesc, cchDesc * sizeof(WCHAR));
        RegSetValueExW(fmtkey, icnf, 0, REG_SZ, (LPBYTE)pchIconFile, cchFile * sizeof(WCHAR));
        RegSetValueExW(fmtkey, icni, 0, REG_DWORD, (LPBYTE)&uIconIndex, sizeof(DWORD));
        RegCloseKey(fmtkey);
    }
    RegCloseKey(tipkey);

    if (!res)
        return S_OK;
    else
        return E_FAIL;
}

static HRESULT WINAPI InputProcessorProfiles_RemoveLanguageProfile(
        ITfInputProcessorProfiles *iface, REFCLSID rclsid, LANGID langid,
        REFGUID guidProfile)
{
    InputProcessorProfiles *This = (InputProcessorProfiles*)iface;
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI InputProcessorProfiles_EnumInputProcessorInfo(
        ITfInputProcessorProfiles *iface, IEnumGUID **ppEnum)
{
    InputProcessorProfiles *This = (InputProcessorProfiles*)iface;
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI InputProcessorProfiles_GetDefaultLanguageProfile(
        ITfInputProcessorProfiles *iface, LANGID langid, REFGUID catid,
        CLSID *pclsid, GUID *pguidProfile)
{
    InputProcessorProfiles *This = (InputProcessorProfiles*)iface;
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI InputProcessorProfiles_SetDefaultLanguageProfile(
        ITfInputProcessorProfiles *iface, LANGID langid, REFCLSID rclsid,
        REFGUID guidProfiles)
{
    InputProcessorProfiles *This = (InputProcessorProfiles*)iface;
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI InputProcessorProfiles_ActivateLanguageProfile(
        ITfInputProcessorProfiles *iface, REFCLSID rclsid, LANGID langid,
        REFGUID guidProfiles)
{
    InputProcessorProfiles *This = (InputProcessorProfiles*)iface;
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI InputProcessorProfiles_GetActiveLanguageProfile(
        ITfInputProcessorProfiles *iface, REFCLSID rclsid, LANGID *plangid,
        GUID *pguidProfile)
{
    InputProcessorProfiles *This = (InputProcessorProfiles*)iface;
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI InputProcessorProfiles_GetLanguageProfileDescription(
        ITfInputProcessorProfiles *iface, REFCLSID rclsid, LANGID langid,
        REFGUID guidProfile, BSTR *pbstrProfile)
{
    InputProcessorProfiles *This = (InputProcessorProfiles*)iface;
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI InputProcessorProfiles_GetCurrentLanguage(
        ITfInputProcessorProfiles *iface, LANGID *plangid)
{
    InputProcessorProfiles *This = (InputProcessorProfiles*)iface;
    TRACE("(%p) 0x%x\n",This,This->currentLanguage);

    if (!plangid)
        return E_INVALIDARG;

    *plangid = This->currentLanguage;

    return S_OK;
}

static HRESULT WINAPI InputProcessorProfiles_ChangeCurrentLanguage(
        ITfInputProcessorProfiles *iface, LANGID langid)
{
    InputProcessorProfiles *This = (InputProcessorProfiles*)iface;
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI InputProcessorProfiles_GetLanguageList(
        ITfInputProcessorProfiles *iface, LANGID **ppLangId, ULONG *pulCount)
{
    InputProcessorProfiles *This = (InputProcessorProfiles*)iface;
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI InputProcessorProfiles_EnumLanguageProfiles(
        ITfInputProcessorProfiles *iface, LANGID langid,
        IEnumTfLanguageProfiles **ppEnum)
{
    InputProcessorProfiles *This = (InputProcessorProfiles*)iface;
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI InputProcessorProfiles_EnableLanguageProfile(
        ITfInputProcessorProfiles *iface, REFCLSID rclsid, LANGID langid,
        REFGUID guidProfile, BOOL fEnable)
{
    InputProcessorProfiles *This = (InputProcessorProfiles*)iface;
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI InputProcessorProfiles_IsEnabledLanguageProfile(
        ITfInputProcessorProfiles *iface, REFCLSID rclsid, LANGID langid,
        REFGUID guidProfile, BOOL *pfEnable)
{
    InputProcessorProfiles *This = (InputProcessorProfiles*)iface;
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI InputProcessorProfiles_EnableLanguageProfileByDefault(
        ITfInputProcessorProfiles *iface, REFCLSID rclsid, LANGID langid,
        REFGUID guidProfile, BOOL fEnable)
{
    InputProcessorProfiles *This = (InputProcessorProfiles*)iface;
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI InputProcessorProfiles_SubstituteKeyboardLayout(
        ITfInputProcessorProfiles *iface, REFCLSID rclsid, LANGID langid,
        REFGUID guidProfile, HKL hKL)
{
    InputProcessorProfiles *This = (InputProcessorProfiles*)iface;
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}


static const ITfInputProcessorProfilesVtbl InputProcessorProfiles_InputProcessorProfilesVtbl =
{
    InputProcessorProfiles_QueryInterface,
    InputProcessorProfiles_AddRef,
    InputProcessorProfiles_Release,

    InputProcessorProfiles_Register,
    InputProcessorProfiles_Unregister,
    InputProcessorProfiles_AddLanguageProfile,
    InputProcessorProfiles_RemoveLanguageProfile,
    InputProcessorProfiles_EnumInputProcessorInfo,
    InputProcessorProfiles_GetDefaultLanguageProfile,
    InputProcessorProfiles_SetDefaultLanguageProfile,
    InputProcessorProfiles_ActivateLanguageProfile,
    InputProcessorProfiles_GetActiveLanguageProfile,
    InputProcessorProfiles_GetLanguageProfileDescription,
    InputProcessorProfiles_GetCurrentLanguage,
    InputProcessorProfiles_ChangeCurrentLanguage,
    InputProcessorProfiles_GetLanguageList,
    InputProcessorProfiles_EnumLanguageProfiles,
    InputProcessorProfiles_EnableLanguageProfile,
    InputProcessorProfiles_IsEnabledLanguageProfile,
    InputProcessorProfiles_EnableLanguageProfileByDefault,
    InputProcessorProfiles_SubstituteKeyboardLayout
};

HRESULT InputProcessorProfiles_Constructor(IUnknown *pUnkOuter, IUnknown **ppOut)
{
    InputProcessorProfiles *This;
    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    This = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(InputProcessorProfiles));
    if (This == NULL)
        return E_OUTOFMEMORY;

    This->InputProcessorProfilesVtbl= &InputProcessorProfiles_InputProcessorProfilesVtbl;
    This->refCount = 1;
    This->currentLanguage = GetUserDefaultLCID();

    TRACE("returning %p\n", This);
    *ppOut = (IUnknown *)This;
    return S_OK;
}
