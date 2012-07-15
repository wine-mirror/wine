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

#include "config.h"
#include "wine/port.h"

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
#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(relay);

#if defined(__i386__) || defined(__x86_64__) || defined(__arm__)

WINE_DECLARE_DEBUG_CHANNEL(timestamp);

struct relay_descr  /* descriptor for a module */
{
    void               *magic;               /* signature */
    void               *relay_call;          /* functions to call from relay thunks */
    void               *relay_call_regs;
    void               *private;             /* reserved for the relay code private data */
    const char         *entry_point_base;    /* base address of entry point thunks */
    const unsigned int *entry_point_offsets; /* offsets of entry points thunks */
    const unsigned int *arg_types;           /* table of argument types for all entry points */
};

#define RELAY_DESCR_MAGIC  ((void *)0xdeb90001)
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

static BOOL init_done;

/* compare an ASCII and a Unicode string without depending on the current codepage */
static inline int strcmpAW( const char *strA, const WCHAR *strW )
{
    while (*strA && ((unsigned char)*strA == *strW)) { strA++; strW++; }
    return (unsigned char)*strA - *strW;
}

/* compare an ASCII and a Unicode string without depending on the current codepage */
static inline int strncmpiAW( const char *strA, const WCHAR *strW, int n )
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
        WCHAR *q = str;

        strcpyW( str, buffer );
        count = 0;
        for (;;)
        {
            ret[count++] = q;
            if (!(q = strchrW( q, ';' ))) break;
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
static void init_debug_lists(void)
{
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING name;
    HANDLE root, hkey;
    static const WCHAR configW[] = {'S','o','f','t','w','a','r','e','\\',
                                    'W','i','n','e','\\',
                                    'D','e','b','u','g',0};
    static const WCHAR RelayIncludeW[] = {'R','e','l','a','y','I','n','c','l','u','d','e',0};
    static const WCHAR RelayExcludeW[] = {'R','e','l','a','y','E','x','c','l','u','d','e',0};
    static const WCHAR SnoopIncludeW[] = {'S','n','o','o','p','I','n','c','l','u','d','e',0};
    static const WCHAR SnoopExcludeW[] = {'S','n','o','o','p','E','x','c','l','u','d','e',0};
    static const WCHAR RelayFromIncludeW[] = {'R','e','l','a','y','F','r','o','m','I','n','c','l','u','d','e',0};
    static const WCHAR RelayFromExcludeW[] = {'R','e','l','a','y','F','r','o','m','E','x','c','l','u','d','e',0};
    static const WCHAR SnoopFromIncludeW[] = {'S','n','o','o','p','F','r','o','m','I','n','c','l','u','d','e',0};
    static const WCHAR SnoopFromExcludeW[] = {'S','n','o','o','p','F','r','o','m','E','x','c','l','u','d','e',0};

    if (init_done) return;
    init_done = TRUE;

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

    debug_relay_includelist = load_list( hkey, RelayIncludeW );
    debug_relay_excludelist = load_list( hkey, RelayExcludeW );
    debug_snoop_includelist = load_list( hkey, SnoopIncludeW );
    debug_snoop_excludelist = load_list( hkey, SnoopExcludeW );
    debug_from_relay_includelist = load_list( hkey, RelayFromIncludeW );
    debug_from_relay_excludelist = load_list( hkey, RelayFromExcludeW );
    debug_from_snoop_includelist = load_list( hkey, SnoopFromIncludeW );
    debug_from_snoop_excludelist = load_list( hkey, SnoopFromExcludeW );

    NtClose( hkey );
}


/***********************************************************************
 *           check_list
 *
 * Check if a given module and function is in the list.
 */
static BOOL check_list( const char *module, int ordinal, const char *func, const WCHAR *const *list )
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
 *           check_from_module
 *
 * Check if calls from a given module must be included in the relay/snoop output,
 * given the exclusion and inclusion lists.
 */
static BOOL check_from_module( const WCHAR **includelist, const WCHAR **excludelist, const WCHAR *module )
{
    static const WCHAR dllW[] = {'.','d','l','l',0 };
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

        if (!strcmpiW( *listitem, module )) return !show;
        len = strlenW( *listitem );
        if (!strncmpiW( *listitem, module, len ) && !strcmpiW( module + len, dllW ))
            return !show;
    }
    return show;
}

