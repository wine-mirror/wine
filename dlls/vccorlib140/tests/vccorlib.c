/*
 * Unit tests for miscellaneous vccorlib functions
 *
 * Copyright 2025 Piotr Caban
 * Copyright 2025 Vibhav Pant
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

#include <stdbool.h>

#include "initguid.h"
#include "activation.h"
#include "objbase.h"
#include "weakreference.h"
#define WIDL_using_Windows_Foundation
#include "windows.foundation.h"
#include "winstring.h"
#include "wine/test.h"

#define DEFINE_EXPECT(func) \
    static BOOL expect_ ## func = FALSE; static unsigned int called_ ## func = 0

#define SET_EXPECT(func) \
    expect_ ## func = TRUE

#define CHECK_EXPECT2(func) \
    do { \
        ok(expect_ ##func, "unexpected call " #func "\n"); \
        called_ ## func++; \
    }while(0)

#define CHECK_EXPECT(func) \
    do { \
        CHECK_EXPECT2(func); \
        expect_ ## func = FALSE; \
    }while(0)

#define CHECK_CALLED(func, n) \
    do { \
        ok(called_ ## func == n, "expected " #func " called %u times, got %u\n", n, called_ ## func); \
        expect_ ## func = FALSE; \
        called_ ## func = 0; \
    }while(0)

#undef __thiscall
#ifdef __i386__

#pragma pack(push,1)
struct thiscall_thunk
{
    BYTE pop_eax;    /* popl  %eax (ret addr) */
    BYTE pop_edx;    /* popl  %edx (func) */
    BYTE pop_ecx;    /* popl  %ecx (this) */
    BYTE push_eax;   /* pushl %eax */
    WORD jmp_edx;    /* jmp  *%edx */
};
#pragma pack( pop )

static ULONG_PTR (WINAPI *call_thiscall_func1)(void *func, void *this);
static ULONG_PTR (WINAPI *call_thiscall_func2)(void *func, void *this, void *arg1);

static void init_thiscall_thunk(void)
{
    struct thiscall_thunk *thunk = VirtualAlloc(NULL, sizeof(*thunk), MEM_COMMIT, PAGE_EXECUTE_READWRITE);
    thunk->pop_eax  = 0x58;   /* popl  %eax */
    thunk->pop_edx  = 0x5a;   /* popl  %edx */
    thunk->pop_ecx  = 0x59;   /* popl  %ecx */
    thunk->push_eax = 0x50;   /* pushl %eax */
    thunk->jmp_edx  = 0xe2ff; /* jmp  *%edx */
    call_thiscall_func1 = (void *)thunk;
    call_thiscall_func2 = (void *)thunk;
}

#define __thiscall __stdcall
#define call_func1(func,_this) call_thiscall_func1(func,_this)
#define call_func2(func,_this,arg1) call_thiscall_func2(func, _this, arg1)

#else

#define init_thiscall_thunk()
#define __thiscall __cdecl
#define call_func1(func,_this) func(_this)
#define call_func2(func,_this,arg1) func(_this,arg1)

#endif /* __i386__ */

#ifdef __i386__
#define __thiscall __stdcall
#else
#define __thiscall __cdecl
#endif

DEFINE_EXPECT(PreInitialize);
DEFINE_EXPECT(PostInitialize);
DEFINE_EXPECT(PreUninitialize);
DEFINE_EXPECT(PostUninitialize);

struct __abi_type_descriptor
{
    const WCHAR *name;
    int type_id;
};

static HRESULT (__cdecl *pInitializeData)(int);
static void (__cdecl *pUninitializeData)(int);
static HRESULT (WINAPI *pGetActivationFactoryByPCWSTR)(const WCHAR *, const GUID *, void **);
static HRESULT (WINAPI *pGetIidsFn)(UINT32, UINT32 *, const GUID *, GUID **);
static void *(__cdecl *pAllocate)(size_t);
static void (__cdecl *pFree)(void *);
static void *(__cdecl *pAllocateWithWeakRef)(ptrdiff_t, size_t);
static void (__thiscall *pReleaseTarget)(void *);
static void *(WINAPI *p___abi_make_type_id)(const struct __abi_type_descriptor *desc);
static bool (__cdecl *p_platform_type_Equals_Object)(void *, void *);
static int (__cdecl *p_platform_type_GetTypeCode)(void *);
static HSTRING (__cdecl *p_platform_type_ToString)(void *);
static HSTRING (__cdecl *p_platform_type_get_FullName)(void *);
static void *(WINAPI *pCreateValue)(int type, const void *);
static void *(__cdecl *pAllocateException)(size_t);
static void (__cdecl *pFreeException)(void *);

static void *(__cdecl *p__RTtypeid)(const void *);
static const char *(__thiscall *p_type_info_name)(void *);
static const char *(__thiscall *p_type_info_raw_name)(void *);
static int (__thiscall *p_type_info_opequals_equals)(void *, void *);
static int (__thiscall *p_type_info_opnot_equals)(void *, void *);

