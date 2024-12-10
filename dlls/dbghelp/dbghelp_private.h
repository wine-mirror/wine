/*
 * File dbghelp_private.h - dbghelp internal definitions
 *
 * Copyright (C) 1995, Alexandre Julliard
 * Copyright (C) 1996, Eric Youngdale.
 * Copyright (C) 1999-2000, Ulrich Weigand.
 * Copyright (C) 2004-2007, Eric Pouech.
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

#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "winver.h"
#include "dbghelp.h"
#include "objbase.h"
#include "oaidl.h"
#include "winnls.h"
#include "wine/list.h"
#include "wine/rbtree.h"

#include "cvconst.h"

struct pool /* poor's man */
{
    HANDLE      heap;
};

void     pool_init(struct pool* a, size_t arena_size);
void     pool_destroy(struct pool* a);
void*    pool_alloc(struct pool* a, size_t len) __WINE_ALLOC_SIZE(2) __WINE_MALLOC;
void*    pool_realloc(struct pool* a, void* ptr, size_t len) __WINE_ALLOC_SIZE(3);
char*    pool_strdup(struct pool* a, const char* str) __WINE_MALLOC;
WCHAR*   pool_wcsdup(struct pool* a, const WCHAR* str) __WINE_MALLOC;
void     pool_free(struct pool* a, void* ptr);

struct vector
{
    void**      buckets;
    unsigned    elt_size;
    unsigned    shift;
    unsigned    num_elts;
    unsigned    num_buckets;
    unsigned    buckets_allocated;
};

void     vector_init(struct vector* v, unsigned elt_sz, unsigned bucket_sz);
unsigned vector_length(const struct vector* v);
void*    vector_at(const struct vector* v, unsigned pos);
void*    vector_add(struct vector* v, struct pool* pool);

struct sparse_array
{
    struct vector               key2index;
    struct vector               elements;
};

void     sparse_array_init(struct sparse_array* sa, unsigned elt_sz, unsigned bucket_sz);
void*    sparse_array_find(const struct sparse_array* sa, ULONG_PTR idx);
void*    sparse_array_add(struct sparse_array* sa, ULONG_PTR key, struct pool* pool);
unsigned sparse_array_length(const struct sparse_array* sa);

struct hash_table_elt
{
    const char*                 name;
    struct hash_table_elt*      next;
};

struct hash_table_bucket
{
    struct hash_table_elt*      first;
    struct hash_table_elt*      last;
};

struct hash_table
{
    unsigned                    num_elts;
    unsigned                    num_buckets;
    struct hash_table_bucket*   buckets;
    struct pool*                pool;
};

void     hash_table_init(struct pool* pool, struct hash_table* ht,
                         unsigned num_buckets);
void     hash_table_destroy(struct hash_table* ht);
void     hash_table_add(struct hash_table* ht, struct hash_table_elt* elt);

struct hash_table_iter
{
    const struct hash_table*    ht;
    struct hash_table_elt*      element;
    int                         index;
    int                         last;
};

void     hash_table_iter_init(const struct hash_table* ht,
                              struct hash_table_iter* hti, const char* name);
void*    hash_table_iter_up(struct hash_table_iter* hti);


extern unsigned dbghelp_options;
extern BOOL     dbghelp_opt_native;
extern BOOL     dbghelp_opt_extension_api;
extern BOOL     dbghelp_opt_real_path;
extern BOOL     dbghelp_opt_source_actual_path;
extern SYSTEM_INFO sysinfo;

/* FIXME: this could be optimized later on by using relative offsets and smaller integral sizes */
struct addr_range
{
    DWORD64                     low;            /* absolute address of first byte of the range */
    DWORD64                     high;           /* absolute address of first byte after the range */
};

static inline DWORD64 addr_range_size(const struct addr_range* ar)
{
    return ar->high - ar->low;
}

/* tests whether ar2 is inside ar1 */
static inline BOOL addr_range_inside(const struct addr_range* ar1, const struct addr_range* ar2)
{
    return ar1->low <= ar2->low && ar2->high <= ar1->high;
}

/* tests whether ar1 and ar2 are disjoint */
static inline BOOL addr_range_disjoint(const struct addr_range* ar1, const struct addr_range* ar2)
{
    return ar1->high <= ar2->low || ar2->high <= ar1->low;
}

enum location_kind {loc_error,          /* reg is the error code */
                    loc_unavailable,    /* location is not available */
                    loc_absolute,       /* offset is the location */
                    loc_register,       /* reg is the location */
                    loc_regrel,         /* [reg+offset] is the location */
                    loc_tlsrel,         /* offset is the address of the TLS index */
                    loc_user,           /* value is debug information dependent,
                                           reg & offset can be used ad libidem */
};