/***********************************************************************
 *           RELAY_PrintArgs
 */
static inline void RELAY_PrintArgs( const INT_PTR *args, int nb_args, unsigned int typemask )
{
    while (nb_args--)
    {
        if ((typemask & 3) && !IS_INTARG(*args))
        {
	    if (typemask & 2)
                DPRINTF( "%08lx %s", *args, debugstr_w((LPCWSTR)*args) );
            else
                DPRINTF( "%08lx %s", *args, debugstr_a((LPCSTR)*args) );
	}
        else DPRINTF( "%08lx", *args );
        if (nb_args) DPRINTF( "," );
        args++;
        typemask >>= 2;
    }
}

extern LONGLONG CDECL call_entry_point( void *func, int nb_args, const INT_PTR *args, int flags );
#ifdef __i386__
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
                   "testl $2,20(%ebp)\n\t"  /* (flags & 2) -> thiscall */
                   "jz 1f\n\t"
                   "popl %ecx\n\t"
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
#elif defined(__arm__)
__ASM_GLOBAL_FUNC( call_entry_point,
                   "push {r4, r5, LR}\n\t"
                   "mov r4, r0\n\t"
                   "mov r5, SP\n\t"
                   "lsl r3, r1, #2\n\t"
                   "cmp r3, #0\n\t"
                   "beq 5f\n\t"
                   "sub SP, SP, r3\n\t"
                   "bic SP, SP, #15\n\t"
                   "1:\tsub r3, r3, #4\n\t"
                   "ldr r0, [r2, r3]\n\t"
                   "str r0, [SP, r3]\n\t"
                   "cmp r3, #0\n\t"
                   "bgt 1b\n\t"
                   "cmp r1, #1\n\t"
                   "bgt 2f\n\t"
                   "pop {r0}\n\t"
                   "b 5f\n\t"
                   "2:\tcmp r1, #2\n\t"
                   "bgt 3f\n\t"
                   "pop {r0-r1}\n\t"
                   "b 5f\n\t"
                   "3:\tcmp r1, #3\n\t"
                   "bgt 4f\n\t"
                   "pop {r0-r2}\n\t"
                   "b 5f\n\t"
                   "4:\tpop {r0-r3}\n\t"
                   "5:\tblx r4\n\t"
                   "mov SP, r5\n\t"
                   "pop {r4, r5, PC}" )
#else
__ASM_GLOBAL_FUNC( call_entry_point,
                   "pushq %rbp\n\t"
                   __ASM_CFI(".cfi_adjust_cfa_offset 8\n\t")
                   __ASM_CFI(".cfi_rel_offset %rbp,0\n\t")
                   "movq %rsp,%rbp\n\t"
                   __ASM_CFI(".cfi_def_cfa_register %rbp\n\t")
                   "pushq %rsi\n\t"
                   __ASM_CFI(".cfi_rel_offset %rsi,-8\n\t")
                   "pushq %rdi\n\t"
                   __ASM_CFI(".cfi_rel_offset %rdi,-16\n\t")
                   "movq %rcx,%rax\n\t"
                   "movq $4,%rcx\n\t"
                   "cmp %rcx,%rdx\n\t"
                   "cmovgq %rdx,%rcx\n\t"
                   "leaq 0(,%rcx,8),%rdx\n\t"
                   "subq %rdx,%rsp\n\t"
                   "andq $~15,%rsp\n\t"
                   "movq %rsp,%rdi\n\t"
                   "movq %r8,%rsi\n\t"
                   "rep; movsq\n\t"
                   "movq 0(%rsp),%rcx\n\t"
                   "movq 8(%rsp),%rdx\n\t"
                   "movq 16(%rsp),%r8\n\t"
                   "movq 24(%rsp),%r9\n\t"
                   "movq %rcx,%xmm0\n\t"
                   "movq %rdx,%xmm1\n\t"
                   "movq %r8,%xmm2\n\t"
                   "movq %r9,%xmm3\n\t"
                   "callq *%rax\n\t"
                   "leaq -16(%rbp),%rsp\n\t"
                   "popq %rdi\n\t"
                   __ASM_CFI(".cfi_same_value %rdi\n\t")
                   "popq %rsi\n\t"
                   __ASM_CFI(".cfi_same_value %rsi\n\t")
                   __ASM_CFI(".cfi_def_cfa_register %rsp\n\t")
                   "popq %rbp\n\t"
                   __ASM_CFI(".cfi_adjust_cfa_offset -8\n\t")
                   __ASM_CFI(".cfi_same_value %rbp\n\t")
                   "ret")
