/*
 * Loader functions
 *
 * Copyright 1995, 2003 Alexandre Julliard
 * Copyright 2002 Dmitry Timoshkov for Codeweavers
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <assert.h>

#include "winbase.h"
#include "winnt.h"
#include "winternl.h"

#include "module.h"
#include "file.h"
#include "wine/exception.h"
#include "excpt.h"
#include "snoop.h"
#include "wine/debug.h"
#include "wine/server.h"
#include "ntdll_misc.h"

WINE_DEFAULT_DEBUG_CHANNEL(module);
WINE_DECLARE_DEBUG_CHANNEL(relay);
WINE_DECLARE_DEBUG_CHANNEL(snoop);
WINE_DECLARE_DEBUG_CHANNEL(loaddll);

typedef DWORD (CALLBACK *DLLENTRYPROC)(HMODULE,DWORD,LPVOID);

static int process_detaching = 0;  /* set on process detach to avoid deadlocks with thread detach */
static int free_lib_count;   /* recursion depth of LdrUnloadDll calls */

/* filter for page-fault exceptions */
static WINE_EXCEPTION_FILTER(page_fault)
{
    if (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION)
        return EXCEPTION_EXECUTE_HANDLER;
    return EXCEPTION_CONTINUE_SEARCH;
}

static const char * const reason_names[] =
{
    "PROCESS_DETACH",
    "PROCESS_ATTACH",
    "THREAD_ATTACH",
    "THREAD_DETACH"
};

static UINT tls_module_count;      /* number of modules with TLS directory */
static UINT tls_total_size;        /* total size of TLS storage */
static const IMAGE_TLS_DIRECTORY **tls_dirs;  /* array of TLS directories */

static CRITICAL_SECTION loader_section = CRITICAL_SECTION_INIT( "loader_section" );
static WINE_MODREF *cached_modref;
static WINE_MODREF *current_modref;

static NTSTATUS load_dll( LPCSTR libname, DWORD flags, WINE_MODREF** pwm );
static FARPROC find_named_export( HMODULE module, IMAGE_EXPORT_DIRECTORY *exports,
                                  DWORD exp_size, const char *name, int hint );

/* convert PE image VirtualAddress to Real Address */
inline static void *get_rva( HMODULE module, DWORD va )
{
    return (void *)((char *)module + va);
}

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


/*************************************************************************
 *		find_forwarded_export
 *
 * Find the final function pointer for a forwarded function.
 * The loader_section must be locked while calling this function.
 */
