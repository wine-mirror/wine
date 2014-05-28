/*
 * Loader functions
 *
 * Copyright 1995, 2003 Alexandre Julliard
 * Copyright 2002 Dmitry Timoshkov for CodeWeavers
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
#include "wine/port.h"

#include <assert.h>
#include <stdarg.h>
#ifdef HAVE_SYS_MMAN_H
# include <sys/mman.h>
#endif

#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winnt.h"
#include "winternl.h"
#include "delayloadhandler.h"

#include "wine/exception.h"
#include "wine/library.h"
#include "wine/unicode.h"
#include "wine/debug.h"
#include "wine/server.h"
#include "ntdll_misc.h"
#include "ddk/wdm.h"

WINE_DEFAULT_DEBUG_CHANNEL(module);
WINE_DECLARE_DEBUG_CHANNEL(relay);
WINE_DECLARE_DEBUG_CHANNEL(snoop);
WINE_DECLARE_DEBUG_CHANNEL(loaddll);
WINE_DECLARE_DEBUG_CHANNEL(imports);

/* we don't want to include winuser.h */
#define RT_MANIFEST                         ((ULONG_PTR)24)
#define ISOLATIONAWARE_MANIFEST_RESOURCE_ID ((ULONG_PTR)2)

typedef DWORD (CALLBACK *DLLENTRYPROC)(HMODULE,DWORD,LPVOID);

static BOOL process_detaching = FALSE;  /* set on process detach to avoid deadlocks with thread detach */
static int free_lib_count;   /* recursion depth of LdrUnloadDll calls */

static const char * const reason_names[] =
{
    "PROCESS_DETACH",
    "PROCESS_ATTACH",
    "THREAD_ATTACH",
    "THREAD_DETACH",
    NULL, NULL, NULL, NULL,
    "WINE_PREATTACH"
};

static const WCHAR dllW[] = {'.','d','l','l',0};

/* internal representation of 32bit modules. per process. */
typedef struct _wine_modref
{
    LDR_MODULE            ldr;
    int                   nDeps;
    struct _wine_modref **deps;
} WINE_MODREF;

/* info about the current builtin dll load */
/* used to keep track of things across the register_dll constructor call */
struct builtin_load_info
{
    const WCHAR *load_path;
    const WCHAR *filename;
    NTSTATUS     status;
    WINE_MODREF *wm;
};

static struct builtin_load_info default_load_info;
static struct builtin_load_info *builtin_load_info = &default_load_info;

static HANDLE main_exe_file;
static UINT tls_module_count;      /* number of modules with TLS directory */
static const IMAGE_TLS_DIRECTORY **tls_dirs;  /* array of TLS directories */
LIST_ENTRY tls_links = { &tls_links, &tls_links };

static RTL_CRITICAL_SECTION loader_section;
static RTL_CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &loader_section,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": loader_section") }
};
static RTL_CRITICAL_SECTION loader_section = { &critsect_debug, -1, 0, 0, 0, 0 };

static WINE_MODREF *cached_modref;
static WINE_MODREF *current_modref;
static WINE_MODREF *last_failed_modref;

static NTSTATUS load_dll( LPCWSTR load_path, LPCWSTR libname, DWORD flags, WINE_MODREF** pwm );
static NTSTATUS process_attach( WINE_MODREF *wm, LPVOID lpReserved );
static FARPROC find_ordinal_export( HMODULE module, const IMAGE_EXPORT_DIRECTORY *exports,
                                    DWORD exp_size, DWORD ordinal, LPCWSTR load_path );
static FARPROC find_named_export( HMODULE module, const IMAGE_EXPORT_DIRECTORY *exports,
                                  DWORD exp_size, const char *name, int hint, LPCWSTR load_path );

/* convert PE image VirtualAddress to Real Address */
static inline void *get_rva( HMODULE module, DWORD va )
{
    return (void *)((char *)module + va);
}

/* check whether the file name contains a path */
static inline BOOL contains_path( LPCWSTR name )
{
    return ((*name && (name[1] == ':')) || strchrW(name, '/') || strchrW(name, '\\'));
}

/* convert from straight ASCII to Unicode without depending on the current codepage */
static inline void ascii_to_unicode( WCHAR *dst, const char *src, size_t len )
{
    while (len--) *dst++ = (unsigned char)*src++;
}


/*************************************************************************
 *		call_dll_entry_point
 *
 * Some brain-damaged dlls (ir32_32.dll for instance) modify ebx in
 * their entry point, so we need a small asm wrapper.
 */
#ifdef __i386__
extern BOOL call_dll_entry_point( DLLENTRYPROC proc, void *module, UINT reason, void *reserved );
__ASM_GLOBAL_FUNC(call_dll_entry_point,
                  "pushl %ebp\n\t"
                  __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")
                  __ASM_CFI(".cfi_rel_offset %ebp,0\n\t")
                  "movl %esp,%ebp\n\t"
                  __ASM_CFI(".cfi_def_cfa_register %ebp\n\t")
                  "pushl %ebx\n\t"
                  __ASM_CFI(".cfi_rel_offset %ebx,-4\n\t")
                  "subl $8,%esp\n\t"
                  "pushl 20(%ebp)\n\t"
                  "pushl 16(%ebp)\n\t"
                  "pushl 12(%ebp)\n\t"
                  "movl 8(%ebp),%eax\n\t"
                  "call *%eax\n\t"
                  "leal -4(%ebp),%esp\n\t"
                  "popl %ebx\n\t"
                  __ASM_CFI(".cfi_same_value %ebx\n\t")
                  "popl %ebp\n\t"
                  __ASM_CFI(".cfi_def_cfa %esp,4\n\t")
                  __ASM_CFI(".cfi_same_value %ebp\n\t")
                  "ret" )
#else /* __i386__ */
static inline BOOL call_dll_entry_point( DLLENTRYPROC proc, void *module,
                                         UINT reason, void *reserved )
{
    return proc( module, reason, reserved );
}
#endif /* __i386__ */


#if defined(__i386__) || defined(__x86_64__) || defined(__arm__)
/*************************************************************************
 *		stub_entry_point
 *
 * Entry point for stub functions.
 */
static void stub_entry_point( const char *dll, const char *name, void *ret_addr )
{
    EXCEPTION_RECORD rec;

    rec.ExceptionCode           = EXCEPTION_WINE_STUB;
    rec.ExceptionFlags          = EH_NONCONTINUABLE;
    rec.ExceptionRecord         = NULL;
    rec.ExceptionAddress        = ret_addr;
    rec.NumberParameters        = 2;
    rec.ExceptionInformation[0] = (ULONG_PTR)dll;
    rec.ExceptionInformation[1] = (ULONG_PTR)name;
    for (;;) RtlRaiseException( &rec );
}


#include "pshpack1.h"
#ifdef __i386__
struct stub
{
    BYTE        pushl1;     /* pushl $name */
    const char *name;
    BYTE        pushl2;     /* pushl $dll */
    const char *dll;
    BYTE        call;       /* call stub_entry_point */
    DWORD       entry;
};
#elif defined(__arm__)
struct stub
{
    BYTE ldr_r0[4];        /* ldr r0, $dll */
    BYTE mov_pc_pc1[4];    /* mov pc,pc */
    const char *dll;
    BYTE ldr_r1[4];        /* ldr r1, $name */
    BYTE mov_pc_pc2[4];    /* mov pc,pc */
    const char *name;
    BYTE mov_r2_lr[4];     /* mov r2, lr */
    BYTE ldr_pc_pc[4];     /* ldr pc, [pc, #-4] */
    const void* entry;
};
#else
struct stub
{
    BYTE movq_rdi[2];      /* movq $dll,%rdi */
    const char *dll;
    BYTE movq_rsi[2];      /* movq $name,%rsi */
    const char *name;
    BYTE movq_rsp_rdx[4];  /* movq (%rsp),%rdx */
    BYTE movq_rax[2];      /* movq $entry, %rax */
    const void* entry;
    BYTE jmpq_rax[2];      /* jmp %rax */
};
#endif
#include "poppack.h"

/*************************************************************************
 *		allocate_stub
 *
 * Allocate a stub entry point.
 */
static ULONG_PTR allocate_stub( const char *dll, const char *name )
{
#define MAX_SIZE 65536
    static struct stub *stubs;
    static unsigned int nb_stubs;
    struct stub *stub;

    if (nb_stubs >= MAX_SIZE / sizeof(*stub)) return 0xdeadbeef;

    if (!stubs)
    {
        SIZE_T size = MAX_SIZE;
        if (NtAllocateVirtualMemory( NtCurrentProcess(), (void **)&stubs, 0, &size,
                                     MEM_COMMIT, PAGE_EXECUTE_READWRITE ) != STATUS_SUCCESS)
            return 0xdeadbeef;
    }
    stub = &stubs[nb_stubs++];
#ifdef __i386__
    stub->pushl1    = 0x68;  /* pushl $name */
    stub->name      = name;
    stub->pushl2    = 0x68;  /* pushl $dll */
    stub->dll       = dll;
    stub->call      = 0xe8;  /* call stub_entry_point */
    stub->entry     = (BYTE *)stub_entry_point - (BYTE *)(&stub->entry + 1);
#elif defined(__arm__)
    stub->ldr_r0[0]     = 0x00;   /* ldr r0, $dll */
    stub->ldr_r0[1]     = 0x00;
    stub->ldr_r0[2]     = 0x9f;
    stub->ldr_r0[3]     = 0xe5;
    stub->mov_pc_pc1[0] = 0x0f;   /* mov pc,pc */
    stub->mov_pc_pc1[1] = 0xf0;
    stub->mov_pc_pc1[2] = 0xa0;
    stub->mov_pc_pc1[3] = 0xe1;
    stub->dll           = dll;
    stub->ldr_r1[0]     = 0x00;   /* ldr r1, $name */
    stub->ldr_r1[1]     = 0x10;
    stub->ldr_r1[2]     = 0x9f;
    stub->ldr_r1[3]     = 0xe5;
    stub->mov_pc_pc2[0] = 0x0f;   /* mov pc,pc */
    stub->mov_pc_pc2[1] = 0xf0;
    stub->mov_pc_pc2[2] = 0xa0;
    stub->mov_pc_pc2[3] = 0xe1;
    stub->name          = name;
    stub->mov_r2_lr[0]  = 0x0e;   /* mov r2, lr */
    stub->mov_r2_lr[1]  = 0x20;
    stub->mov_r2_lr[2]  = 0xa0;
    stub->mov_r2_lr[3]  = 0xe1;
    stub->ldr_pc_pc[0]  = 0x04;   /* ldr pc, [pc, #-4] */
    stub->ldr_pc_pc[1]  = 0xf0;
    stub->ldr_pc_pc[2]  = 0x1f;
    stub->ldr_pc_pc[3]  = 0xe5;
    stub->entry         = stub_entry_point;
#else
    stub->movq_rdi[0]     = 0x48;  /* movq $dll,%rdi */
    stub->movq_rdi[1]     = 0xbf;
    stub->dll             = dll;
    stub->movq_rsi[0]     = 0x48;  /* movq $name,%rsi */
    stub->movq_rsi[1]     = 0xbe;
    stub->name            = name;
    stub->movq_rsp_rdx[0] = 0x48;  /* movq (%rsp),%rdx */
    stub->movq_rsp_rdx[1] = 0x8b;
    stub->movq_rsp_rdx[2] = 0x14;
    stub->movq_rsp_rdx[3] = 0x24;
    stub->movq_rax[0]     = 0x48;  /* movq $entry, %rax */
    stub->movq_rax[1]     = 0xb8;
    stub->entry           = stub_entry_point;
    stub->jmpq_rax[0]     = 0xff;  /* jmp %rax */
    stub->jmpq_rax[1]     = 0xe0;
#endif
    return (ULONG_PTR)stub;
}

#else  /* __i386__ */
static inline ULONG_PTR allocate_stub( const char *dll, const char *name ) { return 0xdeadbeef; }
#endif  /* __i386__ */


/*************************************************************************
 *		get_modref
 *
 * Looks for the referenced HMODULE in the current process
 * The loader_section must be locked while calling this function.
 */
static WINE_MODREF *get_modref( HMODULE hmod )
{
    PLIST_ENTRY mark, entry;
    PLDR_MODULE mod;

    if (cached_modref && cached_modref->ldr.BaseAddress == hmod) return cached_modref;

    mark = &NtCurrentTeb()->Peb->LdrData->InMemoryOrderModuleList;
    for (entry = mark->Flink; entry != mark; entry = entry->Flink)
    {
        mod = CONTAINING_RECORD(entry, LDR_MODULE, InMemoryOrderModuleList);
        if (mod->BaseAddress == hmod)
            return cached_modref = CONTAINING_RECORD(mod, WINE_MODREF, ldr);
        if (mod->BaseAddress > (void*)hmod) break;
    }
    return NULL;
}


/**********************************************************************
 *	    find_basename_module
 *
 * Find a module from its base name.
 * The loader_section must be locked while calling this function
 */
static WINE_MODREF *find_basename_module( LPCWSTR name )
{
    PLIST_ENTRY mark, entry;

    if (cached_modref && !strcmpiW( name, cached_modref->ldr.BaseDllName.Buffer ))
        return cached_modref;

    mark = &NtCurrentTeb()->Peb->LdrData->InLoadOrderModuleList;
    for (entry = mark->Flink; entry != mark; entry = entry->Flink)
    {
        LDR_MODULE *mod = CONTAINING_RECORD(entry, LDR_MODULE, InLoadOrderModuleList);
        if (!strcmpiW( name, mod->BaseDllName.Buffer ))
        {
            cached_modref = CONTAINING_RECORD(mod, WINE_MODREF, ldr);
            return cached_modref;
        }
    }
    return NULL;
}


/**********************************************************************
 *	    find_fullname_module
 *
 * Find a module from its full path name.
 * The loader_section must be locked while calling this function
 */
static WINE_MODREF *find_fullname_module( LPCWSTR name )
{
    PLIST_ENTRY mark, entry;

    if (cached_modref && !strcmpiW( name, cached_modref->ldr.FullDllName.Buffer ))
        return cached_modref;

    mark = &NtCurrentTeb()->Peb->LdrData->InLoadOrderModuleList;
    for (entry = mark->Flink; entry != mark; entry = entry->Flink)
    {
        LDR_MODULE *mod = CONTAINING_RECORD(entry, LDR_MODULE, InLoadOrderModuleList);
        if (!strcmpiW( name, mod->FullDllName.Buffer ))
        {
            cached_modref = CONTAINING_RECORD(mod, WINE_MODREF, ldr);
            return cached_modref;
        }
    }
    return NULL;
}


/*************************************************************************
 *		find_forwarded_export
 *
 * Find the final function pointer for a forwarded function.
 * The loader_section must be locked while calling this function.
 */
