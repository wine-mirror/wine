/*
 * 32-bit spec files
 *
 * Copyright 1993 Robert J. Amstadt
 * Copyright 1995 Martin von Loewis
 * Copyright 1995, 1996, 1997 Alexandre Julliard
 * Copyright 1997 Eric Youngdale
 * Copyright 1999 Ulrich Weigand
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

#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <string.h>

#include "build.h"

#define IMAGE_FILE_MACHINE_UNKNOWN 0
#define IMAGE_FILE_MACHINE_I386    0x014c
#define IMAGE_FILE_MACHINE_POWERPC 0x01f0
#define IMAGE_FILE_MACHINE_AMD64   0x8664
#define IMAGE_FILE_MACHINE_ARMNT   0x01c4
#define IMAGE_FILE_MACHINE_ARM64   0xaa64

#define IMAGE_SIZEOF_NT_OPTIONAL32_HEADER 224
#define IMAGE_SIZEOF_NT_OPTIONAL64_HEADER 240

#define IMAGE_NT_OPTIONAL_HDR32_MAGIC 0x10b
#define IMAGE_NT_OPTIONAL_HDR64_MAGIC 0x20b
#define IMAGE_ROM_OPTIONAL_HDR_MAGIC  0x107

int needs_get_pc_thunk = 0;

static const char builtin_signature[32] = "Wine builtin DLL";
static const char fakedll_signature[32] = "Wine placeholder DLL";
static struct strarray spec_extra_ld_symbols = { 0 }; /* list of extra symbols that ld should resolve */

/* add a symbol to the list of extra symbols that ld must resolve */
void add_spec_extra_ld_symbol( const char *name )
{
    strarray_add( &spec_extra_ld_symbols, name );
}

static unsigned int hash_filename( const char *name )
{
    /* FNV-1 hash */
    unsigned int ret = 2166136261u;
    while (*name) ret = (ret * 16777619) ^ *name++;
    return ret;
}

/* check if entry point needs a relay thunk */
static inline int needs_relay( const ORDDEF *odp )
{
    /* skip nonexistent entry points */
    if (!odp) return 0;
    /* skip non-functions */
    switch (odp->type)
    {
    case TYPE_STDCALL:
    case TYPE_CDECL:
        break;
    case TYPE_STUB:
        if (odp->u.func.nb_args != -1) break;
        /* fall through */
    default:
        return 0;
    }
    /* skip norelay and forward entry points */
    if (odp->flags & (FLAG_NORELAY|FLAG_FORWARD)) return 0;
    return 1;
}

static int is_float_arg( const ORDDEF *odp, int arg )
{
    if (arg >= odp->u.func.nb_args) return 0;
    return (odp->u.func.args[arg] == ARG_FLOAT || odp->u.func.args[arg] == ARG_DOUBLE);
}

/* check if dll will output relay thunks */
static int has_relays( struct exports *exports )
{
    int i;

    if (target.cpu == CPU_ARM64EC) return 0;

    for (i = exports->base; i <= exports->limit; i++)
    {
        ORDDEF *odp = exports->ordinals[i];
        if (needs_relay( odp )) return 1;
    }
    return 0;
}

static int get_exports_count( struct exports *exports )
{
    if (exports->base > exports->limit) return 0;
    return exports->limit - exports->base + 1;
}

static int cmp_func_args( const void *p1, const void *p2 )
{
    const ORDDEF *odp1 = *(const ORDDEF **)p1;
    const ORDDEF *odp2 = *(const ORDDEF **)p2;

    return odp2->u.func.nb_args - odp1->u.func.nb_args;
}

static void get_arg_string( ORDDEF *odp, char str[MAX_ARGUMENTS + 1] )
{
    int i;

    for (i = 0; i < odp->u.func.nb_args; i++)
    {
        switch (odp->u.func.args[i])
        {
        case ARG_STR: str[i] = 's'; break;
        case ARG_WSTR: str[i] = 'w'; break;
        case ARG_FLOAT: str[i] = 'f'; break;
        case ARG_DOUBLE: str[i] = 'd'; break;
        case ARG_INT64:
        case ARG_INT128:
            if (get_ptr_size() == 4)
            {
                str[i] = (odp->u.func.args[i] == ARG_INT64) ? 'j' : 'k';
                break;
            }
            /* fall through */
        case ARG_LONG:
        case ARG_PTR:
        default:
            str[i] = 'i';
            break;
        }
    }
    if (odp->flags & (FLAG_THISCALL | FLAG_FASTCALL)) str[0] = 't';
    if ((odp->flags & FLAG_FASTCALL) && odp->u.func.nb_args > 1) str[1] = 't';

    /* append return value */
    if (get_ptr_size() == 4 && (odp->flags & FLAG_RET64))
        strcpy( str + i, "J" );
    else
        strcpy( str + i, "I" );
}

static void output_data_directories( const char *names[16] )
{
    int i;

    for (i = 0; i < 16; i++)
    {
        if (names[i])
        {
            output_rva( "%s", names[i] );
            output( "\t.long %s_end - %s\n", names[i], names[i] );
        }
        else output( "\t.long 0,0\n" );
    }
}

/*******************************************************************
 *         build_args_string
 */
static char *build_args_string( struct exports *exports )
{
    int i, count = 0, len = 1;
    char *p, *buffer;
    char str[MAX_ARGUMENTS + 2];
    ORDDEF **funcs;

    funcs = xmalloc( (exports->limit + 1 - exports->base) * sizeof(*funcs) );
    for (i = exports->base; i <= exports->limit; i++)
    {
        ORDDEF *odp = exports->ordinals[i];

        if (!needs_relay( odp )) continue;
        funcs[count++] = odp;
        len += odp->u.func.nb_args + 1;
    }
    /* sort functions by decreasing number of arguments */
    qsort( funcs, count, sizeof(*funcs), cmp_func_args );
    buffer = xmalloc( len );
    buffer[0] = 0;
    /* build the arguments string, reusing substrings where possible */
    for (i = 0; i < count; i++)
    {
        get_arg_string( funcs[i], str );
        if (!(p = strstr( buffer, str )))
        {
            p = buffer + strlen( buffer );
            strcpy( p, str );
        }
        funcs[i]->u.func.args_str_offset = p - buffer;
    }
    free( funcs );
    return buffer;
}

/*******************************************************************
 *         output_relay_debug
 *
 * Output entry points for relay debugging
 */
