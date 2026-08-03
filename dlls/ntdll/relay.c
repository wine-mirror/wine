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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <assert.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"
#include "wine/exception.h"
#include "ntdll_misc.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(relay);

#if (defined(__i386__) || defined(__x86_64__) || defined(__arm__) || defined(__aarch64__)) && !defined(__arm64ec__)

struct relay_descr  /* descriptor for a module */
{
    ULONG_PTR           magic;               /* signature */
    void               *relay_call;          /* functions to call from relay thunks */
    void               *private;             /* reserved for the relay code private data */
    const char         *entry_point_base;    /* base address of entry point thunks */
    const unsigned int *entry_point_offsets; /* offsets of entry points thunks */
    const char         *args_string;         /* string describing the arguments */
};

struct relay_descr_rva  /* RVA to the descriptor for PE dlls */
{
    DWORD magic;
    DWORD descr;
};

#define RELAY_DESCR_MAGIC  0xdeb90002
#define IS_INTARG(x)       (((ULONG_PTR)(x) >> 16) == 0)

/* private data built at dll load time */

struct relay_entry_point
{
    void       *orig_func;    /* original entry point function */
    const char *name;         /* function name (if any) */
};

struct relay_private_data
{
    HMODULE                  module;            /* module handle of this dll */
    unsigned int             base;              /* ordinal base */
    char                     dllname[40];       /* dll name (without .dll extension) */
    struct relay_entry_point entry_points[1];   /* list of dll entry points */
};

static const WCHAR **debug_relay_excludelist;
static const WCHAR **debug_relay_includelist;
static const WCHAR **debug_snoop_excludelist;
static const WCHAR **debug_snoop_includelist;
static const WCHAR **debug_from_relay_excludelist;
static const WCHAR **debug_from_relay_includelist;
static const WCHAR **debug_from_snoop_excludelist;
static const WCHAR **debug_from_snoop_includelist;

static RTL_RUN_ONCE init_once = RTL_RUN_ONCE_INIT;

/* compare an ASCII and a Unicode string without depending on the current codepage */
static inline int strcmpAW( const char *strA, const WCHAR *strW )
{
    while (*strA && ((unsigned char)*strA == *strW)) { strA++; strW++; }
    return (unsigned char)*strA - *strW;
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

    while ((p = wcschr( p, ';' )))
    {
        count++;
        p++;
    }
    /* allocate count+1 pointers, plus the space for a copy of the string */
    if ((ret = RtlAllocateHeap( GetProcessHeap(), 0,
                                (count+1) * sizeof(WCHAR*) + (wcslen(buffer)+1) * sizeof(WCHAR) )))
    {
        WCHAR *str = (WCHAR *)(ret + count + 1);
        WCHAR *q = str;

        wcscpy( str, buffer );
        count = 0;
        for (;;)
        {
            ret[count++] = q;
            if (!(q = wcschr( q, ';' ))) break;
            *q++ = 0;
        }
        ret[count++] = NULL;
    }
    return ret;
}

/***********************************************************************
 *           load_list_value
 *
 * Load a function list from a registry value.
 */
static const WCHAR **load_list( HKEY hkey, const WCHAR *value )
{
    char initial_buffer[4096];
    char *buffer = initial_buffer;
    DWORD count;
    NTSTATUS status;
    UNICODE_STRING name;
    const WCHAR **list = NULL;

    RtlInitUnicodeString( &name, value );
    status = NtQueryValueKey( hkey, &name, KeyValuePartialInformation, buffer, sizeof(initial_buffer), &count );
    if (status == STATUS_BUFFER_OVERFLOW)
    {
        buffer = RtlAllocateHeap( GetProcessHeap(), 0, count );
        status = NtQueryValueKey( hkey, &name, KeyValuePartialInformation, buffer, count, &count );
    }
    if (status == STATUS_SUCCESS)
    {
        WCHAR *str = (WCHAR *)((KEY_VALUE_PARTIAL_INFORMATION *)buffer)->Data;
        list = build_list( str );
        if (list) TRACE( "%s = %s\n", debugstr_w(value), debugstr_w(str) );
    }

    if (buffer != initial_buffer) RtlFreeHeap( GetProcessHeap(), 0, buffer );
    return list;
}

/***********************************************************************
 *           init_debug_lists
 *
 * Build the relay include/exclude function lists.
 */
static DWORD WINAPI init_debug_lists( RTL_RUN_ONCE *once, void *param, void **context )
{
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING name = RTL_CONSTANT_STRING( L"Software\\Wine\\Debug" );
    HANDLE root, hkey;

    RtlOpenCurrentUser( KEY_ALL_ACCESS, &root );
    InitializeObjectAttributes( &attr, &name, OBJ_CASE_INSENSITIVE, root, NULL );

    /* @@ Wine registry key: HKCU\Software\Wine\Debug */
    if (NtOpenKey( &hkey, KEY_ALL_ACCESS, &attr )) hkey = 0;
    NtClose( root );
    if (!hkey) return TRUE;

    debug_relay_includelist = load_list( hkey, L"RelayInclude" );
    debug_relay_excludelist = load_list( hkey, L"RelayExclude" );
    debug_snoop_includelist = load_list( hkey, L"SnoopInclude" );
    debug_snoop_excludelist = load_list( hkey, L"SnoopExclude" );
    debug_from_relay_includelist = load_list( hkey, L"RelayFromInclude" );
    debug_from_relay_excludelist = load_list( hkey, L"RelayFromExclude" );
    debug_from_snoop_includelist = load_list( hkey, L"SnoopFromInclude" );
    debug_from_snoop_excludelist = load_list( hkey, L"SnoopFromExclude" );

    NtClose( hkey );
    return TRUE;
}


/***********************************************************************
 *           check_list
 *
 * Check if a given module and function is in the list.
 */
