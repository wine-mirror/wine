/*
 * Unix interface for loader functions
 *
 * Copyright (C) 2020 Alexandre Julliard
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

#if 0
#pragma makedep unix
#endif

#include "config.h"
#include "wine/port.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#ifdef HAVE_SYS_MMAN_H
# include <sys/mman.h>
#endif
#ifdef __APPLE__
# include <CoreFoundation/CoreFoundation.h>
# define LoadResource MacLoadResource
# define GetCurrentThread MacGetCurrentThread
# include <CoreServices/CoreServices.h>
# undef LoadResource
# undef GetCurrentThread
# include <pthread.h>
# include <mach-o/getsect.h>
#endif

#include "ntstatus.h"
#define WIN32_NO_STATUS
#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "windef.h"
#include "winnt.h"
#include "winternl.h"
#include "unixlib.h"
#include "wine/library.h"

extern IMAGE_NT_HEADERS __wine_spec_nt_header;
extern void CDECL __wine_set_unix_funcs( int version, const struct unix_funcs *funcs );

static inline void *get_rva( const IMAGE_NT_HEADERS *nt, ULONG_PTR addr )
{
    return (BYTE *)nt + addr;
}

/* adjust an array of pointers to make them into RVAs */
static inline void fixup_rva_ptrs( void *array, BYTE *base, unsigned int count )
{
    BYTE **src = array;
    DWORD *dst = array;

    for ( ; count; count--, src++, dst++) *dst = *src ? *src - base : 0;
}

/* fixup an array of RVAs by adding the specified delta */
static inline void fixup_rva_dwords( DWORD *ptr, int delta, unsigned int count )
{
    for ( ; count; count--, ptr++) if (*ptr) *ptr += delta;
}


/* fixup an array of name/ordinal RVAs by adding the specified delta */
static inline void fixup_rva_names( UINT_PTR *ptr, int delta )
{
    for ( ; *ptr; ptr++) if (!(*ptr & IMAGE_ORDINAL_FLAG)) *ptr += delta;
}


/* fixup RVAs in the resource directory */
static void fixup_so_resources( IMAGE_RESOURCE_DIRECTORY *dir, BYTE *root, int delta )
{
    IMAGE_RESOURCE_DIRECTORY_ENTRY *entry = (IMAGE_RESOURCE_DIRECTORY_ENTRY *)(dir + 1);
    unsigned int i;

    for (i = 0; i < dir->NumberOfNamedEntries + dir->NumberOfIdEntries; i++, entry++)
    {
        void *ptr = root + entry->u2.s2.OffsetToDirectory;
        if (entry->u2.s2.DataIsDirectory) fixup_so_resources( ptr, root, delta );
        else fixup_rva_dwords( &((IMAGE_RESOURCE_DATA_ENTRY *)ptr)->OffsetToData, delta, 1 );
    }
}


/*************************************************************************
 *		map_so_dll
 *
 * Map a builtin dll in memory and fixup RVAs.
 */