static void output_relay_debug( struct exports *exports )
{
    int i;

    /* first the table of entry point offsets */

    output( "\t%s\n", get_asm_rodata_section() );
    output( "\t.balign 4\n" );
    output( ".L__wine_spec_relay_entry_point_offsets:\n" );

    for (i = exports->base; i <= exports->limit; i++)
    {
        ORDDEF *odp = exports->ordinals[i];

        if (needs_relay( odp ))
            output( "\t.long __wine_spec_relay_entry_point_%d-__wine_spec_relay_entry_points\n", i );
        else
            output( "\t.long 0\n" );
    }

    /* then the strings of argument types */

    output( ".L__wine_spec_relay_args_string:\n" );
    output( "\t%s \"%s\"\n", get_asm_string_keyword(), build_args_string( exports ));

    /* then the relay thunks */

    output( "\t.text\n" );
    output( "__wine_spec_relay_entry_points:\n" );
    output( "\tnop\n" );  /* to avoid 0 offset */

    for (i = exports->base; i <= exports->limit; i++)
    {
        ORDDEF *odp = exports->ordinals[i];

        if (!needs_relay( odp )) continue;

        switch (target.cpu)
        {
        case CPU_i386:
            output( "\t.balign 4\n" );
            output( "\t.long 0x90909090,0x90909090\n" );
            output( "__wine_spec_relay_entry_point_%d:\n", i );
            output_cfi( ".cfi_startproc" );
            output( "\t.byte 0x8b,0xff,0x55,0x8b,0xec,0x5d\n" );  /* hotpatch prolog */
            if (odp->flags & (FLAG_THISCALL | FLAG_FASTCALL))  /* add the register arguments */
            {
                output( "\tpopl %%eax\n" );
                if ((odp->flags & FLAG_FASTCALL) && get_args_size( odp ) > 4) output( "\tpushl %%edx\n" );
                output( "\tpushl %%ecx\n" );
                output( "\tpushl %%eax\n" );
            }
            output( "\tpushl $%u\n", (odp->u.func.args_str_offset << 16) | (i - exports->base) );
            output_cfi( ".cfi_adjust_cfa_offset 4" );

            if (UsePIC)
            {
                output( "\tcall %s\n", asm_name("__wine_spec_get_pc_thunk_eax") );
                output( "1:\tleal .L__wine_spec_relay_descr-1b(%%eax),%%eax\n" );
                needs_get_pc_thunk = 1;
            }
            else output( "\tmovl $.L__wine_spec_relay_descr,%%eax\n" );
            output( "\tpushl %%eax\n" );
            output_cfi( ".cfi_adjust_cfa_offset 4" );

            output( "\tcall *4(%%eax)\n" );
            output_cfi( ".cfi_adjust_cfa_offset -8" );
            if (odp->type == TYPE_STDCALL)
                output( "\tret $%u\n", get_args_size( odp ));
            else
                output( "\tret\n" );
            output_cfi( ".cfi_endproc" );
            break;

        case CPU_ARM:
        {
            int j, has_float = 0;

            for (j = 0; j < odp->u.func.nb_args && !has_float; j++)
                has_float = is_float_arg( odp, j );

            output( "\t.balign 4\n" );
            output( "__wine_spec_relay_entry_point_%d:\n", i );
            output( "\t.seh_proc __wine_spec_relay_entry_point_%d\n", i );
            output( "\tpush {r0-r3}\n" );
            output( "\t.seh_save_regs {r0-r3}\n" );
            if (has_float)
            {
                output( "\tvpush {d0-d7}\n" );
                output( "\t.seh_save_fregs {d0-d7}\n" );
            }
            output( "\tpush {r4,lr}\n" );
            output( "\t.seh_save_regs {r4,lr}\n" );
            output( "\t.seh_endprologue\n" );
            output( "\tmovw r1,#%u\n", i - exports->base );
            output( "\tmovt r1,#%u\n", odp->u.func.args_str_offset );
            output( "\tmovw r0, :lower16:.L__wine_spec_relay_descr\n" );
            output( "\tmovt r0, :upper16:.L__wine_spec_relay_descr\n" );
            output( "\tldr IP, [r0, #4]\n");
            output( "\tblx IP\n");
            output( "\tldr IP, [SP, #4]\n" );
            output( "\tadd SP, #%u\n", 24 + (has_float ? 64 : 0) );
            output( "\tbx IP\n");
            output( "\t.seh_endproc\n" );
            break;
        }

        case CPU_ARM64:
        {
            int stack_size = 16 * ((min(odp->u.func.nb_args, 8) + 1) / 2);

            output( "\t.balign 4\n" );
            output( "__wine_spec_relay_entry_point_%d:\n", i );
            output( "\t.seh_proc __wine_spec_relay_entry_point_%d\n", i );
            output( "\tstp x29, x30, [sp, #-%u]!\n", stack_size + 16 );
            output( "\t.seh_save_fplr_x %u\n", stack_size + 16 );
            output( "\tmov x29, sp\n" );
            output( "\t.seh_set_fp\n" );
            output( "\t.seh_endprologue\n" );
            switch (stack_size)
            {
            case 64: output( "\tstp x6, x7, [sp, #64]\n" );
            /* fall through */
            case 48: output( "\tstp x4, x5, [sp, #48]\n" );
            /* fall through */
            case 32: output( "\tstp x2, x3, [sp, #32]\n" );
            /* fall through */
            case 16: output( "\tstp x0, x1, [sp, #16]\n" );
            /* fall through */
            default: break;
            }
            output( "\tadd x2, sp, #16\n");
            output( "\tstp x8, x9, [SP,#-16]!\n" );
            output( "\tmov w1, #%u\n", odp->u.func.args_str_offset << 16 );
            if (i - exports->base) output( "\tadd w1, w1, #%u\n", i - exports->base );
            output( "\tadrp x0, .L__wine_spec_relay_descr\n" );
            output( "\tadd x0, x0, #:lo12:.L__wine_spec_relay_descr\n" );
            output( "\tldr x3, [x0, #8]\n");
            output( "\tblr x3\n");
            output( "\tmov sp, x29\n" );
            output( "\tldp x29, x30, [sp], #%u\n", stack_size + 16 );
            output( "\tret\n");
            output( "\t.seh_endproc\n" );
            break;
        }

        case CPU_x86_64:
            output( "\t.balign 4\n" );
            output( "\t.long 0x90909090,0x90909090\n" );
            output( "__wine_spec_relay_entry_point_%d:\n", i );
            output_seh( ".seh_proc __wine_spec_relay_entry_point_%d", i );
            output_seh( ".seh_endprologue" );
            switch (odp->u.func.nb_args)
            {
            default: output( "\tmovq %%%s,32(%%rsp)\n", is_float_arg( odp, 3 ) ? "xmm3" : "r9" );
            /* fall through */
            case 3:  output( "\tmovq %%%s,24(%%rsp)\n", is_float_arg( odp, 2 ) ? "xmm2" : "r8" );
            /* fall through */
            case 2:  output( "\tmovq %%%s,16(%%rsp)\n", is_float_arg( odp, 1 ) ? "xmm1" : "rdx" );
            /* fall through */
            case 1:  output( "\tmovq %%%s,8(%%rsp)\n", is_float_arg( odp, 0 ) ? "xmm0" : "rcx" );
            /* fall through */
            case 0:  break;
            }
            output( "\tmovl $%u,%%edx\n", (odp->u.func.args_str_offset << 16) | (i - exports->base) );
            output( "\tleaq .L__wine_spec_relay_descr(%%rip),%%rcx\n" );
            output( "\tcallq *8(%%rcx)\n" );
            output( "\tret\n" );
            output_seh( ".seh_endproc" );
            break;

        default:
            assert(0);
        }
    }
}

/*******************************************************************
 *         output_exports
 *
 * Output the export table for a Win32 module.
 */
