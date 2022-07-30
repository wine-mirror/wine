/*
 * IMM32 library
 *
 * Copyright 1998 Patrik Stridvall
 * Copyright 2002, 2003, 2007 CodeWeavers, Aric Stewart
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

#include <stdarg.h>
#include <stdio.h>

#include "initguid.h"
#include "objbase.h"
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "ntuser.h"
#include "winerror.h"
#include "wine/debug.h"
#include "imm.h"
#include "ddk/imm.h"
#include "winnls.h"
#include "winreg.h"
#include "wine/list.h"

WINE_DEFAULT_DEBUG_CHANNEL(imm);

#define IMM_INIT_MAGIC 0x19650412
BOOL WINAPI User32InitializeImmEntryTable(DWORD);

/* MSIME messages */
static UINT WM_MSIME_SERVICE;
static UINT WM_MSIME_RECONVERTOPTIONS;
static UINT WM_MSIME_MOUSE;
static UINT WM_MSIME_RECONVERTREQUEST;
static UINT WM_MSIME_RECONVERT;
static UINT WM_MSIME_QUERYPOSITION;
static UINT WM_MSIME_DOCUMENTFEED;

typedef struct _tagImmHkl{
    struct list entry;
    HKL         hkl;
    HMODULE     hIME;
    IMEINFO     imeInfo;
    WCHAR       imeClassName[17]; /* 16 character max */
    ULONG       uSelected;
    HWND        UIWnd;

    /* Function Pointers */
    BOOL (WINAPI *pImeInquire)(IMEINFO *, WCHAR *, const WCHAR *);
    BOOL (WINAPI *pImeConfigure)(HKL, HWND, DWORD, void *);
    BOOL (WINAPI *pImeDestroy)(UINT);
    LRESULT (WINAPI *pImeEscape)(HIMC, UINT, void *);
    BOOL (WINAPI *pImeSelect)(HIMC, BOOL);
    BOOL (WINAPI *pImeSetActiveContext)(HIMC, BOOL);
    UINT (WINAPI *pImeToAsciiEx)(UINT, UINT, const BYTE *, DWORD *, UINT, HIMC);
    BOOL (WINAPI *pNotifyIME)(HIMC, DWORD, DWORD, DWORD);
    BOOL (WINAPI *pImeRegisterWord)(const WCHAR *, DWORD, const WCHAR *);
    BOOL (WINAPI *pImeUnregisterWord)(const WCHAR *, DWORD, const WCHAR *);
    UINT (WINAPI *pImeEnumRegisterWord)(REGISTERWORDENUMPROCW, const WCHAR *, DWORD, const WCHAR *, void *);
    BOOL (WINAPI *pImeSetCompositionString)(HIMC, DWORD, const void *, DWORD, const void *, DWORD);
    DWORD (WINAPI *pImeConversionList)(HIMC, const WCHAR *, CANDIDATELIST *, DWORD, UINT);
    BOOL (WINAPI *pImeProcessKey)(HIMC, UINT, LPARAM, const BYTE *);
    UINT (WINAPI *pImeGetRegisterWordStyle)(UINT, STYLEBUFW *);
    DWORD (WINAPI *pImeGetImeMenuItems)(HIMC, DWORD, DWORD, IMEMENUITEMINFOW *, IMEMENUITEMINFOW *, DWORD);
} ImmHkl;

typedef struct tagInputContextData
{
        HIMC            handle;
        DWORD           dwLock;
        INPUTCONTEXT    IMC;
        DWORD           threadID;

        ImmHkl          *immKbd;
        UINT            lastVK;
        BOOL            threadDefault;
} InputContextData;

#define WINE_IMC_VALID_MAGIC 0x56434D49

typedef struct _tagTRANSMSG {
    UINT message;
    WPARAM wParam;
    LPARAM lParam;
} TRANSMSG, *LPTRANSMSG;

struct coinit_spy
{
    IInitializeSpy IInitializeSpy_iface;
    LONG ref;
    ULARGE_INTEGER cookie;
    enum
    {
        IMM_APT_INIT = 0x1,
        IMM_APT_CREATED = 0x2,
        IMM_APT_CAN_FREE = 0x4,
        IMM_APT_BROKEN = 0x8
    } apt_flags;
};

static struct list ImmHklList = LIST_INIT(ImmHklList);

static const WCHAR szImeRegFmt[] = L"System\\CurrentControlSet\\Control\\Keyboard Layouts\\%08lx";

static inline BOOL is_himc_ime_unicode(const InputContextData *data)
{
    return !!(data->immKbd->imeInfo.fdwProperty & IME_PROP_UNICODE);
}

static inline BOOL is_kbd_ime_unicode(const ImmHkl *hkl)
{
    return !!(hkl->imeInfo.fdwProperty & IME_PROP_UNICODE);
}

static BOOL IMM_DestroyContext(HIMC hIMC);
static InputContextData* get_imc_data(HIMC hIMC);

static inline WCHAR *strdupAtoW( const char *str )
{
    WCHAR *ret = NULL;
    if (str)
    {
        DWORD len = MultiByteToWideChar( CP_ACP, 0, str, -1, NULL, 0 );
        if ((ret = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) )))
            MultiByteToWideChar( CP_ACP, 0, str, -1, ret, len );
    }
    return ret;
}

static inline CHAR *strdupWtoA( const WCHAR *str )
{
    CHAR *ret = NULL;
    if (str)
    {
        DWORD len = WideCharToMultiByte( CP_ACP, 0, str, -1, NULL, 0, NULL, NULL );
        if ((ret = HeapAlloc( GetProcessHeap(), 0, len )))
            WideCharToMultiByte( CP_ACP, 0, str, -1, ret, len, NULL, NULL );
    }
    return ret;
}

static DWORD convert_candidatelist_WtoA(
        LPCANDIDATELIST lpSrc, LPCANDIDATELIST lpDst, DWORD dwBufLen)
{
    DWORD ret, i, len;

    ret = FIELD_OFFSET( CANDIDATELIST, dwOffset[lpSrc->dwCount] );
    if ( lpDst && dwBufLen > 0 )
    {
        *lpDst = *lpSrc;
        lpDst->dwOffset[0] = ret;
    }

    for ( i = 0; i < lpSrc->dwCount; i++)
    {
        LPBYTE src = (LPBYTE)lpSrc + lpSrc->dwOffset[i];

        if ( lpDst && dwBufLen > 0 )
        {
            LPBYTE dest = (LPBYTE)lpDst + lpDst->dwOffset[i];

            len = WideCharToMultiByte(CP_ACP, 0, (LPCWSTR)src, -1,
                                      (LPSTR)dest, dwBufLen, NULL, NULL);

            if ( i + 1 < lpSrc->dwCount )
                lpDst->dwOffset[i+1] = lpDst->dwOffset[i] + len * sizeof(char);
            dwBufLen -= len * sizeof(char);
        }
        else
            len = WideCharToMultiByte(CP_ACP, 0, (LPCWSTR)src, -1, NULL, 0, NULL, NULL);

        ret += len * sizeof(char);
    }

    if ( lpDst )
        lpDst->dwSize = ret;

    return ret;
}

static DWORD convert_candidatelist_AtoW(
        LPCANDIDATELIST lpSrc, LPCANDIDATELIST lpDst, DWORD dwBufLen)
{
    DWORD ret, i, len;

    ret = FIELD_OFFSET( CANDIDATELIST, dwOffset[lpSrc->dwCount] );
    if ( lpDst && dwBufLen > 0 )
    {
        *lpDst = *lpSrc;
        lpDst->dwOffset[0] = ret;
    }

    for ( i = 0; i < lpSrc->dwCount; i++)
    {
        LPBYTE src = (LPBYTE)lpSrc + lpSrc->dwOffset[i];

        if ( lpDst && dwBufLen > 0 )
        {
            LPBYTE dest = (LPBYTE)lpDst + lpDst->dwOffset[i];

            len = MultiByteToWideChar(CP_ACP, 0, (LPCSTR)src, -1,
                                      (LPWSTR)dest, dwBufLen);

            if ( i + 1 < lpSrc->dwCount )
                lpDst->dwOffset[i+1] = lpDst->dwOffset[i] + len * sizeof(WCHAR);
            dwBufLen -= len * sizeof(WCHAR);
        }
        else
            len = MultiByteToWideChar(CP_ACP, 0, (LPCSTR)src, -1, NULL, 0);

        ret += len * sizeof(WCHAR);
    }

    if ( lpDst )
        lpDst->dwSize = ret;

    return ret;
}

static struct coinit_spy *get_thread_coinit_spy(void)
{
    return (struct coinit_spy *)(UINT_PTR)NtUserGetThreadInfo()->client_imm;
}

static void imm_couninit_thread(BOOL cleanup)
{
    struct coinit_spy *spy;

    TRACE("implicit COM deinitialization\n");

    if (!(spy = get_thread_coinit_spy()) || (spy->apt_flags & IMM_APT_BROKEN))
        return;

    if (cleanup && spy->cookie.QuadPart)
    {
        CoRevokeInitializeSpy(spy->cookie);
        spy->cookie.QuadPart = 0;
    }

    if (!(spy->apt_flags & IMM_APT_INIT))
        return;
    spy->apt_flags &= ~IMM_APT_INIT;

    if (spy->apt_flags & IMM_APT_CREATED)
    {
        spy->apt_flags &= ~IMM_APT_CREATED;
        if (spy->apt_flags & IMM_APT_CAN_FREE)
            CoUninitialize();
    }
    if (cleanup)
        spy->apt_flags = 0;
}

static inline struct coinit_spy *impl_from_IInitializeSpy(IInitializeSpy *iface)
{
    return CONTAINING_RECORD(iface, struct coinit_spy, IInitializeSpy_iface);
}

