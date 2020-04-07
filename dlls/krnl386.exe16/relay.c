/*
 * Copyright 1993 Robert J. Amstadt
 * Copyright 1995 Alexandre Julliard
 * Copyright 2002 Jukka Heinonen
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

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "wine/winbase16.h"
#include "winternl.h"
#include "kernel16_private.h"
#include "dosexe.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(relay);

/*
 * Magic DWORD used to check stack integrity.
 */
#define RELAY_MAGIC 0xabcdef00

/*
 * Memory block for temporary 16-bit stacks used with relay calls.
 */
typedef struct {
    DWORD inuse;          /* non-zero if stack block is in use */
    DWORD eip;            /* saved ip */
    DWORD seg_cs;         /* saved cs */
    DWORD esp;            /* saved sp */
    DWORD seg_ss;         /* saved ss */
    DWORD stack_bottom;   /* guard dword */
    BYTE  stack[256-7*4]; /* 16-bit stack */
    DWORD stack_top;      /* guard dword */
} RELAY_Stack16;


static const char **debug_relay_excludelist;
static const char **debug_relay_includelist;
static const char **debug_snoop_excludelist;
static const char **debug_snoop_includelist;


/***********************************************************************
 *           build_list
 *
 * Build a function list from a ';'-separated string.
 */