static BOOL init(void)
{
    HMODULE hmod = LoadLibraryA("vccorlib140.dll");
    HMODULE msvcrt = LoadLibraryA("msvcrt.dll");

    if (!hmod)
    {
        win_skip("vccorlib140.dll not available\n");
        return FALSE;
    }

    pInitializeData = (void *)GetProcAddress(hmod,
            "?InitializeData@Details@Platform@@YAJH@Z");
    ok(pInitializeData != NULL, "InitializeData not available\n");
    pUninitializeData = (void *)GetProcAddress(hmod,
            "?UninitializeData@Details@Platform@@YAXH@Z");
    ok(pUninitializeData != NULL, "UninitializeData not available\n");
    p__RTtypeid = (void *)GetProcAddress(msvcrt, "__RTtypeid");
    ok(p__RTtypeid != NULL, "__RTtypeid not available\n");

#ifdef __arm__
    pGetActivationFactoryByPCWSTR = (void *)GetProcAddress(hmod,
            "?GetActivationFactoryByPCWSTR@@YAJPAXAAVGuid@Platform@@PAPAX@Z");
    pGetIidsFn = (void *)GetProcAddress(hmod, "?GetIidsFn@@YAJHPAKPBU__s_GUID@@PAPAVGuid@Platform@@@Z");
    pAllocate = (void *)GetProcAddress(hmod, "?Allocate@Heap@Details@Platform@@SAPAXI@Z");
    pFree = (void *)GetProcAddress(hmod, "?Free@Heap@Details@Platform@@SAXPAX@Z");
    pAllocateWithWeakRef = (void *)GetProcAddress(hmod, "?Allocate@Heap@Details@Platform@@SAPAXII@Z");
    pReleaseTarget = (void *)GetProcAddress(hmod, "?ReleaseTarget@ControlBlock@Details@Platform@@AAAXXZ");
    p___abi_make_type_id = (void *)GetProcAddress(hmod,
            "?__abi_make_type_id@@YAP$AAVType@Platform@@ABU__abi_type_descriptor@@@Z");
    p_platform_type_Equals_Object = (void *)GetProcAddress(hmod, "?Equals@Type@Platform@@U$AAA_NP$AAVObject@2@@Z");
    p_platform_type_GetTypeCode = (void *)GetProcAddress(hmod,
            "?GetTypeCode@Type@Platform@@SA?AW4TypeCode@2@P$AAV12@@Z");
    p_platform_type_ToString = (void *)GetProcAddress(hmod, "?ToString@Type@Platform@@U$AAAP$AAVString@2@XZ");
    p_platform_type_get_FullName = (void *)GetProcAddress(hmod, "?get@FullName@Type@Platform@@Q$AAAP$AAVString@3@XZ");
    pCreateValue = (void *)GetProcAddress(hmod, "?CreateValue@Details@Platform@@YAP$AAVObject@2@W4TypeCode@2@PBX@Z");
    pAllocateException = (void *)GetProcAddress(hmod, "?AllocateException@Heap@Details@Platform@@SAPAXI@Z");
    pFreeException = (void *)GetProcAddress(hmod, "?FreeException@Heap@Details@Platform@@SAXPAX@Z");
    p_type_info_name = (void *)GetProcAddress(msvcrt, "?name@type_info@@QBAPBDXZ");
    p_type_info_raw_name = (void *)GetProcAddress(msvcrt, "?raw_name@type_info@@QBAPBDXZ");
    p_type_info_opequals_equals = (void *)GetProcAddress(msvcrt, "??8type_info@@QBAHABV0@@Z");
    p_type_info_opnot_equals = (void *)GetProcAddress(msvcrt, "??9type_info@@QBAHABV0@@Z");
#else
    if (sizeof(void *) == 8)
    {
        pGetActivationFactoryByPCWSTR = (void *)GetProcAddress(hmod,
                "?GetActivationFactoryByPCWSTR@@YAJPEAXAEAVGuid@Platform@@PEAPEAX@Z");
        pGetIidsFn = (void *)GetProcAddress(hmod, "?GetIidsFn@@YAJHPEAKPEBU__s_GUID@@PEAPEAVGuid@Platform@@@Z");
        pAllocate = (void *)GetProcAddress(hmod, "?Allocate@Heap@Details@Platform@@SAPEAX_K@Z");
        pFree = (void *)GetProcAddress(hmod, "?Free@Heap@Details@Platform@@SAXPEAX@Z");
        pAllocateWithWeakRef = (void *)GetProcAddress(hmod, "?Allocate@Heap@Details@Platform@@SAPEAX_K0@Z");
        pReleaseTarget = (void *)GetProcAddress(hmod, "?ReleaseTarget@ControlBlock@Details@Platform@@AEAAXXZ");
        p___abi_make_type_id = (void *)GetProcAddress(hmod,
                "?__abi_make_type_id@@YAPE$AAVType@Platform@@AEBU__abi_type_descriptor@@@Z");
        p_platform_type_Equals_Object = (void *)GetProcAddress(hmod,
                "?Equals@Type@Platform@@UE$AAA_NPE$AAVObject@2@@Z");
        p_platform_type_GetTypeCode = (void *)GetProcAddress(hmod,
                "?GetTypeCode@Type@Platform@@SA?AW4TypeCode@2@PE$AAV12@@Z");
        p_platform_type_ToString = (void *)GetProcAddress(hmod, "?ToString@Type@Platform@@UE$AAAPE$AAVString@2@XZ");
        p_platform_type_get_FullName = (void *)GetProcAddress(hmod,
                "?get@FullName@Type@Platform@@QE$AAAPE$AAVString@3@XZ");
        pCreateValue = (void *)GetProcAddress(hmod,
                "?CreateValue@Details@Platform@@YAPE$AAVObject@2@W4TypeCode@2@PEBX@Z");
        pAllocateException = (void *)GetProcAddress(hmod, "?AllocateException@Heap@Details@Platform@@SAPEAX_K@Z");
        pFreeException = (void *)GetProcAddress(hmod, "?FreeException@Heap@Details@Platform@@SAXPEAX@Z");
        p_type_info_name = (void *)GetProcAddress(msvcrt, "?name@type_info@@QEBAPEBDXZ");
        p_type_info_raw_name = (void *)GetProcAddress(msvcrt, "?raw_name@type_info@@QEBAPEBDXZ");
        p_type_info_opequals_equals = (void *)GetProcAddress(msvcrt, "??8type_info@@QEBAHAEBV0@@Z");
        p_type_info_opnot_equals = (void *)GetProcAddress(msvcrt, "??9type_info@@QEBAHAEBV0@@Z");
    }
    else
    {
        pGetActivationFactoryByPCWSTR = (void *)GetProcAddress(hmod,
                "?GetActivationFactoryByPCWSTR@@YGJPAXAAVGuid@Platform@@PAPAX@Z");
        pGetIidsFn = (void *)GetProcAddress(hmod, "?GetIidsFn@@YGJHPAKPBU__s_GUID@@PAPAVGuid@Platform@@@Z");
        pAllocate = (void *)GetProcAddress(hmod, "?Allocate@Heap@Details@Platform@@SAPAXI@Z");
        pFree = (void *)GetProcAddress(hmod, "?Free@Heap@Details@Platform@@SAXPAX@Z");
        pAllocateWithWeakRef = (void *)GetProcAddress(hmod, "?Allocate@Heap@Details@Platform@@SAPAXII@Z");
        pReleaseTarget = (void *)GetProcAddress(hmod, "?ReleaseTarget@ControlBlock@Details@Platform@@AAEXXZ");
        p___abi_make_type_id = (void *)GetProcAddress(hmod,
                "?__abi_make_type_id@@YGP$AAVType@Platform@@ABU__abi_type_descriptor@@@Z");
        p_platform_type_Equals_Object = (void *)GetProcAddress(hmod, "?Equals@Type@Platform@@U$AAA_NP$AAVObject@2@@Z");
        p_platform_type_GetTypeCode = (void *)GetProcAddress(hmod,
                "?GetTypeCode@Type@Platform@@SA?AW4TypeCode@2@P$AAV12@@Z");
        p_platform_type_ToString = (void *)GetProcAddress(hmod, "?ToString@Type@Platform@@U$AAAP$AAVString@2@XZ");
        p_platform_type_get_FullName = (void *)GetProcAddress(hmod,
                "?get@FullName@Type@Platform@@Q$AAAP$AAVString@3@XZ");
        pCreateValue = (void *)GetProcAddress(hmod,
                "?CreateValue@Details@Platform@@YGP$AAVObject@2@W4TypeCode@2@PBX@Z");
        pAllocateException = (void *)GetProcAddress(hmod, "?AllocateException@Heap@Details@Platform@@SAPAXI@Z");
        pFreeException = (void *)GetProcAddress(hmod, "?FreeException@Heap@Details@Platform@@SAXPAX@Z");
        p_type_info_name = (void *)GetProcAddress(msvcrt, "?name@type_info@@QBEPBDXZ");
        p_type_info_raw_name = (void *)GetProcAddress(msvcrt, "?raw_name@type_info@@QBEPBDXZ");
        p_type_info_opequals_equals = (void *)GetProcAddress(msvcrt, "??8type_info@@QBEHABV0@@Z");
        p_type_info_opnot_equals = (void *)GetProcAddress(msvcrt, "??9type_info@@QBEHABV0@@Z");
    }
#endif
    ok(pGetActivationFactoryByPCWSTR != NULL, "GetActivationFactoryByPCWSTR not available\n");
    ok(pGetIidsFn != NULL, "GetIidsFn not available\n");
    ok(pAllocate != NULL, "Allocate not available\n");
    ok(pFree != NULL, "Free not available\n");
    ok(pAllocateWithWeakRef != NULL, "AllocateWithWeakRef not available\n");
    ok(pReleaseTarget != NULL, "ReleaseTarget not available\n");
    ok(p___abi_make_type_id != NULL, "__abi_make_type_id not available\n");
    ok(p_platform_type_Equals_Object != NULL, "Platform::Type::Equals not available\n");
    ok(p_platform_type_GetTypeCode != NULL, "Platform::Type::GetTypeCode not available\n");
    ok(p_platform_type_ToString != NULL, "Platform::Type::ToString not available\n");
    ok(p_platform_type_get_FullName != NULL, "Platform::Type::FullName not available\n");
    ok(pCreateValue != NULL, "CreateValue not available\n");
    ok(pAllocateException != NULL, "AllocateException not available\n");
    ok(pFreeException != NULL, "FreeException not available\n");

    ok(p_type_info_name != NULL, "type_info::name not available\n");
    ok(p_type_info_raw_name != NULL, "type_info::raw_name not available\n");
    ok(p_type_info_opequals_equals != NULL, "type_info::operator== not available\n");
    ok(p_type_info_opnot_equals != NULL, "type_info::operator!= not available\n");

    init_thiscall_thunk();

    return TRUE;
}