void output_exports( DLLSPEC *spec )
{
    struct exports *exports = &spec->exports;
    int i, fwd_size = 0;
    int needs_imports = 0;
    int needs_relay = has_relays( exports );
    int nr_exports = get_exports_count( exports );
    const char *func_ptr = is_pe() ? ".rva" : get_asm_ptr_keyword();
    const char *name;

    if (!nr_exports) return;

    /* ARM64EC exports are more tricky than other targets. For functions implemented in ARM64EC,
     * linker generates x86_64 thunk and relevant metadata. Use .drectve section to pass export
     * directives to the linker. */
    if (target.cpu == CPU_ARM64EC)
    {
        output( "\t.section .drectve\n" );
        for (i = exports->base; i <= exports->limit; i++)
        {
            ORDDEF *odp = exports->ordinals[i];
            const char *symbol;

            if (!odp) continue;

            switch (odp->type)
            {
            case TYPE_EXTERN:
            case TYPE_STDCALL:
            case TYPE_VARARGS:
            case TYPE_CDECL:
                if (odp->flags & FLAG_FORWARD)
                    symbol = odp->link_name;
                else if (odp->flags & FLAG_EXT_LINK)
                    symbol = strmake( "%s_%s", asm_name("__wine_spec_ext_link"), odp->link_name );
                else
                    symbol = asm_name( get_link_name( odp ));
                break;
            case TYPE_STUB:
                symbol = asm_name( get_stub_name( odp, spec ));
                break;
            default:
                assert( 0 );
            }

            output( "\t.ascii \" -export:%s=%s,@%u%s%s\"\n",
                    odp->name ? odp->name : strmake( "_noname%u", i ),
                    symbol, i,
                    odp->name ? "" : ",NONAME",
                    odp->type == TYPE_EXTERN ? ",DATA" : "" );
        }
        return;
    }

    output( "\n/* export table */\n\n" );
    output( "\t%s\n", get_asm_export_section() );
    output( "\t.balign 4\n" );
    output( ".L__wine_spec_exports:\n" );

    /* export directory header */

    output( "\t.long 0\n" );                       /* Characteristics */
    output( "\t.long %u\n", hash_filename(spec->file_name) ); /* TimeDateStamp */
    output( "\t.long 0\n" );                       /* MajorVersion/MinorVersion */
    output_rva( ".L__wine_spec_exp_names" );       /* Name */
    output( "\t.long %u\n", exports->base );       /* Base */
    output( "\t.long %u\n", nr_exports );          /* NumberOfFunctions */
    output( "\t.long %u\n", exports->nb_names );   /* NumberOfNames */
    output_rva( ".L__wine_spec_exports_funcs " );  /* AddressOfFunctions */
    if (exports->nb_names)
    {
        output_rva( ".L__wine_spec_exp_name_ptrs" ); /* AddressOfNames */
        output_rva( ".L__wine_spec_exp_ordinals" );  /* AddressOfNameOrdinals */
    }
    else
    {
        output( "\t.long 0\n" );  /* AddressOfNames */
        output( "\t.long 0\n" );  /* AddressOfNameOrdinals */
    }

    /* output the function pointers */

    output( "\n.L__wine_spec_exports_funcs:\n" );
    for (i = exports->base; i <= exports->limit; i++)
    {
        ORDDEF *odp = exports->ordinals[i];
        if (!odp) output( "\t%s 0\n", is_pe() ? ".long" : get_asm_ptr_keyword() );
        else switch(odp->type)
        {
        case TYPE_EXTERN:
        case TYPE_STDCALL:
        case TYPE_VARARGS:
        case TYPE_CDECL:
            if (odp->flags & FLAG_FORWARD)
            {
                output( "\t%s .L__wine_spec_forwards+%u\n", func_ptr, fwd_size );
                fwd_size += strlen(odp->link_name) + 1;
            }
            else if ((odp->flags & FLAG_IMPORT) && (target.cpu == CPU_i386 || target.cpu == CPU_x86_64))
            {
                name = odp->name ? odp->name : odp->export_name;
                if (name) output( "\t%s %s_%s\n", func_ptr, asm_name("__wine_spec_imp"), name );
                else output( "\t%s %s_%u\n", func_ptr, asm_name("__wine_spec_imp"), i );
                needs_imports = 1;
            }
            else if (odp->flags & FLAG_EXT_LINK)
            {
                output( "\t%s %s_%s\n", func_ptr, asm_name("__wine_spec_ext_link"), odp->link_name );
            }
            else
            {
                output( "\t%s %s\n", func_ptr, asm_name( get_link_name( odp )));
            }
            break;
        case TYPE_STUB:
            output( "\t%s %s\n", func_ptr, asm_name( get_stub_name( odp, spec )) );
            break;
        default:
            assert(0);
        }
    }

    if (exports->nb_names)
    {
        /* output the function name pointers */

        int namepos = strlen(spec->file_name) + 1;

        output( "\n.L__wine_spec_exp_name_ptrs:\n" );
        for (i = 0; i < exports->nb_names; i++)
        {
            output_rva( ".L__wine_spec_exp_names + %u", namepos );
            namepos += strlen(exports->names[i]->name) + 1;
        }

        /* output the function ordinals */

        output( "\n.L__wine_spec_exp_ordinals:\n" );
        for (i = 0; i < exports->nb_names; i++)
        {
            output( "\t.short %d\n", exports->names[i]->ordinal - exports->base );
        }
        if (exports->nb_names % 2)
        {
            output( "\t.short 0\n" );
        }
    }

    if (needs_relay)
    {
        output( "\t.long 0xdeb90002\n" );  /* magic */
        if (is_pe()) output_rva( ".L__wine_spec_relay_descr" );
        else output( "\t.long 0\n" );
    }

    /* output the export name strings */

    output( "\n.L__wine_spec_exp_names:\n" );
    output( "\t%s \"%s\"\n", get_asm_string_keyword(), spec->file_name );
    for (i = 0; i < exports->nb_names; i++)
        output( "\t%s \"%s\"\n",
                 get_asm_string_keyword(), exports->names[i]->name );

    /* output forward strings */

    if (fwd_size)
    {
        output( "\n.L__wine_spec_forwards:\n" );
        for (i = exports->base; i <= exports->limit; i++)
        {
            ORDDEF *odp = exports->ordinals[i];
            if (odp && (odp->flags & FLAG_FORWARD))
                output( "\t%s \"%s\"\n", get_asm_string_keyword(), odp->link_name );
        }
    }

    /* output relays */

    if (needs_relay)
    {
        if (is_pe())
        {
            output( "\t.data\n" );
            output( "\t.balign %u\n", get_ptr_size() );
        }
        else
        {
            output( "\t.balign %u\n", get_ptr_size() );
            output( ".L__wine_spec_exports_end:\n" );
        }

        output( ".L__wine_spec_relay_descr:\n" );
        output( "\t%s 0xdeb90002\n", get_asm_ptr_keyword() );  /* magic */
        output( "\t%s 0\n", get_asm_ptr_keyword() );           /* relay func */
        output( "\t%s 0\n", get_asm_ptr_keyword() );           /* private data */
        output( "\t%s __wine_spec_relay_entry_points\n", get_asm_ptr_keyword() );
        output( "\t%s .L__wine_spec_relay_entry_point_offsets\n", get_asm_ptr_keyword() );
        output( "\t%s .L__wine_spec_relay_args_string\n", get_asm_ptr_keyword() );

        output_relay_debug( exports );
    }
    else if (!is_pe())
    {
            output( "\t.balign %u\n", get_ptr_size() );
            output( ".L__wine_spec_exports_end:\n" );
            output( "\t%s 0\n", get_asm_ptr_keyword() );
    }

    /* output import thunks */

    if (!needs_imports) return;
    output( "\t.text\n" );
    for (i = exports->base; i <= exports->limit; i++)
    {
        ORDDEF *odp = exports->ordinals[i];
        if (!odp) continue;
        if (!(odp->flags & FLAG_IMPORT)) continue;

        name = odp->name ? odp->name : odp->export_name;

        output( "\t.balign 4\n" );
        output( "\t.long 0x90909090,0x90909090\n" );
        if (name) output( "%s_%s:\n", asm_name("__wine_spec_imp"), name );
        else output( "%s_%u:\n", asm_name("__wine_spec_imp"), i );

        switch (target.cpu)
        {
        case CPU_i386:
            output( "\t.byte 0x8b,0xff,0x55,0x8b,0xec,0x5d\n" );  /* hotpatch prolog */
            if (UsePIC)
            {
                output( "\tcall %s\n", asm_name("__wine_spec_get_pc_thunk_eax") );
                output( "1:\tjmp *__imp_%s-1b(%%eax)\n", asm_name( get_link_name( odp )));
                needs_get_pc_thunk = 1;
            }
            else output( "\tjmp *__imp_%s\n", asm_name( get_link_name( odp )));
            break;
        case CPU_x86_64:
            output( "\t.byte 0x48,0x8d,0xa4,0x24,0x00,0x00,0x00,0x00\n" );  /* hotpatch prolog */
            output( "\tjmp *__imp_%s(%%rip)\n", asm_name( get_link_name( odp )));
            break;
        default:
            assert(0);
        }
    }
}


/*******************************************************************
 *         output_load_config
 *
 * Output the load configuration structure.
 */