static const char **build_list( const WCHAR *buffer )
{
    int count = 1;
    const WCHAR *p = buffer;
    const char **ret;

    while ((p = wcschr( p, ';' )))
    {
        count++;
        p++;
    }
    /* allocate count+1 pointers, plus the space for a copy of the string */
    if ((ret = RtlAllocateHeap( GetProcessHeap(), 0,
                                (count + 1) * sizeof(char *) + (lstrlenW(buffer) + 1) )))
    {
        char *str = (char *)(ret + count + 1);
        char *p = str;

        while ((*str++ = *buffer++));
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

    RtlOpenCurrentUser( KEY_READ, &root );
    attr.Length = sizeof(attr);
    attr.RootDirectory = root;
    attr.ObjectName = &name;
    attr.Attributes = 0;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;
    RtlInitUnicodeString( &name, configW );

    /* @@ Wine registry key: HKCU\Software\Wine\Debug */
    if (NtOpenKey( &hkey, KEY_READ, &attr )) hkey = 0;
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
static BOOL check_list( const char *module, int ordinal, const char *func, const char **list )
{
    char ord_str[10];

    sprintf( ord_str, "%d", ordinal );
    for(; *list; list++)
    {
        const char *p = strrchr( *list, '.' );
        if (p && p > *list)  /* check module and function */
        {
            int len = p - *list;
            if (_strnicmp( module, *list, len-1 ) || module[len]) continue;
            if (p[1] == '*' && !p[2]) return TRUE;
            if (!strcmp( ord_str, p + 1 )) return TRUE;
            if (func && !stricmp( func, p + 1 )) return TRUE;
        }
        else  /* function only */
        {
            if (func && !stricmp( func, *list )) return TRUE;
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
BOOL SNOOP16_ShowDebugmsgSnoop(const char *module, int ordinal, const char *func)
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

    *pOrd = 0;
    if (!(pModule = NE_GetPtr( FarGetOwner16( GlobalHandle16( frame->module_cs ) ))))
        return NULL;

    max_offset = 0;
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
    return (CALLFROM16 *)(p - FIELD_OFFSET( CALLFROM16, ret ));
}


extern int call_entry_point( void *func, int nb_args, const int *args );
__ASM_GLOBAL_FUNC( call_entry_point,
                   "pushl %ebp\n\t"
                   __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")
                   __ASM_CFI(".cfi_rel_offset %ebp,0\n\t")
                   "movl %esp,%ebp\n\t"
                   __ASM_CFI(".cfi_def_cfa_register %ebp\n\t")
                   "pushl %esi\n\t"
                   __ASM_CFI(".cfi_rel_offset %esi,-4\n\t")
                   "pushl %edi\n\t"
                   __ASM_CFI(".cfi_rel_offset %edi,-8\n\t")
                   "movl 12(%ebp),%edx\n\t"
                   "shll $2,%edx\n\t"
                   "jz 1f\n\t"
                   "subl %edx,%esp\n\t"
                   "andl $~15,%esp\n\t"
                   "movl 12(%ebp),%ecx\n\t"
                   "movl 16(%ebp),%esi\n\t"
                   "movl %esp,%edi\n\t"
                   "cld\n\t"
                   "rep; movsl\n"
                   "1:\tcall *8(%ebp)\n\t"
                   "leal -8(%ebp),%esp\n\t"
                   "popl %edi\n\t"
                   __ASM_CFI(".cfi_same_value %edi\n\t")
                   "popl %esi\n\t"
                   __ASM_CFI(".cfi_same_value %esi\n\t")
                   "popl %ebp\n\t"
                   __ASM_CFI(".cfi_def_cfa %esp,4\n\t")
                   __ASM_CFI(".cfi_same_value %ebp\n\t")
                   "ret" )


/***********************************************************************
 *           relay_call_from_16_no_debug
 *
 * Same as relay_call_from_16 but doesn't print any debug information.
 */
static int relay_call_from_16_no_debug( void *entry_point, unsigned char *args16, CONTEXT *context,
                                        const CALLFROM16 *call )
{
    unsigned int i, j, nb_args = 0;
    int args32[20];

    /* look for the ret instruction */
    for (j = 0; j < ARRAY_SIZE(call->ret); j++)
        if (call->ret[j] == 0xca66 || call->ret[j] == 0xcb66) break;

    if (call->ret[j] == 0xcb66)  /* cdecl */
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
        args16 += call->ret[j + 1];
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

    if (!j)  /* register function */
        args32[nb_args++] = (int)context;

    SYSLEVEL_CheckNotLevel( 2 );

    return call_entry_point( entry_point, nb_args, args32 );
}


/***********************************************************************
 *           relay_call_from_16
 *
 * Replacement for the 16-bit relay functions when relay debugging is on.
 */
int relay_call_from_16( void *entry_point, unsigned char *args16, CONTEXT *context )
{
    STACK16FRAME *frame;
    WORD ordinal;
    unsigned int i, j, nb_args = 0;
    int ret_val, args32[20];
    char module[10], func[64];
    const CALLFROM16 *call;

    frame = CURRENT_STACK16;
    call = get_entry_point( frame, module, func, &ordinal );
    if (!TRACE_ON(relay) || !RELAY_ShowDebugmsgRelay( module, ordinal, func ))
        return relay_call_from_16_no_debug( entry_point, args16, context, call );

    TRACE( "\1Call %s.%d: %s(", module, ordinal, func );

    /* look for the ret instruction */
    for (j = 0; j < ARRAY_SIZE(call->ret); j++)
        if (call->ret[j] == 0xca66 || call->ret[j] == 0xcb66) break;

    if (call->ret[j] == 0xcb66)  /* cdecl */
    {
        for (i = 0; i < 20; i++, nb_args++)
        {
            int type = (call->arg_types[i / 10] >> (3 * (i % 10))) & 7;

            if (type == ARG_NONE) break;
            if (i) TRACE( "," );
            switch(type)
            {
            case ARG_WORD:
                TRACE( "%04x", *(WORD *)args16 );
                args32[nb_args] = *(WORD *)args16;
                args16 += sizeof(WORD);
                break;
            case ARG_SWORD:
                TRACE( "%04x", *(WORD *)args16 );
                args32[nb_args] = *(short *)args16;
                args16 += sizeof(WORD);
                break;
            case ARG_LONG:
                TRACE( "%08x", *(int *)args16 );
                args32[nb_args] = *(int *)args16;
                args16 += sizeof(int);
                break;
            case ARG_PTR:
                TRACE( "%04x:%04x", *(WORD *)(args16+2), *(WORD *)args16 );
                args32[nb_args] = (int)MapSL( *(SEGPTR *)args16 );
                args16 += sizeof(SEGPTR);
                break;
            case ARG_STR:
                TRACE( "%08x %s", *(int *)args16, debugstr_a( MapSL(*(SEGPTR *)args16 )));
                args32[nb_args] = (int)MapSL( *(SEGPTR *)args16 );
                args16 += sizeof(int);
                break;
            case ARG_SEGSTR:
                TRACE( "%04x:%04x %s", *(WORD *)(args16+2), *(WORD *)args16,
                       debugstr_a( MapSL(*(SEGPTR *)args16 )) );
                args32[nb_args] = *(SEGPTR *)args16;
                args16 += sizeof(SEGPTR);
                break;
            case ARG_VARARG:
                TRACE( "..." );
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
        args16 += call->ret[j + 1];
        for (i = 0; i < 20; i++, nb_args++)
        {
            int type = (call->arg_types[i / 10] >> (3 * (i % 10))) & 7;

            if (type == ARG_NONE) break;
            if (i) TRACE( "," );
            switch(type)
            {
            case ARG_WORD:
                args16 -= sizeof(WORD);
                args32[nb_args] = *(WORD *)args16;
                TRACE( "%04x", *(WORD *)args16 );
                break;
            case ARG_SWORD:
                args16 -= sizeof(WORD);
                args32[nb_args] = *(short *)args16;
                TRACE( "%04x", *(WORD *)args16 );
                break;
            case ARG_LONG:
                args16 -= sizeof(int);
                args32[nb_args] = *(int *)args16;
                TRACE( "%08x", *(int *)args16 );
                break;
            case ARG_PTR:
                args16 -= sizeof(SEGPTR);
                args32[nb_args] = (int)MapSL( *(SEGPTR *)args16 );
                TRACE( "%04x:%04x", *(WORD *)(args16+2), *(WORD *)args16 );
                break;
            case ARG_STR:
                args16 -= sizeof(int);
                args32[nb_args] = (int)MapSL( *(SEGPTR *)args16 );
                TRACE( "%08x %s", *(int *)args16, debugstr_a( MapSL(*(SEGPTR *)args16 )));
                break;
            case ARG_SEGSTR:
                args16 -= sizeof(SEGPTR);
                args32[nb_args] = *(SEGPTR *)args16;
                TRACE( "%04x:%04x %s", *(WORD *)(args16+2), *(WORD *)args16,
                       debugstr_a( MapSL(*(SEGPTR *)args16 )) );
                break;
            case ARG_VARARG:
                TRACE( "..." );
                args32[nb_args] = (int)args16;
                break;
            default:
                break;
            }
        }
    }

    if (!j)  /* register function */
    {
        args32[nb_args++] = (int)context;
        TRACE( ") ret=%04x:%04x ax=%04x bx=%04x cx=%04x dx=%04x si=%04x di=%04x ds=%04x es=%04x efl=%08x\n",
               frame->cs, frame->ip, (WORD)context->Eax, (WORD)context->Ebx, (WORD)context->Ecx,
               (WORD)context->Edx, (WORD)context->Esi, (WORD)context->Edi, (WORD)context->SegDs,
               (WORD)context->SegEs, context->EFlags );
    }
    else TRACE( ") ret=%04x:%04x ds=%04x\n", frame->cs, frame->ip, frame->ds );

    SYSLEVEL_CheckNotLevel( 2 );

    ret_val = call_entry_point( entry_point, nb_args, args32 );

    SYSLEVEL_CheckNotLevel( 2 );

    TRACE( "\1Ret  %s.%d: %s() ", module, ordinal, func );
    if (!j)  /* register function */
    {
        TRACE( "retval=none ret=%04x:%04x ax=%04x bx=%04x cx=%04x dx=%04x si=%04x di=%04x ds=%04x es=%04x efl=%08x\n",
               (WORD)context->SegCs, LOWORD(context->Eip), (WORD)context->Eax, (WORD)context->Ebx,
               (WORD)context->Ecx, (WORD)context->Edx, (WORD)context->Esi, (WORD)context->Edi,
               (WORD)context->SegDs, (WORD)context->SegEs, context->EFlags );
    }
    else
    {
        frame = CURRENT_STACK16;  /* might have be changed by the entry point */
        if (j == 1)  /* 16-bit return sequence */
            TRACE( "retval=%04x ret=%04x:%04x ds=%04x\n",
                     ret_val & 0xffff, frame->cs, frame->ip, frame->ds );
        else
            TRACE( "retval=%08x ret=%04x:%04x ds=%04x\n",
                     ret_val, frame->cs, frame->ip, frame->ds );
    }
    return ret_val;
}

/**********************************************************************
 *          RELAY_GetPointer
 *
 * Get pointer to stack block when given esp pointing to 16-bit stack
 * inside relay data segment.
 */
static RELAY_Stack16 *RELAY_GetPointer( DWORD offset )
{
    offset = offset / sizeof(RELAY_Stack16) * sizeof(RELAY_Stack16);
    return MapSL(MAKESEGPTR(relay_data_sel, offset));
}


/**********************************************************************
 *          RELAY_MakeShortContext
 *
 * Allocate separate 16-bit stack, make stack pointer point to this
 * stack and make code pointer point to stub that restores everything.
 * So, after this routine, SS and CS are guaranteed to be 16-bit.
 *
 * Note: This might be called from signal handler, so the stack
 *       allocation algorithm must be signal safe.
 */
static void RELAY_MakeShortContext( CONTEXT *context )
{
    DWORD offset = offsetof(RELAY_Stack16, stack_top);
    RELAY_Stack16 *stack = RELAY_GetPointer( 0 );

    while (stack->inuse && offset < DOSVM_RELAY_DATA_SIZE) {
        stack++;
        offset += sizeof(RELAY_Stack16);
    }

    if (offset >= DOSVM_RELAY_DATA_SIZE)
        ERR( "Too many nested interrupts!\n" );

    stack->inuse = 1;
    stack->eip = context->Eip;
    stack->seg_cs = context->SegCs;
    stack->esp = context->Esp;
    stack->seg_ss = context->SegSs;

    stack->stack_bottom = RELAY_MAGIC;
    stack->stack_top = RELAY_MAGIC;

    context->SegSs = relay_data_sel;
    context->Esp = offset;
    context->SegCs = relay_code_sel;
    context->Eip = 3;
}


/**********************************************************************
 *          RELAY_RelayStub
 *
 * This stub is called by __wine_call_from_16_regs in order to marshall
 * relay parameters.
 */
static void __stdcall RELAY_RelayStub( DOSRELAY proc, unsigned char *args, CONTEXT *context )
{
    if (proc)
    {
        RELAY_Stack16 *stack = RELAY_GetPointer( context->Esp );

        DWORD old_seg_cs = context->SegCs;
        DWORD old_eip    = context->Eip;
        DWORD old_seg_ss = context->SegSs;
        DWORD old_esp    = context->Esp;

        context->SegCs = stack->seg_cs;
        context->Eip   = stack->eip;
        context->SegSs = stack->seg_ss;
        context->Esp   = stack->esp;

        proc( context, *(LPVOID *)args );

        stack->seg_cs = context->SegCs;
        stack->eip    = context->Eip;
        stack->seg_ss = context->SegSs;
        stack->esp    = context->Esp;

        context->SegCs = old_seg_cs;
        context->Eip   = old_eip;
        context->SegSs = old_seg_ss;
        context->Esp   = old_esp;
    }
}


/**********************************************************************
 *          DOSVM_RelayHandler
 *
 * Restore saved code and stack pointers and release stack block.
 */
void DOSVM_RelayHandler( CONTEXT *context )
{
    RELAY_Stack16 *stack = RELAY_GetPointer( context->Esp );

    context->SegSs = stack->seg_ss;
    context->Esp = stack->esp;
    context->SegCs = stack->seg_cs;
    context->Eip = stack->eip;

    if (!stack->inuse ||
        stack->stack_bottom != RELAY_MAGIC ||
        stack->stack_top != RELAY_MAGIC)
        ERR( "Stack corrupted!\n" );

    stack->inuse = 0;
}


/**********************************************************************
 *          DOSVM_BuildCallFrame
 *
 * Modifies the context so that return to context calls DOSRELAY and
 * only after return from DOSRELAY the original context will be returned to.
 */
void DOSVM_BuildCallFrame( CONTEXT *context, DOSRELAY relay, LPVOID data )
{
    WORD  code_sel = relay_code_sel;

    /*
     * Allocate separate stack for relay call.
     */
    RELAY_MakeShortContext( context );

    /*
     * Build call frame.
     */
    PUSH_WORD16( context, HIWORD(data) );            /* argument.hiword */
    PUSH_WORD16( context, LOWORD(data) );            /* argument.loword */
    PUSH_WORD16( context, context->SegCs );          /* STACK16FRAME.cs */
    PUSH_WORD16( context, LOWORD(context->Eip) );    /* STACK16FRAME.ip */
    PUSH_WORD16( context, LOWORD(context->Ebp) );    /* STACK16FRAME.bp */
    PUSH_WORD16( context, HIWORD(relay) );           /* STACK16FRAME.entry_point.hiword */
    PUSH_WORD16( context, LOWORD(relay) );           /* STACK16FRAME.entry_point.loword */
    PUSH_WORD16( context, 0 );                       /* STACK16FRAME.entry_ip */
    PUSH_WORD16( context, HIWORD(RELAY_RelayStub) ); /* STACK16FRAME.relay.hiword */
    PUSH_WORD16( context, LOWORD(RELAY_RelayStub) ); /* STACK16FRAME.relay.loword */
    PUSH_WORD16( context, 0 );                       /* STACK16FRAME.module_cs.hiword */
    PUSH_WORD16( context, code_sel );                /* STACK16FRAME.module_cs.loword */
    PUSH_WORD16( context, 0 );                       /* STACK16FRAME.callfrom_ip.hiword */
    PUSH_WORD16( context, 0 );                       /* STACK16FRAME.callfrom_ip.loword */

    /*
     * Adjust code pointer.
     */
    context->SegCs = get_cs();
    context->Eip = (DWORD)__wine_call_from_16_regs;
}