enum location_error {loc_err_internal = -1,     /* internal while computing */
                     loc_err_too_complex = -2,  /* couldn't compute location (even at runtime) */
                     loc_err_out_of_scope = -3, /* variable isn't available at current address */
                     loc_err_cant_read = -4,    /* couldn't read memory at given address */
                     loc_err_no_location = -5,  /* likely optimized away (by compiler) */
};

struct location
{
    unsigned            kind : 8,
                        reg;
    ULONG_PTR           offset;
};

struct symt
{
    enum SymTagEnum             tag;
};

struct symt_ht
{
    struct symt                 symt;
    struct hash_table_elt       hash_elt;        /* if global symbol or type */
};

static inline BOOL symt_check_tag(const struct symt* s, enum SymTagEnum tag)
{
    return s && s->tag == tag;
}

typedef ULONG_PTR symref_t;

/* lexical tree */
struct symt_block
{
    struct symt                 symt;
    struct symt*                container;      /* block, or func */
    struct vector               vchildren;      /* sub-blocks & local variables */
    unsigned                    num_ranges;
    struct addr_range           ranges[];
};

struct symt_module /* in fact any of .exe, .dll... */
{
    struct symt                 symt;           /* module */
    struct vector               vchildren;      /* compilation units */
    struct module*              module;
};

struct symt_compiland
{
    struct symt                 symt;
    struct symt_module*         container;      /* symt_module */
    ULONG_PTR                   address;
    unsigned                    source;
    struct vector               vchildren;      /* global variables & functions */
    void*                       user;           /* when debug info provider needs to store information */
};

struct symt_data
{
    struct symt                 symt;
    struct hash_table_elt       hash_elt;       /* if global symbol */
    enum DataKind               kind;
    struct symt*                container;
    symref_t                    type;
    union                                       /* depends on kind */
    {
        /* DataIs{Global, FileStatic, StaticLocal}:
         *      with loc.kind
         *              loc_absolute    loc.offset is address
         *              loc_tlsrel      loc.offset is TLS index address
         * DataIs{Local,Param}:
         *      with loc.kind
         *              loc_absolute    not supported
         *              loc_register    location is in register loc.reg
         *              loc_regrel      location is at address loc.reg + loc.offset
         *              >= loc_user     ask debug info provider for resolution
         */
        struct location         var;
        /* DataIs{Member} (all values are in bits, not bytes) */
        struct
        {
            LONG_PTR                    offset;
            ULONG_PTR                   bit_length;
            ULONG_PTR                   bit_offset;
        } member;
        /* DataIsConstant */
        VARIANT                 value;
    } u;
};

/* Understanding functions internal storage:
 * - functions, inline sites and blocks can be described as spreading across
 *   several chunks of memory (hence describing potentially a non contiguous
 *   memory space).
 * - this is described internally as an array of address ranges
 *   (struct addr_range)
 *
 * - there's a hierarchical (aka lexical) relationship:
 *   + function's parent is a compiland or the module itself
 *   + inline site's parent is either a function or another inline site
 *   + block's parent is either a function, an inline site or another block.
 *
 * - internal storage rely on the following assumptions:
 *   + in an array of address ranges, one address range doesn't overlap over
 *     one of its siblings
 *   + each address range of a block is inside a single range of its lexical
 *     parent (and outside of the others since they don't overlap)
 *   + each address range of an inline site is inside a single range its
 *     lexical parent
 *   + a function (as of today) is only represented by a single address range
 *     (A).
 *
 * - all inline sites of a function are stored in a linked list:
 *   + this linked list shall preserve the weak order of the lexical-parent
 *     relationship (eg for any inline site A, which has inline site B
 *     as lexical parent, A must appear before B in the linked list)
 *
 * - lookup:
 *   + when looking up which inline site contains a given address, the first
 *     range containing that address found while walking the list of inline
 *     sites is the right one.
 *   + when lookup up which inner-most block contains an address, descend the
 *     blocks tree with branching on the block (if any) which contains the given
 *     address in one of its ranges
 *
 * Notes:
 *   (A): shall evolve but storage in native is awkward: from PGO testing, the
 *        top function is stored with its first range of address; all the others
 *        are stored as blocks, children of compiland, but which lexical parent
 *        is the top function. This breaks the natural assumption that
 *        children <> lexical parent is symmetrical.
 *   (B): see dwarf.c for some gory discrepancies between native & builtin
 *        DbgHelp.
 */

struct symt_function
{
    struct symt                 symt;           /* SymTagFunction or SymTagInlineSite */
    struct hash_table_elt       hash_elt;       /* if global symbol, inline site */
    struct symt*                container;      /* compiland (for SymTagFunction) or function (for SymTagInlineSite) */
    symref_t                    type;           /* points to function_signature */
    struct vector               vlines;
    struct vector               vchildren;      /* locals, params, blocks, start/end, labels, inline sites */
    struct symt_function*       next_inlinesite;/* linked list of inline sites in this function */
    unsigned                    num_ranges;
    struct addr_range           ranges[];
};