static FARPROC find_forwarded_export( HMODULE module, const char *forward, LPCWSTR load_path )
{
    const IMAGE_EXPORT_DIRECTORY *exports;
    DWORD exp_size;
    WINE_MODREF *wm;
    WCHAR mod_name[32];
    const char *end = strrchr(forward, '.');
    FARPROC proc = NULL;

    if (!end) return NULL;
    if ((end - forward) * sizeof(WCHAR) >= sizeof(mod_name)) return NULL;
    ascii_to_unicode( mod_name, forward, end - forward );
    mod_name[end - forward] = 0;
    if (!strchrW( mod_name, '.' ))
    {
        if ((end - forward) * sizeof(WCHAR) >= sizeof(mod_name) - sizeof(dllW)) return NULL;
        memcpy( mod_name + (end - forward), dllW, sizeof(dllW) );
    }

    if (!(wm = find_basename_module( mod_name )))
    {
        TRACE( "delay loading %s for '%s'\n", debugstr_w(mod_name), forward );
        if (load_dll( load_path, mod_name, 0, &wm ) == STATUS_SUCCESS &&
            !(wm->ldr.Flags & LDR_DONT_RESOLVE_REFS))
        {
            if (process_attach( wm, NULL ) != STATUS_SUCCESS)
            {
                LdrUnloadDll( wm->ldr.BaseAddress );
                wm = NULL;
            }
        }

        if (!wm)
        {
            ERR( "module not found for forward '%s' used by %s\n",
                 forward, debugstr_w(get_modref(module)->ldr.FullDllName.Buffer) );
            return NULL;
        }
    }
    if ((exports = RtlImageDirectoryEntryToData( wm->ldr.BaseAddress, TRUE,
                                                 IMAGE_DIRECTORY_ENTRY_EXPORT, &exp_size )))
    {
        const char *name = end + 1;
        if (*name == '#')  /* ordinal */
            proc = find_ordinal_export( wm->ldr.BaseAddress, exports, exp_size, atoi(name+1), load_path );
        else
            proc = find_named_export( wm->ldr.BaseAddress, exports, exp_size, name, -1, load_path );
    }

    if (!proc)
    {
        ERR("function not found for forward '%s' used by %s."
            " If you are using builtin %s, try using the native one instead.\n",
            forward, debugstr_w(get_modref(module)->ldr.FullDllName.Buffer),
            debugstr_w(get_modref(module)->ldr.BaseDllName.Buffer) );
    }
    return proc;
}


/*************************************************************************
 *		find_ordinal_export
 *
 * Find an exported function by ordinal.
 * The exports base must have been subtracted from the ordinal already.
 * The loader_section must be locked while calling this function.
 */
static FARPROC find_ordinal_export( HMODULE module, const IMAGE_EXPORT_DIRECTORY *exports,
                                    DWORD exp_size, DWORD ordinal, LPCWSTR load_path )
{
    FARPROC proc;
    const DWORD *functions = get_rva( module, exports->AddressOfFunctions );

    if (ordinal >= exports->NumberOfFunctions)
    {
        TRACE("	ordinal %d out of range!\n", ordinal + exports->Base );
        return NULL;
    }
    if (!functions[ordinal]) return NULL;

    proc = get_rva( module, functions[ordinal] );

    /* if the address falls into the export dir, it's a forward */
    if (((const char *)proc >= (const char *)exports) && 
        ((const char *)proc < (const char *)exports + exp_size))
        return find_forwarded_export( module, (const char *)proc, load_path );

    if (TRACE_ON(snoop))
    {
        const WCHAR *user = current_modref ? current_modref->ldr.BaseDllName.Buffer : NULL;
        proc = SNOOP_GetProcAddress( module, exports, exp_size, proc, ordinal, user );
    }
    if (TRACE_ON(relay))
    {
        const WCHAR *user = current_modref ? current_modref->ldr.BaseDllName.Buffer : NULL;
        proc = RELAY_GetProcAddress( module, exports, exp_size, proc, ordinal, user );
    }
    return proc;
}


/*************************************************************************
 *		find_named_export
 *
 * Find an exported function by name.
 * The loader_section must be locked while calling this function.
 */
static FARPROC find_named_export( HMODULE module, const IMAGE_EXPORT_DIRECTORY *exports,
                                  DWORD exp_size, const char *name, int hint, LPCWSTR load_path )
{
    const WORD *ordinals = get_rva( module, exports->AddressOfNameOrdinals );
    const DWORD *names = get_rva( module, exports->AddressOfNames );
    int min = 0, max = exports->NumberOfNames - 1;

    /* first check the hint */
    if (hint >= 0 && hint <= max)
    {
        char *ename = get_rva( module, names[hint] );
        if (!strcmp( ename, name ))
            return find_ordinal_export( module, exports, exp_size, ordinals[hint], load_path );
    }

    /* then do a binary search */
    while (min <= max)
    {
        int res, pos = (min + max) / 2;
        char *ename = get_rva( module, names[pos] );
        if (!(res = strcmp( ename, name )))
            return find_ordinal_export( module, exports, exp_size, ordinals[pos], load_path );
        if (res > 0) max = pos - 1;
        else min = pos + 1;
    }
    return NULL;

}


/*************************************************************************
 *		import_dll
 *
 * Import the dll specified by the given import descriptor.
 * The loader_section must be locked while calling this function.
 */
static WINE_MODREF *import_dll( HMODULE module, const IMAGE_IMPORT_DESCRIPTOR *descr, LPCWSTR load_path )
{
    NTSTATUS status;
    WINE_MODREF *wmImp;
    HMODULE imp_mod;
    const IMAGE_EXPORT_DIRECTORY *exports;
    DWORD exp_size;
    const IMAGE_THUNK_DATA *import_list;
    IMAGE_THUNK_DATA *thunk_list;
    WCHAR buffer[32];
    const char *name = get_rva( module, descr->Name );
    DWORD len = strlen(name);
    PVOID protect_base;
    SIZE_T protect_size = 0;
    DWORD protect_old;

    thunk_list = get_rva( module, (DWORD)descr->FirstThunk );
    if (descr->u.OriginalFirstThunk)
        import_list = get_rva( module, (DWORD)descr->u.OriginalFirstThunk );
    else
        import_list = thunk_list;

    while (len && name[len-1] == ' ') len--;  /* remove trailing spaces */

    if (len * sizeof(WCHAR) < sizeof(buffer))
    {
        ascii_to_unicode( buffer, name, len );
        buffer[len] = 0;
        status = load_dll( load_path, buffer, 0, &wmImp );
    }
    else  /* need to allocate a larger buffer */
    {
        WCHAR *ptr = RtlAllocateHeap( GetProcessHeap(), 0, (len + 1) * sizeof(WCHAR) );
        if (!ptr) return NULL;
        ascii_to_unicode( ptr, name, len );
        ptr[len] = 0;
        status = load_dll( load_path, ptr, 0, &wmImp );
        RtlFreeHeap( GetProcessHeap(), 0, ptr );
    }

    if (status)
    {
        if (status == STATUS_DLL_NOT_FOUND)
            ERR("Library %s (which is needed by %s) not found\n",
                name, debugstr_w(current_modref->ldr.FullDllName.Buffer));
        else
            ERR("Loading library %s (which is needed by %s) failed (error %x).\n",
                name, debugstr_w(current_modref->ldr.FullDllName.Buffer), status);
        return NULL;
    }

    /* unprotect the import address table since it can be located in
     * readonly section */
    while (import_list[protect_size].u1.Ordinal) protect_size++;
    protect_base = thunk_list;
    protect_size *= sizeof(*thunk_list);
    NtProtectVirtualMemory( NtCurrentProcess(), &protect_base,
                            &protect_size, PAGE_READWRITE, &protect_old );

    imp_mod = wmImp->ldr.BaseAddress;
    exports = RtlImageDirectoryEntryToData( imp_mod, TRUE, IMAGE_DIRECTORY_ENTRY_EXPORT, &exp_size );

    if (!exports)
    {
        /* set all imported function to deadbeef */
        while (import_list->u1.Ordinal)
        {
            if (IMAGE_SNAP_BY_ORDINAL(import_list->u1.Ordinal))
            {
                int ordinal = IMAGE_ORDINAL(import_list->u1.Ordinal);
                WARN("No implementation for %s.%d", name, ordinal );
                thunk_list->u1.Function = allocate_stub( name, IntToPtr(ordinal) );
            }
            else
            {
                IMAGE_IMPORT_BY_NAME *pe_name = get_rva( module, (DWORD)import_list->u1.AddressOfData );
                WARN("No implementation for %s.%s", name, pe_name->Name );
                thunk_list->u1.Function = allocate_stub( name, (const char*)pe_name->Name );
            }
            WARN(" imported from %s, allocating stub %p\n",
                 debugstr_w(current_modref->ldr.FullDllName.Buffer),
                 (void *)thunk_list->u1.Function );
            import_list++;
            thunk_list++;
        }
        goto done;
    }

    while (import_list->u1.Ordinal)
    {
        if (IMAGE_SNAP_BY_ORDINAL(import_list->u1.Ordinal))
        {
            int ordinal = IMAGE_ORDINAL(import_list->u1.Ordinal);

            thunk_list->u1.Function = (ULONG_PTR)find_ordinal_export( imp_mod, exports, exp_size,
                                                                      ordinal - exports->Base, load_path );
            if (!thunk_list->u1.Function)
            {
                thunk_list->u1.Function = allocate_stub( name, IntToPtr(ordinal) );
                WARN("No implementation for %s.%d imported from %s, setting to %p\n",
                     name, ordinal, debugstr_w(current_modref->ldr.FullDllName.Buffer),
                     (void *)thunk_list->u1.Function );
            }
            TRACE_(imports)("--- Ordinal %s.%d = %p\n", name, ordinal, (void *)thunk_list->u1.Function );
        }
        else  /* import by name */
        {
            IMAGE_IMPORT_BY_NAME *pe_name;
            pe_name = get_rva( module, (DWORD)import_list->u1.AddressOfData );
            thunk_list->u1.Function = (ULONG_PTR)find_named_export( imp_mod, exports, exp_size,
                                                                    (const char*)pe_name->Name,
                                                                    pe_name->Hint, load_path );
            if (!thunk_list->u1.Function)
            {
                thunk_list->u1.Function = allocate_stub( name, (const char*)pe_name->Name );
                WARN("No implementation for %s.%s imported from %s, setting to %p\n",
                     name, pe_name->Name, debugstr_w(current_modref->ldr.FullDllName.Buffer),
                     (void *)thunk_list->u1.Function );
            }
            TRACE_(imports)("--- %s %s.%d = %p\n",
                            pe_name->Name, name, pe_name->Hint, (void *)thunk_list->u1.Function);
        }
        import_list++;
        thunk_list++;
    }

done:
    /* restore old protection of the import address table */
    NtProtectVirtualMemory( NtCurrentProcess(), &protect_base, &protect_size, protect_old, NULL );
    return wmImp;
}


/***********************************************************************
 *           create_module_activation_context
 */
static NTSTATUS create_module_activation_context( LDR_MODULE *module )
{
    NTSTATUS status;
    LDR_RESOURCE_INFO info;
    const IMAGE_RESOURCE_DATA_ENTRY *entry;

    info.Type = RT_MANIFEST;
    info.Name = ISOLATIONAWARE_MANIFEST_RESOURCE_ID;
    info.Language = 0;
    if (!(status = LdrFindResource_U( module->BaseAddress, &info, 3, &entry )))
    {
        ACTCTXW ctx;
        ctx.cbSize   = sizeof(ctx);
        ctx.lpSource = NULL;
        ctx.dwFlags  = ACTCTX_FLAG_RESOURCE_NAME_VALID | ACTCTX_FLAG_HMODULE_VALID;
        ctx.hModule  = module->BaseAddress;
        ctx.lpResourceName = (LPCWSTR)ISOLATIONAWARE_MANIFEST_RESOURCE_ID;
        status = RtlCreateActivationContext( &module->ActivationContext, &ctx );
    }
    return status;
}


/*************************************************************************
 *		is_dll_native_subsystem
 *
 * Check if dll is a proper native driver.
 * Some dlls (corpol.dll from IE6 for instance) are incorrectly marked as native
 * while being perfectly normal DLLs.  This heuristic should catch such breakages.
 */
static BOOL is_dll_native_subsystem( HMODULE module, const IMAGE_NT_HEADERS *nt, LPCWSTR filename )
{
    static const WCHAR ntdllW[]    = {'n','t','d','l','l','.','d','l','l',0};
    static const WCHAR kernel32W[] = {'k','e','r','n','e','l','3','2','.','d','l','l',0};
    const IMAGE_IMPORT_DESCRIPTOR *imports;
    DWORD i, size;
    WCHAR buffer[16];

    if (nt->OptionalHeader.Subsystem != IMAGE_SUBSYSTEM_NATIVE) return FALSE;
    if (nt->OptionalHeader.SectionAlignment < page_size) return TRUE;

    if ((imports = RtlImageDirectoryEntryToData( module, TRUE,
                                                 IMAGE_DIRECTORY_ENTRY_IMPORT, &size )))
    {
        for (i = 0; imports[i].Name; i++)
        {
            const char *name = get_rva( module, imports[i].Name );
            DWORD len = strlen(name);
            if (len * sizeof(WCHAR) >= sizeof(buffer)) continue;
            ascii_to_unicode( buffer, name, len + 1 );
            if (!strcmpiW( buffer, ntdllW ) || !strcmpiW( buffer, kernel32W ))
            {
                TRACE( "%s imports %s, assuming not native\n", debugstr_w(filename), debugstr_w(buffer) );
                return FALSE;
            }
        }
    }
    return TRUE;
}

/*************************************************************************
 *		alloc_tls_slot
 *
 * Allocate a TLS slot for a newly-loaded module.
 * The loader_section must be locked while calling this function.
 */
