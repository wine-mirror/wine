/*
 * 386-specific Win32 relay functions
 *
 * Copyright 1997 Alexandre Julliard
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

#include "config.h"
#include "wine/port.h"

#include <assert.h>
#include <string.h>
#include <stdio.h>

#include "winternl.h"
#include "stackframe.h"
#include "module.h"
#include "wine/unicode.h"
#include "wine/debug.h"
#include "ntdll_misc.h"

WINE_DEFAULT_DEBUG_CHANNEL(relay);
WINE_DECLARE_DEBUG_CHANNEL(snoop);

const char **debug_relay_excludelist = NULL;
const char **debug_relay_includelist = NULL;
const char **debug_snoop_excludelist = NULL;
const char **debug_snoop_includelist = NULL;

static const char **debug_from_relay_excludelist;
static const char **debug_from_relay_includelist;

/***********************************************************************
 *           build_list
 *
 * Build a function list from a ';'-separated string.
 */
static const char **build_list( const WCHAR *bufferW )
{
    int count = 1;
    char buffer[1024];
    const char *p = buffer;
    const char **ret;

    RtlUnicodeToMultiByteN( buffer, sizeof(buffer), NULL,
                            bufferW, (strlenW(bufferW)+1) * sizeof(WCHAR) );

    while ((p = strchr( p, ';' )))
    {
         count++;
        p++;
    }
    /* allocate count+1 pointers, plus the space for a copy of the string */
    if ((ret = RtlAllocateHeap( ntdll_get_process_heap(), 0, (count+1) * sizeof(char*) + strlen(buffer) + 1 )))
    {
        char *str = (char *)(ret + count + 1);
        char *p = str;

        strcpy( str, buffer );
        count = 0;
        for (;;)
        {
            ret[count++] = p;
            if (!(p = strchr( p, ';' ))) break;
            *p++ = 0;
        }
        ret[count++] = NULL;
    }
    return ret;
}


/***********************************************************************
 *           RELAY_InitDebugLists
 *
 * Build the relay include/exclude function lists.
 */
void RELAY_InitDebugLists(void)
{
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING name;
    char buffer[1024];
    HKEY hkey;
    DWORD count;
    WCHAR *str;
    static const WCHAR configW[] = {'M','a','c','h','i','n','e','\\',
                                    'S','o','f','t','w','a','r','e','\\',
                                    'W','i','n','e','\\',
                                    'W','i','n','e','\\',
                                    'C','o','n','f','i','g','\\',
                                    'D','e','b','u','g',0};
    static const WCHAR RelayIncludeW[] = {'R','e','l','a','y','I','n','c','l','u','d','e',0};
    static const WCHAR RelayExcludeW[] = {'R','e','l','a','y','E','x','c','l','u','d','e',0};
    static const WCHAR SnoopIncludeW[] = {'S','n','o','o','p','I','n','c','l','u','d','e',0};
    static const WCHAR SnoopExcludeW[] = {'S','n','o','o','p','E','x','c','l','u','d','e',0};
    static const WCHAR RelayFromIncludeW[] = {'R','e','l','a','y','F','r','o','m','I','n','c','l','u','d','e',0};
    static const WCHAR RelayFromExcludeW[] = {'R','e','l','a','y','F','r','o','m','E','x','c','l','u','d','e',0};

    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.ObjectName = &name;
    attr.Attributes = 0;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;
    RtlInitUnicodeString( &name, configW );

    if (NtOpenKey( &hkey, KEY_ALL_ACCESS, &attr )) return;

    str = (WCHAR *)((KEY_VALUE_PARTIAL_INFORMATION *)buffer)->Data;
    RtlInitUnicodeString( &name, RelayIncludeW );
    if (!NtQueryValueKey( hkey, &name, KeyValuePartialInformation, buffer, sizeof(buffer), &count ))
    {
        TRACE("RelayInclude = %s\n", debugstr_w(str) );
        debug_relay_includelist = build_list( str );
    }

    RtlInitUnicodeString( &name, RelayExcludeW );
    if (!NtQueryValueKey( hkey, &name, KeyValuePartialInformation, buffer, sizeof(buffer), &count ))
    {
        TRACE( "RelayExclude = %s\n", debugstr_w(str) );
        debug_relay_excludelist = build_list( str );
    }

    RtlInitUnicodeString( &name, SnoopIncludeW );
    if (!NtQueryValueKey( hkey, &name, KeyValuePartialInformation, buffer, sizeof(buffer), &count ))
    {
        TRACE_(snoop)( "SnoopInclude = %s\n", debugstr_w(str) );
        debug_snoop_includelist = build_list( str );
    }

    RtlInitUnicodeString( &name, SnoopExcludeW );
    if (!NtQueryValueKey( hkey, &name, KeyValuePartialInformation, buffer, sizeof(buffer), &count ))
    {
        TRACE_(snoop)( "SnoopExclude = %s\n", debugstr_w(str) );
        debug_snoop_excludelist = build_list( str );
    }

    RtlInitUnicodeString( &name, RelayFromIncludeW );
    if (!NtQueryValueKey( hkey, &name, KeyValuePartialInformation, buffer, sizeof(buffer), &count ))
    {
        TRACE("RelayFromInclude = %s\n", debugstr_w(str) );
        debug_from_relay_includelist = build_list( str );
    }

    RtlInitUnicodeString( &name, RelayFromExcludeW );
    if (!NtQueryValueKey( hkey, &name, KeyValuePartialInformation, buffer, sizeof(buffer), &count ))
    {
        TRACE( "RelayFromExclude = %s\n", debugstr_w(str) );
        debug_from_relay_excludelist = build_list( str );
    }

    NtClose( hkey );
}