#endif


static void print_timestamp(void)
{
    ULONG ticks = NtGetTickCount();
    DPRINTF( "%3u.%03u:", ticks / 1000, ticks % 1000 );
}


/***********************************************************************
 *           relay_call
 *
 * stack points to the return address, i.e. the first argument is stack[1].
 */
static LONGLONG WINAPI relay_call( struct relay_descr *descr, unsigned int idx, const INT_PTR *stack )
{
    LONGLONG ret;
    WORD ordinal = LOWORD(idx);
    BYTE nb_args = LOBYTE(HIWORD(idx));
    BYTE flags   = HIBYTE(HIWORD(idx));
    struct relay_private_data *data = descr->private;
    struct relay_entry_point *entry_point = data->entry_points + ordinal;

    if (!TRACE_ON(relay))
        ret = call_entry_point( entry_point->orig_func, nb_args, stack + 1, flags );
    else
    {
        if (TRACE_ON(timestamp))
            print_timestamp();
        if (entry_point->name)
            DPRINTF( "%04x:Call %s.%s(", GetCurrentThreadId(), data->dllname, entry_point->name );
        else
            DPRINTF( "%04x:Call %s.%u(", GetCurrentThreadId(), data->dllname, data->base + ordinal );
        RELAY_PrintArgs( stack + 1, nb_args, descr->arg_types[ordinal] );
        DPRINTF( ") ret=%08lx\n", stack[0] );

        ret = call_entry_point( entry_point->orig_func, nb_args, stack + 1, flags );

        if (TRACE_ON(timestamp))
            print_timestamp();
        if (entry_point->name)
            DPRINTF( "%04x:Ret  %s.%s()", GetCurrentThreadId(), data->dllname, entry_point->name );
        else
            DPRINTF( "%04x:Ret  %s.%u()", GetCurrentThreadId(), data->dllname, data->base + ordinal );

        if (flags & 1)  /* 64-bit return value */
            DPRINTF( " retval=%08x%08x ret=%08lx\n",
                     (UINT)(ret >> 32), (UINT)ret, stack[0] );
        else
            DPRINTF( " retval=%08lx ret=%08lx\n", (UINT_PTR)ret, stack[0] );
    }
    return ret;
}


/***********************************************************************
 *           relay_call_regs
 */