struct symt_hierarchy_point
{
    struct symt                 symt;           /* either SymTagFunctionDebugStart, SymTagFunctionDebugEnd, SymTagLabel */
    struct hash_table_elt       hash_elt;       /* if label (and in compiland's hash table if global) */
    struct symt*                parent;         /* symt_function or symt_compiland */
    struct location             loc;
};

struct symt_public
{
    struct symt                 symt;
    struct hash_table_elt       hash_elt;
    struct symt*                container;      /* compiland */
    BOOL is_function;
    ULONG_PTR                   address;
    ULONG_PTR                   size;
};

struct symt_thunk
{
    struct symt                 symt;
    struct hash_table_elt       hash_elt;
    struct symt*                container;      /* compiland */
    ULONG_PTR                   address;
    ULONG_PTR                   size;
    THUNK_ORDINAL               ordinal;        /* FIXME: doesn't seem to be accessible */
};

struct symt_custom
{
    struct symt                 symt;
    struct hash_table_elt       hash_elt;
    DWORD64                     address;
    DWORD                       size;
};

/* class tree */
struct symt_array
{
    struct symt                 symt;
    int		                start;
    DWORD                       count;
    struct symt*                base_type;
    struct symt*                index_type;
};

struct symt_basic
{
    struct symt                 symt;
    enum BasicType              bt;
    ULONG_PTR                   size;
};

struct symt_enum
{
    struct symt                 symt;
    struct hash_table_elt       hash_elt;
    struct symt*                base_type;
    struct vector               vchildren;
};

struct symt_function_signature
{
    struct symt                 symt;
    struct symt*                rettype;
    struct vector               vchildren;
    enum CV_call_e              call_conv;
};

struct symt_function_arg_type
{
    struct symt                 symt;
    struct symt*                arg_type;
};

struct symt_pointer
{
    struct symt                 symt;
    struct symt*                pointsto;
    ULONG_PTR                   size;
};

struct symt_typedef
{
    struct symt                 symt;
    struct hash_table_elt       hash_elt;
    struct symt*                type;
};

struct symt_udt
{
    struct symt                 symt;
    struct hash_table_elt       hash_elt;
    enum UdtKind                kind;
    int		                size;
    struct vector               vchildren;
};

struct process;
struct module;

/* a module can be made of several debug information formats, so we have to
 * support them all
 */
enum format_info
{
    DFI_ELF,
    DFI_PE,
    DFI_MACHO,
    DFI_DWARF,
    DFI_PDB,
    DFI_LAST
};

struct lineinfo_t
{
    BOOL                        unicode;
    PVOID                       key;
    DWORD                       line_number;
    union
    {
        CHAR*                   file_nameA;
        WCHAR*                  file_nameW;
    };
    DWORD64                     address;
};

struct module_format;
enum method_result {MR_SUCCESS, MR_FAILURE, MR_NOT_FOUND};
struct module_format_vtable
{
    /* module handling */
    void                        (*remove)(struct module_format* modfmt);
    /* stack walk */
    void                        (*loc_compute)(const struct module_format* modfmt,
                                               const struct symt_function* func,
                                               struct location* loc);
    /* line information */
    enum method_result          (*get_line_from_address)(struct module_format *modfmt,
                                                         DWORD64 address, struct lineinfo_t *line_info);
    enum method_result          (*advance_line_info)(struct module_format *modfmt,
                                                     struct lineinfo_t *line_info, BOOL forward);
    enum method_result          (*enumerate_lines)(struct module_format *modfmt, const WCHAR* compiland_regex,
                                                   const WCHAR *source_file_regex, PSYM_ENUMLINES_CALLBACK cb, void *user);

    /* source files information */
    enum method_result          (*enumerate_sources)(struct module_format *modfmt, const WCHAR *sourcefile_regex,
                                                     PSYM_ENUMSOURCEFILES_CALLBACKW cb, void *user);
};

struct module_format
{
    struct module*                      module;
    const struct module_format_vtable*  vtable;

    union
    {
        struct elf_module_info*         elf_info;
        struct dwarf2_module_info_s*    dwarf2_info;
        struct pe_module_info*          pe_info;
        struct macho_module_info*	macho_info;
        struct pdb_module_info*         pdb_info;
    } u;
};

struct cpu;

struct module
{
    struct process*             process;
    IMAGEHLP_MODULEW64          module;
    WCHAR                       modulename[64]; /* used for enumeration */
    WCHAR*                      alt_modulename; /* used in symbol lookup */
    struct module*              next;
    enum dhext_module_type	type : 16;
    unsigned short              is_virtual : 1,
                                dont_load_symbols : 1,
                                is_wine_builtin : 1,
                                has_file_image : 1;
    struct cpu*                 cpu;
    DWORD64                     reloc_delta;
    WCHAR*                      real_path;

    /* specific information for debug types */
    struct module_format*       format_info[DFI_LAST];
    unsigned                    debug_format_bitmask;

    /* memory allocation pool */
    struct pool                 pool;