static HRESULT WINAPI InitializeSpy_QI(IInitializeSpy *iface, REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IInitializeSpy) || IsEqualIID(riid, &IID_IUnknown))
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
    return 2;
}

static ULONG WINAPI InitializeSpy_Release(IInitializeSpy *iface)
{
    return 1;
}

static DWORD exp_coinit;
static HRESULT WINAPI InitializeSpy_PreInitialize(IInitializeSpy *iface, DWORD coinit, DWORD aptrefs)
{
    CHECK_EXPECT(PreInitialize);
    ok(coinit == exp_coinit, "coinit = %lx\n", coinit);
    return S_OK;
}

static HRESULT WINAPI InitializeSpy_PostInitialize(IInitializeSpy *iface, HRESULT hr, DWORD coinit, DWORD aptrefs)
{
    CHECK_EXPECT(PostInitialize);
    return hr;
}

static HRESULT WINAPI InitializeSpy_PreUninitialize(IInitializeSpy *iface, DWORD aptrefs)
{
    CHECK_EXPECT(PreUninitialize);
    return S_OK;
}

static HRESULT WINAPI InitializeSpy_PostUninitialize(IInitializeSpy *iface, DWORD aptrefs)
{
    CHECK_EXPECT(PostUninitialize);
    return S_OK;
}

static const IInitializeSpyVtbl InitializeSpyVtbl =
{
    InitializeSpy_QI,
    InitializeSpy_AddRef,
    InitializeSpy_Release,
    InitializeSpy_PreInitialize,
    InitializeSpy_PostInitialize,
    InitializeSpy_PreUninitialize,
    InitializeSpy_PostUninitialize
};

static IInitializeSpy InitializeSpy = { &InitializeSpyVtbl };