static SHORT alloc_tls_slot( LDR_MODULE *mod )
{
    const IMAGE_TLS_DIRECTORY *dir;
    ULONG i, size;
    void *new_ptr;
    LIST_ENTRY *entry;

    if (!(dir = RtlImageDirectoryEntryToData( mod->BaseAddress, TRUE, IMAGE_DIRECTORY_ENTRY_TLS, &size )))
        return -1;

    size = dir->EndAddressOfRawData - dir->StartAddressOfRawData;
    if (!size && !dir->SizeOfZeroFill && !dir->AddressOfCallBacks) return -1;

    for (i = 0; i < tls_module_count; i++) if (!tls_dirs[i]) break;

    TRACE( "module %p data %p-%p zerofill %u index %p callback %p flags %x -> slot %u\n", mod->BaseAddress,
           (void *)dir->StartAddressOfRawData, (void *)dir->EndAddressOfRawData, dir->SizeOfZeroFill,
           (void *)dir->AddressOfIndex, (void *)dir->AddressOfCallBacks, dir->Characteristics, i );

    if (i == tls_module_count)
    {
        UINT new_count = max( 32, tls_module_count * 2 );

        if (!tls_dirs)
            new_ptr = RtlAllocateHeap( GetProcessHeap(), HEAP_ZERO_MEMORY, new_count * sizeof(*tls_dirs) );
        else
            new_ptr = RtlReAllocateHeap( GetProcessHeap(), HEAP_ZERO_MEMORY, tls_dirs,
                                         new_count * sizeof(*tls_dirs) );
        if (!new_ptr) return -1;

        /* resize the pointer block in all running threads */
        for (entry = tls_links.Flink; entry != &tls_links; entry = entry->Flink)
        {
            TEB *teb = CONTAINING_RECORD( entry, TEB, TlsLinks );
            void **old = teb->ThreadLocalStoragePointer;
            void **new = RtlAllocateHeap( GetProcessHeap(), HEAP_ZERO_MEMORY, new_count * sizeof(*new));

            if (!new) return -1;
            if (old) memcpy( new, old, tls_module_count * sizeof(*new) );
            teb->ThreadLocalStoragePointer = new;
            TRACE( "thread %04lx tls block %p -> %p\n", (ULONG_PTR)teb->ClientId.UniqueThread, old, new );
            /* FIXME: can't free old block here, should be freed at thread exit */
        }

        tls_dirs = new_ptr;
        tls_module_count = new_count;
    }

    /* allocate the data block in all running threads */
    for (entry = tls_links.Flink; entry != &tls_links; entry = entry->Flink)
    {
        TEB *teb = CONTAINING_RECORD( entry, TEB, TlsLinks );

        if (!(new_ptr = RtlAllocateHeap( GetProcessHeap(), 0, size + dir->SizeOfZeroFill ))) return -1;
        memcpy( new_ptr, (void *)dir->StartAddressOfRawData, size );
        memset( (char *)new_ptr + size, 0, dir->SizeOfZeroFill );

        TRACE( "thread %04lx slot %u: %u/%u bytes at %p\n",
               (ULONG_PTR)teb->ClientId.UniqueThread, i, size, dir->SizeOfZeroFill, new_ptr );

        RtlFreeHeap( GetProcessHeap(), 0,
                     interlocked_xchg_ptr( (void **)teb->ThreadLocalStoragePointer + i, new_ptr ));
    }

    *(DWORD *)dir->AddressOfIndex = i;
    tls_dirs[i] = dir;
    return i;
}


/*************************************************************************
 *		free_tls_slot
 *
 * Free the module TLS slot on unload.
 * The loader_section must be locked while calling this function.
 */
static void free_tls_slot( LDR_MODULE *mod )
{
    ULONG i = (USHORT)mod->TlsIndex;

    if (mod->TlsIndex == -1) return;
    assert( i < tls_module_count );
    tls_dirs[i] = NULL;
}


/****************************************************************
 *       fixup_imports
 *
 * Fixup all imports of a given module.
 * The loader_section must be locked while calling this function.
 */
static NTSTATUS fixup_imports( WINE_MODREF *wm, LPCWSTR load_path )
{
    int i, nb_imports;
    const IMAGE_IMPORT_DESCRIPTOR *imports;
    WINE_MODREF *prev;
    DWORD size;
    NTSTATUS status;
    ULONG_PTR cookie;

    if (!(wm->ldr.Flags & LDR_DONT_RESOLVE_REFS)) return STATUS_SUCCESS;  /* already done */
    wm->ldr.Flags &= ~LDR_DONT_RESOLVE_REFS;

    wm->ldr.TlsIndex = alloc_tls_slot( &wm->ldr );

    if (!(imports = RtlImageDirectoryEntryToData( wm->ldr.BaseAddress, TRUE,
                                                  IMAGE_DIRECTORY_ENTRY_IMPORT, &size )))
        return STATUS_SUCCESS;

    nb_imports = 0;
    while (imports[nb_imports].Name && imports[nb_imports].FirstThunk) nb_imports++;

    if (!nb_imports) return STATUS_SUCCESS;  /* no imports */

    if (!create_module_activation_context( &wm->ldr ))
        RtlActivateActivationContext( 0, wm->ldr.ActivationContext, &cookie );

    /* Allocate module dependency list */
    wm->nDeps = nb_imports;
    wm->deps  = RtlAllocateHeap( GetProcessHeap(), 0, nb_imports*sizeof(WINE_MODREF *) );

    /* load the imported modules. They are automatically
     * added to the modref list of the process.
     */
    prev = current_modref;
    current_modref = wm;
    status = STATUS_SUCCESS;
    for (i = 0; i < nb_imports; i++)
    {
        if (!(wm->deps[i] = import_dll( wm->ldr.BaseAddress, &imports[i], load_path )))
            status = STATUS_DLL_NOT_FOUND;
    }
    current_modref = prev;
    if (wm->ldr.ActivationContext) RtlDeactivateActivationContext( 0, cookie );
    return status;
}


/*************************************************************************
 *		alloc_module
 *
 * Allocate a WINE_MODREF structure and add it to the process list
 * The loader_section must be locked while calling this function.
 */
static WINE_MODREF *alloc_module( HMODULE hModule, LPCWSTR filename )
{
    WINE_MODREF *wm;
    const WCHAR *p;
    const IMAGE_NT_HEADERS *nt = RtlImageNtHeader(hModule);
    PLIST_ENTRY entry, mark;

    if (!(wm = RtlAllocateHeap( GetProcessHeap(), 0, sizeof(*wm) ))) return NULL;

    wm->nDeps    = 0;
    wm->deps     = NULL;

    wm->ldr.BaseAddress   = hModule;
    wm->ldr.EntryPoint    = NULL;
    wm->ldr.SizeOfImage   = nt->OptionalHeader.SizeOfImage;
    wm->ldr.Flags         = LDR_DONT_RESOLVE_REFS;
    wm->ldr.TlsIndex      = -1;
    wm->ldr.LoadCount     = 1;
    wm->ldr.SectionHandle = NULL;
    wm->ldr.CheckSum      = 0;
    wm->ldr.TimeDateStamp = 0;
    wm->ldr.ActivationContext = 0;

    RtlCreateUnicodeString( &wm->ldr.FullDllName, filename );
    if ((p = strrchrW( wm->ldr.FullDllName.Buffer, '\\' ))) p++;
    else p = wm->ldr.FullDllName.Buffer;
    RtlInitUnicodeString( &wm->ldr.BaseDllName, p );

    if ((nt->FileHeader.Characteristics & IMAGE_FILE_DLL) && !is_dll_native_subsystem( hModule, nt, p ))
    {
        wm->ldr.Flags |= LDR_IMAGE_IS_DLL;
        if (nt->OptionalHeader.AddressOfEntryPoint)
            wm->ldr.EntryPoint = (char *)hModule + nt->OptionalHeader.AddressOfEntryPoint;
    }

    InsertTailList(&NtCurrentTeb()->Peb->LdrData->InLoadOrderModuleList,
                   &wm->ldr.InLoadOrderModuleList);

    /* insert module in MemoryList, sorted in increasing base addresses */
    mark = &NtCurrentTeb()->Peb->LdrData->InMemoryOrderModuleList;
    for (entry = mark->Flink; entry != mark; entry = entry->Flink)
    {
        if (CONTAINING_RECORD(entry, LDR_MODULE, InMemoryOrderModuleList)->BaseAddress > wm->ldr.BaseAddress)
            break;
    }
    entry->Blink->Flink = &wm->ldr.InMemoryOrderModuleList;
    wm->ldr.InMemoryOrderModuleList.Blink = entry->Blink;
    wm->ldr.InMemoryOrderModuleList.Flink = entry;
    entry->Blink = &wm->ldr.InMemoryOrderModuleList;

    /* wait until init is called for inserting into this list */
    wm->ldr.InInitializationOrderModuleList.Flink = NULL;
    wm->ldr.InInitializationOrderModuleList.Blink = NULL;

    if (!(nt->OptionalHeader.DllCharacteristics & IMAGE_DLLCHARACTERISTICS_NX_COMPAT))
    {
        ULONG flags = MEM_EXECUTE_OPTION_ENABLE;
        WARN( "disabling no-exec because of %s\n", debugstr_w(wm->ldr.BaseDllName.Buffer) );
        NtSetInformationProcess( GetCurrentProcess(), ProcessExecuteFlags, &flags, sizeof(flags) );
    }
    return wm;
}


/*************************************************************************
 *              alloc_thread_tls
 *
 * Allocate the per-thread structure for module TLS storage.
 */
static NTSTATUS alloc_thread_tls(void)
{
    void **pointers;
    UINT i, size;

    if (!tls_module_count) return STATUS_SUCCESS;

    if (!(pointers = RtlAllocateHeap( GetProcessHeap(), HEAP_ZERO_MEMORY,
                                      tls_module_count * sizeof(*pointers) )))
        return STATUS_NO_MEMORY;

    for (i = 0; i < tls_module_count; i++)
    {
        const IMAGE_TLS_DIRECTORY *dir = tls_dirs[i];

        if (!dir) continue;
        size = dir->EndAddressOfRawData - dir->StartAddressOfRawData;
        if (!size && !dir->SizeOfZeroFill) continue;

        if (!(pointers[i] = RtlAllocateHeap( GetProcessHeap(), 0, size + dir->SizeOfZeroFill )))
        {
            while (i) RtlFreeHeap( GetProcessHeap(), 0, pointers[--i] );
            RtlFreeHeap( GetProcessHeap(), 0, pointers );
            return STATUS_NO_MEMORY;
        }
        memcpy( pointers[i], (void *)dir->StartAddressOfRawData, size );
        memset( (char *)pointers[i] + size, 0, dir->SizeOfZeroFill );

        TRACE( "thread %04x slot %u: %u/%u bytes at %p\n",
               GetCurrentThreadId(), i, size, dir->SizeOfZeroFill, pointers[i] );
    }
    NtCurrentTeb()->ThreadLocalStoragePointer = pointers;
    return STATUS_SUCCESS;
}


/*************************************************************************
 *              call_tls_callbacks
 */
static void call_tls_callbacks( HMODULE module, UINT reason )
{
    const IMAGE_TLS_DIRECTORY *dir;
    const PIMAGE_TLS_CALLBACK *callback;
    ULONG dirsize;

    dir = RtlImageDirectoryEntryToData( module, TRUE, IMAGE_DIRECTORY_ENTRY_TLS, &dirsize );
    if (!dir || !dir->AddressOfCallBacks) return;

    for (callback = (const PIMAGE_TLS_CALLBACK *)dir->AddressOfCallBacks; *callback; callback++)
    {
        if (TRACE_ON(relay))
            DPRINTF("%04x:Call TLS callback (proc=%p,module=%p,reason=%s,reserved=0)\n",
                    GetCurrentThreadId(), *callback, module, reason_names[reason] );
        __TRY
        {
            (*callback)( module, reason, NULL );
        }
        __EXCEPT_ALL
        {
            if (TRACE_ON(relay))
                DPRINTF("%04x:exception in TLS callback (proc=%p,module=%p,reason=%s,reserved=0)\n",
                        GetCurrentThreadId(), callback, module, reason_names[reason] );
            return;
        }
        __ENDTRY
        if (TRACE_ON(relay))
            DPRINTF("%04x:Ret  TLS callback (proc=%p,module=%p,reason=%s,reserved=0)\n",
                    GetCurrentThreadId(), *callback, module, reason_names[reason] );
    }
}


/*************************************************************************
 *              MODULE_InitDLL
 */
static NTSTATUS MODULE_InitDLL( WINE_MODREF *wm, UINT reason, LPVOID lpReserved )
{
    WCHAR mod_name[32];
    NTSTATUS status = STATUS_SUCCESS;
    DLLENTRYPROC entry = wm->ldr.EntryPoint;
    void *module = wm->ldr.BaseAddress;
    BOOL retv = FALSE;

    /* Skip calls for modules loaded with special load flags */

    if (wm->ldr.Flags & LDR_DONT_RESOLVE_REFS) return STATUS_SUCCESS;
    if (wm->ldr.TlsIndex != -1) call_tls_callbacks( wm->ldr.BaseAddress, reason );
    if (!entry) return STATUS_SUCCESS;

    if (TRACE_ON(relay))
    {
        size_t len = min( wm->ldr.BaseDllName.Length, sizeof(mod_name)-sizeof(WCHAR) );
        memcpy( mod_name, wm->ldr.BaseDllName.Buffer, len );
        mod_name[len / sizeof(WCHAR)] = 0;
        DPRINTF("%04x:Call PE DLL (proc=%p,module=%p %s,reason=%s,res=%p)\n",
                GetCurrentThreadId(), entry, module, debugstr_w(mod_name),
                reason_names[reason], lpReserved );
    }
    else TRACE("(%p %s,%s,%p) - CALL\n", module, debugstr_w(wm->ldr.BaseDllName.Buffer),
               reason_names[reason], lpReserved );

    __TRY
    {
        retv = call_dll_entry_point( entry, module, reason, lpReserved );
        if (!retv)
            status = STATUS_DLL_INIT_FAILED;
    }
    __EXCEPT_ALL
    {
        if (TRACE_ON(relay))
            DPRINTF("%04x:exception in PE entry point (proc=%p,module=%p,reason=%s,res=%p)\n",
                    GetCurrentThreadId(), entry, module, reason_names[reason], lpReserved );
        status = GetExceptionCode();
    }
    __ENDTRY

    /* The state of the module list may have changed due to the call
       to the dll. We cannot assume that this module has not been
       deleted.  */
    if (TRACE_ON(relay))
        DPRINTF("%04x:Ret  PE DLL (proc=%p,module=%p %s,reason=%s,res=%p) retval=%x\n",
                GetCurrentThreadId(), entry, module, debugstr_w(mod_name),
                reason_names[reason], lpReserved, retv );
    else TRACE("(%p,%s,%p) - RETURN %d\n", module, reason_names[reason], lpReserved, retv );

    return status;
}


/*************************************************************************
 *		process_attach
 *
 * Send the process attach notification to all DLLs the given module
 * depends on (recursively). This is somewhat complicated due to the fact that
 *
 * - we have to respect the module dependencies, i.e. modules implicitly
 *   referenced by another module have to be initialized before the module
 *   itself can be initialized
 *
 * - the initialization routine of a DLL can itself call LoadLibrary,
 *   thereby introducing a whole new set of dependencies (even involving
 *   the 'old' modules) at any time during the whole process
 *
 * (Note that this routine can be recursively entered not only directly
 *  from itself, but also via LoadLibrary from one of the called initialization
 *  routines.)
 *
 * Furthermore, we need to rearrange the main WINE_MODREF list to allow
 * the process *detach* notifications to be sent in the correct order.
 * This must not only take into account module dependencies, but also
 * 'hidden' dependencies created by modules calling LoadLibrary in their
 * attach notification routine.
 *
 * The strategy is rather simple: we move a WINE_MODREF to the head of the
 * list after the attach notification has returned.  This implies that the
 * detach notifications are called in the reverse of the sequence the attach
 * notifications *returned*.
 *
 * The loader_section must be locked while calling this function.
 */