static void output_load_config(void)
{
    if (!is_pe()) return;

    output( "\n/* load_config */\n\n" );
    output( "\t%s\n", get_asm_rodata_section() );
    output( "\t.globl %s\n", asm_name( "_load_config_used" ));
    output( "\t.balign %u\n", get_ptr_size() );
    output( "%s:\n", asm_name( "_load_config_used" ));
    output( "\t.long %u\n", get_ptr_size() == 8 ? 0x140 : 0xc0 ); /* Size */
    output( "\t.long 0\n" );                          /* TimeDateStamp */
    output( "\t.short 0\n" );                         /* MajorVersion */
    output( "\t.short 0\n" );                         /* MinorVersion */
    output( "\t.long 0\n" );                          /* GlobalFlagsClear */
    output( "\t.long 0\n" );                          /* GlobalFlagsSet */
    output( "\t.long 0\n" );                          /* CriticalSectionDefaultTimeout */
    output( "\t%s 0\n", get_asm_ptr_keyword() );      /* DeCommitFreeBlockThreshold */
    output( "\t%s 0\n", get_asm_ptr_keyword() );      /* DeCommitTotalFreeThreshold */
    output( "\t%s 0\n", get_asm_ptr_keyword() );      /* LockPrefixTable */
    output( "\t%s 0\n", get_asm_ptr_keyword() );      /* MaximumAllocationSize */
    output( "\t%s 0\n", get_asm_ptr_keyword() );      /* VirtualMemoryThreshold */
    output( "\t%s 0\n", get_asm_ptr_keyword() );      /* ProcessAffinityMask */
    output( "\t.long 0\n" );                          /* ProcessHeapFlags */
    output( "\t.short 0\n" );                         /* CSDVersion */
    output( "\t.short 0\n" );                         /* DependentLoadFlags */
    output( "\t%s 0\n", get_asm_ptr_keyword() );      /* EditList */
    output( "\t%s 0\n", get_asm_ptr_keyword() );      /* SecurityCookie */
    output( "\t%s 0\n", get_asm_ptr_keyword() );      /* SEHandlerTable */
    output( "\t%s 0\n", get_asm_ptr_keyword() );      /* SEHandlerCount */
    if (target.cpu == CPU_ARM64EC)
    {
        output( "\t%s %s\n", get_asm_ptr_keyword(), asm_name( "__guard_check_icall_fptr" ));
        output( "\t%s %s\n", get_asm_ptr_keyword(), asm_name( "__guard_dispatch_icall_fptr" ));
    }
    else
    {
        output( "\t%s 0\n", get_asm_ptr_keyword() );  /* GuardCFCheckFunctionPointer */
        output( "\t%s 0\n", get_asm_ptr_keyword() );  /* GuardCFDispatchFunctionPointer */
    }
    output( "\t%s 0\n", get_asm_ptr_keyword() );      /* GuardCFFunctionTable */
    output( "\t%s 0\n", get_asm_ptr_keyword() );      /* GuardCFFunctionCount */
    if (target.cpu == CPU_ARM64EC)
        output( "\t.long %s\n", asm_name( "__guard_flags" ));
    else
        output( "\t.long 0\n" );                      /* GuardFlags */
    output( "\t.short 0\n" );                         /* CodeIntegrity.Flags */
    output( "\t.short 0\n" );                         /* CodeIntegrity.Catalog */
    output( "\t.long  0\n" );                         /* CodeIntegrity.CatalogOffset */
    output( "\t.long  0\n" );                         /* CodeIntegrity.Reserved */
    output( "\t%s 0\n", get_asm_ptr_keyword() );      /* GuardAddressTakenIatEntryTable */
    output( "\t%s 0\n", get_asm_ptr_keyword() );      /* GuardAddressTakenIatEntryCount */
    output( "\t%s 0\n", get_asm_ptr_keyword() );      /* GuardLongJumpTargetTable */
    output( "\t%s 0\n", get_asm_ptr_keyword() );      /* GuardLongJumpTargetCount */
    output( "\t%s 0\n", get_asm_ptr_keyword() );      /* DynamicValueRelocTable */
    if (target.cpu == CPU_ARM64EC)
        output( "\t%s %s\n", get_asm_ptr_keyword(), asm_name( "__chpe_metadata" ));
     else
        output( "\t%s  0\n", get_asm_ptr_keyword() ); /* CHPEMetadataPointer */
    output( "\t%s 0\n", get_asm_ptr_keyword() );      /* GuardRFFailureRoutine */
    output( "\t%s 0\n", get_asm_ptr_keyword() );      /* GuardRFFailureRoutineFunctionPointer */
    output( "\t.long  0\n" );                         /* DynamicValueRelocTableOffset */
    output( "\t.short 0\n" );                         /* DynamicValueRelocTableSection */
    output( "\t.short 0\n" );                         /* Reserved2 */
    output( "\t%s 0\n", get_asm_ptr_keyword() );      /* GuardRFVerifyStackPointerFunctionPointer */
    output( "\t.long  0\n" );                         /* HotPatchTableOffset */
    output( "\t.long  0\n" );                         /* Reserved3 */
    output( "\t%s 0\n", get_asm_ptr_keyword() );      /* EnclaveConfigurationPointer */
    output( "\t%s 0\n", get_asm_ptr_keyword() );      /* VolatileMetadataPointer */
    output( "\t%s 0\n", get_asm_ptr_keyword() );      /* GuardEHContinuationTable */
    output( "\t%s 0\n", get_asm_ptr_keyword() );      /* GuardEHContinuationCount */
    output( "\t%s 0\n", get_asm_ptr_keyword() );      /* GuardXFGCheckFunctionPointer */
    output( "\t%s 0\n", get_asm_ptr_keyword() );      /* GuardXFGDispatchFunctionPointer */
    output( "\t%s 0\n", get_asm_ptr_keyword() );      /* GuardXFGTableDispatchFunctionPointer */
    output( "\t%s 0\n", get_asm_ptr_keyword() );      /* CastGuardOsDeterminedFailureMode */
    output( "\t%s 0\n", get_asm_ptr_keyword() );      /* GuardMemcpyFunctionPointer */
}


/*******************************************************************
 *         output_module
 *
 * Output the module data.
 */
void output_module( DLLSPEC *spec )
{
    int machine = 0;
    int i;
    unsigned int page_size = 0x1000;
    const char *data_dirs[16] = { NULL };

    /* Reserve some space for the PE header */

    switch (target.platform)
    {
    case PLATFORM_MINGW:
    case PLATFORM_WINDOWS:
        return;  /* nothing to do */
    case PLATFORM_APPLE:
        output( "\t.text\n" );
        output( "\t.balign %u\n", page_size );
        output( "__wine_spec_pe_header:\n" );
        output( "\t.space 65536\n" );
        break;
    case PLATFORM_SOLARIS:
        output( "\n\t.section \".text\",\"ax\"\n" );
        output( "__wine_spec_pe_header:\n" );
        output( "\t.skip %u\n", 65536 + page_size );
        break;
    default:
        output( "\n\t.section \".init\",\"ax\"\n" );
        output( "\tjmp 1f\n" );
        output( "__wine_spec_pe_header:\n" );
        output( "\t.skip %u\n", 65536 + page_size );
        output( "1:\n" );
        break;
    }

    /* Output the NT header */

    output( "\n\t.data\n" );
    output( "\t.balign %u\n", get_ptr_size() );
    output( "\t.globl %s\n", asm_name("__wine_spec_nt_header") );
    output( "%s:\n", asm_name("__wine_spec_nt_header") );
    output( ".L__wine_spec_rva_base:\n" );

    output( "\t.long 0x4550\n" );         /* Signature */
    switch (target.cpu)
    {
    case CPU_i386:    machine = IMAGE_FILE_MACHINE_I386; break;
    case CPU_ARM64EC:
    case CPU_x86_64:  machine = IMAGE_FILE_MACHINE_AMD64; break;
    case CPU_ARM:     machine = IMAGE_FILE_MACHINE_ARMNT; break;
    case CPU_ARM64:   machine = IMAGE_FILE_MACHINE_ARM64; break;
    }
    output( "\t.short 0x%04x\n",          /* Machine */
             machine );
    output( "\t.short 0\n" );             /* NumberOfSections */
    output( "\t.long %u\n", hash_filename(spec->file_name) );  /* TimeDateStamp */
    output( "\t.long 0\n" );              /* PointerToSymbolTable */
    output( "\t.long 0\n" );              /* NumberOfSymbols */
    output( "\t.short %d\n",              /* SizeOfOptionalHeader */
             get_ptr_size() == 8 ? IMAGE_SIZEOF_NT_OPTIONAL64_HEADER : IMAGE_SIZEOF_NT_OPTIONAL32_HEADER );
    output( "\t.short 0x%04x\n",          /* Characteristics */
             spec->characteristics );
    output( "\t.short 0x%04x\n",          /* Magic */
             get_ptr_size() == 8 ? IMAGE_NT_OPTIONAL_HDR64_MAGIC : IMAGE_NT_OPTIONAL_HDR32_MAGIC );
    output( "\t.byte 7\n" );              /* MajorLinkerVersion */
    output( "\t.byte 10\n" );             /* MinorLinkerVersion */
    output( "\t.long 0\n" );              /* SizeOfCode */
    output( "\t.long 0\n" );              /* SizeOfInitializedData */
    output( "\t.long 0\n" );              /* SizeOfUninitializedData */

    for (i = 0; i < spec_extra_ld_symbols.count; i++)
        output( "\t.globl %s\n", asm_name(spec_extra_ld_symbols.str[i]) );

    /* note: we expand the AddressOfEntryPoint field on 64-bit by overwriting the BaseOfCode field */
    output( "\t%s %s\n",                  /* AddressOfEntryPoint */
            get_asm_ptr_keyword(), spec->init_func ? asm_name(spec->init_func) : "0" );
    if (get_ptr_size() == 4)
    {
        output( "\t.long 0\n" );          /* BaseOfCode */
        output( "\t.long 0\n" );          /* BaseOfData */
    }
    output( "\t%s __wine_spec_pe_header\n",         /* ImageBase */
             get_asm_ptr_keyword() );
    output( "\t.long %u\n", page_size );  /* SectionAlignment */
    output( "\t.long %u\n", page_size );  /* FileAlignment */
    output( "\t.short 1,0\n" );           /* Major/MinorOperatingSystemVersion */
    output( "\t.short 0,0\n" );           /* Major/MinorImageVersion */
    output( "\t.short %u,%u\n",           /* Major/MinorSubsystemVersion */
             spec->subsystem_major, spec->subsystem_minor );
    output( "\t.long 0\n" );                          /* Win32VersionValue */
    output_rva( "%s", asm_name("_end") ); /* SizeOfImage */
    output( "\t.long %u\n", page_size );  /* SizeOfHeaders */
    output( "\t.long 0\n" );              /* CheckSum */
    output( "\t.short 0x%04x\n",          /* Subsystem */
             spec->subsystem );
    output( "\t.short 0x%04x\n",          /* DllCharacteristics */
            spec->dll_characteristics );
    output( "\t%s %u,%u\n",               /* SizeOfStackReserve/Commit */
             get_asm_ptr_keyword(), (spec->stack_size ? spec->stack_size : 1024) * 1024, page_size );
    output( "\t%s %u,%u\n",               /* SizeOfHeapReserve/Commit */
             get_asm_ptr_keyword(), (spec->heap_size ? spec->heap_size : 1024) * 1024, page_size );
    output( "\t.long 0\n" );              /* LoaderFlags */
    output( "\t.long 16\n" );             /* NumberOfRvaAndSizes */

    if (get_exports_count( &spec->exports ))
        data_dirs[0] = ".L__wine_spec_exports";   /* DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT] */
    if (has_imports())
        data_dirs[1] = ".L__wine_spec_imports";   /* DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT] */
    if (spec->nb_resources)
        data_dirs[2] = ".L__wine_spec_resources"; /* DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE] */
    if (has_delay_imports())
        data_dirs[13] = ".L__wine_spec_delay_imports"; /* DataDirectory[IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT] */

    output_data_directories( data_dirs );

    if (target.platform == PLATFORM_APPLE)
        output( "\t.lcomm %s,4\n", asm_name("_end") );
}