#ifdef __i386__

typedef struct
{
    BYTE          call;                    /* 0xe8 call callfrom32 (relative) */
    DWORD         callfrom32 WINE_PACKED;  /* RELAY_CallFrom32 relative addr */
    BYTE          ret;                     /* 0xc2 ret $n  or  0xc3 ret */
    WORD          args;                    /* nb of args to remove from the stack */
    void         *orig;                    /* original entry point */
    DWORD         argtypes;                /* argument types */
} DEBUG_ENTRY_POINT;


/***********************************************************************
 *           check_relay_include
 *
 * Check if a given function must be included in the relay output.
 */
static BOOL check_relay_include( const char *module, const char *func )
{
    const char **listitem;
    BOOL show;

    if (!debug_relay_excludelist && !debug_relay_includelist) return TRUE;
    if (debug_relay_excludelist)
    {
        show = TRUE;
        listitem = debug_relay_excludelist;
    }
    else
    {
        show = FALSE;
        listitem = debug_relay_includelist;
    }
    for(; *listitem; listitem++)
    {
        char *p = strrchr( *listitem, '.' );
        if (p && p > *listitem)  /* check module and function */
        {
            int len = p - *listitem;
            if (strncasecmp( *listitem, module, len-1 ) || module[len]) continue;
            if (!strcmp( p + 1, func ) || !strcmp( p + 1, "*" )) return !show;
        }
        else  /* function only */
        {
            if (!strcmp( *listitem, func )) return !show;
        }
    }
    return show;
}


/***********************************************************************
 *           check_relay_from_module
 *
 * Check if calls from a given module must be included in the relay output.
 */
static BOOL check_relay_from_module( const char *module )
{
    const char **listitem;
    BOOL show;

    if (!debug_from_relay_excludelist && !debug_from_relay_includelist) return TRUE;
    if (debug_from_relay_excludelist)
    {
        show = TRUE;
        listitem = debug_from_relay_excludelist;
    }
    else
    {
        show = FALSE;
        listitem = debug_from_relay_includelist;
    }
    for(; *listitem; listitem++)
    {
        int len;

        if (!strcasecmp( *listitem, module )) return !show;
        len = strlen( *listitem );
        if (!strncasecmp( *listitem, module, len ) && !strcasecmp( module + len, ".dll" ))
            return !show;
    }
    return show;
}


/***********************************************************************
 *           find_exported_name
 *
 * Find the name of an exported function.
 */
static const char *find_exported_name( const char *module,
                                       IMAGE_EXPORT_DIRECTORY *exp, int ordinal )
{
    int i;
    const char *ret = NULL;

    WORD *ordptr = (WORD *)(module + exp->AddressOfNameOrdinals);
    for (i = 0; i < exp->NumberOfNames; i++, ordptr++)
        if (*ordptr + exp->Base == ordinal) break;
    if (i < exp->NumberOfNames)
        ret = module + ((DWORD*)(module + exp->AddressOfNames))[i];
    return ret;
}