static NTSTATUS process_attach( WINE_MODREF *wm, LPVOID lpReserved )
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG_PTR cookie;
    int i;

    if (process_detaching) return status;

    /* prevent infinite recursion in case of cyclical dependencies */
    if (    ( wm->ldr.Flags & LDR_LOAD_IN_PROGRESS )
         || ( wm->ldr.Flags & LDR_PROCESS_ATTACHED ) )
        return status;

    TRACE("(%s,%p) - START\n", debugstr_w(wm->ldr.BaseDllName.Buffer), lpReserved );

    /* Tag current MODREF to prevent recursive loop */
    wm->ldr.Flags |= LDR_LOAD_IN_PROGRESS;
    if (lpReserved) wm->ldr.LoadCount = -1;  /* pin it if imported by the main exe */
    if (wm->ldr.ActivationContext) RtlActivateActivationContext( 0, wm->ldr.ActivationContext, &cookie );

    /* Recursively attach all DLLs this one depends on */
    for ( i = 0; i < wm->nDeps; i++ )
    {
        if (!wm->deps[i]) continue;
        if ((status = process_attach( wm->deps[i], lpReserved )) != STATUS_SUCCESS) break;
    }

    /* Call DLL entry point */
    if (status == STATUS_SUCCESS)
    {
        WINE_MODREF *prev = current_modref;
        current_modref = wm;
        status = MODULE_InitDLL( wm, DLL_PROCESS_ATTACH, lpReserved );
        if (status == STATUS_SUCCESS)
            wm->ldr.Flags |= LDR_PROCESS_ATTACHED;
        else
        {
            MODULE_InitDLL( wm, DLL_PROCESS_DETACH, lpReserved );
            /* point to the name so LdrInitializeThunk can print it */
            last_failed_modref = wm;
            WARN("Initialization of %s failed\n", debugstr_w(wm->ldr.BaseDllName.Buffer));
        }
        current_modref = prev;
    }

    if (!wm->ldr.InInitializationOrderModuleList.Flink)
        InsertTailList(&NtCurrentTeb()->Peb->LdrData->InInitializationOrderModuleList,
                       &wm->ldr.InInitializationOrderModuleList);

    if (wm->ldr.ActivationContext) RtlDeactivateActivationContext( 0, cookie );
    /* Remove recursion flag */
    wm->ldr.Flags &= ~LDR_LOAD_IN_PROGRESS;

    TRACE("(%s,%p) - END\n", debugstr_w(wm->ldr.BaseDllName.Buffer), lpReserved );
    return status;
}


/**********************************************************************
 *	    attach_implicitly_loaded_dlls
 *
 * Attach to the (builtin) dlls that have been implicitly loaded because
 * of a dependency at the Unix level, but not imported at the Win32 level.
 */
static void attach_implicitly_loaded_dlls( LPVOID reserved )
{
    for (;;)
    {
        PLIST_ENTRY mark, entry;

        mark = &NtCurrentTeb()->Peb->LdrData->InLoadOrderModuleList;
        for (entry = mark->Flink; entry != mark; entry = entry->Flink)
        {
            LDR_MODULE *mod = CONTAINING_RECORD(entry, LDR_MODULE, InLoadOrderModuleList);

            if (mod->Flags & (LDR_LOAD_IN_PROGRESS | LDR_PROCESS_ATTACHED)) continue;
            TRACE( "found implicitly loaded %s, attaching to it\n",
                   debugstr_w(mod->BaseDllName.Buffer));
            process_attach( CONTAINING_RECORD(mod, WINE_MODREF, ldr), reserved );
            break;  /* restart the search from the start */
        }
        if (entry == mark) break;  /* nothing found */
    }
}


/*************************************************************************
 *		process_detach
 *
 * Send DLL process detach notifications.  See the comment about calling
 * sequence at process_attach.
 */
static void process_detach(void)
{
    PLIST_ENTRY mark, entry;
    PLDR_MODULE mod;

    mark = &NtCurrentTeb()->Peb->LdrData->InInitializationOrderModuleList;
    do
    {
        for (entry = mark->Blink; entry != mark; entry = entry->Blink)
        {
            mod = CONTAINING_RECORD(entry, LDR_MODULE, 
                                    InInitializationOrderModuleList);
            /* Check whether to detach this DLL */
            if ( !(mod->Flags & LDR_PROCESS_ATTACHED) )
                continue;
            if ( mod->LoadCount && !process_detaching )
                continue;

            /* Call detach notification */
            mod->Flags &= ~LDR_PROCESS_ATTACHED;
            MODULE_InitDLL( CONTAINING_RECORD(mod, WINE_MODREF, ldr), 
                            DLL_PROCESS_DETACH, ULongToPtr(process_detaching) );

            /* Restart at head of WINE_MODREF list, as entries might have
               been added and/or removed while performing the call ... */
            break;
        }
    } while (entry != mark);
}

/*************************************************************************
 *		MODULE_DllThreadAttach
 *
 * Send DLL thread attach notifications. These are sent in the
 * reverse sequence of process detach notification.
 *
 */
NTSTATUS MODULE_DllThreadAttach( LPVOID lpReserved )
{
    PLIST_ENTRY mark, entry;
    PLDR_MODULE mod;
    NTSTATUS    status;

    /* don't do any attach calls if process is exiting */
    if (process_detaching) return STATUS_SUCCESS;

    RtlEnterCriticalSection( &loader_section );

    RtlAcquirePebLock();
    InsertHeadList( &tls_links, &NtCurrentTeb()->TlsLinks );
    RtlReleasePebLock();

    if ((status = alloc_thread_tls()) != STATUS_SUCCESS) goto done;

    mark = &NtCurrentTeb()->Peb->LdrData->InInitializationOrderModuleList;
    for (entry = mark->Flink; entry != mark; entry = entry->Flink)
    {
        mod = CONTAINING_RECORD(entry, LDR_MODULE, 
                                InInitializationOrderModuleList);
        if ( !(mod->Flags & LDR_PROCESS_ATTACHED) )
            continue;
        if ( mod->Flags & LDR_NO_DLL_CALLS )
            continue;

        MODULE_InitDLL( CONTAINING_RECORD(mod, WINE_MODREF, ldr),
                        DLL_THREAD_ATTACH, lpReserved );
    }

done:
    RtlLeaveCriticalSection( &loader_section );
    return status;
}

/******************************************************************
 *		LdrDisableThreadCalloutsForDll (NTDLL.@)
 *
 */
NTSTATUS WINAPI LdrDisableThreadCalloutsForDll(HMODULE hModule)
{
    WINE_MODREF *wm;
    NTSTATUS    ret = STATUS_SUCCESS;

    RtlEnterCriticalSection( &loader_section );

    wm = get_modref( hModule );
    if (!wm || wm->ldr.TlsIndex != -1)
        ret = STATUS_DLL_NOT_FOUND;
    else
        wm->ldr.Flags |= LDR_NO_DLL_CALLS;

    RtlLeaveCriticalSection( &loader_section );

    return ret;
}

/******************************************************************
 *              LdrFindEntryForAddress (NTDLL.@)
 *
 * The loader_section must be locked while calling this function
 */
NTSTATUS WINAPI LdrFindEntryForAddress(const void* addr, PLDR_MODULE* pmod)
{
    PLIST_ENTRY mark, entry;
    PLDR_MODULE mod;

    mark = &NtCurrentTeb()->Peb->LdrData->InMemoryOrderModuleList;
    for (entry = mark->Flink; entry != mark; entry = entry->Flink)
    {
        mod = CONTAINING_RECORD(entry, LDR_MODULE, InMemoryOrderModuleList);
        if (mod->BaseAddress <= addr &&
            (const char *)addr < (char*)mod->BaseAddress + mod->SizeOfImage)
        {
            *pmod = mod;
            return STATUS_SUCCESS;
        }
        if (mod->BaseAddress > addr) break;
    }
    return STATUS_NO_MORE_ENTRIES;
}

/******************************************************************
 *		LdrLockLoaderLock  (NTDLL.@)
 *
 * Note: some flags are not implemented.
 * Flag 0x01 is used to raise exceptions on errors.
 */
NTSTATUS WINAPI LdrLockLoaderLock( ULONG flags, ULONG *result, ULONG_PTR *magic )
{
    if (flags & ~0x2) FIXME( "flags %x not supported\n", flags );

    if (result) *result = 0;
    if (magic) *magic = 0;
    if (flags & ~0x3) return STATUS_INVALID_PARAMETER_1;
    if (!result && (flags & 0x2)) return STATUS_INVALID_PARAMETER_2;
    if (!magic) return STATUS_INVALID_PARAMETER_3;

    if (flags & 0x2)
    {
        if (!RtlTryEnterCriticalSection( &loader_section ))
        {
            *result = 2;
            return STATUS_SUCCESS;
        }
        *result = 1;
    }
    else
    {
        RtlEnterCriticalSection( &loader_section );
        if (result) *result = 1;
    }
    *magic = GetCurrentThreadId();
    return STATUS_SUCCESS;
}


/******************************************************************
 *		LdrUnlockLoaderUnlock  (NTDLL.@)
 */
NTSTATUS WINAPI LdrUnlockLoaderLock( ULONG flags, ULONG_PTR magic )
{
    if (magic)
    {
        if (magic != GetCurrentThreadId()) return STATUS_INVALID_PARAMETER_2;
        RtlLeaveCriticalSection( &loader_section );
    }
    return STATUS_SUCCESS;
}


/******************************************************************
 *		LdrGetProcedureAddress  (NTDLL.@)
 */
NTSTATUS WINAPI LdrGetProcedureAddress(HMODULE module, const ANSI_STRING *name,
                                       ULONG ord, PVOID *address)
{
    IMAGE_EXPORT_DIRECTORY *exports;
    DWORD exp_size;
    NTSTATUS ret = STATUS_PROCEDURE_NOT_FOUND;

    RtlEnterCriticalSection( &loader_section );

    /* check if the module itself is invalid to return the proper error */
    if (!get_modref( module )) ret = STATUS_DLL_NOT_FOUND;
    else if ((exports = RtlImageDirectoryEntryToData( module, TRUE,
                                                      IMAGE_DIRECTORY_ENTRY_EXPORT, &exp_size )))
    {
        LPCWSTR load_path = NtCurrentTeb()->Peb->ProcessParameters->DllPath.Buffer;
        void *proc = name ? find_named_export( module, exports, exp_size, name->Buffer, -1, load_path )
                          : find_ordinal_export( module, exports, exp_size, ord - exports->Base, load_path );
        if (proc)
        {
            *address = proc;
            ret = STATUS_SUCCESS;
        }
    }

    RtlLeaveCriticalSection( &loader_section );
    return ret;
}


/***********************************************************************
 *           is_fake_dll
 *
 * Check if a loaded native dll is a Wine fake dll.
 */
static BOOL is_fake_dll( HANDLE handle )
{
    static const char fakedll_signature[] = "Wine placeholder DLL";
    char buffer[sizeof(IMAGE_DOS_HEADER) + sizeof(fakedll_signature)];
    const IMAGE_DOS_HEADER *dos = (const IMAGE_DOS_HEADER *)buffer;
    IO_STATUS_BLOCK io;
    LARGE_INTEGER offset;

    offset.QuadPart = 0;
    if (NtReadFile( handle, 0, NULL, 0, &io, buffer, sizeof(buffer), &offset, NULL )) return FALSE;
    if (io.Information < sizeof(buffer)) return FALSE;
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) return FALSE;
    if (dos->e_lfanew >= sizeof(*dos) + sizeof(fakedll_signature) &&
        !memcmp( dos + 1, fakedll_signature, sizeof(fakedll_signature) )) return TRUE;
    return FALSE;
}


/***********************************************************************
 *           get_builtin_fullname
 *
 * Build the full pathname for a builtin dll.
 */
static WCHAR *get_builtin_fullname( const WCHAR *path, const char *filename )
{
    static const WCHAR soW[] = {'.','s','o',0};
    WCHAR *p, *fullname;
    size_t i, len = strlen(filename);

    /* check if path can correspond to the dll we have */
    if (path && (p = strrchrW( path, '\\' )))
    {
        p++;
        for (i = 0; i < len; i++)
            if (tolowerW(p[i]) != tolowerW( (WCHAR)filename[i]) ) break;
        if (i == len && (!p[len] || !strcmpiW( p + len, soW )))
        {
            /* the filename matches, use path as the full path */
            len += p - path;
            if ((fullname = RtlAllocateHeap( GetProcessHeap(), 0, (len + 1) * sizeof(WCHAR) )))
            {
                memcpy( fullname, path, len * sizeof(WCHAR) );
                fullname[len] = 0;
            }
            return fullname;
        }
    }

    if ((fullname = RtlAllocateHeap( GetProcessHeap(), 0,
                                     system_dir.MaximumLength + (len + 1) * sizeof(WCHAR) )))
    {
        memcpy( fullname, system_dir.Buffer, system_dir.Length );
        p = fullname + system_dir.Length / sizeof(WCHAR);
        if (p > fullname && p[-1] != '\\') *p++ = '\\';
        ascii_to_unicode( p, filename, len + 1 );
    }
    return fullname;
}


/*************************************************************************
 *		is_16bit_builtin
 */
static BOOL is_16bit_builtin( HMODULE module )
{
    const IMAGE_EXPORT_DIRECTORY *exports;
    DWORD exp_size;

    if (!(exports = RtlImageDirectoryEntryToData( module, TRUE,
                                                  IMAGE_DIRECTORY_ENTRY_EXPORT, &exp_size )))
        return FALSE;

    return find_named_export( module, exports, exp_size, "__wine_spec_dos_header", -1, NULL ) != NULL;
}


/***********************************************************************
 *           load_builtin_callback
 *
 * Load a library in memory; callback function for wine_dll_register
 */
static void load_builtin_callback( void *module, const char *filename )
{
    static const WCHAR emptyW[1];
    IMAGE_NT_HEADERS *nt;
    WINE_MODREF *wm;
    WCHAR *fullname;
    const WCHAR *load_path;

    if (!module)
    {
        ERR("could not map image for %s\n", filename ? filename : "main exe" );
        return;
    }
    if (!(nt = RtlImageNtHeader( module )))
    {
        ERR( "bad module for %s\n", filename ? filename : "main exe" );
        builtin_load_info->status = STATUS_INVALID_IMAGE_FORMAT;
        return;
    }

    virtual_create_builtin_view( module );

    /* create the MODREF */

    if (!(fullname = get_builtin_fullname( builtin_load_info->filename, filename )))
    {
        ERR( "can't load %s\n", filename );
        builtin_load_info->status = STATUS_NO_MEMORY;
        return;
    }

    wm = alloc_module( module, fullname );
    RtlFreeHeap( GetProcessHeap(), 0, fullname );
    if (!wm)
    {
        ERR( "can't load %s\n", filename );
        builtin_load_info->status = STATUS_NO_MEMORY;
        return;
    }
    wm->ldr.Flags |= LDR_WINE_INTERNAL;

    if ((nt->FileHeader.Characteristics & IMAGE_FILE_DLL) ||
        nt->OptionalHeader.Subsystem == IMAGE_SUBSYSTEM_NATIVE ||
        is_16bit_builtin( module ))
    {
        /* fixup imports */

        load_path = builtin_load_info->load_path;
        if (!load_path) load_path = NtCurrentTeb()->Peb->ProcessParameters->DllPath.Buffer;
        if (!load_path) load_path = emptyW;
        if (fixup_imports( wm, load_path ) != STATUS_SUCCESS)
        {
            /* the module has only be inserted in the load & memory order lists */
            RemoveEntryList(&wm->ldr.InLoadOrderModuleList);
            RemoveEntryList(&wm->ldr.InMemoryOrderModuleList);
            /* FIXME: free the modref */
            builtin_load_info->status = STATUS_DLL_NOT_FOUND;
            return;
        }
    }

    builtin_load_info->wm = wm;
    TRACE( "loaded %s %p %p\n", filename, wm, module );

    /* send the DLL load event */

    SERVER_START_REQ( load_dll )
    {
        req->mapping    = 0;
        req->base       = wine_server_client_ptr( module );
        req->size       = nt->OptionalHeader.SizeOfImage;
        req->dbg_offset = nt->FileHeader.PointerToSymbolTable;
        req->dbg_size   = nt->FileHeader.NumberOfSymbols;
        req->name       = wine_server_client_ptr( &wm->ldr.FullDllName.Buffer );
        wine_server_add_data( req, wm->ldr.FullDllName.Buffer, wm->ldr.FullDllName.Length );
        wine_server_call( req );
    }
    SERVER_END_REQ;

    /* setup relay debugging entry points */
    if (TRACE_ON(relay)) RELAY_SetupDLL( module );
}