static FARPROC find_forwarded_export( HMODULE module, const char *forward )
{
    IMAGE_EXPORT_DIRECTORY *exports;
    DWORD exp_size;
    WINE_MODREF *wm;
    char mod_name[256];
    char *end = strchr(forward, '.');
    FARPROC proc = NULL;

    if (!end) return NULL;
    if (end - forward >= sizeof(mod_name)) return NULL;
    memcpy( mod_name, forward, end - forward );
    mod_name[end-forward] = 0;

    if (!(wm = MODULE_FindModule( mod_name )))
    {
        ERR("module not found for forward '%s' used by '%s'\n",
            forward, get_modref(module)->filename );
        return NULL;
    }
    if ((exports = RtlImageDirectoryEntryToData( wm->ldr.BaseAddress, TRUE,
                                                 IMAGE_DIRECTORY_ENTRY_EXPORT, &exp_size )))
        proc = find_named_export( wm->ldr.BaseAddress, exports, exp_size, end + 1, -1 );

    if (!proc)
    {
        ERR("function not found for forward '%s' used by '%s'."
            " If you are using builtin '%s', try using the native one instead.\n",
            forward, get_modref(module)->filename, get_modref(module)->modname );
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
static FARPROC find_ordinal_export( HMODULE module, IMAGE_EXPORT_DIRECTORY *exports,
                                    DWORD exp_size, int ordinal )
{
    FARPROC proc;
    DWORD *functions = get_rva( module, exports->AddressOfFunctions );

    if (ordinal >= exports->NumberOfFunctions)
    {
        TRACE("	ordinal %ld out of range!\n", ordinal + exports->Base );
        return NULL;
    }
    if (!functions[ordinal]) return NULL;

    proc = get_rva( module, functions[ordinal] );

    /* if the address falls into the export dir, it's a forward */
    if (((char *)proc >= (char *)exports) && ((char *)proc < (char *)exports + exp_size))
        return find_forwarded_export( module, (char *)proc );

    if (TRACE_ON(snoop))
    {
        proc = SNOOP_GetProcAddress( module, exports, exp_size, proc, ordinal );
    }
    if (TRACE_ON(relay) && current_modref)
    {
        proc = RELAY_GetProcAddress( module, exports, exp_size, proc, current_modref->modname );
    }
    return proc;
}


/*************************************************************************
 *		find_named_export
 *
 * Find an exported function by name.
 * The loader_section must be locked while calling this function.
 */
static FARPROC find_named_export( HMODULE module, IMAGE_EXPORT_DIRECTORY *exports,
                                  DWORD exp_size, const char *name, int hint )
{
    WORD *ordinals = get_rva( module, exports->AddressOfNameOrdinals );
    DWORD *names = get_rva( module, exports->AddressOfNames );
    int min = 0, max = exports->NumberOfNames - 1;

    /* first check the hint */
    if (hint >= 0 && hint <= max)
    {
        char *ename = get_rva( module, names[hint] );
        if (!strcmp( ename, name ))
            return find_ordinal_export( module, exports, exp_size, ordinals[hint] );
    }

    /* then do a binary search */
    while (min <= max)
    {
        int res, pos = (min + max) / 2;
        char *ename = get_rva( module, names[pos] );
        if (!(res = strcmp( ename, name )))
            return find_ordinal_export( module, exports, exp_size, ordinals[pos] );
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
static WINE_MODREF *import_dll( HMODULE module, IMAGE_IMPORT_DESCRIPTOR *descr )
{
    NTSTATUS status;
    WINE_MODREF *wmImp;
    HMODULE imp_mod;
    IMAGE_EXPORT_DIRECTORY *exports;
    DWORD exp_size;
    IMAGE_THUNK_DATA *import_list, *thunk_list;
    char *name = get_rva( module, descr->Name );

    status = load_dll( name, 0, &wmImp );
    if (status)
    {
        if (status == STATUS_NO_SUCH_FILE)
            ERR("Module (file) %s (which is needed by %s) not found\n",
                name, current_modref->filename);
        else
            ERR("Loading module (file) %s (which is needed by %s) failed (error %ld).\n",
                name, current_modref->filename, status);
        return NULL;
    }

    imp_mod = wmImp->ldr.BaseAddress;
    if (!(exports = RtlImageDirectoryEntryToData( imp_mod, TRUE,
                                                  IMAGE_DIRECTORY_ENTRY_EXPORT, &exp_size )))
        return NULL;

    thunk_list = get_rva( module, (DWORD)descr->FirstThunk );
    if (descr->u.OriginalFirstThunk)
        import_list = get_rva( module, (DWORD)descr->u.OriginalFirstThunk );
    else
        import_list = thunk_list;

    while (import_list->u1.Ordinal)
    {
        if (IMAGE_SNAP_BY_ORDINAL(import_list->u1.Ordinal))
        {
            int ordinal = IMAGE_ORDINAL(import_list->u1.Ordinal);

            TRACE("--- Ordinal %s,%d\n", name, ordinal);
            thunk_list->u1.Function = (PDWORD)find_ordinal_export( imp_mod, exports, exp_size,
                                                                   ordinal - exports->Base );
            if (!thunk_list->u1.Function)
            {
                ERR("No implementation for %s.%d imported from %s, setting to 0xdeadbeef\n",
                    name, ordinal, current_modref->filename );
                thunk_list->u1.Function = (PDWORD)0xdeadbeef;
            }
        }
        else  /* import by name */
        {
            IMAGE_IMPORT_BY_NAME *pe_name;
            pe_name = get_rva( module, (DWORD)import_list->u1.AddressOfData );
            TRACE("--- %s %s.%d\n", pe_name->Name, name, pe_name->Hint);
            thunk_list->u1.Function = (PDWORD)find_named_export( imp_mod, exports, exp_size,
                                                                 pe_name->Name, pe_name->Hint );
            if (!thunk_list->u1.Function)
            {
                ERR("No implementation for %s.%s imported from %s, setting to 0xdeadbeef\n",
                    name, pe_name->Name, current_modref->filename );
                thunk_list->u1.Function = (PDWORD)0xdeadbeef;
            }
        }
        import_list++;
        thunk_list++;
    }
    return wmImp;
}


/****************************************************************
 * 	PE_fixup_imports
 *
 * Fixup all imports of a given module.
 * The loader_section must be locked while calling this function.
 */
DWORD PE_fixup_imports( WINE_MODREF *wm )
{
    int i, nb_imports;
    IMAGE_IMPORT_DESCRIPTOR *imports;
    WINE_MODREF *prev;
    DWORD size;

    if (!(imports = RtlImageDirectoryEntryToData( wm->ldr.BaseAddress, TRUE,
                                                  IMAGE_DIRECTORY_ENTRY_IMPORT, &size )))
        return 0;

    nb_imports = size / sizeof(*imports);
    for (i = 0; i < nb_imports; i++)
    {
        if (!imports[i].Name)
        {
            nb_imports = i;
            break;
        }
    }
    if (!nb_imports) return 0;  /* no imports */

    /* Allocate module dependency list */
    wm->nDeps = nb_imports;
    wm->deps  = RtlAllocateHeap( ntdll_get_process_heap(), 0, nb_imports*sizeof(WINE_MODREF *) );

    /* load the imported modules. They are automatically
     * added to the modref list of the process.
     */
    prev = current_modref;
    current_modref = wm;
    for (i = 0; i < nb_imports; i++)
    {
        if (!(wm->deps[i] = import_dll( wm->ldr.BaseAddress, &imports[i] ))) break;
    }
    current_modref = prev;
    return (i < nb_imports);
}


/*************************************************************************
 *		MODULE_AllocModRef
 *
 * Allocate a WINE_MODREF structure and add it to the process list
 * The loader_section must be locked while calling this function.
 */
WINE_MODREF *MODULE_AllocModRef( HMODULE hModule, LPCSTR filename )
{
    WINE_MODREF *wm;
    IMAGE_NT_HEADERS *nt = RtlImageNtHeader(hModule);
    PLIST_ENTRY entry, mark;
    BOOLEAN linked = FALSE;

    DWORD long_len = strlen( filename );
    DWORD short_len = GetShortPathNameA( filename, NULL, 0 );

    if ((wm = RtlAllocateHeap( ntdll_get_process_heap(), HEAP_ZERO_MEMORY,
                               sizeof(*wm) + long_len + short_len + 1 )))
    {
        wm->filename = wm->data;
        memcpy( wm->filename, filename, long_len + 1 );
        if ((wm->modname = strrchr( wm->filename, '\\' ))) wm->modname++;
        else wm->modname = wm->filename;

        wm->short_filename = wm->filename + long_len + 1;
        GetShortPathNameA( wm->filename, wm->short_filename, short_len + 1 );
        if ((wm->short_modname = strrchr( wm->short_filename, '\\' ))) wm->short_modname++;
        else wm->short_modname = wm->short_filename;

        wm->ldr.BaseAddress = hModule;
        wm->ldr.EntryPoint = (nt->OptionalHeader.AddressOfEntryPoint) ?
                             ((char *)hModule + nt->OptionalHeader.AddressOfEntryPoint) : 0;
        wm->ldr.SizeOfImage = nt->OptionalHeader.SizeOfImage;
        RtlCreateUnicodeStringFromAsciiz( &wm->ldr.FullDllName, wm->filename);
        RtlCreateUnicodeStringFromAsciiz( &wm->ldr.BaseDllName, wm->modname);
        wm->ldr.Flags = 0;
        wm->ldr.LoadCount = 0;
        wm->ldr.TlsIndex = -1;
        wm->ldr.SectionHandle = NULL;
        wm->ldr.CheckSum = 0;
        wm->ldr.TimeDateStamp = 0;

        /* this is a bit ugly, but we need to have app module first in LoadOrder
         * list, But in wine, ntdll is loaded first, so by inserting DLLs at the tail
         * and app module at the head we insure that order
         */
        if (!(nt->FileHeader.Characteristics & IMAGE_FILE_DLL))
        {
            /* is first loaded module a DLL or an exec ? */
            mark = &NtCurrentTeb()->Peb->LdrData->InLoadOrderModuleList;
            if (mark->Flink == mark ||
                (CONTAINING_RECORD(mark->Flink, LDR_MODULE, InLoadOrderModuleList)->Flags & LDR_IMAGE_IS_DLL))
            {
                InsertHeadList(&NtCurrentTeb()->Peb->LdrData->InLoadOrderModuleList, 
                               &wm->ldr.InLoadOrderModuleList);
                linked = TRUE;
            }
        }
        else wm->ldr.Flags |= LDR_IMAGE_IS_DLL;

        if (!linked)
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
    }
    return wm;
}


/*************************************************************************
 *              alloc_process_tls
 *
 * Allocate the process-wide structure for module TLS storage.
 */
static NTSTATUS alloc_process_tls(void)
{
    PLIST_ENTRY mark, entry;
    PLDR_MODULE mod;
    IMAGE_TLS_DIRECTORY *dir;
    ULONG size, i;

    mark = &NtCurrentTeb()->Peb->LdrData->InMemoryOrderModuleList;
    for (entry = mark->Flink; entry != mark; entry = entry->Flink)
    {
        mod = CONTAINING_RECORD(entry, LDR_MODULE, InMemoryOrderModuleList);
        if (!(dir = RtlImageDirectoryEntryToData( mod->BaseAddress, TRUE,
                                                  IMAGE_DIRECTORY_ENTRY_TLS, &size )))
            continue;
        size = (dir->EndAddressOfRawData - dir->StartAddressOfRawData) + dir->SizeOfZeroFill;
        if (!size) continue;
        tls_total_size += size;
        tls_module_count++;
    }
    if (!tls_module_count) return STATUS_SUCCESS;

    TRACE( "count %u size %u\n", tls_module_count, tls_total_size );

    tls_dirs = RtlAllocateHeap( ntdll_get_process_heap(), 0, tls_module_count * sizeof(*tls_dirs) );
    if (!tls_dirs) return STATUS_NO_MEMORY;

    for (i = 0, entry = mark->Flink; entry != mark; entry = entry->Flink)
    {
        mod = CONTAINING_RECORD(entry, LDR_MODULE, InMemoryOrderModuleList);
        if (!(dir = RtlImageDirectoryEntryToData( mod->BaseAddress, TRUE,
                                                  IMAGE_DIRECTORY_ENTRY_TLS, &size )))
            continue;
        tls_dirs[i] = dir;
        *dir->AddressOfIndex = i;
        mod->TlsIndex = i;
        mod->LoadCount = -1;  /* can't unload it */
        i++;
    }
    return STATUS_SUCCESS;
}


/*************************************************************************
 *              alloc_thread_tls
 *
 * Allocate the per-thread structure for module TLS storage.
 */
static NTSTATUS alloc_thread_tls(void)
{
    void **pointers;
    char *data;
    UINT i;

    if (!tls_module_count) return STATUS_SUCCESS;

    if (!(pointers = RtlAllocateHeap( ntdll_get_process_heap(), 0,
                                      tls_module_count * sizeof(*pointers) )))
        return STATUS_NO_MEMORY;

    if (!(data = RtlAllocateHeap( ntdll_get_process_heap(), 0, tls_total_size )))
    {
        RtlFreeHeap( ntdll_get_process_heap(), 0, pointers );
        return STATUS_NO_MEMORY;
    }

    for (i = 0; i < tls_module_count; i++)
    {
        const IMAGE_TLS_DIRECTORY *dir = tls_dirs[i];
        ULONG size = dir->EndAddressOfRawData - dir->StartAddressOfRawData;

        TRACE( "thread %04lx idx %d: %ld/%ld bytes from %p to %p\n",
               GetCurrentThreadId(), i, size, dir->SizeOfZeroFill,
               (void *)dir->StartAddressOfRawData, data );

        pointers[i] = data;
        memcpy( data, (void *)dir->StartAddressOfRawData, size );
        data += size;
        memset( data, 0, dir->SizeOfZeroFill );
        data += dir->SizeOfZeroFill;
    }
    NtCurrentTeb()->tls_ptr = pointers;
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

    for (callback = dir->AddressOfCallBacks; *callback; callback++)
    {
        if (TRACE_ON(relay))
            DPRINTF("%04lx:Call TLS callback (proc=%p,module=%p,reason=%s,reserved=0)\n",
                    GetCurrentThreadId(), *callback, module, reason_names[reason] );
        (*callback)( module, reason, NULL );
        if (TRACE_ON(relay))
            DPRINTF("%04lx:Ret  TLS callback (proc=%p,module=%p,reason=%s,reserved=0)\n",
                    GetCurrentThreadId(), *callback, module, reason_names[reason] );
    }
}


/*************************************************************************
 *              MODULE_InitDLL
 */
static BOOL MODULE_InitDLL( WINE_MODREF *wm, UINT reason, LPVOID lpReserved )
{
    char mod_name[32];
    BOOL retv = TRUE;
    DLLENTRYPROC entry = wm->ldr.EntryPoint;
    void *module = wm->ldr.BaseAddress;

    /* Skip calls for modules loaded with special load flags */

    if (wm->ldr.Flags & LDR_DONT_RESOLVE_REFS) return TRUE;
    if (wm->ldr.TlsIndex != -1) call_tls_callbacks( wm->ldr.BaseAddress, reason );
    if (!entry || !(wm->ldr.Flags & LDR_IMAGE_IS_DLL)) return TRUE;

    if (TRACE_ON(relay))
    {
        size_t len = max( strlen(wm->modname), sizeof(mod_name)-1 );
        memcpy( mod_name, wm->modname, len );
        mod_name[len] = 0;
        DPRINTF("%04lx:Call PE DLL (proc=%p,module=%p (%s),reason=%s,res=%p)\n",
                GetCurrentThreadId(), entry, module, mod_name, reason_names[reason], lpReserved );
    }
    else TRACE("(%p (%s),%s,%p) - CALL\n", module, wm->modname, reason_names[reason], lpReserved );

    retv = entry( module, reason, lpReserved );

    /* The state of the module list may have changed due to the call
       to the dll. We cannot assume that this module has not been
       deleted.  */
    if (TRACE_ON(relay))
        DPRINTF("%04lx:Ret  PE DLL (proc=%p,module=%p (%s),reason=%s,res=%p) retval=%x\n",
                GetCurrentThreadId(), entry, module, mod_name, reason_names[reason], lpReserved, retv );
    else TRACE("(%p,%s,%p) - RETURN %d\n", module, reason_names[reason], lpReserved, retv );

    return retv;
}


/*************************************************************************
 *		MODULE_DllProcessAttach
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
 */
NTSTATUS MODULE_DllProcessAttach( WINE_MODREF *wm, LPVOID lpReserved )
{
    NTSTATUS status = STATUS_SUCCESS;
    int i;

    RtlEnterCriticalSection( &loader_section );

    if (!wm)
    {
        PLIST_ENTRY mark;

        mark = &NtCurrentTeb()->Peb->LdrData->InLoadOrderModuleList;
        wm = CONTAINING_RECORD(CONTAINING_RECORD(mark->Flink, 
                                                 LDR_MODULE, InLoadOrderModuleList),
                               WINE_MODREF, ldr);
        wm->ldr.LoadCount = -1;  /* can't unload main exe */
        if ((status = alloc_process_tls()) != STATUS_SUCCESS) goto done;
        if ((status = alloc_thread_tls()) != STATUS_SUCCESS) goto done;
    }
    assert( wm );

    /* prevent infinite recursion in case of cyclical dependencies */
    if (    ( wm->ldr.Flags & LDR_LOAD_IN_PROGRESS )
         || ( wm->ldr.Flags & LDR_PROCESS_ATTACHED ) )
        goto done;

    TRACE("(%s,%p) - START\n", wm->modname, lpReserved );

    /* Tag current MODREF to prevent recursive loop */
    wm->ldr.Flags |= LDR_LOAD_IN_PROGRESS;

    /* Recursively attach all DLLs this one depends on */
    for ( i = 0; i < wm->nDeps; i++ )
    {
        if (!wm->deps[i]) continue;
        if ((status = MODULE_DllProcessAttach( wm->deps[i], lpReserved )) != STATUS_SUCCESS) break;
    }

    /* Call DLL entry point */
    if (status == STATUS_SUCCESS)
    {
        WINE_MODREF *prev = current_modref;
        current_modref = wm;
        if (MODULE_InitDLL( wm, DLL_PROCESS_ATTACH, lpReserved ))
            wm->ldr.Flags |= LDR_PROCESS_ATTACHED;
        else
            status = STATUS_DLL_INIT_FAILED;
        current_modref = prev;
    }

    InsertTailList(&NtCurrentTeb()->Peb->LdrData->InInitializationOrderModuleList, 
                   &wm->ldr.InInitializationOrderModuleList);
        
    /* Remove recursion flag */
    wm->ldr.Flags &= ~LDR_LOAD_IN_PROGRESS;

    TRACE("(%s,%p) - END\n", wm->modname, lpReserved );

 done:
    RtlLeaveCriticalSection( &loader_section );
    return status;
}

/*************************************************************************
 *		MODULE_DllProcessDetach
 *
 * Send DLL process detach notifications.  See the comment about calling
 * sequence at MODULE_DllProcessAttach.  Unless the bForceDetach flag
 * is set, only DLLs with zero refcount are notified.
 */
static void MODULE_DllProcessDetach( BOOL bForceDetach, LPVOID lpReserved )
{
    PLIST_ENTRY mark, entry;
    PLDR_MODULE mod;

    RtlEnterCriticalSection( &loader_section );
    if (bForceDetach) process_detaching = 1;
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
            if ( mod->LoadCount && !bForceDetach )
                continue;

            /* Call detach notification */
            mod->Flags &= ~LDR_PROCESS_ATTACHED;
            MODULE_InitDLL( CONTAINING_RECORD(mod, WINE_MODREF, ldr), 
                            DLL_PROCESS_DETACH, lpReserved );

            /* Restart at head of WINE_MODREF list, as entries might have
               been added and/or removed while performing the call ... */
            break;
        }
    } while (entry != mark);

    RtlLeaveCriticalSection( &loader_section );
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
    /* FIXME: there is still a race here */

    RtlEnterCriticalSection( &loader_section );

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
        if ((const void *)mod->BaseAddress <= addr &&
            (char *)addr < (char*)mod->BaseAddress + mod->SizeOfImage)
        {
            *pmod = mod;
            return STATUS_SUCCESS;
        }
        if ((const void *)mod->BaseAddress > addr) break;
    }
    return STATUS_NO_MORE_ENTRIES;
}

/**********************************************************************
 *	    MODULE_FindModule
 *
 * Find a (loaded) win32 module depending on path
 *	LPCSTR path: [in] pathname of module/library to be found
 *
 * The loader_section must be locked while calling this function
 * RETURNS
 *	the module handle if found
 * 	0 if not
 */
WINE_MODREF *MODULE_FindModule(LPCSTR path)
{
    WINE_MODREF	*wm;
    PLIST_ENTRY mark, entry;
    PLDR_MODULE mod;
    char dllname[260], *p;

    /* Append .DLL to name if no extension present */
    strcpy( dllname, path );
    if (!(p = strrchr( dllname, '.')) || strchr( p, '/' ) || strchr( p, '\\'))
            strcat( dllname, ".DLL" );

    if ((wm = cached_modref) != NULL)
    {
        if ( !FILE_strcasecmp( dllname, wm->modname ) ) return wm;
        if ( !FILE_strcasecmp( dllname, wm->filename ) ) return wm;
        if ( !FILE_strcasecmp( dllname, wm->short_modname ) ) return wm;
        if ( !FILE_strcasecmp( dllname, wm->short_filename ) ) return wm;
    }

    mark = &NtCurrentTeb()->Peb->LdrData->InLoadOrderModuleList;
    for (entry = mark->Flink; entry != mark; entry = entry->Flink)
    {
        mod = CONTAINING_RECORD(entry, LDR_MODULE, InLoadOrderModuleList);
        wm = CONTAINING_RECORD(mod, WINE_MODREF, ldr);

        if ( !FILE_strcasecmp( dllname, wm->modname ) ) break;
        if ( !FILE_strcasecmp( dllname, wm->filename ) ) break;
        if ( !FILE_strcasecmp( dllname, wm->short_modname ) ) break;
        if ( !FILE_strcasecmp( dllname, wm->short_filename ) ) break;
    }
    if (entry == mark) wm = NULL;

    cached_modref = wm;
    return wm;
}


/******************************************************************
 *		LdrLockLoaderLock  (NTDLL.@)
 *
 * Note: flags are not implemented.
 * Flag 0x01 is used to raise exceptions on errors.
 * Flag 0x02 is used to avoid waiting on the section (does RtlTryEnterCriticalSection instead).
 */
NTSTATUS WINAPI LdrLockLoaderLock( ULONG flags, ULONG *result, ULONG *magic )
{
    if (flags) FIXME( "flags %lx not supported\n", flags );

    if (result) *result = 1;
    if (!magic) return STATUS_INVALID_PARAMETER_3;
    RtlEnterCriticalSection( &loader_section );
    *magic = GetCurrentThreadId();
    return STATUS_SUCCESS;
}


/******************************************************************
 *		LdrUnlockLoaderUnlock  (NTDLL.@)
 */
NTSTATUS WINAPI LdrUnlockLoaderLock( ULONG flags, ULONG magic )
{
    if (magic)
    {
        if (magic != GetCurrentThreadId()) return STATUS_INVALID_PARAMETER_2;
        RtlLeaveCriticalSection( &loader_section );
    }
    return STATUS_SUCCESS;
}


/******************************************************************
 *		LdrGetDllHandle (NTDLL.@)
 *
 *
 */
NTSTATUS WINAPI LdrGetDllHandle(ULONG x, ULONG y, PUNICODE_STRING name, HMODULE *base)
{
    WINE_MODREF *wm;
    STRING str;

    if (x != 0 || y != 0)
        FIXME("Unknown behavior, please report\n");

    /* FIXME: we should store module name information as unicode */
    RtlUnicodeStringToAnsiString( &str, name, TRUE );
    wm = MODULE_FindModule( str.Buffer );
    RtlFreeAnsiString( &str );

    if (!wm)
    {
        *base = 0;
        return STATUS_DLL_NOT_FOUND;
    }

    *base = wm->ldr.BaseAddress;

    TRACE("%lx %lx %s -> %p\n", x, y, debugstr_us(name), *base);

    return STATUS_SUCCESS;
}


/******************************************************************
 *		LdrGetProcedureAddress  (NTDLL.@)
 */
NTSTATUS WINAPI LdrGetProcedureAddress(HMODULE module, PANSI_STRING name, ULONG ord, PVOID *address)
{
    IMAGE_EXPORT_DIRECTORY *exports;
    DWORD exp_size;
    NTSTATUS ret = STATUS_PROCEDURE_NOT_FOUND;

    RtlEnterCriticalSection( &loader_section );

    if ((exports = RtlImageDirectoryEntryToData( module, TRUE,
                                                 IMAGE_DIRECTORY_ENTRY_EXPORT, &exp_size )))
    {
        void *proc = name ? find_named_export( module, exports, exp_size, name->Buffer, -1 )
                          : find_ordinal_export( module, exports, exp_size, ord - exports->Base );
        if (proc)
        {
            *address = proc;
            ret = STATUS_SUCCESS;
        }
    }
    else
    {
        /* check if the module itself is invalid to return the proper error */
        if (!get_modref( module )) ret = STATUS_DLL_NOT_FOUND;
    }

    RtlLeaveCriticalSection( &loader_section );
    return ret;
}


/***********************************************************************
 *      allocate_lib_dir
 *
 * helper for MODULE_LoadLibraryExA.  Allocate space to hold the directory
 * portion of the provided name and put the name in it.
 *
 */
static LPCSTR allocate_lib_dir(LPCSTR libname)
{
    LPCSTR p, pmax;
    LPSTR result;
    int length;

    pmax = libname;
    if ((p = strrchr( pmax, '\\' ))) pmax = p + 1;
    if ((p = strrchr( pmax, '/' ))) pmax = p + 1; /* Naughty.  MSDN says don't */
    if (pmax == libname && pmax[0] && pmax[1] == ':') pmax += 2;

    length = pmax - libname;

    result = RtlAllocateHeap (ntdll_get_process_heap(), 0, length+1);

    if (result)
    {
        strncpy (result, libname, length);
        result [length] = '\0';
    }

    return result;
}

/***********************************************************************
 *	load_dll  (internal)
 *
 * Load a PE style module according to the load order.
 *
 * libdir is used to support LOAD_WITH_ALTERED_SEARCH_PATH during the recursion
 *        on this function.  When first called from LoadLibraryExA it will be
 *        NULL but thereafter it may point to a buffer containing the path
 *        portion of the library name.  Note that the recursion all occurs
 *        within a Critical section (see LoadLibraryExA) so the use of a
 *        static is acceptable.
 *        (We have to use a static variable at some point anyway, to pass the
 *        information from BUILTIN32_dlopen through dlopen and the builtin's
 *        init function into load_library).
 * allocated_libdir is TRUE in the stack frame that allocated libdir
 */
static NTSTATUS load_dll( LPCSTR libname, DWORD flags, WINE_MODREF** pwm )
{
    int i;
    enum loadorder_type loadorder[LOADORDER_NTYPES];
    LPSTR filename;
    const char *filetype = "";
    DWORD found;
    BOOL allocated_libdir = FALSE;
    static LPCSTR libdir = NULL; /* See above */
    NTSTATUS nts = STATUS_NO_SUCH_FILE;

    *pwm = NULL;
    if ( !libname ) return STATUS_DLL_NOT_FOUND; /* FIXME ? */

    filename = RtlAllocateHeap ( ntdll_get_process_heap(), 0, MAX_PATH + 1 );
    if ( !filename ) return STATUS_NO_MEMORY;
    *filename = 0; /* Just in case we don't set it before goto error */

    RtlEnterCriticalSection( &loader_section );

    if ((flags & LOAD_WITH_ALTERED_SEARCH_PATH) && FILE_contains_path(libname))
    {
        if (!(libdir = allocate_lib_dir(libname)))
        {
            nts = STATUS_NO_MEMORY;
            goto error;
        }
        allocated_libdir = TRUE;
    }

    if (!libdir || allocated_libdir)
        found = SearchPathA(NULL, libname, ".dll", MAX_PATH, filename, NULL);
    else
        found = DIR_SearchAlternatePath(libdir, libname, ".dll", MAX_PATH, filename, NULL);

    /* build the modules filename */
    if (!found)
    {
        if (!MODULE_GetBuiltinPath( libname, ".dll", filename, MAX_PATH ))
        {
            nts = STATUS_INTERNAL_ERROR;
            goto error;
        }
    }

    /* Check for already loaded module */
    if (!(*pwm = MODULE_FindModule(filename)) && !FILE_contains_path(libname))
    {
        LPSTR	fn = RtlAllocateHeap ( ntdll_get_process_heap(), 0, MAX_PATH + 1 );
        if (fn)
        {
            /* since the default loading mechanism uses a more detailed algorithm
             * than SearchPath (like using PATH, which can even be modified between
             * two attempts of loading the same DLL), the look-up above (with
             * SearchPath) can have put the file in system directory, whereas it
             * has already been loaded but with a different path. So do a specific
             * look-up with filename (without any path)
             */
            strcpy ( fn, libname );
            /* if the filename doesn't have an extension append .DLL */
            if (!strrchr( fn, '.')) strcat( fn, ".dll" );
            if ((*pwm = MODULE_FindModule( fn )) != NULL)
                strcpy( filename, fn );
            RtlFreeHeap( ntdll_get_process_heap(), 0, fn );
        }
    }
    if (*pwm)
    {
        if ((*pwm)->ldr.LoadCount != -1) (*pwm)->ldr.LoadCount++;

        if (((*pwm)->ldr.Flags & LDR_DONT_RESOLVE_REFS) &&
            !(flags & DONT_RESOLVE_DLL_REFERENCES))
        {
            (*pwm)->ldr.Flags &= ~LDR_DONT_RESOLVE_REFS;
            PE_fixup_imports( *pwm );
        }
        TRACE("Already loaded module '%s' at %p, count=%d\n", filename, (*pwm)->ldr.BaseAddress, (*pwm)->ldr.LoadCount);
        if (allocated_libdir)
        {
            RtlFreeHeap( ntdll_get_process_heap(), 0, (LPSTR)libdir );
            libdir = NULL;
        }
        RtlLeaveCriticalSection( &loader_section );
        RtlFreeHeap( ntdll_get_process_heap(), 0, filename );
        return STATUS_SUCCESS;
    }

    MODULE_GetLoadOrder( loadorder, filename, TRUE);

    for (i = 0; i < LOADORDER_NTYPES; i++)
    {
        if (loadorder[i] == LOADORDER_INVALID) break;

        switch (loadorder[i])
        {
        case LOADORDER_DLL:
            TRACE("Trying native dll '%s'\n", filename);
            nts = PE_LoadLibraryExA(filename, flags, pwm);
            filetype = "native";
            break;
            
        case LOADORDER_BI:
            TRACE("Trying built-in '%s'\n", filename);
            nts = BUILTIN32_LoadLibraryExA(filename, flags, pwm);
            filetype = "builtin";
            break;
            
        default:
            nts = STATUS_INTERNAL_ERROR;
            break;
        }

        if (nts == STATUS_SUCCESS)
        {
            /* Initialize DLL just loaded */
            TRACE("Loaded module '%s' (%s) at %p\n", filename, filetype, (*pwm)->ldr.BaseAddress);
            if (!TRACE_ON(module))
                TRACE_(loaddll)("Loaded module '%s' : %s\n", filename, filetype);
            /* Set the ldr.LoadCount here so that an attach failure will */
            /* decrement the dependencies through the MODULE_FreeLibrary call. */
            (*pwm)->ldr.LoadCount = 1;
            
            if (allocated_libdir)
            {
                RtlFreeHeap( ntdll_get_process_heap(), 0, (LPSTR)libdir );
                libdir = NULL;
            }
            RtlLeaveCriticalSection( &loader_section );
            RtlFreeHeap( ntdll_get_process_heap(), 0, filename );
            return nts;
        }

        if (nts != STATUS_NO_SUCH_FILE)
        {
            WARN("Loading of %s DLL %s failed (status %ld).\n",
                 filetype, filename, nts);
            break;
        }
    }

 error:
    if (allocated_libdir)
    {
        RtlFreeHeap( ntdll_get_process_heap(), 0, (LPSTR)libdir );
        libdir = NULL;
    }
    RtlLeaveCriticalSection( &loader_section );
    WARN("Failed to load module '%s'; status=%ld\n", filename, nts);
    RtlFreeHeap( ntdll_get_process_heap(), 0, filename );
    return nts;
}

/******************************************************************
 *		LdrLoadDll (NTDLL.@)
 */
NTSTATUS WINAPI LdrLoadDll(LPCWSTR path_name, DWORD flags, PUNICODE_STRING libname, HMODULE* hModule)
{
    WINE_MODREF *wm;
    NTSTATUS    nts = STATUS_SUCCESS;
    STRING      str;

    RtlUnicodeStringToAnsiString(&str, libname, TRUE);

    RtlEnterCriticalSection( &loader_section );

    switch (nts = load_dll( str.Buffer, flags, &wm ))
    {
    case STATUS_SUCCESS:
        nts = MODULE_DllProcessAttach( wm, NULL );
        if (nts != STATUS_SUCCESS)
        {
            WARN("Attach failed for module '%s'.\n", str.Buffer);
            LdrUnloadDll(wm->ldr.BaseAddress);
            wm = NULL;
        }
        break;
    case STATUS_NO_SUCH_FILE:
        nts = STATUS_DLL_NOT_FOUND;
        break;
    default: /* keep error code as it is (memory...) */
        break;
    }

    *hModule = (wm) ? wm->ldr.BaseAddress : NULL;
    
    RtlLeaveCriticalSection( &loader_section );

    RtlFreeAnsiString(&str);

    return nts;
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
            sm->Id = 0; /* FIXME */
            sm->Rank = 0; /* FIXME */
            sm->Unknown = 0; /* FIXME */
            str.Length = 0;
            str.MaximumLength = MAXIMUM_FILENAME_LENGTH;
            str.Buffer = sm->Name;
            RtlUnicodeStringToAnsiString(&str, &mod->FullDllName, FALSE);
            ptr = strrchr(sm->Name, '\\');
            sm->NameOffset = (ptr != NULL) ? (ptr - (char*)sm->Name + 1) : 0;

            smi->ModulesCount++;
            sm++;
        }
        else nts = STATUS_INFO_LENGTH_MISMATCH;
    }
    RtlLeaveCriticalSection( &loader_section );

    if (req_size) *req_size = size;

    return nts;
}