/*******************************************************************
 *         output_spec32_file
 *
 * Build a Win32 C file from a spec file.
 */
void output_spec32_file( DLLSPEC *spec )
{
    needs_get_pc_thunk = 0;
    open_output_file();
    output_standard_file_header();
    output_module( spec );
    output_stubs( spec );
    output_exports( spec );
    output_imports( spec );
    if (needs_get_pc_thunk) output_get_pc_thunk();
    output_load_config();
    output_resources( spec );
    output_gnu_stack_note();
    close_output_file();
}


struct sec_data
{
    char         name[8];
    const void  *ptr;
    unsigned int size;
    unsigned int flags;
    unsigned int file_size;
    unsigned int virt_size;
    unsigned int filepos;
    unsigned int rva;
};

struct dir_data
{
    unsigned int rva;
    unsigned int size;
};

struct exp_data
{
    unsigned int rva;
    const char  *name;
};

static struct
{
    unsigned int    section_align;
    unsigned int    file_align;
    unsigned int    sec_count;
    unsigned int    exp_count;
    struct dir_data dir[16];
    struct sec_data sec[8];
    struct exp_data exp[8];
} pe;

static void set_dir( unsigned int idx, unsigned int rva, unsigned int size )
{
    pe.dir[idx].rva  = rva;
    pe.dir[idx].size = size;
}

static void add_export( unsigned int rva, const char *name )
{
    pe.exp[pe.exp_count].rva  = rva;
    pe.exp[pe.exp_count].name = name;
    pe.exp_count++;
}

static unsigned int current_rva(void)
{
    if (!pe.sec_count) return pe.section_align;
    return pe.sec[pe.sec_count - 1].rva + pe.sec[pe.sec_count - 1].virt_size;
}

static unsigned int current_filepos(void)
{
    if (!pe.sec_count) return pe.file_align;
    return pe.sec[pe.sec_count - 1].filepos + pe.sec[pe.sec_count - 1].file_size;
}

static unsigned int flush_output_to_section( const char *name, int dir_idx, unsigned int flags )
{
    struct sec_data *sec = &pe.sec[pe.sec_count];

    if (!output_buffer_pos) return 0;

    memset( sec->name, 0, sizeof(sec->name) );
    memcpy( sec->name, name, min( strlen(name), sizeof(sec->name) ));
    sec->ptr       = output_buffer;
    sec->size      = output_buffer_pos;
    sec->flags     = flags;
    sec->rva       = current_rva();
    sec->filepos   = current_filepos();
    sec->file_size = (sec->size + pe.file_align - 1) & ~(pe.file_align - 1);
    sec->virt_size = (sec->size + pe.section_align - 1) & ~(pe.section_align - 1);
    if (dir_idx >= 0) set_dir( dir_idx, sec->rva, sec->size );
    init_output_buffer();
    pe.sec_count++;
    return sec->size;
}

static void output_pe_exports( DLLSPEC *spec )
{
    struct exports *exports = &spec->exports;
    unsigned int i, exp_count = get_exports_count( exports );
    unsigned int exp_rva = current_rva() + 40; /* sizeof(IMAGE_EXPORT_DIRECTORY) */
    unsigned int pos, str_rva = exp_rva + 4 * exp_count + 6 * exports->nb_names;

    if (!exports->nb_entry_points) return;

    init_output_buffer();
    put_dword( 0 );               /* Characteristics */
    put_dword( hash_filename(spec->file_name) );     /* TimeDateStamp */
    put_word( 0 );                /* MajorVersion */
    put_word( 0 );                /* MinorVersion */
    put_dword( str_rva );         /* Name */
    put_dword( exports->base );   /* Base */
    put_dword( exp_count );       /* NumberOfFunctions */
    put_dword( exports->nb_names );  /* NumberOfNames */
    put_dword( exp_rva );         /* AddressOfFunctions */
    if (exports->nb_names)
    {
        put_dword( exp_rva + 4 * exp_count );                       /* AddressOfNames */
        put_dword( exp_rva + 4 * exp_count + 4 * exports->nb_names );  /* AddressOfNameOrdinals */
    }
    else
    {
        put_dword( 0 );   /* AddressOfNames */
        put_dword( 0 );   /* AddressOfNameOrdinals */
    }

    /* functions */
    for (i = 0, pos = str_rva + strlen(spec->file_name) + 1; i < exports->nb_names; i++)
        pos += strlen( exports->names[i]->name ) + 1;
    for (i = exports->base; i <= exports->limit; i++)
    {
        ORDDEF *odp = exports->ordinals[i];
        if (odp && (odp->flags & FLAG_FORWARD))
        {
            put_dword( pos );
            pos += strlen(odp->link_name) + 1;
        }
        else put_dword( 0 );
    }

    /* names */
    for (i = 0, pos = str_rva + strlen(spec->file_name) + 1; i < exports->nb_names; i++)
    {
        put_dword( pos );
        pos += strlen(exports->names[i]->name) + 1;
    }

    /* ordinals */
    for (i = 0; i < exports->nb_names; i++) put_word( exports->names[i]->ordinal - exports->base );

    /* strings */
    put_data( spec->file_name, strlen(spec->file_name) + 1 );
    for (i = 0; i < exports->nb_names; i++)
        put_data( exports->names[i]->name, strlen(exports->names[i]->name) + 1 );

    for (i = exports->base; i <= exports->limit; i++)
    {
        ORDDEF *odp = exports->ordinals[i];
        if (odp && (odp->flags & FLAG_FORWARD)) put_data( odp->link_name, strlen(odp->link_name) + 1 );
    }

    flush_output_to_section( ".edata", 0 /* IMAGE_DIRECTORY_ENTRY_EXPORT */,
                             0x40000040 /* CNT_INITIALIZED_DATA|MEM_READ */ );
}