/******************************************************************************
 *	load_native_dll  (internal)
 */
static NTSTATUS load_native_dll( LPCWSTR load_path, LPCWSTR name, HANDLE file,
                                 DWORD flags, WINE_MODREF** pwm )
{
    void *module;
    HANDLE mapping;
    LARGE_INTEGER size;
    IMAGE_NT_HEADERS *nt;
    SIZE_T len = 0;
    WINE_MODREF *wm;
    NTSTATUS status;

    TRACE("Trying native dll %s\n", debugstr_w(name));

    size.QuadPart = 0;
    status = NtCreateSection( &mapping, STANDARD_RIGHTS_REQUIRED | SECTION_QUERY | SECTION_MAP_READ,
                              NULL, &size, PAGE_EXECUTE_READ, SEC_IMAGE, file );
    if (status != STATUS_SUCCESS) return status;

    module = NULL;
    status = NtMapViewOfSection( mapping, NtCurrentProcess(),
                                 &module, 0, 0, &size, &len, ViewShare, 0, PAGE_EXECUTE_READ );
    if (status < 0) goto done;

    /* create the MODREF */

    if (!(wm = alloc_module( module, name )))
    {
        status = STATUS_NO_MEMORY;
        goto done;
    }

    /* fixup imports */

    nt = RtlImageNtHeader( module );

    if (!(flags & DONT_RESOLVE_DLL_REFERENCES) &&
        ((nt->FileHeader.Characteristics & IMAGE_FILE_DLL) ||
         nt->OptionalHeader.Subsystem == IMAGE_SUBSYSTEM_NATIVE))
    {
        if ((status = fixup_imports( wm, load_path )) != STATUS_SUCCESS)
        {
            /* the module has only be inserted in the load & memory order lists */
            RemoveEntryList(&wm->ldr.InLoadOrderModuleList);
            RemoveEntryList(&wm->ldr.InMemoryOrderModuleList);

            /* FIXME: there are several more dangling references
             * left. Including dlls loaded by this dll before the
             * failed one. Unrolling is rather difficult with the
             * current structure and we can leave them lying
             * around with no problems, so we don't care.
             * As these might reference our wm, we don't free it.
             */
            goto done;
        }
    }

    /* send DLL load event */

    SERVER_START_REQ( load_dll )
    {
        req->mapping    = wine_server_obj_handle( mapping );
        req->base       = wine_server_client_ptr( module );
        req->size       = nt->OptionalHeader.SizeOfImage;
        req->dbg_offset = nt->FileHeader.PointerToSymbolTable;
        req->dbg_size   = nt->FileHeader.NumberOfSymbols;
        req->name       = wine_server_client_ptr( &wm->ldr.FullDllName.Buffer );
        wine_server_add_data( req, wm->ldr.FullDllName.Buffer, wm->ldr.FullDllName.Length );
        wine_server_call( req );
    }
    SERVER_END_REQ;

    if ((wm->ldr.Flags & LDR_IMAGE_IS_DLL) && TRACE_ON(snoop)) SNOOP_SetupDLL( module );

    TRACE_(loaddll)( "Loaded %s at %p: native\n", debugstr_w(wm->ldr.FullDllName.Buffer), module );

    wm->ldr.LoadCount = 1;
    *pwm = wm;
    status = STATUS_SUCCESS;
done:
    NtClose( mapping );
    return status;
}


/***********************************************************************
 *           load_builtin_dll
 */
static NTSTATUS load_builtin_dll( LPCWSTR load_path, LPCWSTR path, HANDLE file,
                                  DWORD flags, WINE_MODREF** pwm )
{
    char error[256], dllname[MAX_PATH];
    const WCHAR *name, *p;
    DWORD len, i;
    void *handle = NULL;
    struct builtin_load_info info, *prev_info;

    /* Fix the name in case we have a full path and extension */
    name = path;
    if ((p = strrchrW( name, '\\' ))) name = p + 1;
    if ((p = strrchrW( name, '/' ))) name = p + 1;

    /* load_library will modify info.status. Note also that load_library can be
     * called several times, if the .so file we're loading has dependencies.
     * info.status will gather all the errors we may get while loading all these
     * libraries
     */
    info.load_path = load_path;
    info.filename  = NULL;
    info.status    = STATUS_SUCCESS;
    info.wm        = NULL;

    if (file)  /* we have a real file, try to load it */
    {
        UNICODE_STRING nt_name;
        ANSI_STRING unix_name;

        TRACE("Trying built-in %s\n", debugstr_w(path));

        if (!RtlDosPathNameToNtPathName_U( path, &nt_name, NULL, NULL ))
            return STATUS_DLL_NOT_FOUND;

        if (wine_nt_to_unix_file_name( &nt_name, &unix_name, FILE_OPEN, FALSE ))
        {
            RtlFreeUnicodeString( &nt_name );
            return STATUS_DLL_NOT_FOUND;
        }
        prev_info = builtin_load_info;
        info.filename = nt_name.Buffer + 4;  /* skip \??\ */
        builtin_load_info = &info;
        handle = wine_dlopen( unix_name.Buffer, RTLD_NOW, error, sizeof(error) );
        builtin_load_info = prev_info;
        RtlFreeUnicodeString( &nt_name );
        RtlFreeHeap( GetProcessHeap(), 0, unix_name.Buffer );
        if (!handle)
        {
            WARN( "failed to load .so lib for builtin %s: %s\n", debugstr_w(path), error );
            return STATUS_INVALID_IMAGE_FORMAT;
        }
    }
    else
    {
        int file_exists;

        TRACE("Trying built-in %s\n", debugstr_w(name));

        /* we don't want to depend on the current codepage here */
        len = strlenW( name ) + 1;
        if (len >= sizeof(dllname)) return STATUS_NAME_TOO_LONG;
        for (i = 0; i < len; i++)
        {
            if (name[i] > 127) return STATUS_DLL_NOT_FOUND;
            dllname[i] = (char)name[i];
            if (dllname[i] >= 'A' && dllname[i] <= 'Z') dllname[i] += 'a' - 'A';
        }

        prev_info = builtin_load_info;
        builtin_load_info = &info;
        handle = wine_dll_load( dllname, error, sizeof(error), &file_exists );
        builtin_load_info = prev_info;
        if (!handle)
        {
            if (!file_exists)
            {
                /* The file does not exist -> WARN() */
                WARN("cannot open .so lib for builtin %s: %s\n", debugstr_w(name), error);
                return STATUS_DLL_NOT_FOUND;
            }
            /* ERR() for all other errors (missing functions, ...) */
            ERR("failed to load .so lib for builtin %s: %s\n", debugstr_w(name), error );
            return STATUS_PROCEDURE_NOT_FOUND;
        }
    }

    if (info.status != STATUS_SUCCESS)
    {
        wine_dll_unload( handle );
        return info.status;
    }

    if (!info.wm)
    {
        PLIST_ENTRY mark, entry;

        /* The constructor wasn't called, this means the .so is already
         * loaded under a different name. Try to find the wm for it. */

        mark = &NtCurrentTeb()->Peb->LdrData->InLoadOrderModuleList;
        for (entry = mark->Flink; entry != mark; entry = entry->Flink)
        {
            LDR_MODULE *mod = CONTAINING_RECORD(entry, LDR_MODULE, InLoadOrderModuleList);
            if (mod->Flags & LDR_WINE_INTERNAL && mod->SectionHandle == handle)
            {
                info.wm = CONTAINING_RECORD(mod, WINE_MODREF, ldr);
                TRACE( "Found %s at %p for builtin %s\n",
                       debugstr_w(info.wm->ldr.FullDllName.Buffer), info.wm->ldr.BaseAddress, debugstr_w(path) );
                break;
            }
        }
        wine_dll_unload( handle );  /* release the libdl refcount */
        if (!info.wm) return STATUS_INVALID_IMAGE_FORMAT;
        if (info.wm->ldr.LoadCount != -1) info.wm->ldr.LoadCount++;
    }
    else
    {
        TRACE_(loaddll)( "Loaded %s at %p: builtin\n", debugstr_w(info.wm->ldr.FullDllName.Buffer), info.wm->ldr.BaseAddress );
        info.wm->ldr.LoadCount = 1;
        info.wm->ldr.SectionHandle = handle;
    }

    *pwm = info.wm;
    return STATUS_SUCCESS;
}


/***********************************************************************
 *	find_actctx_dll
 *
 * Find the full path (if any) of the dll from the activation context.
 */
static NTSTATUS find_actctx_dll( LPCWSTR libname, LPWSTR *fullname )
{
    static const WCHAR winsxsW[] = {'\\','w','i','n','s','x','s','\\'};
    static const WCHAR dotManifestW[] = {'.','m','a','n','i','f','e','s','t',0};

    ACTIVATION_CONTEXT_ASSEMBLY_DETAILED_INFORMATION *info;
    ACTCTX_SECTION_KEYED_DATA data;
    UNICODE_STRING nameW;
    NTSTATUS status;
    SIZE_T needed, size = 1024;
    WCHAR *p;

    RtlInitUnicodeString( &nameW, libname );
    data.cbSize = sizeof(data);
    status = RtlFindActivationContextSectionString( FIND_ACTCTX_SECTION_KEY_RETURN_HACTCTX, NULL,
                                                    ACTIVATION_CONTEXT_SECTION_DLL_REDIRECTION,
                                                    &nameW, &data );
    if (status != STATUS_SUCCESS) return status;

    for (;;)
    {
        if (!(info = RtlAllocateHeap( GetProcessHeap(), 0, size )))
        {
            status = STATUS_NO_MEMORY;
            goto done;
        }
        status = RtlQueryInformationActivationContext( 0, data.hActCtx, &data.ulAssemblyRosterIndex,
                                                       AssemblyDetailedInformationInActivationContext,
                                                       info, size, &needed );
        if (status == STATUS_SUCCESS) break;
        if (status != STATUS_BUFFER_TOO_SMALL) goto done;
        RtlFreeHeap( GetProcessHeap(), 0, info );
        size = needed;
        /* restart with larger buffer */
    }

    if (!info->lpAssemblyManifestPath || !info->lpAssemblyDirectoryName)
    {
        status = STATUS_SXS_KEY_NOT_FOUND;
        goto done;
    }

    if ((p = strrchrW( info->lpAssemblyManifestPath, '\\' )))
    {
        DWORD dirlen = info->ulAssemblyDirectoryNameLength / sizeof(WCHAR);

        p++;
        if (strncmpiW( p, info->lpAssemblyDirectoryName, dirlen ) || strcmpiW( p + dirlen, dotManifestW ))
        {
            /* manifest name does not match directory name, so it's not a global
             * windows/winsxs manifest; use the manifest directory name instead */
            dirlen = p - info->lpAssemblyManifestPath;
            needed = (dirlen + 1) * sizeof(WCHAR) + nameW.Length;
            if (!(*fullname = p = RtlAllocateHeap( GetProcessHeap(), 0, needed )))
            {
                status = STATUS_NO_MEMORY;
                goto done;
            }
            memcpy( p, info->lpAssemblyManifestPath, dirlen * sizeof(WCHAR) );
            p += dirlen;
            strcpyW( p, libname );
            goto done;
        }
    }

    needed = (strlenW(user_shared_data->NtSystemRoot) * sizeof(WCHAR) +
              sizeof(winsxsW) + info->ulAssemblyDirectoryNameLength + nameW.Length + 2*sizeof(WCHAR));

    if (!(*fullname = p = RtlAllocateHeap( GetProcessHeap(), 0, needed )))
    {
        status = STATUS_NO_MEMORY;
        goto done;
    }
    strcpyW( p, user_shared_data->NtSystemRoot );
    p += strlenW(p);
    memcpy( p, winsxsW, sizeof(winsxsW) );
    p += sizeof(winsxsW) / sizeof(WCHAR);
    memcpy( p, info->lpAssemblyDirectoryName, info->ulAssemblyDirectoryNameLength );
    p += info->ulAssemblyDirectoryNameLength / sizeof(WCHAR);
    *p++ = '\\';
    strcpyW( p, libname );
done:
    RtlFreeHeap( GetProcessHeap(), 0, info );
    RtlReleaseActivationContext( data.hActCtx );
    return status;
}


/***********************************************************************
 *	find_dll_file
 *
 * Find the file (or already loaded module) for a given dll name.
 */