static HRESULT WINAPI InitializeSpy_QueryInterface(IInitializeSpy *iface, REFIID riid, void **obj)
{
    if (IsEqualIID(&IID_IInitializeSpy, riid) ||
            IsEqualIID(&IID_IUnknown, riid))
    {
        *obj = iface;
        IInitializeSpy_AddRef(iface);
        return S_OK;
    }

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI InitializeSpy_AddRef(IInitializeSpy *iface)
{
    struct coinit_spy *spy = impl_from_IInitializeSpy(iface);
    return InterlockedIncrement(&spy->ref);
}

static ULONG WINAPI InitializeSpy_Release(IInitializeSpy *iface)
{
    struct coinit_spy *spy = impl_from_IInitializeSpy(iface);
    LONG ref = InterlockedDecrement(&spy->ref);
    if (!ref)
    {
        HeapFree(GetProcessHeap(), 0, spy);
        NtUserGetThreadInfo()->client_imm = 0;
    }
    return ref;
}

static HRESULT WINAPI InitializeSpy_PreInitialize(IInitializeSpy *iface,
        DWORD coinit, DWORD refs)
{
    struct coinit_spy *spy = impl_from_IInitializeSpy(iface);

    if ((spy->apt_flags & IMM_APT_CREATED) &&
            !(coinit & COINIT_APARTMENTTHREADED) && refs == 1)
    {
        imm_couninit_thread(TRUE);
        spy->apt_flags |= IMM_APT_BROKEN;
    }
    return S_OK;
}

static HRESULT WINAPI InitializeSpy_PostInitialize(IInitializeSpy *iface,
        HRESULT hr, DWORD coinit, DWORD refs)
{
    struct coinit_spy *spy = impl_from_IInitializeSpy(iface);

    if ((spy->apt_flags & IMM_APT_CREATED) && hr == S_FALSE && refs == 2)
        hr = S_OK;
    if (SUCCEEDED(hr))
        spy->apt_flags |= IMM_APT_CAN_FREE;
    return hr;
}

static HRESULT WINAPI InitializeSpy_PreUninitialize(IInitializeSpy *iface, DWORD refs)
{
    return S_OK;
}

static HRESULT WINAPI InitializeSpy_PostUninitialize(IInitializeSpy *iface, DWORD refs)
{
    struct coinit_spy *spy = impl_from_IInitializeSpy(iface);

    TRACE("%lu %p\n", refs, ImmGetDefaultIMEWnd(0));

    if (refs == 1 && !ImmGetDefaultIMEWnd(0))
        imm_couninit_thread(FALSE);
    else if (!refs)
        spy->apt_flags &= ~IMM_APT_CAN_FREE;
    return S_OK;
}

static const IInitializeSpyVtbl InitializeSpyVtbl =
{
    InitializeSpy_QueryInterface,
    InitializeSpy_AddRef,
    InitializeSpy_Release,
    InitializeSpy_PreInitialize,
    InitializeSpy_PostInitialize,
    InitializeSpy_PreUninitialize,
    InitializeSpy_PostUninitialize,
};

static void imm_coinit_thread(void)
{
    struct coinit_spy *spy;
    HRESULT hr;

    TRACE("implicit COM initialization\n");

    if (!(spy = get_thread_coinit_spy()))
    {
        if (!(spy = HeapAlloc(GetProcessHeap(), 0, sizeof(*spy)))) return;
        spy->IInitializeSpy_iface.lpVtbl = &InitializeSpyVtbl;
        spy->ref = 1;
        spy->cookie.QuadPart = 0;
        spy->apt_flags = 0;
        NtUserGetThreadInfo()->client_imm = (UINT_PTR)spy;

    }

    if (spy->apt_flags & (IMM_APT_INIT | IMM_APT_BROKEN))
        return;
    spy->apt_flags |= IMM_APT_INIT;

    if(!spy->cookie.QuadPart)
    {
        hr = CoRegisterInitializeSpy(&spy->IInitializeSpy_iface, &spy->cookie);
        if (FAILED(hr))
            return;
    }

    hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (SUCCEEDED(hr))
        spy->apt_flags |= IMM_APT_CREATED;
}

static BOOL IMM_IsDefaultContext(HIMC imc)
{
    InputContextData *data = get_imc_data(imc);

    if (!data)
        return FALSE;

    return data->threadDefault;
}

static InputContextData *query_imc_data(HIMC handle)
{
    InputContextData *ret;

    if (!handle) return NULL;
    ret = (void *)NtUserQueryInputContext(handle, NtUserInputContextClientPtr);
    return ret && ret->handle == handle ? ret : NULL;
}

static BOOL free_input_context_data(HIMC hIMC)
{
    InputContextData *data = query_imc_data(hIMC);

    if (!data)
        return FALSE;

    TRACE("Destroying %p\n", hIMC);

    data->immKbd->uSelected--;
    data->immKbd->pImeSelect(hIMC, FALSE);
    SendMessageW(data->IMC.hWnd, WM_IME_SELECT, FALSE, (LPARAM)data->immKbd);

    ImmDestroyIMCC(data->IMC.hCompStr);
    ImmDestroyIMCC(data->IMC.hCandInfo);
    ImmDestroyIMCC(data->IMC.hGuideLine);
    ImmDestroyIMCC(data->IMC.hPrivate);
    ImmDestroyIMCC(data->IMC.hMsgBuf);

    HeapFree(GetProcessHeap(), 0, data);

    return TRUE;
}

static void IMM_FreeThreadData(void)
{
    struct coinit_spy *spy;

    free_input_context_data(UlongToHandle(NtUserGetThreadInfo()->default_imc));
    if ((spy = get_thread_coinit_spy()))
        IInitializeSpy_Release(&spy->IInitializeSpy_iface);
}

static HMODULE load_graphics_driver(void)
{
    static const WCHAR key_pathW[] = L"System\\CurrentControlSet\\Control\\Video\\{";
    static const WCHAR displayW[] = L"}\\0000";

    HMODULE ret = 0;
    HKEY hkey;
    DWORD size;
    WCHAR path[MAX_PATH];
    WCHAR key[ARRAY_SIZE( key_pathW ) + ARRAY_SIZE( displayW ) + 40];
    UINT guid_atom = HandleToULong( GetPropW( GetDesktopWindow(), L"__wine_display_device_guid" ));

    if (!guid_atom) return 0;
    memcpy( key, key_pathW, sizeof(key_pathW) );
    if (!GlobalGetAtomNameW( guid_atom, key + lstrlenW(key), 40 )) return 0;
    lstrcatW( key, displayW );
    if (RegOpenKeyW( HKEY_LOCAL_MACHINE, key, &hkey )) return 0;
    size = sizeof(path);
    if (!RegQueryValueExW( hkey, L"GraphicsDriver", NULL, NULL, (BYTE *)path, &size ))
        ret = LoadLibraryW( path );
    RegCloseKey( hkey );
    TRACE( "%s %p\n", debugstr_w(path), ret );
    return ret;
}

/* ImmHkl loading and freeing */
#define LOAD_FUNCPTR(f) if((ptr->p##f = (LPVOID)GetProcAddress(ptr->hIME, #f)) == NULL){WARN("Can't find function %s in ime\n", #f);}
static ImmHkl *IMM_GetImmHkl(HKL hkl)
{
    ImmHkl *ptr;
    WCHAR filename[MAX_PATH];

    TRACE("Seeking ime for keyboard %p\n",hkl);

    LIST_FOR_EACH_ENTRY(ptr, &ImmHklList, ImmHkl, entry)
    {
        if (ptr->hkl == hkl)
            return ptr;
    }
    /* not found... create it */

    ptr = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(ImmHkl));

    ptr->hkl = hkl;
    if (ImmGetIMEFileNameW(hkl, filename, MAX_PATH)) ptr->hIME = LoadLibraryW(filename);
    if (!ptr->hIME) ptr->hIME = load_graphics_driver();
    if (ptr->hIME)
    {
        LOAD_FUNCPTR(ImeInquire);
        if (!ptr->pImeInquire || !ptr->pImeInquire(&ptr->imeInfo, ptr->imeClassName, NULL))
        {
            FreeLibrary(ptr->hIME);
            ptr->hIME = NULL;
        }
        else
        {
            LOAD_FUNCPTR(ImeDestroy);
            LOAD_FUNCPTR(ImeSelect);
            if (!ptr->pImeSelect || !ptr->pImeDestroy)
            {
                FreeLibrary(ptr->hIME);
                ptr->hIME = NULL;
            }
            else
            {
                LOAD_FUNCPTR(ImeConfigure);
                LOAD_FUNCPTR(ImeEscape);
                LOAD_FUNCPTR(ImeSetActiveContext);
                LOAD_FUNCPTR(ImeToAsciiEx);
                LOAD_FUNCPTR(NotifyIME);
                LOAD_FUNCPTR(ImeRegisterWord);
                LOAD_FUNCPTR(ImeUnregisterWord);
                LOAD_FUNCPTR(ImeEnumRegisterWord);
                LOAD_FUNCPTR(ImeSetCompositionString);
                LOAD_FUNCPTR(ImeConversionList);
                LOAD_FUNCPTR(ImeProcessKey);
                LOAD_FUNCPTR(ImeGetRegisterWordStyle);
                LOAD_FUNCPTR(ImeGetImeMenuItems);
                /* make sure our classname is WCHAR */
                if (!is_kbd_ime_unicode(ptr))
                {
                    WCHAR bufW[17];
                    MultiByteToWideChar(CP_ACP, 0, (LPSTR)ptr->imeClassName,
                                        -1, bufW, 17);
                    lstrcpyW(ptr->imeClassName, bufW);
                }
            }
        }
    }
    list_add_head(&ImmHklList,&ptr->entry);

    return ptr;
}
#undef LOAD_FUNCPTR


static void IMM_FreeAllImmHkl(void)
{
    ImmHkl *ptr,*cursor2;

    LIST_FOR_EACH_ENTRY_SAFE(ptr, cursor2, &ImmHklList, ImmHkl, entry)
    {
        list_remove(&ptr->entry);
        if (ptr->hIME)
        {
            ptr->pImeDestroy(1);
            FreeLibrary(ptr->hIME);
        }
        if (ptr->UIWnd)
            DestroyWindow(ptr->UIWnd);
        HeapFree(GetProcessHeap(),0,ptr);
    }
}

BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID lpReserved)
{
    TRACE("%p, %lx, %p\n",hInstDLL,fdwReason,lpReserved);
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            if (!User32InitializeImmEntryTable(IMM_INIT_MAGIC))
            {
                return FALSE;
            }
            break;
        case DLL_THREAD_ATTACH:
            break;
        case DLL_THREAD_DETACH:
            IMM_FreeThreadData();
            break;
        case DLL_PROCESS_DETACH:
            if (lpReserved) break;
            IMM_FreeThreadData();
            IMM_FreeAllImmHkl();
            break;
    }
    return TRUE;
}

/* for posting messages as the IME */
static void ImmInternalPostIMEMessage(InputContextData *data, UINT msg, WPARAM wParam, LPARAM lParam)
{
    HWND target = GetFocus();
    if (!target)
       PostMessageW(data->IMC.hWnd,msg,wParam,lParam);
    else
       PostMessageW(target, msg, wParam, lParam);
}

/* for sending messages as the IME */
static void ImmInternalSendIMEMessage(InputContextData *data, UINT msg, WPARAM wParam, LPARAM lParam)
{
    HWND target = GetFocus();
    if (!target)
       SendMessageW(data->IMC.hWnd,msg,wParam,lParam);
    else
       SendMessageW(target, msg, wParam, lParam);
}

static LRESULT ImmInternalSendIMENotify(InputContextData *data, WPARAM notify, LPARAM lParam)
{
    HWND target;

    target = data->IMC.hWnd;
    if (!target) target = GetFocus();

    if (target)
       return SendMessageW(target, WM_IME_NOTIFY, notify, lParam);

    return 0;
}

static HIMCC ImmCreateBlankCompStr(void)
{
    HIMCC rc;
    LPCOMPOSITIONSTRING ptr;
    rc = ImmCreateIMCC(sizeof(COMPOSITIONSTRING));
    ptr = ImmLockIMCC(rc);
    memset(ptr,0,sizeof(COMPOSITIONSTRING));
    ptr->dwSize = sizeof(COMPOSITIONSTRING);
    ImmUnlockIMCC(rc);
    return rc;
}

static BOOL IMM_IsCrossThreadAccess(HWND hWnd,  HIMC hIMC)
{
    InputContextData *data;

    if (hWnd)
    {
        DWORD thread = GetWindowThreadProcessId(hWnd, NULL);
        if (thread != GetCurrentThreadId()) return TRUE;
    }
    data = get_imc_data(hIMC);
    if (data && data->threadID != GetCurrentThreadId())
        return TRUE;

    return FALSE;
}

/***********************************************************************
 *		ImmSetActiveContext (IMM32.@)
 */
BOOL WINAPI ImmSetActiveContext(HWND hwnd, HIMC himc, BOOL activate)
{
    InputContextData *data = get_imc_data(himc);

    TRACE("(%p, %p, %x)\n", hwnd, himc, activate);

    if (himc && !data && activate)
        return FALSE;

    imm_coinit_thread();

    if (data)
    {
        data->IMC.hWnd = activate ? hwnd : NULL;

        if (data->immKbd->hIME && data->immKbd->pImeSetActiveContext)
            data->immKbd->pImeSetActiveContext(himc, activate);
    }

    if (IsWindow(hwnd))
    {
        SendMessageW(hwnd, WM_IME_SETCONTEXT, activate, ISC_SHOWUIALL);
        /* TODO: send WM_IME_NOTIFY */
    }
    SetLastError(0);
    return TRUE;
}

/***********************************************************************
 *		ImmAssociateContext (IMM32.@)
 */
HIMC WINAPI ImmAssociateContext(HWND hwnd, HIMC imc)
{
    HIMC old;
    UINT ret;

    TRACE("(%p, %p):\n", hwnd, imc);

    old = NtUserGetWindowInputContext(hwnd);
    ret = NtUserAssociateInputContext(hwnd, imc, 0);
    if (ret == AICR_FOCUS_CHANGED)
    {
        ImmSetActiveContext(hwnd, old, FALSE);
        ImmSetActiveContext(hwnd, imc, TRUE);
    }
    return ret == AICR_FAILED ? 0 : old;
}


/*
 * Helper function for ImmAssociateContextEx
 */
static BOOL CALLBACK _ImmAssociateContextExEnumProc(HWND hwnd, LPARAM lParam)
{
    HIMC hImc = (HIMC)lParam;
    ImmAssociateContext(hwnd,hImc);
    return TRUE;
}

/***********************************************************************
 *              ImmAssociateContextEx (IMM32.@)
 */
BOOL WINAPI ImmAssociateContextEx(HWND hwnd, HIMC imc, DWORD flags)
{
    HIMC old;
    UINT ret;

    TRACE("(%p, %p, 0x%lx):\n", hwnd, imc, flags);

    if (!hwnd)
        return FALSE;

    if (flags == IACE_CHILDREN)
    {
        EnumChildWindows(hwnd, _ImmAssociateContextExEnumProc, (LPARAM)imc);
        return TRUE;
    }

    old = NtUserGetWindowInputContext(hwnd);
    ret = NtUserAssociateInputContext(hwnd, imc, flags);
    if (ret == AICR_FOCUS_CHANGED)
    {
        ImmSetActiveContext(hwnd, old, FALSE);
        ImmSetActiveContext(hwnd, imc, TRUE);
    }
    return ret != AICR_FAILED;
}

/***********************************************************************
 *		ImmConfigureIMEA (IMM32.@)
 */
BOOL WINAPI ImmConfigureIMEA(
  HKL hKL, HWND hWnd, DWORD dwMode, LPVOID lpData)
{
    ImmHkl *immHkl = IMM_GetImmHkl(hKL);

    TRACE("(%p, %p, %ld, %p):\n", hKL, hWnd, dwMode, lpData);

    if (dwMode == IME_CONFIG_REGISTERWORD && !lpData)
        return FALSE;

    if (immHkl->hIME && immHkl->pImeConfigure)
    {
        if (dwMode != IME_CONFIG_REGISTERWORD || !is_kbd_ime_unicode(immHkl))
            return immHkl->pImeConfigure(hKL,hWnd,dwMode,lpData);
        else
        {
            REGISTERWORDW rww;
            REGISTERWORDA *rwa = lpData;
            BOOL rc;

            rww.lpReading = strdupAtoW(rwa->lpReading);
            rww.lpWord = strdupAtoW(rwa->lpWord);
            rc = immHkl->pImeConfigure(hKL,hWnd,dwMode,&rww);
            HeapFree(GetProcessHeap(),0,rww.lpReading);
            HeapFree(GetProcessHeap(),0,rww.lpWord);
            return rc;
        }
    }
    else
        return FALSE;
}

/***********************************************************************
 *		ImmConfigureIMEW (IMM32.@)
 */
BOOL WINAPI ImmConfigureIMEW(
  HKL hKL, HWND hWnd, DWORD dwMode, LPVOID lpData)
{
    ImmHkl *immHkl = IMM_GetImmHkl(hKL);

    TRACE("(%p, %p, %ld, %p):\n", hKL, hWnd, dwMode, lpData);

    if (dwMode == IME_CONFIG_REGISTERWORD && !lpData)
        return FALSE;

    if (immHkl->hIME && immHkl->pImeConfigure)
    {
        if (dwMode != IME_CONFIG_REGISTERWORD || is_kbd_ime_unicode(immHkl))
            return immHkl->pImeConfigure(hKL,hWnd,dwMode,lpData);
        else
        {
            REGISTERWORDW *rww = lpData;
            REGISTERWORDA rwa;
            BOOL rc;

            rwa.lpReading = strdupWtoA(rww->lpReading);
            rwa.lpWord = strdupWtoA(rww->lpWord);
            rc = immHkl->pImeConfigure(hKL,hWnd,dwMode,&rwa);
            HeapFree(GetProcessHeap(),0,rwa.lpReading);
            HeapFree(GetProcessHeap(),0,rwa.lpWord);
            return rc;
        }
    }
    else
        return FALSE;
}