/***********************************************************************
 *           get_entry_point
 *
 * Get the name of the DLL entry point corresponding to a relay address.
 */
static void get_entry_point( char *buffer, DEBUG_ENTRY_POINT *relay )
{
    IMAGE_EXPORT_DIRECTORY *exp = NULL;
    DEBUG_ENTRY_POINT *debug;
    char *p, *base = NULL;
    const char *name;
    int ordinal = 0;
    WINE_MODREF *wm;
    DWORD size;

    /* First find the module */

    for (wm = MODULE_modref_list; wm; wm = wm->next)
    {
        if (!(wm->ldr.Flags & LDR_WINE_INTERNAL)) continue;
        exp = RtlImageDirectoryEntryToData( wm->ldr.BaseAddress, TRUE, IMAGE_DIRECTORY_ENTRY_EXPORT, &size );
        if (!exp) continue;
        debug = (DEBUG_ENTRY_POINT *)((char *)exp + size);
        if (debug <= relay && relay < debug + exp->NumberOfFunctions)
        {
            ordinal = relay - debug;
            break;
        }
    }

    /* Now find the function */

    base = (char *)wm->ldr.BaseAddress;
    strcpy( buffer, base + exp->Name );
    p = buffer + strlen(buffer);
    if (p > buffer + 4 && !strcasecmp( p - 4, ".dll" )) p -= 4;

    if ((name = find_exported_name( base, exp, ordinal + exp->Base )))
        sprintf( p, ".%s", name );
    else
        sprintf( p, ".%ld", ordinal + exp->Base );
}


/***********************************************************************
 *           RELAY_PrintArgs
 */
static inline void RELAY_PrintArgs( int *args, int nb_args, unsigned int typemask )
{
    while (nb_args--)
    {
	if ((typemask & 3) && HIWORD(*args))
        {
	    if (typemask & 2)
	    	DPRINTF( "%08x %s", *args, debugstr_w((LPWSTR)*args) );
            else
	    	DPRINTF( "%08x %s", *args, debugstr_a((LPCSTR)*args) );
	}
        else DPRINTF( "%08x", *args );
        if (nb_args) DPRINTF( "," );
        args++;
        typemask >>= 2;
    }
}


typedef LONGLONG (*LONGLONG_CPROC)();
typedef LONGLONG (WINAPI *LONGLONG_FARPROC)();


/***********************************************************************
 *           call_cdecl_function
 */
static LONGLONG call_cdecl_function( LONGLONG_CPROC func, int nb_args, const int *args )
{
    LONGLONG ret;
    switch(nb_args)
    {
    case 0: ret = func(); break;
    case 1: ret = func(args[0]); break;
    case 2: ret = func(args[0],args[1]); break;
    case 3: ret = func(args[0],args[1],args[2]); break;
    case 4: ret = func(args[0],args[1],args[2],args[3]); break;
    case 5: ret = func(args[0],args[1],args[2],args[3],args[4]); break;
    case 6: ret = func(args[0],args[1],args[2],args[3],args[4],
                       args[5]); break;
    case 7: ret = func(args[0],args[1],args[2],args[3],args[4],args[5],
                       args[6]); break;
    case 8: ret = func(args[0],args[1],args[2],args[3],args[4],args[5],
                       args[6],args[7]); break;
    case 9: ret = func(args[0],args[1],args[2],args[3],args[4],args[5],
                       args[6],args[7],args[8]); break;
    case 10: ret = func(args[0],args[1],args[2],args[3],args[4],args[5],
                        args[6],args[7],args[8],args[9]); break;
    case 11: ret = func(args[0],args[1],args[2],args[3],args[4],args[5],
                        args[6],args[7],args[8],args[9],args[10]); break;
    case 12: ret = func(args[0],args[1],args[2],args[3],args[4],args[5],
                        args[6],args[7],args[8],args[9],args[10],
                        args[11]); break;
    case 13: ret = func(args[0],args[1],args[2],args[3],args[4],args[5],
                        args[6],args[7],args[8],args[9],args[10],args[11],
                        args[12]); break;
    case 14: ret = func(args[0],args[1],args[2],args[3],args[4],args[5],
                        args[6],args[7],args[8],args[9],args[10],args[11],
                        args[12],args[13]); break;
    case 15: ret = func(args[0],args[1],args[2],args[3],args[4],args[5],
                        args[6],args[7],args[8],args[9],args[10],args[11],
                        args[12],args[13],args[14]); break;
    case 16: ret = func(args[0],args[1],args[2],args[3],args[4],args[5],
                        args[6],args[7],args[8],args[9],args[10],args[11],
                        args[12],args[13],args[14],args[15]); break;
    default:
        ERR( "Unsupported nb of args %d\n", nb_args );
        assert(FALSE);
        ret = 0;
        break;
    }
    return ret;
}