static void test_InitializeData(void)
{
    ULARGE_INTEGER cookie;
    HRESULT hr;

    hr = CoRegisterInitializeSpy(&InitializeSpy, &cookie);
    ok(hr == S_OK, "CoRegisterInitializeSpy returned %lx\n", hr);

    hr = pInitializeData(0);
    ok(hr == S_OK, "InitializeData returned %lx\n", hr);

    exp_coinit = COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE;
    SET_EXPECT(PreInitialize);
    SET_EXPECT(PostInitialize);
    hr = pInitializeData(1);
    ok(hr == S_OK, "InitializeData returned %lx\n", hr);
    CHECK_CALLED(PreInitialize, 1);
    CHECK_CALLED(PostInitialize, 1);

    SET_EXPECT(PreInitialize);
    SET_EXPECT(PostInitialize);
    hr = pInitializeData(1);
    ok(hr == S_OK, "InitializeData returned %lx\n", hr);
    CHECK_CALLED(PreInitialize, 1);
    CHECK_CALLED(PostInitialize, 1);

    exp_coinit = COINIT_MULTITHREADED;
    SET_EXPECT(PreInitialize);
    SET_EXPECT(PostInitialize);
    hr = pInitializeData(2);
    ok(hr == RPC_E_CHANGED_MODE, "InitializeData returned %lx\n", hr);
    CHECK_CALLED(PreInitialize, 1);
    CHECK_CALLED(PostInitialize, 1);

    pUninitializeData(0);
    SET_EXPECT(PreUninitialize);
    SET_EXPECT(PostUninitialize);
    pUninitializeData(1);
    CHECK_CALLED(PreUninitialize, 1);
    CHECK_CALLED(PostUninitialize, 1);
    SET_EXPECT(PreUninitialize);
    SET_EXPECT(PostUninitialize);
    pUninitializeData(2);
    CHECK_CALLED(PreUninitialize, 1);
    CHECK_CALLED(PostUninitialize, 1);

    SET_EXPECT(PreInitialize);
    SET_EXPECT(PostInitialize);
    hr = pInitializeData(2);
    ok(hr == S_OK, "InitializeData returned %lx\n", hr);
    CHECK_CALLED(PreInitialize, 1);
    CHECK_CALLED(PostInitialize, 1);
    SET_EXPECT(PreUninitialize);
    SET_EXPECT(PostUninitialize);
    pUninitializeData(2);
    CHECK_CALLED(PreUninitialize, 1);
    CHECK_CALLED(PostUninitialize, 1);

    SET_EXPECT(PreInitialize);
    SET_EXPECT(PostInitialize);
    hr = pInitializeData(3);
    ok(hr == S_OK, "InitializeData returned %lx\n", hr);
    CHECK_CALLED(PreInitialize, 1);
    CHECK_CALLED(PostInitialize, 1);
    SET_EXPECT(PreUninitialize);
    SET_EXPECT(PostUninitialize);
    pUninitializeData(3);
    CHECK_CALLED(PreUninitialize, 1);
    CHECK_CALLED(PostUninitialize, 1);

    hr = CoRevokeInitializeSpy(cookie);
    ok(hr == S_OK, "CoRevokeInitializeSpy returned %lx\n", hr);
}

static const GUID guid_null = {0};

static void test_GetActivationFactoryByPCWSTR(void)
{
    HRESULT hr;
    void *out;

    hr = pGetActivationFactoryByPCWSTR(L"Wine.Nonexistent.RuntimeClass", &IID_IActivationFactory, &out);
    ok(hr == CO_E_NOTINITIALIZED, "got hr %#lx\n", hr);

    hr = pInitializeData(1);
    ok(hr == S_OK, "got hr %#lx\n", hr);

    hr = pGetActivationFactoryByPCWSTR(L"Wine.Nonexistent.RuntimeClass", &IID_IActivationFactory, &out);
    ok(hr == REGDB_E_CLASSNOTREG, "got hr %#lx\n", hr);

    hr = pGetActivationFactoryByPCWSTR(L"Windows.Foundation.Metadata.ApiInformation", &IID_IActivationFactory, &out);
    ok(hr == S_OK, "got hr %#lx\n", hr);
    IActivationFactory_Release(out);

    hr = pGetActivationFactoryByPCWSTR(L"Windows.Foundation.Metadata.ApiInformation", &IID_IInspectable, &out);
    ok(hr == S_OK, "got hr %#lx\n", hr);
    IActivationFactory_Release(out);

    hr = pGetActivationFactoryByPCWSTR(L"Windows.Foundation.Metadata.ApiInformation", &guid_null, &out);
    ok(hr == E_NOINTERFACE, "got hr %#lx\n", hr);

    pUninitializeData(1);
}

static void test_GetIidsFn(void)
{
    static const GUID guids_src[] = {IID_IUnknown, IID_IInspectable, IID_IAgileObject, IID_IMarshal, guid_null};
    GUID *guids_dest;
    UINT32 copied;
    HRESULT hr;

    guids_dest = NULL;
    copied = 0xdeadbeef;
    hr = pGetIidsFn(0, &copied, NULL, &guids_dest);
    ok(hr == S_OK, "got hr %#lx\n", hr);
    ok(copied == 0, "got copied %I32u\n", copied);
    ok(guids_dest != NULL, "got guids_dest %p\n", guids_dest);
    CoTaskMemFree(guids_dest);

    guids_dest = NULL;
    copied = 0;
    hr = pGetIidsFn(ARRAY_SIZE(guids_src), &copied, guids_src, &guids_dest);
    ok(hr == S_OK, "got hr %#lx\n", hr);
    ok(copied == ARRAY_SIZE(guids_src), "got copied %I32u\n", copied);
    ok(guids_dest != NULL, "got guids_dest %p\n", guids_dest);
    ok(!memcmp(guids_src, guids_dest, sizeof(*guids_dest) * copied), "unexpected guids_dest value.\n");
    CoTaskMemFree(guids_dest);
}

static void test_Allocate(void)
{
    void *addr, **ptr, **base;

    addr = pAllocate(0);
    ok(!!addr, "got addr %p\n", addr);
    pFree(addr);

    addr = pAllocate(sizeof(void *));
    ok(!!addr, "got addr %p\n", addr);
    pFree(addr);
    pFree(NULL);

    /* AllocateException allocates additional space for two pointer-width fields, with the second field used as the
     * back-pointer to the exception data. */
    addr = pAllocateException(0);
    todo_wine ok(!!addr, "got addr %p\n", addr);
    if (!addr) return;
    ptr = (void **)((ULONG_PTR)addr - sizeof(void *));
    *ptr = NULL; /* The write should succeed. */
    base = (void **)((ULONG_PTR)addr - 2 * sizeof(void *));
    *base = NULL;
    /* Since base is the actual allocation base, Free(base) should succeed. */
    pFree(base);

    addr = pAllocateException(sizeof(void *));
    ok(!!addr, "got addr %p\n", addr);
    ptr = (void **)((ULONG_PTR)addr - sizeof(void *));
    *ptr = NULL;
    base = (void **)((ULONG_PTR)addr - 2 * sizeof(void *));
    *base = NULL;
    /* FreeException will derive the allocation base itself. */
    pFreeException(addr);

    /* Do what AllocateException does, manually. */
    base = pAllocate(sizeof(void *) * 2 + sizeof(UINT32));
    ok(!!base, "got base %p\n", base);
    /* addr is what AllocateException would return. */
    addr = (void *)((ULONG_PTR)base + sizeof(void *) * 2);
    /* FreeException should succeed with addr. */
    pFreeException(addr);
}