/* apiset hash table */
struct apiset_hash_entry
{
    unsigned int hash;
    unsigned int index;
};

static int apiset_hash_cmp( const void *h1, const void *h2 )
{
    const struct apiset_hash_entry *entry1 = h1;
    const struct apiset_hash_entry *entry2 = h2;

    if (entry1->hash > entry2->hash) return 1;
    if (entry1->hash < entry2->hash) return -1;
    return 0;
}

static void output_apiset_section( const struct apiset *apiset )
{
    struct apiset_hash_entry *hash;
    struct apiset_entry *e;
    unsigned int i, j, str_pos, value_pos, hash_pos, size;

    init_output_buffer();

    value_pos = 0x1c /* header */ + apiset->count * 0x18; /* names */
    str_pos = value_pos;
    for (i = 0, e = apiset->entries; i < apiset->count; i++, e++)
        str_pos += 0x14 * max( 1, e->val_count );  /* values */

    hash_pos = str_pos + ((apiset->str_pos * 2 + 3) & ~3);
    size = hash_pos + apiset->count * 8;  /* hashes */

    /* header */

    put_dword( 6 );      /* Version */
    put_dword( size );   /* Size */
    put_dword( 0 );      /* Flags */
    put_dword( apiset->count );  /* Count */
    put_dword( 0x1c );   /* EntryOffset */
    put_dword( hash_pos ); /* HashOffset */
    put_dword( apiset_hash_factor );   /* HashFactor */

    /* name entries */

    value_pos = 0x1c /* header */ + apiset->count * 0x18; /* names */
    for (i = 0, e = apiset->entries; i < apiset->count; i++, e++)
    {
        put_dword( 1 );  /* Flags */
        put_dword( str_pos + e->name_off * 2 );  /* NameOffset */
        put_dword( e->name_len * 2 );  /* NameLength */
        put_dword( e->hash_len * 2 );  /* HashedLength */
        put_dword( value_pos );  /* ValueOffset */
        put_dword( max( 1, e->val_count ));          /* ValueCount */
        value_pos += 0x14 * max( 1, e->val_count );
    }

    /* values */

    for (i = 0, e = apiset->entries; i < apiset->count; i++, e++)
    {
        if (!e->val_count)
        {
            put_dword( 0 );  /* Flags */
            put_dword( 0 );  /* NameOffset */
            put_dword( 0 );  /* NameLength */
            put_dword( 0 );  /* ValueOffset */
            put_dword( 0 );  /* ValueLength */
        }
        else for (j = 0; j < e->val_count; j++)
        {
            put_dword( 0 );  /* Flags */
            if (e->values[j].name_off)
            {
                put_dword( str_pos + e->values[j].name_off * 2 );  /* NameOffset */
                put_dword( e->values[j].name_len * 2 );  /* NameLength */
            }
            else
            {
                put_dword( 0 );  /* NameOffset */
                put_dword( 0 );  /* NameLength */
            }
            put_dword( str_pos + e->values[j].val_off * 2 );  /* ValueOffset */
            put_dword( e->values[j].val_len * 2 );  /* ValueLength */
        }
    }

    /* strings */

    for (i = 0; i < apiset->str_pos; i++) put_word( apiset->strings[i] );
    align_output( 4 );

    /* hash table */

    hash = xmalloc( apiset->count * sizeof(*hash) );
    for (i = 0, e = apiset->entries; i < apiset->count; i++, e++)
    {
        hash[i].hash = e->hash;
        hash[i].index = i;
    }
    qsort( hash, apiset->count, sizeof(*hash), apiset_hash_cmp );
    for (i = 0; i < apiset->count; i++)
    {
        put_dword( hash[i].hash );
        put_dword( hash[i].index );
    }
    free( hash );

    flush_output_to_section( ".apiset", -1, 0x40000040 /* CNT_INITIALIZED_DATA|MEM_READ */ );
}


static void output_pe_file( DLLSPEC *spec, const char signature[32] )
{
    const unsigned int lfanew = 0x40 + 32;
    unsigned int i, code_size = 0, data_size = 0;

    init_output_buffer();

    for (i = 0; i < pe.sec_count; i++)
        if (pe.sec[i].flags & 0x20) /* CNT_CODE */
            code_size += pe.sec[i].file_size;

    /* .rsrc section */
    if (spec->type == SPEC_WIN32)
    {
        output_bin_resources( spec, current_rva() );
        flush_output_to_section( ".rsrc", 2 /* IMAGE_DIRECTORY_ENTRY_RESOURCE */,
                                 0x40000040 /* CNT_INITIALIZED_DATA|MEM_READ */ );
    }

    /* .reloc section */
    if (code_size)
    {
        put_dword( 0 );  /* VirtualAddress */
        put_dword( 0 );  /* Size */
        flush_output_to_section( ".reloc", 5 /* IMAGE_DIRECTORY_ENTRY_BASERELOC */,
                                 0x42000040 /* CNT_INITIALIZED_DATA|MEM_DISCARDABLE|MEM_READ */ );
    }

    for (i = 0; i < pe.sec_count; i++)
        if ((pe.sec[i].flags & 0x60) == 0x40) /* CNT_INITIALIZED_DATA */
            data_size += pe.sec[i].file_size;

    put_word( 0x5a4d );       /* e_magic */
    put_word( 0x40 );         /* e_cblp */
    put_word( 0x01 );         /* e_cp */
    put_word( 0 );            /* e_crlc */
    put_word( lfanew / 16 );  /* e_cparhdr */
    put_word( 0x0000 );       /* e_minalloc */
    put_word( 0xffff );       /* e_maxalloc */
    put_word( 0x0000 );       /* e_ss */
    put_word( 0x00b8 );       /* e_sp */
    put_word( 0 );            /* e_csum */
    put_word( 0 );            /* e_ip */
    put_word( 0 );            /* e_cs */
    put_word( lfanew );       /* e_lfarlc */
    put_word( 0 );            /* e_ovno */
    put_dword( 0 );           /* e_res */
    put_dword( 0 );
    put_word( 0 );            /* e_oemid */
    put_word( 0 );            /* e_oeminfo */
    put_dword( 0 );           /* e_res2 */
    put_dword( 0 );
    put_dword( 0 );
    put_dword( 0 );
    put_dword( 0 );
    put_dword( lfanew );

    put_data( signature, 32 );

    put_dword( 0x4550 );                             /* Signature */
    switch (target.cpu)
    {
    case CPU_i386:    put_word( IMAGE_FILE_MACHINE_I386 ); break;
    case CPU_ARM64EC:
    case CPU_x86_64:  put_word( IMAGE_FILE_MACHINE_AMD64 ); break;
    case CPU_ARM:     put_word( IMAGE_FILE_MACHINE_ARMNT ); break;
    case CPU_ARM64:   put_word( IMAGE_FILE_MACHINE_ARM64 ); break;
    }
    put_word( pe.sec_count );                        /* NumberOfSections */
    put_dword( hash_filename(spec->file_name) );     /* TimeDateStamp */
    put_dword( 0 );                                  /* PointerToSymbolTable */
    put_dword( 0 );                                  /* NumberOfSymbols */
    put_word( get_ptr_size() == 8 ?
              IMAGE_SIZEOF_NT_OPTIONAL64_HEADER :
              IMAGE_SIZEOF_NT_OPTIONAL32_HEADER );   /* SizeOfOptionalHeader */
    put_word( spec->characteristics );               /* Characteristics */
    put_word( get_ptr_size() == 8 ?
              IMAGE_NT_OPTIONAL_HDR64_MAGIC :
              IMAGE_NT_OPTIONAL_HDR32_MAGIC );       /* Magic */
    put_byte(  7 );                                  /* MajorLinkerVersion */
    put_byte(  10 );                                 /* MinorLinkerVersion */
    put_dword( code_size );                          /* SizeOfCode */
    put_dword( data_size );                          /* SizeOfInitializedData */
    put_dword( 0 );                                  /* SizeOfUninitializedData */
    put_dword( code_size ? pe.sec[0].rva : 0 );      /* AddressOfEntryPoint */
    put_dword( code_size ? pe.sec[0].rva : 0 );      /* BaseOfCode */
    if (get_ptr_size() == 4)
    {
        put_dword( 0 );                              /* BaseOfData */
        put_dword( 0x10000000 );                     /* ImageBase */
    }
    else
    {
        put_dword( 0x80000000 );                     /* ImageBase */
        put_dword( 0x00000001 );
    }
    put_dword( pe.section_align );                   /* SectionAlignment */
    put_dword( pe.file_align );                      /* FileAlignment */
    put_word( 1 );                                   /* MajorOperatingSystemVersion */
    put_word( 0 );                                   /* MinorOperatingSystemVersion */
    put_word( 0 );                                   /* MajorImageVersion */
    put_word( 0 );                                   /* MinorImageVersion */
    put_word( spec->subsystem_major );               /* MajorSubsystemVersion */
    put_word( spec->subsystem_minor );               /* MinorSubsystemVersion */
    put_dword( 0 );                                  /* Win32VersionValue */
    put_dword( current_rva() );                      /* SizeOfImage */
    put_dword( pe.file_align );                      /* SizeOfHeaders */
    put_dword( 0 );                                  /* CheckSum */
    put_word( spec->subsystem );                     /* Subsystem */
    put_word( spec->dll_characteristics );           /* DllCharacteristics */
    put_pword( (spec->stack_size ? spec->stack_size : 1024) * 1024 ); /* SizeOfStackReserve */
    put_pword( pe.section_align );                   /* SizeOfStackCommit */
    put_pword( (spec->heap_size ? spec->heap_size : 1024) * 1024 );   /* SizeOfHeapReserve */
    put_pword( pe.section_align );                   /* SizeOfHeapCommit */
    put_dword( 0 );                                  /* LoaderFlags */
    put_dword( 16 );                                 /* NumberOfRvaAndSizes */

    /* image directories */
    for (i = 0; i < 16; i++)
    {
        put_dword( pe.dir[i].rva );  /* VirtualAddress */
        put_dword( pe.dir[i].size ); /* Size */
    }

    /* sections */
    for (i = 0; i < pe.sec_count; i++)
    {
        put_data( pe.sec[i].name, 8 );    /* Name */
        put_dword( pe.sec[i].size );      /* VirtualSize */
        put_dword( pe.sec[i].rva );       /* VirtualAddress */
        put_dword( pe.sec[i].file_size ); /* SizeOfRawData */
        put_dword( pe.sec[i].filepos );   /* PointerToRawData */
        put_dword( 0 );                   /* PointerToRelocations */
        put_dword( 0 );                   /* PointerToLinenumbers */
        put_word( 0 );                    /* NumberOfRelocations */
        put_word( 0 );                    /* NumberOfLinenumbers */
        put_dword( pe.sec[i].flags );     /* Characteristics  */
    }
    align_output( pe.file_align );

    /* section data */
    for (i = 0; i < pe.sec_count; i++)
    {
        put_data( pe.sec[i].ptr, pe.sec[i].size );
        align_output( pe.file_align );
    }

    flush_output_buffer( output_file_name ? output_file_name : spec->file_name );
}