static InputContextData *create_input_context(HIMC default_imc)
{
    InputContextData *new_context;
    LPGUIDELINE gl;
    LPCANDIDATEINFO ci;
    int i;

    new_context = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(InputContextData));

    /* Load the IME */
    new_context->threadDefault = !!default_imc;
    new_context->immKbd = IMM_GetImmHkl(GetKeyboardLayout(0));

    if (!new_context->immKbd->hIME)
    {
        TRACE("IME dll could not be loaded\n");
        HeapFree(GetProcessHeap(),0,new_context);
        return 0;
    }

    /* the HIMCCs are never NULL */
    new_context->IMC.hCompStr = ImmCreateBlankCompStr();
    new_context->IMC.hMsgBuf = ImmCreateIMCC(0);
    new_context->IMC.hCandInfo = ImmCreateIMCC(sizeof(CANDIDATEINFO));
    ci = ImmLockIMCC(new_context->IMC.hCandInfo);
    memset(ci,0,sizeof(CANDIDATEINFO));
    ci->dwSize = sizeof(CANDIDATEINFO);
    ImmUnlockIMCC(new_context->IMC.hCandInfo);
    new_context->IMC.hGuideLine = ImmCreateIMCC(sizeof(GUIDELINE));
    gl = ImmLockIMCC(new_context->IMC.hGuideLine);
    memset(gl,0,sizeof(GUIDELINE));
    gl->dwSize = sizeof(GUIDELINE);
    ImmUnlockIMCC(new_context->IMC.hGuideLine);

    for (i = 0; i < ARRAY_SIZE(new_context->IMC.cfCandForm); i++)
        new_context->IMC.cfCandForm[i].dwIndex = ~0u;

    /* Initialize the IME Private */
    new_context->IMC.hPrivate = ImmCreateIMCC(new_context->immKbd->imeInfo.dwPrivateDataSize);

    new_context->IMC.fdwConversion = new_context->immKbd->imeInfo.fdwConversionCaps;
    new_context->IMC.fdwSentence = new_context->immKbd->imeInfo.fdwSentenceCaps;

    if (!default_imc)
        new_context->handle = NtUserCreateInputContext((UINT_PTR)new_context);
    else if (NtUserUpdateInputContext(default_imc, NtUserInputContextClientPtr, (UINT_PTR)new_context))
        new_context->handle = default_imc;
    if (!new_context->handle)
    {
        free_input_context_data(new_context);
        return 0;
    }

    if (!new_context->immKbd->pImeSelect(new_context->handle, TRUE))
    {
        TRACE("Selection of IME failed\n");
        IMM_DestroyContext(new_context);
        return 0;
    }
    new_context->threadID = GetCurrentThreadId();
    SendMessageW(GetFocus(), WM_IME_SELECT, TRUE, (LPARAM)new_context->immKbd);

    new_context->immKbd->uSelected++;
    TRACE("Created context %p\n", new_context);
    return new_context;
}

static InputContextData* get_imc_data(HIMC handle)
{
    InputContextData *ret;

    if ((ret = query_imc_data(handle)) || !handle) return ret;
    return create_input_context(handle);
}

/***********************************************************************
 *		ImmCreateContext (IMM32.@)
 */
HIMC WINAPI ImmCreateContext(void)
{
    InputContextData *new_context;

    if (!(new_context = create_input_context(0))) return 0;
    return new_context->handle;
}

static BOOL IMM_DestroyContext(HIMC hIMC)
{
    if (!free_input_context_data(hIMC)) return FALSE;
    NtUserDestroyInputContext(hIMC);
    return TRUE;
}

/***********************************************************************
 *		ImmDestroyContext (IMM32.@)
 */
BOOL WINAPI ImmDestroyContext(HIMC hIMC)
{
    if (!IMM_IsDefaultContext(hIMC) && !IMM_IsCrossThreadAccess(NULL, hIMC))
        return IMM_DestroyContext(hIMC);
    else
        return FALSE;
}

/***********************************************************************
 *		ImmEnumRegisterWordA (IMM32.@)
 */
UINT WINAPI ImmEnumRegisterWordA(
  HKL hKL, REGISTERWORDENUMPROCA lpfnEnumProc,
  LPCSTR lpszReading, DWORD dwStyle,
  LPCSTR lpszRegister, LPVOID lpData)
{
    ImmHkl *immHkl = IMM_GetImmHkl(hKL);
    TRACE("(%p, %p, %s, %ld, %s, %p):\n", hKL, lpfnEnumProc,
        debugstr_a(lpszReading), dwStyle, debugstr_a(lpszRegister), lpData);
    if (immHkl->hIME && immHkl->pImeEnumRegisterWord)
    {
        if (!is_kbd_ime_unicode(immHkl))
            return immHkl->pImeEnumRegisterWord((REGISTERWORDENUMPROCW)lpfnEnumProc,
                (LPCWSTR)lpszReading, dwStyle, (LPCWSTR)lpszRegister, lpData);
        else
        {
            LPWSTR lpszwReading = strdupAtoW(lpszReading);
            LPWSTR lpszwRegister = strdupAtoW(lpszRegister);
            BOOL rc;

            rc = immHkl->pImeEnumRegisterWord((REGISTERWORDENUMPROCW)lpfnEnumProc,
                                              lpszwReading, dwStyle, lpszwRegister,
                                              lpData);

            HeapFree(GetProcessHeap(),0,lpszwReading);
            HeapFree(GetProcessHeap(),0,lpszwRegister);
            return rc;
        }
    }
    else
        return 0;
}

/***********************************************************************
 *		ImmEnumRegisterWordW (IMM32.@)
 */
UINT WINAPI ImmEnumRegisterWordW(
  HKL hKL, REGISTERWORDENUMPROCW lpfnEnumProc,
  LPCWSTR lpszReading, DWORD dwStyle,
  LPCWSTR lpszRegister, LPVOID lpData)
{
    ImmHkl *immHkl = IMM_GetImmHkl(hKL);
    TRACE("(%p, %p, %s, %ld, %s, %p):\n", hKL, lpfnEnumProc,
        debugstr_w(lpszReading), dwStyle, debugstr_w(lpszRegister), lpData);
    if (immHkl->hIME && immHkl->pImeEnumRegisterWord)
    {
        if (is_kbd_ime_unicode(immHkl))
            return immHkl->pImeEnumRegisterWord(lpfnEnumProc, lpszReading, dwStyle,
                                            lpszRegister, lpData);
        else
        {
            LPSTR lpszaReading = strdupWtoA(lpszReading);
            LPSTR lpszaRegister = strdupWtoA(lpszRegister);
            BOOL rc;

            rc = immHkl->pImeEnumRegisterWord(lpfnEnumProc, (LPCWSTR)lpszaReading,
                                              dwStyle, (LPCWSTR)lpszaRegister, lpData);

            HeapFree(GetProcessHeap(),0,lpszaReading);
            HeapFree(GetProcessHeap(),0,lpszaRegister);
            return rc;
        }
    }
    else
        return 0;
}

static inline BOOL EscapeRequiresWA(UINT uEscape)
{
        if (uEscape == IME_ESC_GET_EUDC_DICTIONARY ||
            uEscape == IME_ESC_SET_EUDC_DICTIONARY ||
            uEscape == IME_ESC_IME_NAME ||
            uEscape == IME_ESC_GETHELPFILENAME)
        return TRUE;
    return FALSE;
}

/***********************************************************************
 *		ImmEscapeA (IMM32.@)
 */
LRESULT WINAPI ImmEscapeA(
  HKL hKL, HIMC hIMC,
  UINT uEscape, LPVOID lpData)
{
    ImmHkl *immHkl = IMM_GetImmHkl(hKL);
    TRACE("(%p, %p, %d, %p):\n", hKL, hIMC, uEscape, lpData);

    if (immHkl->hIME && immHkl->pImeEscape)
    {
        if (!EscapeRequiresWA(uEscape) || !is_kbd_ime_unicode(immHkl))
            return immHkl->pImeEscape(hIMC,uEscape,lpData);
        else
        {
            WCHAR buffer[81]; /* largest required buffer should be 80 */
            LRESULT rc;
            if (uEscape == IME_ESC_SET_EUDC_DICTIONARY)
            {
                MultiByteToWideChar(CP_ACP,0,lpData,-1,buffer,81);
                rc = immHkl->pImeEscape(hIMC,uEscape,buffer);
            }
            else
            {
                rc = immHkl->pImeEscape(hIMC,uEscape,buffer);
                WideCharToMultiByte(CP_ACP,0,buffer,-1,lpData,80, NULL, NULL);
            }
            return rc;
        }
    }
    else
        return 0;
}

/***********************************************************************
 *		ImmEscapeW (IMM32.@)
 */
LRESULT WINAPI ImmEscapeW(
  HKL hKL, HIMC hIMC,
  UINT uEscape, LPVOID lpData)
{
    ImmHkl *immHkl = IMM_GetImmHkl(hKL);
    TRACE("(%p, %p, %d, %p):\n", hKL, hIMC, uEscape, lpData);

    if (immHkl->hIME && immHkl->pImeEscape)
    {
        if (!EscapeRequiresWA(uEscape) || is_kbd_ime_unicode(immHkl))
            return immHkl->pImeEscape(hIMC,uEscape,lpData);
        else
        {
            CHAR buffer[81]; /* largest required buffer should be 80 */
            LRESULT rc;
            if (uEscape == IME_ESC_SET_EUDC_DICTIONARY)
            {
                WideCharToMultiByte(CP_ACP,0,lpData,-1,buffer,81, NULL, NULL);
                rc = immHkl->pImeEscape(hIMC,uEscape,buffer);
            }
            else
            {
                rc = immHkl->pImeEscape(hIMC,uEscape,buffer);
                MultiByteToWideChar(CP_ACP,0,buffer,-1,lpData,80);
            }
            return rc;
        }
    }
    else
        return 0;
}

/***********************************************************************
 *		ImmGetCandidateListA (IMM32.@)
 */
DWORD WINAPI ImmGetCandidateListA(
  HIMC hIMC, DWORD dwIndex,
  LPCANDIDATELIST lpCandList, DWORD dwBufLen)
{
    InputContextData *data = get_imc_data(hIMC);
    LPCANDIDATEINFO candinfo;
    LPCANDIDATELIST candlist;
    DWORD ret = 0;

    TRACE("%p, %ld, %p, %ld\n", hIMC, dwIndex, lpCandList, dwBufLen);

    if (!data || !data->IMC.hCandInfo)
       return 0;

    candinfo = ImmLockIMCC(data->IMC.hCandInfo);
    if (dwIndex >= candinfo->dwCount || dwIndex >= ARRAY_SIZE(candinfo->dwOffset))
        goto done;

    candlist = (LPCANDIDATELIST)((LPBYTE)candinfo + candinfo->dwOffset[dwIndex]);
    if ( !candlist->dwSize || !candlist->dwCount )
        goto done;

    if ( !is_himc_ime_unicode(data) )
    {
        ret = candlist->dwSize;
        if ( lpCandList && dwBufLen >= ret )
            memcpy(lpCandList, candlist, ret);
    }
    else
        ret = convert_candidatelist_WtoA( candlist, lpCandList, dwBufLen);

done:
    ImmUnlockIMCC(data->IMC.hCandInfo);
    return ret;
}

/***********************************************************************
 *		ImmGetCandidateListCountA (IMM32.@)
 */
DWORD WINAPI ImmGetCandidateListCountA(
  HIMC hIMC, LPDWORD lpdwListCount)
{
    InputContextData *data = get_imc_data(hIMC);
    LPCANDIDATEINFO candinfo;
    DWORD ret, count;

    TRACE("%p, %p\n", hIMC, lpdwListCount);

    if (!data || !lpdwListCount || !data->IMC.hCandInfo)
       return 0;

    candinfo = ImmLockIMCC(data->IMC.hCandInfo);

    *lpdwListCount = count = candinfo->dwCount;

    if ( !is_himc_ime_unicode(data) )
        ret = candinfo->dwSize;
    else
    {
        ret = sizeof(CANDIDATEINFO);
        while ( count-- )
            ret += ImmGetCandidateListA(hIMC, count, NULL, 0);
    }

    ImmUnlockIMCC(data->IMC.hCandInfo);
    return ret;
}

/***********************************************************************
 *		ImmGetCandidateListCountW (IMM32.@)
 */
DWORD WINAPI ImmGetCandidateListCountW(
  HIMC hIMC, LPDWORD lpdwListCount)
{
    InputContextData *data = get_imc_data(hIMC);
    LPCANDIDATEINFO candinfo;
    DWORD ret, count;

    TRACE("%p, %p\n", hIMC, lpdwListCount);

    if (!data || !lpdwListCount || !data->IMC.hCandInfo)
       return 0;

    candinfo = ImmLockIMCC(data->IMC.hCandInfo);

    *lpdwListCount = count = candinfo->dwCount;

    if ( is_himc_ime_unicode(data) )
        ret = candinfo->dwSize;
    else
    {
        ret = sizeof(CANDIDATEINFO);
        while ( count-- )
            ret += ImmGetCandidateListW(hIMC, count, NULL, 0);
    }

    ImmUnlockIMCC(data->IMC.hCandInfo);
    return ret;
}

/***********************************************************************
 *		ImmGetCandidateListW (IMM32.@)
 */
DWORD WINAPI ImmGetCandidateListW(
  HIMC hIMC, DWORD dwIndex,
  LPCANDIDATELIST lpCandList, DWORD dwBufLen)
{
    InputContextData *data = get_imc_data(hIMC);
    LPCANDIDATEINFO candinfo;
    LPCANDIDATELIST candlist;
    DWORD ret = 0;

    TRACE("%p, %ld, %p, %ld\n", hIMC, dwIndex, lpCandList, dwBufLen);

    if (!data || !data->IMC.hCandInfo)
       return 0;

    candinfo = ImmLockIMCC(data->IMC.hCandInfo);
    if (dwIndex >= candinfo->dwCount || dwIndex >= ARRAY_SIZE(candinfo->dwOffset))
        goto done;

    candlist = (LPCANDIDATELIST)((LPBYTE)candinfo + candinfo->dwOffset[dwIndex]);
    if ( !candlist->dwSize || !candlist->dwCount )
        goto done;

    if ( is_himc_ime_unicode(data) )
    {
        ret = candlist->dwSize;
        if ( lpCandList && dwBufLen >= ret )
            memcpy(lpCandList, candlist, ret);
    }
    else
        ret = convert_candidatelist_AtoW( candlist, lpCandList, dwBufLen);

done:
    ImmUnlockIMCC(data->IMC.hCandInfo);
    return ret;
}

