/*
 * Loader functions
 *
 * Copyright 1995 Alexandre Julliard
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
#include "wine/debug.h"
#include "wine/server.h"
#include "ntdll_misc.h"

WINE_DEFAULT_DEBUG_CHANNEL(ntdll);
WINE_DECLARE_DEBUG_CHANNEL(module);
WINE_DECLARE_DEBUG_CHANNEL(module);
WINE_DECLARE_DEBUG_CHANNEL(loaddll);

WINE_MODREF *MODULE_modref_list = NULL;

static WINE_MODREF *exe_modref;
static int process_detaching = 0;  /* set on process detach to avoid deadlocks with thread detach */
static int free_lib_count;   /* recursion depth of LdrUnloadDll calls */

/* filter for page-fault exceptions */
static WINE_EXCEPTION_FILTER(page_fault)
{
    if (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION)
        return EXCEPTION_EXECUTE_HANDLER;
    return EXCEPTION_CONTINUE_SEARCH;
}

static CRITICAL_SECTION loader_section = CRITICAL_SECTION_INIT( "loader_section" );

/*************************************************************************
 *		MODULE32_LookupHMODULE
 * looks for the referenced HMODULE in the current process
 * NOTE: Assumes that the process critical section is held!
 */
static WINE_MODREF *MODULE32_LookupHMODULE( HMODULE hmod )
{
    WINE_MODREF	*wm;

    if (!hmod)
    	return exe_modref;

    if (!HIWORD(hmod)) {
    	ERR("tried to lookup %p in win32 module handler!\n",hmod);
	return NULL;
    }
    for ( wm = MODULE_modref_list; wm; wm=wm->next )
	if (wm->module == hmod)
	    return wm;
    return NULL;
}

/*************************************************************************
 *		MODULE_AllocModRef
 *
 * Allocate a WINE_MODREF structure and add it to the process list
 * NOTE: Assumes that the process critical section is held!
 */
WINE_MODREF *MODULE_AllocModRef( HMODULE hModule, LPCSTR filename )
{
    WINE_MODREF *wm;
    IMAGE_NT_HEADERS *nt = RtlImageNtHeader(hModule);

    DWORD long_len = strlen( filename );
    DWORD short_len = GetShortPathNameA( filename, NULL, 0 );

    if ((wm = RtlAllocateHeap( ntdll_get_process_heap(), HEAP_ZERO_MEMORY,
                               sizeof(*wm) + long_len + short_len + 1 )))
    {
        wm->module = hModule;
        wm->tlsindex = -1;

        wm->filename = wm->data;
        memcpy( wm->filename, filename, long_len + 1 );
        if ((wm->modname = strrchr( wm->filename, '\\' ))) wm->modname++;
        else wm->modname = wm->filename;

        wm->short_filename = wm->filename + long_len + 1;
        GetShortPathNameA( wm->filename, wm->short_filename, short_len + 1 );
        if ((wm->short_modname = strrchr( wm->short_filename, '\\' ))) wm->short_modname++;
        else wm->short_modname = wm->short_filename;

        wm->next = MODULE_modref_list;
        if (wm->next) wm->next->prev = wm;
        MODULE_modref_list = wm;

        wm->ldr.InLoadOrderModuleList.Flink = NULL;
        wm->ldr.InLoadOrderModuleList.Blink = NULL;
        wm->ldr.InMemoryOrderModuleList.Flink = NULL;
        wm->ldr.InMemoryOrderModuleList.Blink = NULL;
        wm->ldr.InInitializationOrderModuleList.Flink = NULL;
        wm->ldr.InInitializationOrderModuleList.Blink = NULL;
        wm->ldr.BaseAddress = hModule;
        wm->ldr.EntryPoint = (nt->OptionalHeader.AddressOfEntryPoint) ?
                             ((char *)hModule + nt->OptionalHeader.AddressOfEntryPoint) : 0;
        wm->ldr.SizeOfImage = nt->OptionalHeader.SizeOfImage;
        RtlCreateUnicodeStringFromAsciiz( &wm->ldr.FullDllName, wm->filename);
        RtlCreateUnicodeStringFromAsciiz( &wm->ldr.BaseDllName, wm->modname);
        wm->ldr.Flags = 0;
        wm->ldr.LoadCount = 0;
        wm->ldr.TlsIndex = 0;
        wm->ldr.SectionHandle = NULL;
        wm->ldr.CheckSum = 0;
        wm->ldr.TimeDateStamp = 0;

        if (!(nt->FileHeader.Characteristics & IMAGE_FILE_DLL))
        {
            if (!exe_modref) exe_modref = wm;
            else FIXME( "Trying to load second .EXE file: %s\n", filename );
        }
    }
    return wm;
}