/***********************************************************************
 *           call_stdcall_function
 */
static LONGLONG call_stdcall_function( LONGLONG_FARPROC func, int nb_args, const int *args )
{
    LONGLONG ret;
    switch(nb_args)
    {
    case 0: ret = func(); break;
    case 1: ret = func(args[0]); break;
    case 2: ret = func(args[0],args[1]); break;
    case 3: ret = func(args[0],args[1],args[2]); break;
    case 4: ret = func(args[0],args[1],args[2],args[3]); break;
    case 5: ret = func(args[0],args[1],args[2],args[3],args[4]); break;
    case 6: ret = func(args[0],args[1],args[2],args[3],args[4],
                       args[5]); break;
    case 7: ret = func(args[0],args[1],args[2],args[3],args[4],args[5],
                       args[6]); break;
    case 8: ret = func(args[0],args[1],args[2],args[3],args[4],args[5],
                       args[6],args[7]); break;
    case 9: ret = func(args[0],args[1],args[2],args[3],args[4],args[5],
                       args[6],args[7],args[8]); break;
    case 10: ret = func(args[0],args[1],args[2],args[3],args[4],args[5],
                        args[6],args[7],args[8],args[9]); break;
    case 11: ret = func(args[0],args[1],args[2],args[3],args[4],args[5],
                        args[6],args[7],args[8],args[9],args[10]); break;
    case 12: ret = func(args[0],args[1],args[2],args[3],args[4],args[5],
                        args[6],args[7],args[8],args[9],args[10],
                        args[11]); break;
    case 13: ret = func(args[0],args[1],args[2],args[3],args[4],args[5],
                        args[6],args[7],args[8],args[9],args[10],args[11],
                        args[12]); break;
    case 14: ret = func(args[0],args[1],args[2],args[3],args[4],args[5],
                        args[6],args[7],args[8],args[9],args[10],args[11],
                        args[12],args[13]); break;
    case 15: ret = func(args[0],args[1],args[2],args[3],args[4],args[5],
                        args[6],args[7],args[8],args[9],args[10],args[11],
                        args[12],args[13],args[14]); break;
    case 16: ret = func(args[0],args[1],args[2],args[3],args[4],args[5],
                        args[6],args[7],args[8],args[9],args[10],args[11],
                        args[12],args[13],args[14],args[15]); break;
    default:
        ERR( "Unsupported nb of args %d\n", nb_args );
        assert(FALSE);
        ret = 0;
        break;
    }
    return ret;
}


/***********************************************************************
 *           RELAY_CallFrom32
 *
 * Stack layout on entry to this function:
 *  ...      ...
 * (esp+12)  arg2
 * (esp+8)   arg1
 * (esp+4)   ret_addr
 * (esp)     return addr to relay code
 */
static LONGLONG RELAY_CallFrom32( int ret_addr, ... )
{
    LONGLONG ret;
    char buffer[80];

    int *args = &ret_addr + 1;
    /* Relay addr is the return address for this function */
    BYTE *relay_addr = (BYTE *)__builtin_return_address(0);
    DEBUG_ENTRY_POINT *relay = (DEBUG_ENTRY_POINT *)(relay_addr - 5);
    WORD nb_args = relay->args / sizeof(int);

    if (TRACE_ON(relay))
    {
        get_entry_point( buffer, relay );

        DPRINTF( "%04lx:Call %s(", GetCurrentThreadId(), buffer );
        RELAY_PrintArgs( args, nb_args, relay->argtypes );
        DPRINTF( ") ret=%08x\n", ret_addr );
    }

    if (relay->ret == 0xc3) /* cdecl */
    {
        ret = call_cdecl_function( (LONGLONG_CPROC)relay->orig, nb_args, args );
    }
    else  /* stdcall */
    {
        ret = call_stdcall_function( (LONGLONG_FARPROC)relay->orig, nb_args, args );
    }

    if (TRACE_ON(relay))
    {
        BOOL ret64 = (relay->argtypes & 0x80000000) && (nb_args < 16);
        if (ret64)
            DPRINTF( "%04lx:Ret  %s() retval=%08x%08x ret=%08x\n",
                     GetCurrentThreadId(),
                     buffer, (UINT)(ret >> 32), (UINT)ret, ret_addr );
        else
            DPRINTF( "%04lx:Ret  %s() retval=%08x ret=%08x\n",
                     GetCurrentThreadId(),
                     buffer, (UINT)ret, ret_addr );
    }
    return ret;
}