static NTSTATUS CDECL map_so_dll( const IMAGE_NT_HEADERS *nt_descr, HMODULE module )
{
    static const char builtin_signature[32] = "Wine builtin DLL";
    IMAGE_DATA_DIRECTORY *dir;
    IMAGE_DOS_HEADER *dos;
    IMAGE_NT_HEADERS *nt;
    IMAGE_SECTION_HEADER *sec;
    BYTE *addr = (BYTE *)module;
    DWORD code_start, code_end, data_start, data_end, align_mask;
    int delta, nb_sections = 2;  /* code + data */
    unsigned int i;
    DWORD size = (sizeof(IMAGE_DOS_HEADER)
                  + sizeof(builtin_signature)
                  + sizeof(IMAGE_NT_HEADERS)
                  + nb_sections * sizeof(IMAGE_SECTION_HEADER));

    if (wine_anon_mmap( addr, size, PROT_READ | PROT_WRITE, MAP_FIXED ) != addr) return STATUS_NO_MEMORY;

    dos = (IMAGE_DOS_HEADER *)addr;
    nt  = (IMAGE_NT_HEADERS *)((BYTE *)(dos + 1) + sizeof(builtin_signature));
    sec = (IMAGE_SECTION_HEADER *)(nt + 1);

    /* build the DOS and NT headers */

    dos->e_magic    = IMAGE_DOS_SIGNATURE;
    dos->e_cblp     = 0x90;
    dos->e_cp       = 3;
    dos->e_cparhdr  = (sizeof(*dos) + 0xf) / 0x10;
    dos->e_minalloc = 0;
    dos->e_maxalloc = 0xffff;
    dos->e_ss       = 0x0000;
    dos->e_sp       = 0x00b8;
    dos->e_lfanew   = sizeof(*dos) + sizeof(builtin_signature);

    *nt = *nt_descr;

    delta      = (const BYTE *)nt_descr - addr;
    align_mask = nt->OptionalHeader.SectionAlignment - 1;
    code_start = (size + align_mask) & ~align_mask;
    data_start = delta & ~align_mask;
#ifdef __APPLE__
    {
        Dl_info dli;
        unsigned long data_size;
        /* need the mach_header, not the PE header, to give to getsegmentdata(3) */
        dladdr(addr, &dli);
        code_end   = getsegmentdata(dli.dli_fbase, "__DATA", &data_size) - addr;
        data_end   = (code_end + data_size + align_mask) & ~align_mask;
    }
#else
    code_end   = data_start;
    data_end   = (nt->OptionalHeader.SizeOfImage + delta + align_mask) & ~align_mask;
#endif

    fixup_rva_ptrs( &nt->OptionalHeader.AddressOfEntryPoint, addr, 1 );

    nt->FileHeader.NumberOfSections                = nb_sections;
    nt->OptionalHeader.BaseOfCode                  = code_start;
#ifndef _WIN64
    nt->OptionalHeader.BaseOfData                  = data_start;
#endif
    nt->OptionalHeader.SizeOfCode                  = code_end - code_start;
    nt->OptionalHeader.SizeOfInitializedData       = data_end - data_start;
    nt->OptionalHeader.SizeOfUninitializedData     = 0;
    nt->OptionalHeader.SizeOfImage                 = data_end;
    nt->OptionalHeader.ImageBase                   = (ULONG_PTR)addr;

    /* build the code section */

    memcpy( sec->Name, ".text", sizeof(".text") );
    sec->SizeOfRawData = code_end - code_start;
    sec->Misc.VirtualSize = sec->SizeOfRawData;
    sec->VirtualAddress   = code_start;
    sec->PointerToRawData = code_start;
    sec->Characteristics  = (IMAGE_SCN_CNT_CODE | IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ);
    sec++;

    /* build the data section */

    memcpy( sec->Name, ".data", sizeof(".data") );
    sec->SizeOfRawData = data_end - data_start;
    sec->Misc.VirtualSize = sec->SizeOfRawData;
    sec->VirtualAddress   = data_start;
    sec->PointerToRawData = data_start;
    sec->Characteristics  = (IMAGE_SCN_CNT_INITIALIZED_DATA |
                             IMAGE_SCN_MEM_WRITE | IMAGE_SCN_MEM_READ);
    sec++;

    for (i = 0; i < nt->OptionalHeader.NumberOfRvaAndSizes; i++)
        fixup_rva_dwords( &nt->OptionalHeader.DataDirectory[i].VirtualAddress, delta, 1 );

    /* build the import directory */

    dir = &nt->OptionalHeader.DataDirectory[IMAGE_FILE_IMPORT_DIRECTORY];
    if (dir->Size)
    {
        IMAGE_IMPORT_DESCRIPTOR *imports = (IMAGE_IMPORT_DESCRIPTOR *)(addr + dir->VirtualAddress);

        while (imports->Name)
        {
            fixup_rva_dwords( &imports->u.OriginalFirstThunk, delta, 1 );
            fixup_rva_dwords( &imports->Name, delta, 1 );
            fixup_rva_dwords( &imports->FirstThunk, delta, 1 );
            if (imports->u.OriginalFirstThunk)
                fixup_rva_names( (UINT_PTR *)(addr + imports->u.OriginalFirstThunk), delta );
            if (imports->FirstThunk)
                fixup_rva_names( (UINT_PTR *)(addr + imports->FirstThunk), delta );
            imports++;
        }
    }

    /* build the resource directory */

    dir = &nt->OptionalHeader.DataDirectory[IMAGE_FILE_RESOURCE_DIRECTORY];
    if (dir->Size)
    {
        void *ptr = addr + dir->VirtualAddress;
        fixup_so_resources( ptr, ptr, delta );
    }

    /* build the export directory */

    dir = &nt->OptionalHeader.DataDirectory[IMAGE_FILE_EXPORT_DIRECTORY];
    if (dir->Size)
    {
        IMAGE_EXPORT_DIRECTORY *exports = (IMAGE_EXPORT_DIRECTORY *)(addr + dir->VirtualAddress);

        fixup_rva_dwords( &exports->Name, delta, 1 );
        fixup_rva_dwords( &exports->AddressOfFunctions, delta, 1 );
        fixup_rva_dwords( &exports->AddressOfNames, delta, 1 );
        fixup_rva_dwords( &exports->AddressOfNameOrdinals, delta, 1 );
        fixup_rva_dwords( (DWORD *)(addr + exports->AddressOfNames), delta, exports->NumberOfNames );
        fixup_rva_ptrs( addr + exports->AddressOfFunctions, addr, exports->NumberOfFunctions );
    }
    return STATUS_SUCCESS;
}