/*******************************************************************
 *         output_fake_module
 *
 * Build a fake binary module from a spec file.
 */
void output_fake_module( DLLSPEC *spec )
{
    static const unsigned char dll_code_section[] = { 0x31, 0xc0,          /* xor %eax,%eax */
                                                      0xc2, 0x0c, 0x00 };  /* ret $12 */

    static const unsigned char exe_code_section[] = { 0xb8, 0x01, 0x00, 0x00, 0x00,  /* movl $1,%eax */
                                                      0xc2, 0x04, 0x00 };            /* ret $4 */
    unsigned int i;

    resolve_imports( spec );
    pe.section_align = 0x1000;
    pe.file_align    = 0x200;
    init_output_buffer();

    /* .text section */
    if (spec->characteristics & IMAGE_FILE_DLL) put_data( dll_code_section, sizeof(dll_code_section) );
    else put_data( exe_code_section, sizeof(exe_code_section) );
    flush_output_to_section( ".text", -1, 0x60000020 /* CNT_CODE|MEM_EXECUTE|MEM_READ */ );

    if (spec->type == SPEC_WIN16)
    {
        add_export( current_rva(), "__wine_spec_dos_header" );

        /* .rdata section */
        output_fake_module16( spec );
        if (spec->main_module)
        {
            add_export( current_rva() + output_buffer_pos, "__wine_spec_main_module" );
            put_data( spec->main_module, strlen(spec->main_module) + 1 );
        }
        flush_output_to_section( ".rdata", -1, 0x40000040 /* CNT_INITIALIZED_DATA|MEM_READ */ );
    }

    /* .edata section */
    if (pe.exp_count)
    {
        unsigned int exp_rva = current_rva() + 40; /* sizeof(IMAGE_EXPORT_DIRECTORY) */
        unsigned int pos, str_rva = exp_rva + 10 * pe.exp_count;

	put_dword( 0 );               /* Characteristics */
        put_dword( hash_filename(spec->file_name) );     /* TimeDateStamp */
        put_word( 0 );                /* MajorVersion */
        put_word( 0 );                /* MinorVersion */
        put_dword( str_rva );         /* Name */
        put_dword( 1 );               /* Base */
        put_dword( pe.exp_count );    /* NumberOfFunctions */
        put_dword( pe.exp_count );    /* NumberOfNames */
        put_dword( exp_rva );         /* AddressOfFunctions */
        put_dword( exp_rva + 4 * pe.exp_count );     /* AddressOfNames */
        put_dword( exp_rva + 8 * pe.exp_count );     /* AddressOfNameOrdinals */

        /* functions */
        for (i = 0; i < pe.exp_count; i++) put_dword( pe.exp[i].rva );
        /* names */
        for (i = 0, pos = str_rva + strlen(spec->file_name) + 1; i < pe.exp_count; i++)
        {
            put_dword( pos );
            pos += strlen( pe.exp[i].name ) + 1;
        }
        /* ordinals */
        for (i = 0; i < pe.exp_count; i++) put_word( i );
        /* strings */
        put_data( spec->file_name, strlen(spec->file_name) + 1 );
        for (i = 0; i < pe.exp_count; i++) put_data( pe.exp[i].name, strlen(pe.exp[i].name) + 1 );
        flush_output_to_section( ".edata", 0 /* IMAGE_DIRECTORY_ENTRY_EXPORT */,
                                 0x40000040 /* CNT_INITIALIZED_DATA|MEM_READ */ );
    }

    output_pe_file( spec, fakedll_signature );
}


/*******************************************************************
 *         output_data_module
 *
 * Build a data-only module from a spec file.
 */
void output_data_module( DLLSPEC *spec )
{
    pe.section_align = pe.file_align = get_section_alignment();

    output_pe_exports( spec );
    if (spec->apiset.count) output_apiset_section( &spec->apiset );
    output_pe_file( spec, builtin_signature );
}


/*******************************************************************
 *         output_def_file
 *
 * Build a Win32 def file from a spec file.
 */
void output_def_file( DLLSPEC *spec, struct exports *exports, int import_only )
{
    DLLSPEC *spec32 = NULL;
    const char *name;
    int i, total;

    if (spec->type == SPEC_WIN16)
    {
        spec32 = alloc_dll_spec();
        add_16bit_exports( spec32, spec );
        spec = spec32;
        exports = &spec->exports;
    }

    if (spec_file_name)
        output( "; File generated automatically from %s; do not edit!\n\n",
                 spec_file_name );
    else
        output( "; File generated automatically; do not edit!\n\n" );

    output( "LIBRARY %s\n\n", spec->file_name);
    output( "EXPORTS\n");

    /* Output the exports and relay entry points */

    for (i = total = 0; i < exports->nb_entry_points; i++)
    {
        const ORDDEF *odp = exports->entry_points[i];
        int is_data = 0, is_private = odp->flags & FLAG_PRIVATE;

        if (odp->name) name = odp->name;
        else if (odp->export_name) name = odp->export_name;
        else continue;

        if (!is_private) total++;
        if (import_only && odp->type == TYPE_STUB) continue;

        if ((odp->flags & FLAG_FASTCALL) && is_pe())
            name = strmake( "@%s", name );

        output( "  %s", name );

        switch(odp->type)
        {
        case TYPE_EXTERN:
            is_data = 1;
            /* fall through */
        case TYPE_VARARGS:
        case TYPE_CDECL:
            /* try to reduce output */
            if(!import_only && (strcmp(name, odp->link_name) || (odp->flags & FLAG_FORWARD)))
                output( "=%s", odp->link_name );
            break;
        case TYPE_STDCALL:
        {
            int at_param = get_args_size( odp );
            if (!kill_at && target.cpu == CPU_i386) output( "@%d", at_param );
            if (import_only) break;
            if  (odp->flags & FLAG_FORWARD)
                output( "=%s", odp->link_name );
            else if (strcmp(name, odp->link_name)) /* try to reduce output */
                output( "=%s", get_link_name( odp ));
            break;
        }
        case TYPE_STUB:
            if (!kill_at && target.cpu == CPU_i386) output( "@%d", get_args_size( odp ));
            is_private = 1;
            break;
        default:
            assert(0);
        }
        output( " @%d", odp->ordinal );
        if (!odp->name || (odp->flags & FLAG_ORDINAL)) output( " NONAME" );
        if (is_data) output( " DATA" );
        if (is_private) output( " PRIVATE" );
        output( "\n" );
    }
    if (!total) warning( "%s: Import library doesn't export anything\n", spec->file_name );
    if (spec32) free_dll_spec( spec32 );
}


