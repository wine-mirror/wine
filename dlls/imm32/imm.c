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
#include "initguid.h"
#include "imm_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(imm);

#define IMM_INIT_MAGIC 0x19650412
BOOL WINAPI User32InitializeImmEntryTable(DWORD);

HMODULE imm32_module;

/* MSIME messages */
UINT WM_MSIME_SERVICE;
UINT WM_MSIME_RECONVERTOPTIONS;
UINT WM_MSIME_MOUSE;
UINT WM_MSIME_RECONVERTREQUEST;
UINT WM_MSIME_RECONVERT;
UINT WM_MSIME_QUERYPOSITION;
UINT WM_MSIME_DOCUMENTFEED;

struct imc_entry
{
    HIMC himc;
    INPUTCONTEXT context;
    struct list entry;
};

struct ime
{
    LONG refcount; /* guarded by ime_cs */

    HKL hkl;
    HMODULE module;
    struct list entry;

    IMEINFO info;
    WCHAR ui_class[17];
    struct list input_contexts;

    BOOL (WINAPI *pImeInquire)(IMEINFO *, void *, DWORD);
    BOOL (WINAPI *pImeConfigure)(HKL, HWND, DWORD, void *);
    BOOL (WINAPI *pImeDestroy)(UINT);
    LRESULT (WINAPI *pImeEscape)(HIMC, UINT, void *);
    BOOL (WINAPI *pImeSelect)(HIMC, BOOL);
    BOOL (WINAPI *pImeSetActiveContext)(HIMC, BOOL);
    UINT (WINAPI *pImeToAsciiEx)(UINT, UINT, const BYTE *, TRANSMSGLIST *, UINT, HIMC);
    BOOL (WINAPI *pNotifyIME)(HIMC, DWORD, DWORD, DWORD);
    BOOL (WINAPI *pImeRegisterWord)(const void/*TCHAR*/*, DWORD, const void/*TCHAR*/*);
    BOOL (WINAPI *pImeUnregisterWord)(const void/*TCHAR*/*, DWORD, const void/*TCHAR*/*);
    UINT (WINAPI *pImeEnumRegisterWord)(void */*REGISTERWORDENUMPROCW*/, const void/*TCHAR*/*, DWORD, const void/*TCHAR*/*, void *);
    BOOL (WINAPI *pImeSetCompositionString)(HIMC, DWORD, const void/*TCHAR*/*, DWORD, const void/*TCHAR*/*, DWORD);
    DWORD (WINAPI *pImeConversionList)(HIMC, const void/*TCHAR*/*, CANDIDATELIST*, DWORD, UINT);
    UINT (WINAPI *pImeGetRegisterWordStyle)(UINT, void/*STYLEBUFW*/*);
    BOOL (WINAPI *pImeProcessKey)(HIMC, UINT, LPARAM, const BYTE*);
    DWORD (WINAPI *pImeGetImeMenuItems)(HIMC, DWORD, DWORD, void/*IMEMENUITEMINFOW*/*, void/*IMEMENUITEMINFOW*/*, DWORD);
};

static HRESULT (WINAPI *pCoRevokeInitializeSpy)(ULARGE_INTEGER cookie);
static void (WINAPI *pCoUninitialize)(void);

struct imc
{
        HIMC            handle;
        DWORD           dwLock;
        INPUTCONTEXT    IMC;

    struct ime *ime;
    UINT vkey;

    HWND ui_hwnd; /* IME UI window, on the default input context */
};

#define WINE_IMC_VALID_MAGIC 0x56434D49

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

static CRITICAL_SECTION ime_cs;
static CRITICAL_SECTION_DEBUG ime_cs_debug =
{
    0, 0, &ime_cs,
    { &ime_cs_debug.ProcessLocksList, &ime_cs_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": ime_cs") }
};
static CRITICAL_SECTION ime_cs = { &ime_cs_debug, -1, 0, 0, 0, 0 };
static struct list ime_list = LIST_INIT( ime_list );

static const WCHAR layouts_formatW[] = L"System\\CurrentControlSet\\Control\\Keyboard Layouts\\%08lx";

static const char *debugstr_composition( const COMPOSITIONFORM *composition )
{
    if (!composition) return "(null)";
    return wine_dbg_sprintf( "style %#lx, pos %s, area %s", composition->dwStyle,
                             wine_dbgstr_point( &composition->ptCurrentPos ),
                             wine_dbgstr_rect( &composition->rcArea ) );
}

static const char *debugstr_candidate( const CANDIDATEFORM *candidate )
{
    if (!candidate) return "(null)";
    return wine_dbg_sprintf( "idx %#lx, style %#lx, pos %s, area %s", candidate->dwIndex,
                             candidate->dwStyle, wine_dbgstr_point( &candidate->ptCurrentPos ),
                             wine_dbgstr_rect( &candidate->rcArea ) );
}

static BOOL ime_is_unicode( const struct ime *ime )
{
    return !!(ime->info.fdwProperty & IME_PROP_UNICODE);
}

static BOOL input_context_is_unicode( INPUTCONTEXT *ctx )
{
    struct imc *imc = CONTAINING_RECORD( ctx, struct imc, IMC );
    return !imc->ime || ime_is_unicode( imc->ime );
}

static BOOL IMM_DestroyContext(HIMC hIMC);
static struct imc *get_imc_data( HIMC hIMC );

static inline WCHAR *strdupAtoW( const char *str )
{
    WCHAR *ret = NULL;
    if (str)
    {
        DWORD len = MultiByteToWideChar( CP_ACP, 0, str, -1, NULL, 0 );
        if ((ret = malloc( len * sizeof(WCHAR) ))) MultiByteToWideChar( CP_ACP, 0, str, -1, ret, len );
    }
    return ret;
}