static const IMAGE_EXPORT_DIRECTORY *get_export_dir( HMODULE module )
{
    const IMAGE_DOS_HEADER *dos = (const IMAGE_DOS_HEADER *)module;
    const IMAGE_NT_HEADERS *nt;
    DWORD addr;

    if (dos->e_magic != IMAGE_DOS_SIGNATURE) return NULL;
    nt = (IMAGE_NT_HEADERS *)((const BYTE *)dos + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE) return NULL;
    addr = nt->OptionalHeader.DataDirectory[IMAGE_FILE_EXPORT_DIRECTORY].VirtualAddress;
    if (!addr) return NULL;
    return (IMAGE_EXPORT_DIRECTORY *)((BYTE *)module + addr);
}

static ULONG_PTR find_ordinal_export( HMODULE module, const IMAGE_EXPORT_DIRECTORY *exports, DWORD ordinal )
{
    const DWORD *functions = (const DWORD *)((BYTE *)module + exports->AddressOfFunctions);

    if (ordinal >= exports->NumberOfFunctions) return 0;
    if (!functions[ordinal]) return 0;
    return (ULONG_PTR)module + functions[ordinal];
}

static ULONG_PTR find_named_export( HMODULE module, const IMAGE_EXPORT_DIRECTORY *exports,
                                    const IMAGE_IMPORT_BY_NAME *name )
{
    const WORD *ordinals = (const WORD *)((BYTE *)module + exports->AddressOfNameOrdinals);
    const DWORD *names = (const DWORD *)((BYTE *)module + exports->AddressOfNames);
    int min = 0, max = exports->NumberOfNames - 1;

    /* first check the hint */
    if (name->Hint <= max)
    {
        char *ename = (char *)module + names[name->Hint];
        if (!strcmp( ename, (char *)name->Name ))
            return find_ordinal_export( module, exports, ordinals[name->Hint] );
    }

    /* then do a binary search */
    while (min <= max)
    {
        int res, pos = (min + max) / 2;
        char *ename = (char *)module + names[pos];
        if (!(res = strcmp( ename, (char *)name->Name )))
            return find_ordinal_export( module, exports, ordinals[pos] );
        if (res > 0) max = pos - 1;
        else min = pos + 1;
    }
    return 0;
}