    /* symbols & symbol tables */
    struct vector               vsymt;
    struct vector               vcustom_symt;
    int                         sortlist_valid;
    unsigned                    num_sorttab;    /* number of symbols with addresses */
    unsigned                    num_symbols;
    unsigned                    sorttab_size;
    struct symt_ht**            addr_sorttab;
    struct hash_table           ht_symbols;
    struct symt_module*         top;

    /* types */
    struct hash_table           ht_types;

    /* source files */
    unsigned                    sources_used;
    unsigned                    sources_alloc;
    char*                       sources;
    struct wine_rb_tree         sources_offsets_tree;
};

struct module_format_vtable_iterator
{
    int dfi;
    struct module_format *modfmt;
};

#define MODULE_FORMAT_VTABLE_INDEX(f) (offsetof(struct module_format_vtable, f) / sizeof(void*))

static inline BOOL module_format_vtable_iterator_next(struct module *module, struct module_format_vtable_iterator *iter, size_t method_index)
{
    for ( ; iter->dfi < DFI_LAST; iter->dfi++)
    {
        iter->modfmt = module->format_info[iter->dfi];
        if (iter->modfmt && ((const void**)iter->modfmt->vtable)[method_index])
        {
            iter->dfi++;
            return TRUE;
        }
    }
    iter->modfmt = NULL;
    return FALSE;
}

typedef BOOL (*enum_modules_cb)(const WCHAR*, ULONG_PTR addr, void* user);

struct loader_ops
{
    BOOL (*synchronize_module_list)(struct process* process);
    struct module* (*load_module)(struct process* process, const WCHAR* name, ULONG_PTR addr);
    BOOL (*load_debug_info)(struct process *process, struct module* module);
    BOOL (*enum_modules)(struct process* process, enum_modules_cb callback, void* user);
    BOOL (*fetch_file_info)(struct process* process, const WCHAR* name, ULONG_PTR load_addr, DWORD_PTR* base, DWORD* size, DWORD* checksum);
};

struct process 
{
    struct process*             next;
    HANDLE                      handle;
    const struct loader_ops*    loader;
    WCHAR*                      search_path;
    WCHAR*                      environment;

    PSYMBOL_REGISTERED_CALLBACK64       reg_cb;
    PSYMBOL_REGISTERED_CALLBACK reg_cb32;
    BOOL                        reg_is_unicode;
    DWORD64                     reg_user;

    struct module*              lmodules;
    ULONG_PTR                   dbg_hdr_addr;

    IMAGEHLP_STACK_FRAME        ctx_frame;
    DWORD64                     localscope_pc;
    struct symt*                localscope_symt;

    unsigned                    buffer_size;
    void*                       buffer;

    BOOL                        is_64bit;
    BOOL                        is_host_64bit;
};

static inline BOOL read_process_memory(const struct process *process, UINT64 addr, void *buf, size_t size)
{
    return ReadProcessMemory(process->handle, (void*)(UINT_PTR)addr, buf, size, NULL);
}

static inline BOOL read_process_integral_value(const struct process* process, UINT64 addr, UINT64* pvalue, size_t size)
{
    /* Assuming that debugger and debuggee are little endian. */
    UINT64 value = 0;
    if (size > sizeof(value) || !read_process_memory(process, addr, &value, size)) return FALSE;
    *pvalue = value;
    return TRUE;
}

struct line_info
{
    ULONG_PTR                   is_first : 1,
                                is_last : 1,
                                is_source_file : 1,
                                line_number;
    union
    {
        ULONG_PTR                   address;     /* absolute, if is_source_file isn't set */
        unsigned                    source_file; /* if is_source_file is set */
    } u;
};

struct module_pair
{
    struct process*             pcs;
    struct module*              requested; /* in:  to module_get_debug() */
    struct module*              effective; /* out: module with debug info */
};

struct cpu_stack_walk
{
    HANDLE                      hProcess;
    HANDLE                      hThread;
    BOOL                        is32;
    struct cpu *                cpu;
    union
    {
        struct
        {
            PREAD_PROCESS_MEMORY_ROUTINE        f_read_mem;
            PTRANSLATE_ADDRESS_ROUTINE          f_xlat_adr;
            PFUNCTION_TABLE_ACCESS_ROUTINE      f_tabl_acs;
            PGET_MODULE_BASE_ROUTINE            f_modl_bas;
        } s32;
        struct
        {
            PREAD_PROCESS_MEMORY_ROUTINE64      f_read_mem;
            PTRANSLATE_ADDRESS_ROUTINE64        f_xlat_adr;
            PFUNCTION_TABLE_ACCESS_ROUTINE64    f_tabl_acs;
            PGET_MODULE_BASE_ROUTINE64          f_modl_bas;
        } s64;
    } u;
};

struct dump_memory
{
    ULONG64                             base;
    ULONG                               size;
    ULONG                               rva;
};