/*******************************************************************
 *         make_builtin_files
 */
void make_builtin_files( struct strarray files )
{
    int i, fd;
    struct
    {
        unsigned short e_magic;
        unsigned short unused[29];
        unsigned int   e_lfanew;
    } header;

    for (i = 0; i < files.count; i++)
    {
        if ((fd = open( files.str[i], O_RDWR | O_BINARY )) == -1)
            fatal_perror( "Cannot open %s", files.str[i] );
        if (read( fd, &header, sizeof(header) ) == sizeof(header) && !memcmp( &header.e_magic, "MZ", 2 ))
        {
            if (header.e_lfanew < sizeof(header) + sizeof(builtin_signature))
                fatal_error( "%s: Not enough space (%x) for Wine signature\n", files.str[i], header.e_lfanew );
            write( fd, builtin_signature, sizeof(builtin_signature) );

            if (prefer_native)
            {
                unsigned int pos = header.e_lfanew + 0x5e;  /* OptionalHeader.DllCharacteristics */
                unsigned short dll_charact;
                lseek( fd, pos, SEEK_SET );
                if (read( fd, &dll_charact, sizeof(dll_charact) ) == sizeof(dll_charact))
                {
                    dll_charact |= IMAGE_DLLCHARACTERISTICS_PREFER_NATIVE;
                    lseek( fd, pos, SEEK_SET );
                    write( fd, &dll_charact, sizeof(dll_charact) );
                }
            }
        }
        else fatal_error( "%s: Unrecognized file format\n", files.str[i] );
        close( fd );
    }
}

static void fixup_elf32( const char *name, int fd, void *header, size_t header_size )
{
    struct
    {
        unsigned char  e_ident[16];
        unsigned short e_type;
        unsigned short e_machine;
        unsigned int   e_version;
        unsigned int   e_entry;
        unsigned int   e_phoff;
        unsigned int   e_shoff;
        unsigned int   e_flags;
        unsigned short e_ehsize;
        unsigned short e_phentsize;
        unsigned short e_phnum;
        unsigned short e_shentsize;
        unsigned short e_shnum;
        unsigned short e_shstrndx;
    } *elf = header;
    struct
    {
        unsigned int p_type;
        unsigned int p_offset;
        unsigned int p_vaddr;
        unsigned int p_paddr;
        unsigned int p_filesz;
        unsigned int p_memsz;
        unsigned int p_flags;
        unsigned int p_align;
    } *phdr;
    struct
    {
        unsigned int d_tag;
        unsigned int d_val;
    } *dyn;

    unsigned int i, size;

    if (header_size < sizeof(*elf)) return;
    if (elf->e_ident[6] != 1 /* EV_CURRENT */) return;

    size = elf->e_phnum * elf->e_phentsize;
    phdr = xmalloc( size );
    lseek( fd, elf->e_phoff, SEEK_SET );
    if (read( fd, phdr, size ) != size) return;

    for (i = 0; i < elf->e_phnum; i++)
    {
        if (phdr->p_type == 2 /* PT_DYNAMIC */ ) break;
        phdr = (void *)((char *)phdr + elf->e_phentsize);
    }
    if (i == elf->e_phnum) return;

    dyn = xmalloc( phdr->p_filesz );
    lseek( fd, phdr->p_offset, SEEK_SET );
    if (read( fd, dyn, phdr->p_filesz ) != phdr->p_filesz) return;
    for (i = 0; i < phdr->p_filesz / sizeof(*dyn) && dyn[i].d_tag; i++)
    {
        switch (dyn[i].d_tag)
        {
        case 25: dyn[i].d_tag = 0x60009994; break;  /* DT_INIT_ARRAY */
        case 27: dyn[i].d_tag = 0x60009995; break;  /* DT_INIT_ARRAYSZ */
        case 12: dyn[i].d_tag = 0x60009996; break;  /* DT_INIT */
        }
    }
    lseek( fd, phdr->p_offset, SEEK_SET );
    write( fd, dyn, phdr->p_filesz );
}

static void fixup_elf64( const char *name, int fd, void *header, size_t header_size )
{
    struct
    {
        unsigned char    e_ident[16];
        unsigned short   e_type;
        unsigned short   e_machine;
        unsigned int     e_version;
        unsigned __int64 e_entry;
        unsigned __int64 e_phoff;
        unsigned __int64 e_shoff;
        unsigned int     e_flags;
        unsigned short   e_ehsize;
        unsigned short   e_phentsize;
        unsigned short   e_phnum;
        unsigned short   e_shentsize;
        unsigned short   e_shnum;
        unsigned short   e_shstrndx;
    } *elf = header;
    struct
    {
        unsigned int     p_type;
        unsigned int     p_flags;
        unsigned __int64 p_offset;
        unsigned __int64 p_vaddr;
        unsigned __int64 p_paddr;
        unsigned __int64 p_filesz;
        unsigned __int64 p_memsz;
        unsigned __int64 p_align;
    } *phdr;
    struct
    {
        unsigned __int64 d_tag;
        unsigned __int64 d_val;
    } *dyn;

    unsigned int i, size;

    if (header_size < sizeof(*elf)) return;
    if (elf->e_ident[6] != 1 /* EV_CURRENT */) return;

    size = elf->e_phnum * elf->e_phentsize;
    phdr = xmalloc( size );
    lseek( fd, elf->e_phoff, SEEK_SET );
    if (read( fd, phdr, size ) != size) return;

    for (i = 0; i < elf->e_phnum; i++)
    {
        if (phdr->p_type == 2 /* PT_DYNAMIC */ ) break;
        phdr = (void *)((char *)phdr + elf->e_phentsize);
    }
    if (i == elf->e_phnum) return;

    dyn = xmalloc( phdr->p_filesz );
    lseek( fd, phdr->p_offset, SEEK_SET );
    if (read( fd, dyn, phdr->p_filesz ) != phdr->p_filesz) return;
    for (i = 0; i < phdr->p_filesz / sizeof(*dyn) && dyn[i].d_tag; i++)
    {
        switch (dyn[i].d_tag)
        {
        case 25: dyn[i].d_tag = 0x60009994; break;  /* DT_INIT_ARRAY */
        case 27: dyn[i].d_tag = 0x60009995; break;  /* DT_INIT_ARRAYSZ */
        case 12: dyn[i].d_tag = 0x60009996; break;  /* DT_INIT */
        }
    }
    lseek( fd, phdr->p_offset, SEEK_SET );
    write( fd, dyn, phdr->p_filesz );
}

/*******************************************************************
 *         fixup_constructors
 */
void fixup_constructors( struct strarray files )
{
    int i, fd, size;
    unsigned int header[64];

    for (i = 0; i < files.count; i++)
    {
        if ((fd = open( files.str[i], O_RDWR | O_BINARY )) == -1)
            fatal_perror( "Cannot open %s", files.str[i] );
        size = read( fd, &header, sizeof(header) );
        if (size > 5)
        {
            if (!memcmp( header, "\177ELF\001", 5 )) fixup_elf32( files.str[i], fd, header, size );
            else if (!memcmp( header, "\177ELF\002", 5 )) fixup_elf64( files.str[i], fd, header, size );
        }
        close( fd );
    }
}