/*************************************************************************
 *              MODULE_InitDLL
 */
static BOOL MODULE_InitDLL( WINE_MODREF *wm, DWORD type, LPVOID lpReserved )
{
    static const char * const typeName[] = { "PROCESS_DETACH", "PROCESS_ATTACH",
                                             "THREAD_ATTACH", "THREAD_DETACH" };
    BOOL retv = TRUE;

    /* Skip calls for modules loaded with special load flags */

    if (wm->flags & WINE_MODREF_DONT_RESOLVE_REFS) return TRUE;

    TRACE("(%s,%s,%p) - CALL\n", wm->modname, typeName[type], lpReserved );

    /* Call the initialization routine */
    retv = PE_InitDLL( wm->module, type, lpReserved );

    /* The state of the module list may have changed due to the call
       to PE_InitDLL. We cannot assume that this module has not been
       deleted.  */
    TRACE("(%p,%s,%p) - RETURN %d\n", wm, typeName[type], lpReserved, retv );

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
BOOL MODULE_DllProcessAttach( WINE_MODREF *wm, LPVOID lpReserved )
{
    BOOL retv = TRUE;
    int i;

    RtlEnterCriticalSection( &loader_section );

    if (!wm)
    {
        wm = exe_modref;
        PE_InitTls();
    }
    assert( wm );

    /* prevent infinite recursion in case of cyclical dependencies */
    if (    ( wm->flags & WINE_MODREF_MARKER )
         || ( wm->flags & WINE_MODREF_PROCESS_ATTACHED ) )
        goto done;

    TRACE("(%s,%p) - START\n", wm->modname, lpReserved );

    /* Tag current MODREF to prevent recursive loop */
    wm->flags |= WINE_MODREF_MARKER;

    /* Recursively attach all DLLs this one depends on */
    for ( i = 0; retv && i < wm->nDeps; i++ )
        if ( wm->deps[i] )
            retv = MODULE_DllProcessAttach( wm->deps[i], lpReserved );

    /* Call DLL entry point */
    if ( retv )
    {
        retv = MODULE_InitDLL( wm, DLL_PROCESS_ATTACH, lpReserved );
        if ( retv )
            wm->flags |= WINE_MODREF_PROCESS_ATTACHED;
    }

    /* Re-insert MODREF at head of list */
    if ( retv && wm->prev )
    {
        wm->prev->next = wm->next;
        if ( wm->next ) wm->next->prev = wm->prev;

        wm->prev = NULL;
        wm->next = MODULE_modref_list;
        MODULE_modref_list = wm->next->prev = wm;
    }

    /* Remove recursion flag */
    wm->flags &= ~WINE_MODREF_MARKER;

    TRACE("(%s,%p) - END\n", wm->modname, lpReserved );

 done:
    RtlLeaveCriticalSection( &loader_section );
    return retv;
}

/*************************************************************************
 *		MODULE_DllProcessDetach
 *
 * Send DLL process detach notifications.  See the comment about calling
 * sequence at MODULE_DllProcessAttach.  Unless the bForceDetach flag
 * is set, only DLLs with zero refcount are notified.
 */
void MODULE_DllProcessDetach( BOOL bForceDetach, LPVOID lpReserved )
{
    WINE_MODREF *wm;

    RtlEnterCriticalSection( &loader_section );
    if (bForceDetach) process_detaching = 1;
    do
    {
        for ( wm = MODULE_modref_list; wm; wm = wm->next )
        {
            /* Check whether to detach this DLL */
            if ( !(wm->flags & WINE_MODREF_PROCESS_ATTACHED) )
                continue;
            if ( wm->refCount > 0 && !bForceDetach )
                continue;

            /* Call detach notification */
            wm->flags &= ~WINE_MODREF_PROCESS_ATTACHED;
            MODULE_InitDLL( wm, DLL_PROCESS_DETACH, lpReserved );

            /* Restart at head of WINE_MODREF list, as entries might have
               been added and/or removed while performing the call ... */
            break;
        }
    } while ( wm );

    RtlLeaveCriticalSection( &loader_section );
}

/*************************************************************************
 *		MODULE_DllThreadAttach
 *
 * Send DLL thread attach notifications. These are sent in the
 * reverse sequence of process detach notification.
 *
 */
void MODULE_DllThreadAttach( LPVOID lpReserved )
{
    WINE_MODREF *wm;

    /* don't do any attach calls if process is exiting */
    if (process_detaching) return;
    /* FIXME: there is still a race here */

    RtlEnterCriticalSection( &loader_section );

    PE_InitTls();

    for ( wm = MODULE_modref_list; wm; wm = wm->next )
        if ( !wm->next )
            break;

    for ( ; wm; wm = wm->prev )
    {
        if ( !(wm->flags & WINE_MODREF_PROCESS_ATTACHED) )
            continue;
        if ( wm->flags & WINE_MODREF_NO_DLL_CALLS )
            continue;

        MODULE_InitDLL( wm, DLL_THREAD_ATTACH, lpReserved );
    }

    RtlLeaveCriticalSection( &loader_section );
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

    wm = MODULE32_LookupHMODULE( hModule );
    if ( !wm )
        ret = STATUS_DLL_NOT_FOUND;
    else
        wm->flags |= WINE_MODREF_NO_DLL_CALLS;

    RtlLeaveCriticalSection( &loader_section );

    return ret;
}

/******************************************************************
 *              LdrFindEntryForAddress (NTDLL.@)
 *
 * The loader_section must be locked while calling this function
 */
NTSTATUS WINAPI LdrFindEntryForAddress(const void* addr, PLDR_MODULE* mod)
{
    WINE_MODREF*        wm;

    for ( wm = MODULE_modref_list; wm; wm = wm->next )
    {
        if ((const void *)wm->module <= addr &&
            (char *)addr < (char*)wm->module + wm->ldr.SizeOfImage)
        {
            *mod = &wm->ldr;
            return STATUS_SUCCESS;
        }
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
    char dllname[260], *p;

    /* Append .DLL to name if no extension present */
    strcpy( dllname, path );
    if (!(p = strrchr( dllname, '.')) || strchr( p, '/' ) || strchr( p, '\\'))
            strcat( dllname, ".DLL" );

    for ( wm = MODULE_modref_list; wm; wm = wm->next )
    {
        if ( !FILE_strcasecmp( dllname, wm->modname ) )
            break;
        if ( !FILE_strcasecmp( dllname, wm->filename ) )
            break;
        if ( !FILE_strcasecmp( dllname, wm->short_modname ) )
            break;
        if ( !FILE_strcasecmp( dllname, wm->short_filename ) )
            break;
    }

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

    TRACE("%08lx %08lx %s %p\n",
          x, y, name ? debugstr_wn(name->Buffer, name->Length) : NULL, base);

    if (x != 0 || y != 0)
        FIXME("Unknown behavior, please report\n");

    /* FIXME: we should store module name information as unicode */
    if (name)
    {
        STRING str;

        RtlUnicodeStringToAnsiString( &str, name, TRUE );

        wm = MODULE_FindModule( str.Buffer );
        RtlFreeAnsiString( &str );
    }
    else
        wm = exe_modref;

    if (!wm)
    {
        *base = 0;
        return STATUS_DLL_NOT_FOUND;
    }

    *base = wm->module;
    return STATUS_SUCCESS;
}

/***********************************************************************
 *           MODULE_GetProcAddress   		(internal)
 */
FARPROC MODULE_GetProcAddress(
	HMODULE hModule, 	/* [in] current module handle */
	LPCSTR function,	/* [in] function to be looked up */
	int hint,
	BOOL snoop )
{
    WINE_MODREF	*wm;
    FARPROC	retproc = 0;

    if (HIWORD(function))
	TRACE("(%p,%s (%d))\n",hModule,function,hint);
    else
	TRACE("(%p,%p)\n",hModule,function);

    RtlEnterCriticalSection( &loader_section );
    if ((wm = MODULE32_LookupHMODULE( hModule )))
    {
        retproc = wm->find_export( wm, function, hint, snoop );
    }
    RtlLeaveCriticalSection( &loader_section );
    return retproc;
}


/******************************************************************
 *		LdrGetProcedureAddress (NTDLL.@)
 *
 *
 */
NTSTATUS WINAPI LdrGetProcedureAddress(HMODULE base, PANSI_STRING name, ULONG ord, PVOID *address)
{
    WARN("%p %s %ld %p\n", base, name ? debugstr_an(name->Buffer, name->Length) : NULL, ord, address);

    *address = MODULE_GetProcAddress( base, name ? name->Buffer : (LPSTR)ord, -1, TRUE );

    return (*address) ? STATUS_SUCCESS : STATUS_PROCEDURE_NOT_FOUND;
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
 *	MODULE_LoadLibraryExA	(internal)
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
NTSTATUS MODULE_LoadLibraryExA( LPCSTR libname, DWORD flags, WINE_MODREF** pwm)
{
    int i;
    enum loadorder_type loadorder[LOADORDER_NTYPES];
    LPSTR filename;
    const char *filetype = "";
    DWORD found;
    BOOL allocated_libdir = FALSE;
    static LPCSTR libdir = NULL; /* See above */
    NTSTATUS nts = STATUS_SUCCESS;

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
        (*pwm)->refCount++;
        
        if (((*pwm)->flags & WINE_MODREF_DONT_RESOLVE_REFS) &&
            !(flags & DONT_RESOLVE_DLL_REFERENCES))
        {
            (*pwm)->flags &= ~WINE_MODREF_DONT_RESOLVE_REFS;
            PE_fixup_imports( *pwm );
        }
        TRACE("Already loaded module '%s' at %p, count=%d\n", filename, (*pwm)->module, (*pwm)->refCount);
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
            TRACE("Loaded module '%s' at %p\n", filename, (*pwm)->module);
            if (!TRACE_ON(module))
                TRACE_(loaddll)("Loaded module '%s' : %s\n", filename, filetype);
            /* Set the refCount here so that an attach failure will */
            /* decrement the dependencies through the MODULE_FreeLibrary call. */
            (*pwm)->refCount = 1;
            
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

    switch (nts = MODULE_LoadLibraryExA( str.Buffer, flags, &wm ))
    {
    case STATUS_SUCCESS:
        if ( !MODULE_DllProcessAttach( wm, NULL ) )
        {
            WARN_(module)("Attach failed for module '%s'.\n", str.Buffer);
            LdrUnloadDll(wm->module);
            nts = STATUS_DLL_INIT_FAILED;
            wm = NULL;
        }
        break;
    case STATUS_NO_SUCH_FILE:
        nts = STATUS_DLL_NOT_FOUND;
        break;
    default: /* keep error code as it is (memory...) */
        break;
    }

    *hModule = (wm) ? wm->module : NULL;
    
    RtlLeaveCriticalSection( &loader_section );

    RtlFreeAnsiString(&str);

    return nts;
}

/******************************************************************
 *		LdrShutdownProcess (NTDLL.@)
 *
 */
NTSTATUS    WINAPI  LdrShutdownProcess(void)
{
    TRACE("()\n");
    MODULE_DllProcessDetach( TRUE, (LPVOID)1 );
    return STATUS_SUCCESS; /* FIXME */
}

/******************************************************************
 *		LdrShutdownThread (NTDLL.@)
 *
 */
NTSTATUS WINAPI LdrShutdownThread(void)
{
    WINE_MODREF *wm;
    TRACE("()\n");

    /* don't do any detach calls if process is exiting */
    if (process_detaching) return STATUS_SUCCESS;
    /* FIXME: there is still a race here */

    RtlEnterCriticalSection( &loader_section );

    for ( wm = MODULE_modref_list; wm; wm = wm->next )
    {
        if ( !(wm->flags & WINE_MODREF_PROCESS_ATTACHED) )
            continue;
        if ( wm->flags & WINE_MODREF_NO_DLL_CALLS )
            continue;

        MODULE_InitDLL( wm, DLL_THREAD_DETACH, NULL );
    }

    RtlLeaveCriticalSection( &loader_section );
    return STATUS_SUCCESS; /* FIXME */
}

/***********************************************************************
 *           MODULE_FlushModrefs
 *
 * NOTE: Assumes that the process critical section is held!
 *
 * Remove all unused modrefs and call the internal unloading routines
 * for the library type.
 */
static void MODULE_FlushModrefs(void)
{
    WINE_MODREF *wm, *next;

    for (wm = MODULE_modref_list; wm; wm = next)
    {
        next = wm->next;

        if (wm->refCount)
            continue;

        /* Unlink this modref from the chain */
        if (wm->next)
            wm->next->prev = wm->prev;
        if (wm->prev)
            wm->prev->next = wm->next;
        if (wm == MODULE_modref_list)
            MODULE_modref_list = wm->next;

        TRACE(" unloading %s\n", wm->filename);
        if (!TRACE_ON(module))
            TRACE_(loaddll)("Unloaded module '%s' : %s\n", wm->filename,
                            wm->dlhandle ? "builtin" : "native" );

        SERVER_START_REQ( unload_dll )
        {
            req->base = (void *)wm->module;
            wine_server_call( req );
        }
        SERVER_END_REQ;

        if (wm->dlhandle) wine_dll_unload( wm->dlhandle );
        else UnmapViewOfFile( (LPVOID)wm->module );
        FreeLibrary16( wm->hDummyMod );
        RtlFreeHeap( ntdll_get_process_heap(), 0, wm->deps );
        RtlFreeHeap( ntdll_get_process_heap(), 0, wm );
    }
}

/***********************************************************************
 *           MODULE_DecRefCount
 *
 * NOTE: Assumes that the process critical section is held!
 */
static void MODULE_DecRefCount( WINE_MODREF *wm )
{
    int i;

    if ( wm->flags & WINE_MODREF_MARKER )
        return;

    if ( wm->refCount <= 0 )
        return;

    --wm->refCount;
    TRACE("(%s) refCount: %d\n", wm->modname, wm->refCount );

    if ( wm->refCount == 0 )
    {
        wm->flags |= WINE_MODREF_MARKER;

        for ( i = 0; i < wm->nDeps; i++ )
            if ( wm->deps[i] )
                MODULE_DecRefCount( wm->deps[i] );

        wm->flags &= ~WINE_MODREF_MARKER;
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
        if ((wm = MODULE32_LookupHMODULE( hModule )) != NULL)
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
