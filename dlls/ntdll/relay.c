/*
 * Win32 relay and snoop functions
 *
 * Copyright 1997 Alexandre Julliard
 * Copyright 1998 Marcus Meissner
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
#include <stdarg.h>
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winternl.h"
#include "excpt.h"
#include "wine/exception.h"
#include "ntdll_misc.h"
#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(relay);
WINE_DECLARE_DEBUG_CHANNEL(snoop);
WINE_DECLARE_DEBUG_CHANNEL(seh);

static const WCHAR **debug_relay_excludelist;
static const WCHAR **debug_relay_includelist;
static const WCHAR **debug_snoop_excludelist;
static const WCHAR **debug_snoop_includelist;
static const WCHAR **debug_from_relay_excludelist;
static const WCHAR **debug_from_relay_includelist;

/* compare an ASCII and a Unicode string without depending on the current codepage */
inline static int strcmpAW( const char *strA, const WCHAR *strW )
{
    while (*strA && ((unsigned char)*strA == *strW)) { strA++; strW++; }
    return (unsigned char)*strA - *strW;
}

/* compare an ASCII and a Unicode string without depending on the current codepage */
inline static int strncmpiAW( const char *strA, const WCHAR *strW, int n )
{
    int ret = 0;
    for ( ; n > 0; n--, strA++, strW++)
        if ((ret = toupperW((unsigned char)*strA) - toupperW(*strW)) || !*strA) break;
    return ret;
}

/***********************************************************************
 *           build_list
 *
 * Build a function list from a ';'-separated string.
 */