#ifdef __i386__
void WINAPI __regs_relay_call_regs( struct relay_descr *descr, unsigned int idx,
                                    unsigned int orig_eax, unsigned int ret_addr,
                                    CONTEXT *context )
{
    WORD ordinal = LOWORD(idx);
    BYTE nb_args = LOBYTE(HIWORD(idx));
    struct relay_private_data *data = descr->private;
    struct relay_entry_point *entry_point = data->entry_points + ordinal;
    BYTE *orig_func = entry_point->orig_func;
    INT_PTR *args = (INT_PTR *)context->Esp;
    INT_PTR args_copy[32];

    /* restore the context to what it was before the relay thunk */
    context->Eax = orig_eax;
    context->Eip = ret_addr;
    context->Esp += nb_args * sizeof(int);

    if (TRACE_ON(relay))
    {
        if (entry_point->name)
            DPRINTF( "%04x:Call %s.%s(", GetCurrentThreadId(), data->dllname, entry_point->name );
        else
            DPRINTF( "%04x:Call %s.%u(", GetCurrentThreadId(), data->dllname, data->base + ordinal );
        RELAY_PrintArgs( args, nb_args, descr->arg_types[ordinal] );
        DPRINTF( ") ret=%08x\n", ret_addr );

        DPRINTF( "%04x:  eax=%08x ebx=%08x ecx=%08x edx=%08x esi=%08x edi=%08x "
                 "ebp=%08x esp=%08x ds=%04x es=%04x fs=%04x gs=%04x flags=%08x\n",
                 GetCurrentThreadId(), context->Eax, context->Ebx, context->Ecx,
                 context->Edx, context->Esi, context->Edi, context->Ebp, context->Esp,
                 context->SegDs, context->SegEs, context->SegFs, context->SegGs, context->EFlags );

        assert( orig_func[0] == 0x68 /* pushl func */ );
        assert( orig_func[5] == 0x6a /* pushl args */ );
        assert( orig_func[7] == 0xe8 /* call */ );
    }

    /* now call the real function */

    memcpy( args_copy, args, nb_args * sizeof(args[0]) );
    args_copy[nb_args++] = (INT_PTR)context;  /* append context argument */

    call_entry_point( orig_func + 12 + *(int *)(orig_func + 1), nb_args, args_copy, 0 );


    if (TRACE_ON(relay))
    {
        if (entry_point->name)
            DPRINTF( "%04x:Ret  %s.%s() retval=%08x ret=%08x\n",
                     GetCurrentThreadId(), data->dllname, entry_point->name,
                     context->Eax, context->Eip );
        else
            DPRINTF( "%04x:Ret  %s.%u() retval=%08x ret=%08x\n",
                     GetCurrentThreadId(), data->dllname, data->base + ordinal,
                     context->Eax, context->Eip );
        DPRINTF( "%04x:  eax=%08x ebx=%08x ecx=%08x edx=%08x esi=%08x edi=%08x "
                 "ebp=%08x esp=%08x ds=%04x es=%04x fs=%04x gs=%04x flags=%08x\n",
                 GetCurrentThreadId(), context->Eax, context->Ebx, context->Ecx,
                 context->Edx, context->Esi, context->Edi, context->Ebp, context->Esp,
                 context->SegDs, context->SegEs, context->SegFs, context->SegGs, context->EFlags );
    }
}
extern void WINAPI relay_call_regs(void);
DEFINE_REGS_ENTRYPOINT( relay_call_regs, 4 )

#else  /* __i386__ */

void WINAPI relay_call_regs( struct relay_descr *descr, INT_PTR idx, INT_PTR *stack )
{
    assert(0);  /* should never be called */
}

#endif  /* __i386__ */


/***********************************************************************
 *           RELAY_GetProcAddress
 *
 * Return the proc address to use for a given function.
 */
FARPROC RELAY_GetProcAddress( HMODULE module, const IMAGE_EXPORT_DIRECTORY *exports,
                              DWORD exp_size, FARPROC proc, DWORD ordinal, const WCHAR *user )
{
    struct relay_private_data *data;
    const struct relay_descr *descr = (const struct relay_descr *)((const char *)exports + exp_size);

    if (descr->magic != RELAY_DESCR_MAGIC || !(data = descr->private)) return proc;  /* no relay data */
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
    DWORD size, entry_point_rva;
    struct relay_descr *descr;
    struct relay_private_data *data;
    const WORD *ordptr;

    if (!init_done) init_debug_lists();

    exports = RtlImageDirectoryEntryToData( module, TRUE, IMAGE_DIRECTORY_ENTRY_EXPORT, &size );
    if (!exports) return;

    descr = (struct relay_descr *)((char *)exports + size);
    if (descr->magic != RELAY_DESCR_MAGIC) return;

    if (!(data = RtlAllocateHeap( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*data) +
                                  (exports->NumberOfFunctions-1) * sizeof(data->entry_points) )))
        return;

    descr->relay_call = relay_call;
    descr->relay_call_regs = relay_call_regs;
    descr->private = data;

    data->module = module;
    data->base   = exports->Base;
    len = strlen( (char *)module + exports->Name );
    if (len > 4 && !strcasecmp( (char *)module + exports->Name + len - 4, ".dll" )) len -= 4;
    len = min( len, sizeof(data->dllname) - 1 );
    memcpy( data->dllname, (char *)module + exports->Name, len );
    data->dllname[len] = 0;

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
    for (i = 0; i < exports->NumberOfFunctions; i++, funcs++)
    {
        if (!descr->entry_point_offsets[i]) continue;   /* not a normal function */
        if (!check_relay_include( data->dllname, i + exports->Base, data->entry_points[i].name ))
            continue;  /* don't include this entry point */

        data->entry_points[i].orig_func = (char *)module + *funcs;
        *funcs = entry_point_rva + descr->entry_point_offsets[i];
    }
}