#define test_refcount(a, b) test_refcount_(__LINE__, (a), (b))
static void test_refcount_(int line, void *obj, LONG val)
{
    LONG count;

    IUnknown_AddRef((IUnknown *)obj);
    count = IUnknown_Release((IUnknown *)obj);
    ok_(__FILE__, line)(count == val, "got refcount %lu != %lu\n", count, val);
}

struct control_block
{
    IWeakReference IWeakReference_iface;
    LONG ref_weak;
    LONG ref_strong;
    IUnknown *object;
    bool is_inline;
    UINT16 unknown;
#ifdef _WIN32
    char _padding[4];
#endif
};

struct unknown_impl
{
    IUnknown IUnknown_iface;
    ULONG strong_ref_free_val; /* Should be a negative value  */
    struct control_block *weakref;
};

static struct unknown_impl *impl_from_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, struct unknown_impl, IUnknown_iface);
}

static HRESULT WINAPI unknown_QueryInterface(IUnknown *iface, const GUID *iid, void **out)
{
    struct unknown_impl *impl = impl_from_IUnknown(iface);

    if (winetest_debug > 1)
        trace("(%p, %s, %p)\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown) || IsEqualGUID(iid, &IID_IAgileObject))
    {
        *out = &impl->IUnknown_iface;
        IUnknown_AddRef(&impl->IUnknown_iface);
        return S_OK;
    }

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI unknown_AddRef(IUnknown *iface)
{
    struct unknown_impl *impl = impl_from_IUnknown(iface);

    return InterlockedIncrement(&impl->weakref->ref_strong);
}

static ULONG WINAPI unknown_Release(IUnknown *iface)
{
    struct unknown_impl *impl = impl_from_IUnknown(iface);
    LONG ref = InterlockedDecrement(&impl->weakref->ref_strong);

    if (!ref)
    {
        struct control_block *weak = impl->weakref;
        BOOL is_inline = weak->is_inline;
        IUnknown *out = NULL;
        LONG count;
        HRESULT hr;

        /* The object will only be freed when the strong refcount is < 0. */
        call_func1(pReleaseTarget, weak);
        hr = IWeakReference_QueryInterface(&weak->IWeakReference_iface, &IID_IWeakReference, (void **)&out);
        ok(hr == S_OK, "got hr %#lx\n", hr);
        test_refcount(out, 3);
        IUnknown_Release(out);

        /* Resolve on native seems to *not* set out to NULL if the weak reference is no longer there. */
        out = (IUnknown *)0xdeadbeef;
        hr = IWeakReference_Resolve(&weak->IWeakReference_iface, &IID_IAgileObject, (IInspectable **)&out);
        ok(hr == S_OK, "got hr %#lx\n", hr);
        ok(out == (IUnknown *)0xdeadbeef, "got out %p\n", out);

        impl->weakref->ref_strong = impl->strong_ref_free_val;
        /* Frees this object. */
        call_func1(pReleaseTarget, weak);
        if (is_inline)
        {
            /* For inline allocations, ReleaseTarget should do nothing.  */
            out = NULL;
            hr = IWeakReference_QueryInterface(&weak->IWeakReference_iface, &IID_IWeakReference, (void **)&out);
            ok(hr == S_OK, "got hr %#lx\n", hr);
            test_refcount(out, 3);
            IUnknown_Release(out);
        }

        /* ReleaseTarget can still be called after the object has been freed. */
        call_func1(pReleaseTarget, weak);
        count = IWeakReference_Release(&weak->IWeakReference_iface);
        ok(count == 1, "got count %lu\n", count);
    }
    return ref;
}


static const IUnknownVtbl unknown_impl_vtbl =
{
    unknown_QueryInterface,
    unknown_AddRef,
    unknown_Release,
};

/* The maximum size for inline allocations. */
#ifdef _WIN64
#define INLINE_MAX 128
#else
#define INLINE_MAX 64
#endif
/* Make sure that unknown_impl can be allocated inline. */
C_ASSERT(sizeof(struct unknown_impl) <= INLINE_MAX);

static void test_AllocateWithWeakRef_inline(void)
{
    struct unknown_impl *object;
    IWeakReference *weakref;
    IUnknown *out;
    ULONG count;
    HRESULT hr;

    /* Test inline allocation. */
    object = pAllocateWithWeakRef(offsetof(struct unknown_impl, weakref), sizeof(struct unknown_impl));
    ok(object != NULL, "got object %p\n", object);
    if (!object)
    {
        skip("AllocateWithWeakRef returned NULL\n");
        return;
    }

    object->strong_ref_free_val = -1;
    ok(object->weakref != NULL, "got weakref %p\n", object->weakref);
    object->IUnknown_iface.lpVtbl = &unknown_impl_vtbl;
    weakref = &object->weakref->IWeakReference_iface;
    test_refcount(weakref, 1);
    ok(object->weakref->is_inline, "got is_inline %d\n", object->weakref->is_inline);
    ok(object->weakref->ref_strong == 1, "got ref_strong %lu\n", object->weakref->ref_strong);
    ok(object->weakref->object == &object->IUnknown_iface, "got object %p != %p\n", object->weakref->object,
       &object->IUnknown_iface);
    ok(object->weakref->unknown == 0, "got unknown %d\n", object->weakref->unknown);
    /* The object is allocate within the weakref. */
    ok((char *)object->weakref == ((char *)object - sizeof(struct control_block)), "got %p != %p\n", object->weakref,
       (char *)object - sizeof(struct control_block));

    hr = IWeakReference_Resolve(weakref, &IID_IAgileObject, (IInspectable **)&out);
    ok(hr == S_OK, "got hr %#lx\n", hr);
    test_refcount(&object->IUnknown_iface, 2);
    IUnknown_Release(out);

    /* Doesn't do anything if the object is still available. */
    call_func1(pReleaseTarget, object->weakref);
    hr = IWeakReference_Resolve(weakref, &IID_IAgileObject, (IInspectable **)&out);
    ok(hr == S_OK, "got hr %#lx\n", hr);
    test_refcount(&object->IUnknown_iface, 2);
    IUnknown_Release(out);

    count = IWeakReference_AddRef(weakref);
    ok(count == 2, "got count %lu\n", count);

    count = IUnknown_Release(&object->IUnknown_iface);
    ok(count == 0, "got count %lu\n", count);
    test_refcount(weakref, 1);
    out = (IUnknown *)0xdeadbeef;
    hr = IWeakReference_Resolve(weakref, &IID_IAgileObject, (IInspectable **)&out);
    ok(hr == S_OK, "got hr %#lx\n", hr);
    ok(out == (IUnknown *)0xdeadbeef, "got out %p\n", out);
    count = IWeakReference_Release(weakref);
    ok(count == 0, "got count %lu\n", count);
}

static void test_AllocateWithWeakRef(void)
{
    struct unknown_impl *object;
    IWeakReference *weakref;
    IUnknown *out;
    ULONG count;
    HRESULT hr;

    /* Test non-inline allocation. */
    object = pAllocateWithWeakRef(offsetof(struct unknown_impl, weakref), INLINE_MAX + 1);
    ok(object != NULL, "got object %p\n", object);
    if (!object)
    {
        skip("AllocateWithWeakRef returned NULL\n");
        return;
    }

    object->strong_ref_free_val = -100;
    ok(object->weakref != NULL, "got weakref %p\n", object->weakref);
    object->IUnknown_iface.lpVtbl = &unknown_impl_vtbl;
    weakref = &object->weakref->IWeakReference_iface;
    test_refcount(weakref, 1);
    ok(!object->weakref->is_inline, "got is_inline %d\n", object->weakref->is_inline);
    ok(object->weakref->ref_strong == 1, "got ref_strong %lu\n", object->weakref->ref_strong);
    ok(object->weakref->object == &object->IUnknown_iface, "got object %p != %p\n", object->weakref->object,
       &object->IUnknown_iface);
    ok(object->weakref->unknown == 0, "got unknown %d\n", object->weakref->unknown);

    hr = IWeakReference_Resolve(weakref, &IID_IAgileObject, (IInspectable **)&out);
    ok(hr == S_OK, "got hr %#lx\n", hr);
    test_refcount(&object->IUnknown_iface, 2);
    IUnknown_Release(out);

    call_func1(pReleaseTarget, object->weakref);
    hr = IWeakReference_Resolve(weakref, &IID_IAgileObject, (IInspectable **)&out);
    ok(hr == S_OK, "got hr %#lx\n", hr);
    test_refcount(&object->IUnknown_iface, 2);
    IUnknown_Release(out);

    count = IWeakReference_AddRef(weakref);
    ok(count == 2, "got count %lu\n", count);

    count = IUnknown_Release(&object->IUnknown_iface);
    ok(count == 0, "got count %lu\n", count);
    test_refcount(weakref, 1);
    out = (IUnknown *)0xdeadbeef;
    hr = IWeakReference_Resolve(weakref, &IID_IAgileObject, (IInspectable **)&out);
    ok(hr == S_OK, "got hr %#lx\n", hr);
    ok(out == (IUnknown *)0xdeadbeef, "got out %p\n", out);
    count = IWeakReference_Release(weakref);
    ok(count == 0, "got count %lu\n", count);
}

#define check_interface(o, i) check_interface_(__LINE__, (o), (i))
static void check_interface_(int line, void *obj, const GUID *iid)
{
    HRESULT hr;
    void *out;

    hr = IUnknown_QueryInterface((IUnknown *)obj, iid, &out);
    ok_(__FILE__, line)(hr == S_OK, "got hr %#lx\n", hr);
    if (SUCCEEDED(hr)) IUnknown_Release((IUnknown *)out);
}

/* Additional interfaces implemented by Platform::Object. */
DEFINE_GUID(IID_IEquatable,0xde0cbaec,0xe077,0x4e92,0x84,0xbe,0xc6,0xd4,0x8d,0xca,0x30,0x06);
DEFINE_GUID(IID_IPrintable,0xde0cbaeb,0x8065,0x4a45,0x96,0xb1,0xc9,0xd4,0x43,0xf9,0xba,0xb3);

static void test___abi_make_type_id(void)
{
    const struct __abi_type_descriptor desc = {L"foo", 0xdeadbeef}, desc2 = {L"foo", 0xdeadbeef}, desc3 = {NULL, 1};
    void *type_obj, *type_obj2, *type_info, *type_info2;
    const char *name, *raw_name;
    IClosable *closable;
    int typecode, ret;
    const WCHAR *buf;
    HSTRING str;
    ULONG count;
    bool equals;
    HRESULT hr;

    type_obj = p___abi_make_type_id(&desc);
    ok(type_obj != NULL, "got type_obj %p\n", type_obj);
    if (!type_obj)
    {
        skip("__abi_make_type_id failed\n");
        return;
    }

    check_interface(type_obj, &IID_IInspectable);
    check_interface(type_obj, &IID_IClosable);
    check_interface(type_obj, &IID_IMarshal);
    check_interface(type_obj, &IID_IAgileObject);
    todo_wine check_interface(type_obj, &IID_IEquatable);
    todo_wine check_interface(type_obj, &IID_IPrintable);

    hr = IInspectable_GetRuntimeClassName(type_obj, &str);
    ok(hr == S_OK, "got hr %#lx\n", hr);
    buf = WindowsGetStringRawBuffer(str, NULL);
    ok(buf && !wcscmp(buf, L"Platform.Type"), "got buf %s\n", debugstr_w(buf));
    WindowsDeleteString(str);

    equals = p_platform_type_Equals_Object(type_obj, type_obj);
    ok(equals, "got equals %d\n", equals);
    equals = p_platform_type_Equals_Object(type_obj, NULL);
    ok(!equals, "got equals %d\n", equals);

    typecode = p_platform_type_GetTypeCode(type_obj);
    ok(typecode == 0xdeadbeef, "got typecode %d\n", typecode);

    str = p_platform_type_ToString(type_obj);
    buf = WindowsGetStringRawBuffer(str, NULL);
    ok(buf && !wcscmp(buf, L"foo"), "got buf %s\n", debugstr_w(buf));
    WindowsDeleteString(str);

    str = p_platform_type_get_FullName(type_obj);
    ok(str != NULL, "got str %p\n", str);
    buf = WindowsGetStringRawBuffer(str, NULL);
    ok(buf && !wcscmp(buf, L"foo"), "got buf %s != %s\n", debugstr_w(buf), debugstr_w(L"foo"));
    WindowsDeleteString(str);

    type_obj2 = p___abi_make_type_id(&desc);
    ok(type_obj2 != NULL, "got type_obj %p\n", type_obj);
    ok(type_obj2 != type_obj, "got type_obj2 %p\n", type_obj2);

    type_info = p__RTtypeid(type_obj);
    ok(type_info != NULL, "got type_info %p\n", type_info);
    name = (char *)call_func1(p_type_info_name, type_info);
    ok(!strcmp(name, "class Platform::Type"), "got name %s\n", debugstr_a(name));
    raw_name = (char *)call_func1(p_type_info_raw_name, type_info);
    ok(!strcmp(raw_name, ".?AVType@Platform@@"), "got raw_name %s\n", debugstr_a(raw_name));

    hr = IInspectable_QueryInterface(type_obj, &IID_IClosable, (void **)&closable);
    ok(hr == S_OK, "got hr %#lx\n", hr);
    type_info2 = p__RTtypeid(closable);
    ok(type_info2 != NULL, "got type_info %p\n", type_info2);
    name = (char *)call_func1(p_type_info_name, type_info2);
    ok(!strcmp(name, "class Platform::Type"), "got name %s\n", debugstr_a(name));

    raw_name = (char *)call_func1(p_type_info_raw_name, type_info2);
    ok(!strcmp(raw_name, ".?AVType@Platform@@"), "got raw_name %s\n", debugstr_a(raw_name));

    ret = call_func2(p_type_info_opequals_equals, type_info, type_info2);
    ok(ret, "got ret %d\n", ret);
    ret = call_func2(p_type_info_opnot_equals, type_info, type_info2);
    ok(!ret, "got ret %d\n", ret);

    IClosable_Release(closable);

    check_interface(type_obj2, &IID_IInspectable);
    check_interface(type_obj2, &IID_IClosable);
    check_interface(type_obj2, &IID_IMarshal);
    check_interface(type_obj2, &IID_IAgileObject);
    todo_wine check_interface(type_obj2, &IID_IEquatable);
    todo_wine check_interface(type_obj2, &IID_IPrintable);

    equals = p_platform_type_Equals_Object(type_obj2, type_obj);
    ok(equals, "got equals %d\n", equals);

    count = IInspectable_Release(type_obj);
    ok(count == 0, "got count %lu\n", count);

    equals = p_platform_type_Equals_Object(NULL, NULL);
    ok(equals, "got equals %d\n", equals);

    type_obj = p___abi_make_type_id(&desc2);
    ok(type_obj != NULL, "got type_obj %p\n", type_obj);

    /* Platform::Type::Equals only seems to compare the value of the __abi_type_descriptor pointer. */
    equals = p_platform_type_Equals_Object(type_obj, type_obj2);
    ok(!equals, "got equals %d\n", equals);

    count = IInspectable_Release(type_obj);
    ok(count == 0, "got count %lu\n", count);

    type_obj = p___abi_make_type_id(&desc3);
    ok(type_obj != NULL, "got type_obj %p\n", type_obj);

    equals = p_platform_type_Equals_Object(type_obj, type_obj2);
    ok(!equals, "got equals %d\n", equals);

    typecode = p_platform_type_GetTypeCode(type_obj);
    ok(typecode == 1, "got typecode %d\n", typecode);

    str = p_platform_type_ToString(type_obj);
    ok(str == NULL, "got str %s\n", debugstr_hstring(str));

    str = p_platform_type_get_FullName(type_obj);
    ok(str == NULL, "got str %s\n", debugstr_hstring(str));

    count = IInspectable_Release(type_obj);
    ok(count == 0, "got count %lu\n", count);

    count = IInspectable_Release(type_obj2);
    ok(count == 0, "got count %lu\n", count);
}

enum typecode
{
    TYPECODE_BOOLEAN = 3,
    TYPECODE_CHAR16 = 4,
    TYPECODE_UINT8 = 6,
    TYPECODE_INT16 = 7,
    TYPECODE_UINT16 = 8,
    TYPECODE_INT32 = 9,
    TYPECODE_UINT32 = 10,
    TYPECODE_INT64 = 11,
    TYPECODE_UINT64 = 12,
    TYPECODE_SINGLE = 13,
    TYPECODE_DOUBLE = 14,
    TYPECODE_DATETIME = 16,
    TYPECODE_STRING = 18,
    TYPECODE_TIMESPAN = 19,
    TYPECODE_POINT = 20,
    TYPECODE_SIZE = 21,
    TYPECODE_RECT = 22,
    TYPECODE_GUID = 23,
};

static void test_CreateValue(void)
{
    union value {
        UINT8 uint8;
        UINT16 uint16;
        UINT32 uint32;
        UINT64 uint64;
        FLOAT single;
        DOUBLE dbl;
        boolean boolean;
        GUID guid;
        DateTime time;
        TimeSpan span;
        Point point;
        Size size;
        Rect rect;
    };
    static const struct {
        int typecode;
        union value value;
        PropertyType exp_winrt_type;
        SIZE_T size;
    } test_cases[] = {
        {TYPECODE_BOOLEAN, {.boolean = true}, PropertyType_Boolean, sizeof(boolean)},
        {TYPECODE_CHAR16, {.uint16 = 0xbeef}, PropertyType_Char16, sizeof(UINT16)},
        {TYPECODE_UINT8, {.uint8 = 0xbe}, PropertyType_UInt8, sizeof(UINT8)},
        {TYPECODE_INT16, {.uint16 = 0xbeef}, PropertyType_Int16, sizeof(INT16)},
        {TYPECODE_UINT16, {.uint16 = 0xbeef}, PropertyType_UInt16, sizeof(UINT16)},
        {TYPECODE_INT32, {.uint32 = 0xdeadbeef}, PropertyType_Int32, sizeof(INT32)},
        {TYPECODE_UINT32, {.uint32 = 0xdeadbeef}, PropertyType_UInt32, sizeof(UINT32)},
        {TYPECODE_INT64, {.uint64 = 0xdeadbeefdeadbeef}, PropertyType_Int64, sizeof(INT64)},
        {TYPECODE_UINT64, {.uint64 = 0xdeadbeefdeadbeef}, PropertyType_UInt64, sizeof(UINT64)},
        {TYPECODE_SINGLE, {.single = 2.71828}, PropertyType_Single, sizeof(FLOAT)},
        {TYPECODE_DOUBLE, {.dbl = 2.7182818284}, PropertyType_Double, sizeof(DOUBLE)},
        {TYPECODE_DATETIME, {.time = {0xdeadbeefdeadbeef}}, PropertyType_DateTime, sizeof(DateTime)},
        {TYPECODE_TIMESPAN, {.span = {0xdeadbeefdeadbeef}}, PropertyType_TimeSpan, sizeof(TimeSpan)},
        {TYPECODE_POINT, {.point = {2.71828, 3.14159}}, PropertyType_Point, sizeof(Point)},
        {TYPECODE_SIZE, {.size = {2.71828, 3.14159}}, PropertyType_Size, sizeof(Size)},
        {TYPECODE_RECT, {.rect = {2.71828, 3.14159, 23.140692, 0.20787}}, PropertyType_Rect, sizeof(Rect)},
        {TYPECODE_GUID, {.guid = IID_IInspectable}, PropertyType_Guid, sizeof(GUID)},
    };
    static const int null_typecodes[] = {0, 1, 2, 5,15, 17};
    PropertyType type;
    const WCHAR *buf;
    ULONG count, i;
    HSTRING str;
    HRESULT hr;
    void *obj;

    hr = pInitializeData(1);
    ok(hr == S_OK, "got hr %#lx.\n", hr);

    for (i = 0; i < ARRAY_SIZE(test_cases); i++)
    {
        union value value = {0};
        type = PropertyType_Empty;

        winetest_push_context("test_cases[%lu]", i);

        obj = pCreateValue(test_cases[i].typecode, &test_cases[i].value);
        ok(obj != NULL, "got value %p\n", obj);

        check_interface(obj, &IID_IInspectable);
        check_interface(obj, &IID_IPropertyValue);

        hr = IPropertyValue_get_Type(obj, &type);
        ok(hr == S_OK, "got hr %#lx\n", hr);
        ok(type == test_cases[i].exp_winrt_type, "got type %d != %d\n", type, test_cases[i].exp_winrt_type);

        switch (test_cases[i].exp_winrt_type)
        {
        case PropertyType_Boolean:
            hr = IPropertyValue_GetBoolean(obj, &value.boolean);
            break;
        case PropertyType_Char16:
            hr = IPropertyValue_GetChar16(obj, &value.uint16);
            break;
        case PropertyType_UInt8:
            hr = IPropertyValue_GetUInt8(obj, &value.uint8);
            break;
        case PropertyType_Int16:
            hr = IPropertyValue_GetInt16(obj, (INT16 *)&value.uint16);
            break;
        case PropertyType_UInt16:
            hr = IPropertyValue_GetUInt16(obj, &value.uint16);
            break;
        case PropertyType_Int32:
            hr = IPropertyValue_GetInt32(obj, (INT32 *)&value.uint32);
            break;
        case PropertyType_UInt32:
            hr = IPropertyValue_GetUInt32(obj, &value.uint32);
            break;
        case PropertyType_Int64:
            hr = IPropertyValue_GetInt64(obj, (INT64 *)&value.uint64);
            break;
        case PropertyType_UInt64:
            hr = IPropertyValue_GetUInt64(obj, &value.uint64);
            break;
        case PropertyType_Single:
            hr = IPropertyValue_GetSingle(obj, &value.single);
            break;
        case PropertyType_Double:
            hr = IPropertyValue_GetDouble(obj, &value.dbl);
            break;
        case PropertyType_DateTime:
            hr = IPropertyValue_GetDateTime(obj, &value.time);
            break;
        case PropertyType_TimeSpan:
            hr = IPropertyValue_GetTimeSpan(obj, &value.span);
            break;
        case PropertyType_Point:
            hr = IPropertyValue_GetPoint(obj, &value.point);
            break;
        case PropertyType_Size:
            hr = IPropertyValue_GetSize(obj, &value.size);
            break;
        case PropertyType_Rect:
            hr = IPropertyValue_GetRect(obj, &value.rect);
            break;
        case PropertyType_Guid:
            hr = IPropertyValue_GetGuid(obj, &value.guid);
            break;
        DEFAULT_UNREACHABLE;
        }

        ok(hr == S_OK, "got hr %#lx\n", hr);
        ok(!memcmp(&test_cases[i].value, &value, test_cases[i].size), "got unexpected value\n");
        count = IPropertyValue_Release(obj);
        ok(count == 0, "got count %lu\n", count);

        winetest_pop_context();
    }

    hr = WindowsCreateString(L"foo", 3, &str);
    ok(hr == S_OK, "got hr %#lx\n", hr);
    obj = pCreateValue(TYPECODE_STRING, &str);
    WindowsDeleteString(str);
    ok(obj != NULL || broken(obj == NULL), "got obj %p\n", obj); /* Returns NULL on i386 Windows 10. */
    if (obj)
    {
        type = PropertyType_Empty;
        hr = IPropertyValue_get_Type(obj, &type);
        ok(hr == S_OK, "got hr %#lx\n", hr);
        ok(type == PropertyType_String, "got type %d\n", type);
        str = NULL;
        hr = IPropertyValue_GetString(obj, &str);
        ok(hr == S_OK, "got hr %#lx\n", hr);
        ok(str != NULL, "got str %p\n", str);
        buf = WindowsGetStringRawBuffer(str, NULL);
        ok(buf && !wcscmp(buf, L"foo"), "got buf %s\n", debugstr_w(buf));
        WindowsDeleteString(str);
        count = IPropertyValue_Release(obj);
        ok(count == 0, "got count %lu\n", count);
    }

    for (i = 0; i < ARRAY_SIZE(null_typecodes); i++)
    {
        obj = pCreateValue(null_typecodes[i], &i);
        ok(obj == NULL, "got obj %p\n", obj);
    }

    pUninitializeData(0);
}

START_TEST(vccorlib)
{
    if(!init())
        return;

    test_InitializeData();
    test_GetActivationFactoryByPCWSTR();
    test_GetIidsFn();
    test_Allocate();
    test_AllocateWithWeakRef_inline();
    test_AllocateWithWeakRef();
    test___abi_make_type_id();
    test_CreateValue();
}
