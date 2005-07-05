/*
 * Copyright 1993 Robert J. Amstadt
 * Copyright 1995 Alexandre Julliard
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
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "wine/winbase16.h"
#include "winternl.h"
#include "kernel_private.h"
#include "kernel16_private.h"
#include "wine/unicode.h"
#include "wine/library.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(relay);

#ifdef __i386__

static const WCHAR **debug_relay_excludelist;
static const WCHAR **debug_relay_includelist;
static const WCHAR **debug_snoop_excludelist;
static const WCHAR **debug_snoop_includelist;

/* compare an ASCII and a Unicode string without depending on the current codepage */
inline static int strcmpiAW( const char *strA, const WCHAR *strW )
{
    while (*strA && (toupperW((unsigned char)*strA) == toupperW(*strW))) { strA++; strW++; }
    return toupperW((unsigned char)*strA) - toupperW(*strW);
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
 *           RELAY16_InitDebugLists
 *
 * Build the relay include/exclude function lists.
 */
void RELAY16_InitDebugLists(void)
{
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING name;
    char buffer[1024];
    HANDLE root, hkey;
    DWORD count;
    WCHAR *str;
    static const WCHAR configW[] = {'S','o','f','t','w','a','r','e','\\',
                                    'W','i','n','e','\\',
                                    'D','e','b','u','g',0};
    static const WCHAR RelayIncludeW[] = {'R','e','l','a','y','I','n','c','l','u','d','e',0};
    static const WCHAR RelayExcludeW[] = {'R','e','l','a','y','E','x','c','l','u','d','e',0};
    static const WCHAR SnoopIncludeW[] = {'S','n','o','o','p','I','n','c','l','u','d','e',0};
    static const WCHAR SnoopExcludeW[] = {'S','n','o','o','p','E','x','c','l','u','d','e',0};

    RtlOpenCurrentUser( KEY_ALL_ACCESS, &root );
    attr.Length = sizeof(attr);
    attr.RootDirectory = root;
    attr.ObjectName = &name;
    attr.Attributes = 0;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;
    RtlInitUnicodeString( &name, configW );

    /* @@ Wine registry key: HKCU\Software\Wine\Debug */
    if (NtOpenKey( &hkey, KEY_ALL_ACCESS, &attr )) hkey = 0;
    NtClose( root );
    if (!hkey) return;

    str = (WCHAR *)((KEY_VALUE_PARTIAL_INFORMATION *)buffer)->Data;
    RtlInitUnicodeString( &name, RelayIncludeW );
    if (!NtQueryValueKey( hkey, &name, KeyValuePartialInformation, buffer, sizeof(buffer), &count ))
    {
        debug_relay_includelist = build_list( str );
    }

    RtlInitUnicodeString( &name, RelayExcludeW );
    if (!NtQueryValueKey( hkey, &name, KeyValuePartialInformation, buffer, sizeof(buffer), &count ))
    {
        debug_relay_excludelist = build_list( str );
    }

    RtlInitUnicodeString( &name, SnoopIncludeW );
    if (!NtQueryValueKey( hkey, &name, KeyValuePartialInformation, buffer, sizeof(buffer), &count ))
    {
        debug_snoop_includelist = build_list( str );
    }

    RtlInitUnicodeString( &name, SnoopExcludeW );
    if (!NtQueryValueKey( hkey, &name, KeyValuePartialInformation, buffer, sizeof(buffer), &count ))
    {
        debug_snoop_excludelist = build_list( str );
    }
    NtClose( hkey );
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
            if (!strcmpiAW( ord_str, p + 1 )) return TRUE;
            if (func && !strcmpiAW( func, p + 1 )) return TRUE;
        }
        else  /* function only */
        {
            if (func && !strcmpiAW( func, *list )) return TRUE;
        }
    }
    return FALSE;
}