/******************************************************************
 *		LdrShutdownProcess (NTDLL.@)
 *
 */
void WINAPI LdrShutdownProcess(void)
{
    TRACE("()\n");
    MODULE_DllProcessDetach( TRUE, (LPVOID)1 );
}

/******************************************************************
 *		LdrShutdownThread (NTDLL.@)
 *
 */
void WINAPI LdrShutdownThread(void)
{
    PLIST_ENTRY mark, entry;
    PLDR_MODULE mod;

    TRACE("()\n");

    /* don't do any detach calls if process is exiting */
    if (process_detaching) return;
    /* FIXME: there is still a race here */

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

    RtlLeaveCriticalSection( &loader_section );
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
        mod = CONTAINING_RECORD(entry, LDR_MODULE, 
                                InInitializationOrderModuleList);
        wm = CONTAINING_RECORD(mod, WINE_MODREF, ldr);

        prev = entry->Blink;
        if (wm->ldr.LoadCount) continue;

        RemoveEntryList(&wm->ldr.InLoadOrderModuleList);
        RemoveEntryList(&wm->ldr.InMemoryOrderModuleList);
        RemoveEntryList(&wm->ldr.InInitializationOrderModuleList);

        TRACE(" unloading %s\n", wm->filename);
        if (!TRACE_ON(module))
            TRACE_(loaddll)("Unloaded module '%s' : %s\n", wm->filename,
                            wm->dlhandle ? "builtin" : "native" );

        SERVER_START_REQ( unload_dll )
        {
            req->base = wm->ldr.BaseAddress;
            wine_server_call( req );
        }
        SERVER_END_REQ;

        if (wm->dlhandle) wine_dll_unload( wm->dlhandle );
        else NtUnmapViewOfSection( GetCurrentProcess(), wm->ldr.BaseAddress );
        if (cached_modref == wm) cached_modref = NULL;
        RtlFreeHeap( ntdll_get_process_heap(), 0, wm->deps );
        RtlFreeHeap( ntdll_get_process_heap(), 0, wm );
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
    TRACE("(%s) ldr.LoadCount: %d\n", wm->modname, wm->ldr.LoadCount );

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
    NTSTATUS retv = STATUS_SUCCESS;

    TRACE("(%p)\n", hModule);

    RtlEnterCriticalSection( &loader_section );

    /* if we're stopping the whole process (and forcing the removal of all
     * DLLs) the library will be freed anyway
     */
    if (!process_detaching)
    {
        WINE_MODREF *wm;

        free_lib_count++;
        if ((wm = get_modref( hModule )) != NULL)
        {
            TRACE("(%s) - START\n", wm->modname);

            /* Recursively decrement reference counts */
            MODULE_DecRefCount( wm );

            /* Call process detach notifications */
            if ( free_lib_count <= 1 )
            {
                MODULE_DllProcessDetach( FALSE, NULL );
                MODULE_FlushModrefs();
            }

            TRACE("END\n");
        }
        else
            retv = STATUS_DLL_NOT_FOUND;

        free_lib_count--;
    }

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
    __EXCEPT(page_fault)
    {
        return NULL;
    }
    __ENDTRY
    return ret;
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
    if (dir >= nt->OptionalHeader.NumberOfRvaAndSizes) return NULL;
    if (!(addr = nt->OptionalHeader.DataDirectory[dir].VirtualAddress)) return NULL;
    *size = nt->OptionalHeader.DataDirectory[dir].Size;
    if (image || addr < nt->OptionalHeader.SizeOfHeaders) return (char *)module + addr;

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
    IMAGE_SECTION_HEADER *sec = (IMAGE_SECTION_HEADER*)((char*)&nt->OptionalHeader +
                                                        nt->FileHeader.SizeOfOptionalHeader);
    for (i = 0; i < nt->FileHeader.NumberOfSections; i++, sec++)
    {
        if ((sec->VirtualAddress <= rva) && (sec->VirtualAddress + sec->SizeOfRawData > rva))
            return sec;
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