/***********************************************************************
 *		ImmGetCandidateWindow (IMM32.@)
 */
BOOL WINAPI ImmGetCandidateWindow(
  HIMC hIMC, DWORD dwIndex, LPCANDIDATEFORM lpCandidate)
{
    InputContextData *data = get_imc_data(hIMC);

    TRACE("%p, %ld, %p\n", hIMC, dwIndex, lpCandidate);

    if (!data || !lpCandidate)
        return FALSE;

    if (dwIndex >= ARRAY_SIZE(data->IMC.cfCandForm))
        return FALSE;

    if (data->IMC.cfCandForm[dwIndex].dwIndex != dwIndex)
        return FALSE;

    *lpCandidate = data->IMC.cfCandForm[dwIndex];

    return TRUE;
}

/***********************************************************************
 *		ImmGetCompositionFontA (IMM32.@)
 */
BOOL WINAPI ImmGetCompositionFontA(HIMC hIMC, LPLOGFONTA lplf)
{
    LOGFONTW lfW;
    BOOL rc;

    TRACE("(%p, %p):\n", hIMC, lplf);

    rc = ImmGetCompositionFontW(hIMC,&lfW);
    if (!rc || !lplf)
        return FALSE;

    memcpy(lplf,&lfW,sizeof(LOGFONTA));
    WideCharToMultiByte(CP_ACP, 0, lfW.lfFaceName, -1, lplf->lfFaceName,
                        LF_FACESIZE, NULL, NULL);
    return TRUE;
}

/***********************************************************************
 *		ImmGetCompositionFontW (IMM32.@)
 */
BOOL WINAPI ImmGetCompositionFontW(HIMC hIMC, LPLOGFONTW lplf)
{
    InputContextData *data = get_imc_data(hIMC);

    TRACE("(%p, %p):\n", hIMC, lplf);

    if (!data || !lplf)
        return FALSE;

    *lplf = data->IMC.lfFont.W;

    return TRUE;
}


/* Helpers for the GetCompositionString functions */

/* Source encoding is defined by context, source length is always given in respective characters. Destination buffer
   length is always in bytes. */
static INT CopyCompStringIMEtoClient(const InputContextData *data, const void *src, INT src_len, void *dst,
        INT dst_len, BOOL unicode)
{
    int char_size = unicode ? sizeof(WCHAR) : sizeof(char);
    INT ret;

    if (is_himc_ime_unicode(data) ^ unicode)
    {
        if (unicode)
            ret = MultiByteToWideChar(CP_ACP, 0, src, src_len, dst, dst_len / sizeof(WCHAR));
        else
            ret = WideCharToMultiByte(CP_ACP, 0, src, src_len, dst, dst_len, NULL, NULL);
        ret *= char_size;
    }
    else
    {
        if (dst_len)
        {
            ret = min(src_len * char_size, dst_len);
            memcpy(dst, src, ret);
        }
        else
            ret = src_len * char_size;
    }

    return ret;
}

/* Composition string encoding is defined by context, returned attributes correspond to string, converted according to
   passed mode. String length is in characters, attributes are in byte arrays. */
static INT CopyCompAttrIMEtoClient(const InputContextData *data, const BYTE *src, INT src_len, const void *comp_string,
        INT str_len, BYTE *dst, INT dst_len, BOOL unicode)
{
    union
    {
        const void *str;
        const WCHAR *strW;
        const char *strA;
    } string;
    INT rc;

    string.str = comp_string;

    if (is_himc_ime_unicode(data) && !unicode)
    {
        rc = WideCharToMultiByte(CP_ACP, 0, string.strW, str_len, NULL, 0, NULL, NULL);
        if (dst_len)
        {
            int i, j = 0, k = 0;

            if (rc < dst_len)
                dst_len = rc;
            for (i = 0; i < str_len; ++i)
            {
                int len;

                len = WideCharToMultiByte(CP_ACP, 0, string.strW + i, 1, NULL, 0, NULL, NULL);
                for (; len > 0; --len)
                {
                    dst[j++] = src[k];

                    if (j >= dst_len)
                        goto end;
                }
                ++k;
            }
        end:
            rc = j;
        }
    }
    else if (!is_himc_ime_unicode(data) && unicode)
    {
        rc = MultiByteToWideChar(CP_ACP, 0, string.strA, str_len, NULL, 0);
        if (dst_len)
        {
            int i, j = 0;

            if (rc < dst_len)
                dst_len = rc;
            for (i = 0; i < str_len; ++i)
            {
                if (IsDBCSLeadByte(string.strA[i]))
                    continue;

                dst[j++] = src[i];

                if (j >= dst_len)
                    break;
            }
            rc = j;
        }
    }
    else
    {
        memcpy(dst, src, min(src_len, dst_len));
        rc = src_len;
    }

    return rc;
}

static INT CopyCompClauseIMEtoClient(InputContextData *data, LPBYTE source, INT slen, LPBYTE ssource,
                                     LPBYTE target, INT tlen, BOOL unicode )
{
    INT rc;

    if (is_himc_ime_unicode(data) && !unicode)
    {
        if (tlen)
        {
            int i;

            if (slen < tlen)
                tlen = slen;
            tlen /= sizeof (DWORD);
            for (i = 0; i < tlen; ++i)
            {
                ((DWORD *)target)[i] = WideCharToMultiByte(CP_ACP, 0, (LPWSTR)ssource,
                                                          ((DWORD *)source)[i],
                                                          NULL, 0,
                                                          NULL, NULL);
            }
            rc = sizeof (DWORD) * i;
        }
        else
            rc = slen;
    }
    else if (!is_himc_ime_unicode(data) && unicode)
    {
        if (tlen)
        {
            int i;

            if (slen < tlen)
                tlen = slen;
            tlen /= sizeof (DWORD);
            for (i = 0; i < tlen; ++i)
            {
                ((DWORD *)target)[i] = MultiByteToWideChar(CP_ACP, 0, (LPSTR)ssource,
                                                          ((DWORD *)source)[i],
                                                          NULL, 0);
            }
            rc = sizeof (DWORD) * i;
        }
        else
            rc = slen;
    }
    else
    {
        memcpy( target, source, min(slen,tlen));
        rc = slen;
    }

    return rc;
}

static INT CopyCompOffsetIMEtoClient(InputContextData *data, DWORD offset, LPBYTE ssource, BOOL unicode)
{
    int rc;

    if (is_himc_ime_unicode(data) && !unicode)
    {
        rc = WideCharToMultiByte(CP_ACP, 0, (LPWSTR)ssource, offset, NULL, 0, NULL, NULL);
    }
    else if (!is_himc_ime_unicode(data) && unicode)
    {
        rc = MultiByteToWideChar(CP_ACP, 0, (LPSTR)ssource, offset, NULL, 0);
    }
    else
        rc = offset;

    return rc;
}

static LONG ImmGetCompositionStringT( HIMC hIMC, DWORD dwIndex, LPVOID lpBuf,
                                      DWORD dwBufLen, BOOL unicode)
{
    LONG rc = 0;
    InputContextData *data = get_imc_data(hIMC);
    LPCOMPOSITIONSTRING compstr;
    LPBYTE compdata;

    TRACE("(%p, 0x%lx, %p, %ld)\n", hIMC, dwIndex, lpBuf, dwBufLen);

    if (!data)
       return FALSE;

    if (!data->IMC.hCompStr)
       return FALSE;

    compdata = ImmLockIMCC(data->IMC.hCompStr);
    compstr = (LPCOMPOSITIONSTRING)compdata;

    switch (dwIndex)
    {
    case GCS_RESULTSTR:
        TRACE("GCS_RESULTSTR\n");
        rc = CopyCompStringIMEtoClient(data, compdata + compstr->dwResultStrOffset, compstr->dwResultStrLen, lpBuf, dwBufLen, unicode);
        break;
    case GCS_COMPSTR:
        TRACE("GCS_COMPSTR\n");
        rc = CopyCompStringIMEtoClient(data, compdata + compstr->dwCompStrOffset, compstr->dwCompStrLen, lpBuf, dwBufLen, unicode);
        break;
    case GCS_COMPATTR:
        TRACE("GCS_COMPATTR\n");
        rc = CopyCompAttrIMEtoClient(data, compdata + compstr->dwCompAttrOffset, compstr->dwCompAttrLen,
                                     compdata + compstr->dwCompStrOffset, compstr->dwCompStrLen,
                                     lpBuf, dwBufLen, unicode);
        break;
    case GCS_COMPCLAUSE:
        TRACE("GCS_COMPCLAUSE\n");
        rc = CopyCompClauseIMEtoClient(data, compdata + compstr->dwCompClauseOffset,compstr->dwCompClauseLen,
                                       compdata + compstr->dwCompStrOffset,
                                       lpBuf, dwBufLen, unicode);
        break;
    case GCS_RESULTCLAUSE:
        TRACE("GCS_RESULTCLAUSE\n");
        rc = CopyCompClauseIMEtoClient(data, compdata + compstr->dwResultClauseOffset,compstr->dwResultClauseLen,
                                       compdata + compstr->dwResultStrOffset,
                                       lpBuf, dwBufLen, unicode);
        break;
    case GCS_RESULTREADSTR:
        TRACE("GCS_RESULTREADSTR\n");
        rc = CopyCompStringIMEtoClient(data, compdata + compstr->dwResultReadStrOffset, compstr->dwResultReadStrLen, lpBuf, dwBufLen, unicode);
        break;
    case GCS_RESULTREADCLAUSE:
        TRACE("GCS_RESULTREADCLAUSE\n");
        rc = CopyCompClauseIMEtoClient(data, compdata + compstr->dwResultReadClauseOffset,compstr->dwResultReadClauseLen,
                                       compdata + compstr->dwResultStrOffset,
                                       lpBuf, dwBufLen, unicode);
        break;
    case GCS_COMPREADSTR:
        TRACE("GCS_COMPREADSTR\n");
        rc = CopyCompStringIMEtoClient(data, compdata + compstr->dwCompReadStrOffset, compstr->dwCompReadStrLen, lpBuf, dwBufLen, unicode);
        break;
    case GCS_COMPREADATTR:
        TRACE("GCS_COMPREADATTR\n");
        rc = CopyCompAttrIMEtoClient(data, compdata + compstr->dwCompReadAttrOffset, compstr->dwCompReadAttrLen,
                                     compdata + compstr->dwCompReadStrOffset, compstr->dwCompReadStrLen,
                                     lpBuf, dwBufLen, unicode);
        break;
    case GCS_COMPREADCLAUSE:
        TRACE("GCS_COMPREADCLAUSE\n");
        rc = CopyCompClauseIMEtoClient(data, compdata + compstr->dwCompReadClauseOffset,compstr->dwCompReadClauseLen,
                                       compdata + compstr->dwCompStrOffset,
                                       lpBuf, dwBufLen, unicode);
        break;
    case GCS_CURSORPOS:
        TRACE("GCS_CURSORPOS\n");
        rc = CopyCompOffsetIMEtoClient(data, compstr->dwCursorPos, compdata + compstr->dwCompStrOffset, unicode);
        break;
    case GCS_DELTASTART:
        TRACE("GCS_DELTASTART\n");
        rc = CopyCompOffsetIMEtoClient(data, compstr->dwDeltaStart, compdata + compstr->dwCompStrOffset, unicode);
        break;
    default:
        FIXME("Unhandled index 0x%lx\n",dwIndex);
        break;
    }

    ImmUnlockIMCC(data->IMC.hCompStr);

    return rc;
}

/***********************************************************************
 *		ImmGetCompositionStringA (IMM32.@)
 */
LONG WINAPI ImmGetCompositionStringA(
  HIMC hIMC, DWORD dwIndex, LPVOID lpBuf, DWORD dwBufLen)
{
    return ImmGetCompositionStringT(hIMC, dwIndex, lpBuf, dwBufLen, FALSE);
}


/***********************************************************************
 *		ImmGetCompositionStringW (IMM32.@)
 */
LONG WINAPI ImmGetCompositionStringW(
  HIMC hIMC, DWORD dwIndex,
  LPVOID lpBuf, DWORD dwBufLen)
{
    return ImmGetCompositionStringT(hIMC, dwIndex, lpBuf, dwBufLen, TRUE);
}

/***********************************************************************
 *		ImmGetCompositionWindow (IMM32.@)
 */
BOOL WINAPI ImmGetCompositionWindow(HIMC hIMC, LPCOMPOSITIONFORM lpCompForm)
{
    InputContextData *data = get_imc_data(hIMC);

    TRACE("(%p, %p)\n", hIMC, lpCompForm);

    if (!data)
        return FALSE;

    *lpCompForm = data->IMC.cfCompForm;
    return TRUE;
}

/***********************************************************************
 *		ImmGetContext (IMM32.@)
 *
 */
HIMC WINAPI ImmGetContext(HWND hWnd)
{
    HIMC rc;

    TRACE("%p\n", hWnd);

    rc = NtUserGetWindowInputContext(hWnd);

    if (rc)
    {
        InputContextData *data = get_imc_data(rc);
        if (data) data->IMC.hWnd = hWnd;
        else rc = 0;
    }

    TRACE("returning %p\n", rc);

    return rc;
}

/***********************************************************************
 *		ImmGetConversionListA (IMM32.@)
 */
DWORD WINAPI ImmGetConversionListA(
  HKL hKL, HIMC hIMC,
  LPCSTR pSrc, LPCANDIDATELIST lpDst,
  DWORD dwBufLen, UINT uFlag)
{
    ImmHkl *immHkl = IMM_GetImmHkl(hKL);
    TRACE("(%p, %p, %s, %p, %ld, %d):\n", hKL, hIMC, debugstr_a(pSrc), lpDst,
                dwBufLen, uFlag);
    if (immHkl->hIME && immHkl->pImeConversionList)
    {
        if (!is_kbd_ime_unicode(immHkl))
            return immHkl->pImeConversionList(hIMC,(LPCWSTR)pSrc,lpDst,dwBufLen,uFlag);
        else
        {
            LPCANDIDATELIST lpwDst;
            DWORD ret = 0, len;
            LPWSTR pwSrc = strdupAtoW(pSrc);

            len = immHkl->pImeConversionList(hIMC, pwSrc, NULL, 0, uFlag);
            lpwDst = HeapAlloc(GetProcessHeap(), 0, len);
            if ( lpwDst )
            {
                immHkl->pImeConversionList(hIMC, pwSrc, lpwDst, len, uFlag);
                ret = convert_candidatelist_WtoA( lpwDst, lpDst, dwBufLen);
                HeapFree(GetProcessHeap(), 0, lpwDst);
            }
            HeapFree(GetProcessHeap(), 0, pwSrc);

            return ret;
        }
    }
    else
        return 0;
}