/***********************************************************************
 *           RELAY_ShowDebugmsgRelay
 *
 * Simple function to decide if a particular debugging message is
 * wanted.
 */
static BOOL RELAY_ShowDebugmsgRelay(const char *module, int ordinal, const char *func)
{
    if (debug_relay_excludelist && check_list( module, ordinal, func, debug_relay_excludelist ))
        return FALSE;
    if (debug_relay_includelist && !check_list( module, ordinal, func, debug_relay_includelist ))
        return FALSE;
    return TRUE;
}


/***********************************************************************
 *          SNOOP16_ShowDebugmsgSnoop
 *
 * Simple function to decide if a particular debugging message is
 * wanted.
 */
int SNOOP16_ShowDebugmsgSnoop(const char *module, int ordinal, const char *func)
{
    if (debug_snoop_excludelist && check_list( module, ordinal, func, debug_snoop_excludelist ))
        return FALSE;
    if (debug_snoop_includelist && !check_list( module, ordinal, func, debug_snoop_includelist ))
        return FALSE;
    return TRUE;
}


/***********************************************************************
 *           get_entry_point
 *
 * Return the ordinal, name, and type info corresponding to a CS:IP address.
 */
static const CALLFROM16 *get_entry_point( STACK16FRAME *frame, LPSTR module, LPSTR func, WORD *pOrd )
{
    WORD i, max_offset;
    register BYTE *p;
    NE_MODULE *pModule;
    ET_BUNDLE *bundle;
    ET_ENTRY *entry;

    if (!(pModule = NE_GetPtr( FarGetOwner16( GlobalHandle16( frame->module_cs ) ))))
        return NULL;

    max_offset = 0;
    *pOrd = 0;
    bundle = (ET_BUNDLE *)((BYTE *)pModule + pModule->ne_enttab);
    do
    {
        entry = (ET_ENTRY *)((BYTE *)bundle+6);
	for (i = bundle->first + 1; i <= bundle->last; i++)
        {
	    if ((entry->offs < frame->entry_ip)
	    && (entry->segnum == 1) /* code segment ? */
	    && (entry->offs >= max_offset))
            {
		max_offset = entry->offs;
		*pOrd = i;
            }
	    entry++;
        }
    } while ( (bundle->next)
	   && (bundle = (ET_BUNDLE *)((BYTE *)pModule+bundle->next)));

    /* Search for the name in the resident names table */
    /* (built-in modules have no non-resident table)   */

    p = (BYTE *)pModule + pModule->ne_restab;
    memcpy( module, p + 1, *p );
    module[*p] = 0;

    while (*p)
    {
        p += *p + 1 + sizeof(WORD);
        if (*(WORD *)(p + *p + 1) == *pOrd) break;
    }
    memcpy( func, p + 1, *p );
    func[*p] = 0;

    /* Retrieve entry point call structure */
    p = MapSL( MAKESEGPTR( frame->module_cs, frame->callfrom_ip ) );
    /* p now points to lret, get the start of CALLFROM16 structure */
    return (CALLFROM16 *)(p - (BYTE *)&((CALLFROM16 *)0)->lret);
}


typedef int (*CDECL_PROC)();
typedef int (WINAPI *PASCAL_PROC)();

/***********************************************************************
 *           call_cdecl_function
 */
static int call_cdecl_function( CDECL_PROC func, int nb_args, const int *args )
{
    int ret;
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
    case 17: ret = func(args[0],args[1],args[2],args[3],args[4],args[5],
                        args[6],args[7],args[8],args[9],args[10],args[11],
                        args[12],args[13],args[14],args[15],args[16]); break;
    case 18: ret = func(args[0],args[1],args[2],args[3],args[4],args[5],
                        args[6],args[7],args[8],args[9],args[10],args[11],
                        args[12],args[13],args[14],args[15],args[16],
                        args[17]); break;
    default:
        ERR( "Unsupported nb of args %d\n", nb_args );
        assert(FALSE);
        ret = 0;
        break;
    }
    return ret;
}