struct dump_memory64
{
    ULONG64                             base;
    ULONG64                             size;
};

struct dump_module
{
    unsigned                            is_elf;
    ULONG64                             base;
    ULONG                               size;
    DWORD                               timestamp;
    DWORD                               checksum;
    WCHAR                               name[MAX_PATH];
};

struct dump_thread
{
    ULONG                               tid;
    ULONG                               prio_class;
    ULONG                               curr_prio;
};

struct dump_context
{
    /* process & thread information */
    struct process                     *process;
    DWORD                               pid;
    unsigned                            flags_out;
    /* thread information */
    struct dump_thread*                 threads;
    unsigned                            num_threads;
    /* module information */
    struct dump_module*                 modules;
    unsigned                            num_modules;
    unsigned                            alloc_modules;
    /* outter information */
    MINIDUMP_EXCEPTION_INFORMATION     *except_param;
    MINIDUMP_USER_STREAM_INFORMATION   *user_stream;
    /* output information */
    MINIDUMP_TYPE                       type;
    HANDLE                              hFile;
    RVA                                 rva;
    struct dump_memory*                 mem;
    unsigned                            num_mem;
    unsigned                            alloc_mem;
    struct dump_memory64*               mem64;
    unsigned                            num_mem64;
    unsigned                            alloc_mem64;
    /* callback information */
    MINIDUMP_CALLBACK_INFORMATION*      cb;
};

union ctx
{
    CONTEXT ctx;
    WOW64_CONTEXT x86;
};

enum cpu_addr {cpu_addr_pc, cpu_addr_stack, cpu_addr_frame};
struct cpu
{
    DWORD       machine;
    DWORD       word_size;
    DWORD       frame_regno;

    /* address manipulation */
    BOOL        (*get_addr)(HANDLE hThread, const CONTEXT* ctx,
                            enum cpu_addr, ADDRESS64* addr);

    /* stack manipulation */
    BOOL        (*stack_walk)(struct cpu_stack_walk *csw, STACKFRAME64 *frame,
                              union ctx *ctx);

    /* module manipulation */
    void*       (*find_runtime_function)(struct module*, DWORD64 addr);

    /* dwarf dedicated information */
    unsigned    (*map_dwarf_register)(unsigned regno, const struct module* module, BOOL eh_frame);

    /* context related manipulation */
    void *      (*fetch_context_reg)(union ctx *ctx, unsigned regno, unsigned *size);
    const char* (*fetch_regname)(unsigned regno);

    /* minidump per CPU extension */
    BOOL        (*fetch_minidump_thread)(struct dump_context* dc, unsigned index, unsigned flags, const CONTEXT* ctx);
    BOOL        (*fetch_minidump_module)(struct dump_context* dc, unsigned index, unsigned flags);
};

extern struct cpu*      dbghelp_current_cpu;

/* PDB and Codeview */

struct msc_debug_info
{
    struct module*              module;
    int                         nsect;
    const IMAGE_SECTION_HEADER* sectp;
    int                         nomap;
    const OMAP*                 omapp;
    const BYTE*                 root;
};

/* coff.c */
extern BOOL coff_process_info(const struct msc_debug_info* msc_dbg);

/* dbghelp.c */
extern struct process* process_find_by_handle(HANDLE hProcess);
extern BOOL         validate_addr64(DWORD64 addr);
extern BOOL         pcs_callback(const struct process* pcs, ULONG action, void* data);
extern void*        fetch_buffer(struct process* pcs, unsigned size);
extern const char*  wine_dbgstr_addr(const ADDRESS64* addr);
extern struct cpu*  cpu_find(DWORD);
extern const WCHAR *process_getenv(const struct process *process, const WCHAR *name);
extern const struct cpu* process_get_cpu(const struct process* pcs);
extern DWORD calc_crc32(HANDLE handle);

/* elf_module.c */
extern BOOL         elf_read_wine_loader_dbg_info(struct process* pcs, ULONG_PTR addr);
struct elf_thunk_area;
extern int          elf_is_in_thunk_area(ULONG_PTR addr, const struct elf_thunk_area* thunks);

/* macho_module.c */
extern BOOL         macho_read_wine_loader_dbg_info(struct process* pcs, ULONG_PTR addr);

/* minidump.c */
void minidump_add_memory_block(struct dump_context* dc, ULONG64 base, ULONG size, ULONG rva);

/* module.c */
extern const WCHAR      S_WineLoaderW[];
extern const struct loader_ops no_loader_ops;
extern const struct loader_ops empty_loader_ops;

extern BOOL         module_init_pair(struct module_pair* pair, HANDLE hProcess,
                                     DWORD64 addr);
extern struct module*
                    module_find_by_addr(const struct process* pcs, DWORD64 addr);
extern struct module*
                    module_find_by_nameW(const struct process* pcs,
                                         const WCHAR* name);
extern struct module*
                    module_find_by_nameA(const struct process* pcs,
                                         const char* name);