#else  /* __i386__ || __x86_64__ */

FARPROC RELAY_GetProcAddress( HMODULE module, const IMAGE_EXPORT_DIRECTORY *exports,
                              DWORD exp_size, FARPROC proc, DWORD ordinal, const WCHAR *user )
{
    return proc;
}

void RELAY_SetupDLL( HMODULE module )
{
}

#endif  /* __i386__ || __x86_64__ */


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
    ULONG size32;
    IMAGE_EXPORT_DIRECTORY *exports;

    if (!init_done) init_debug_lists();

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
    if (p > (*dll)->name && !strcasecmp( p, ".dll" )) *p = 0;

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

    DPRINTF("%08x",x);
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
    __EXCEPT_PAGE_FAULT
    {
    }
    __ENDTRY
}

#define CALLER1REF (*(DWORD*)context->Esp)

void WINAPI __regs_SNOOP_Entry( CONTEXT *context )
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
		FIXME("entrypoint 0x%08x not found\n",entry);
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
                VOID* addr = NULL;

                NtAllocateVirtualMemory(NtCurrentProcess(), &addr, 0, &size, 
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

	if (TRACE_ON(timestamp))
		print_timestamp();
	if (fun->name) DPRINTF("%04x:CALL %s.%s(",GetCurrentThreadId(),dll->name,fun->name);
	else DPRINTF("%04x:CALL %s.%d(",GetCurrentThreadId(),dll->name,dll->ordbase+ordinal);
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
		ret->args = RtlAllocateHeap(GetProcessHeap(),
                                            0,16*sizeof(DWORD));
		memcpy(ret->args,(LPBYTE)(context->Esp + 4),sizeof(DWORD)*16);
	}
	DPRINTF(") ret=%08x\n",(DWORD)ret->origreturn);
}


void WINAPI __regs_SNOOP_Return( CONTEXT *context )
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
	if (TRACE_ON(timestamp))
		print_timestamp();
	if (ret->args) {
		int	i,max;

                if (fun->name)
                    DPRINTF("%04x:RET  %s.%s(", GetCurrentThreadId(), ret->dll->name, fun->name);
                else
                    DPRINTF("%04x:RET  %s.%d(", GetCurrentThreadId(),
                            ret->dll->name,ret->dll->ordbase+ret->ordinal);

		max = fun->nrofargs;
		if (max>16) max=16;

		for (i=0;i<max;i++)
                {
                    SNOOP_PrintArg(ret->args[i]);
                    if (i<max-1) DPRINTF(",");
                }
		DPRINTF(") retval=%08x ret=%08x\n",
			context->Eax,(DWORD)ret->origreturn );
		RtlFreeHeap(GetProcessHeap(),0,ret->args);
		ret->args = NULL;
	}
        else
        {
            if (fun->name)
		DPRINTF("%04x:RET  %s.%s() retval=%08x ret=%08x\n",
			GetCurrentThreadId(),
			ret->dll->name, fun->name, context->Eax, (DWORD)ret->origreturn);
            else
		DPRINTF("%04x:RET  %s.%d() retval=%08x ret=%08x\n",
			GetCurrentThreadId(),
			ret->dll->name,ret->dll->ordbase+ret->ordinal,
			context->Eax, (DWORD)ret->origreturn);
        }
	ret->origreturn = NULL; /* mark as empty */
}

/* assembly wrappers that save the context */
DEFINE_REGS_ENTRYPOINT( SNOOP_Entry, 0 )
DEFINE_REGS_ENTRYPOINT( SNOOP_Return, 0 )

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