/***********************************************************************
 *           call_pascal_function
 */
static inline int call_pascal_function( PASCAL_PROC func, int nb_args, const int *args )
{
    int ret;
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
    case 17: ret = func(args[0],args[1],args[2],args[3],args[4],args[5],
                        args[6],args[7],args[8],args[9],args[10],args[11],
                        args[12],args[13],args[14],args[15],args[16]); break;
    case 18: ret = func(args[0],args[1],args[2],args[3],args[4],args[5],
                        args[6],args[7],args[8],args[9],args[10],args[11],
                        args[12],args[13],args[14],args[15],args[16],
                        args[17]); break;
    default:
        ERR( "Unsupported nb of args %d\n", nb_args );
        assert(FALSE);
        ret = 0;
        break;
    }
    return ret;
}


/***********************************************************************
 *           relay_call_from_16_no_debug
 *
 * Same as relay_call_from_16 but doesn't print any debug information.
 */
static int relay_call_from_16_no_debug( void *entry_point, unsigned char *args16, CONTEXT86 *context,
                                        const CALLFROM16 *call )
{
    int i, nb_args, args32[20];

    nb_args = 0;
    if (call->lret == 0xcb66)  /* cdecl */
    {
        for (i = 0; i < 20; i++, nb_args++)
        {
            int type = (call->arg_types[i / 10] >> (3 * (i % 10))) & 7;

            if (type == ARG_NONE) break;
            switch(type)
            {
            case ARG_WORD:
                args32[nb_args] = *(WORD *)args16;
                args16 += sizeof(WORD);
                break;
            case ARG_SWORD:
                args32[nb_args] = *(short *)args16;
                args16 += sizeof(WORD);
                break;
            case ARG_LONG:
            case ARG_SEGSTR:
                args32[nb_args] = *(int *)args16;
                args16 += sizeof(int);
                break;
            case ARG_PTR:
            case ARG_STR:
                args32[nb_args] = (int)MapSL( *(SEGPTR *)args16 );
                args16 += sizeof(SEGPTR);
                break;
            case ARG_VARARG:
                args32[nb_args] = (int)args16;
                break;
            default:
                break;
            }
        }
    }
    else  /* not cdecl */
    {
        /* Start with the last arg */
        args16 += call->nArgs;
        for (i = 0; i < 20; i++, nb_args++)
        {
            int type = (call->arg_types[i / 10] >> (3 * (i % 10))) & 7;

            if (type == ARG_NONE) break;
            switch(type)
            {
            case ARG_WORD:
                args16 -= sizeof(WORD);
                args32[nb_args] = *(WORD *)args16;
                break;
            case ARG_SWORD:
                args16 -= sizeof(WORD);
                args32[nb_args] = *(short *)args16;
                break;
            case ARG_LONG:
            case ARG_SEGSTR:
                args16 -= sizeof(int);
                args32[nb_args] = *(int *)args16;
                break;
            case ARG_PTR:
            case ARG_STR:
                args16 -= sizeof(SEGPTR);
                args32[nb_args] = (int)MapSL( *(SEGPTR *)args16 );
                break;
            default:
                break;
            }
        }
    }

    if (call->arg_types[0] & ARG_REGISTER) args32[nb_args++] = (int)context;

    SYSLEVEL_CheckNotLevel( 2 );

    if (call->lret == 0xcb66)  /* cdecl */
        return call_cdecl_function( entry_point, nb_args, args32 );
    else
        return call_pascal_function( entry_point, nb_args, args32 );
}


/***********************************************************************
 *           relay_call_from_16
 *
 * Replacement for the 16-bit relay functions when relay debugging is on.
 */