extern struct module*
                    module_is_already_loaded(const struct process* pcs,
                                             const WCHAR* imgname);
extern BOOL         module_get_debug(struct module_pair*);
extern struct module*
                    module_new(struct process* pcs, const WCHAR* name,
                               enum dhext_module_type type, BOOL builtin, BOOL virtual,
                               DWORD64 addr, DWORD64 size,
                               ULONG_PTR stamp, ULONG_PTR checksum, WORD machine);
extern struct module*
                    module_get_containee(const struct process* pcs,
                                         const struct module* inner);
extern void         module_reset_debug_info(struct module* module);
extern BOOL         module_remove(struct process* pcs,
                                  struct module* module);
extern void         module_set_module(struct module* module, const WCHAR* name);
extern const WCHAR *get_wine_loader_name(struct process *pcs);
extern BOOL         module_is_wine_host(const WCHAR* module_name, const WCHAR* ext);
extern BOOL         module_refresh_list(struct process *pcs);

/* msc.c */
extern BOOL         pe_load_debug_directory(const struct process* pcs,
                                            struct module* module,
                                            const BYTE* mapping,
                                            const IMAGE_SECTION_HEADER* sectp, DWORD nsect,
                                            const IMAGE_DEBUG_DIRECTORY* dbg, int nDbg);
extern DWORD        msc_get_file_indexinfo(void* image, const IMAGE_DEBUG_DIRECTORY* dbgdir, DWORD size,
                                           SYMSRV_INDEX_INFOW* info);
struct pdb_cmd_pair {
    const char*         name;
    DWORD*              pvalue;
};
extern BOOL pdb_virtual_unwind(struct cpu_stack_walk *csw, DWORD_PTR ip,
    union ctx *context, struct pdb_cmd_pair *cpair);
extern DWORD pdb_get_file_indexinfo(void* image, DWORD size, SYMSRV_INDEX_INFOW* info);
extern DWORD dbg_get_file_indexinfo(void* image, DWORD size, SYMSRV_INDEX_INFOW* info);

/* path.c */
extern BOOL         path_find_symbol_file(const struct process *pcs, const struct module *module,
                                          PCSTR full_path, BOOL is_pdb, const GUID* guid, DWORD dw1, DWORD dw2,
                                          SYMSRV_INDEX_INFOW *info, BOOL *unmatched);
extern WCHAR *get_dos_file_name(const WCHAR *filename) __WINE_DEALLOC(HeapFree, 3) __WINE_MALLOC;
extern BOOL         search_dll_path(const struct process* process, const WCHAR *name, WORD machine,
                                    BOOL (*match)(void*, HANDLE, const WCHAR*), void *param);
extern BOOL search_unix_path(const WCHAR *name, const WCHAR *path, BOOL (*match)(void*, HANDLE, const WCHAR*), void *param);
extern const WCHAR* file_name(const WCHAR* str);
extern const char* file_nameA(const char* str);

/* pe_module.c */
extern BOOL         pe_load_nt_header(HANDLE hProc, DWORD64 base, IMAGE_NT_HEADERS* nth, BOOL* is_builtin);
extern struct module*
                    pe_load_native_module(struct process* pcs, const WCHAR* name,
                                          HANDLE hFile, DWORD64 base, DWORD size);
extern struct module*
                    pe_load_builtin_module(struct process* pcs, const WCHAR* name,
                                           DWORD64 base, DWORD64 size);
extern BOOL         pe_load_debug_info(const struct process* pcs,
                                       struct module* module);
extern const char*  pe_map_directory(struct module* module, int dirno, DWORD* size);
extern BOOL         pe_unmap_directory(struct module* module, int dirno, const char*);
extern DWORD        pe_get_file_indexinfo(void* image, DWORD size, SYMSRV_INDEX_INFOW* info);
extern const BYTE*  pe_lock_region_from_rva(struct module *module, DWORD rva, DWORD size, DWORD *length);
extern BOOL         pe_unlock_region(struct module *module, const BYTE* region);
struct image_file_map;
extern BOOL         pe_has_buildid_debug(struct image_file_map *fmap, GUID *guid);

/* source.c */
extern unsigned     source_new(struct module* module, const char* basedir, const char* source);
extern const char*  source_get(const struct module* module, unsigned idx);
extern int          source_rb_compare(const void *key, const struct wine_rb_entry *entry);

/* stabs.c */
typedef void (*stabs_def_cb)(struct module* module, ULONG_PTR load_offset,
                                const char* name, ULONG_PTR offset,
                                BOOL is_public, BOOL is_global, unsigned char other,
                                struct symt_compiland* compiland, void* user);
extern BOOL         stabs_parse(struct module* module, ULONG_PTR load_offset,
                                const char* stabs, size_t nstab, size_t stabsize,
                                const char* strs, int strtablen,
                                stabs_def_cb callback, void* user);