/***********************************************************************
 *		ImmGetConversionListW (IMM32.@)
 */
DWORD WINAPI ImmGetConversionListW(
  HKL hKL, HIMC hIMC,
  LPCWSTR pSrc, LPCANDIDATELIST lpDst,
  DWORD dwBufLen, UINT uFlag)
{
    ImmHkl *immHkl = IMM_GetImmHkl(hKL);
    TRACE("(%p, %p, %s, %p, %ld, %d):\n", hKL, hIMC, debugstr_w(pSrc), lpDst,
                dwBufLen, uFlag);
    if (immHkl->hIME && immHkl->pImeConversionList)
    {
        if (is_kbd_ime_unicode(immHkl))
            return immHkl->pImeConversionList(hIMC,pSrc,lpDst,dwBufLen,uFlag);
        else
        {
            LPCANDIDATELIST lpaDst;
            DWORD ret = 0, len;
            LPSTR paSrc = strdupWtoA(pSrc);

            len = immHkl->pImeConversionList(hIMC, (LPCWSTR)paSrc, NULL, 0, uFlag);
            lpaDst = HeapAlloc(GetProcessHeap(), 0, len);
            if ( lpaDst )
            {
                immHkl->pImeConversionList(hIMC, (LPCWSTR)paSrc, lpaDst, len, uFlag);
                ret = convert_candidatelist_AtoW( lpaDst, lpDst, dwBufLen);
                HeapFree(GetProcessHeap(), 0, lpaDst);
            }
            HeapFree(GetProcessHeap(), 0, paSrc);

            return ret;
        }
    }
    else
        return 0;
}

/***********************************************************************
 *		ImmGetConversionStatus (IMM32.@)
 */
BOOL WINAPI ImmGetConversionStatus(
  HIMC hIMC, LPDWORD lpfdwConversion, LPDWORD lpfdwSentence)
{
    InputContextData *data = get_imc_data(hIMC);

    TRACE("%p %p %p\n", hIMC, lpfdwConversion, lpfdwSentence);

    if (!data)
        return FALSE;

    if (lpfdwConversion)
        *lpfdwConversion = data->IMC.fdwConversion;
    if (lpfdwSentence)
        *lpfdwSentence = data->IMC.fdwSentence;

    return TRUE;
}

/***********************************************************************
 *		ImmGetDefaultIMEWnd (IMM32.@)
 */
HWND WINAPI ImmGetDefaultIMEWnd(HWND hWnd)
{
    return NtUserGetDefaultImeWindow(hWnd);
}

/***********************************************************************
 *		ImmGetDescriptionA (IMM32.@)
 */
UINT WINAPI ImmGetDescriptionA(
  HKL hKL, LPSTR lpszDescription, UINT uBufLen)
{
  WCHAR *buf;
  DWORD len;

  TRACE("%p %p %d\n", hKL, lpszDescription, uBufLen);

  /* find out how many characters in the unicode buffer */
  len = ImmGetDescriptionW( hKL, NULL, 0 );
  if (!len)
    return 0;

  /* allocate a buffer of that size */
  buf = HeapAlloc( GetProcessHeap(), 0, (len + 1) * sizeof (WCHAR) );
  if( !buf )
  return 0;

  /* fetch the unicode buffer */
  len = ImmGetDescriptionW( hKL, buf, len + 1 );

  /* convert it back to ANSI */
  len = WideCharToMultiByte( CP_ACP, 0, buf, len + 1,
                             lpszDescription, uBufLen, NULL, NULL );

  HeapFree( GetProcessHeap(), 0, buf );

  if (len == 0)
    return 0;

  return len - 1;
}

/***********************************************************************
 *		ImmGetDescriptionW (IMM32.@)
 */
UINT WINAPI ImmGetDescriptionW(HKL hKL, LPWSTR lpszDescription, UINT uBufLen)
{
  FIXME("(%p, %p, %d): semi stub\n", hKL, lpszDescription, uBufLen);

  if (!hKL) return 0;
  if (!uBufLen) return lstrlenW(L"Wine XIM" );
  lstrcpynW( lpszDescription, L"Wine XIM", uBufLen );
  return lstrlenW( lpszDescription );
}

/***********************************************************************
 *		ImmGetGuideLineA (IMM32.@)
 */