int relay_call_from_16( void *entry_point, unsigned char *args16, CONTEXT86 *context )
{
    STACK16FRAME *frame;
    WORD ordinal;
    int i, ret_val, nb_args, args32[20];
    char module[10], func[64];
    const CALLFROM16 *call;

    frame = CURRENT_STACK16;
    call = get_entry_point( frame, module, func, &ordinal );
    if (!TRACE_ON(relay) || !RELAY_ShowDebugmsgRelay( module, ordinal, func ))
        return relay_call_from_16_no_debug( entry_point, args16, context, call );

    DPRINTF( "%04lx:Call %s.%d: %s(",GetCurrentThreadId(), module, ordinal, func );

    nb_args = 0;
    if (call->lret == 0xcb66)  /* cdecl */
    {
        for (i = 0; i < 20; i++, nb_args++)
        {
            int type = (call->arg_types[i / 10] >> (3 * (i % 10))) & 7;

            if (type == ARG_NONE) break;
            if (i) DPRINTF( "," );
            switch(type)
            {
            case ARG_WORD:
                DPRINTF( "%04x", *(WORD *)args16 );
                args32[nb_args] = *(WORD *)args16;
                args16 += sizeof(WORD);
                break;
            case ARG_SWORD:
                DPRINTF( "%04x", *(WORD *)args16 );
                args32[nb_args] = *(short *)args16;
                args16 += sizeof(WORD);
                break;
            case ARG_LONG:
                DPRINTF( "%08x", *(int *)args16 );
                args32[nb_args] = *(int *)args16;
                args16 += sizeof(int);
                break;
            case ARG_PTR:
                DPRINTF( "%04x:%04x", *(WORD *)(args16+2), *(WORD *)args16 );
                args32[nb_args] = (int)MapSL( *(SEGPTR *)args16 );
                args16 += sizeof(SEGPTR);
                break;
            case ARG_STR:
                DPRINTF( "%08x %s", *(int *)args16,
                         debugstr_a( MapSL(*(SEGPTR *)args16 )));
                args32[nb_args] = (int)MapSL( *(SEGPTR *)args16 );
                args16 += sizeof(int);
                break;
            case ARG_SEGSTR:
                DPRINTF( "%04x:%04x %s", *(WORD *)(args16+2), *(WORD *)args16,
                         debugstr_a( MapSL(*(SEGPTR *)args16 )) );
                args32[nb_args] = *(SEGPTR *)args16;
                args16 += sizeof(SEGPTR);
                break;
            case ARG_VARARG:
                DPRINTF( "..." );
                args32[nb_args] = (int)args16;
                break;
            default:
                break;
            }
        }
    }
    else  /* not cdecl */
    {
        /* Start with the last arg */
        args16 += call->nArgs;
        for (i = 0; i < 20; i++, nb_args++)
        {
            int type = (call->arg_types[i / 10] >> (3 * (i % 10))) & 7;

            if (type == ARG_NONE) break;
            if (i) DPRINTF( "," );
            switch(type)
            {
            case ARG_WORD:
                args16 -= sizeof(WORD);
                args32[nb_args] = *(WORD *)args16;
                DPRINTF( "%04x", *(WORD *)args16 );
                break;
            case ARG_SWORD:
                args16 -= sizeof(WORD);
                args32[nb_args] = *(short *)args16;
                DPRINTF( "%04x", *(WORD *)args16 );
                break;
            case ARG_LONG:
                args16 -= sizeof(int);
                args32[nb_args] = *(int *)args16;
                DPRINTF( "%08x", *(int *)args16 );
                break;
            case ARG_PTR:
                args16 -= sizeof(SEGPTR);
                args32[nb_args] = (int)MapSL( *(SEGPTR *)args16 );
                DPRINTF( "%04x:%04x", *(WORD *)(args16+2), *(WORD *)args16 );
                break;
            case ARG_STR:
                args16 -= sizeof(int);
                args32[nb_args] = (int)MapSL( *(SEGPTR *)args16 );
                DPRINTF( "%08x %s", *(int *)args16,
                         debugstr_a( MapSL(*(SEGPTR *)args16 )));
                break;
            case ARG_SEGSTR:
                args16 -= sizeof(SEGPTR);
                args32[nb_args] = *(SEGPTR *)args16;
                DPRINTF( "%04x:%04x %s", *(WORD *)(args16+2), *(WORD *)args16,
                         debugstr_a( MapSL(*(SEGPTR *)args16 )) );
                break;
            case ARG_VARARG:
                DPRINTF( "..." );
                args32[nb_args] = (int)args16;
                break;
            default:
                break;
            }
        }
    }

    DPRINTF( ") ret=%04x:%04x ds=%04x\n", frame->cs, frame->ip, frame->ds );

    if (call->arg_types[0] & ARG_REGISTER)
    {
        args32[nb_args++] = (int)context;
        DPRINTF("     AX=%04x BX=%04x CX=%04x DX=%04x SI=%04x DI=%04x ES=%04x EFL=%08lx\n",
                (WORD)context->Eax, (WORD)context->Ebx, (WORD)context->Ecx,
                (WORD)context->Edx, (WORD)context->Esi, (WORD)context->Edi,
                (WORD)context->SegEs, context->EFlags );
    }

    SYSLEVEL_CheckNotLevel( 2 );

    if (call->lret == 0xcb66)  /* cdecl */
        ret_val = call_cdecl_function( entry_point, nb_args, args32 );
    else
        ret_val = call_pascal_function( entry_point, nb_args, args32 );

    SYSLEVEL_CheckNotLevel( 2 );

    DPRINTF( "%04lx:Ret  %s.%d: %s() ",GetCurrentThreadId(), module, ordinal, func );
    if (call->arg_types[0] & ARG_REGISTER)
    {
        DPRINTF("retval=none ret=%04x:%04x ds=%04x\n",
                (WORD)context->SegCs, LOWORD(context->Eip), (WORD)context->SegDs);
        DPRINTF("     AX=%04x BX=%04x CX=%04x DX=%04x SI=%04x DI=%04x ES=%04x EFL=%08lx\n",
                (WORD)context->Eax, (WORD)context->Ebx, (WORD)context->Ecx,
                (WORD)context->Edx, (WORD)context->Esi, (WORD)context->Edi,
                (WORD)context->SegEs, context->EFlags );
    }
    else
    {
        frame = CURRENT_STACK16;  /* might have be changed by the entry point */
        if (call->arg_types[0] & ARG_RET16)
            DPRINTF( "retval=%04x ret=%04x:%04x ds=%04x\n",
                     ret_val & 0xffff, frame->cs, frame->ip, frame->ds );
        else
            DPRINTF( "retval=%08x ret=%04x:%04x ds=%04x\n",
                     ret_val, frame->cs, frame->ip, frame->ds );
    }
    return ret_val;
}

#else /* __i386__ */

/*
 * Stubs for the CallTo16/CallFrom16 routines on non-Intel architectures
 * (these will never be called but need to be present to satisfy the linker ...)
 */

/***********************************************************************
 *		__wine_call_from_16_word (KERNEL32.@)
 */
WORD __wine_call_from_16_word()
{
    assert( FALSE );
}

/***********************************************************************
 *		__wine_call_from_16_long (KERNEL32.@)
 */
LONG __wine_call_from_16_long()
{
    assert( FALSE );
}

/***********************************************************************
 *		__wine_call_from_16_regs (KERNEL32.@)
 */
void __wine_call_from_16_regs()
{
    assert( FALSE );
}

DWORD WINAPI CALL32_CBClient( FARPROC proc, LPWORD args, DWORD *esi )
{ assert( FALSE ); }

DWORD WINAPI CALL32_CBClientEx( FARPROC proc, LPWORD args, DWORD *esi, INT *nArgs )
{ assert( FALSE ); }

#endif  /* __i386__ */