static void fixup_ntdll_imports( const IMAGE_NT_HEADERS *nt, HMODULE ntdll_module )
{
    const IMAGE_EXPORT_DIRECTORY *ntdll_exports = get_export_dir( ntdll_module );
    const IMAGE_IMPORT_DESCRIPTOR *descr;
    const IMAGE_THUNK_DATA *import_list;
    IMAGE_THUNK_DATA *thunk_list;

    assert( ntdll_exports );

    descr = get_rva( nt, nt->OptionalHeader.DataDirectory[IMAGE_FILE_IMPORT_DIRECTORY].VirtualAddress );
    while (descr->Name)
    {
        /* ntdll must be the only import */
        assert( !strcmp( get_rva( nt, descr->Name ), "ntdll.dll" ));

        thunk_list = get_rva( nt, (DWORD)descr->FirstThunk );
        if (descr->u.OriginalFirstThunk)
            import_list = get_rva( nt, (DWORD)descr->u.OriginalFirstThunk );
        else
            import_list = thunk_list;


        while (import_list->u1.Ordinal)
        {
            if (IMAGE_SNAP_BY_ORDINAL( import_list->u1.Ordinal ))
            {
                int ordinal = IMAGE_ORDINAL( import_list->u1.Ordinal ) - ntdll_exports->Base;
                thunk_list->u1.Function = find_ordinal_export( ntdll_module, ntdll_exports, ordinal );
                if (!thunk_list->u1.Function) fprintf( stderr, "ntdll: ordinal %u not found\n", ordinal );
            }
            else  /* import by name */
            {
                IMAGE_IMPORT_BY_NAME *pe_name = get_rva( nt, import_list->u1.AddressOfData );
                thunk_list->u1.Function = find_named_export( ntdll_module, ntdll_exports, pe_name );
                if (!thunk_list->u1.Function) fprintf( stderr, "ntdll: %s not found\n", pe_name->Name );
            }
            import_list++;
            thunk_list++;
        }

        descr++;
    }
}

/***********************************************************************
 *           load_ntdll
 */
static HMODULE load_ntdll(void)
{
    const IMAGE_NT_HEADERS *nt;
    HMODULE module;
    Dl_info info;
    char *name;
    void *handle;

    if (!dladdr( load_ntdll, &info ))
    {
        fprintf( stderr, "cannot get path to ntdll.so\n" );
        exit(1);
    }
    name = malloc( strlen(info.dli_fname) + 5 );
    strcpy( name, info.dli_fname );
    strcpy( name + strlen(info.dli_fname) - 3, ".dll.so" );
    if (!(handle = dlopen( name, RTLD_NOW )))
    {
        fprintf( stderr, "failed to load %s: %s\n", name, dlerror() );
        exit(1);
    }
    if (!(nt = dlsym( handle, "__wine_spec_nt_header" )))
    {
        fprintf( stderr, "NT header not found in %s (too old?)\n", name );
        exit(1);
    }
    free( name );
    module = (HMODULE)((nt->OptionalHeader.ImageBase + 0xffff) & ~0xffff);
    map_so_dll( nt, module );
    return module;
}


/***********************************************************************
 *           unix_funcs
 */
static struct unix_funcs unix_funcs =
{
    map_so_dll,
};


#ifdef __APPLE__
struct apple_stack_info
{
    void *stack;
    size_t desired_size;
};

static void *apple_wine_thread( void *arg )
{
    __wine_set_unix_funcs( NTDLL_UNIXLIB_VERSION, &unix_funcs );
    return NULL;
}

/***********************************************************************
 *           apple_alloc_thread_stack
 *
 * Callback for wine_mmap_enum_reserved_areas to allocate space for
 * the secondary thread's stack.
 */
#ifndef _WIN64
static int apple_alloc_thread_stack( void *base, size_t size, void *arg )
{
    struct apple_stack_info *info = arg;

    /* For mysterious reasons, putting the thread stack at the very top
     * of the address space causes subsequent execs to fail, even on the
     * child side of a fork.  Avoid the top 16MB. */
    char * const limit = (char*)0xff000000;
    if ((char *)base >= limit) return 0;
    if (size > limit - (char*)base)
        size = limit - (char*)base;
    if (size < info->desired_size) return 0;
    info->stack = wine_anon_mmap( (char *)base + size - info->desired_size,
                                  info->desired_size, PROT_READ|PROT_WRITE, MAP_FIXED );
    return (info->stack != (void *)-1);
}
#endif

/***********************************************************************
 *           apple_create_wine_thread
 *
 * Spin off a secondary thread to complete Wine initialization, leaving
 * the original thread for the Mac frameworks.
 *
 * Invoked as a CFRunLoopSource perform callback.
 */