/* dwarf.c */
struct image_file_map;
extern BOOL         dwarf2_parse(struct module* module, ULONG_PTR load_offset,
                                 const struct elf_thunk_area* thunks,
                                 struct image_file_map* fmap);
extern BOOL dwarf2_virtual_unwind(struct cpu_stack_walk *csw, DWORD_PTR ip,
    union ctx *ctx, DWORD64 *cfa);

/* stack.c */
extern BOOL         sw_read_mem(struct cpu_stack_walk* csw, DWORD64 addr, void* ptr, DWORD sz);
extern DWORD64      sw_xlat_addr(struct cpu_stack_walk* csw, ADDRESS64* addr);
extern void*        sw_table_access(struct cpu_stack_walk* csw, DWORD64 addr);
extern DWORD64      sw_module_base(struct cpu_stack_walk* csw, DWORD64 addr);

/* symbol.c */
extern const char*  symt_get_name(const struct symt* sym);
extern WCHAR*       symt_get_nameW(const struct symt* sym);
extern BOOL         symt_get_address(const struct symt* type, ULONG64* addr);
extern int __cdecl  symt_cmp_addr(const void* p1, const void* p2);
extern void         copy_symbolW(SYMBOL_INFOW* siw, const SYMBOL_INFO* si);
extern void         symbol_setname(SYMBOL_INFO* si, const char* name);
extern BOOL         symt_match_stringAW(const char *string, const WCHAR *re, BOOL _case);
extern struct symt_ht*
                    symt_find_nearest(struct module* module, DWORD_PTR addr);
extern struct symt_ht*
                    symt_find_symbol_at(struct module* module, DWORD_PTR addr);
extern struct symt_module*
                    symt_new_module(struct module* module);
extern struct symt_compiland*
                    symt_new_compiland(struct module* module, unsigned src_idx);
extern struct symt_public*
                    symt_new_public(struct module* module, 
                                    struct symt_compiland* parent, 
                                    const char* typename,
                                    BOOL is_function,
                                    ULONG_PTR address,
                                    unsigned size);
extern struct symt_data*
                    symt_new_global_variable(struct module* module,
                                             struct symt_compiland* parent,
                                             const char* name, unsigned is_static,
                                             struct location loc, ULONG_PTR size,
                                             symref_t type);
extern struct symt_function*
                    symt_new_function(struct module* module,
                                      struct symt_compiland* parent,
                                      const char* name,
                                      ULONG_PTR addr, ULONG_PTR size,
                                      symref_t type);
extern struct symt_function*
                    symt_new_inlinesite(struct module* module,
                                        struct symt_function* func,
                                        struct symt* parent,
                                        const char* name,
                                        symref_t type,
                                        unsigned num_ranges);
extern void         symt_add_func_line(struct module* module,
                                       struct symt_function* func, 
                                       unsigned source_idx, int line_num, 
                                       ULONG_PTR offset);
extern struct symt_data*
                    symt_add_func_local(struct module* module,
                                        struct symt_function* func,
                                        enum DataKind dt, const struct location* loc,
                                        struct symt_block* block,
                                        symref_t, const char* name);
extern struct symt_data*
                    symt_add_func_constant(struct module* module,
                                           struct symt_function* func, struct symt_block* block,
                                           symref_t, const char* name, VARIANT* v);
extern struct symt_block*
                    symt_open_func_block(struct module* module,
                                         struct symt_function* func,
                                         struct symt_block* block,
                                         unsigned num_ranges);
extern struct symt_block*
                    symt_close_func_block(struct module* module,
                                          const struct symt_function* func,
                                          struct symt_block* block);
extern struct symt_hierarchy_point*
                    symt_add_function_point(struct module* module, 
                                            struct symt_function* func,
                                            enum SymTagEnum point, 
                                            const struct location* loc,
                                            const char* name);
extern struct symt_thunk*
                    symt_new_thunk(struct module* module,
                                   struct symt_compiland* parent,
                                   const char* name, THUNK_ORDINAL ord,
                                   ULONG_PTR addr, ULONG_PTR size);
extern struct symt_data*
                    symt_new_constant(struct module* module,
                                      struct symt_compiland* parent,
                                      const char* name, symref_t type,
                                      const VARIANT* v);
extern struct symt_hierarchy_point*
                    symt_new_label(struct module* module,
                                   struct symt_compiland* compiland,
                                   const char* name, ULONG_PTR address);
extern struct symt* symt_index_to_ptr(struct module* module, DWORD id);
extern DWORD        symt_ptr_to_index(struct module* module, const struct symt* sym);
extern DWORD        symt_symref_to_index(struct module* module, symref_t ref);
static inline symref_t
                    symt_ptr_to_symref(const struct symt *symt) {return (ULONG_PTR)symt;}
extern struct symt_custom*
                    symt_new_custom(struct module* module, const char* name,
                                    DWORD64 addr, DWORD size);