static const WCHAR **build_list( const WCHAR *buffer )
{
    int count = 1;
    const WCHAR *p = buffer;
    const WCHAR **ret;

    while ((p = strchrW( p, ';' )))
    {
        count++;
        p++;
    }
    /* allocate count+1 pointers, plus the space for a copy of the string */
    if ((ret = RtlAllocateHeap( GetProcessHeap(), 0,
                                (count+1) * sizeof(WCHAR*) + (strlenW(buffer)+1) * sizeof(WCHAR) )))
    {
        WCHAR *str = (WCHAR *)(ret + count + 1);
        WCHAR *p = str;

        strcpyW( str, buffer );
        count = 0;
        for (;;)
        {
            ret[count++] = p;
            if (!(p = strchrW( p, ';' ))) break;
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

#include "pshpack1.h"

typedef struct
{
    BYTE          call;         /* 0xe8 call callfrom32 (relative) */
    DWORD         callfrom32;   /* RELAY_CallFrom32 relative addr */
    BYTE          ret;          /* 0xc2 ret $n  or  0xc3 ret */
    WORD          args;         /* nb of args to remove from the stack */
    void         *orig;         /* original entry point */
    DWORD         argtypes;     /* argument types */
} DEBUG_ENTRY_POINT;

typedef	struct
{
	/* code part */
	BYTE		lcall;		/* 0xe8 call snoopentry (relative) */
	/* NOTE: If you move snoopentry OR nrofargs fix the relative offset
	 * calculation!
	 */
	DWORD		snoopentry;	/* SNOOP_Entry relative */
	/* unreached */
	int		nrofargs;
	FARPROC	origfun;
	const char *name;
} SNOOP_FUN;

typedef struct tagSNOOP_DLL {
	HMODULE	hmod;
	SNOOP_FUN	*funs;
	DWORD		ordbase;
	DWORD		nrofordinals;
	struct tagSNOOP_DLL	*next;
	char name[1];
} SNOOP_DLL;

typedef struct
{
	/* code part */
	BYTE		lcall;		/* 0xe8 call snoopret relative*/
	/* NOTE: If you move snoopret OR origreturn fix the relative offset
	 * calculation!
	 */
	DWORD		snoopret;	/* SNOOP_Ret relative */
	/* unreached */
	FARPROC	origreturn;
	SNOOP_DLL	*dll;
	DWORD		ordinal;
	DWORD		origESP;
	DWORD		*args;		/* saved args across a stdcall */
} SNOOP_RETURNENTRY;

typedef struct tagSNOOP_RETURNENTRIES {
	SNOOP_RETURNENTRY entry[4092/sizeof(SNOOP_RETURNENTRY)];
	struct tagSNOOP_RETURNENTRIES	*next;
} SNOOP_RETURNENTRIES;

#include "poppack.h"

extern void WINAPI SNOOP_Entry();
extern void WINAPI SNOOP_Return();

static SNOOP_DLL *firstdll;
static SNOOP_RETURNENTRIES *firstrets;

static WINE_EXCEPTION_FILTER(page_fault)
{
    if (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ||
        GetExceptionCode() == EXCEPTION_PRIV_INSTRUCTION)
        return EXCEPTION_EXECUTE_HANDLER;
    return EXCEPTION_CONTINUE_SEARCH;
}

/***********************************************************************
 *           check_list
 *
 * Check if a given module and function is in the list.
 */
static BOOL check_list( const char *module, int ordinal, const char *func, const WCHAR **list )
{
    char ord_str[10];

    sprintf( ord_str, "%d", ordinal );
    for(; *list; list++)
    {
        const WCHAR *p = strrchrW( *list, '.' );
        if (p && p > *list)  /* check module and function */
        {
            int len = p - *list;
            if (strncmpiAW( module, *list, len-1 ) || module[len]) continue;
            if (p[1] == '*' && !p[2]) return TRUE;
            if (!strcmpAW( ord_str, p + 1 )) return TRUE;
            if (func && !strcmpAW( func, p + 1 )) return TRUE;
        }
        else  /* function only */
        {
            if (func && !strcmpAW( func, *list )) return TRUE;
        }
    }
    return FALSE;
}


/***********************************************************************
 *           check_relay_include
 *
 * Check if a given function must be included in the relay output.
 */
static BOOL check_relay_include( const char *module, int ordinal, const char *func )
{
    if (debug_relay_excludelist && check_list( module, ordinal, func, debug_relay_excludelist ))
        return FALSE;
    if (debug_relay_includelist && !check_list( module, ordinal, func, debug_relay_includelist ))
        return FALSE;
    return TRUE;
}


/***********************************************************************
 *           check_relay_from_module
 *
 * Check if calls from a given module must be included in the relay output.
 */
static BOOL check_relay_from_module( const WCHAR *module )
{
    static const WCHAR dllW[] = {'.','d','l','l',0 };
    const WCHAR **listitem;
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

        if (!strcmpiW( *listitem, module )) return !show;
        len = strlenW( *listitem );
        if (!strncmpiW( *listitem, module, len ) && !strcmpiW( module + len, dllW ))
            return !show;
    }
    return show;
}


/***********************************************************************
 *           find_exported_name
 *
 * Find the name of an exported function.
 */
static const char *find_exported_name( HMODULE module,
                                       IMAGE_EXPORT_DIRECTORY *exp, int ordinal )
{
    int i;
    const char *ret = NULL;

    WORD *ordptr = (WORD *)((char *)module + exp->AddressOfNameOrdinals);
    for (i = 0; i < exp->NumberOfNames; i++, ordptr++)
        if (*ordptr + exp->Base == ordinal) break;
    if (i < exp->NumberOfNames)
        ret = (char *)module + ((DWORD*)((char *)module + exp->AddressOfNames))[i];
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
    char *p;
    const char *name;
    int ordinal = 0;
    PLIST_ENTRY mark, entry;
    PLDR_MODULE mod = NULL;
    DWORD size;

    /* First find the module */

    mark = &NtCurrentTeb()->Peb->LdrData->InLoadOrderModuleList;
    for (entry = mark->Flink; entry != mark; entry = entry->Flink)
    {
        mod = CONTAINING_RECORD(entry, LDR_MODULE, InLoadOrderModuleList);
        if (!(mod->Flags & LDR_WINE_INTERNAL)) continue;
        exp = RtlImageDirectoryEntryToData( mod->BaseAddress, TRUE, IMAGE_DIRECTORY_ENTRY_EXPORT, &size );
        if (!exp) continue;
        debug = (DEBUG_ENTRY_POINT *)((char *)exp + size);
        if (debug <= relay && relay < debug + exp->NumberOfFunctions)
        {
            ordinal = relay - debug;
            break;
        }
    }

    /* Now find the function */

    strcpy( buffer, (char *)mod->BaseAddress + exp->Name );
    p = buffer + strlen(buffer);
    if (p > buffer + 4 && !strcasecmp( p - 4, ".dll" )) p -= 4;

    if ((name = find_exported_name( mod->BaseAddress, exp, ordinal + exp->Base )))
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
    context->Eip = *(DWORD *)context->Esp;
    context->Esp += sizeof(DWORD);
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
                              DWORD exp_size, FARPROC proc, const WCHAR *user )
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
void RELAY_SetupDLL( HMODULE module )
{
    IMAGE_EXPORT_DIRECTORY *exports;
    DEBUG_ENTRY_POINT *debug;
    DWORD *funcs;
    int i;
    const char *name;
    char *p, dllname[80];
    DWORD size;

    exports = RtlImageDirectoryEntryToData( module, TRUE, IMAGE_DIRECTORY_ENTRY_EXPORT, &size );
    if (!exports) return;
    debug = (DEBUG_ENTRY_POINT *)((char *)exports + size);
    funcs = (DWORD *)((char *)module + exports->AddressOfFunctions);
    strcpy( dllname, (char *)module + exports->Name );
    p = dllname + strlen(dllname) - 4;
    if (p > dllname && !strcasecmp( p, ".dll" )) *p = 0;

    for (i = 0; i < exports->NumberOfFunctions; i++, funcs++, debug++)
    {
        int on = 1;

        if (!debug->call) continue;  /* not a normal function */
        if (debug->call != 0xe8 && debug->call != 0xe9) break; /* not a debug thunk at all */

        name = find_exported_name( module, exports, i + exports->Base );
        on = check_relay_include( dllname, i + exports->Base, name );

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

        debug->orig = (FARPROC)((char *)module + (DWORD)*funcs);
        *funcs = (char *)debug - (char *)module;
    }
}


/***********************************************************************
 *          SNOOP_ShowDebugmsgSnoop
 *
 * Simple function to decide if a particular debugging message is
 * wanted.
 */
static BOOL SNOOP_ShowDebugmsgSnoop(const char *module, int ordinal, const char *func)
{
    if (debug_snoop_excludelist && check_list( module, ordinal, func, debug_snoop_excludelist ))
        return FALSE;
    if (debug_snoop_includelist && !check_list( module, ordinal, func, debug_snoop_includelist ))
        return FALSE;
    return TRUE;
}


/***********************************************************************
 *           SNOOP_SetupDLL
 *
 * Setup snoop debugging for a native dll.
 */
void SNOOP_SetupDLL(HMODULE hmod)
{
    SNOOP_DLL **dll = &firstdll;
    char *p, *name;
    void *addr;
    SIZE_T size;
    IMAGE_EXPORT_DIRECTORY *exports;

    exports = RtlImageDirectoryEntryToData( hmod, TRUE, IMAGE_DIRECTORY_ENTRY_EXPORT, &size );
    if (!exports) return;
    name = (char *)hmod + exports->Name;

    TRACE("hmod=%p, name=%s\n", hmod, name);

    while (*dll) {
        if ((*dll)->hmod == hmod)
        {
            /* another dll, loaded at the same address */
            addr = (*dll)->funs;
            size = (*dll)->nrofordinals * sizeof(SNOOP_FUN);
            NtFreeVirtualMemory(GetCurrentProcess(), &addr, &size, MEM_RELEASE);
            break;
        }
        dll = &((*dll)->next);
    }
    if (*dll)
        *dll = RtlReAllocateHeap(GetProcessHeap(),
                             HEAP_ZERO_MEMORY, *dll,
                             sizeof(SNOOP_DLL) + strlen(name));
    else
        *dll = RtlAllocateHeap(GetProcessHeap(),
                             HEAP_ZERO_MEMORY,
                             sizeof(SNOOP_DLL) + strlen(name));
    (*dll)->hmod	= hmod;
    (*dll)->ordbase = exports->Base;
    (*dll)->nrofordinals = exports->NumberOfFunctions;
    strcpy( (*dll)->name, name );
    p = (*dll)->name + strlen((*dll)->name) - 4;
    if (p > (*dll)->name && !strcasecmp( p, ".dll" )) *p = 0;

    size = exports->NumberOfFunctions * sizeof(SNOOP_FUN);
    NtAllocateVirtualMemory(GetCurrentProcess(), &addr, NULL, &size,
                            MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!addr) {
        RtlFreeHeap(ntdll_get_process_heap(),0,*dll);
        FIXME("out of memory\n");
        return;
    }
    (*dll)->funs = addr;
    memset((*dll)->funs,0,size);
}


/***********************************************************************
 *           SNOOP_GetProcAddress
 *
 * Return the proc address to use for a given function.
 */
FARPROC SNOOP_GetProcAddress( HMODULE hmod, IMAGE_EXPORT_DIRECTORY *exports,
                              DWORD exp_size, FARPROC origfun, DWORD ordinal )
{
        int i;
        const char *ename;
        WORD *ordinals;
        DWORD *names;
	SNOOP_DLL			*dll = firstdll;
	SNOOP_FUN			*fun;
        IMAGE_SECTION_HEADER *sec;

	if (!TRACE_ON(snoop)) return origfun;
	if (!*(LPBYTE)origfun) /* 0x00 is an imposs. opcode, poss. dataref. */
		return origfun;

        sec = RtlImageRvaToSection( RtlImageNtHeader(hmod), hmod, (char *)origfun - (char *)hmod );

        if (!sec || !(sec->Characteristics & IMAGE_SCN_CNT_CODE))
            return origfun;  /* most likely a data reference */

	while (dll) {
		if (hmod == dll->hmod)
			break;
		dll=dll->next;
	}
	if (!dll)	/* probably internal */
		return origfun;

        /* try to find a name for it */
        ename = NULL;
        names = (DWORD *)((char *)hmod + exports->AddressOfNames);
        ordinals = (WORD *)((char *)hmod + exports->AddressOfNameOrdinals);
        if (names) for (i = 0; i < exports->NumberOfNames; i++)
        {
            if (ordinals[i] == ordinal)
            {
                ename = (char *)hmod + names[i];
                break;
            }
        }
	if (!SNOOP_ShowDebugmsgSnoop(dll->name,ordinal,ename))
		return origfun;
	assert(ordinal < dll->nrofordinals);
	fun = dll->funs+ordinal;
	if (!fun->name)
	  {
	    fun->name = ename;
	    fun->lcall	= 0xe8;
	    /* NOTE: origreturn struct member MUST come directly after snoopentry */
	    fun->snoopentry	= (char*)SNOOP_Entry-((char*)(&fun->nrofargs));
	    fun->origfun	= origfun;
	    fun->nrofargs	= -1;
	  }
	return (FARPROC)&(fun->lcall);
}

static void SNOOP_PrintArg(DWORD x)
{
    int i,nostring;

    DPRINTF("%08lx",x);
    if (!HIWORD(x) || TRACE_ON(seh)) return; /* trivial reject to avoid faults */
    __TRY
    {
        LPBYTE s=(LPBYTE)x;
        i=0;nostring=0;
        while (i<80) {
            if (s[i]==0) break;
            if (s[i]<0x20) {nostring=1;break;}
            if (s[i]>=0x80) {nostring=1;break;}
            i++;
        }
        if (!nostring && i > 5)
            DPRINTF(" %s",debugstr_an((LPSTR)x,i));
        else  /* try unicode */
        {
            LPWSTR s=(LPWSTR)x;
            i=0;nostring=0;
            while (i<80) {
                if (s[i]==0) break;
                if (s[i]<0x20) {nostring=1;break;}
                if (s[i]>0x100) {nostring=1;break;}
                i++;
            }
            if (!nostring && i > 5) DPRINTF(" %s",debugstr_wn((LPWSTR)x,i));
        }
    }
    __EXCEPT(page_fault)
    {
    }
    __ENDTRY
}

#define CALLER1REF (*(DWORD*)context->Esp)

void WINAPI SNOOP_DoEntry( CONTEXT86 *context )
{
	DWORD		ordinal=0,entry = context->Eip - 5;
	SNOOP_DLL	*dll = firstdll;
	SNOOP_FUN	*fun = NULL;
	SNOOP_RETURNENTRIES	**rets = &firstrets;
	SNOOP_RETURNENTRY	*ret;
	int		i=0, max;

	while (dll) {
		if (	((char*)entry>=(char*)dll->funs)	&&
			((char*)entry<=(char*)(dll->funs+dll->nrofordinals))
		) {
			fun = (SNOOP_FUN*)entry;
			ordinal = fun-dll->funs;
			break;
		}
		dll=dll->next;
	}
	if (!dll) {
		FIXME("entrypoint 0x%08lx not found\n",entry);
		return; /* oops */
	}
	/* guess cdecl ... */
	if (fun->nrofargs<0) {
		/* Typical cdecl return frame is:
		 *     add esp, xxxxxxxx
		 * which has (for xxxxxxxx up to 255 the opcode "83 C4 xx".
		 * (after that 81 C2 xx xx xx xx)
		 */
		LPBYTE	reteip = (LPBYTE)CALLER1REF;

		if (reteip) {
			if ((reteip[0]==0x83)&&(reteip[1]==0xc4))
				fun->nrofargs=reteip[2]/4;
		}
	}


	while (*rets) {
		for (i=0;i<sizeof((*rets)->entry)/sizeof((*rets)->entry[0]);i++)
			if (!(*rets)->entry[i].origreturn)
				break;
		if (i!=sizeof((*rets)->entry)/sizeof((*rets)->entry[0]))
			break;
		rets = &((*rets)->next);
	}
	if (!*rets) {
                SIZE_T size = 4096;
                VOID* addr;

                NtAllocateVirtualMemory(GetCurrentProcess(), &addr, NULL, &size, 
                                        MEM_COMMIT | MEM_RESERVE,
                                        PAGE_EXECUTE_READWRITE);
                if (!addr) return;
                *rets = addr;
		memset(*rets,0,4096);
		i = 0;	/* entry 0 is free */
	}
	ret = &((*rets)->entry[i]);
	ret->lcall	= 0xe8;
	/* NOTE: origreturn struct member MUST come directly after snoopret */
	ret->snoopret	= ((char*)SNOOP_Return)-(char*)(&ret->origreturn);
	ret->origreturn	= (FARPROC)CALLER1REF;
	CALLER1REF	= (DWORD)&ret->lcall;
	ret->dll	= dll;
	ret->args	= NULL;
	ret->ordinal	= ordinal;
	ret->origESP	= context->Esp;

	context->Eip = (DWORD)fun->origfun;

	if (fun->name) DPRINTF("%04lx:CALL %s.%s(",GetCurrentThreadId(),dll->name,fun->name);
	else DPRINTF("%04lx:CALL %s.%ld(",GetCurrentThreadId(),dll->name,dll->ordbase+ordinal);
	if (fun->nrofargs>0) {
		max = fun->nrofargs; if (max>16) max=16;
		for (i=0;i<max;i++)
                {
                    SNOOP_PrintArg(*(DWORD*)(context->Esp + 4 + sizeof(DWORD)*i));
                    if (i<fun->nrofargs-1) DPRINTF(",");
                }
		if (max!=fun->nrofargs)
			DPRINTF(" ...");
	} else if (fun->nrofargs<0) {
		DPRINTF("<unknown, check return>");
		ret->args = RtlAllocateHeap(ntdll_get_process_heap(),
                                            0,16*sizeof(DWORD));
		memcpy(ret->args,(LPBYTE)(context->Esp + 4),sizeof(DWORD)*16);
	}
	DPRINTF(") ret=%08lx\n",(DWORD)ret->origreturn);
}


void WINAPI SNOOP_DoReturn( CONTEXT86 *context )
{
	SNOOP_RETURNENTRY	*ret = (SNOOP_RETURNENTRY*)(context->Eip - 5);
        SNOOP_FUN *fun = &ret->dll->funs[ret->ordinal];

	/* We haven't found out the nrofargs yet. If we called a cdecl
	 * function it is too late anyway and we can just set '0' (which
	 * will be the difference between orig and current ESP
	 * If stdcall -> everything ok.
	 */
	if (ret->dll->funs[ret->ordinal].nrofargs<0)
		ret->dll->funs[ret->ordinal].nrofargs=(context->Esp - ret->origESP-4)/4;
	context->Eip = (DWORD)ret->origreturn;
	if (ret->args) {
		int	i,max;

                if (fun->name)
                    DPRINTF("%04lx:RET  %s.%s(", GetCurrentThreadId(), ret->dll->name, fun->name);
                else
                    DPRINTF("%04lx:RET  %s.%ld(", GetCurrentThreadId(),
                            ret->dll->name,ret->dll->ordbase+ret->ordinal);

		max = fun->nrofargs;
		if (max>16) max=16;

		for (i=0;i<max;i++)
                {
                    SNOOP_PrintArg(ret->args[i]);
                    if (i<max-1) DPRINTF(",");
                }
		DPRINTF(") retval=%08lx ret=%08lx\n",
			context->Eax,(DWORD)ret->origreturn );
		RtlFreeHeap(ntdll_get_process_heap(),0,ret->args);
		ret->args = NULL;
	}
        else
        {
            if (fun->name)
		DPRINTF("%04lx:RET  %s.%s() retval=%08lx ret=%08lx\n",
			GetCurrentThreadId(),
			ret->dll->name, fun->name, context->Eax, (DWORD)ret->origreturn);
            else
		DPRINTF("%04lx:RET  %s.%ld() retval=%08lx ret=%08lx\n",
			GetCurrentThreadId(),
			ret->dll->name,ret->dll->ordbase+ret->ordinal,
			context->Eax, (DWORD)ret->origreturn);
        }
	ret->origreturn = NULL; /* mark as empty */
}

/* assembly wrappers that save the context */
__ASM_GLOBAL_FUNC( SNOOP_Entry,
                   "call " __ASM_NAME("__wine_call_from_32_regs") "\n\t"
                   ".long " __ASM_NAME("SNOOP_DoEntry") ",0" );
__ASM_GLOBAL_FUNC( SNOOP_Return,
                   "call " __ASM_NAME("__wine_call_from_32_regs") "\n\t"
                   ".long " __ASM_NAME("SNOOP_DoReturn") ",0" );

#else  /* __i386__ */

FARPROC RELAY_GetProcAddress( HMODULE module, IMAGE_EXPORT_DIRECTORY *exports,
                              DWORD exp_size, FARPROC proc, const WCHAR *user )
{
    return proc;
}

FARPROC SNOOP_GetProcAddress( HMODULE hmod, IMAGE_EXPORT_DIRECTORY *exports,
                              DWORD exp_size, FARPROC origfun, DWORD ordinal )
{
    return origfun;
}

void RELAY_SetupDLL( HMODULE module )
{
}

void SNOOP_SetupDLL( HMODULE hmod )
{
    FIXME("snooping works only on i386 for now.\n");
}

#endif /* __i386__ */