static BOOL check_list( const WCHAR *module, int ordinal, const char *func, const WCHAR *const *list )
{
    char ord_str[10];

    sprintf( ord_str, "%d", ordinal );
    for(; *list; list++)
    {
        const WCHAR *p = wcsrchr( *list, '.' );
        if (p && p > *list)  /* check module and function */
        {
            int len = p - *list;
            if (wcsnicmp( module, *list, len - 1 ) || module[len]) continue;
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
static BOOL check_relay_include( const WCHAR *module, int ordinal, const char *func )
{
    if (debug_relay_excludelist && check_list( module, ordinal, func, debug_relay_excludelist ))
        return FALSE;
    if (debug_relay_includelist && !check_list( module, ordinal, func, debug_relay_includelist ))
        return FALSE;
    return TRUE;
}

/***********************************************************************
 *           check_from_module
 *
 * Check if calls from a given module must be included in the relay/snoop output,
 * given the exclusion and inclusion lists.
 */
static BOOL check_from_module( const WCHAR **includelist, const WCHAR **excludelist, const WCHAR *module )
{
    const WCHAR **listitem;
    BOOL show;

    if (!module) return TRUE;
    if (!includelist && !excludelist) return TRUE;
    if (excludelist)
    {
        show = TRUE;
        listitem = excludelist;
    }
    else
    {
        show = FALSE;
        listitem = includelist;
    }
    for(; *listitem; listitem++)
    {
        int len;

        if (!wcsicmp( *listitem, module )) return !show;
        len = wcslen( *listitem );
        if (!wcsnicmp( *listitem, module, len ) && !wcsicmp( module + len, L".dll" ))
            return !show;
    }
    return show;
}


static BOOL is_ret_val( char type )
{
    return type >= 'A' && type <= 'Z';
}

static const char *func_name( struct relay_private_data *data, unsigned int ordinal )
{
    struct relay_entry_point *entry_point = data->entry_points + ordinal;

    if (entry_point->name)
        return wine_dbg_sprintf( "%s.%s", data->dllname, entry_point->name );
    else
        return wine_dbg_sprintf( "%s.%u", data->dllname, data->base + ordinal );
}

static void trace_string_a( INT_PTR ptr )
{
    if (!IS_INTARG( ptr )) TRACE( "%08Ix %s", ptr, debugstr_a( (char *)ptr ));
    else TRACE( "%08Ix", ptr );
}

static void trace_string_w( INT_PTR ptr )
{
    if (!IS_INTARG( ptr )) TRACE( "%08Ix %s", ptr, debugstr_w( (WCHAR *)ptr ));
    else TRACE( "%08Ix", ptr );
}

#ifdef __i386__

/***********************************************************************
 *           relay_trace_entry
 */
void * WINAPI relay_trace_entry( struct relay_descr *descr, unsigned int idx,
                                 const DWORD *stack, unsigned int *nb_args )
{
    WORD ordinal = LOWORD(idx);
    const char *arg_types = descr->args_string + HIWORD(idx);
    struct relay_private_data *data = descr->private;
    struct relay_entry_point *entry_point = data->entry_points + ordinal;
    unsigned int i, pos;

    TRACE( "\1Call %s(", func_name( data, ordinal ));

    for (i = pos = 0; !is_ret_val( arg_types[i] ); i++)
    {
        switch (arg_types[i])
        {
        case 'j': /* int64 */
            TRACE( "%lx%08lx", stack[pos+1], stack[pos] );
            pos += 2;
            break;
        case 'k': /* int128 */
            TRACE( "{%08lx,%08lx,%08lx,%08lx}", stack[pos], stack[pos+1], stack[pos+2], stack[pos+3] );
            pos += 4;
            break;
        case 's': /* str */
            trace_string_a( stack[pos++] );
            break;
        case 'w': /* wstr */
            trace_string_w( stack[pos++] );
            break;
        case 'f': /* float */
            TRACE( "%g", *(const float *)&stack[pos++] );
            break;
        case 'd': /* double */
            TRACE( "%g", *(const double *)&stack[pos] );
            pos += 2;
            break;
        case 'i': /* long */
        default:
            TRACE( "%08lx", stack[pos++] );
            break;
        }
        if (!is_ret_val( arg_types[i+1] )) TRACE( "," );
    }
    *nb_args = pos;
    if (arg_types[0] == 't')
    {
        *nb_args |= 0x80000000;  /* thiscall/fastcall */
        if (arg_types[1] == 't') *nb_args |= 0x40000000;  /* fastcall */
    }
    TRACE( ") ret=%08lx\n", stack[-1] );
    return entry_point->orig_func;
}

/***********************************************************************
 *           relay_trace_exit
 */
void WINAPI relay_trace_exit( struct relay_descr *descr, unsigned int idx,
                              void *retaddr, LONGLONG retval )
{
    const char *arg_types = descr->args_string + HIWORD(idx);

    TRACE( "\1Ret  %s()", func_name( descr->private, LOWORD(idx) ));

    while (!is_ret_val( *arg_types )) arg_types++;
    if (*arg_types == 'J')  /* int64 return value */
        TRACE( " retval=%08x%08x ret=%08x\n",
               (UINT)(retval >> 32), (UINT)retval, (UINT)retaddr );
    else
        TRACE( " retval=%08x ret=%08x\n", (UINT)retval, (UINT)retaddr );
}

extern LONGLONG WINAPI relay_call( struct relay_descr *descr, unsigned int idx );
__ASM_STDCALL_FUNC( relay_call, 8,
                   "pushl %ebp\n\t"
                   __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")
                   __ASM_CFI(".cfi_rel_offset %ebp,0\n\t")
                   "movl %esp,%ebp\n\t"
                   __ASM_CFI(".cfi_def_cfa_register %ebp\n\t")
                   "pushl %esi\n\t"
                  __ASM_CFI(".cfi_rel_offset %esi,-4\n\t")
                   "pushl %edi\n\t"
                  __ASM_CFI(".cfi_rel_offset %edi,-8\n\t")
                   "pushl %ecx\n\t"
                  __ASM_CFI(".cfi_rel_offset %ecx,-12\n\t")
                   /* trace the parameters */
                   "pushl %eax\n\t"
                   "pushl %esp\n\t"  /* number of args return ptr */
                   "leal 20(%ebp),%esi\n\t"  /* stack */
                   "pushl %esi\n\t"
                   "pushl 12(%ebp)\n\t"
                   "pushl 8(%ebp)\n\t"
                   "call " __ASM_STDCALL("relay_trace_entry",16) "\n\t"
                   /* copy the arguments*/
                   "movzwl -16(%ebp),%ecx\n\t"  /* number of args */
                   "jecxz 1f\n\t"
                   "leal 0(,%ecx,4),%edx\n\t"
                   "subl %edx,%esp\n\t"
                   "andl $~15,%esp\n\t"
                   "movl %esp,%edi\n\t"
                   "cld\n\t"
                   "rep; movsl\n\t"
                   "testl $0x80000000,-16(%ebp)\n\t"  /* thiscall */
                   "jz 1f\n\t"
                   "popl %ecx\n\t"
                   "testl $0x40000000,-16(%ebp)\n\t"  /* fastcall */
                   "jz 1f\n\t"
                   "popl %edx\n"
                   /* call the entry point */
                   "1:\tcall *%eax\n\t"
                   "movl %eax,%esi\n\t"
                   "movl %edx,%edi\n\t"
                   /* trace the return value */
                   "leal -20(%ebp),%esp\n\t"
                   "pushl %edx\n\t"
                   "pushl %eax\n\t"
                   "pushl 16(%ebp)\n\t"
                   "pushl 12(%ebp)\n\t"
                   "pushl 8(%ebp)\n\t"
                   "call " __ASM_STDCALL("relay_trace_exit",20) "\n\t"
                   /* restore return value and return */
                   "leal -12(%ebp),%esp\n\t"
                   "movl %esi,%eax\n\t"
                   "movl %edi,%edx\n\t"
                   "popl %ecx\n\t"
                   __ASM_CFI(".cfi_same_value %ecx\n\t")
                   "popl %edi\n\t"
                   __ASM_CFI(".cfi_same_value %edi\n\t")
                   "popl %esi\n\t"
                   __ASM_CFI(".cfi_same_value %esi\n\t")
                   "popl %ebp\n\t"
                   __ASM_CFI(".cfi_def_cfa %esp,4\n\t")
                   __ASM_CFI(".cfi_same_value %ebp\n\t")
                   "ret $8" )

#elif defined(__arm__)

/***********************************************************************
 *           relay_trace_entry
 */
void * WINAPI relay_trace_entry( struct relay_descr *descr, unsigned int idx,
                                 const DWORD *stack, unsigned int *nb_args )
{
    WORD ordinal = LOWORD(idx);
    const char *arg_types = descr->args_string + HIWORD(idx);
    struct relay_private_data *data = descr->private;
    struct relay_entry_point *entry_point = data->entry_points + ordinal;
    unsigned int i, pos;
    unsigned int float_pos = 0, double_pos = 0;
    const union fpregs { float s[16]; double d[8]; } *fpstack = (const union fpregs *)stack - 1;

    TRACE( "\1Call %s(", func_name( data, ordinal ));

    for (i = pos = 0; !is_ret_val( arg_types[i] ); i++)
    {
        switch (arg_types[i])
        {
        case 'j': /* int64 */
            pos = (pos + 1) & ~1;
            TRACE( "%lx%08lx", stack[pos+1], stack[pos] );
            pos += 2;
            break;
        case 'k': /* int128 */
            TRACE( "{%08lx,%08lx,%08lx,%08lx}", stack[pos], stack[pos+1], stack[pos+2], stack[pos+3] );
            pos += 4;
            break;
        case 's': /* str */
            trace_string_a( stack[pos++] );
            break;
        case 'w': /* wstr */
            trace_string_w( stack[pos++] );
            break;
        case 'f': /* float */
            if (!(float_pos % 2)) float_pos = max( float_pos, double_pos * 2 );
            if (float_pos < 16)
            {
                TRACE( "%g", fpstack->s[float_pos++] );
                break;
            }
            TRACE( "%g", *(const float *)&stack[pos++] );
            break;
        case 'd': /* double */
            double_pos = max( (float_pos + 1) / 2, double_pos );
            if (double_pos < 8)
            {
                TRACE( "%g", fpstack->d[double_pos++] );
                break;
            }
            pos = (pos + 1) & ~1;
            TRACE( "%g", *(const double *)&stack[pos] );
            pos += 2;
            break;
        case 'i': /* long */
        default:
            TRACE( "%08lx", stack[pos++] );
            break;
        }
        if (!is_ret_val( arg_types[i+1] )) TRACE( "," );
    }

    if (float_pos || double_pos)
    {
        pos |= 0x80000000;
        stack = (const DWORD *)fpstack;  /* retaddr is below the fp regs */
    }
    *nb_args = pos;
    TRACE( ") ret=%08lx\n", stack[-1] );
    return entry_point->orig_func;
}

/***********************************************************************
 *           relay_trace_exit
 */
void WINAPI relay_trace_exit( struct relay_descr *descr, unsigned int idx,
                              DWORD retaddr, LONGLONG retval )
{
    const char *arg_types = descr->args_string + HIWORD(idx);

    TRACE( "\1Ret  %s()", func_name( descr->private, LOWORD(idx) ));

    while (!is_ret_val( *arg_types )) arg_types++;
    if (*arg_types == 'J')  /* int64 return value */
        TRACE( " retval=%08x%08x ret=%08lx\n",
               (UINT)(retval >> 32), (UINT)retval, retaddr );
    else
        TRACE( " retval=%08x ret=%08lx\n", (UINT)retval, retaddr );
}

extern LONGLONG WINAPI relay_call( struct relay_descr *descr, unsigned int idx, const DWORD *stack );
__ASM_GLOBAL_FUNC( relay_call,
                   "push {r4-r8,lr}\n\t"
                   "sub sp, #16\n\t"
                   "mov r6, r2\n\t"
                   "add r3, sp, #12\n\t"
                   "mov r7, r0\n\t"
                   "mov r8, r1\n\t"
                   "bl relay_trace_entry\n\t"
                   "mov ip, r0\n\t"  /* entry point */
                   "mov r5, sp\n\t"
                   "ldr r1, [sp, #12]\n\t"  /* number of args */
                   "lsl r3, r1, #2\n\t"
                   "subs r3, #16\n\t"   /* first 4 args are in registers */
                   "ble 2f\n\t"
                   "add r3, #7\n\t"
                   "and r3, #~7\n"
                   "sub sp, r3\n\t"
                   "add r2, r6, #16\n\t"   /* skip r0-r3 */
                   "1:\tsubs r3, r3, #4\n\t"
                   "ldr r0, [r2, r3]\n\t"
                   "str r0, [sp, r3]\n\t"
                   "bgt 1b\n"
                   "2:\t"
                   "tst r1, #0x80000000\n\t"
                   "ldm r6, {r0-r3}\n\t"
                   "it ne\n\t"
                   "vldmdbne r6!, {s0-s15}\n\t"
                   "blx ip\n\t"
                   "mov sp, r5\n\t"
                   "ldr r2, [r6, #-4]\n\t"  /* retaddr */
                   "mov r4, r0\n\t"
                   "mov r5, r1\n\t"
                   "mov r0, r7\n\t"
                   "mov r1, r8\n\t"
                   "str r4, [sp]\n\t"
                   "str r5, [sp, #4]\n\t"
                   "vstr d0, [sp, #8]\n\t"  /* preserve floating point retval */
                   "bl relay_trace_exit\n\t"
                   "vldr d0, [sp, #8]\n\t"
                   "mov r0, r4\n\t"
                   "mov r1, r5\n\t"
                   "add sp, #16\n\t"
                   "pop {r4-r8,pc}" )

#elif defined(__aarch64__)

/***********************************************************************
 *           relay_trace_entry
 */
void * WINAPI relay_trace_entry( struct relay_descr *descr, unsigned int idx,
                                 const INT_PTR *stack, unsigned int *nb_args )
{
    WORD ordinal = LOWORD(idx);
    const char *arg_types = descr->args_string + HIWORD(idx);
    struct relay_private_data *data = descr->private;
    struct relay_entry_point *entry_point = data->entry_points + ordinal;
    unsigned int i;

    TRACE( "\1Call %s(", func_name( data, ordinal ));

    for (i = 0; !is_ret_val( arg_types[i] ); i++)
    {
        switch (arg_types[i])
        {
        case 's': /* str */
            trace_string_a( stack[i] );
            break;
        case 'w': /* wstr */
            trace_string_w( stack[i] );
            break;
        case 'i': /* long */
        default:
            TRACE( "%08Ix", stack[i] );
            break;
        }
        if (!is_ret_val( arg_types[i + 1] )) TRACE( "," );
    }
    *nb_args = i;
    TRACE( ") ret=%08Ix\n", stack[-1] );
    return entry_point->orig_func;
}

/***********************************************************************
 *           relay_trace_exit
 */
void WINAPI relay_trace_exit( struct relay_descr *descr, unsigned int idx,
                              INT_PTR retaddr, INT_PTR retval )
{
    TRACE( "\1Ret  %s() retval=%08Ix ret=%08Ix\n",
           func_name( descr->private, LOWORD(idx) ), retval, retaddr );
}

extern LONGLONG CDECL call_entry_point( void *func, int nb_args, const INT_PTR *args );
__ASM_GLOBAL_FUNC( call_entry_point,
                   "stp x29, x30, [SP,#-16]!\n\t"
                   ".seh_save_fplr_x 16\n\t"
                   "mov x29, SP\n\t"
                   ".seh_set_fp\n\t"
                   ".seh_endprologue\n\t"
                   "ldr x8, [x2, #-32]\n\t"
                   "mov x9, x0\n\t"
                   "cbz w1, 2f\n\t"
                   "mov w10, w1\n\t"
                   "mov x11, x2\n\t"
                   "mov x12, #0\n\t"
                   "ldp x0, x1, [x11], #16\n\t"
                   "add w12, w12, #2\n\t"
                   "cmp w12, w10\n\t"
                   "b.hs 2f\n\t"
                   "ldp x2, x3, [x11], #16\n\t"
                   "add w12, w12, #2\n\t"
                   "cmp w12, w10\n\t"
                   "b.hs 2f\n\t"
                   "ldp x4, x5, [x11], #16\n\t"
                   "add w12, w12, #2\n\t"
                   "cmp w12, w10\n\t"
                   "b.hs 2f\n\t"
                   "ldp x6, x7, [x11], #16\n\t"
                   "add w12, w12, #2\n\t"
                   "cmp w12, w10\n\t"
                   "b.hs 2f\n\t"
                   "sub w12, w10, #8\n\t"
                   "lsl w12, w12, #3\n\t"
                   "sub SP, SP, w12, uxtw\n\t"
                   "tbz w12, #3, 1f\n\t"
                   "sub SP, SP, #8\n\t"
                   "1: sub w12, w12, #8\n\t"
                   "ldr x13, [x11, x12]\n\t"
                   "str x13, [SP, x12]\n\t"
                   "cbnz w12, 1b\n\t"
                   "2: blr x9\n\t"
                   "mov SP, x29\n\t"
                   "ldp x29, x30, [SP], #16\n\t"
                   "ret\n" )

static LONGLONG WINAPI relay_call( struct relay_descr *descr, unsigned int idx, const INT_PTR *stack )
{
    unsigned int nb_args;
    void *func = relay_trace_entry( descr, idx, stack, &nb_args );
    LONGLONG ret = call_entry_point( func, nb_args, stack );
    relay_trace_exit( descr, idx, stack[-1], ret );
    return ret;
}

#elif defined(__x86_64__)

/***********************************************************************
 *           relay_trace_entry
 */
void * WINAPI relay_trace_entry( struct relay_descr *descr, unsigned int idx,
                                 const INT_PTR *stack, unsigned int *nb_args )
{
    WORD ordinal = LOWORD(idx);
    const char *arg_types = descr->args_string + HIWORD(idx);
    struct relay_private_data *data = descr->private;
    struct relay_entry_point *entry_point = data->entry_points + ordinal;
    unsigned int i;

    TRACE( "\1Call %s(", func_name( data, ordinal ));

    for (i = 0; !is_ret_val( arg_types[i] ); i++)
    {
        switch (arg_types[i])
        {
        case 's': /* str */
            trace_string_a( stack[i] );
            break;
        case 'w': /* wstr */
            trace_string_w( stack[i] );
            break;
        case 'f': /* float */
            TRACE( "%g", *(const float *)&stack[i] );
            break;
        case 'd': /* double */
            TRACE( "%g", *(const double *)&stack[i] );
            break;
        case 'i': /* long */
        default:
            TRACE( "%08Ix", stack[i] );
            break;
        }
        if (!is_ret_val( arg_types[i+1] )) TRACE( "," );
    }
    *nb_args = i;
    TRACE( ") ret=%08Ix\n", stack[-1] );
    return entry_point->orig_func;
}

/***********************************************************************
 *           relay_trace_exit
 */
void WINAPI relay_trace_exit( struct relay_descr *descr, unsigned int idx,
                              INT_PTR retaddr, INT_PTR retval )
{
    TRACE( "\1Ret  %s() retval=%08Ix ret=%08Ix\n",
           func_name( descr->private, LOWORD(idx) ), retval, retaddr );
}

extern INT_PTR WINAPI relay_call( struct relay_descr *descr, unsigned int idx, const INT_PTR *stack );
__ASM_GLOBAL_FUNC( relay_call,
                   "pushq %rbp\n\t"
                   __ASM_SEH(".seh_pushreg %rbp\n\t")
                   __ASM_CFI(".cfi_adjust_cfa_offset 8\n\t")
                   __ASM_CFI(".cfi_rel_offset %rbp,0\n\t")
                   "movq %rsp,%rbp\n\t"
                   __ASM_SEH(".seh_setframe %rbp,0\n\t")
                   __ASM_SEH(".seh_endprologue\n\t")
                   __ASM_CFI(".cfi_def_cfa_register %rbp\n\t")
                   "leaq -0x48(%rbp),%rsp\n\t"
                   "andq $~15,%rsp\n\t"
                   "movq %rcx,-32(%rbp)\n\t"
                   __ASM_CFI(".cfi_rel_offset %rcx,-32\n\t")
                   "movq %rdx,-24(%rbp)\n\t"
                   __ASM_CFI(".cfi_rel_offset %rdx,-24\n\t")
                   "movq %rsi,-16(%rbp)\n\t"
                   __ASM_CFI(".cfi_rel_offset %rsi,-16\n\t")
                   "movq %rdi,-8(%rbp)\n\t"
                   __ASM_CFI(".cfi_rel_offset %rdi,-8\n\t")
                   /* trace the parameters */
                   "leaq 24(%rbp),%r8\n\t"   /* stack */
                   "leaq -40(%rbp),%r9\n\t"
                   "call " __ASM_NAME("relay_trace_entry") "\n\t"
                   /* copy the arguments */
                   "movl -40(%rbp),%edx\n\t"  /* number of args */
                   "movq $4,%rcx\n\t"
                   "cmp %rcx,%rdx\n\t"
                   "cmovgq %rdx,%rcx\n\t"
                   "leaq -16(,%rcx,8),%rdx\n\t"
                   "andq $~15,%rdx\n\t"
                   "subq %rdx,%rsp\n\t"
                   "leaq 24(%rbp),%rsi\n\t"  /* original stack */
                   "movq %rsp,%rdi\n\t"
                   "rep; movsq\n\t"
                   /* call the entry point */
                   "movq 0(%rsp),%rcx\n\t"
                   "movq 8(%rsp),%rdx\n\t"
                   "movq 16(%rsp),%r8\n\t"
                   "movq 24(%rsp),%r9\n\t"
                   "movq 0(%rsp),%xmm0\n\t"
                   "movq 8(%rsp),%xmm1\n\t"
                   "movq 16(%rsp),%xmm2\n\t"
                   "movq 24(%rsp),%xmm3\n\t"
                   "callq *%rax\n\t"
                   /* trace the return value */
                   "movq -32(%rbp),%rcx\n\t"
                   "movq -24(%rbp),%rdx\n\t"
                   "movq 16(%rbp),%r8\n\t"   /* retaddr */
                   "movq %rax,%rsi\n\t"
                   "movaps %xmm0,32(%rsp)\n\t"
                   "movq %rax,%r9\n\t"
                   "call " __ASM_NAME("relay_trace_exit") "\n\t"
                   /* restore return value and return */
                   "movq %rsi,%rax\n\t"
                   "movaps 32(%rsp),%xmm0\n\t"
                   "movq -16(%rbp),%rsi\n\t"
                   __ASM_CFI(".cfi_same_value %rsi\n\t")
                   "movq -8(%rbp),%rdi\n\t"
                   __ASM_CFI(".cfi_same_value %rdi\n\t")
                   "leaq 0(%rbp),%rsp\n\t"
                   __ASM_CFI(".cfi_def_cfa_register %rsp\n\t")
                   "popq %rbp\n\t"
                   __ASM_CFI(".cfi_adjust_cfa_offset -8\n\t")
                   __ASM_CFI(".cfi_same_value %rbp\n\t")
                   "ret")

#else
#error Not supported on this CPU
#endif


static struct relay_descr *get_relay_descr( HMODULE module, const IMAGE_EXPORT_DIRECTORY *exports,
                                            DWORD exp_size )
{
    struct relay_descr *descr;
    struct relay_descr_rva *rva;
    ULONG_PTR ptr = (ULONG_PTR)module + exports->Name;

    /* sanity checks */
    if (ptr <= (ULONG_PTR)(exports + 1)) return NULL;
    if (ptr > (ULONG_PTR)exports + exp_size) return NULL;
    if (ptr % sizeof(DWORD)) return NULL;

    rva = (struct relay_descr_rva *)ptr - 1;
    if (rva->magic != RELAY_DESCR_MAGIC) return NULL;
    if (rva->descr) descr = (struct relay_descr *)((char *)module + rva->descr);
    else descr = (struct relay_descr *)((const char *)exports + exp_size);
    if (descr->magic != RELAY_DESCR_MAGIC) return NULL;
    return descr;
}

/***********************************************************************
 *           RELAY_GetProcAddress
 *
 * Return the proc address to use for a given function.
 */
FARPROC RELAY_GetProcAddress( HMODULE module, const IMAGE_EXPORT_DIRECTORY *exports,
                              DWORD exp_size, FARPROC proc, DWORD ordinal, const WCHAR *user )
{
    struct relay_private_data *data;
    const struct relay_descr *descr = get_relay_descr( module, exports, exp_size );

    if (!descr || !(data = descr->private)) return proc;  /* no relay data */
    if (!data->entry_points[ordinal].orig_func) return proc;  /* not a relayed function */
    if (check_from_module( debug_from_relay_includelist, debug_from_relay_excludelist, user ))
        return proc;  /* we want to relay it */
    return data->entry_points[ordinal].orig_func;
}


/***********************************************************************
 *           RELAY_SetupDLL
 *
 * Setup relay debugging for a built-in dll.
 */
void RELAY_SetupDLL( HMODULE module )
{
    IMAGE_EXPORT_DIRECTORY *exports;
    DWORD *funcs;
    unsigned int i, len;
    DWORD size, entry_point_rva, old_prot;
    struct relay_descr *descr;
    struct relay_private_data *data;
    WCHAR dllnameW[sizeof(data->dllname)];
    const WORD *ordptr;
    void *func_base;
    SIZE_T func_size;

    RtlRunOnceExecuteOnce( &init_once, init_debug_lists, NULL, NULL );

    exports = RtlImageDirectoryEntryToData( module, TRUE, IMAGE_DIRECTORY_ENTRY_EXPORT, &size );
    if (!exports) return;

    if (!(descr = get_relay_descr( module, exports, size ))) return;

    if (!(data = RtlAllocateHeap( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*data) +
                                  (exports->NumberOfFunctions-1) * sizeof(data->entry_points) )))
        return;

    descr->relay_call = relay_call;
    descr->private = data;

    data->module = module;
    data->base   = exports->Base;
    len = strlen( (char *)module + exports->Name );
    if (len > 4 && !_stricmp( (char *)module + exports->Name + len - 4, ".dll" )) len -= 4;
    len = min( len, sizeof(data->dllname) - 1 );
    memcpy( data->dllname, (char *)module + exports->Name, len );
    data->dllname[len] = 0;
    ascii_to_unicode( dllnameW, data->dllname, len + 1 );

    /* fetch name pointer for all entry points and store them in the private structure */

    ordptr = (const WORD *)((char *)module + exports->AddressOfNameOrdinals);
    for (i = 0; i < exports->NumberOfNames; i++, ordptr++)
    {
        DWORD name_rva = ((DWORD*)((char *)module + exports->AddressOfNames))[i];
        data->entry_points[*ordptr].name = (const char *)module + name_rva;
    }

    /* patch the functions in the export table to point to the relay thunks */

    funcs = (DWORD *)((char *)module + exports->AddressOfFunctions);
    entry_point_rva = descr->entry_point_base - (const char *)module;

    func_base = funcs;
    func_size = exports->NumberOfFunctions * sizeof(*funcs);
    NtProtectVirtualMemory( NtCurrentProcess(), &func_base, &func_size, PAGE_READWRITE, &old_prot );
    for (i = 0; i < exports->NumberOfFunctions; i++, funcs++)
    {
        if (!descr->entry_point_offsets[i]) continue;   /* not a normal function */
        if (!check_relay_include( dllnameW, i + exports->Base, data->entry_points[i].name ))
            continue;  /* don't include this entry point */

        data->entry_points[i].orig_func = (char *)module + *funcs;
        *funcs = entry_point_rva + descr->entry_point_offsets[i];
    }
    if (old_prot != PAGE_READWRITE)
        NtProtectVirtualMemory( NtCurrentProcess(), &func_base, &func_size, old_prot, &old_prot );
}

#else  /* __i386__ || __x86_64__ || __arm__ || __aarch64__ */

FARPROC RELAY_GetProcAddress( HMODULE module, const IMAGE_EXPORT_DIRECTORY *exports,
                              DWORD exp_size, FARPROC proc, DWORD ordinal, const WCHAR *user )
{
    return proc;
}

void RELAY_SetupDLL( HMODULE module )
{
}

#endif  /* __i386__ || __x86_64__ || __arm__ || __aarch64__ */


/***********************************************************************/
/* snoop support */
/***********************************************************************/

#ifdef __i386__

WINE_DECLARE_DEBUG_CHANNEL(seh);
WINE_DECLARE_DEBUG_CHANNEL(snoop);

#include "pshpack1.h"

typedef	struct
{
	/* code part */
	BYTE		lcall;		/* 0xe8 call snoopentry (relative) */
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
	DWORD		snoopret;	/* SNOOP_Ret relative */
	/* unreached */
	FARPROC	origreturn;
	SNOOP_DLL	*dll;
	DWORD		ordinal;
	void		**origESP;
	DWORD		*args;		/* saved args across a stdcall */
} SNOOP_RETURNENTRY;

typedef struct tagSNOOP_RETURNENTRIES {
	SNOOP_RETURNENTRY entry[4092/sizeof(SNOOP_RETURNENTRY)];
	struct tagSNOOP_RETURNENTRIES	*next;
} SNOOP_RETURNENTRIES;

#include "poppack.h"

extern void WINAPI SNOOP_Entry(void);
extern void WINAPI SNOOP_Return(void);

static SNOOP_DLL *firstdll;
static SNOOP_RETURNENTRIES *firstrets;


/***********************************************************************
 *          SNOOP_ShowDebugmsgSnoop
 *
 * Simple function to decide if a particular debugging message is
 * wanted.
 */
static BOOL SNOOP_ShowDebugmsgSnoop(const char *module, int ordinal, const char *func)
{
    WCHAR moduleW[40];
    int len = strlen(module);

    if (len >= ARRAY_SIZE( moduleW )) return FALSE;
    ascii_to_unicode( moduleW, module, len + 1 );
    if (debug_snoop_excludelist && check_list( moduleW, ordinal, func, debug_snoop_excludelist ))
        return FALSE;
    if (debug_snoop_includelist && !check_list( moduleW, ordinal, func, debug_snoop_includelist ))
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
    ULONG size32;
    IMAGE_EXPORT_DIRECTORY *exports;

    RtlRunOnceExecuteOnce( &init_once, init_debug_lists, NULL, NULL );

    exports = RtlImageDirectoryEntryToData( hmod, TRUE, IMAGE_DIRECTORY_ENTRY_EXPORT, &size32 );
    if (!exports || !exports->NumberOfFunctions) return;
    name = (char *)hmod + exports->Name;
    size = size32;

    TRACE_(snoop)("hmod=%p, name=%s\n", hmod, name);

    while (*dll) {
        if ((*dll)->hmod == hmod)
        {
            /* another dll, loaded at the same address */
            addr = (*dll)->funs;
            size = (*dll)->nrofordinals * sizeof(SNOOP_FUN);
            NtFreeVirtualMemory(NtCurrentProcess(), &addr, &size, MEM_RELEASE);
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
    if (p > (*dll)->name && !_stricmp( p, ".dll" )) *p = 0;

    size = exports->NumberOfFunctions * sizeof(SNOOP_FUN);
    addr = NULL;
    NtAllocateVirtualMemory(NtCurrentProcess(), &addr, 0, &size,
                            MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!addr) {
        RtlFreeHeap(GetProcessHeap(),0,*dll);
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
FARPROC SNOOP_GetProcAddress( HMODULE hmod, const IMAGE_EXPORT_DIRECTORY *exports,
                              DWORD exp_size, FARPROC origfun, DWORD ordinal,
                              const WCHAR *user)
{
    unsigned int i;
    const char *ename;
    const WORD *ordinals;
    const DWORD *names;
    SNOOP_DLL *dll = firstdll;
    SNOOP_FUN *fun;
    const IMAGE_SECTION_HEADER *sec;

    if (!TRACE_ON(snoop)) return origfun;
    if (!check_from_module( debug_from_snoop_includelist, debug_from_snoop_excludelist, user ))
        return origfun; /* the calling module was explicitly excluded */

    if (!*(LPBYTE)origfun) /* 0x00 is an impossible opcode, possible dataref. */
        return origfun;

    sec = RtlImageRvaToSection( RtlImageNtHeader(hmod), hmod, (char *)origfun - (char *)hmod );

    if (!sec || !(sec->Characteristics & IMAGE_SCN_CNT_CODE))
        return origfun;  /* most likely a data reference */

    while (dll) {
        if (hmod == dll->hmod)
            break;
        dll = dll->next;
    }
    if (!dll)	/* probably internal */
        return origfun;

    /* try to find a name for it */
    ename = NULL;
    names = (const DWORD *)((const char *)hmod + exports->AddressOfNames);
    ordinals = (const WORD *)((const char *)hmod + exports->AddressOfNameOrdinals);
    if (names) for (i = 0; i < exports->NumberOfNames; i++)
    {
        if (ordinals[i] == ordinal)
        {
            ename = (const char *)hmod + names[i];
            break;
        }
    }
    if (!SNOOP_ShowDebugmsgSnoop(dll->name,ordinal,ename))
        return origfun;
    assert(ordinal < dll->nrofordinals);
    fun = dll->funs + ordinal;
    if (!fun->name)
    {
        fun->name       = ename;
        fun->lcall	= 0xe8;
        fun->snoopentry	= (char *)SNOOP_Entry - (char *)(&fun->snoopentry + 1);
        fun->origfun	= origfun;
        fun->nrofargs	= -1;
    }
    return (FARPROC)&(fun->lcall);
}

static void SNOOP_PrintArg(DWORD x)
{
    int i,nostring;

    TRACE_(snoop)("%08lx",x);
    if (IS_INTARG(x) || TRACE_ON(seh)) return; /* trivial reject to avoid faults */
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
            TRACE_(snoop)(" %s",debugstr_an((LPSTR)x,i));
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
            if (!nostring && i > 5) TRACE_(snoop)(" %s",debugstr_wn((LPWSTR)x,i));
        }
    }
    __EXCEPT_PAGE_FAULT
    {
    }
    __ENDTRY
}

void WINAPI __regs_SNOOP_Entry( void **stack )
{
	SNOOP_DLL *dll;
	SNOOP_FUN *fun = (SNOOP_FUN *)((char *)stack[0] - 5);
	SNOOP_RETURNENTRIES	**rets = &firstrets;
	SNOOP_RETURNENTRY	*ret;
	int		i=0, max;

	for (dll = firstdll; dll; dll = dll->next )
            if (fun >= dll->funs && fun < dll->funs + dll->nrofordinals) break;

	if (!dll) {
		FIXME("entrypoint %p not found\n", fun);
		return; /* oops */
	}
	/* guess cdecl ... */
	if (fun->nrofargs<0) {
		/* Typical cdecl return frame is:
		 *     add esp, xxxxxxxx
		 * which has (for xxxxxxxx up to 255 the opcode "83 C4 xx".
		 * (after that 81 C2 xx xx xx xx)
		 */
		LPBYTE	reteip = stack[1];

		if (reteip) {
			if ((reteip[0]==0x83)&&(reteip[1]==0xc4))
				fun->nrofargs=reteip[2]/4;
		}
	}


	while (*rets) {
		for (i=0;i<ARRAY_SIZE( (*rets)->entry );i++)
			if (!(*rets)->entry[i].origreturn)
				break;
		if (i!=ARRAY_SIZE( (*rets)->entry ))
			break;
		rets = &((*rets)->next);
	}
	if (!*rets) {
                SIZE_T size = 4096;
                VOID* addr = NULL;

                NtAllocateVirtualMemory(NtCurrentProcess(), &addr, 0, &size, 
                                        MEM_COMMIT | MEM_RESERVE,
                                        PAGE_EXECUTE_READWRITE);
                if (!addr) return;
                *rets = addr;
		i = 0;	/* entry 0 is free */
	}
	ret = &((*rets)->entry[i]);
	ret->lcall	= 0xe8;
	ret->snoopret	= (char *)SNOOP_Return - (char *)(&ret->snoopret + 1);
	ret->origreturn	= stack[1];
	stack[1]	= &ret->lcall;
	ret->dll	= dll;
	ret->args	= NULL;
	ret->ordinal	= fun - dll->funs;
	ret->origESP	= stack;

	stack[0] = fun->origfun;

        if (!TRACE_ON(snoop)) return;

	if (fun->name) TRACE_(snoop)("\1CALL %s.%s(", dll->name, fun->name);
	else TRACE_(snoop)("\1CALL %s.%ld(", dll->name, dll->ordbase+ret->ordinal);
	if (fun->nrofargs>0) {
		max = fun->nrofargs; if (max>16) max=16;
		for (i=0;i<max;i++)
                {
                    SNOOP_PrintArg( (DWORD)stack[i + 2] );
                    if (i<fun->nrofargs-1) TRACE_(snoop)(",");
                }
		if (max!=fun->nrofargs)
			TRACE_(snoop)(" ...");
	} else if (fun->nrofargs<0) {
		TRACE_(snoop)("<unknown, check return>");
		ret->args = RtlAllocateHeap(GetProcessHeap(),
                                            0,16*sizeof(DWORD));
		memcpy(ret->args, stack + 2, sizeof(DWORD)*16);
	}
	TRACE_(snoop)(") ret=%08lx\n",(DWORD)ret->origreturn);
}

void WINAPI __regs_SNOOP_Return( void **stack )
{
	SNOOP_RETURNENTRY *ret = (SNOOP_RETURNENTRY*)((char *)stack[0] - 5);
        SNOOP_FUN *fun = &ret->dll->funs[ret->ordinal];
        DWORD retval = (DWORD)stack[-1];  /* get saved %eax from the stack */

	/* We haven't found out the nrofargs yet. If we called a cdecl
	 * function it is too late anyway and we can just set '0' (which
	 * will be the difference between orig and current ESP
	 * If stdcall -> everything ok.
	 */
	if (ret->dll->funs[ret->ordinal].nrofargs<0)
		ret->dll->funs[ret->ordinal].nrofargs = stack - ret->origESP - 1;
	stack[0] = ret->origreturn;

        if (!TRACE_ON(snoop)) {
            ret->origreturn = NULL; /* mark as empty */
            return;
        }

	if (ret->args) {
		int	i,max;

                if (fun->name)
                    TRACE_(snoop)("\1RET  %s.%s(", ret->dll->name, fun->name);
                else
                    TRACE_(snoop)("\1RET  %s.%ld(", ret->dll->name, ret->dll->ordbase+ret->ordinal);

		max = fun->nrofargs;
		if (max>16) max=16;

		for (i=0;i<max;i++)
                {
                    SNOOP_PrintArg(ret->args[i]);
                    if (i<max-1) TRACE_(snoop)(",");
                }
		TRACE_(snoop)(") retval=%08lx ret=%08lx\n", retval, (DWORD)ret->origreturn );
		RtlFreeHeap(GetProcessHeap(),0,ret->args);
		ret->args = NULL;
	}
        else
        {
            if (fun->name)
		TRACE_(snoop)("\1RET  %s.%s() retval=%08lx ret=%08lx\n",
                        ret->dll->name, fun->name, retval, (DWORD)ret->origreturn);
            else
		TRACE_(snoop)("\1RET  %s.%ld() retval=%08lx ret=%08lx\n",
			ret->dll->name,ret->dll->ordbase+ret->ordinal,
			retval, (DWORD)ret->origreturn);
        }
	ret->origreturn = NULL; /* mark as empty */
}

/* small wrappers that save registers and get the stack pointer */
#define SNOOP_WRAPPER(name) \
    __ASM_STDCALL_FUNC( name, 0,                                        \
                        "pushl %eax\n\t"                                \
                        __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")       \
                        "pushl %ecx\n\t"                                \
                        __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")       \
                        "pushl %edx\n\t"                                \
                        __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")       \
                        "leal 12(%esp),%eax\n\t"                        \
                        "pushl %eax\n\t"                                \
                        __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")       \
                        "call " __ASM_STDCALL("__regs_" #name,4) "\n\t" \
                        __ASM_CFI(".cfi_adjust_cfa_offset -4\n\t")      \
                        "popl %edx\n\t"                                 \
                        __ASM_CFI(".cfi_adjust_cfa_offset -4\n\t")      \
                        "popl %ecx\n\t"                                 \
                        __ASM_CFI(".cfi_adjust_cfa_offset -4\n\t")      \
                        "popl %eax\n\t"                                 \
                        __ASM_CFI(".cfi_adjust_cfa_offset -4\n\t")      \
                        "ret" )

SNOOP_WRAPPER( SNOOP_Entry )
SNOOP_WRAPPER( SNOOP_Return )

#else  /* __i386__ */

FARPROC SNOOP_GetProcAddress( HMODULE hmod, const IMAGE_EXPORT_DIRECTORY *exports, DWORD exp_size,
                              FARPROC origfun, DWORD ordinal, const WCHAR *user )
{
    return origfun;
}

void SNOOP_SetupDLL( HMODULE hmod )
{
    FIXME("snooping works only on i386 for now.\n");
}

#endif /* __i386__ */