DWORD WINAPI ImmGetGuideLineA(
  HIMC hIMC, DWORD dwIndex, LPSTR lpBuf, DWORD dwBufLen)
{
  FIXME("(%p, %ld, %s, %ld): stub\n",
    hIMC, dwIndex, debugstr_a(lpBuf), dwBufLen
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

/***********************************************************************
 *		ImmGetGuideLineW (IMM32.@)
 */
DWORD WINAPI ImmGetGuideLineW(HIMC hIMC, DWORD dwIndex, LPWSTR lpBuf, DWORD dwBufLen)
{
  FIXME("(%p, %ld, %s, %ld): stub\n",
    hIMC, dwIndex, debugstr_w(lpBuf), dwBufLen
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

/***********************************************************************
 *		ImmGetIMEFileNameA (IMM32.@)
 */
UINT WINAPI ImmGetIMEFileNameA( HKL hKL, LPSTR lpszFileName, UINT uBufLen)
{
    LPWSTR bufW = NULL;
    UINT wBufLen = uBufLen;
    UINT rc;

    if (uBufLen && lpszFileName)
        bufW = HeapAlloc(GetProcessHeap(),0,uBufLen * sizeof(WCHAR));
    else /* We need this to get the number of byte required */
    {
        bufW = HeapAlloc(GetProcessHeap(),0,MAX_PATH * sizeof(WCHAR));
        wBufLen = MAX_PATH;
    }

    rc = ImmGetIMEFileNameW(hKL,bufW,wBufLen);

    if (rc > 0)
    {
        if (uBufLen && lpszFileName)
            rc = WideCharToMultiByte(CP_ACP, 0, bufW, -1, lpszFileName,
                                 uBufLen, NULL, NULL);
        else /* get the length */
            rc = WideCharToMultiByte(CP_ACP, 0, bufW, -1, NULL, 0, NULL,
                                     NULL);
    }

    HeapFree(GetProcessHeap(),0,bufW);
    return rc;
}

/***********************************************************************
 *		ImmGetIMEFileNameW (IMM32.@)
 */
UINT WINAPI ImmGetIMEFileNameW(HKL hKL, LPWSTR lpszFileName, UINT uBufLen)
{
    HKEY hkey;
    DWORD length;
    DWORD rc;
    WCHAR regKey[ARRAY_SIZE(szImeRegFmt)+8];

    wsprintfW( regKey, szImeRegFmt, (ULONG_PTR)hKL );
    rc = RegOpenKeyW( HKEY_LOCAL_MACHINE, regKey, &hkey);
    if (rc != ERROR_SUCCESS)
    {
        SetLastError(rc);
        return 0;
    }

    length = 0;
    rc = RegGetValueW(hkey, NULL, L"Ime File", RRF_RT_REG_SZ, NULL, NULL, &length);

    if (rc != ERROR_SUCCESS)
    {
        RegCloseKey(hkey);
        SetLastError(rc);
        return 0;
    }
    if (length > uBufLen * sizeof(WCHAR) || !lpszFileName)
    {
        RegCloseKey(hkey);
        if (lpszFileName)
        {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return 0;
        }
        else
            return length / sizeof(WCHAR);
    }

    RegGetValueW(hkey, NULL, L"Ime File", RRF_RT_REG_SZ, NULL, lpszFileName, &length);

    RegCloseKey(hkey);

    return length / sizeof(WCHAR);
}

/***********************************************************************
 *		ImmGetOpenStatus (IMM32.@)
 */
BOOL WINAPI ImmGetOpenStatus(HIMC hIMC)
{
  InputContextData *data = get_imc_data(hIMC);
  static int i;

    if (!data)
        return FALSE;

    TRACE("(%p): semi-stub\n", hIMC);

    if (!i++)
      FIXME("(%p): semi-stub\n", hIMC);

  return data->IMC.fOpen;
}

/***********************************************************************
 *		ImmGetProperty (IMM32.@)
 */
DWORD WINAPI ImmGetProperty(HKL hKL, DWORD fdwIndex)
{
    DWORD rc = 0;
    ImmHkl *kbd;

    TRACE("(%p, %ld)\n", hKL, fdwIndex);
    kbd = IMM_GetImmHkl(hKL);

    if (kbd && kbd->hIME)
    {
        switch (fdwIndex)
        {
            case IGP_PROPERTY: rc = kbd->imeInfo.fdwProperty; break;
            case IGP_CONVERSION: rc = kbd->imeInfo.fdwConversionCaps; break;
            case IGP_SENTENCE: rc = kbd->imeInfo.fdwSentenceCaps; break;
            case IGP_SETCOMPSTR: rc = kbd->imeInfo.fdwSCSCaps; break;
            case IGP_SELECT: rc = kbd->imeInfo.fdwSelectCaps; break;
            case IGP_GETIMEVERSION: rc = IMEVER_0400; break;
            case IGP_UI: rc = 0; break;
            default: rc = 0;
        }
    }
    return rc;
}

/***********************************************************************
 *		ImmGetRegisterWordStyleA (IMM32.@)
 */
UINT WINAPI ImmGetRegisterWordStyleA(
  HKL hKL, UINT nItem, LPSTYLEBUFA lpStyleBuf)
{
    ImmHkl *immHkl = IMM_GetImmHkl(hKL);
    TRACE("(%p, %d, %p):\n", hKL, nItem, lpStyleBuf);
    if (immHkl->hIME && immHkl->pImeGetRegisterWordStyle)
    {
        if (!is_kbd_ime_unicode(immHkl))
            return immHkl->pImeGetRegisterWordStyle(nItem,(LPSTYLEBUFW)lpStyleBuf);
        else
        {
            STYLEBUFW sbw;
            UINT rc;

            rc = immHkl->pImeGetRegisterWordStyle(nItem,&sbw);
            WideCharToMultiByte(CP_ACP, 0, sbw.szDescription, -1,
                lpStyleBuf->szDescription, 32, NULL, NULL);
            lpStyleBuf->dwStyle = sbw.dwStyle;
            return rc;
        }
    }
    else
        return 0;
}

/***********************************************************************
 *		ImmGetRegisterWordStyleW (IMM32.@)
 */
UINT WINAPI ImmGetRegisterWordStyleW(
  HKL hKL, UINT nItem, LPSTYLEBUFW lpStyleBuf)
{
    ImmHkl *immHkl = IMM_GetImmHkl(hKL);
    TRACE("(%p, %d, %p):\n", hKL, nItem, lpStyleBuf);
    if (immHkl->hIME && immHkl->pImeGetRegisterWordStyle)
    {
        if (is_kbd_ime_unicode(immHkl))
            return immHkl->pImeGetRegisterWordStyle(nItem,lpStyleBuf);
        else
        {
            STYLEBUFA sba;
            UINT rc;

            rc = immHkl->pImeGetRegisterWordStyle(nItem,(LPSTYLEBUFW)&sba);
            MultiByteToWideChar(CP_ACP, 0, sba.szDescription, -1,
                lpStyleBuf->szDescription, 32);
            lpStyleBuf->dwStyle = sba.dwStyle;
            return rc;
        }
    }
    else
        return 0;
}

/***********************************************************************
 *		ImmGetStatusWindowPos (IMM32.@)
 */
BOOL WINAPI ImmGetStatusWindowPos(HIMC hIMC, LPPOINT lpptPos)
{
    InputContextData *data = get_imc_data(hIMC);

    TRACE("(%p, %p)\n", hIMC, lpptPos);

    if (!data || !lpptPos)
        return FALSE;

    *lpptPos = data->IMC.ptStatusWndPos;

    return TRUE;
}

/***********************************************************************
 *		ImmGetVirtualKey (IMM32.@)
 */
UINT WINAPI ImmGetVirtualKey(HWND hWnd)
{
  OSVERSIONINFOA version;
  InputContextData *data = get_imc_data( ImmGetContext( hWnd ));
  TRACE("%p\n", hWnd);

  if ( data )
      return data->lastVK;

  version.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);
  GetVersionExA( &version );
  switch(version.dwPlatformId)
  {
  case VER_PLATFORM_WIN32_WINDOWS:
      return VK_PROCESSKEY;
  case VER_PLATFORM_WIN32_NT:
      return 0;
  default:
      FIXME("%ld not supported\n",version.dwPlatformId);
      return VK_PROCESSKEY;
  }
}

/***********************************************************************
 *		ImmInstallIMEA (IMM32.@)
 */
HKL WINAPI ImmInstallIMEA(
  LPCSTR lpszIMEFileName, LPCSTR lpszLayoutText)
{
    LPWSTR lpszwIMEFileName;
    LPWSTR lpszwLayoutText;
    HKL hkl;

    TRACE ("(%s, %s)\n", debugstr_a(lpszIMEFileName),
                         debugstr_a(lpszLayoutText));

    lpszwIMEFileName = strdupAtoW(lpszIMEFileName);
    lpszwLayoutText = strdupAtoW(lpszLayoutText);

    hkl = ImmInstallIMEW(lpszwIMEFileName, lpszwLayoutText);

    HeapFree(GetProcessHeap(),0,lpszwIMEFileName);
    HeapFree(GetProcessHeap(),0,lpszwLayoutText);
    return hkl;
}

/***********************************************************************
 *		ImmInstallIMEW (IMM32.@)
 */
HKL WINAPI ImmInstallIMEW(
  LPCWSTR lpszIMEFileName, LPCWSTR lpszLayoutText)
{
    INT lcid = GetUserDefaultLCID();
    INT count;
    HKL hkl;
    DWORD rc;
    HKEY hkey;
    WCHAR regKey[ARRAY_SIZE(szImeRegFmt)+8];

    TRACE ("(%s, %s):\n", debugstr_w(lpszIMEFileName),
                          debugstr_w(lpszLayoutText));

    /* Start with 2.  e001 will be blank and so default to the wine internal IME */
    count = 2;

    while (count < 0xfff)
    {
        DWORD disposition = 0;

        hkl = (HKL)MAKELPARAM( lcid, 0xe000 | count );
        wsprintfW( regKey, szImeRegFmt, (ULONG_PTR)hkl);

        rc = RegCreateKeyExW(HKEY_LOCAL_MACHINE, regKey, 0, NULL, 0, KEY_WRITE, NULL, &hkey, &disposition);
        if (rc == ERROR_SUCCESS && disposition == REG_CREATED_NEW_KEY)
            break;
        else if (rc == ERROR_SUCCESS)
            RegCloseKey(hkey);

        count++;
    }

    if (count == 0xfff)
    {
        WARN("Unable to find slot to install IME\n");
        return 0;
    }

    if (rc == ERROR_SUCCESS)
    {
        rc = RegSetValueExW(hkey, L"Ime File", 0, REG_SZ, (const BYTE*)lpszIMEFileName,
                            (lstrlenW(lpszIMEFileName) + 1) * sizeof(WCHAR));
        if (rc == ERROR_SUCCESS)
            rc = RegSetValueExW(hkey, L"Layout Text", 0, REG_SZ, (const BYTE*)lpszLayoutText,
                                (lstrlenW(lpszLayoutText) + 1) * sizeof(WCHAR));
        RegCloseKey(hkey);
        return hkl;
    }
    else
    {
        WARN("Unable to set IME registry values\n");
        return 0;
    }
}

/***********************************************************************
 *		ImmIsIME (IMM32.@)
 */
BOOL WINAPI ImmIsIME(HKL hKL)
{
    ImmHkl *ptr;
    TRACE("(%p):\n", hKL);
    ptr = IMM_GetImmHkl(hKL);
    return (ptr && ptr->hIME);
}

/***********************************************************************
 *		ImmIsUIMessageA (IMM32.@)
 */
BOOL WINAPI ImmIsUIMessageA(
  HWND hWndIME, UINT msg, WPARAM wParam, LPARAM lParam)
{
    TRACE("(%p, %x, %Id, %Id)\n", hWndIME, msg, wParam, lParam);
    if ((msg >= WM_IME_STARTCOMPOSITION && msg <= WM_IME_KEYLAST) ||
            (msg == WM_IME_SETCONTEXT) ||
            (msg == WM_IME_NOTIFY) ||
            (msg == WM_IME_COMPOSITIONFULL) ||
            (msg == WM_IME_SELECT) ||
            (msg == 0x287 /* FIXME: WM_IME_SYSTEM */))
    {
        if (hWndIME)
            SendMessageA(hWndIME, msg, wParam, lParam);

        return TRUE;
    }
    return FALSE;
}

/***********************************************************************
 *		ImmIsUIMessageW (IMM32.@)
 */
BOOL WINAPI ImmIsUIMessageW(
  HWND hWndIME, UINT msg, WPARAM wParam, LPARAM lParam)
{
    TRACE("(%p, %x, %Id, %Id)\n", hWndIME, msg, wParam, lParam);
    if ((msg >= WM_IME_STARTCOMPOSITION && msg <= WM_IME_KEYLAST) ||
            (msg == WM_IME_SETCONTEXT) ||
            (msg == WM_IME_NOTIFY) ||
            (msg == WM_IME_COMPOSITIONFULL) ||
            (msg == WM_IME_SELECT) ||
            (msg == 0x287 /* FIXME: WM_IME_SYSTEM */))
    {
        if (hWndIME)
            SendMessageW(hWndIME, msg, wParam, lParam);

        return TRUE;
    }
    return FALSE;
}

/***********************************************************************
 *		ImmNotifyIME (IMM32.@)
 */
BOOL WINAPI ImmNotifyIME(
  HIMC hIMC, DWORD dwAction, DWORD dwIndex, DWORD dwValue)
{
    InputContextData *data = get_imc_data(hIMC);

    TRACE("(%p, %ld, %ld, %ld)\n",
        hIMC, dwAction, dwIndex, dwValue);

    if (hIMC == NULL)
    {
        SetLastError(ERROR_SUCCESS);
        return FALSE;
    }

    if (!data || ! data->immKbd->pNotifyIME)
    {
        return FALSE;
    }

    return data->immKbd->pNotifyIME(hIMC,dwAction,dwIndex,dwValue);
}

/***********************************************************************
 *		ImmRegisterWordA (IMM32.@)
 */
BOOL WINAPI ImmRegisterWordA(
  HKL hKL, LPCSTR lpszReading, DWORD dwStyle, LPCSTR lpszRegister)
{
    ImmHkl *immHkl = IMM_GetImmHkl(hKL);
    TRACE("(%p, %s, %ld, %s):\n", hKL, debugstr_a(lpszReading), dwStyle,
                    debugstr_a(lpszRegister));
    if (immHkl->hIME && immHkl->pImeRegisterWord)
    {
        if (!is_kbd_ime_unicode(immHkl))
            return immHkl->pImeRegisterWord((LPCWSTR)lpszReading,dwStyle,
                                            (LPCWSTR)lpszRegister);
        else
        {
            LPWSTR lpszwReading = strdupAtoW(lpszReading);
            LPWSTR lpszwRegister = strdupAtoW(lpszRegister);
            BOOL rc;

            rc = immHkl->pImeRegisterWord(lpszwReading,dwStyle,lpszwRegister);
            HeapFree(GetProcessHeap(),0,lpszwReading);
            HeapFree(GetProcessHeap(),0,lpszwRegister);
            return rc;
        }
    }
    else
        return FALSE;
}

/***********************************************************************
 *		ImmRegisterWordW (IMM32.@)
 */
BOOL WINAPI ImmRegisterWordW(
  HKL hKL, LPCWSTR lpszReading, DWORD dwStyle, LPCWSTR lpszRegister)
{
    ImmHkl *immHkl = IMM_GetImmHkl(hKL);
    TRACE("(%p, %s, %ld, %s):\n", hKL, debugstr_w(lpszReading), dwStyle,
                    debugstr_w(lpszRegister));
    if (immHkl->hIME && immHkl->pImeRegisterWord)
    {
        if (is_kbd_ime_unicode(immHkl))
            return immHkl->pImeRegisterWord(lpszReading,dwStyle,lpszRegister);
        else
        {
            LPSTR lpszaReading = strdupWtoA(lpszReading);
            LPSTR lpszaRegister = strdupWtoA(lpszRegister);
            BOOL rc;

            rc = immHkl->pImeRegisterWord((LPCWSTR)lpszaReading,dwStyle,
                                          (LPCWSTR)lpszaRegister);
            HeapFree(GetProcessHeap(),0,lpszaReading);
            HeapFree(GetProcessHeap(),0,lpszaRegister);
            return rc;
        }
    }
    else
        return FALSE;
}

/***********************************************************************
 *		ImmReleaseContext (IMM32.@)
 */
BOOL WINAPI ImmReleaseContext(HWND hWnd, HIMC hIMC)
{
  static BOOL shown = FALSE;

  if (!shown) {
     FIXME("(%p, %p): stub\n", hWnd, hIMC);
     shown = TRUE;
  }
  return TRUE;
}

/***********************************************************************
*              ImmRequestMessageA(IMM32.@)
*/
LRESULT WINAPI ImmRequestMessageA(HIMC hIMC, WPARAM wParam, LPARAM lParam)
{
    InputContextData *data = get_imc_data(hIMC);

    TRACE("%p %Id %Id\n", hIMC, wParam, wParam);

    if (data) return SendMessageA(data->IMC.hWnd, WM_IME_REQUEST, wParam, lParam);

    SetLastError(ERROR_INVALID_HANDLE);
    return 0;
}

/***********************************************************************
*              ImmRequestMessageW(IMM32.@)
*/
LRESULT WINAPI ImmRequestMessageW(HIMC hIMC, WPARAM wParam, LPARAM lParam)
{
    InputContextData *data = get_imc_data(hIMC);

    TRACE("%p %Id %Id\n", hIMC, wParam, wParam);

    if (data) return SendMessageW(data->IMC.hWnd, WM_IME_REQUEST, wParam, lParam);

    SetLastError(ERROR_INVALID_HANDLE);
    return 0;
}

/***********************************************************************
 *		ImmSetCandidateWindow (IMM32.@)
 */
BOOL WINAPI ImmSetCandidateWindow(
  HIMC hIMC, LPCANDIDATEFORM lpCandidate)
{
    InputContextData *data = get_imc_data(hIMC);

    TRACE("(%p, %p)\n", hIMC, lpCandidate);

    if (!data || !lpCandidate)
        return FALSE;

    if (IMM_IsCrossThreadAccess(NULL, hIMC))
        return FALSE;

    TRACE("\t%lx, %lx, %s, %s\n",
          lpCandidate->dwIndex, lpCandidate->dwStyle,
          wine_dbgstr_point(&lpCandidate->ptCurrentPos),
          wine_dbgstr_rect(&lpCandidate->rcArea));

    if (lpCandidate->dwIndex >= ARRAY_SIZE(data->IMC.cfCandForm))
        return FALSE;

    data->IMC.cfCandForm[lpCandidate->dwIndex] = *lpCandidate;
    ImmNotifyIME(hIMC, NI_CONTEXTUPDATED, 0, IMC_SETCANDIDATEPOS);
    ImmInternalSendIMENotify(data, IMN_SETCANDIDATEPOS, 1 << lpCandidate->dwIndex);

    return TRUE;
}

/***********************************************************************
 *		ImmSetCompositionFontA (IMM32.@)
 */
BOOL WINAPI ImmSetCompositionFontA(HIMC hIMC, LPLOGFONTA lplf)
{
    InputContextData *data = get_imc_data(hIMC);
    TRACE("(%p, %p)\n", hIMC, lplf);

    if (!data || !lplf)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if (IMM_IsCrossThreadAccess(NULL, hIMC))
        return FALSE;

    memcpy(&data->IMC.lfFont.W,lplf,sizeof(LOGFONTA));
    MultiByteToWideChar(CP_ACP, 0, lplf->lfFaceName, -1, data->IMC.lfFont.W.lfFaceName,
                        LF_FACESIZE);
    ImmNotifyIME(hIMC, NI_CONTEXTUPDATED, 0, IMC_SETCOMPOSITIONFONT);
    ImmInternalSendIMENotify(data, IMN_SETCOMPOSITIONFONT, 0);

    return TRUE;
}

/***********************************************************************
 *		ImmSetCompositionFontW (IMM32.@)
 */
BOOL WINAPI ImmSetCompositionFontW(HIMC hIMC, LPLOGFONTW lplf)
{
    InputContextData *data = get_imc_data(hIMC);
    TRACE("(%p, %p)\n", hIMC, lplf);

    if (!data || !lplf)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if (IMM_IsCrossThreadAccess(NULL, hIMC))
        return FALSE;

    data->IMC.lfFont.W = *lplf;
    ImmNotifyIME(hIMC, NI_CONTEXTUPDATED, 0, IMC_SETCOMPOSITIONFONT);
    ImmInternalSendIMENotify(data, IMN_SETCOMPOSITIONFONT, 0);

    return TRUE;
}

/***********************************************************************
 *		ImmSetCompositionStringA (IMM32.@)
 */
BOOL WINAPI ImmSetCompositionStringA(
  HIMC hIMC, DWORD dwIndex,
  LPCVOID lpComp, DWORD dwCompLen,
  LPCVOID lpRead, DWORD dwReadLen)
{
    DWORD comp_len;
    DWORD read_len;
    WCHAR *CompBuffer = NULL;
    WCHAR *ReadBuffer = NULL;
    BOOL rc;
    InputContextData *data = get_imc_data(hIMC);

    TRACE("(%p, %ld, %p, %ld, %p, %ld):\n",
            hIMC, dwIndex, lpComp, dwCompLen, lpRead, dwReadLen);

    if (!data)
        return FALSE;

    if (!(dwIndex == SCS_SETSTR ||
          dwIndex == SCS_CHANGEATTR ||
          dwIndex == SCS_CHANGECLAUSE ||
          dwIndex == SCS_SETRECONVERTSTRING ||
          dwIndex == SCS_QUERYRECONVERTSTRING))
        return FALSE;

    if (!is_himc_ime_unicode(data))
        return data->immKbd->pImeSetCompositionString(hIMC, dwIndex, lpComp,
                        dwCompLen, lpRead, dwReadLen);

    comp_len = MultiByteToWideChar(CP_ACP, 0, lpComp, dwCompLen, NULL, 0);
    if (comp_len)
    {
        CompBuffer = HeapAlloc(GetProcessHeap(),0,comp_len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, lpComp, dwCompLen, CompBuffer, comp_len);
    }

    read_len = MultiByteToWideChar(CP_ACP, 0, lpRead, dwReadLen, NULL, 0);
    if (read_len)
    {
        ReadBuffer = HeapAlloc(GetProcessHeap(),0,read_len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, lpRead, dwReadLen, ReadBuffer, read_len);
    }

    rc =  ImmSetCompositionStringW(hIMC, dwIndex, CompBuffer, comp_len,
                                   ReadBuffer, read_len);

    HeapFree(GetProcessHeap(), 0, CompBuffer);
    HeapFree(GetProcessHeap(), 0, ReadBuffer);

    return rc;
}

/***********************************************************************
 *		ImmSetCompositionStringW (IMM32.@)
 */
BOOL WINAPI ImmSetCompositionStringW(
	HIMC hIMC, DWORD dwIndex,
	LPCVOID lpComp, DWORD dwCompLen,
	LPCVOID lpRead, DWORD dwReadLen)
{
    DWORD comp_len;
    DWORD read_len;
    CHAR *CompBuffer = NULL;
    CHAR *ReadBuffer = NULL;
    BOOL rc;
    InputContextData *data = get_imc_data(hIMC);

    TRACE("(%p, %ld, %p, %ld, %p, %ld):\n",
            hIMC, dwIndex, lpComp, dwCompLen, lpRead, dwReadLen);

    if (!data)
        return FALSE;

    if (!(dwIndex == SCS_SETSTR ||
          dwIndex == SCS_CHANGEATTR ||
          dwIndex == SCS_CHANGECLAUSE ||
          dwIndex == SCS_SETRECONVERTSTRING ||
          dwIndex == SCS_QUERYRECONVERTSTRING))
        return FALSE;

    if (is_himc_ime_unicode(data))
        return data->immKbd->pImeSetCompositionString(hIMC, dwIndex, lpComp,
                        dwCompLen, lpRead, dwReadLen);

    comp_len = WideCharToMultiByte(CP_ACP, 0, lpComp, dwCompLen, NULL, 0, NULL,
                                   NULL);
    if (comp_len)
    {
        CompBuffer = HeapAlloc(GetProcessHeap(),0,comp_len);
        WideCharToMultiByte(CP_ACP, 0, lpComp, dwCompLen, CompBuffer, comp_len,
                            NULL, NULL);
    }

    read_len = WideCharToMultiByte(CP_ACP, 0, lpRead, dwReadLen, NULL, 0, NULL,
                                   NULL);
    if (read_len)
    {
        ReadBuffer = HeapAlloc(GetProcessHeap(),0,read_len);
        WideCharToMultiByte(CP_ACP, 0, lpRead, dwReadLen, ReadBuffer, read_len,
                            NULL, NULL);
    }

    rc =  ImmSetCompositionStringA(hIMC, dwIndex, CompBuffer, comp_len,
                                   ReadBuffer, read_len);

    HeapFree(GetProcessHeap(), 0, CompBuffer);
    HeapFree(GetProcessHeap(), 0, ReadBuffer);

    return rc;
}

/***********************************************************************
 *		ImmSetCompositionWindow (IMM32.@)
 */
BOOL WINAPI ImmSetCompositionWindow(
  HIMC hIMC, LPCOMPOSITIONFORM lpCompForm)
{
    BOOL reshow = FALSE;
    InputContextData *data = get_imc_data(hIMC);

    TRACE("(%p, %p)\n", hIMC, lpCompForm);
    if (lpCompForm)
        TRACE("\t%lx, %s, %s\n", lpCompForm->dwStyle,
              wine_dbgstr_point(&lpCompForm->ptCurrentPos),
              wine_dbgstr_rect(&lpCompForm->rcArea));

    if (!data)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if (IMM_IsCrossThreadAccess(NULL, hIMC))
        return FALSE;

    data->IMC.cfCompForm = *lpCompForm;

    if (IsWindowVisible(data->immKbd->UIWnd))
    {
        reshow = TRUE;
        ShowWindow(data->immKbd->UIWnd,SW_HIDE);
    }

    /* FIXME: this is a partial stub */

    if (reshow)
        ShowWindow(data->immKbd->UIWnd,SW_SHOWNOACTIVATE);

    ImmInternalSendIMENotify(data, IMN_SETCOMPOSITIONWINDOW, 0);
    return TRUE;
}

/***********************************************************************
 *		ImmSetConversionStatus (IMM32.@)
 */
BOOL WINAPI ImmSetConversionStatus(
  HIMC hIMC, DWORD fdwConversion, DWORD fdwSentence)
{
    DWORD oldConversion, oldSentence;
    InputContextData *data = get_imc_data(hIMC);

    TRACE("%p %ld %ld\n", hIMC, fdwConversion, fdwSentence);

    if (!data)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if (IMM_IsCrossThreadAccess(NULL, hIMC))
        return FALSE;

    if ( fdwConversion != data->IMC.fdwConversion )
    {
        oldConversion = data->IMC.fdwConversion;
        data->IMC.fdwConversion = fdwConversion;
        ImmNotifyIME(hIMC, NI_CONTEXTUPDATED, oldConversion, IMC_SETCONVERSIONMODE);
        ImmInternalSendIMENotify(data, IMN_SETCONVERSIONMODE, 0);
    }
    if ( fdwSentence != data->IMC.fdwSentence )
    {
        oldSentence = data->IMC.fdwSentence;
        data->IMC.fdwSentence = fdwSentence;
        ImmNotifyIME(hIMC, NI_CONTEXTUPDATED, oldSentence, IMC_SETSENTENCEMODE);
        ImmInternalSendIMENotify(data, IMN_SETSENTENCEMODE, 0);
    }

    return TRUE;
}

/***********************************************************************
 *		ImmSetOpenStatus (IMM32.@)
 */
BOOL WINAPI ImmSetOpenStatus(HIMC hIMC, BOOL fOpen)
{
    InputContextData *data = get_imc_data(hIMC);

    TRACE("%p %d\n", hIMC, fOpen);

    if (!data)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if (IMM_IsCrossThreadAccess(NULL, hIMC))
        return FALSE;

    if (data->immKbd->UIWnd == NULL)
    {
        /* create the ime window */
        data->immKbd->UIWnd = CreateWindowExW( WS_EX_TOOLWINDOW,
                    data->immKbd->imeClassName, NULL, WS_POPUP, 0, 0, 1, 1, 0,
                    0, data->immKbd->hIME, 0);
        SetWindowLongPtrW(data->immKbd->UIWnd, IMMGWL_IMC, (LONG_PTR)data);
    }
    else if (fOpen)
        SetWindowLongPtrW(data->immKbd->UIWnd, IMMGWL_IMC, (LONG_PTR)data);

    if (!fOpen != !data->IMC.fOpen)
    {
        data->IMC.fOpen = fOpen;
        ImmNotifyIME( hIMC, NI_CONTEXTUPDATED, 0, IMC_SETOPENSTATUS);
        ImmInternalSendIMENotify(data, IMN_SETOPENSTATUS, 0);
    }

    return TRUE;
}

/***********************************************************************
 *		ImmSetStatusWindowPos (IMM32.@)
 */
BOOL WINAPI ImmSetStatusWindowPos(HIMC hIMC, LPPOINT lpptPos)
{
    InputContextData *data = get_imc_data(hIMC);

    TRACE("(%p, %p)\n", hIMC, lpptPos);

    if (!data || !lpptPos)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if (IMM_IsCrossThreadAccess(NULL, hIMC))
        return FALSE;

    TRACE("\t%s\n", wine_dbgstr_point(lpptPos));

    data->IMC.ptStatusWndPos = *lpptPos;
    ImmNotifyIME( hIMC, NI_CONTEXTUPDATED, 0, IMC_SETSTATUSWINDOWPOS);
    ImmInternalSendIMENotify(data, IMN_SETSTATUSWINDOWPOS, 0);

    return TRUE;
}

/***********************************************************************
 *              ImmCreateSoftKeyboard(IMM32.@)
 */
HWND WINAPI ImmCreateSoftKeyboard(UINT uType, UINT hOwner, int x, int y)
{
    FIXME("(%d, %d, %d, %d): stub\n", uType, hOwner, x, y);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/***********************************************************************
 *              ImmDestroySoftKeyboard(IMM32.@)
 */
BOOL WINAPI ImmDestroySoftKeyboard(HWND hSoftWnd)
{
    FIXME("(%p): stub\n", hSoftWnd);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/***********************************************************************
 *              ImmShowSoftKeyboard(IMM32.@)
 */
BOOL WINAPI ImmShowSoftKeyboard(HWND hSoftWnd, int nCmdShow)
{
    FIXME("(%p, %d): stub\n", hSoftWnd, nCmdShow);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/***********************************************************************
 *		ImmSimulateHotKey (IMM32.@)
 */
BOOL WINAPI ImmSimulateHotKey(HWND hWnd, DWORD dwHotKeyID)
{
  FIXME("(%p, %ld): stub\n", hWnd, dwHotKeyID);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *		ImmUnregisterWordA (IMM32.@)
 */
BOOL WINAPI ImmUnregisterWordA(
  HKL hKL, LPCSTR lpszReading, DWORD dwStyle, LPCSTR lpszUnregister)
{
    ImmHkl *immHkl = IMM_GetImmHkl(hKL);
    TRACE("(%p, %s, %ld, %s):\n", hKL, debugstr_a(lpszReading), dwStyle,
            debugstr_a(lpszUnregister));
    if (immHkl->hIME && immHkl->pImeUnregisterWord)
    {
        if (!is_kbd_ime_unicode(immHkl))
            return immHkl->pImeUnregisterWord((LPCWSTR)lpszReading,dwStyle,
                                              (LPCWSTR)lpszUnregister);
        else
        {
            LPWSTR lpszwReading = strdupAtoW(lpszReading);
            LPWSTR lpszwUnregister = strdupAtoW(lpszUnregister);
            BOOL rc;

            rc = immHkl->pImeUnregisterWord(lpszwReading,dwStyle,lpszwUnregister);
            HeapFree(GetProcessHeap(),0,lpszwReading);
            HeapFree(GetProcessHeap(),0,lpszwUnregister);
            return rc;
        }
    }
    else
        return FALSE;
}

/***********************************************************************
 *		ImmUnregisterWordW (IMM32.@)
 */
BOOL WINAPI ImmUnregisterWordW(
  HKL hKL, LPCWSTR lpszReading, DWORD dwStyle, LPCWSTR lpszUnregister)
{
    ImmHkl *immHkl = IMM_GetImmHkl(hKL);
    TRACE("(%p, %s, %ld, %s):\n", hKL, debugstr_w(lpszReading), dwStyle,
            debugstr_w(lpszUnregister));
    if (immHkl->hIME && immHkl->pImeUnregisterWord)
    {
        if (is_kbd_ime_unicode(immHkl))
            return immHkl->pImeUnregisterWord(lpszReading,dwStyle,lpszUnregister);
        else
        {
            LPSTR lpszaReading = strdupWtoA(lpszReading);
            LPSTR lpszaUnregister = strdupWtoA(lpszUnregister);
            BOOL rc;

            rc = immHkl->pImeUnregisterWord((LPCWSTR)lpszaReading,dwStyle,
                                            (LPCWSTR)lpszaUnregister);
            HeapFree(GetProcessHeap(),0,lpszaReading);
            HeapFree(GetProcessHeap(),0,lpszaUnregister);
            return rc;
        }
    }
    else
        return FALSE;
}

/***********************************************************************
 *		ImmGetImeMenuItemsA (IMM32.@)
 */
DWORD WINAPI ImmGetImeMenuItemsA( HIMC hIMC, DWORD dwFlags, DWORD dwType,
   LPIMEMENUITEMINFOA lpImeParentMenu, LPIMEMENUITEMINFOA lpImeMenu,
    DWORD dwSize)
{
    InputContextData *data = get_imc_data(hIMC);
    TRACE("(%p, %li, %li, %p, %p, %li):\n", hIMC, dwFlags, dwType,
        lpImeParentMenu, lpImeMenu, dwSize);

    if (!data)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return 0;
    }

    if (data->immKbd->hIME && data->immKbd->pImeGetImeMenuItems)
    {
        if (!is_himc_ime_unicode(data) || (!lpImeParentMenu && !lpImeMenu))
            return data->immKbd->pImeGetImeMenuItems(hIMC, dwFlags, dwType,
                                (IMEMENUITEMINFOW*)lpImeParentMenu,
                                (IMEMENUITEMINFOW*)lpImeMenu, dwSize);
        else
        {
            IMEMENUITEMINFOW lpImeParentMenuW;
            IMEMENUITEMINFOW *lpImeMenuW, *parent = NULL;
            DWORD rc;

            if (lpImeParentMenu)
                parent = &lpImeParentMenuW;
            if (lpImeMenu)
            {
                int count = dwSize / sizeof(LPIMEMENUITEMINFOA);
                dwSize = count * sizeof(IMEMENUITEMINFOW);
                lpImeMenuW = HeapAlloc(GetProcessHeap(), 0, dwSize);
            }
            else
                lpImeMenuW = NULL;

            rc = data->immKbd->pImeGetImeMenuItems(hIMC, dwFlags, dwType,
                                parent, lpImeMenuW, dwSize);

            if (lpImeParentMenu)
            {
                memcpy(lpImeParentMenu,&lpImeParentMenuW,sizeof(IMEMENUITEMINFOA));
                lpImeParentMenu->hbmpItem = lpImeParentMenuW.hbmpItem;
                WideCharToMultiByte(CP_ACP, 0, lpImeParentMenuW.szString,
                    -1, lpImeParentMenu->szString, IMEMENUITEM_STRING_SIZE,
                    NULL, NULL);
            }
            if (lpImeMenu && rc)
            {
                unsigned int i;
                for (i = 0; i < rc; i++)
                {
                    memcpy(&lpImeMenu[i],&lpImeMenuW[1],sizeof(IMEMENUITEMINFOA));
                    lpImeMenu[i].hbmpItem = lpImeMenuW[i].hbmpItem;
                    WideCharToMultiByte(CP_ACP, 0, lpImeMenuW[i].szString,
                        -1, lpImeMenu[i].szString, IMEMENUITEM_STRING_SIZE,
                        NULL, NULL);
                }
            }
            HeapFree(GetProcessHeap(),0,lpImeMenuW);
            return rc;
        }
    }
    else
        return 0;
}

/***********************************************************************
*		ImmGetImeMenuItemsW (IMM32.@)
*/
DWORD WINAPI ImmGetImeMenuItemsW( HIMC hIMC, DWORD dwFlags, DWORD dwType,
   LPIMEMENUITEMINFOW lpImeParentMenu, LPIMEMENUITEMINFOW lpImeMenu,
   DWORD dwSize)
{
    InputContextData *data = get_imc_data(hIMC);
    TRACE("(%p, %li, %li, %p, %p, %li):\n", hIMC, dwFlags, dwType,
        lpImeParentMenu, lpImeMenu, dwSize);

    if (!data)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return 0;
    }

    if (data->immKbd->hIME && data->immKbd->pImeGetImeMenuItems)
    {
        if (is_himc_ime_unicode(data) || (!lpImeParentMenu && !lpImeMenu))
            return data->immKbd->pImeGetImeMenuItems(hIMC, dwFlags, dwType,
                                lpImeParentMenu, lpImeMenu, dwSize);
        else
        {
            IMEMENUITEMINFOA lpImeParentMenuA;
            IMEMENUITEMINFOA *lpImeMenuA, *parent = NULL;
            DWORD rc;

            if (lpImeParentMenu)
                parent = &lpImeParentMenuA;
            if (lpImeMenu)
            {
                int count = dwSize / sizeof(LPIMEMENUITEMINFOW);
                dwSize = count * sizeof(IMEMENUITEMINFOA);
                lpImeMenuA = HeapAlloc(GetProcessHeap(), 0, dwSize);
            }
            else
                lpImeMenuA = NULL;

            rc = data->immKbd->pImeGetImeMenuItems(hIMC, dwFlags, dwType,
                                (IMEMENUITEMINFOW*)parent,
                                (IMEMENUITEMINFOW*)lpImeMenuA, dwSize);

            if (lpImeParentMenu)
            {
                memcpy(lpImeParentMenu,&lpImeParentMenuA,sizeof(IMEMENUITEMINFOA));
                lpImeParentMenu->hbmpItem = lpImeParentMenuA.hbmpItem;
                MultiByteToWideChar(CP_ACP, 0, lpImeParentMenuA.szString,
                    -1, lpImeParentMenu->szString, IMEMENUITEM_STRING_SIZE);
            }
            if (lpImeMenu && rc)
            {
                unsigned int i;
                for (i = 0; i < rc; i++)
                {
                    memcpy(&lpImeMenu[i],&lpImeMenuA[1],sizeof(IMEMENUITEMINFOA));
                    lpImeMenu[i].hbmpItem = lpImeMenuA[i].hbmpItem;
                    MultiByteToWideChar(CP_ACP, 0, lpImeMenuA[i].szString,
                        -1, lpImeMenu[i].szString, IMEMENUITEM_STRING_SIZE);
                }
            }
            HeapFree(GetProcessHeap(),0,lpImeMenuA);
            return rc;
        }
    }
    else
        return 0;
}

/***********************************************************************
*		ImmLockIMC(IMM32.@)
*/
LPINPUTCONTEXT WINAPI ImmLockIMC(HIMC hIMC)
{
    InputContextData *data = get_imc_data(hIMC);

    if (!data)
        return NULL;
    data->dwLock++;
    return &data->IMC;
}

/***********************************************************************
*		ImmUnlockIMC(IMM32.@)
*/
BOOL WINAPI ImmUnlockIMC(HIMC hIMC)
{
    InputContextData *data = get_imc_data(hIMC);

    if (!data)
        return FALSE;
    if (data->dwLock)
        data->dwLock--;
    return TRUE;
}

/***********************************************************************
*		ImmGetIMCLockCount(IMM32.@)
*/
DWORD WINAPI ImmGetIMCLockCount(HIMC hIMC)
{
    InputContextData *data = get_imc_data(hIMC);
    if (!data)
        return 0;
    return data->dwLock;
}

/***********************************************************************
*		ImmCreateIMCC(IMM32.@)
*/
HIMCC  WINAPI ImmCreateIMCC(DWORD size)
{
    return GlobalAlloc(GMEM_ZEROINIT | GMEM_MOVEABLE, size);
}

/***********************************************************************
*       ImmDestroyIMCC(IMM32.@)
*/
HIMCC WINAPI ImmDestroyIMCC(HIMCC block)
{
    return GlobalFree(block);
}

/***********************************************************************
*		ImmLockIMCC(IMM32.@)
*/
LPVOID WINAPI ImmLockIMCC(HIMCC imcc)
{
    return GlobalLock(imcc);
}

/***********************************************************************
*		ImmUnlockIMCC(IMM32.@)
*/
BOOL WINAPI ImmUnlockIMCC(HIMCC imcc)
{
    return GlobalUnlock(imcc);
}

/***********************************************************************
*		ImmGetIMCCLockCount(IMM32.@)
*/
DWORD WINAPI ImmGetIMCCLockCount(HIMCC imcc)
{
    return GlobalFlags(imcc) & GMEM_LOCKCOUNT;
}

/***********************************************************************
*		ImmReSizeIMCC(IMM32.@)
*/
HIMCC  WINAPI ImmReSizeIMCC(HIMCC imcc, DWORD size)
{
    return GlobalReAlloc(imcc, size, GMEM_ZEROINIT | GMEM_MOVEABLE);
}

/***********************************************************************
*		ImmGetIMCCSize(IMM32.@)
*/
DWORD WINAPI ImmGetIMCCSize(HIMCC imcc)
{
    return GlobalSize(imcc);
}

/***********************************************************************
*		ImmGenerateMessage(IMM32.@)
*/
BOOL WINAPI ImmGenerateMessage(HIMC hIMC)
{
    InputContextData *data = get_imc_data(hIMC);

    if (!data)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    TRACE("%li messages queued\n",data->IMC.dwNumMsgBuf);
    if (data->IMC.dwNumMsgBuf > 0)
    {
        LPTRANSMSG lpTransMsg;
        HIMCC hMsgBuf;
        DWORD i, dwNumMsgBuf;

        /* We are going to detach our hMsgBuff so that if processing messages
           generates new messages they go into a new buffer */
        hMsgBuf = data->IMC.hMsgBuf;
        dwNumMsgBuf = data->IMC.dwNumMsgBuf;

        data->IMC.hMsgBuf = ImmCreateIMCC(0);
        data->IMC.dwNumMsgBuf = 0;

        lpTransMsg = ImmLockIMCC(hMsgBuf);
        for (i = 0; i < dwNumMsgBuf; i++)
            ImmInternalSendIMEMessage(data, lpTransMsg[i].message, lpTransMsg[i].wParam, lpTransMsg[i].lParam);

        ImmUnlockIMCC(hMsgBuf);
        ImmDestroyIMCC(hMsgBuf);
    }

    return TRUE;
}

/***********************************************************************
*       ImmTranslateMessage(IMM32.@)
*       ( Undocumented, call internally and from user32.dll )
*/
BOOL WINAPI ImmTranslateMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lKeyData)
{
    InputContextData *data;
    HIMC imc = ImmGetContext(hwnd);
    BYTE state[256];
    UINT scancode;
    LPVOID list = 0;
    UINT msg_count;
    UINT uVirtKey;
    static const DWORD list_count = 10;

    TRACE("%p %x %x %x\n",hwnd, msg, (UINT)wParam, (UINT)lKeyData);

    if (!(data = get_imc_data( imc ))) return FALSE;

    if (!data->immKbd->hIME || !data->immKbd->pImeToAsciiEx || data->lastVK == VK_PROCESSKEY)
        return FALSE;

    GetKeyboardState(state);
    scancode = lKeyData >> 0x10 & 0xff;

    list = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, list_count * sizeof(TRANSMSG) + sizeof(DWORD));
    ((DWORD*)list)[0] = list_count;

    if (data->immKbd->imeInfo.fdwProperty & IME_PROP_KBD_CHAR_FIRST)
    {
        WCHAR chr;

        if (!is_himc_ime_unicode(data))
            ToAscii(data->lastVK, scancode, state, &chr, 0);
        else
            ToUnicodeEx(data->lastVK, scancode, state, &chr, 1, 0, GetKeyboardLayout(0));
        uVirtKey = MAKELONG(data->lastVK,chr);
    }
    else
        uVirtKey = data->lastVK;

    msg_count = data->immKbd->pImeToAsciiEx(uVirtKey, scancode, state, list, 0, imc);
    TRACE("%i messages generated\n",msg_count);
    if (msg_count && msg_count <= list_count)
    {
        UINT i;
        LPTRANSMSG msgs = (LPTRANSMSG)((LPBYTE)list + sizeof(DWORD));

        for (i = 0; i < msg_count; i++)
            ImmInternalPostIMEMessage(data, msgs[i].message, msgs[i].wParam, msgs[i].lParam);
    }
    else if (msg_count > list_count)
        ImmGenerateMessage(imc);

    HeapFree(GetProcessHeap(),0,list);

    data->lastVK = VK_PROCESSKEY;

    return (msg_count > 0);
}

/***********************************************************************
*		ImmProcessKey(IMM32.@)
*       ( Undocumented, called from user32.dll )
*/
BOOL WINAPI ImmProcessKey(HWND hwnd, HKL hKL, UINT vKey, LPARAM lKeyData, DWORD unknown)
{
    InputContextData *data;
    HIMC imc = ImmGetContext(hwnd);
    BYTE state[256];

    TRACE("%p %p %x %x %lx\n",hwnd, hKL, vKey, (UINT)lKeyData, unknown);

    if (!(data = get_imc_data( imc ))) return FALSE;

    /* Make sure we are inputting to the correct keyboard */
    if (data->immKbd->hkl != hKL)
    {
        ImmHkl *new_hkl = IMM_GetImmHkl(hKL);
        if (new_hkl)
        {
            data->immKbd->pImeSelect(imc, FALSE);
            data->immKbd->uSelected--;
            data->immKbd = new_hkl;
            data->immKbd->pImeSelect(imc, TRUE);
            data->immKbd->uSelected++;
        }
        else
            return FALSE;
    }

    if (!data->immKbd->hIME || !data->immKbd->pImeProcessKey)
        return FALSE;

    GetKeyboardState(state);
    if (data->immKbd->pImeProcessKey(imc, vKey, lKeyData, state))
    {
        data->lastVK = vKey;
        return TRUE;
    }

    data->lastVK = VK_PROCESSKEY;
    return FALSE;
}

/***********************************************************************
*		ImmDisableTextFrameService(IMM32.@)
*/
BOOL WINAPI ImmDisableTextFrameService(DWORD idThread)
{
    FIXME("Stub\n");
    return FALSE;
}

/***********************************************************************
 *              ImmEnumInputContext(IMM32.@)
 */

BOOL WINAPI ImmEnumInputContext(DWORD idThread, IMCENUMPROC lpfn, LPARAM lParam)
{
    FIXME("Stub\n");
    return FALSE;
}

/***********************************************************************
 *              ImmGetHotKey(IMM32.@)
 */

BOOL WINAPI ImmGetHotKey(DWORD hotkey, UINT *modifiers, UINT *key, HKL hkl)
{
    FIXME("%lx, %p, %p, %p: stub\n", hotkey, modifiers, key, hkl);
    return FALSE;
}

/***********************************************************************
 *              ImmDisableLegacyIME(IMM32.@)
 */
BOOL WINAPI ImmDisableLegacyIME(void)
{
    FIXME("stub\n");
    return TRUE;
}

static HWND get_ui_window(HKL hkl)
{
    ImmHkl *immHkl = IMM_GetImmHkl(hkl);
    return immHkl->UIWnd;
}

static BOOL is_ime_ui_msg(UINT msg)
{
    switch (msg)
    {
    case WM_IME_STARTCOMPOSITION:
    case WM_IME_ENDCOMPOSITION:
    case WM_IME_COMPOSITION:
    case WM_IME_SETCONTEXT:
    case WM_IME_NOTIFY:
    case WM_IME_CONTROL:
    case WM_IME_COMPOSITIONFULL:
    case WM_IME_SELECT:
    case WM_IME_CHAR:
    case WM_IME_REQUEST:
    case WM_IME_KEYDOWN:
    case WM_IME_KEYUP:
        return TRUE;
    default:
        return msg == WM_MSIME_RECONVERTOPTIONS ||
            msg == WM_MSIME_SERVICE ||
            msg == WM_MSIME_MOUSE ||
            msg == WM_MSIME_RECONVERTREQUEST ||
            msg == WM_MSIME_RECONVERT ||
            msg == WM_MSIME_QUERYPOSITION ||
            msg == WM_MSIME_DOCUMENTFEED;
    }
}

static LRESULT ime_internal_msg( WPARAM wparam, LPARAM lparam)
{
    HWND hwnd = (HWND)lparam;
    HIMC himc;

    switch (wparam)
    {
    case IME_INTERNAL_ACTIVATE:
    case IME_INTERNAL_DEACTIVATE:
        himc = ImmGetContext(hwnd);
        ImmSetActiveContext(hwnd, himc, wparam == IME_INTERNAL_ACTIVATE);
        ImmReleaseContext(hwnd, himc);
        break;
    default:
        FIXME("wparam = %Ix\n", wparam);
        break;
    }

    return 0;
}

static void init_messages(void)
{
    static BOOL initialized;

    if (initialized) return;

    WM_MSIME_SERVICE = RegisterWindowMessageW(L"MSIMEService");
    WM_MSIME_RECONVERTOPTIONS = RegisterWindowMessageW(L"MSIMEReconvertOptions");
    WM_MSIME_MOUSE = RegisterWindowMessageW(L"MSIMEMouseOperation");
    WM_MSIME_RECONVERTREQUEST = RegisterWindowMessageW(L"MSIMEReconvertRequest");
    WM_MSIME_RECONVERT = RegisterWindowMessageW(L"MSIMEReconvert");
    WM_MSIME_QUERYPOSITION = RegisterWindowMessageW(L"MSIMEQueryPosition");
    WM_MSIME_DOCUMENTFEED = RegisterWindowMessageW(L"MSIMEDocumentFeed");
    initialized = TRUE;
}

LRESULT WINAPI __wine_ime_wnd_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam, BOOL ansi)
{
    HWND uiwnd;

    switch (msg)
    {
    case WM_CREATE:
        init_messages();
        return TRUE;

    case WM_DESTROY:
        {
            HWND default_hwnd = ImmGetDefaultIMEWnd(0);
            if (!default_hwnd || hwnd == default_hwnd)
                imm_couninit_thread(TRUE);
        }
        return TRUE;

    case WM_IME_INTERNAL:
        return ime_internal_msg(wparam, lparam);
    }

    if (is_ime_ui_msg(msg))
    {
        if ((uiwnd = get_ui_window(NtUserGetKeyboardLayout(0))))
        {
            if (ansi)
                return SendMessageA(uiwnd, msg, wparam, lparam);
            else
                return SendMessageW(uiwnd, msg, wparam, lparam);
        }
        return FALSE;
    }

    if (ansi)
        return DefWindowProcA(hwnd, msg, wparam, lparam);
    else
        return DefWindowProcW(hwnd, msg, wparam, lparam);
}