static void apple_create_wine_thread( void *arg )
{
    int success = 0;
    pthread_t thread;
    pthread_attr_t attr;

    if (!pthread_attr_init( &attr ))
    {
#ifndef _WIN64
        struct apple_stack_info info;

        /* Try to put the new thread's stack in the reserved area.  If this
         * fails, just let it go wherever.  It'll be a waste of space, but we
         * can go on. */
        if (!pthread_attr_getstacksize( &attr, &info.desired_size ) &&
            wine_mmap_enum_reserved_areas( apple_alloc_thread_stack, &info, 1 ))
        {
            wine_mmap_remove_reserved_area( info.stack, info.desired_size, 0 );
            pthread_attr_setstackaddr( &attr, (char*)info.stack + info.desired_size );
        }
#endif

        if (!pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_JOINABLE ) &&
            !pthread_create( &thread, &attr, apple_wine_thread, NULL ))
            success = 1;

        pthread_attr_destroy( &attr );
    }
    if (!success) exit(1);
}


/***********************************************************************
 *           apple_main_thread
 *
 * Park the process's original thread in a Core Foundation run loop for
 * use by the Mac frameworks, especially receiving and handling
 * distributed notifications.  Spin off a new thread for the rest of the
 * Wine initialization.
 */
static void apple_main_thread(void)
{
    CFRunLoopSourceContext source_context = { 0 };
    CFRunLoopSourceRef source;

    if (!pthread_main_np()) return;

    /* Multi-processing Services can get confused about the main thread if the
     * first time it's used is on a secondary thread.  Use it here to make sure
     * that doesn't happen. */
    MPTaskIsPreemptive(MPCurrentTaskID());

    /* Give ourselves the best chance of having the distributed notification
     * center scheduled on this thread's run loop.  In theory, it's scheduled
     * in the first thread to ask for it. */
    CFNotificationCenterGetDistributedCenter();

    /* We use this run loop source for two purposes.  First, a run loop exits
     * if it has no more sources scheduled.  So, we need at least one source
     * to keep the run loop running.  Second, although it's not critical, it's
     * preferable for the Wine initialization to not proceed until we know
     * the run loop is running.  So, we signal our source immediately after
     * adding it and have its callback spin off the Wine thread. */
    source_context.perform = apple_create_wine_thread;
    source = CFRunLoopSourceCreate( NULL, 0, &source_context );
    CFRunLoopAddSource( CFRunLoopGetCurrent(), source, kCFRunLoopCommonModes );
    CFRunLoopSourceSignal( source );
    CFRelease( source );
    CFRunLoopRun(); /* Should never return, except on error. */
}
#endif  /* __APPLE__ */

/***********************************************************************
 *           __wine_main
 *
 * Main entry point called by the wine loader.
 */
void __wine_main( int argc, char *argv[], char *envp[] )
{
    extern int __wine_main_argc;
    extern char **__wine_main_argv;
    extern char **__wine_main_environ;
    HMODULE module;

    wine_init_argv0_path( argv[0] );
    __wine_main_argc = argc;
    __wine_main_argv = argv;
    __wine_main_environ = envp;

    module = load_ntdll();
    fixup_ntdll_imports( &__wine_spec_nt_header, module );
#ifdef __APPLE__
    apple_main_thread();
#endif
    __wine_set_unix_funcs( NTDLL_UNIXLIB_VERSION, &unix_funcs );
}

/***********************************************************************
 *           __wine_init_unix_lib
 *
 * Lib entry point called by ntdll.dll.so if not yet initialized.
 */
NTSTATUS __cdecl __wine_init_unix_lib( HMODULE module, const void *ptr_in, void *ptr_out )
{
    const IMAGE_NT_HEADERS *nt = ptr_in;

    map_so_dll( nt, module );
    fixup_ntdll_imports( &__wine_spec_nt_header, module );
    *(struct unix_funcs **)ptr_out = &unix_funcs;
    return STATUS_SUCCESS;
}

BOOL WINAPI DECLSPEC_HIDDEN DllMain( HINSTANCE inst, DWORD reason, LPVOID reserved )
{
    if (reason == DLL_PROCESS_ATTACH) LdrDisableThreadCalloutsForDll( inst );
    return TRUE;
}