static NTSTATUS find_dll_file( const WCHAR *load_path, const WCHAR *libname,
                               WCHAR *filename, ULONG *size, WINE_MODREF **pwm, HANDLE *handle )
{
    OBJECT_ATTRIBUTES attr;
    IO_STATUS_BLOCK io;
    UNICODE_STRING nt_name;
    WCHAR *file_part, *ext, *dllname;
    ULONG len;

    /* first append .dll if needed */

    dllname = NULL;
    if (!(ext = strrchrW( libname, '.')) || strchrW( ext, '/' ) || strchrW( ext, '\\'))
    {
        if (!(dllname = RtlAllocateHeap( GetProcessHeap(), 0,
                                         (strlenW(libname) * sizeof(WCHAR)) + sizeof(dllW) )))
            return STATUS_NO_MEMORY;
        strcpyW( dllname, libname );
        strcatW( dllname, dllW );
        libname = dllname;
    }

    nt_name.Buffer = NULL;

    if (!contains_path( libname ))
    {
        NTSTATUS status;
        WCHAR *fullname = NULL;

        if ((*pwm = find_basename_module( libname )) != NULL) goto found;

        status = find_actctx_dll( libname, &fullname );
        if (status == STATUS_SUCCESS)
        {
            TRACE ("found %s for %s\n", debugstr_w(fullname), debugstr_w(libname) );
            RtlFreeHeap( GetProcessHeap(), 0, dllname );
            libname = dllname = fullname;
        }
        else if (status != STATUS_SXS_KEY_NOT_FOUND)
        {
            RtlFreeHeap( GetProcessHeap(), 0, dllname );
            return status;
        }
    }

    if (RtlDetermineDosPathNameType_U( libname ) == RELATIVE_PATH)
    {
        /* we need to search for it */
        len = RtlDosSearchPath_U( load_path, libname, NULL, *size, filename, &file_part );
        if (len)
        {
            if (len >= *size) goto overflow;
            if ((*pwm = find_fullname_module( filename )) || !handle) goto found;

            if (!RtlDosPathNameToNtPathName_U( filename, &nt_name, NULL, NULL ))
            {
                RtlFreeHeap( GetProcessHeap(), 0, dllname );
                return STATUS_NO_MEMORY;
            }
            attr.Length = sizeof(attr);
            attr.RootDirectory = 0;
            attr.Attributes = OBJ_CASE_INSENSITIVE;
            attr.ObjectName = &nt_name;
            attr.SecurityDescriptor = NULL;
            attr.SecurityQualityOfService = NULL;
            if (NtOpenFile( handle, GENERIC_READ, &attr, &io, FILE_SHARE_READ|FILE_SHARE_DELETE, FILE_SYNCHRONOUS_IO_NONALERT|FILE_NON_DIRECTORY_FILE )) *handle = 0;
            goto found;
        }

        /* not found */

        if (!contains_path( libname ))
        {
            /* if libname doesn't contain a path at all, we simply return the name as is,
             * to be loaded as builtin */
            len = strlenW(libname) * sizeof(WCHAR);
            if (len >= *size) goto overflow;
            strcpyW( filename, libname );
            goto found;
        }
    }

    /* absolute path name, or relative path name but not found above */

    if (!RtlDosPathNameToNtPathName_U( libname, &nt_name, &file_part, NULL ))
    {
        RtlFreeHeap( GetProcessHeap(), 0, dllname );
        return STATUS_NO_MEMORY;
    }
    len = nt_name.Length - 4*sizeof(WCHAR);  /* for \??\ prefix */
    if (len >= *size) goto overflow;
    memcpy( filename, nt_name.Buffer + 4, len + sizeof(WCHAR) );
    if (!(*pwm = find_fullname_module( filename )) && handle)
    {
        attr.Length = sizeof(attr);
        attr.RootDirectory = 0;
        attr.Attributes = OBJ_CASE_INSENSITIVE;
        attr.ObjectName = &nt_name;
        attr.SecurityDescriptor = NULL;
        attr.SecurityQualityOfService = NULL;
        if (NtOpenFile( handle, GENERIC_READ, &attr, &io, FILE_SHARE_READ|FILE_SHARE_DELETE, FILE_SYNCHRONOUS_IO_NONALERT|FILE_NON_DIRECTORY_FILE )) *handle = 0;
    }
found:
    RtlFreeUnicodeString( &nt_name );
    RtlFreeHeap( GetProcessHeap(), 0, dllname );
    return STATUS_SUCCESS;

overflow:
    RtlFreeUnicodeString( &nt_name );
    RtlFreeHeap( GetProcessHeap(), 0, dllname );
    *size = len + sizeof(WCHAR);
    return STATUS_BUFFER_TOO_SMALL;
}


/***********************************************************************
 *	load_dll  (internal)
 *
 * Load a PE style module according to the load order.
 * The loader_section must be locked while calling this function.
 */
static NTSTATUS load_dll( LPCWSTR load_path, LPCWSTR libname, DWORD flags, WINE_MODREF** pwm )
{
    enum loadorder loadorder;
    WCHAR buffer[32];
    WCHAR *filename;
    ULONG size;
    WINE_MODREF *main_exe;
    HANDLE handle = 0;
    NTSTATUS nts;

    TRACE( "looking for %s in %s\n", debugstr_w(libname), debugstr_w(load_path) );

    *pwm = NULL;
    filename = buffer;
    size = sizeof(buffer);
    for (;;)
    {
        nts = find_dll_file( load_path, libname, filename, &size, pwm, &handle );
        if (nts == STATUS_SUCCESS) break;
        if (filename != buffer) RtlFreeHeap( GetProcessHeap(), 0, filename );
        if (nts != STATUS_BUFFER_TOO_SMALL) return nts;
        /* grow the buffer and retry */
        if (!(filename = RtlAllocateHeap( GetProcessHeap(), 0, size ))) return STATUS_NO_MEMORY;
    }

    if (*pwm)  /* found already loaded module */
    {
        if ((*pwm)->ldr.LoadCount != -1) (*pwm)->ldr.LoadCount++;

        TRACE("Found %s for %s at %p, count=%d\n",
              debugstr_w((*pwm)->ldr.FullDllName.Buffer), debugstr_w(libname),
              (*pwm)->ldr.BaseAddress, (*pwm)->ldr.LoadCount);
        if (filename != buffer) RtlFreeHeap( GetProcessHeap(), 0, filename );
        return STATUS_SUCCESS;
    }

    main_exe = get_modref( NtCurrentTeb()->Peb->ImageBaseAddress );
    loadorder = get_load_order( main_exe ? main_exe->ldr.BaseDllName.Buffer : NULL, filename );

    if (handle && is_fake_dll( handle ))
    {
        TRACE( "%s is a fake Wine dll\n", debugstr_w(filename) );
        NtClose( handle );
        handle = 0;
    }

    switch(loadorder)
    {
    case LO_INVALID:
        nts = STATUS_NO_MEMORY;
        break;
    case LO_DISABLED:
        nts = STATUS_DLL_NOT_FOUND;
        break;
    case LO_NATIVE:
    case LO_NATIVE_BUILTIN:
        if (!handle) nts = STATUS_DLL_NOT_FOUND;
        else
        {
            nts = load_native_dll( load_path, filename, handle, flags, pwm );
            if (nts == STATUS_INVALID_IMAGE_NOT_MZ)
                /* not in PE format, maybe it's a builtin */
                nts = load_builtin_dll( load_path, filename, handle, flags, pwm );
        }
        if (nts == STATUS_DLL_NOT_FOUND && loadorder == LO_NATIVE_BUILTIN)
            nts = load_builtin_dll( load_path, filename, 0, flags, pwm );
        break;
    case LO_BUILTIN:
    case LO_BUILTIN_NATIVE:
    case LO_DEFAULT:  /* default is builtin,native */
        nts = load_builtin_dll( load_path, filename, handle, flags, pwm );
        if (!handle) break;  /* nothing else we can try */
        /* file is not a builtin library, try without using the specified file */
        if (nts != STATUS_SUCCESS)
            nts = load_builtin_dll( load_path, filename, 0, flags, pwm );
        if (nts == STATUS_SUCCESS && loadorder == LO_DEFAULT &&
            (MODULE_InitDLL( *pwm, DLL_WINE_PREATTACH, NULL ) != STATUS_SUCCESS))
        {
            /* stub-only dll, try native */
            TRACE( "%s pre-attach returned FALSE, preferring native\n", debugstr_w(filename) );
            LdrUnloadDll( (*pwm)->ldr.BaseAddress );
            nts = STATUS_DLL_NOT_FOUND;
        }
        if (nts == STATUS_DLL_NOT_FOUND && loadorder != LO_BUILTIN)
            nts = load_native_dll( load_path, filename, handle, flags, pwm );
        break;
    }

    if (nts == STATUS_SUCCESS)
    {
        /* Initialize DLL just loaded */
        TRACE("Loaded module %s (%s) at %p\n", debugstr_w(filename),
              ((*pwm)->ldr.Flags & LDR_WINE_INTERNAL) ? "builtin" : "native",
              (*pwm)->ldr.BaseAddress);
        if (handle) NtClose( handle );
        if (filename != buffer) RtlFreeHeap( GetProcessHeap(), 0, filename );
        return nts;
    }

    WARN("Failed to load module %s; status=%x\n", debugstr_w(libname), nts);
    if (handle) NtClose( handle );
    if (filename != buffer) RtlFreeHeap( GetProcessHeap(), 0, filename );
    return nts;
}

/******************************************************************
 *		LdrLoadDll (NTDLL.@)
 */
NTSTATUS WINAPI DECLSPEC_HOTPATCH LdrLoadDll(LPCWSTR path_name, DWORD flags,
                                             const UNICODE_STRING *libname, HMODULE* hModule)
{
    WINE_MODREF *wm;
    NTSTATUS nts;

    RtlEnterCriticalSection( &loader_section );

    if (!path_name) path_name = NtCurrentTeb()->Peb->ProcessParameters->DllPath.Buffer;
    nts = load_dll( path_name, libname->Buffer, flags, &wm );

    if (nts == STATUS_SUCCESS && !(wm->ldr.Flags & LDR_DONT_RESOLVE_REFS))
    {
        nts = process_attach( wm, NULL );
        if (nts != STATUS_SUCCESS)
        {
            LdrUnloadDll(wm->ldr.BaseAddress);
            wm = NULL;
        }
    }
    *hModule = (wm) ? wm->ldr.BaseAddress : NULL;

    RtlLeaveCriticalSection( &loader_section );
    return nts;
}


/******************************************************************
 *		LdrGetDllHandle (NTDLL.@)
 */
NTSTATUS WINAPI LdrGetDllHandle( LPCWSTR load_path, ULONG flags, const UNICODE_STRING *name, HMODULE *base )
{
    NTSTATUS status;
    WCHAR buffer[128];
    WCHAR *filename;
    ULONG size;
    WINE_MODREF *wm;

    RtlEnterCriticalSection( &loader_section );

    if (!load_path) load_path = NtCurrentTeb()->Peb->ProcessParameters->DllPath.Buffer;

    filename = buffer;
    size = sizeof(buffer);
    for (;;)
    {
        status = find_dll_file( load_path, name->Buffer, filename, &size, &wm, NULL );
        if (filename != buffer) RtlFreeHeap( GetProcessHeap(), 0, filename );
        if (status != STATUS_BUFFER_TOO_SMALL) break;
        /* grow the buffer and retry */
        if (!(filename = RtlAllocateHeap( GetProcessHeap(), 0, size )))
        {
            status = STATUS_NO_MEMORY;
            break;
        }
    }

    if (status == STATUS_SUCCESS)
    {
        if (wm) *base = wm->ldr.BaseAddress;
        else status = STATUS_DLL_NOT_FOUND;
    }

    RtlLeaveCriticalSection( &loader_section );
    TRACE( "%s -> %p (load path %s)\n", debugstr_us(name), status ? NULL : *base, debugstr_w(load_path) );
    return status;
}


/******************************************************************
 *		LdrAddRefDll (NTDLL.@)
 */
NTSTATUS WINAPI LdrAddRefDll( ULONG flags, HMODULE module )
{
    NTSTATUS ret = STATUS_SUCCESS;
    WINE_MODREF *wm;

    if (flags & ~LDR_ADDREF_DLL_PIN) FIXME( "%p flags %x not implemented\n", module, flags );

    RtlEnterCriticalSection( &loader_section );

    if ((wm = get_modref( module )))
    {
        if (flags & LDR_ADDREF_DLL_PIN)
            wm->ldr.LoadCount = -1;
        else
            if (wm->ldr.LoadCount != -1) wm->ldr.LoadCount++;
        TRACE( "(%s) ldr.LoadCount: %d\n", debugstr_w(wm->ldr.BaseDllName.Buffer), wm->ldr.LoadCount );
    }
    else ret = STATUS_INVALID_PARAMETER;

    RtlLeaveCriticalSection( &loader_section );
    return ret;
}


/***********************************************************************
 *           LdrProcessRelocationBlock  (NTDLL.@)
 *
 * Apply relocations to a given page of a mapped PE image.
 */
IMAGE_BASE_RELOCATION * WINAPI LdrProcessRelocationBlock( void *page, UINT count,
                                                          USHORT *relocs, INT_PTR delta )
{
    while (count--)
    {
        USHORT offset = *relocs & 0xfff;
        int type = *relocs >> 12;
        switch(type)
        {
        case IMAGE_REL_BASED_ABSOLUTE:
            break;
        case IMAGE_REL_BASED_HIGH:
            *(short *)((char *)page + offset) += HIWORD(delta);
            break;
        case IMAGE_REL_BASED_LOW:
            *(short *)((char *)page + offset) += LOWORD(delta);
            break;
        case IMAGE_REL_BASED_HIGHLOW:
            *(int *)((char *)page + offset) += delta;
            break;
#ifdef __x86_64__
        case IMAGE_REL_BASED_DIR64:
            *(INT_PTR *)((char *)page + offset) += delta;
            break;
#elif defined(__arm__)
        case IMAGE_REL_BASED_THUMB_MOV32:
        {
            DWORD inst = *(INT_PTR *)((char *)page + offset);
            DWORD imm16 = ((inst << 1) & 0x0800) + ((inst << 12) & 0xf000) +
                          ((inst >> 20) & 0x0700) + ((inst >> 16) & 0x00ff);
            DWORD hi_delta;

            if ((inst & 0x8000fbf0) != 0x0000f240)
                ERR("wrong Thumb2 instruction %08x, expected MOVW\n", inst);

            imm16 += LOWORD(delta);
            hi_delta = HIWORD(delta) + HIWORD(imm16);
            *(INT_PTR *)((char *)page + offset) = (inst & 0x8f00fbf0) + ((imm16 >> 1) & 0x0400) +
                                                  ((imm16 >> 12) & 0x000f) +
                                                  ((imm16 << 20) & 0x70000000) +
                                                  ((imm16 << 16) & 0xff0000);

            if (hi_delta != 0)
            {
                inst = *(INT_PTR *)((char *)page + offset + 4);
                imm16 = ((inst << 1) & 0x0800) + ((inst << 12) & 0xf000) +
                        ((inst >> 20) & 0x0700) + ((inst >> 16) & 0x00ff);

                if ((inst & 0x8000fbf0) != 0x0000f2c0)
                    ERR("wrong Thumb2 instruction %08x, expected MOVT\n", inst);

                imm16 += hi_delta;
                if (imm16 > 0xffff)
                    ERR("resulting immediate value won't fit: %08x\n", imm16);
                *(INT_PTR *)((char *)page + offset + 4) = (inst & 0x8f00fbf0) +
                                                          ((imm16 >> 1) & 0x0400) +
                                                          ((imm16 >> 12) & 0x000f) +
                                                          ((imm16 << 20) & 0x70000000) +
                                                          ((imm16 << 16) & 0xff0000);
            }
        }
            break;
#endif
        default:
            FIXME("Unknown/unsupported fixup type %x.\n", type);
            return NULL;
        }
        relocs++;
    }
    return (IMAGE_BASE_RELOCATION *)relocs;  /* return address of next block */
}


/******************************************************************
 *		LdrQueryProcessModuleInformation
 *
 */