extern BOOL         lineinfo_set_nameA(struct process* pcs, struct lineinfo_t* intl, char* str);

/* type.c */
extern void         symt_init_basic(struct module* module);
extern BOOL         symt_get_info(struct module* module, const struct symt* type,
                                  IMAGEHLP_SYMBOL_TYPE_INFO req, void* pInfo);
extern BOOL         symt_get_info_from_index(struct module* module, DWORD index,
                                             IMAGEHLP_SYMBOL_TYPE_INFO req, void* pInfo);
extern BOOL         symt_get_info_from_symref(struct module* module, symref_t type,
                                              IMAGEHLP_SYMBOL_TYPE_INFO req, void* pInfo);
extern struct symt_basic*
                    symt_get_basic(enum BasicType, unsigned size);
extern struct symt_udt*
                    symt_new_udt(struct module* module, const char* typename,
                                 unsigned size, enum UdtKind kind);
extern BOOL         symt_set_udt_size(struct module* module,
                                      struct symt_udt* type, unsigned size);
extern BOOL         symt_add_udt_element(struct module* module,
                                         struct symt_udt* udt_type,
                                         const char* name,
                                         symref_t elt_type, unsigned offset,
                                         unsigned bit_offset, unsigned bit_size);
extern struct symt_enum*
                    symt_new_enum(struct module* module, const char* typename,
                                  struct symt* basetype);
extern BOOL         symt_add_enum_element(struct module* module,
                                          struct symt_enum* enum_type,
                                          const char* name, const VARIANT *value);
extern struct symt_array*
                    symt_new_array(struct module* module, int min, DWORD count,
                                   struct symt* base, struct symt* index);
extern struct symt_function_signature*
                    symt_new_function_signature(struct module* module, 
                                                struct symt* ret_type,
                                                enum CV_call_e call_conv);
extern BOOL         symt_add_function_signature_parameter(struct module* module,
                                                          struct symt_function_signature* sig,
                                                          struct symt* param);
extern struct symt_pointer*
                    symt_new_pointer(struct module* module, 
                                     struct symt* ref_type,
                                     ULONG_PTR size);
extern struct symt_typedef*
                    symt_new_typedef(struct module* module, struct symt* ref, 
                                     const char* name);
extern struct symt_function*
                    symt_find_lowest_inlined(struct symt_function* func, DWORD64 addr);
extern struct symt*
                    symt_get_upper_inlined(struct symt_function* inlined);
static inline struct symt_function*
                    symt_get_function_from_inlined(struct symt_function* inlined)
{
    while (!symt_check_tag(&inlined->symt, SymTagFunction))
        inlined = (struct symt_function*)symt_get_upper_inlined(inlined);
    return inlined;
}
extern struct symt_function*
                    symt_find_inlined_site(struct module* module,
                                           DWORD64 addr, DWORD inline_ctx);

/* Inline context encoding (different from what native does):
 * bits 31:30: 3 ignore (includes INLINE_FRAME_CONTEXT_IGNORE=0xFFFFFFFF)
 *             2 regular frame
 *             1 frame with inlined function(s).
 *             0 init   (includes INLINE_FRAME_CONTEXT_INIT=0)
 * so either stackwalkex is called with:
 * - inlinectx=IGNORE, and we use (old) StackWalk64 behavior:
 * - inlinectx=INIT, and StackWalkEx will upon return swing back&forth between:
 *      INLINE when the frame is from an inline site (inside a function)
 *      REGULAR when the frame is for a function without inline site
 * bits 29:00  depth of inline site (way too big!!)
 *             0 being the lowest inline site
 */
#define IFC_MODE_IGNORE  0xC0000000
#define IFC_MODE_REGULAR 0x80000000
#define IFC_MODE_INLINE  0x40000000
#define IFC_MODE_INIT    0x00000000
#define IFC_DEPTH_MASK   0x3FFFFFFF
#define IFC_MODE(x)      ((x) & ~IFC_DEPTH_MASK)
#define IFC_DEPTH(x)     ((x) & IFC_DEPTH_MASK)

/* temporary helpers for PDB rewriting */
struct _PDB_FPO_DATA;
extern BOOL pdb_fpo_unwind_parse_cmd_string(struct cpu_stack_walk* csw, struct _PDB_FPO_DATA* fpoext,
                                            const char* cmd, struct pdb_cmd_pair* cpair);
extern BOOL pdb_old_virtual_unwind(struct cpu_stack_walk *csw, DWORD_PTR ip,
                                   union ctx *context, struct pdb_cmd_pair *cpair);
struct pdb_reader;
extern BOOL pdb_hack_get_main_info(struct module_format *modfmt, struct pdb_reader **pdb, unsigned *fpoext_stream);
extern void pdb_reader_dispose(struct pdb_reader *pdb);
extern struct pdb_reader *pdb_hack_reader_init(struct module *module, HANDLE file, const IMAGE_SECTION_HEADER *sections, unsigned num_sections);