/***********************************************************************
 *           RELAY_CallFrom32Regs
 *
 * Stack layout (esp is context->Esp, not the current %esp):
 *
 * ...
 * (esp+4) first arg
 * (esp)   return addr to caller
 * (esp-4) return addr to DEBUG_ENTRY_POINT
 * (esp-8) ptr to relay entry code for RELAY_CallFrom32Regs
 *  ...    >128 bytes space free to be modified (ensured by the assembly glue)
 */
void WINAPI RELAY_DoCallFrom32Regs( CONTEXT86 *context )
{
    char buffer[80];
    int* args;
    int args_copy[17];
    BYTE *entry_point;

    BYTE *relay_addr = *((BYTE **)context->Esp - 1);
    DEBUG_ENTRY_POINT *relay = (DEBUG_ENTRY_POINT *)(relay_addr - 5);
    WORD nb_args = relay->args / sizeof(int);

    /* remove extra stuff from the stack */
    context->Eip = stack32_pop(context);
    args = (int *)context->Esp;
    if (relay->ret == 0xc2) /* stdcall */
        context->Esp += nb_args * sizeof(int);

    entry_point = (BYTE *)relay->orig;
    assert( *entry_point == 0xe8 /* lcall */ );

    if (TRACE_ON(relay))
    {
        get_entry_point( buffer, relay );

        DPRINTF( "%04lx:Call %s(", GetCurrentThreadId(), buffer );
        RELAY_PrintArgs( args, nb_args, relay->argtypes );
        DPRINTF( ") ret=%08lx fs=%04lx\n", context->Eip, context->SegFs );

        DPRINTF(" eax=%08lx ebx=%08lx ecx=%08lx edx=%08lx esi=%08lx edi=%08lx\n",
                context->Eax, context->Ebx, context->Ecx,
                context->Edx, context->Esi, context->Edi );
        DPRINTF(" ebp=%08lx esp=%08lx ds=%04lx es=%04lx gs=%04lx flags=%08lx\n",
                context->Ebp, context->Esp, context->SegDs,
                context->SegEs, context->SegGs, context->EFlags );
    }

    /* Now call the real function */

    memcpy( args_copy, args, nb_args * sizeof(args[0]) );
    args_copy[nb_args] = (int)context;  /* append context argument */
    if (relay->ret == 0xc3) /* cdecl */
    {
        call_cdecl_function( *(LONGLONG_CPROC *)(entry_point + 5), nb_args+1, args_copy );
    }
    else  /* stdcall */
    {
        call_stdcall_function( *(LONGLONG_FARPROC *)(entry_point + 5), nb_args+1, args_copy );
    }

    if (TRACE_ON(relay))
    {
        DPRINTF( "%04lx:Ret  %s() retval=%08lx ret=%08lx fs=%04lx\n",
                 GetCurrentThreadId(),
                 buffer, context->Eax, context->Eip, context->SegFs );

        DPRINTF(" eax=%08lx ebx=%08lx ecx=%08lx edx=%08lx esi=%08lx edi=%08lx\n",
                context->Eax, context->Ebx, context->Ecx,
                context->Edx, context->Esi, context->Edi );
        DPRINTF(" ebp=%08lx esp=%08lx ds=%04lx es=%04lx gs=%04lx flags=%08lx\n",
                context->Ebp, context->Esp, context->SegDs,
                context->SegEs, context->SegGs, context->EFlags );
    }
}