NTSTATUS WINAPI LdrQueryProcessModuleInformation(PSYSTEM_MODULE_INFORMATION smi, 
                                                 ULONG buf_size, ULONG* req_size)
{
    SYSTEM_MODULE*      sm = &smi->Modules[0];
    ULONG               size = sizeof(ULONG);
    NTSTATUS            nts = STATUS_SUCCESS;
    ANSI_STRING         str;
    char*               ptr;
    PLIST_ENTRY         mark, entry;
    PLDR_MODULE         mod;
    WORD id = 0;

    smi->ModulesCount = 0;

    RtlEnterCriticalSection( &loader_section );
    mark = &NtCurrentTeb()->Peb->LdrData->InLoadOrderModuleList;
    for (entry = mark->Flink; entry != mark; entry = entry->Flink)
    {
        mod = CONTAINING_RECORD(entry, LDR_MODULE, InLoadOrderModuleList);
        size += sizeof(*sm);
        if (size <= buf_size)
        {
            sm->Reserved1 = 0; /* FIXME */
            sm->Reserved2 = 0; /* FIXME */
            sm->ImageBaseAddress = mod->BaseAddress;
            sm->ImageSize = mod->SizeOfImage;
            sm->Flags = mod->Flags;
            sm->Id = id++;
            sm->Rank = 0; /* FIXME */
            sm->Unknown = 0; /* FIXME */
            str.Length = 0;
            str.MaximumLength = MAXIMUM_FILENAME_LENGTH;
            str.Buffer = (char*)sm->Name;
            RtlUnicodeStringToAnsiString(&str, &mod->FullDllName, FALSE);
            ptr = strrchr(str.Buffer, '\\');
            sm->NameOffset = (ptr != NULL) ? (ptr - str.Buffer + 1) : 0;

            smi->ModulesCount++;
            sm++;
        }
        else nts = STATUS_INFO_LENGTH_MISMATCH;
    }
    RtlLeaveCriticalSection( &loader_section );

    if (req_size) *req_size = size;

    return nts;
}


static NTSTATUS query_dword_option( HANDLE hkey, LPCWSTR name, ULONG *value )
{
    NTSTATUS status;
    UNICODE_STRING str;
    ULONG size;
    WCHAR buffer[64];
    KEY_VALUE_PARTIAL_INFORMATION *info = (KEY_VALUE_PARTIAL_INFORMATION *)buffer;

    RtlInitUnicodeString( &str, name );

    size = sizeof(buffer) - sizeof(WCHAR);
    if ((status = NtQueryValueKey( hkey, &str, KeyValuePartialInformation, buffer, size, &size )))
        return status;

    if (info->Type != REG_DWORD)
    {
        buffer[size / sizeof(WCHAR)] = 0;
        *value = strtoulW( (WCHAR *)info->Data, 0, 16 );
    }
    else memcpy( value, info->Data, sizeof(*value) );
    return status;
}

static NTSTATUS query_string_option( HANDLE hkey, LPCWSTR name, ULONG type,
                                     void *data, ULONG in_size, ULONG *out_size )
{
    NTSTATUS status;
    UNICODE_STRING str;
    ULONG size;
    char *buffer;
    KEY_VALUE_PARTIAL_INFORMATION *info;
    static const int info_size = FIELD_OFFSET( KEY_VALUE_PARTIAL_INFORMATION, Data );

    RtlInitUnicodeString( &str, name );

    size = info_size + in_size;
    if (!(buffer = RtlAllocateHeap( GetProcessHeap(), 0, size ))) return STATUS_NO_MEMORY;
    info = (KEY_VALUE_PARTIAL_INFORMATION *)buffer;
    status = NtQueryValueKey( hkey, &str, KeyValuePartialInformation, buffer, size, &size );
    if (!status || status == STATUS_BUFFER_OVERFLOW)
    {
        if (out_size) *out_size = info->DataLength;
        if (data && !status) memcpy( data, info->Data, info->DataLength );
    }
    RtlFreeHeap( GetProcessHeap(), 0, buffer );
    return status;
}


/******************************************************************
 *		LdrQueryImageFileExecutionOptions  (NTDLL.@)
 */
NTSTATUS WINAPI LdrQueryImageFileExecutionOptions( const UNICODE_STRING *key, LPCWSTR value, ULONG type,
                                                   void *data, ULONG in_size, ULONG *out_size )
{
    static const WCHAR optionsW[] = {'M','a','c','h','i','n','e','\\',
                                     'S','o','f','t','w','a','r','e','\\',
                                     'M','i','c','r','o','s','o','f','t','\\',
                                     'W','i','n','d','o','w','s',' ','N','T','\\',
                                     'C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\',
                                     'I','m','a','g','e',' ','F','i','l','e',' ',
                                     'E','x','e','c','u','t','i','o','n',' ','O','p','t','i','o','n','s','\\'};
    WCHAR path[MAX_PATH + sizeof(optionsW)/sizeof(WCHAR)];
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING name_str;
    HANDLE hkey;
    NTSTATUS status;
    ULONG len;
    WCHAR *p;

    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.ObjectName = &name_str;
    attr.Attributes = OBJ_CASE_INSENSITIVE;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;

    if ((p = memrchrW( key->Buffer, '\\', key->Length / sizeof(WCHAR) ))) p++;
    else p = key->Buffer;
    len = key->Length - (p - key->Buffer) * sizeof(WCHAR);
    name_str.Buffer = path;
    name_str.Length = sizeof(optionsW) + len;
    name_str.MaximumLength = name_str.Length;
    memcpy( path, optionsW, sizeof(optionsW) );
    memcpy( path + sizeof(optionsW)/sizeof(WCHAR), p, len );
    if ((status = NtOpenKey( &hkey, KEY_QUERY_VALUE, &attr ))) return status;

    if (type == REG_DWORD)
    {
        if (out_size) *out_size = sizeof(ULONG);
        if (in_size >= sizeof(ULONG)) status = query_dword_option( hkey, value, data );
        else status = STATUS_BUFFER_OVERFLOW;
    }
    else status = query_string_option( hkey, value, type, data, in_size, out_size );

    NtClose( hkey );
    return status;
}


/******************************************************************
 *		RtlDllShutdownInProgress  (NTDLL.@)
 */
BOOLEAN WINAPI RtlDllShutdownInProgress(void)
{
    return process_detaching;
}

/****************************************************************************
 *              LdrResolveDelayLoadedAPI   (NTDLL.@)
 */
void* WINAPI LdrResolveDelayLoadedAPI( void* base, const IMAGE_DELAYLOAD_DESCRIPTOR* desc,
                                       PDELAYLOAD_FAILURE_DLL_CALLBACK dllhook, void* syshook,
                                       IMAGE_THUNK_DATA* addr, ULONG flags )
{
    IMAGE_THUNK_DATA *pIAT, *pINT;
    DELAYLOAD_INFO delayinfo;
    UNICODE_STRING mod;
    const CHAR* name;
    HMODULE *phmod;
    NTSTATUS nts;
    FARPROC fp;
    DWORD id;

    FIXME("(%p, %p, %p, %p, %p, 0x%08x), partial stub\n", base, desc, dllhook, syshook, addr, flags);

    phmod = get_rva(base, desc->ModuleHandleRVA);
    pIAT = get_rva(base, desc->ImportAddressTableRVA);
    pINT = get_rva(base, desc->ImportNameTableRVA);
    name = get_rva(base, desc->DllNameRVA);
    id = addr - pIAT;

    if (!*phmod)
    {
        if (!RtlCreateUnicodeStringFromAsciiz(&mod, name))
        {
            nts = STATUS_NO_MEMORY;
            goto fail;
        }
        nts = LdrLoadDll(NULL, 0, &mod, phmod);
        RtlFreeUnicodeString(&mod);
        if (nts) goto fail;
    }

    if (IMAGE_SNAP_BY_ORDINAL(pINT[id].u1.Ordinal))
        nts = LdrGetProcedureAddress(*phmod, NULL, LOWORD(pINT[id].u1.Ordinal), (void**)&fp);
    else
    {
        const IMAGE_IMPORT_BY_NAME* iibn = get_rva(base, pINT[id].u1.AddressOfData);
        ANSI_STRING fnc;

        RtlInitAnsiString(&fnc, (char*)iibn->Name);
        nts = LdrGetProcedureAddress(*phmod, &fnc, 0, (void**)&fp);
    }
    if (!nts)
    {
        pIAT[id].u1.Function = (ULONG_PTR)fp;
        return fp;
    }

fail:
    delayinfo.Size = sizeof(delayinfo);
    delayinfo.DelayloadDescriptor = desc;
    delayinfo.ThunkAddress = addr;
    delayinfo.TargetDllName = name;
    delayinfo.TargetApiDescriptor.ImportDescribedByName = !IMAGE_SNAP_BY_ORDINAL(pINT[id].u1.Ordinal);
    delayinfo.TargetApiDescriptor.Description.Ordinal = LOWORD(pINT[id].u1.Ordinal);
    delayinfo.TargetModuleBase = *phmod;
    delayinfo.Unused = NULL;
    delayinfo.LastError = nts;
    return dllhook(4, &delayinfo);
}

/******************************************************************
 *		LdrShutdownProcess (NTDLL.@)
 *
 */
void WINAPI LdrShutdownProcess(void)
{
    TRACE("()\n");
    process_detaching = TRUE;
    process_detach();
}


/******************************************************************
 *		RtlExitUserProcess (NTDLL.@)
 */
void WINAPI RtlExitUserProcess( DWORD status )
{
    RtlEnterCriticalSection( &loader_section );
    RtlAcquirePebLock();
    NtTerminateProcess( 0, status );
    LdrShutdownProcess();
    NtTerminateProcess( GetCurrentProcess(), status );
    exit( status );
}

/******************************************************************
 *		LdrShutdownThread (NTDLL.@)
 *
 */
void WINAPI LdrShutdownThread(void)
{
    PLIST_ENTRY mark, entry;
    PLDR_MODULE mod;
    UINT i;
    void **pointers;

    TRACE("()\n");

    /* don't do any detach calls if process is exiting */
    if (process_detaching) return;

    RtlEnterCriticalSection( &loader_section );

    mark = &NtCurrentTeb()->Peb->LdrData->InInitializationOrderModuleList;
    for (entry = mark->Blink; entry != mark; entry = entry->Blink)
    {
        mod = CONTAINING_RECORD(entry, LDR_MODULE, 
                                InInitializationOrderModuleList);
        if ( !(mod->Flags & LDR_PROCESS_ATTACHED) )
            continue;
        if ( mod->Flags & LDR_NO_DLL_CALLS )
            continue;

        MODULE_InitDLL( CONTAINING_RECORD(mod, WINE_MODREF, ldr), 
                        DLL_THREAD_DETACH, NULL );
    }

    RtlAcquirePebLock();
    RemoveEntryList( &NtCurrentTeb()->TlsLinks );
    RtlReleasePebLock();

    if ((pointers = NtCurrentTeb()->ThreadLocalStoragePointer))
    {
        for (i = 0; i < tls_module_count; i++) RtlFreeHeap( GetProcessHeap(), 0, pointers[i] );
        RtlFreeHeap( GetProcessHeap(), 0, pointers );
    }
    RtlFreeHeap( GetProcessHeap(), 0, NtCurrentTeb()->FlsSlots );
    RtlFreeHeap( GetProcessHeap(), 0, NtCurrentTeb()->TlsExpansionSlots );
    RtlLeaveCriticalSection( &loader_section );
}


/***********************************************************************
 *           free_modref
 *
 */
static void free_modref( WINE_MODREF *wm )
{
    RemoveEntryList(&wm->ldr.InLoadOrderModuleList);
    RemoveEntryList(&wm->ldr.InMemoryOrderModuleList);
    if (wm->ldr.InInitializationOrderModuleList.Flink)
        RemoveEntryList(&wm->ldr.InInitializationOrderModuleList);

    TRACE(" unloading %s\n", debugstr_w(wm->ldr.FullDllName.Buffer));
    if (!TRACE_ON(module))
        TRACE_(loaddll)("Unloaded module %s : %s\n",
                        debugstr_w(wm->ldr.FullDllName.Buffer),
                        (wm->ldr.Flags & LDR_WINE_INTERNAL) ? "builtin" : "native" );

    SERVER_START_REQ( unload_dll )
    {
        req->base = wine_server_client_ptr( wm->ldr.BaseAddress );
        wine_server_call( req );
    }
    SERVER_END_REQ;

    free_tls_slot( &wm->ldr );
    RtlReleaseActivationContext( wm->ldr.ActivationContext );
    NtUnmapViewOfSection( NtCurrentProcess(), wm->ldr.BaseAddress );
    if (wm->ldr.Flags & LDR_WINE_INTERNAL) wine_dll_unload( wm->ldr.SectionHandle );
    if (cached_modref == wm) cached_modref = NULL;
    RtlFreeUnicodeString( &wm->ldr.FullDllName );
    RtlFreeHeap( GetProcessHeap(), 0, wm->deps );
    RtlFreeHeap( GetProcessHeap(), 0, wm );
}

/***********************************************************************
 *           MODULE_FlushModrefs
 *
 * Remove all unused modrefs and call the internal unloading routines
 * for the library type.
 *
 * The loader_section must be locked while calling this function.
 */
static void MODULE_FlushModrefs(void)
{
    PLIST_ENTRY mark, entry, prev;
    PLDR_MODULE mod;
    WINE_MODREF*wm;

    mark = &NtCurrentTeb()->Peb->LdrData->InInitializationOrderModuleList;
    for (entry = mark->Blink; entry != mark; entry = prev)
    {
        mod = CONTAINING_RECORD(entry, LDR_MODULE, InInitializationOrderModuleList);
        wm = CONTAINING_RECORD(mod, WINE_MODREF, ldr);
        prev = entry->Blink;
        if (!mod->LoadCount) free_modref( wm );
    }

    /* check load order list too for modules that haven't been initialized yet */
    mark = &NtCurrentTeb()->Peb->LdrData->InLoadOrderModuleList;
    for (entry = mark->Blink; entry != mark; entry = prev)
    {
        mod = CONTAINING_RECORD(entry, LDR_MODULE, InLoadOrderModuleList);
        wm = CONTAINING_RECORD(mod, WINE_MODREF, ldr);
        prev = entry->Blink;
        if (!mod->LoadCount) free_modref( wm );
    }
}

/***********************************************************************
 *           MODULE_DecRefCount
 *
 * The loader_section must be locked while calling this function.
 */
static void MODULE_DecRefCount( WINE_MODREF *wm )
{
    int i;

    if ( wm->ldr.Flags & LDR_UNLOAD_IN_PROGRESS )
        return;

    if ( wm->ldr.LoadCount <= 0 )
        return;

    --wm->ldr.LoadCount;
    TRACE("(%s) ldr.LoadCount: %d\n", debugstr_w(wm->ldr.BaseDllName.Buffer), wm->ldr.LoadCount );

    if ( wm->ldr.LoadCount == 0 )
    {
        wm->ldr.Flags |= LDR_UNLOAD_IN_PROGRESS;

        for ( i = 0; i < wm->nDeps; i++ )
            if ( wm->deps[i] )
                MODULE_DecRefCount( wm->deps[i] );

        wm->ldr.Flags &= ~LDR_UNLOAD_IN_PROGRESS;
    }
}

/******************************************************************
 *		LdrUnloadDll (NTDLL.@)
 *
 *
 */
NTSTATUS WINAPI LdrUnloadDll( HMODULE hModule )
{
    WINE_MODREF *wm;
    NTSTATUS retv = STATUS_SUCCESS;

    if (process_detaching) return retv;

    TRACE("(%p)\n", hModule);

    RtlEnterCriticalSection( &loader_section );

    free_lib_count++;
    if ((wm = get_modref( hModule )) != NULL)
    {
        TRACE("(%s) - START\n", debugstr_w(wm->ldr.BaseDllName.Buffer));

        /* Recursively decrement reference counts */
        MODULE_DecRefCount( wm );

        /* Call process detach notifications */
        if ( free_lib_count <= 1 )
        {
            process_detach();
            MODULE_FlushModrefs();
        }

        TRACE("END\n");
    }
    else
        retv = STATUS_DLL_NOT_FOUND;

    free_lib_count--;

    RtlLeaveCriticalSection( &loader_section );

    return retv;
}