static inline CHAR *strdupWtoA( const WCHAR *str )
{
    CHAR *ret = NULL;
    if (str)
    {
        DWORD len = WideCharToMultiByte( CP_ACP, 0, str, -1, NULL, 0, NULL, NULL );
        if ((ret = malloc( len ))) WideCharToMultiByte( CP_ACP, 0, str, -1, ret, len, NULL, NULL );
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
        pCoRevokeInitializeSpy(spy->cookie);
        spy->cookie.QuadPart = 0;
    }

    if (!(spy->apt_flags & IMM_APT_INIT))
        return;
    spy->apt_flags &= ~IMM_APT_INIT;

    if (spy->apt_flags & IMM_APT_CREATED)
    {
        spy->apt_flags &= ~IMM_APT_CREATED;
        if (spy->apt_flags & IMM_APT_CAN_FREE)
            pCoUninitialize();
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
        free( spy );
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

static BOOL WINAPI init_ole32_funcs( INIT_ONCE *once, void *param, void **context )
{
    HMODULE module_ole32 = GetModuleHandleA("ole32");
    pCoRevokeInitializeSpy = (void*)GetProcAddress(module_ole32, "CoRevokeInitializeSpy");
    pCoUninitialize = (void*)GetProcAddress(module_ole32, "CoUninitialize");
    return TRUE;
}

static void imm_coinit_thread(void)
{
    struct coinit_spy *spy;
    HRESULT hr;
    static INIT_ONCE init_ole32_once = INIT_ONCE_STATIC_INIT;

    TRACE("implicit COM initialization\n");

    if (!(spy = get_thread_coinit_spy()))
    {
        if (!(spy = malloc( sizeof(*spy) ))) return;
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

    InitOnceExecuteOnce(&init_ole32_once, init_ole32_funcs, NULL, NULL);
}

static struct imc *query_imc_data( HIMC handle )
{
    struct imc *ret;

    if (!handle) return NULL;
    ret = (void *)NtUserQueryInputContext(handle, NtUserInputContextClientPtr);
    return ret && ret->handle == handle ? ret : NULL;
}

/* lookup an IME from a HKL, must hold ime_cs */
static struct ime *find_ime_from_hkl( HKL hkl )
{
    struct ime *ime = NULL;
    LIST_FOR_EACH_ENTRY( ime, &ime_list, struct ime, entry )
        if (ime->hkl == hkl) return ime;
    return NULL;
}

BOOL WINAPI ImmFreeLayout( HKL hkl )
{
    struct imc_entry *imc_entry, *imc_next;
    struct ime *ime;

    TRACE( "hkl %p\n", hkl );

    EnterCriticalSection( &ime_cs );
    if ((ime = find_ime_from_hkl( hkl )))
    {
        list_remove( &ime->entry );
        if (!ime->pImeDestroy( 0 )) WARN( "ImeDestroy failed\n" );
        LIST_FOR_EACH_ENTRY_SAFE( imc_entry, imc_next, &ime->input_contexts, struct imc_entry, entry )
        {
            ImmDestroyIMCC( imc_entry->context.hPrivate );
            free( imc_entry );
        }
    }
    LeaveCriticalSection( &ime_cs );
    if (!ime) return TRUE;

    FreeLibrary( ime->module );
    free( ime );
    return TRUE;
}

BOOL WINAPI ImmLoadIME( HKL hkl )
{
    WCHAR buffer[MAX_PATH] = {0};
    BOOL use_default_ime;
    struct ime *ime;

    TRACE( "hkl %p\n", hkl );

    EnterCriticalSection( &ime_cs );
    if ((ime = find_ime_from_hkl( hkl )) || !(ime = calloc( 1, sizeof(*ime) )))
    {
        LeaveCriticalSection( &ime_cs );
        return !!ime;
    }

    if (!ImmGetIMEFileNameW( hkl, buffer, MAX_PATH )) use_default_ime = TRUE;
    else if (!(ime->module = LoadLibraryW( buffer ))) use_default_ime = TRUE;
    else use_default_ime = FALSE;

    if (use_default_ime)
    {
        if (*buffer) WARN( "Failed to load %s, falling back to default.\n", debugstr_w(buffer) );
        ime->module = LoadLibraryW( L"imm32" );
        ime->pImeInquire = (void *)ImeInquire;
        ime->pImeDestroy = ImeDestroy;
        ime->pImeSelect = ImeSelect;
        ime->pImeConfigure = ImeConfigure;
        ime->pImeEscape = ImeEscape;
        ime->pImeSetActiveContext = ImeSetActiveContext;
        ime->pImeToAsciiEx = (void *)ImeToAsciiEx;
        ime->pNotifyIME = NotifyIME;
        ime->pImeRegisterWord = (void *)ImeRegisterWord;
        ime->pImeUnregisterWord = (void *)ImeUnregisterWord;
        ime->pImeEnumRegisterWord = (void *)ImeEnumRegisterWord;
        ime->pImeSetCompositionString = ImeSetCompositionString;
        ime->pImeConversionList = (void *)ImeConversionList;
        ime->pImeProcessKey = (void *)ImeProcessKey;
        ime->pImeGetRegisterWordStyle = (void *)ImeGetRegisterWordStyle;
        ime->pImeGetImeMenuItems = (void *)ImeGetImeMenuItems;
    }
    else
    {
#define LOAD_FUNCPTR( f )                                                \
        if (!(ime->p##f = (void *)GetProcAddress( ime->module, #f )))    \
        {                                                                \
            WARN( "Can't find function %s in HKL %p IME\n", #f, hkl );   \
            goto failed;                                                 \
        }

        LOAD_FUNCPTR( ImeInquire );
        LOAD_FUNCPTR( ImeDestroy );
        LOAD_FUNCPTR( ImeSelect );
        LOAD_FUNCPTR( ImeConfigure );
        LOAD_FUNCPTR( ImeEscape );
        LOAD_FUNCPTR( ImeSetActiveContext );
        LOAD_FUNCPTR( ImeToAsciiEx );
        LOAD_FUNCPTR( NotifyIME );
        LOAD_FUNCPTR( ImeRegisterWord );
        LOAD_FUNCPTR( ImeUnregisterWord );
        LOAD_FUNCPTR( ImeEnumRegisterWord );
        LOAD_FUNCPTR( ImeSetCompositionString );
        LOAD_FUNCPTR( ImeConversionList );
        LOAD_FUNCPTR( ImeProcessKey );
        LOAD_FUNCPTR( ImeGetRegisterWordStyle );
        LOAD_FUNCPTR( ImeGetImeMenuItems );
#undef LOAD_FUNCPTR
    }

    ime->hkl = hkl;
    if (!ime->pImeInquire( &ime->info, buffer, 0 )) goto failed;

    if (ime_is_unicode( ime )) lstrcpynW( ime->ui_class, buffer, ARRAY_SIZE(ime->ui_class) );
    else MultiByteToWideChar( CP_ACP, 0, (char *)buffer, -1, ime->ui_class, ARRAY_SIZE(ime->ui_class) );
    list_init( &ime->input_contexts );

    list_add_tail( &ime_list, &ime->entry );
    LeaveCriticalSection( &ime_cs );

    TRACE( "Created IME %p for HKL %p\n", ime, hkl );
    return TRUE;

failed:
    LeaveCriticalSection( &ime_cs );

    if (ime->module) FreeLibrary( ime->module );
    free( ime );
    return FALSE;
}

static struct ime *ime_acquire( HKL hkl )
{
    struct ime *ime;

    EnterCriticalSection( &ime_cs );

    if (!ImmLoadIME( hkl )) ime = NULL;
    else ime = find_ime_from_hkl( hkl );

    if (ime)
    {
        ULONG ref = ++ime->refcount;
        TRACE( "ime %p increasing refcount to %lu.\n", ime, ref );
    }

    LeaveCriticalSection( &ime_cs );

    return ime;
}

static void ime_release( struct ime *ime )
{
    ULONG ref;

    EnterCriticalSection( &ime_cs );

    ref = --ime->refcount;
    TRACE( "ime %p decreasing refcount to %lu.\n", ime, ref );

    if (!ref && (ime->info.fdwProperty & IME_PROP_END_UNLOAD))
        ImmFreeLayout( ime->hkl );

    LeaveCriticalSection( &ime_cs );
}

static void ime_save_input_context( struct ime *ime, HIMC himc, INPUTCONTEXT *ctx )
{
    static INPUTCONTEXT default_input_context =
    {
        .cfCandForm = {{.dwIndex = -1}, {.dwIndex = -1}, {.dwIndex = -1}, {.dwIndex = -1}}
    };
    const INPUTCONTEXT old = *ctx;
    struct imc_entry *entry;

    *ctx = default_input_context;
    ctx->hWnd = old.hWnd;
    ctx->hMsgBuf = old.hMsgBuf;
    ctx->hCompStr = old.hCompStr;
    ctx->hCandInfo = old.hCandInfo;
    ctx->hGuideLine = old.hGuideLine;
    if (!(ctx->hPrivate = ImmCreateIMCC( ime->info.dwPrivateDataSize )))
        WARN( "Failed to allocate IME private data\n" );

    if (!(entry = malloc( sizeof(*entry) ))) return;
    entry->himc = himc;
    entry->context = *ctx;

    EnterCriticalSection( &ime_cs );

    /* reference the IME the first time the input context cache is used
     * in the same way Windows does it, so it doesn't get destroyed and
     * INPUTCONTEXT cache lost when keyboard layout is changed
     */
    if (list_empty( &ime->input_contexts )) ime->refcount++;

    list_add_tail( &ime->input_contexts, &entry->entry );
    LeaveCriticalSection( &ime_cs );
}

static INPUTCONTEXT *ime_find_input_context( struct ime *ime, HIMC himc )
{
    struct imc_entry *entry;

    EnterCriticalSection( &ime_cs );
    LIST_FOR_EACH_ENTRY( entry, &ime->input_contexts, struct imc_entry, entry )
        if (entry->himc == himc) break;
    LeaveCriticalSection( &ime_cs );

    if (&entry->entry == &ime->input_contexts) return NULL;
    return &entry->context;
}

static void imc_release_ime( struct imc *imc, struct ime *ime )
{
    INPUTCONTEXT *ctx;

    if (imc->ui_hwnd) DestroyWindow( imc->ui_hwnd );
    imc->ui_hwnd = NULL;
    ime->pImeSelect( imc->handle, FALSE );

    if ((ctx = ime_find_input_context( ime, imc->handle ))) *ctx = imc->IMC;
    ime_release( ime );
}

static struct ime *imc_select_ime( struct imc *imc )
{
    HKL hkl = GetKeyboardLayout( 0 );
    struct ime *ime;

    if ((ime = imc->ime))
    {
        if (ime->hkl == hkl) return ime;
        imc->ime = NULL;
        imc_release_ime( imc, ime );
    }

    if (!(imc->ime = ime_acquire( hkl )))
        WARN( "Failed to acquire IME for HKL %p\n", hkl );
    else
    {
        INPUTCONTEXT *ctx;

        if ((ctx = ime_find_input_context( imc->ime, imc->handle ))) imc->IMC = *ctx;
        else ime_save_input_context( imc->ime, imc->handle, &imc->IMC );

        imc->ime->pImeSelect( imc->handle, TRUE );
    }

    return imc->ime;
}

static BOOL CALLBACK enum_activate_layout( HIMC himc, LPARAM lparam )
{
    if (ImmLockIMC( himc )) ImmUnlockIMC( himc );
    return TRUE;
}

BOOL WINAPI ImmActivateLayout( HKL hkl )
{
    TRACE( "hkl %p\n", hkl );

    if (hkl == GetKeyboardLayout( 0 )) return TRUE;
    if (!ActivateKeyboardLayout( hkl, 0 )) return FALSE;

    ImmEnumInputContext( 0, enum_activate_layout, 0 );

    return TRUE;
}

static BOOL free_input_context_data( HIMC hIMC )
{
    struct imc *data = query_imc_data( hIMC );
    struct ime *ime;

    if (!data) return FALSE;

    TRACE( "Destroying %p\n", hIMC );

    if ((ime = imc_select_ime( data ))) imc_release_ime( data, ime );

    ImmDestroyIMCC( data->IMC.hCompStr );
    ImmDestroyIMCC( data->IMC.hCandInfo );
    ImmDestroyIMCC( data->IMC.hGuideLine );
    ImmDestroyIMCC( data->IMC.hMsgBuf );

    free( data );

    return TRUE;
}

static void input_context_init( INPUTCONTEXT *ctx )
{
    COMPOSITIONSTRING *str;
    CANDIDATEINFO *info;
    GUIDELINE *line;
    UINT i;

    if (!(ctx->hMsgBuf = ImmCreateIMCC( 0 )))
        WARN( "Failed to allocate %p message buffer\n", ctx );

    if (!(ctx->hCompStr = ImmCreateIMCC( sizeof(COMPOSITIONSTRING) )))
        WARN( "Failed to allocate %p COMPOSITIONSTRING\n", ctx );
    else if (!(str = ImmLockIMCC( ctx->hCompStr )))
        WARN( "Failed to lock IMCC for COMPOSITIONSTRING\n" );
    else
    {
        str->dwSize = sizeof(COMPOSITIONSTRING);
        ImmUnlockIMCC( ctx->hCompStr );
    }

    if (!(ctx->hCandInfo = ImmCreateIMCC( sizeof(CANDIDATEINFO) )))
        WARN( "Failed to allocate %p CANDIDATEINFO\n", ctx );
    else if (!(info = ImmLockIMCC( ctx->hCandInfo )))
        WARN( "Failed to lock IMCC for CANDIDATEINFO\n" );
    else
    {
        info->dwSize = sizeof(CANDIDATEINFO);
        ImmUnlockIMCC( ctx->hCandInfo );
    }

    if (!(ctx->hGuideLine = ImmCreateIMCC( sizeof(GUIDELINE) )))
        WARN( "Failed to allocate %p GUIDELINE\n", ctx );
    else if (!(line = ImmLockIMCC( ctx->hGuideLine )))
        WARN( "Failed to lock IMCC for GUIDELINE\n" );
    else
    {
        line->dwSize = sizeof(GUIDELINE);
        ImmUnlockIMCC( ctx->hGuideLine );
    }

    for (i = 0; i < ARRAY_SIZE(ctx->cfCandForm); i++)
        ctx->cfCandForm[i].dwIndex = ~0u;
}

static void IMM_FreeThreadData(void)
{
    struct coinit_spy *spy;

    free_input_context_data( UlongToHandle( NtUserGetThreadInfo()->default_imc ) );
    if ((spy = get_thread_coinit_spy())) IInitializeSpy_Release( &spy->IInitializeSpy_iface );
}

static void IMM_FreeAllImmHkl(void)
{
    struct ime *ime, *ime_next;

    LIST_FOR_EACH_ENTRY_SAFE( ime, ime_next, &ime_list, struct ime, entry )
    {
        struct imc_entry *imc_entry, *imc_next;
        list_remove( &ime->entry );

        ime->pImeDestroy( 1 );
        FreeLibrary( ime->module );
        LIST_FOR_EACH_ENTRY_SAFE( imc_entry, imc_next, &ime->input_contexts, struct imc_entry, entry )
        {
            ImmDestroyIMCC( imc_entry->context.hPrivate );
            free( imc_entry );
        }

        free( ime );
    }
}

BOOL WINAPI DllMain( HINSTANCE instance, DWORD reason, void *reserved )
{
    TRACE( "instance %p, reason %lx, reserved %p\n", instance, reason, reserved );

    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        if (!User32InitializeImmEntryTable( IMM_INIT_MAGIC )) return FALSE;
        imm32_module = instance;
        break;
    case DLL_THREAD_ATTACH:
        break;
    case DLL_THREAD_DETACH:
        IMM_FreeThreadData();
        break;
    case DLL_PROCESS_DETACH:
        if (reserved) break;
        IMM_FreeThreadData();
        IMM_FreeAllImmHkl();
        break;
    }

    return TRUE;
}

/***********************************************************************
 *		ImmSetActiveContext (IMM32.@)
 */
BOOL WINAPI ImmSetActiveContext(HWND hwnd, HIMC himc, BOOL activate)
{
    struct imc *data = get_imc_data( himc );
    struct ime *ime;

    TRACE("(%p, %p, %x)\n", hwnd, himc, activate);

    if (himc && !data && activate)
        return FALSE;

    imm_coinit_thread();

    if (data)
    {
        if (activate) data->IMC.hWnd = hwnd;
        if ((ime = imc_select_ime( data ))) ime->pImeSetActiveContext( himc, activate );
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
 *		ImmConfigureIMEA (IMM32.@)
 */
BOOL WINAPI ImmConfigureIMEA( HKL hkl, HWND hwnd, DWORD mode, void *data )
{
    struct ime *ime;
    BOOL ret;

    TRACE( "hkl %p, hwnd %p, mode %lu, data %p.\n", hkl, hwnd, mode, data );

    if (mode == IME_CONFIG_REGISTERWORD && !data) return FALSE;
    if (!(ime = ime_acquire( hkl ))) return FALSE;

    if (mode != IME_CONFIG_REGISTERWORD || !ime_is_unicode( ime ))
        ret = ime->pImeConfigure( hkl, hwnd, mode, data );
    else
    {
        REGISTERWORDA *wordA = data;
        REGISTERWORDW wordW;
        wordW.lpWord = strdupAtoW( wordA->lpWord );
        wordW.lpReading = strdupAtoW( wordA->lpReading );
        ret = ime->pImeConfigure( hkl, hwnd, mode, &wordW );
        free( wordW.lpReading );
        free( wordW.lpWord );
    }

    ime_release( ime );
    return ret;
}

/***********************************************************************
 *		ImmConfigureIMEW (IMM32.@)
 */
BOOL WINAPI ImmConfigureIMEW( HKL hkl, HWND hwnd, DWORD mode, void *data )
{
    struct ime *ime;
    BOOL ret;

    TRACE( "hkl %p, hwnd %p, mode %lu, data %p.\n", hkl, hwnd, mode, data );

    if (mode == IME_CONFIG_REGISTERWORD && !data) return FALSE;
    if (!(ime = ime_acquire( hkl ))) return FALSE;

    if (mode != IME_CONFIG_REGISTERWORD || ime_is_unicode( ime ))
        ret = ime->pImeConfigure( hkl, hwnd, mode, data );
    else
    {
        REGISTERWORDW *wordW = data;
        REGISTERWORDA wordA;
        wordA.lpWord = strdupWtoA( wordW->lpWord );
        wordA.lpReading = strdupWtoA( wordW->lpReading );
        ret = ime->pImeConfigure( hkl, hwnd, mode, &wordA );
        free( wordA.lpReading );
        free( wordA.lpWord );
    }

    ime_release( ime );
    return ret;
}

static struct imc *create_input_context( HIMC default_imc )
{
    struct imc *new_context;

    if (!(new_context = calloc( 1, sizeof(*new_context) ))) return NULL;
    input_context_init( &new_context->IMC );

    if (!default_imc)
        new_context->handle = NtUserCreateInputContext((UINT_PTR)new_context);
    else if (NtUserUpdateInputContext(default_imc, NtUserInputContextClientPtr, (UINT_PTR)new_context))
        new_context->handle = default_imc;
    if (!new_context->handle)
    {
        free_input_context_data(new_context);
        return 0;
    }

    TRACE("Created context %p\n", new_context);
    return new_context;
}

static struct imc *get_imc_data( HIMC handle )
{
    struct imc *ret;

    if ((ret = query_imc_data(handle)) || !handle) return ret;
    return create_input_context(handle);
}

static struct imc *default_input_context(void)
{
    UINT *himc = &NtUserGetThreadInfo()->default_imc;
    if (!*himc) *himc = (UINT_PTR)NtUserCreateInputContext( 0 );
    return get_imc_data( (HIMC)(UINT_PTR)*himc );
}

static HWND get_ime_ui_window(void)
{
    struct imc *imc = default_input_context();
    struct ime *ime;

    if (!(ime = imc_select_ime( imc ))) return 0;

    if (!imc->ui_hwnd)
    {
        imc->ui_hwnd = CreateWindowExW( WS_EX_TOOLWINDOW, ime->ui_class, NULL, WS_POPUP, 0, 0, 1, 1,
                                        ImmGetDefaultIMEWnd( 0 ), 0, ime->module, 0 );
        SetWindowPos( imc->ui_hwnd, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE );
        SetWindowLongPtrW( imc->ui_hwnd, IMMGWL_IMC, (LONG_PTR)NtUserGetWindowInputContext( GetFocus() ) );
    }
    return imc->ui_hwnd;
}

static void set_ime_ui_window_himc( HIMC himc )
{
    HWND hwnd;
    if (!(hwnd = get_ime_ui_window())) return;
    SetWindowLongPtrW( hwnd, IMMGWL_IMC, (LONG_PTR)himc );
}

/***********************************************************************
 *		ImmCreateContext (IMM32.@)
 */
HIMC WINAPI ImmCreateContext(void)
{
    struct imc *new_context;

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
    if ((UINT_PTR)hIMC == NtUserGetThreadInfo()->default_imc) return FALSE;
    if (NtUserQueryInputContext( hIMC, NtUserInputContextThreadId ) != GetCurrentThreadId()) return FALSE;
    return IMM_DestroyContext(hIMC);
}

/***********************************************************************
 *      ImmAssociateContext (IMM32.@)
 */
HIMC WINAPI ImmAssociateContext( HWND hwnd, HIMC new_himc )
{
    HIMC old_himc;
    UINT ret;

    TRACE( "hwnd %p, new_himc %p\n", hwnd, new_himc );

    old_himc = NtUserGetWindowInputContext( hwnd );
    ret = NtUserAssociateInputContext( hwnd, new_himc, 0 );
    if (ret == AICR_FOCUS_CHANGED)
    {
        ImmSetActiveContext( hwnd, old_himc, FALSE );
        ImmSetActiveContext( hwnd, new_himc, TRUE );
        if (hwnd == GetFocus()) set_ime_ui_window_himc( new_himc );
    }

    return ret == AICR_FAILED ? 0 : old_himc;
}

static BOOL CALLBACK enum_associate_context( HWND hwnd, LPARAM lparam )
{
    ImmAssociateContext( hwnd, (HIMC)lparam );
    return TRUE;
}

/***********************************************************************
 *              ImmAssociateContextEx (IMM32.@)
 */
BOOL WINAPI ImmAssociateContextEx( HWND hwnd, HIMC new_himc, DWORD flags )
{
    HIMC old_himc;
    UINT ret;

    TRACE( "hwnd %p, new_himc %p, flags %#lx\n", hwnd, new_himc, flags );

    if (!hwnd) return FALSE;

    if (flags == IACE_CHILDREN)
    {
        EnumChildWindows( hwnd, enum_associate_context, (LPARAM)new_himc );
        return TRUE;
    }

    old_himc = NtUserGetWindowInputContext( hwnd );
    ret = NtUserAssociateInputContext( hwnd, new_himc, flags );
    if (ret == AICR_FOCUS_CHANGED)
    {
        if (flags == IACE_DEFAULT) new_himc = NtUserGetWindowInputContext( hwnd );
        ImmSetActiveContext( hwnd, old_himc, FALSE );
        ImmSetActiveContext( hwnd, new_himc, TRUE );
        if (hwnd == GetFocus()) set_ime_ui_window_himc( new_himc );
    }

    return ret != AICR_FAILED;
}

struct enum_register_word_params_WtoA
{
    REGISTERWORDENUMPROCA proc;
    void *user;
};

static int CALLBACK enum_register_word_WtoA( const WCHAR *readingW, DWORD style,
                                             const WCHAR *stringW, void *user )
{
    char *readingA = strdupWtoA( readingW ), *stringA = strdupWtoA( stringW );
    struct enum_register_word_params_WtoA *params = user;
    int ret = params->proc( readingA, style, stringA, params->user );
    free( readingA );
    free( stringA );
    return ret;
}

/***********************************************************************
 *		ImmEnumRegisterWordA (IMM32.@)
 */
UINT WINAPI ImmEnumRegisterWordA( HKL hkl, REGISTERWORDENUMPROCA procA, const char *readingA,
                                  DWORD style, const char *stringA, void *user )
{
    struct ime *ime;
    UINT ret;

    TRACE( "hkl %p, procA %p, readingA %s, style %lu, stringA %s, user %p.\n", hkl, procA,
           debugstr_a(readingA), style, debugstr_a(stringA), user );

    if (!(ime = ime_acquire( hkl ))) return 0;

    if (!ime_is_unicode( ime ))
        ret = ime->pImeEnumRegisterWord( procA, readingA, style, stringA, user );
    else
    {
        struct enum_register_word_params_WtoA params = {.proc = procA, .user = user};
        WCHAR *readingW = strdupAtoW( readingA ), *stringW = strdupAtoW( stringA );
        ret = ime->pImeEnumRegisterWord( enum_register_word_WtoA, readingW, style, stringW, &params );
        free( readingW );
        free( stringW );
    }

    ime_release( ime );
    return ret;
}

struct enum_register_word_params_AtoW
{
    REGISTERWORDENUMPROCW proc;
    void *user;
};

static int CALLBACK enum_register_word_AtoW( const char *readingA, DWORD style,
                                             const char *stringA, void *user )
{
    WCHAR *readingW = strdupAtoW( readingA ), *stringW = strdupAtoW( stringA );
    struct enum_register_word_params_AtoW *params = user;
    int ret = params->proc( readingW, style, stringW, params->user );
    free( readingW );
    free( stringW );
    return ret;
}

/***********************************************************************
 *		ImmEnumRegisterWordW (IMM32.@)
 */
UINT WINAPI ImmEnumRegisterWordW( HKL hkl, REGISTERWORDENUMPROCW procW, const WCHAR *readingW,
                                  DWORD style, const WCHAR *stringW, void *user )
{
    struct ime *ime;
    UINT ret;

    TRACE( "hkl %p, procW %p, readingW %s, style %lu, stringW %s, user %p.\n", hkl, procW,
           debugstr_w(readingW), style, debugstr_w(stringW), user );

    if (!(ime = ime_acquire( hkl ))) return 0;

    if (ime_is_unicode( ime ))
        ret = ime->pImeEnumRegisterWord( procW, readingW, style, stringW, user );
    else
    {
        struct enum_register_word_params_AtoW params = {.proc = procW, .user = user};
        char *readingA = strdupWtoA( readingW ), *stringA = strdupWtoA( stringW );
        ret = ime->pImeEnumRegisterWord( enum_register_word_AtoW, readingA, style, stringA, &params );
        free( readingA );
        free( stringA );
    }

    ime_release( ime );
    return ret;
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
LRESULT WINAPI ImmEscapeA( HKL hkl, HIMC himc, UINT code, void *data )
{
    struct ime *ime;
    LRESULT ret;

    TRACE( "hkl %p, himc %p, code %u, data %p.\n", hkl, himc, code, data );

    if (!(ime = ime_acquire( hkl ))) return 0;

    if (!EscapeRequiresWA( code ) || !ime_is_unicode( ime ) || !data)
        ret = ime->pImeEscape( himc, code, data );
    else
    {
        WCHAR buffer[81]; /* largest required buffer should be 80 */
        if (code == IME_ESC_SET_EUDC_DICTIONARY)
        {
            MultiByteToWideChar( CP_ACP, 0, data, -1, buffer, 81 );
            ret = ime->pImeEscape( himc, code, buffer );
        }
        else
        {
            ret = ime->pImeEscape( himc, code, buffer );
            WideCharToMultiByte( CP_ACP, 0, buffer, -1, data, 80, NULL, NULL );
        }
    }

    ime_release( ime );
    return ret;
}

/***********************************************************************
 *		ImmEscapeW (IMM32.@)
 */
LRESULT WINAPI ImmEscapeW( HKL hkl, HIMC himc, UINT code, void *data )
{
    struct ime *ime;
    LRESULT ret;

    TRACE( "hkl %p, himc %p, code %u, data %p.\n", hkl, himc, code, data );

    if (!(ime = ime_acquire( hkl ))) return 0;

    if (!EscapeRequiresWA( code ) || ime_is_unicode( ime ) || !data)
        ret = ime->pImeEscape( himc, code, data );
    else
    {
        char buffer[81]; /* largest required buffer should be 80 */
        if (code == IME_ESC_SET_EUDC_DICTIONARY)
        {
            WideCharToMultiByte( CP_ACP, 0, data, -1, buffer, 81, NULL, NULL );
            ret = ime->pImeEscape( himc, code, buffer );
        }
        else
        {
            ret = ime->pImeEscape( himc, code, buffer );
            MultiByteToWideChar( CP_ACP, 0, buffer, -1, data, 80 );
        }
    }

    ime_release( ime );
    return ret;
}

/***********************************************************************
 *		ImmGetCandidateListA (IMM32.@)
 */
DWORD WINAPI ImmGetCandidateListA(
  HIMC hIMC, DWORD dwIndex,
  LPCANDIDATELIST lpCandList, DWORD dwBufLen)
{
    struct imc *data = get_imc_data( hIMC );
    LPCANDIDATEINFO candinfo;
    LPCANDIDATELIST candlist;
    struct ime *ime;
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

    if (!(ime = imc_select_ime( data )))
        ret = 0;
    else if (!ime_is_unicode( ime ))
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
    struct imc *data = get_imc_data( hIMC );
    LPCANDIDATEINFO candinfo;
    DWORD ret, count;
    struct ime *ime;

    TRACE("%p, %p\n", hIMC, lpdwListCount);

    if (!data || !lpdwListCount || !data->IMC.hCandInfo)
       return 0;

    candinfo = ImmLockIMCC(data->IMC.hCandInfo);

    *lpdwListCount = count = candinfo->dwCount;

    if (!(ime = imc_select_ime( data )))
        ret = 0;
    else if (!ime_is_unicode( ime ))
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
    struct imc *data = get_imc_data( hIMC );
    LPCANDIDATEINFO candinfo;
    DWORD ret, count;
    struct ime *ime;

    TRACE("%p, %p\n", hIMC, lpdwListCount);

    if (!data || !lpdwListCount || !data->IMC.hCandInfo)
       return 0;

    candinfo = ImmLockIMCC(data->IMC.hCandInfo);

    *lpdwListCount = count = candinfo->dwCount;

    if (!(ime = imc_select_ime( data )))
        ret = 0;
    else if (ime_is_unicode( ime ))
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
    struct imc *data = get_imc_data( hIMC );
    LPCANDIDATEINFO candinfo;
    LPCANDIDATELIST candlist;
    struct ime *ime;
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

    if (!(ime = imc_select_ime( data )))
        ret = 0;
    else if (ime_is_unicode( ime ))
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
BOOL WINAPI ImmGetCandidateWindow( HIMC himc, DWORD index, CANDIDATEFORM *candidate )
{
    INPUTCONTEXT *ctx;
    BOOL ret = TRUE;

    TRACE( "himc %p, index %lu, candidate %p\n", himc, index, candidate );

    if (!candidate) return FALSE;

    if (!(ctx = ImmLockIMC( himc ))) return FALSE;
    if (ctx->cfCandForm[index].dwIndex == -1) ret = FALSE;
    else *candidate = ctx->cfCandForm[index];
    ImmUnlockIMC( himc );

    return ret;
}

/***********************************************************************
 *		ImmGetCompositionFontA (IMM32.@)
 */
BOOL WINAPI ImmGetCompositionFontA( HIMC himc, LOGFONTA *fontA )
{
    INPUTCONTEXT *ctx;
    LOGFONTW fontW;
    BOOL ret = TRUE;

    TRACE( "himc %p, fontA %p\n", himc, fontA );

    if (!fontA) return FALSE;

    if (!(ctx = ImmLockIMC( himc ))) return FALSE;
    if (!(ctx->fdwInit & INIT_LOGFONT)) ret = FALSE;
    else if (!input_context_is_unicode( ctx )) *fontA = ctx->lfFont.A;
    else if ((ret = ImmGetCompositionFontW( himc, &fontW )))
    {
        memcpy( fontA, &fontW, offsetof(LOGFONTA, lfFaceName) );
        WideCharToMultiByte( CP_ACP, 0, fontW.lfFaceName, -1, fontA->lfFaceName, LF_FACESIZE, NULL, NULL );
    }
    ImmUnlockIMC( himc );

    return ret;
}

/***********************************************************************
 *		ImmGetCompositionFontW (IMM32.@)
 */
BOOL WINAPI ImmGetCompositionFontW( HIMC himc, LOGFONTW *fontW )
{
    INPUTCONTEXT *ctx;
    LOGFONTA fontA;
    BOOL ret = TRUE;

    TRACE( "himc %p, fontW %p\n", himc, fontW );

    if (!fontW) return FALSE;

    if (!(ctx = ImmLockIMC( himc ))) return FALSE;
    if (!(ctx->fdwInit & INIT_LOGFONT)) ret = FALSE;
    else if (input_context_is_unicode( ctx )) *fontW = ctx->lfFont.W;
    else if ((ret = ImmGetCompositionFontA( himc, &fontA )))
    {
        memcpy( fontW, &fontA, offsetof(LOGFONTW, lfFaceName) );
        MultiByteToWideChar( CP_ACP, 0, fontA.lfFaceName, -1, fontW->lfFaceName, LF_FACESIZE );
    }
    ImmUnlockIMC( himc );

    return ret;
}


/* Helpers for the GetCompositionString functions */

/* Source encoding is defined by context, source length is always given in respective characters. Destination buffer
   length is always in bytes. */
static INT CopyCompStringIMEtoClient( BOOL src_unicode, const void *src, INT src_len,
                                      void *dst, INT dst_len, BOOL dst_unicode )
{
    int char_size = dst_unicode ? sizeof(WCHAR) : sizeof(char);
    INT ret;

    if (src_unicode ^ dst_unicode)
    {
        if (dst_unicode)
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
static INT CopyCompAttrIMEtoClient( BOOL src_unicode, const BYTE *src, INT src_len, const void *comp_string, INT str_len,
                                    BYTE *dst, INT dst_len, BOOL unicode )
{
    union
    {
        const void *str;
        const WCHAR *strW;
        const char *strA;
    } string;
    INT rc;

    string.str = comp_string;

    if (src_unicode && !unicode)
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
    else if (!src_unicode && unicode)
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

static INT CopyCompClauseIMEtoClient( BOOL src_unicode, LPBYTE source, INT slen, LPBYTE ssource,
                                      LPBYTE target, INT tlen, BOOL unicode )
{
    INT rc;

    if (src_unicode && !unicode)
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
    else if (!src_unicode && unicode)
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

static INT CopyCompOffsetIMEtoClient( BOOL src_unicode, DWORD offset, LPBYTE ssource, BOOL unicode )
{
    int rc;

    if (src_unicode && !unicode)
    {
        rc = WideCharToMultiByte(CP_ACP, 0, (LPWSTR)ssource, offset, NULL, 0, NULL, NULL);
    }
    else if (!src_unicode && unicode)
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
    struct imc *data = get_imc_data( hIMC );
    LPCOMPOSITIONSTRING compstr;
    BOOL src_unicode;
    struct ime *ime;
    LPBYTE compdata;

    TRACE("(%p, 0x%lx, %p, %ld)\n", hIMC, dwIndex, lpBuf, dwBufLen);

    if (!data)
       return FALSE;

    if (!data->IMC.hCompStr)
       return FALSE;

    if (!(ime = imc_select_ime( data )))
        return FALSE;
    src_unicode = ime_is_unicode( ime );

    compdata = ImmLockIMCC(data->IMC.hCompStr);
    compstr = (LPCOMPOSITIONSTRING)compdata;

    switch (dwIndex)
    {
    case GCS_RESULTSTR:
        TRACE("GCS_RESULTSTR\n");
        rc = CopyCompStringIMEtoClient(src_unicode, compdata + compstr->dwResultStrOffset, compstr->dwResultStrLen, lpBuf, dwBufLen, unicode);
        break;
    case GCS_COMPSTR:
        TRACE("GCS_COMPSTR\n");
        rc = CopyCompStringIMEtoClient(src_unicode, compdata + compstr->dwCompStrOffset, compstr->dwCompStrLen, lpBuf, dwBufLen, unicode);
        break;
    case GCS_COMPATTR:
        TRACE("GCS_COMPATTR\n");
        rc = CopyCompAttrIMEtoClient(src_unicode, compdata + compstr->dwCompAttrOffset, compstr->dwCompAttrLen,
                                     compdata + compstr->dwCompStrOffset, compstr->dwCompStrLen,
                                     lpBuf, dwBufLen, unicode);
        break;
    case GCS_COMPCLAUSE:
        TRACE("GCS_COMPCLAUSE\n");
        rc = CopyCompClauseIMEtoClient(src_unicode, compdata + compstr->dwCompClauseOffset,compstr->dwCompClauseLen,
                                       compdata + compstr->dwCompStrOffset,
                                       lpBuf, dwBufLen, unicode);
        break;
    case GCS_RESULTCLAUSE:
        TRACE("GCS_RESULTCLAUSE\n");
        rc = CopyCompClauseIMEtoClient(src_unicode, compdata + compstr->dwResultClauseOffset,compstr->dwResultClauseLen,
                                       compdata + compstr->dwResultStrOffset,
                                       lpBuf, dwBufLen, unicode);
        break;
    case GCS_RESULTREADSTR:
        TRACE("GCS_RESULTREADSTR\n");
        rc = CopyCompStringIMEtoClient(src_unicode, compdata + compstr->dwResultReadStrOffset, compstr->dwResultReadStrLen, lpBuf, dwBufLen, unicode);
        break;
    case GCS_RESULTREADCLAUSE:
        TRACE("GCS_RESULTREADCLAUSE\n");
        rc = CopyCompClauseIMEtoClient(src_unicode, compdata + compstr->dwResultReadClauseOffset,compstr->dwResultReadClauseLen,
                                       compdata + compstr->dwResultStrOffset,
                                       lpBuf, dwBufLen, unicode);
        break;
    case GCS_COMPREADSTR:
        TRACE("GCS_COMPREADSTR\n");
        rc = CopyCompStringIMEtoClient(src_unicode, compdata + compstr->dwCompReadStrOffset, compstr->dwCompReadStrLen, lpBuf, dwBufLen, unicode);
        break;
    case GCS_COMPREADATTR:
        TRACE("GCS_COMPREADATTR\n");
        rc = CopyCompAttrIMEtoClient(src_unicode, compdata + compstr->dwCompReadAttrOffset, compstr->dwCompReadAttrLen,
                                     compdata + compstr->dwCompReadStrOffset, compstr->dwCompReadStrLen,
                                     lpBuf, dwBufLen, unicode);
        break;
    case GCS_COMPREADCLAUSE:
        TRACE("GCS_COMPREADCLAUSE\n");
        rc = CopyCompClauseIMEtoClient(src_unicode, compdata + compstr->dwCompReadClauseOffset,compstr->dwCompReadClauseLen,
                                       compdata + compstr->dwCompStrOffset,
                                       lpBuf, dwBufLen, unicode);
        break;
    case GCS_CURSORPOS:
        TRACE("GCS_CURSORPOS\n");
        rc = CopyCompOffsetIMEtoClient(src_unicode, compstr->dwCursorPos, compdata + compstr->dwCompStrOffset, unicode);
        break;
    case GCS_DELTASTART:
        TRACE("GCS_DELTASTART\n");
        rc = CopyCompOffsetIMEtoClient(src_unicode, compstr->dwDeltaStart, compdata + compstr->dwCompStrOffset, unicode);
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
BOOL WINAPI ImmGetCompositionWindow( HIMC himc, COMPOSITIONFORM *composition )
{
    INPUTCONTEXT *ctx;
    BOOL ret;

    TRACE( "himc %p, composition %p\n", himc, composition );

    if (!(ctx = ImmLockIMC( himc ))) return FALSE;
    if ((ret = !!(ctx->fdwInit & INIT_COMPFORM))) *composition = ctx->cfCompForm;
    ImmUnlockIMC( himc );

    return ret;
}

/***********************************************************************
 *		ImmGetContext (IMM32.@)
 *
 */
HIMC WINAPI ImmGetContext( HWND hwnd )
{
    TRACE( "hwnd %p\n", hwnd );
    return NtUserGetWindowInputContext( hwnd );
}

/***********************************************************************
 *		ImmGetConversionListA (IMM32.@)
 */
DWORD WINAPI ImmGetConversionListA( HKL hkl, HIMC himc, const char *srcA, CANDIDATELIST *listA,
                                    DWORD lengthA, UINT flags )
{
    struct ime *ime;
    DWORD ret;

    TRACE( "hkl %p, himc %p, srcA %s, listA %p, lengthA %lu, flags %#x.\n", hkl, himc,
           debugstr_a(srcA), listA, lengthA, flags );

    if (!(ime = ime_acquire( hkl ))) return 0;

    if (!ime_is_unicode( ime ))
        ret = ime->pImeConversionList( himc, srcA, listA, lengthA, flags );
    else
    {
        CANDIDATELIST *listW;
        WCHAR *srcW = strdupAtoW( srcA );
        DWORD lengthW = ime->pImeConversionList( himc, srcW, NULL, 0, flags );

        if (!(listW = malloc( lengthW ))) ret = 0;
        else
        {
            ime->pImeConversionList( himc, srcW, listW, lengthW, flags );
            ret = convert_candidatelist_WtoA( listW, listA, lengthA );
            free( listW );
        }
        free( srcW );
    }

    ime_release( ime );
    return ret;
}

/***********************************************************************
 *		ImmGetConversionListW (IMM32.@)
 */
DWORD WINAPI ImmGetConversionListW( HKL hkl, HIMC himc, const WCHAR *srcW, CANDIDATELIST *listW,
                                    DWORD lengthW, UINT flags )
{
    struct ime *ime;
    DWORD ret;

    TRACE( "hkl %p, himc %p, srcW %s, listW %p, lengthW %lu, flags %#x.\n", hkl, himc,
           debugstr_w(srcW), listW, lengthW, flags );

    if (!(ime = ime_acquire( hkl ))) return 0;

    if (ime_is_unicode( ime ))
        ret = ime->pImeConversionList( himc, srcW, listW, lengthW, flags );
    else
    {
        CANDIDATELIST *listA;
        char *srcA = strdupWtoA( srcW );
        DWORD lengthA = ime->pImeConversionList( himc, srcA, NULL, 0, flags );

        if (!(listA = malloc( lengthA ))) ret = 0;
        else
        {
            ime->pImeConversionList( himc, srcA, listA, lengthA, flags );
            ret = convert_candidatelist_AtoW( listA, listW, lengthW );
            free( listA );
        }
        free( srcA );
    }

    ime_release( ime );
    return ret;
}

/***********************************************************************
 *		ImmGetConversionStatus (IMM32.@)
 */
BOOL WINAPI ImmGetConversionStatus( HIMC himc, DWORD *conversion, DWORD *sentence )
{
    INPUTCONTEXT *ctx;

    TRACE( "himc %p, conversion %p, sentence %p\n", himc, conversion, sentence );

    if (!(ctx = ImmLockIMC( himc ))) return FALSE;
    if (conversion) *conversion = ctx->fdwConversion;
    if (sentence) *sentence = ctx->fdwSentence;
    ImmUnlockIMC( himc );

    return TRUE;
}

/***********************************************************************
 *		ImmGetDefaultIMEWnd (IMM32.@)
 */
HWND WINAPI ImmGetDefaultIMEWnd(HWND hWnd)
{
    return NtUserQueryWindow( hWnd, WindowDefaultImeWindow );
}

/***********************************************************************
 *      ImmGetDescriptionA (IMM32.@)
 */
UINT WINAPI ImmGetDescriptionA( HKL hkl, LPSTR bufferA, UINT lengthA )
{
    WCHAR *bufferW;
    DWORD lengthW;

    TRACE( "hkl %p, bufferA %p, lengthA %d\n", hkl, bufferA, lengthA );

    if (!(lengthW = ImmGetDescriptionW( hkl, NULL, 0 ))) return 0;
    if (!(bufferW = malloc( (lengthW + 1) * sizeof(WCHAR) ))) return 0;
    lengthW = ImmGetDescriptionW( hkl, bufferW, lengthW + 1 );
    lengthA = WideCharToMultiByte( CP_ACP, 0, bufferW, lengthW, bufferA,
                                   bufferA ? lengthA : 0, NULL, NULL );
    if (bufferA) bufferA[lengthA] = 0;
    free( bufferW );

    return lengthA;
}

/***********************************************************************
 *		ImmGetDescriptionW (IMM32.@)
 */
UINT WINAPI ImmGetDescriptionW( HKL hkl, WCHAR *buffer, UINT length )
{
    WCHAR path[MAX_PATH];
    HKEY hkey = 0;
    DWORD size;

    TRACE( "hkl %p, buffer %p, length %u\n", hkl, buffer, length );

    swprintf( path, ARRAY_SIZE(path), layouts_formatW, (ULONG)(ULONG_PTR)hkl );
    if (RegOpenKeyW( HKEY_LOCAL_MACHINE, path, &hkey )) return 0;

    size = ARRAY_SIZE(path) * sizeof(WCHAR);
    if (RegGetValueW( hkey, NULL, L"Layout Text", RRF_RT_REG_SZ, NULL, path, &size )) *path = 0;
    RegCloseKey( hkey );

    size = wcslen( path );
    if (!buffer) return size;

    lstrcpynW( buffer, path, length );
    return wcslen( buffer );
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
 *      ImmGetIMEFileNameA (IMM32.@)
 */
UINT WINAPI ImmGetIMEFileNameA( HKL hkl, char *bufferA, UINT lengthA )
{
    WCHAR *bufferW;
    DWORD lengthW;

    TRACE( "hkl %p, bufferA %p, lengthA %d\n", hkl, bufferA, lengthA );

    if (!(lengthW = ImmGetIMEFileNameW( hkl, NULL, 0 ))) return 0;
    if (!(bufferW = malloc( (lengthW + 1) * sizeof(WCHAR) ))) return 0;
    lengthW = ImmGetIMEFileNameW( hkl, bufferW, lengthW + 1 );
    lengthA = WideCharToMultiByte( CP_ACP, 0, bufferW, lengthW, bufferA,
                                   bufferA ? lengthA : 0, NULL, NULL );
    if (bufferA) bufferA[lengthA] = 0;
    free( bufferW );

    return lengthA;
}

/***********************************************************************
 *		ImmGetIMEFileNameW (IMM32.@)
 */
UINT WINAPI ImmGetIMEFileNameW( HKL hkl, WCHAR *buffer, UINT length )
{
    WCHAR path[MAX_PATH];
    HKEY hkey = 0;
    DWORD size;

    TRACE( "hkl %p, buffer %p, length %u\n", hkl, buffer, length );

    swprintf( path, ARRAY_SIZE(path), layouts_formatW, (ULONG)(ULONG_PTR)hkl );
    if (RegOpenKeyW( HKEY_LOCAL_MACHINE, path, &hkey )) return 0;

    size = ARRAY_SIZE(path) * sizeof(WCHAR);
    if (RegGetValueW( hkey, NULL, L"Ime File", RRF_RT_REG_SZ, NULL, path, &size )) *path = 0;
    RegCloseKey( hkey );

    size = wcslen( path );
    if (!buffer) return size;

    lstrcpynW( buffer, path, length );
    return wcslen( buffer );
}

/***********************************************************************
 *		ImmGetOpenStatus (IMM32.@)
 */
BOOL WINAPI ImmGetOpenStatus( HIMC himc )
{
    INPUTCONTEXT *ctx;
    BOOL status;

    TRACE( "himc %p\n", himc );

    if (!(ctx = ImmLockIMC( himc ))) return FALSE;
    status = ctx->fOpen;
    ImmUnlockIMC( himc );

    return status;
}

/***********************************************************************
 *		ImmGetProperty (IMM32.@)
 */
DWORD WINAPI ImmGetProperty( HKL hkl, DWORD index )
{
    struct ime *ime;
    DWORD ret;

    TRACE( "hkl %p, index %lu.\n", hkl, index );

    if (!(ime = ime_acquire( hkl ))) return 0;

    switch (index)
    {
    case IGP_PROPERTY: ret = ime->info.fdwProperty; break;
    case IGP_CONVERSION: ret = ime->info.fdwConversionCaps; break;
    case IGP_SENTENCE: ret = ime->info.fdwSentenceCaps; break;
    case IGP_SETCOMPSTR: ret = ime->info.fdwSCSCaps; break;
    case IGP_SELECT: ret = ime->info.fdwSelectCaps; break;
    case IGP_GETIMEVERSION: ret = IMEVER_0400; break;
    case IGP_UI: ret = 0; break;
    default: ret = 0; break;
    }

    ime_release( ime );
    return ret;
}

/***********************************************************************
 *		ImmGetRegisterWordStyleA (IMM32.@)
 */
UINT WINAPI ImmGetRegisterWordStyleA( HKL hkl, UINT count, STYLEBUFA *styleA )
{
    struct ime *ime;
    UINT ret;

    TRACE( "hkl %p, count %u, styleA %p.\n", hkl, count, styleA );

    if (!(ime = ime_acquire( hkl ))) return 0;

    if (!ime_is_unicode( ime ))
        ret = ime->pImeGetRegisterWordStyle( count, styleA );
    else
    {
        STYLEBUFW styleW;
        ret = ime->pImeGetRegisterWordStyle( count, &styleW );
        WideCharToMultiByte( CP_ACP, 0, styleW.szDescription, -1, styleA->szDescription, 32, NULL, NULL );
        styleA->dwStyle = styleW.dwStyle;
    }

    ime_release( ime );
    return ret;
}

/***********************************************************************
 *		ImmGetRegisterWordStyleW (IMM32.@)
 */
UINT WINAPI ImmGetRegisterWordStyleW( HKL hkl, UINT count, STYLEBUFW *styleW )
{
    struct ime *ime;
    UINT ret;

    TRACE( "hkl %p, count %u, styleW %p.\n", hkl, count, styleW );

    if (!(ime = ime_acquire( hkl ))) return 0;

    if (ime_is_unicode( ime ))
        ret = ime->pImeGetRegisterWordStyle( count, styleW );
    else
    {
        STYLEBUFA styleA;
        ret = ime->pImeGetRegisterWordStyle( count, &styleA );
        MultiByteToWideChar( CP_ACP, 0, styleA.szDescription, -1, styleW->szDescription, 32 );
        styleW->dwStyle = styleA.dwStyle;
    }

    ime_release( ime );
    return ret;
}

/***********************************************************************
 *		ImmGetStatusWindowPos (IMM32.@)
 */
BOOL WINAPI ImmGetStatusWindowPos( HIMC himc, POINT *pos )
{
    INPUTCONTEXT *ctx;
    BOOL ret;

    TRACE( "himc %p, pos %p\n", himc, pos );

    if (!(ctx = ImmLockIMC( himc ))) return FALSE;
    if ((ret = !!(ctx->fdwInit & INIT_STATUSWNDPOS))) *pos = ctx->ptStatusWndPos;
    ImmUnlockIMC( himc );

    return ret;
}

/***********************************************************************
 *		ImmGetVirtualKey (IMM32.@)
 */
UINT WINAPI ImmGetVirtualKey( HWND hwnd )
{
    HIMC himc = ImmGetContext( hwnd );
    struct imc *imc;

    TRACE( "%p\n", hwnd );

    if ((imc = get_imc_data( himc ))) return imc->vkey;
    return VK_PROCESSKEY;
}

/***********************************************************************
 *		ImmInstallIMEA (IMM32.@)
 */
HKL WINAPI ImmInstallIMEA( const char *filenameA, const char *descriptionA )
{
    WCHAR *filenameW = strdupAtoW( filenameA ), *descriptionW = strdupAtoW( descriptionA );
    HKL hkl;

    TRACE( "filenameA %s, descriptionA %s\n", debugstr_a(filenameA), debugstr_a(descriptionA) );

    hkl = ImmInstallIMEW( filenameW, descriptionW );
    free( descriptionW );
    free( filenameW );

    return hkl;
}

static LCID get_ime_file_lang( const WCHAR *filename )
{
    DWORD *languages;
    LCID lcid = 0;
    void *info;
    UINT len;

    if (!(len = GetFileVersionInfoSizeW( filename, NULL ))) return 0;
    if (!(info = malloc( len ))) goto done;
    if (!GetFileVersionInfoW( filename, 0, len, info )) goto done;
    if (!VerQueryValueW( info, L"\\VarFileInfo\\Translation", (void **)&languages, &len ) || !len) goto done;
    lcid = languages[0];

done:
    free( info );
    return lcid;
}

/***********************************************************************
 *		ImmInstallIMEW (IMM32.@)
 */
HKL WINAPI ImmInstallIMEW( const WCHAR *filename, const WCHAR *description )
{
    WCHAR path[ARRAY_SIZE(layouts_formatW)+8], buffer[MAX_PATH];
    LCID lcid;
    WORD count = 0x20;
    const WCHAR *tmp;
    DWORD length;
    HKEY hkey;
    HKL hkl;

    TRACE( "filename %s, description %s\n", debugstr_w(filename), debugstr_w(description) );

    if (!filename || !description || !(lcid = get_ime_file_lang( filename )))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

    while (count < 0xfff)
    {
        DWORD disposition = 0;

        hkl = (HKL)(UINT_PTR)MAKELONG( lcid, 0xe000 | count );
        swprintf( path, ARRAY_SIZE(path), layouts_formatW, (ULONG)(ULONG_PTR)hkl);
        if (!RegCreateKeyExW( HKEY_LOCAL_MACHINE, path, 0, NULL, 0,
                              KEY_WRITE, NULL, &hkey, &disposition ))
        {
            if (disposition == REG_CREATED_NEW_KEY) break;
            RegCloseKey( hkey );
        }

        count++;
    }

    if (count == 0xfff)
    {
        WARN("Unable to find slot to install IME\n");
        return 0;
    }

    if ((tmp = wcsrchr( filename, '\\' ))) tmp++;
    else tmp = filename;

    length = LCMapStringW( LOCALE_USER_DEFAULT, LCMAP_UPPERCASE, tmp, -1, buffer, ARRAY_SIZE(buffer) );

    if (RegSetValueExW( hkey, L"Ime File", 0, REG_SZ, (const BYTE *)buffer, length * sizeof(WCHAR) ) ||
        RegSetValueExW( hkey, L"Layout Text", 0, REG_SZ, (const BYTE *)description,
                        (wcslen(description) + 1) * sizeof(WCHAR) ))
    {
        WARN( "Unable to write registry to install IME\n");
        hkl = 0;
    }
    RegCloseKey( hkey );

    if (!hkl) RegDeleteKeyW( HKEY_LOCAL_MACHINE, path );
    return hkl;
}

/***********************************************************************
 *		ImmIsIME (IMM32.@)
 */
BOOL WINAPI ImmIsIME( HKL hkl )
{
    TRACE( "hkl %p\n", hkl );
    if (!hkl) return FALSE;
    return TRUE;
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
    struct imc *data = get_imc_data( hIMC );
    struct ime *ime;

    TRACE("(%p, %ld, %ld, %ld)\n",
        hIMC, dwAction, dwIndex, dwValue);

    if (hIMC == NULL)
    {
        SetLastError(ERROR_SUCCESS);
        return FALSE;
    }

    if (!data)
    {
        return FALSE;
    }

    if (!(ime = imc_select_ime( data ))) return FALSE;
    return ime->pNotifyIME( hIMC, dwAction, dwIndex, dwValue );
}

/***********************************************************************
 *		ImmRegisterWordA (IMM32.@)
 */
BOOL WINAPI ImmRegisterWordA( HKL hkl, const char *readingA, DWORD style, const char *stringA )
{
    struct ime *ime;
    BOOL ret;

    TRACE( "hkl %p, readingA %s, style %lu, stringA %s.\n", hkl, debugstr_a(readingA), style, debugstr_a(stringA) );

    if (!(ime = ime_acquire( hkl ))) return FALSE;

    if (!ime_is_unicode( ime ))
        ret = ime->pImeRegisterWord( readingA, style, stringA );
    else
    {
        WCHAR *readingW = strdupAtoW( readingA ), *stringW = strdupAtoW( stringA );
        ret = ime->pImeRegisterWord( readingW, style, stringW );
        free( readingW );
        free( stringW );
    }

    ime_release( ime );
    return ret;
}

/***********************************************************************
 *		ImmRegisterWordW (IMM32.@)
 */
BOOL WINAPI ImmRegisterWordW( HKL hkl, const WCHAR *readingW, DWORD style, const WCHAR *stringW )
{
    struct ime *ime;
    BOOL ret;

    TRACE( "hkl %p, readingW %s, style %lu, stringW %s.\n", hkl, debugstr_w(readingW), style, debugstr_w(stringW) );

    if (!(ime = ime_acquire( hkl ))) return FALSE;

    if (ime_is_unicode( ime ))
        ret = ime->pImeRegisterWord( readingW, style, stringW );
    else
    {
        char *readingA = strdupWtoA( readingW ), *stringA = strdupWtoA( stringW );
        ret = ime->pImeRegisterWord( readingA, style, stringA );
        free( readingA );
        free( stringA );
    }

    ime_release( ime );
    return ret;
}

/***********************************************************************
 *		ImmReleaseContext (IMM32.@)
 */
BOOL WINAPI ImmReleaseContext( HWND hwnd, HIMC himc )
{
    TRACE( "hwnd %p, himc %p\n", hwnd, himc );
    return TRUE;
}

/***********************************************************************
*              ImmRequestMessageA(IMM32.@)
*/
LRESULT WINAPI ImmRequestMessageA( HIMC himc, WPARAM wparam, LPARAM lparam )
{
    INPUTCONTEXT *ctx;
    LRESULT res;

    TRACE( "himc %p, wparam %#Ix, lparam %#Ix\n", himc, wparam, lparam );

    if (NtUserQueryInputContext( himc, NtUserInputContextThreadId ) != GetCurrentThreadId()) return FALSE;

    switch (wparam)
    {
    case IMR_CANDIDATEWINDOW:
    case IMR_COMPOSITIONFONT:
    case IMR_COMPOSITIONWINDOW:
    case IMR_CONFIRMRECONVERTSTRING:
    case IMR_DOCUMENTFEED:
    case IMR_QUERYCHARPOSITION:
    case IMR_RECONVERTSTRING:
        break;
    default:
        return FALSE;
    }

    if (!(ctx = ImmLockIMC( himc ))) return FALSE;
    res = SendMessageA( ctx->hWnd, WM_IME_REQUEST, wparam, lparam );
    ImmUnlockIMC( himc );

    return res;
}

/***********************************************************************
*              ImmRequestMessageW(IMM32.@)
*/
LRESULT WINAPI ImmRequestMessageW( HIMC himc, WPARAM wparam, LPARAM lparam )
{
    INPUTCONTEXT *ctx;
    LRESULT res;

    TRACE( "himc %p, wparam %#Ix, lparam %#Ix\n", himc, wparam, lparam );

    if (NtUserQueryInputContext( himc, NtUserInputContextThreadId ) != GetCurrentThreadId()) return FALSE;

    switch (wparam)
    {
    case IMR_CANDIDATEWINDOW:
    case IMR_COMPOSITIONFONT:
    case IMR_COMPOSITIONWINDOW:
    case IMR_CONFIRMRECONVERTSTRING:
    case IMR_DOCUMENTFEED:
    case IMR_QUERYCHARPOSITION:
    case IMR_RECONVERTSTRING:
        break;
    default:
        return FALSE;
    }

    if (!(ctx = ImmLockIMC( himc ))) return FALSE;
    res = SendMessageW( ctx->hWnd, WM_IME_REQUEST, wparam, lparam );
    ImmUnlockIMC( himc );

    return res;
}

/***********************************************************************
 *		ImmSetCandidateWindow (IMM32.@)
 */
BOOL WINAPI ImmSetCandidateWindow( HIMC himc, CANDIDATEFORM *candidate )
{
    INPUTCONTEXT *ctx;

    TRACE( "hwnd %p, candidate %s\n", himc, debugstr_candidate( candidate ) );

    if (!candidate) return FALSE;
    if (candidate->dwIndex >= ARRAY_SIZE(ctx->cfCandForm)) return FALSE;

    if (NtUserQueryInputContext( himc, NtUserInputContextThreadId ) != GetCurrentThreadId()) return FALSE;
    if (!(ctx = ImmLockIMC( himc ))) return FALSE;

    ctx->cfCandForm[candidate->dwIndex] = *candidate;

    ImmNotifyIME( himc, NI_CONTEXTUPDATED, 0, IMC_SETCANDIDATEPOS );
    SendMessageW( ctx->hWnd, WM_IME_NOTIFY, IMN_SETCANDIDATEPOS, 1 << candidate->dwIndex );

    ImmUnlockIMC( himc );

    return TRUE;
}

/***********************************************************************
 *		ImmSetCompositionFontA (IMM32.@)
 */
BOOL WINAPI ImmSetCompositionFontA( HIMC himc, LOGFONTA *fontA )
{
    INPUTCONTEXT *ctx;
    BOOL ret = TRUE;

    TRACE( "hwnd %p, fontA %p\n", himc, fontA );

    if (!fontA) return FALSE;

    if (NtUserQueryInputContext( himc, NtUserInputContextThreadId ) != GetCurrentThreadId()) return FALSE;
    if (!(ctx = ImmLockIMC( himc ))) return FALSE;

    if (input_context_is_unicode( ctx ))
    {
        LOGFONTW fontW;
        memcpy( &fontW, fontA, offsetof(LOGFONTW, lfFaceName) );
        MultiByteToWideChar( CP_ACP, 0, fontA->lfFaceName, -1, fontW.lfFaceName, LF_FACESIZE );
        ret = ImmSetCompositionFontW( himc, &fontW );
    }
    else
    {
        ctx->lfFont.A = *fontA;
        ctx->fdwInit |= INIT_LOGFONT;

        ImmNotifyIME( himc, NI_CONTEXTUPDATED, 0, IMC_SETCOMPOSITIONFONT );
        SendMessageW( ctx->hWnd, WM_IME_NOTIFY, IMN_SETCOMPOSITIONFONT, 0 );
    }

    ImmUnlockIMC( himc );

    return ret;
}

/***********************************************************************
 *		ImmSetCompositionFontW (IMM32.@)
 */
BOOL WINAPI ImmSetCompositionFontW( HIMC himc, LOGFONTW *fontW )
{
    INPUTCONTEXT *ctx;
    BOOL ret = TRUE;

    TRACE( "hwnd %p, fontW %p\n", himc, fontW );

    if (!fontW) return FALSE;

    if (NtUserQueryInputContext( himc, NtUserInputContextThreadId ) != GetCurrentThreadId()) return FALSE;
    if (!(ctx = ImmLockIMC( himc ))) return FALSE;

    if (!input_context_is_unicode( ctx ))
    {
        LOGFONTA fontA;
        memcpy( &fontA, fontW, offsetof(LOGFONTA, lfFaceName) );
        WideCharToMultiByte( CP_ACP, 0, fontW->lfFaceName, -1, fontA.lfFaceName, LF_FACESIZE, NULL, NULL );
        ret = ImmSetCompositionFontA( himc, &fontA );
    }
    else
    {
        ctx->lfFont.W = *fontW;
        ctx->fdwInit |= INIT_LOGFONT;

        ImmNotifyIME( himc, NI_CONTEXTUPDATED, 0, IMC_SETCOMPOSITIONFONT );
        SendMessageW( ctx->hWnd, WM_IME_NOTIFY, IMN_SETCOMPOSITIONFONT, 0 );
    }

    ImmUnlockIMC( himc );

    return ret;
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
    struct imc *data = get_imc_data( hIMC );
    struct ime *ime;

    TRACE("(%p, %ld, %p, %ld, %p, %ld):\n",
            hIMC, dwIndex, lpComp, dwCompLen, lpRead, dwReadLen);

    if (!data)
        return FALSE;

    if (NtUserQueryInputContext( hIMC, NtUserInputContextThreadId ) != GetCurrentThreadId()) return FALSE;

    if (!(dwIndex == SCS_SETSTR ||
          dwIndex == SCS_CHANGEATTR ||
          dwIndex == SCS_CHANGECLAUSE ||
          dwIndex == SCS_SETRECONVERTSTRING ||
          dwIndex == SCS_QUERYRECONVERTSTRING))
        return FALSE;

    if (!(ime = imc_select_ime( data ))) return FALSE;
    if (!lpComp) dwCompLen = 0;
    if (!lpRead) dwReadLen = 0;

    if (!ime_is_unicode( ime )) return ime->pImeSetCompositionString( hIMC, dwIndex, lpComp, dwCompLen, lpRead, dwReadLen );

    comp_len = MultiByteToWideChar(CP_ACP, 0, lpComp, dwCompLen, NULL, 0);
    if (comp_len)
    {
        CompBuffer = malloc( comp_len * sizeof(WCHAR) );
        MultiByteToWideChar(CP_ACP, 0, lpComp, dwCompLen, CompBuffer, comp_len);
    }

    read_len = MultiByteToWideChar(CP_ACP, 0, lpRead, dwReadLen, NULL, 0);
    if (read_len)
    {
        ReadBuffer = malloc( read_len * sizeof(WCHAR) );
        MultiByteToWideChar(CP_ACP, 0, lpRead, dwReadLen, ReadBuffer, read_len);
    }

    rc =  ImmSetCompositionStringW(hIMC, dwIndex, CompBuffer, comp_len,
                                   ReadBuffer, read_len);

    free( CompBuffer );
    free( ReadBuffer );

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
    struct imc *data = get_imc_data( hIMC );
    struct ime *ime;

    TRACE("(%p, %ld, %p, %ld, %p, %ld):\n",
            hIMC, dwIndex, lpComp, dwCompLen, lpRead, dwReadLen);

    if (!data)
        return FALSE;

    if (NtUserQueryInputContext( hIMC, NtUserInputContextThreadId ) != GetCurrentThreadId()) return FALSE;

    if (!(dwIndex == SCS_SETSTR ||
          dwIndex == SCS_CHANGEATTR ||
          dwIndex == SCS_CHANGECLAUSE ||
          dwIndex == SCS_SETRECONVERTSTRING ||
          dwIndex == SCS_QUERYRECONVERTSTRING))
        return FALSE;

    if (!(ime = imc_select_ime( data ))) return FALSE;
    if (!lpComp) dwCompLen = 0;
    if (!lpRead) dwReadLen = 0;

    if (ime_is_unicode( ime )) return ime->pImeSetCompositionString( hIMC, dwIndex, lpComp, dwCompLen, lpRead, dwReadLen );

    comp_len = WideCharToMultiByte(CP_ACP, 0, lpComp, dwCompLen, NULL, 0, NULL,
                                   NULL);
    if (comp_len)
    {
        CompBuffer = malloc( comp_len );
        WideCharToMultiByte(CP_ACP, 0, lpComp, dwCompLen, CompBuffer, comp_len,
                            NULL, NULL);
    }

    read_len = WideCharToMultiByte(CP_ACP, 0, lpRead, dwReadLen, NULL, 0, NULL,
                                   NULL);
    if (read_len)
    {
        ReadBuffer = malloc( read_len );
        WideCharToMultiByte(CP_ACP, 0, lpRead, dwReadLen, ReadBuffer, read_len,
                            NULL, NULL);
    }

    rc =  ImmSetCompositionStringA(hIMC, dwIndex, CompBuffer, comp_len,
                                   ReadBuffer, read_len);

    free( CompBuffer );
    free( ReadBuffer );

    return rc;
}

/***********************************************************************
 *		ImmSetCompositionWindow (IMM32.@)
 */
BOOL WINAPI ImmSetCompositionWindow( HIMC himc, COMPOSITIONFORM *composition )
{
    INPUTCONTEXT *ctx;
    RECT rect;

    TRACE( "himc %p, composition %s\n", himc, debugstr_composition( composition ) );

    if (NtUserQueryInputContext( himc, NtUserInputContextThreadId ) != GetCurrentThreadId()) return FALSE;
    if (!(ctx = ImmLockIMC( himc ))) return FALSE;

    ctx->cfCompForm = *composition;
    ctx->fdwInit |= INIT_COMPFORM;

    ImmNotifyIME( himc, NI_CONTEXTUPDATED, 0, IMC_SETCOMPOSITIONWINDOW );
    SendMessageW( ctx->hWnd, WM_IME_NOTIFY, IMN_SETCOMPOSITIONWINDOW, 0 );

    if (composition->dwStyle & (CFS_RECT | CFS_POINT | CFS_FORCE_POSITION))
    {
        if (composition->dwStyle & CFS_RECT) rect = composition->rcArea;
        else SetRect( &rect, composition->ptCurrentPos.x, composition->ptCurrentPos.y,
                      composition->ptCurrentPos.x + 1, composition->ptCurrentPos.y + 1 );
        NtUserCallTwoParam( (ULONG_PTR)ctx->hWnd, (ULONG_PTR)&rect, NtUserCallTwoParam_SetIMECompositionRect );
    }

    ImmUnlockIMC( himc );

    return TRUE;
}

/***********************************************************************
 *		ImmSetConversionStatus (IMM32.@)
 */
BOOL WINAPI ImmSetConversionStatus( HIMC himc, DWORD conversion, DWORD sentence )
{
    DWORD old_conversion, old_sentence;
    INPUTCONTEXT *ctx;

    TRACE( "himc %p, conversion %#lx, sentence %#lx\n", himc, conversion, sentence );

    if (NtUserQueryInputContext( himc, NtUserInputContextThreadId ) != GetCurrentThreadId()) return FALSE;
    if (!(ctx = ImmLockIMC( himc ))) return FALSE;

    if (conversion != ctx->fdwConversion)
    {
        old_conversion = ctx->fdwConversion;
        ctx->fdwConversion = conversion;
        ImmNotifyIME( himc, NI_CONTEXTUPDATED, old_conversion, IMC_SETCONVERSIONMODE );
        SendMessageW( ctx->hWnd, WM_IME_NOTIFY, IMN_SETCONVERSIONMODE, 0 );
    }

    if (sentence != ctx->fdwSentence)
    {
        old_sentence = ctx->fdwSentence;
        ctx->fdwSentence = sentence;
        ImmNotifyIME( himc, NI_CONTEXTUPDATED, old_sentence, IMC_SETSENTENCEMODE );
        SendMessageW( ctx->hWnd, WM_IME_NOTIFY, IMN_SETSENTENCEMODE, 0 );
    }

    ImmUnlockIMC( himc );

    return TRUE;
}

/***********************************************************************
 *		ImmSetOpenStatus (IMM32.@)
 */
BOOL WINAPI ImmSetOpenStatus( HIMC himc, BOOL status )
{
    INPUTCONTEXT *ctx;

    TRACE( "himc %p, status %u\n", himc, status );

    if (NtUserQueryInputContext( himc, NtUserInputContextThreadId ) != GetCurrentThreadId()) return FALSE;
    if (!(ctx = ImmLockIMC( himc ))) return FALSE;

    if (status != ctx->fOpen)
    {
        ctx->fOpen = status;
        ImmNotifyIME( himc, NI_CONTEXTUPDATED, 0, IMC_SETOPENSTATUS );
        SendMessageW( ctx->hWnd, WM_IME_NOTIFY, IMN_SETOPENSTATUS, 0 );
    }

    ImmUnlockIMC( himc );

    return TRUE;
}

/***********************************************************************
 *		ImmSetStatusWindowPos (IMM32.@)
 */
BOOL WINAPI ImmSetStatusWindowPos( HIMC himc, POINT *pos )
{
    INPUTCONTEXT *ctx;

    TRACE( "himc %p, pos %s\n", himc, wine_dbgstr_point( pos ) );

    if (!pos)
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return FALSE;
    }

    if (NtUserQueryInputContext( himc, NtUserInputContextThreadId ) != GetCurrentThreadId()) return FALSE;
    if (!(ctx = ImmLockIMC( himc ))) return FALSE;

    ctx->ptStatusWndPos = *pos;
    ctx->fdwInit |= INIT_STATUSWNDPOS;

    ImmNotifyIME( himc, NI_CONTEXTUPDATED, 0, IMC_SETSTATUSWINDOWPOS );
    SendMessageW( ctx->hWnd, WM_IME_NOTIFY, IMN_SETSTATUSWINDOWPOS, 0 );

    ImmUnlockIMC( himc );

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
BOOL WINAPI ImmUnregisterWordA( HKL hkl, const char *readingA, DWORD style, const char *stringA )
{
    struct ime *ime;
    BOOL ret;

    TRACE( "hkl %p, readingA %s, style %lu, stringA %s.\n", hkl, debugstr_a(readingA), style, debugstr_a(stringA) );

    if (!(ime = ime_acquire( hkl ))) return FALSE;

    if (!ime_is_unicode( ime ))
        ret = ime->pImeUnregisterWord( readingA, style, stringA );
    else
    {
        WCHAR *readingW = strdupAtoW( readingA ), *stringW = strdupAtoW( stringA );
        ret = ime->pImeUnregisterWord( readingW, style, stringW );
        free( readingW );
        free( stringW );
    }

    ime_release( ime );
    return ret;
}

/***********************************************************************
 *		ImmUnregisterWordW (IMM32.@)
 */
BOOL WINAPI ImmUnregisterWordW( HKL hkl, const WCHAR *readingW, DWORD style, const WCHAR *stringW )
{
    struct ime *ime;
    BOOL ret;

    TRACE( "hkl %p, readingW %s, style %lu, stringW %s.\n", hkl, debugstr_w(readingW), style, debugstr_w(stringW) );

    if (!(ime = ime_acquire( hkl ))) return FALSE;

    if (ime_is_unicode( ime ))
        ret = ime->pImeUnregisterWord( readingW, style, stringW );
    else
    {
        char *readingA = strdupWtoA( readingW ), *stringA = strdupWtoA( stringW );
        ret = ime->pImeUnregisterWord( readingA, style, stringA );
        free( readingA );
        free( stringA );
    }

    ime_release( ime );
    return ret;
}

/***********************************************************************
 *		ImmGetImeMenuItemsA (IMM32.@)
 */
DWORD WINAPI ImmGetImeMenuItemsA( HIMC himc, DWORD flags, DWORD type, IMEMENUITEMINFOA *parentA,
                                  IMEMENUITEMINFOA *menuA, DWORD size )
{
    struct imc *data = get_imc_data( himc );
    struct ime *ime;
    DWORD ret;

    TRACE( "himc %p, flags %#lx, type %lu, parentA %p, menuA %p, size %lu.\n",
           himc, flags, type, parentA, menuA, size );

    if (!data)
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return 0;
    }

    if (!(ime = imc_select_ime( data ))) return 0;
    if (!ime_is_unicode( ime ) || (!parentA && !menuA))
        ret = ime->pImeGetImeMenuItems( himc, flags, type, parentA, menuA, size );
    else
    {
        IMEMENUITEMINFOW tmpW, *menuW, *parentW = parentA ? &tmpW : NULL;

        if (!menuA) menuW = NULL;
        else
        {
            int count = size / sizeof(LPIMEMENUITEMINFOA);
            size = count * sizeof(IMEMENUITEMINFOW);
            menuW = malloc( size );
        }

        ret = ime->pImeGetImeMenuItems( himc, flags, type, parentW, menuW, size );

        if (parentA)
        {
            memcpy( parentA, parentW, sizeof(IMEMENUITEMINFOA) );
            parentA->hbmpItem = parentW->hbmpItem;
            WideCharToMultiByte( CP_ACP, 0, parentW->szString, -1, parentA->szString,
                                 IMEMENUITEM_STRING_SIZE, NULL, NULL );
        }
        if (menuA && ret)
        {
            unsigned int i;
            for (i = 0; i < ret; i++)
            {
                memcpy( &menuA[i], &menuW[1], sizeof(IMEMENUITEMINFOA) );
                menuA[i].hbmpItem = menuW[i].hbmpItem;
                WideCharToMultiByte( CP_ACP, 0, menuW[i].szString, -1, menuA[i].szString,
                                     IMEMENUITEM_STRING_SIZE, NULL, NULL );
            }
        }
        free( menuW );
    }

    return ret;
}

/***********************************************************************
*		ImmGetImeMenuItemsW (IMM32.@)
*/
DWORD WINAPI ImmGetImeMenuItemsW( HIMC himc, DWORD flags, DWORD type, IMEMENUITEMINFOW *parentW,
                                  IMEMENUITEMINFOW *menuW, DWORD size )
{
    struct imc *data = get_imc_data( himc );
    struct ime *ime;
    DWORD ret;

    TRACE( "himc %p, flags %#lx, type %lu, parentW %p, menuW %p, size %lu.\n",
           himc, flags, type, parentW, menuW, size );

    if (!data)
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return 0;
    }

    if (!(ime = imc_select_ime( data ))) return 0;
    if (ime_is_unicode( ime ) || (!parentW && !menuW))
        ret = ime->pImeGetImeMenuItems( himc, flags, type, parentW, menuW, size );
    else
    {
        IMEMENUITEMINFOA tmpA, *menuA, *parentA = parentW ? &tmpA : NULL;

        if (!menuW) menuA = NULL;
        else
        {
            int count = size / sizeof(LPIMEMENUITEMINFOW);
            size = count * sizeof(IMEMENUITEMINFOA);
            menuA = malloc( size );
        }

        ret = ime->pImeGetImeMenuItems( himc, flags, type, parentA, menuA, size );

        if (parentW)
        {
            memcpy( parentW, parentA, sizeof(IMEMENUITEMINFOA) );
            parentW->hbmpItem = parentA->hbmpItem;
            MultiByteToWideChar( CP_ACP, 0, parentA->szString, -1, parentW->szString, IMEMENUITEM_STRING_SIZE );
        }
        if (menuW && ret)
        {
            unsigned int i;
            for (i = 0; i < ret; i++)
            {
                memcpy( &menuW[i], &menuA[1], sizeof(IMEMENUITEMINFOA) );
                menuW[i].hbmpItem = menuA[i].hbmpItem;
                MultiByteToWideChar( CP_ACP, 0, menuA[i].szString, -1, menuW[i].szString, IMEMENUITEM_STRING_SIZE );
            }
        }
        free( menuA );
    }

    return ret;
}

/***********************************************************************
*		ImmLockIMC(IMM32.@)
*/
INPUTCONTEXT *WINAPI ImmLockIMC( HIMC himc )
{
    struct imc *imc = get_imc_data( himc );

    TRACE( "himc %p\n", himc );

    if (!imc) return NULL;
    imc->dwLock++;

    imc_select_ime( imc );
    return &imc->IMC;
}

/***********************************************************************
*		ImmUnlockIMC(IMM32.@)
*/
BOOL WINAPI ImmUnlockIMC(HIMC hIMC)
{
    struct imc *data = get_imc_data( hIMC );

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
    struct imc *data = get_imc_data( hIMC );
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
BOOL WINAPI ImmGenerateMessage( HIMC himc )
{
    INPUTCONTEXT *ctx;

    TRACE( "himc %p\n", himc );

    if (NtUserQueryInputContext( himc, NtUserInputContextThreadId ) != GetCurrentThreadId()) return FALSE;
    if (!(ctx = ImmLockIMC( himc ))) return FALSE;

    while (ctx->dwNumMsgBuf--)
    {
        TRANSMSG *msgs, msg;
        if (!(msgs = ImmLockIMCC( ctx->hMsgBuf ))) return FALSE;
        msg = msgs[0];
        memmove( msgs, msgs + 1, ctx->dwNumMsgBuf * sizeof(*msgs) );
        ImmUnlockIMCC( ctx->hMsgBuf );
        SendMessageW( ctx->hWnd, msg.message, msg.wParam, msg.lParam );
    }
    ctx->dwNumMsgBuf++;

    return TRUE;
}

/***********************************************************************
*       ImmTranslateMessage(IMM32.@)
*       ( Undocumented, call internally and from user32.dll )
*/
BOOL WINAPI ImmTranslateMessage( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    union
    {
        struct
        {
            UINT uMsgCount;
            TRANSMSG TransMsg[10];
        };
        TRANSMSGLIST list;
    } buffer = {.uMsgCount = ARRAY_SIZE(buffer.TransMsg)};
    TRANSMSG *msgs = buffer.TransMsg;
    UINT scan, vkey, count, i;
    struct imc *data;
    struct ime *ime;
    BYTE state[256];
    WCHAR chr;

    TRACE( "hwnd %p, msg %#x, wparam %#Ix, lparam %#Ix\n", hwnd, msg, wparam, lparam );

    if (msg < WM_KEYDOWN || msg > WM_KEYUP) return FALSE;
    if (!(data = get_imc_data( ImmGetContext( hwnd ) ))) return FALSE;
    if (!(ime = imc_select_ime( data ))) return FALSE;

    if ((vkey = data->vkey) == VK_PROCESSKEY) return FALSE;
    data->vkey = VK_PROCESSKEY;
    GetKeyboardState( state );
    scan = (lparam >> 0x10) & 0xffff;

    if (ime->info.fdwProperty & IME_PROP_KBD_CHAR_FIRST)
    {
        if (!ime_is_unicode( ime )) ToAscii( vkey, scan, state, &chr, 0 );
        else ToUnicodeEx( vkey, scan, state, &chr, 1, 0, GetKeyboardLayout( 0 ) );
        vkey = MAKELONG( vkey, chr );
    }

    count = ime->pImeToAsciiEx( vkey, scan, state, &buffer.list, 0, data->handle );
    if (count >= ARRAY_SIZE(buffer.TransMsg)) return 0;

    for (i = 0; i < count; i++) PostMessageW( hwnd, msgs[i].message, msgs[i].wParam, msgs[i].lParam );
    TRACE( "%u messages generated\n", count );

    return count > 0;
}

/***********************************************************************
*		ImmProcessKey(IMM32.@)
*       ( Undocumented, called from user32.dll )
*/
BOOL WINAPI ImmProcessKey( HWND hwnd, HKL hkl, UINT vkey, LPARAM lparam, DWORD unknown )
{
    struct imc *imc;
    struct ime *ime;
    BYTE state[256];
    BOOL ret;

    TRACE( "hwnd %p, hkl %p, vkey %#x, lparam %#Ix, unknown %#lx\n", hwnd, hkl, vkey, lparam, unknown );

    if (hkl != GetKeyboardLayout( 0 )) return FALSE;
    if (!(imc = get_imc_data( ImmGetContext( hwnd ) ))) return FALSE;
    if (!(ime = imc_select_ime( imc ))) return FALSE;

    GetKeyboardState( state );

    ret = ime->pImeProcessKey( imc->handle, vkey, lparam, state );
    imc->vkey = ret ? vkey : VK_PROCESSKEY;

    return ret;
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

BOOL WINAPI ImmEnumInputContext( DWORD thread, IMCENUMPROC callback, LPARAM lparam )
{
    HIMC buffer[256];
    NTSTATUS status;
    UINT i, size;

    TRACE( "thread %lu, callback %p, lparam %#Ix\n", thread, callback, lparam );

    if ((status = NtUserBuildHimcList( thread, ARRAY_SIZE(buffer), buffer, &size )))
    {
        RtlSetLastWin32Error( RtlNtStatusToDosError( status ) );
        WARN( "NtUserBuildHimcList returned %#lx\n", status );
        return FALSE;
    }

    if (size == ARRAY_SIZE(buffer)) FIXME( "NtUserBuildHimcList returned %u handles\n", size );
    for (i = 0; i < size; i++) if (!callback( buffer[i], lparam )) return FALSE;

    return TRUE;
}

/***********************************************************************
 *              ImmGetHotKey(IMM32.@)
 */

BOOL WINAPI ImmGetHotKey(DWORD hotkey, UINT *modifiers, UINT *key, HKL *hkl)
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
    HWND hwnd;
    HIMC himc;

    switch (wparam)
    {
    case IME_INTERNAL_ACTIVATE:
        hwnd = (HWND)lparam;
        himc = NtUserGetWindowInputContext( hwnd );
        ImmSetActiveContext( hwnd, himc, TRUE );
        set_ime_ui_window_himc( himc );
        break;
    case IME_INTERNAL_DEACTIVATE:
        hwnd = (HWND)lparam;
        himc = NtUserGetWindowInputContext( hwnd );
        ImmSetActiveContext( hwnd, himc, FALSE );
        break;
    case IME_INTERNAL_HKL_ACTIVATE:
        ImmEnumInputContext( 0, enum_activate_layout, 0 );
        if (!(hwnd = get_ime_ui_window())) break;
        SendMessageW( hwnd, WM_IME_SELECT, TRUE, lparam );
        break;
   case IME_INTERNAL_HKL_DEACTIVATE:
        if (!(hwnd = get_ime_ui_window())) break;
        SendMessageW( hwnd, WM_IME_SELECT, FALSE, lparam );
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
    HWND ui_hwnd;

    TRACE( "hwnd %p, msg %s, wparam %#Ix, lparam %#Ix, ansi %u\n",
           hwnd, debugstr_wm_ime(msg), wparam, lparam, ansi );

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
        if ((ui_hwnd = get_ime_ui_window()))
        {
            if (ansi)
                return SendMessageA(ui_hwnd, msg, wparam, lparam);
            else
                return SendMessageW(ui_hwnd, msg, wparam, lparam);
        }
        return FALSE;
    }

    if (ansi)
        return DefWindowProcA(hwnd, msg, wparam, lparam);
    else
        return DefWindowProcW(hwnd, msg, wparam, lparam);
}

/***********************************************************************
 *      CtfImmIsCiceroEnabled (IMM32.@)
 */
BOOL WINAPI CtfImmIsCiceroEnabled(void)
{
    FIXME("(): stub\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}