void WINAPI RELAY_CallFrom32Regs(void);
__ASM_GLOBAL_FUNC( RELAY_CallFrom32Regs,
                   "call " __ASM_NAME("__wine_call_from_32_regs") "\n\t"
                   ".long " __ASM_NAME("RELAY_DoCallFrom32Regs") ",0" );


/* check whether the function at addr starts with a call to __wine_call_from_32_regs */
static BOOL is_register_entry_point( const BYTE *addr )
{
    extern void __wine_call_from_32_regs();
    int *offset;
    void *ptr;

    if (*addr != 0xe8) return FALSE;  /* not a call */
    /* check if call target is __wine_call_from_32_regs */
    offset = (int *)(addr + 1);
    if (*offset == (char *)__wine_call_from_32_regs - (char *)(offset + 1)) return TRUE;
    /* now check if call target is an import table jump to __wine_call_from_32_regs */
    addr = (BYTE *)(offset + 1) + *offset;
    if (addr[0] != 0xff || addr[1] != 0x25) return FALSE;  /* not an indirect jmp */
    ptr = *(void **)(addr + 2);  /* get indirect jmp target address */
    return (*(char **)ptr == (char *)__wine_call_from_32_regs);
}


/***********************************************************************
 *           RELAY_GetProcAddress
 *
 * Return the proc address to use for a given function.
 */
FARPROC RELAY_GetProcAddress( HMODULE module, IMAGE_EXPORT_DIRECTORY *exports,
                              DWORD exp_size, FARPROC proc, const char *user )
{
    DEBUG_ENTRY_POINT *debug = (DEBUG_ENTRY_POINT *)proc;
    DEBUG_ENTRY_POINT *list = (DEBUG_ENTRY_POINT *)((char *)exports + exp_size);

    if (debug < list || debug >= list + exports->NumberOfFunctions) return proc;
    if (list + (debug - list) != debug) return proc;  /* not a valid address */
    if (check_relay_from_module( user )) return proc;  /* we want to relay it */
    if (!debug->call) return proc;  /* not a normal function */
    if (debug->call != 0xe8 && debug->call != 0xe9) return proc; /* not a debug thunk at all */
    return debug->orig;
}


/***********************************************************************
 *           RELAY_SetupDLL
 *
 * Setup relay debugging for a built-in dll.
 */
void RELAY_SetupDLL( const char *module )
{
    IMAGE_EXPORT_DIRECTORY *exports;
    DEBUG_ENTRY_POINT *debug;
    DWORD *funcs;
    int i;
    const char *name;
    char *p, dllname[80];
    DWORD size;

    exports = RtlImageDirectoryEntryToData( (HMODULE)module, TRUE,
                                            IMAGE_DIRECTORY_ENTRY_EXPORT, &size );
    if (!exports) return;
    debug = (DEBUG_ENTRY_POINT *)((char *)exports + size);
    funcs = (DWORD *)(module + exports->AddressOfFunctions);
    strcpy( dllname, module + exports->Name );
    p = dllname + strlen(dllname) - 4;
    if (p > dllname && !strcasecmp( p, ".dll" )) *p = 0;

    for (i = 0; i < exports->NumberOfFunctions; i++, funcs++, debug++)
    {
        int on = 1;

        if (!debug->call) continue;  /* not a normal function */
        if (debug->call != 0xe8 && debug->call != 0xe9) break; /* not a debug thunk at all */

        if ((name = find_exported_name( module, exports, i + exports->Base )))
            on = check_relay_include( dllname, name );

        if (on)
        {
            debug->call = 0xe8;  /* call relative */
            if (is_register_entry_point( debug->orig ))
                debug->callfrom32 = (char *)RELAY_CallFrom32Regs - (char *)&debug->ret;
            else
                debug->callfrom32 = (char *)RELAY_CallFrom32 - (char *)&debug->ret;
        }
        else
        {
            debug->call = 0xe9;  /* jmp relative */
            debug->callfrom32 = (char *)debug->orig - (char *)&debug->ret;
        }

        debug->orig = (FARPROC)(module + (DWORD)*funcs);
        *funcs = (char *)debug - module;
    }
}

#else  /* __i386__ */

FARPROC RELAY_GetProcAddress( HMODULE module, IMAGE_EXPORT_DIRECTORY *exports,
                              DWORD exp_size, FARPROC proc, const char *user )
{
    return proc;
}

void RELAY_SetupDLL( const char *module )
{
}

#endif /* __i386__ */