/***********************************************************************
 *           RtlImageNtHeader   (NTDLL.@)
 */
PIMAGE_NT_HEADERS WINAPI RtlImageNtHeader(HMODULE hModule)
{
    IMAGE_NT_HEADERS *ret;

    __TRY
    {
        IMAGE_DOS_HEADER *dos = (IMAGE_DOS_HEADER *)hModule;

        ret = NULL;
        if (dos->e_magic == IMAGE_DOS_SIGNATURE)
        {
            ret = (IMAGE_NT_HEADERS *)((char *)dos + dos->e_lfanew);
            if (ret->Signature != IMAGE_NT_SIGNATURE) ret = NULL;
        }
    }
    __EXCEPT_PAGE_FAULT
    {
        return NULL;
    }
    __ENDTRY
    return ret;
}


/***********************************************************************
 *           attach_process_dlls
 *
 * Initial attach to all the dlls loaded by the process.
 */
static NTSTATUS attach_process_dlls( void *wm )
{
    NTSTATUS status;

    pthread_sigmask( SIG_UNBLOCK, &server_block_set, NULL );

    RtlEnterCriticalSection( &loader_section );
    if ((status = process_attach( wm, (LPVOID)1 )) != STATUS_SUCCESS)
    {
        if (last_failed_modref)
            ERR( "%s failed to initialize, aborting\n",
                 debugstr_w(last_failed_modref->ldr.BaseDllName.Buffer) + 1 );
        return status;
    }
    attach_implicitly_loaded_dlls( (LPVOID)1 );
    RtlLeaveCriticalSection( &loader_section );
    return status;
}


/***********************************************************************
 *           load_global_options
 */
static void load_global_options(void)
{
    static const WCHAR sessionW[] = {'M','a','c','h','i','n','e','\\',
                                     'S','y','s','t','e','m','\\',
                                     'C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t','\\',
                                     'C','o','n','t','r','o','l','\\',
                                     'S','e','s','s','i','o','n',' ','M','a','n','a','g','e','r',0};
    static const WCHAR globalflagW[] = {'G','l','o','b','a','l','F','l','a','g',0};
    static const WCHAR critsectW[] = {'C','r','i','t','i','c','a','l','S','e','c','t','i','o','n','T','i','m','e','o','u','t',0};
    static const WCHAR heapresW[] = {'H','e','a','p','S','e','g','m','e','n','t','R','e','s','e','r','v','e',0};
    static const WCHAR heapcommitW[] = {'H','e','a','p','S','e','g','m','e','n','t','C','o','m','m','i','t',0};
    static const WCHAR decommittotalW[] = {'H','e','a','p','D','e','C','o','m','m','i','t','T','o','t','a','l','F','r','e','e','T','h','r','e','s','h','o','l','d',0};
    static const WCHAR decommitfreeW[] = {'H','e','a','p','D','e','C','o','m','m','i','t','F','r','e','e','B','l','o','c','k','T','h','r','e','s','h','o','l','d',0};

    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING name_str;
    HANDLE hkey;
    ULONG value;

    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.ObjectName = &name_str;
    attr.Attributes = OBJ_CASE_INSENSITIVE;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;
    RtlInitUnicodeString( &name_str, sessionW );

    if (NtOpenKey( &hkey, KEY_QUERY_VALUE, &attr )) return;

    query_dword_option( hkey, globalflagW, &NtCurrentTeb()->Peb->NtGlobalFlag );

    query_dword_option( hkey, critsectW, &value );
    NtCurrentTeb()->Peb->CriticalSectionTimeout.QuadPart = (ULONGLONG)value * -10000000;

    query_dword_option( hkey, heapresW, &value );
    NtCurrentTeb()->Peb->HeapSegmentReserve = value;

    query_dword_option( hkey, heapcommitW, &value );
    NtCurrentTeb()->Peb->HeapSegmentCommit = value;

    query_dword_option( hkey, decommittotalW, &value );
    NtCurrentTeb()->Peb->HeapDeCommitTotalFreeThreshold = value;

    query_dword_option( hkey, decommitfreeW, &value );
    NtCurrentTeb()->Peb->HeapDeCommitFreeBlockThreshold = value;

    NtClose( hkey );
}


/***********************************************************************
 *           start_process
 */
static void start_process( void *kernel_start )
{
    call_thread_entry_point( kernel_start, NtCurrentTeb()->Peb );
}

/******************************************************************
 *		LdrInitializeThunk (NTDLL.@)
 *
 */
void WINAPI LdrInitializeThunk( void *kernel_start, ULONG_PTR unknown2,
                                ULONG_PTR unknown3, ULONG_PTR unknown4 )
{
    static const WCHAR globalflagW[] = {'G','l','o','b','a','l','F','l','a','g',0};
    NTSTATUS status;
    WINE_MODREF *wm;
    LPCWSTR load_path;
    PEB *peb = NtCurrentTeb()->Peb;

    if (main_exe_file) NtClose( main_exe_file );  /* at this point the main module is created */

    /* allocate the modref for the main exe (if not already done) */
    wm = get_modref( peb->ImageBaseAddress );
    assert( wm );
    if (wm->ldr.Flags & LDR_IMAGE_IS_DLL)
    {
        ERR("%s is a dll, not an executable\n", debugstr_w(wm->ldr.FullDllName.Buffer) );
        exit(1);
    }

    peb->LoaderLock = &loader_section;
    peb->ProcessParameters->ImagePathName = wm->ldr.FullDllName;
    if (!peb->ProcessParameters->WindowTitle.Buffer)
        peb->ProcessParameters->WindowTitle = wm->ldr.FullDllName;
    version_init( wm->ldr.FullDllName.Buffer );
    virtual_set_large_address_space();

    LdrQueryImageFileExecutionOptions( &peb->ProcessParameters->ImagePathName, globalflagW,
                                       REG_DWORD, &peb->NtGlobalFlag, sizeof(peb->NtGlobalFlag), NULL );

    /* the main exe needs to be the first in the load order list */
    RemoveEntryList( &wm->ldr.InLoadOrderModuleList );
    InsertHeadList( &peb->LdrData->InLoadOrderModuleList, &wm->ldr.InLoadOrderModuleList );

    if ((status = virtual_alloc_thread_stack( NtCurrentTeb(), 0, 0 )) != STATUS_SUCCESS) goto error;
    if ((status = server_init_process_done()) != STATUS_SUCCESS) goto error;

    actctx_init();
    load_path = NtCurrentTeb()->Peb->ProcessParameters->DllPath.Buffer;
    if ((status = fixup_imports( wm, load_path )) != STATUS_SUCCESS) goto error;
    heap_set_debug_flags( GetProcessHeap() );

    status = wine_call_on_stack( attach_process_dlls, wm, NtCurrentTeb()->Tib.StackBase );
    if (status != STATUS_SUCCESS) goto error;

    virtual_release_address_space();
    virtual_clear_thread_stack();
    wine_switch_to_stack( start_process, kernel_start, NtCurrentTeb()->Tib.StackBase );

error:
    ERR( "Main exe initialization for %s failed, status %x\n",
         debugstr_w(peb->ProcessParameters->ImagePathName.Buffer), status );
    NtTerminateProcess( GetCurrentProcess(), status );
}


/***********************************************************************
 *           RtlImageDirectoryEntryToData   (NTDLL.@)
 */
PVOID WINAPI RtlImageDirectoryEntryToData( HMODULE module, BOOL image, WORD dir, ULONG *size )
{
    const IMAGE_NT_HEADERS *nt;
    DWORD addr;

    if ((ULONG_PTR)module & 1)  /* mapped as data file */
    {
        module = (HMODULE)((ULONG_PTR)module & ~1);
        image = FALSE;
    }
    if (!(nt = RtlImageNtHeader( module ))) return NULL;
    if (nt->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
    {
        const IMAGE_NT_HEADERS64 *nt64 = (const IMAGE_NT_HEADERS64 *)nt;

        if (dir >= nt64->OptionalHeader.NumberOfRvaAndSizes) return NULL;
        if (!(addr = nt64->OptionalHeader.DataDirectory[dir].VirtualAddress)) return NULL;
        *size = nt64->OptionalHeader.DataDirectory[dir].Size;
        if (image || addr < nt64->OptionalHeader.SizeOfHeaders) return (char *)module + addr;
    }
    else if (nt->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
    {
        const IMAGE_NT_HEADERS32 *nt32 = (const IMAGE_NT_HEADERS32 *)nt;

        if (dir >= nt32->OptionalHeader.NumberOfRvaAndSizes) return NULL;
        if (!(addr = nt32->OptionalHeader.DataDirectory[dir].VirtualAddress)) return NULL;
        *size = nt32->OptionalHeader.DataDirectory[dir].Size;
        if (image || addr < nt32->OptionalHeader.SizeOfHeaders) return (char *)module + addr;
    }
    else return NULL;

    /* not mapped as image, need to find the section containing the virtual address */
    return RtlImageRvaToVa( nt, module, addr, NULL );
}


/***********************************************************************
 *           RtlImageRvaToSection   (NTDLL.@)
 */
PIMAGE_SECTION_HEADER WINAPI RtlImageRvaToSection( const IMAGE_NT_HEADERS *nt,
                                                   HMODULE module, DWORD rva )
{
    int i;
    const IMAGE_SECTION_HEADER *sec;

    sec = (const IMAGE_SECTION_HEADER*)((const char*)&nt->OptionalHeader +
                                        nt->FileHeader.SizeOfOptionalHeader);
    for (i = 0; i < nt->FileHeader.NumberOfSections; i++, sec++)
    {
        if ((sec->VirtualAddress <= rva) && (sec->VirtualAddress + sec->SizeOfRawData > rva))
            return (PIMAGE_SECTION_HEADER)sec;
    }
    return NULL;
}


/***********************************************************************
 *           RtlImageRvaToVa   (NTDLL.@)
 */
PVOID WINAPI RtlImageRvaToVa( const IMAGE_NT_HEADERS *nt, HMODULE module,
                              DWORD rva, IMAGE_SECTION_HEADER **section )
{
    IMAGE_SECTION_HEADER *sec;

    if (section && *section)  /* try this section first */
    {
        sec = *section;
        if ((sec->VirtualAddress <= rva) && (sec->VirtualAddress + sec->SizeOfRawData > rva))
            goto found;
    }
    if (!(sec = RtlImageRvaToSection( nt, module, rva ))) return NULL;
 found:
    if (section) *section = sec;
    return (char *)module + sec->PointerToRawData + (rva - sec->VirtualAddress);
}


/***********************************************************************
 *           RtlPcToFileHeader   (NTDLL.@)
 */
PVOID WINAPI RtlPcToFileHeader( PVOID pc, PVOID *address )
{
    LDR_MODULE *module;
    PVOID ret = NULL;

    RtlEnterCriticalSection( &loader_section );
    if (!LdrFindEntryForAddress( pc, &module )) ret = module->BaseAddress;
    RtlLeaveCriticalSection( &loader_section );
    *address = ret;
    return ret;
}


/***********************************************************************
 *           NtLoadDriver   (NTDLL.@)
 *           ZwLoadDriver   (NTDLL.@)
 */
NTSTATUS WINAPI NtLoadDriver( const UNICODE_STRING *DriverServiceName )
{
    FIXME("(%p), stub!\n",DriverServiceName);
    return STATUS_NOT_IMPLEMENTED;
}


/***********************************************************************
 *           NtUnloadDriver   (NTDLL.@)
 *           ZwUnloadDriver   (NTDLL.@)
 */
NTSTATUS WINAPI NtUnloadDriver( const UNICODE_STRING *DriverServiceName )
{
    FIXME("(%p), stub!\n",DriverServiceName);
    return STATUS_NOT_IMPLEMENTED;
}


/******************************************************************
 *		DllMain   (NTDLL.@)
 */
BOOL WINAPI DllMain( HINSTANCE inst, DWORD reason, LPVOID reserved )
{
    if (reason == DLL_PROCESS_ATTACH) LdrDisableThreadCalloutsForDll( inst );
    return TRUE;
}


/******************************************************************
 *		__wine_init_windows_dir   (NTDLL.@)
 *
 * Windows and system dir initialization once kernel32 has been loaded.
 */
void CDECL __wine_init_windows_dir( const WCHAR *windir, const WCHAR *sysdir )
{
    PLIST_ENTRY mark, entry;
    LPWSTR buffer, p;

    strcpyW( user_shared_data->NtSystemRoot, windir );
    DIR_init_windows_dir( windir, sysdir );

    /* prepend the system dir to the name of the already created modules */
    mark = &NtCurrentTeb()->Peb->LdrData->InLoadOrderModuleList;
    for (entry = mark->Flink; entry != mark; entry = entry->Flink)
    {
        LDR_MODULE *mod = CONTAINING_RECORD( entry, LDR_MODULE, InLoadOrderModuleList );

        assert( mod->Flags & LDR_WINE_INTERNAL );

        buffer = RtlAllocateHeap( GetProcessHeap(), 0,
                                  system_dir.Length + mod->FullDllName.Length + 2*sizeof(WCHAR) );
        if (!buffer) continue;
        strcpyW( buffer, system_dir.Buffer );
        p = buffer + strlenW( buffer );
        if (p > buffer && p[-1] != '\\') *p++ = '\\';
        strcpyW( p, mod->FullDllName.Buffer );
        RtlInitUnicodeString( &mod->FullDllName, buffer );
        RtlInitUnicodeString( &mod->BaseDllName, p );
    }
}


/***********************************************************************
 *           __wine_process_init
 */
void __wine_process_init(void)
{
    static const WCHAR kernel32W[] = {'k','e','r','n','e','l','3','2','.','d','l','l',0};

    WINE_MODREF *wm;
    NTSTATUS status;
    ANSI_STRING func_name;
    void (* DECLSPEC_NORETURN CDECL init_func)(void);

    main_exe_file = thread_init();

    /* retrieve current umask */
    FILE_umask = umask(0777);
    umask( FILE_umask );

    load_global_options();

    /* setup the load callback and create ntdll modref */
    wine_dll_set_callback( load_builtin_callback );

    if ((status = load_builtin_dll( NULL, kernel32W, 0, 0, &wm )) != STATUS_SUCCESS)
    {
        MESSAGE( "wine: could not load kernel32.dll, status %x\n", status );
        exit(1);
    }
    RtlInitAnsiString( &func_name, "UnhandledExceptionFilter" );
    LdrGetProcedureAddress( wm->ldr.BaseAddress, &func_name, 0, (void **)&unhandled_exception_filter );

    RtlInitAnsiString( &func_name, "__wine_kernel_init" );
    if ((status = LdrGetProcedureAddress( wm->ldr.BaseAddress, &func_name,
                                          0, (void **)&init_func )) != STATUS_SUCCESS)
    {
        MESSAGE( "wine: could not find __wine_kernel_init in kernel32.dll, status %x\n", status );
        exit(1);
    }
    init_func();
}
