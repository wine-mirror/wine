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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"
#include "wine/port.h"

#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "wine/exception.h"
#include "build.h"


static int string_compare( const void *ptr1, const void *ptr2 )
{
    const char * const *str1 = ptr1;
    const char * const *str2 = ptr2;
    return strcmp( *str1, *str2 );
}


/*******************************************************************
 *         make_internal_name
 *
 * Generate an internal name for an entry point. Used for stubs etc.
 */
static const char *make_internal_name( const ORDDEF *odp, DLLSPEC *spec, const char *prefix )
{
    static char buffer[256];
    if (odp->name || odp->export_name)
    {
        char *p;
        sprintf( buffer, "__wine_%s_%s_%s", prefix, spec->file_name,
                 odp->name ? odp->name : odp->export_name );
        /* make sure name is a legal C identifier */
        for (p = buffer; *p; p++) if (!isalnum(*p) && *p != '_') break;
        if (!*p) return buffer;
    }
    sprintf( buffer, "__wine_%s_%s_%d", prefix, make_c_identifier(spec->file_name), odp->ordinal );
    return buffer;
}


/*******************************************************************
 *         output_debug
 *
 * Output the debug channels.
 */
static int output_debug( FILE *outfile )
{
    int i;

    if (!nb_debug_channels) return 0;
    qsort( debug_channels, nb_debug_channels, sizeof(debug_channels[0]), string_compare );

    for (i = 0; i < nb_debug_channels; i++)
        fprintf( outfile, "char __wine_dbch_%s[] = \"\\003%s\";\n",
                 debug_channels[i], debug_channels[i] );

    fprintf( outfile, "\nstatic char * const debug_channels[%d] =\n{\n", nb_debug_channels );
    for (i = 0; i < nb_debug_channels; i++)
    {
        fprintf( outfile, "    __wine_dbch_%s", debug_channels[i] );
        if (i < nb_debug_channels - 1) fprintf( outfile, ",\n" );
    }
    fprintf( outfile, "\n};\n\n" );
    fprintf( outfile, "static void *debug_registration;\n\n" );

    return nb_debug_channels;
}


/*******************************************************************
 *         output_exports
 *
 * Output the export table for a Win32 module.
 */
static void output_exports( FILE *outfile, DLLSPEC *spec )
{
    int i, fwd_size = 0;
    int nr_exports = spec->base <= spec->limit ? spec->limit - spec->base + 1 : 0;

    if (!nr_exports) return;

    fprintf( outfile, "/* export table */\n" );
    fprintf( outfile, "asm(\".data\\n\"\n" );
    fprintf( outfile, "    \"\\t.align %d\\n\"\n", get_alignment(4) );
    fprintf( outfile, "    \".L__wine_spec_exports:\\n\"\n" );

    /* export directory header */

    fprintf( outfile, "    \"\\t.long 0\\n\"\n" );                 /* Characteristics */
    fprintf( outfile, "    \"\\t.long 0\\n\"\n" );                 /* TimeDateStamp */
    fprintf( outfile, "    \"\\t.long 0\\n\"\n" );                 /* MajorVersion/MinorVersion */
    fprintf( outfile, "    \"\\t.long .L__wine_spec_exp_names\\n\"\n" ); /* Name */
    fprintf( outfile, "    \"\\t.long %d\\n\"\n", spec->base );        /* Base */
    fprintf( outfile, "    \"\\t.long %d\\n\"\n", nr_exports );        /* NumberOfFunctions */
    fprintf( outfile, "    \"\\t.long %d\\n\"\n", spec->nb_names );    /* NumberOfNames */
    fprintf( outfile, "    \"\\t.long .L__wine_spec_exports_funcs\\n\"\n" ); /* AddressOfFunctions */
    if (spec->nb_names)
    {
        fprintf( outfile, "    \"\\t.long .L__wine_spec_exp_name_ptrs\\n\"\n" );     /* AddressOfNames */
        fprintf( outfile, "    \"\\t.long .L__wine_spec_exp_ordinals\\n\"\n" );  /* AddressOfNameOrdinals */
    }
    else
    {
        fprintf( outfile, "    \"\\t.long 0\\n\"\n" );  /* AddressOfNames */
        fprintf( outfile, "    \"\\t.long 0\\n\"\n" );  /* AddressOfNameOrdinals */
    }

    /* output the function pointers */

    fprintf( outfile, "    \".L__wine_spec_exports_funcs:\\n\"\n" );
    for (i = spec->base; i <= spec->limit; i++)
    {
        ORDDEF *odp = spec->ordinals[i];
        if (!odp) fprintf( outfile, "    \"\\t.long 0\\n\"\n" );
        else switch(odp->type)
        {
        case TYPE_EXTERN:
        case TYPE_STDCALL:
        case TYPE_VARARGS:
        case TYPE_CDECL:
            if (!(odp->flags & FLAG_FORWARD))
            {
                fprintf( outfile, "    \"\\t.long %s\\n\"\n", asm_name(odp->link_name) );
            }
            else
            {
                fprintf( outfile, "    \"\\t.long .L__wine_spec_forwards+%d\\n\"\n", fwd_size );
                fwd_size += strlen(odp->link_name) + 1;
            }
            break;
        case TYPE_STUB:
            fprintf( outfile, "    \"\\t.long %s\\n\"\n",
                     asm_name( make_internal_name( odp, spec, "stub" )) );
            break;
        default:
            assert(0);
        }
    }

    if (spec->nb_names)
    {
        /* output the function name pointers */

        int namepos = strlen(spec->file_name) + 1;

        fprintf( outfile, "    \".L__wine_spec_exp_name_ptrs:\\n\"\n" );
        for (i = 0; i < spec->nb_names; i++)
        {
            fprintf( outfile, "    \"\\t.long .L__wine_spec_exp_names+%d\\n\"\n", namepos );
            namepos += strlen(spec->names[i]->name) + 1;
        }

        /* output the function ordinals */

        fprintf( outfile, "    \".L__wine_spec_exp_ordinals:\\n\"\n" );
        for (i = 0; i < spec->nb_names; i++)
        {
            fprintf( outfile, "    \"\\t%s %d\\n\"\n",
                     get_asm_short_keyword(), spec->names[i]->ordinal - spec->base );
        }
        if (spec->nb_names % 2)
        {
            fprintf( outfile, "    \"\\t%s 0\\n\"\n", get_asm_short_keyword() );
        }
    }

    /* output the export name strings */

    fprintf( outfile, "    \".L__wine_spec_exp_names:\\n\"\n" );
    fprintf( outfile, "    \"\\t%s \\\"%s\\\"\\n\"\n", get_asm_string_keyword(), spec->file_name );
    for (i = 0; i < spec->nb_names; i++)
        fprintf( outfile, "    \"\\t%s \\\"%s\\\"\\n\"\n",
                 get_asm_string_keyword(), spec->names[i]->name );

    /* output forward strings */

    if (fwd_size)
    {
        fprintf( outfile, "    \".L__wine_spec_forwards:\\n\"\n" );
        for (i = spec->base; i <= spec->limit; i++)
        {
            ORDDEF *odp = spec->ordinals[i];
            if (odp && (odp->flags & FLAG_FORWARD))
                fprintf( outfile, "    \"\\t%s \\\"%s\\\"\\n\"\n", get_asm_string_keyword(), odp->link_name );
        }
    }
    fprintf( outfile, "    \"\\t.align %d\\n\"\n", get_alignment(4) );
    fprintf( outfile, "    \".L__wine_spec_exports_end:\\n\"\n" );

    /* output relays */

    /* we only support relay debugging on i386 */
    if (target_cpu == CPU_x86)
    {
        for (i = spec->base; i <= spec->limit; i++)
        {
            ORDDEF *odp = spec->ordinals[i];
            unsigned int j, args, mask = 0;

            /* skip nonexistent entry points */
            if (!odp) goto ignore;
            /* skip non-functions */
            if ((odp->type != TYPE_STDCALL) && (odp->type != TYPE_CDECL)) goto ignore;
            /* skip norelay and forward entry points */
            if (odp->flags & (FLAG_NORELAY|FLAG_FORWARD)) goto ignore;

            for (j = 0; odp->u.func.arg_types[j]; j++)
            {
                if (odp->u.func.arg_types[j] == 't') mask |= 1<< (j*2);
                if (odp->u.func.arg_types[j] == 'W') mask |= 2<< (j*2);
            }
            if ((odp->flags & FLAG_RET64) && (j < 16)) mask |= 0x80000000;

            args = strlen(odp->u.func.arg_types) * get_ptr_size();

            switch(odp->type)
            {
            case TYPE_STDCALL:
                fprintf( outfile, "    \"\\tjmp %s\\n\"\n", asm_name(odp->link_name) );
                fprintf( outfile, "    \"\\tret $%d\\n\"\n", args );
                fprintf( outfile, "    \"\\t.long %s,0x%08x\\n\"\n", asm_name(odp->link_name), mask );
                break;
            case TYPE_CDECL:
                fprintf( outfile, "    \"\\tjmp %s\\n\"\n", asm_name(odp->link_name) );
                fprintf( outfile, "    \"\\tret\\n\"\n" );
                fprintf( outfile, "    \"\\t%s %d\\n\"\n", get_asm_short_keyword(), args );
                fprintf( outfile, "    \"\\t.long %s,0x%08x\\n\"\n", asm_name(odp->link_name), mask );
                break;
            default:
                assert(0);
            }
            continue;

        ignore:
            fprintf( outfile, "    \"\\t.long 0,0,0,0\\n\"\n" );
        }
    }
    else fprintf( outfile, "    \"\\t.long 0\\n\"\n" );

    fprintf( outfile, ");\n" );
}


/*******************************************************************
 *         output_stubs
 *
 * Output the functions for stub entry points
*/
static void output_stubs( FILE *outfile, DLLSPEC *spec )
{
    const char *name, *exp_name;
    int i, pos;

    if (!has_stubs( spec )) return;

    fprintf( outfile, "asm(\".text\\n\"\n" );

    for (i = pos = 0; i < spec->nb_entry_points; i++)
    {
        ORDDEF *odp = &spec->entry_points[i];
        if (odp->type != TYPE_STUB) continue;

        name = make_internal_name( odp, spec, "stub" );
        exp_name = odp->name ? odp->name : odp->export_name;
        fprintf( outfile, "    \"\\t.align %d\\n\"\n", get_alignment(4) );
        fprintf( outfile, "    \"\\t%s\\n\"\n", func_declaration(name) );
        fprintf( outfile, "    \"%s:\\n\"\n", asm_name(name) );
        fprintf( outfile, "    \"\\tcall .L__wine_stub_getpc_%d\\n\"\n", i );
        fprintf( outfile, "    \".L__wine_stub_getpc_%d:\\n\"\n", i );
        fprintf( outfile, "    \"\\tpopl %%eax\\n\"\n" );
        if (exp_name)
        {
            fprintf( outfile, "    \"\\tleal .L__wine_stub_strings+%d-.L__wine_stub_getpc_%d(%%eax),%%ecx\\n\"\n",
                     pos, i );
            fprintf( outfile, "    \"\\tpushl %%ecx\\n\"\n" );
            pos += strlen(exp_name) + 1;
        }
        else
            fprintf( outfile, "    \"\\tpushl $%d\\n\"\n", odp->ordinal );
        fprintf( outfile, "    \"\\tleal %s-.L__wine_stub_getpc_%d(%%eax),%%ecx\\n\"\n",
                 asm_name("__wine_spec_file_name"), i );
        fprintf( outfile, "    \"\\tpushl %%ecx\\n\"\n" );
        fprintf( outfile, "    \"\\tcall %s\\n\"\n", asm_name("__wine_spec_unimplemented_stub") );
        fprintf( outfile, "    \"\\t%s\\n\"\n", func_size(name) );
    }

    if (pos)
    {
        fprintf( outfile, "    \"\\t%s\\n\"\n", get_asm_string_section() );
        fprintf( outfile, "    \".L__wine_stub_strings:\\n\"\n" );
        for (i = 0; i < spec->nb_entry_points; i++)
        {
            ORDDEF *odp = &spec->entry_points[i];
            if (odp->type != TYPE_STUB) continue;
            exp_name = odp->name ? odp->name : odp->export_name;
            if (exp_name)
                fprintf( outfile, "    \"\\t%s \\\"%s\\\"\\n\"\n", get_asm_string_keyword(), exp_name );
        }
    }

    fprintf( outfile, ");\n" );
}


/*******************************************************************
 *         output_dll_init
 *
 * Output code for calling a dll constructor and destructor.
 */
void output_dll_init( FILE *outfile, const char *constructor, const char *destructor )
{
    if (target_platform == PLATFORM_APPLE)
    {
        /* Mach-O doesn't have an init section */
        if (constructor)
        {
            fprintf( outfile, "asm(\"\\t.mod_init_func\\n\"\n" );
            fprintf( outfile, "    \"\\t.align 2\\n\"\n" );
            fprintf( outfile, "    \"\\t.long %s\\n\"\n", asm_name(constructor) );
            fprintf( outfile, "    \"\\t.text\\n\");\n" );
        }
        if (destructor)
        {
            fprintf( outfile, "asm(\"\\t.mod_term_func\\n\"\n" );
            fprintf( outfile, "    \"\\t.align 2\\n\"\n" );
            fprintf( outfile, "    \"\\t.long %s\\n\"\n", asm_name(destructor) );
            fprintf( outfile, "    \"\\t.text\\n\");\n" );
        }
    }
    else switch(target_cpu)
    {
    case CPU_x86:
    case CPU_x86_64:
        if (constructor)
        {
            fprintf( outfile, "asm(\"\\t.section\\t\\\".init\\\" ,\\\"ax\\\"\\n\"\n" );
            fprintf( outfile, "    \"\\tcall %s\\n\"\n", asm_name(constructor) );
            fprintf( outfile, "    \"\\t.section\\t\\\".text\\\"\\n\");\n" );
        }
        if (destructor)
        {
            fprintf( outfile, "asm(\"\\t.section\\t\\\".fini\\\" ,\\\"ax\\\"\\n\"\n" );
            fprintf( outfile, "    \"\\tcall %s\\n\"\n", asm_name(destructor) );
            fprintf( outfile, "    \"\\t.section\\t\\\".text\\\"\\n\");\n" );
        }
        break;
    case CPU_SPARC:
        if (constructor)
        {
            fprintf( outfile, "asm(\"\\t.section\\t\\\".init\\\" ,\\\"ax\\\"\\n\"\n" );
            fprintf( outfile, "    \"\\tcall %s\\n\"\n", asm_name(constructor) );
            fprintf( outfile, "    \"\\tnop\\n\"\n" );
            fprintf( outfile, "    \"\\t.section\\t\\\".text\\\"\\n\");\n" );
        }
        if (destructor)
        {
            fprintf( outfile, "asm(\"\\t.section\\t\\\".fini\\\" ,\\\"ax\\\"\\n\"\n" );
            fprintf( outfile, "    \"\\tcall %s\\n\"\n", asm_name(destructor) );
            fprintf( outfile, "    \"\\tnop\\n\"\n" );
            fprintf( outfile, "    \"\\t.section\\t\\\".text\\\"\\n\");\n" );
        }
        break;
    case CPU_ALPHA:
        if (constructor)
        {
            fprintf( outfile, "asm(\"\\t.section\\t\\\".init\\\" ,\\\"ax\\\"\\n\"\n" );
            fprintf( outfile, "    \"\\tjsr $26,%s\\n\"\n", asm_name(constructor) );
            fprintf( outfile, "    \"\\t.section\\t\\\".text\\\"\\n\");\n" );
        }
        if (destructor)
        {
            fprintf( outfile, "asm(\"\\t.section\\t\\\".fini\\\" ,\\\"ax\\\"\\n\"\n" );
            fprintf( outfile, "    \"\\tjsr $26,%s\\n\"\n", asm_name(destructor) );
            fprintf( outfile, "    \"\\t.section\\t\\\".text\\\"\\n\");\n" );
        }
        break;
    case CPU_POWERPC:
        if (constructor)
        {
            fprintf( outfile, "asm(\"\\t.section\\t\\\".init\\\" ,\\\"ax\\\"\\n\"\n" );
            fprintf( outfile, "    \"\\tbl %s\\n\"\n", asm_name(constructor) );
            fprintf( outfile, "    \"\\t.section\\t\\\".text\\\"\\n\");\n" );
        }
        if (destructor)
        {
            fprintf( outfile, "asm(\"\\t.section\\t\\\".fini\\\" ,\\\"ax\\\"\\n\"\n" );
            fprintf( outfile, "    \"\\tbl %s\\n\"\n", asm_name(destructor) );
            fprintf( outfile, "    \"\\t.section\\t\\\".text\\\"\\n\");\n" );
        }
        break;
    }
}


/*******************************************************************
 *         BuildSpec32File
 *
 * Build a Win32 C file from a spec file.
 */
void BuildSpec32File( FILE *outfile, DLLSPEC *spec )
{
    int machine = 0;
    unsigned int page_size = get_page_size();

    resolve_imports( spec );
    output_standard_file_header( outfile );

    /* Reserve some space for the PE header */

    fprintf( outfile, "#ifndef __GNUC__\n" );
    fprintf( outfile, "static void __asm__dummy(void) {\n" );
    fprintf( outfile, "#endif\n" );

    fprintf( outfile, "asm(\".text\\n\\t\"\n" );
    fprintf( outfile, "    \".align %d\\n\"\n", get_alignment(page_size) );
    fprintf( outfile, "    \"__wine_spec_pe_header:\\t\"\n" );
    if (target_platform == PLATFORM_APPLE)
        fprintf( outfile, "    \".space 65536\\n\\t\"\n" );
    else
        fprintf( outfile, "    \".skip 65536\\n\\t\"\n" );

    /* Output the NT header */

    fprintf( outfile, "    \"\\t.data\\n\\t\"\n" );
    fprintf( outfile, "    \"\\t.align %d\\n\"\n", get_alignment(get_ptr_size()) );
    fprintf( outfile, "    \"\\t.globl %s\\n\"\n", asm_name("__wine_spec_nt_header") );
    fprintf( outfile, "    \"%s:\\n\"\n", asm_name("__wine_spec_nt_header"));

    fprintf( outfile, "    \"\\t.long 0x%04x\\n\"\n", IMAGE_NT_SIGNATURE );    /* Signature */
    switch(target_cpu)
    {
    case CPU_x86:     machine = IMAGE_FILE_MACHINE_I386; break;
    case CPU_x86_64:  machine = IMAGE_FILE_MACHINE_AMD64; break;
    case CPU_POWERPC: machine = IMAGE_FILE_MACHINE_POWERPC; break;
    case CPU_ALPHA:   machine = IMAGE_FILE_MACHINE_ALPHA; break;
    case CPU_SPARC:   machine = IMAGE_FILE_MACHINE_UNKNOWN; break;
    }
    fprintf( outfile, "    \"\\t%s 0x%04x\\n\"\n",              /* Machine */
             get_asm_short_keyword(), machine );
    fprintf( outfile, "    \"\\t%s 0\\n\"\n",                   /* NumberOfSections */
             get_asm_short_keyword() );
    fprintf( outfile, "    \"\\t.long 0\\n\"\n" );              /* TimeDateStamp */
    fprintf( outfile, "    \"\\t.long 0\\n\"\n" );              /* PointerToSymbolTable */
    fprintf( outfile, "    \"\\t.long 0\\n\"\n" );              /* NumberOfSymbols */
    fprintf( outfile, "    \"\\t%s %d\\n\"\n",                  /* SizeOfOptionalHeader */
             get_asm_short_keyword(),
             get_ptr_size() == 8 ? IMAGE_SIZEOF_NT_OPTIONAL64_HEADER : IMAGE_SIZEOF_NT_OPTIONAL32_HEADER );
    fprintf( outfile, "    \"\\t%s 0x%04x\\n\"\n",              /* Characteristics */
             get_asm_short_keyword(), spec->characteristics );
    fprintf( outfile, "    \"\\t%s 0x%04x\\n\"\n",              /* Magic */
             get_asm_short_keyword(),
             get_ptr_size() == 8 ? IMAGE_NT_OPTIONAL_HDR64_MAGIC : IMAGE_NT_OPTIONAL_HDR32_MAGIC );
    fprintf( outfile, "    \"\\t.byte 0\\n\"\n" );              /* MajorLinkerVersion */
    fprintf( outfile, "    \"\\t.byte 0\\n\"\n" );              /* MinorLinkerVersion */
    fprintf( outfile, "    \"\\t.long 0\\n\"\n" );              /* SizeOfCode */
    fprintf( outfile, "    \"\\t.long 0\\n\"\n" );              /* SizeOfInitializedData */
    fprintf( outfile, "    \"\\t.long 0\\n\"\n" );              /* SizeOfUninitializedData */
    fprintf( outfile, "    \"\\t.long %s\\n\"\n",               /* AddressOfEntryPoint */
             asm_name(spec->init_func) );
    fprintf( outfile, "    \"\\t.long 0\\n\"\n" );              /* BaseOfCode */
    if (get_ptr_size() == 4)
        fprintf( outfile, "    \"\\t.long %s\\n\"\n", asm_name("__wine_spec_nt_header") ); /* BaseOfData */
    fprintf( outfile, "    \"\\t%s __wine_spec_pe_header\\n\"\n",         /* ImageBase */
             get_asm_ptr_keyword() );
    fprintf( outfile, "    \"\\t.long %u\\n\"\n", page_size );  /* SectionAlignment */
    fprintf( outfile, "    \"\\t.long %u\\n\"\n", page_size );  /* FileAlignment */
    fprintf( outfile, "    \"\\t%s 1,0\\n\"\n",                 /* Major/MinorOperatingSystemVersion */
             get_asm_short_keyword() );
    fprintf( outfile, "    \"\\t%s 0,0\\n\"\n",                 /* Major/MinorImageVersion */
             get_asm_short_keyword() );
    fprintf( outfile, "    \"\\t%s %u,%u\\n\"\n",               /* Major/MinorSubsystemVersion */
             get_asm_short_keyword(), spec->subsystem_major, spec->subsystem_minor );
    fprintf( outfile, "    \"\\t.long 0\\n\"\n" );              /* Win32VersionValue */
    fprintf( outfile, "    \"\\t.long %s\\n\"\n",               /* SizeOfImage */
             asm_name("_end") );
    fprintf( outfile, "    \"\\t.long %u\\n\"\n", page_size );  /* SizeOfHeaders */
    fprintf( outfile, "    \"\\t.long 0\\n\"\n" );              /* CheckSum */
    fprintf( outfile, "    \"\\t%s 0x%04x\\n\"\n",              /* Subsystem */
             get_asm_short_keyword(), spec->subsystem );
    fprintf( outfile, "    \"\\t%s 0\\n\"\n",                   /* DllCharacteristics */
             get_asm_short_keyword() );
    fprintf( outfile, "    \"\\t%s %u,%u\\n\"\n",               /* SizeOfStackReserve/Commit */
             get_asm_ptr_keyword(), (spec->stack_size ? spec->stack_size : 1024) * 1024, page_size );
    fprintf( outfile, "    \"\\t%s %u,%u\\n\"\n",               /* SizeOfHeapReserve/Commit */
             get_asm_ptr_keyword(), (spec->heap_size ? spec->heap_size : 1024) * 1024, page_size );
    fprintf( outfile, "    \"\\t.long 0\\n\"\n" );              /* LoaderFlags */
    fprintf( outfile, "    \"\\t.long 16\\n\"\n" );             /* NumberOfRvaAndSizes */

    if (spec->base <= spec->limit)   /* DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT] */
        fprintf( outfile, "    \"\\t.long .L__wine_spec_exports, .L__wine_spec_exports_end-.L__wine_spec_exports\\n\"\n" );
    else
        fprintf( outfile, "    \"\\t.long 0,0\\n\"\n" );

    if (has_imports())   /* DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT] */
        fprintf( outfile, "    \"\\t.long .L__wine_spec_imports, .L__wine_spec_imports_end-.L__wine_spec_imports\\n\"\n" );
    else
        fprintf( outfile, "    \"\\t.long 0,0\\n\"\n" );

    if (spec->nb_resources)   /* DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE] */
        fprintf( outfile, "    \"\\t.long .L__wine_spec_resources, .L__wine_spec_resources_end-.L__wine_spec_resources\\n\"\n" );
    else
        fprintf( outfile, "    \"\\t.long 0,0\\n\"\n" );

    fprintf( outfile, "    \"\\t.long 0,0\\n\"\n" );  /* DataDirectory[3] */
    fprintf( outfile, "    \"\\t.long 0,0\\n\"\n" );  /* DataDirectory[4] */
    fprintf( outfile, "    \"\\t.long 0,0\\n\"\n" );  /* DataDirectory[5] */
    fprintf( outfile, "    \"\\t.long 0,0\\n\"\n" );  /* DataDirectory[6] */
    fprintf( outfile, "    \"\\t.long 0,0\\n\"\n" );  /* DataDirectory[7] */
    fprintf( outfile, "    \"\\t.long 0,0\\n\"\n" );  /* DataDirectory[8] */
    fprintf( outfile, "    \"\\t.long 0,0\\n\"\n" );  /* DataDirectory[9] */
    fprintf( outfile, "    \"\\t.long 0,0\\n\"\n" );  /* DataDirectory[10] */
    fprintf( outfile, "    \"\\t.long 0,0\\n\"\n" );  /* DataDirectory[11] */
    fprintf( outfile, "    \"\\t.long 0,0\\n\"\n" );  /* DataDirectory[12] */
    fprintf( outfile, "    \"\\t.long 0,0\\n\"\n" );  /* DataDirectory[13] */
    fprintf( outfile, "    \"\\t.long 0,0\\n\"\n" );  /* DataDirectory[14] */
    fprintf( outfile, "    \"\\t.long 0,0\\n\"\n" );  /* DataDirectory[15] */
    fprintf( outfile, ");\n" );

    fprintf( outfile, "asm(\"%s\\n\"\n", get_asm_string_section() );
    fprintf( outfile, "    \"\\t.globl %s\\n\"\n", asm_name("__wine_spec_file_name") );
    fprintf( outfile, "    \"%s:\\n\"\n", asm_name("__wine_spec_file_name"));
    fprintf( outfile, "    \"\\t%s \\\"%s\\\"\\n\"\n", get_asm_string_keyword(), spec->file_name );
    if (target_platform == PLATFORM_APPLE)
        fprintf( outfile, "    \"\\t.comm %s,4\\n\"\n", asm_name("_end") );

    fprintf( outfile, ");\n" );

    output_stubs( outfile, spec );
    output_exports( outfile, spec );
    output_imports( outfile, spec );
    output_resources( outfile, spec );
    output_dll_init( outfile, "__wine_spec_init_ctor", NULL );

    fprintf( outfile, "#ifndef __GNUC__\n" );
    fprintf( outfile, "}\n" );
    fprintf( outfile, "#endif\n" );
}


/*******************************************************************
 *         BuildDef32File
 *
 * Build a Win32 def file from a spec file.
 */
void BuildDef32File( FILE *outfile, DLLSPEC *spec )
{
    const char *name;
    int i, total;

    if (spec_file_name)
        fprintf( outfile, "; File generated automatically from %s; do not edit!\n\n",
                 spec_file_name );
    else
        fprintf( outfile, "; File generated automatically; do not edit!\n\n" );

    fprintf(outfile, "LIBRARY %s\n\n", spec->file_name);

    fprintf(outfile, "EXPORTS\n");

    /* Output the exports and relay entry points */

    for (i = total = 0; i < spec->nb_entry_points; i++)
    {
        const ORDDEF *odp = &spec->entry_points[i];
        int is_data = 0;

        if (!odp) continue;

        if (odp->name) name = odp->name;
        else if (odp->export_name) name = odp->export_name;
        else continue;

        if (!(odp->flags & FLAG_PRIVATE)) total++;

        if (odp->type == TYPE_STUB) continue;

        fprintf(outfile, "  %s", name);

        switch(odp->type)
        {
        case TYPE_EXTERN:
            is_data = 1;
            /* fall through */
        case TYPE_VARARGS:
        case TYPE_CDECL:
            /* try to reduce output */
            if(strcmp(name, odp->link_name) || (odp->flags & FLAG_FORWARD))
                fprintf(outfile, "=%s", odp->link_name);
            break;
        case TYPE_STDCALL:
        {
            int at_param = strlen(odp->u.func.arg_types) * get_ptr_size();
            if (!kill_at) fprintf(outfile, "@%d", at_param);
            if  (odp->flags & FLAG_FORWARD)
            {
                fprintf(outfile, "=%s", odp->link_name);
            }
            else if (strcmp(name, odp->link_name)) /* try to reduce output */
            {
                fprintf(outfile, "=%s", odp->link_name);
                if (!kill_at) fprintf(outfile, "@%d", at_param);
            }
            break;
        }
        default:
            assert(0);
        }
        fprintf( outfile, " @%d", odp->ordinal );
        if (!odp->name) fprintf( outfile, " NONAME" );
        if (is_data) fprintf( outfile, " DATA" );
        if (odp->flags & FLAG_PRIVATE) fprintf( outfile, " PRIVATE" );
        fprintf( outfile, "\n" );
    }
    if (!total) warning( "%s: Import library doesn't export anything\n", spec->file_name );
}


/*******************************************************************
 *         BuildDebugFile
 *
 * Build the debugging channels source file.
 */
void BuildDebugFile( FILE *outfile, const char *srcdir, char **argv )
{
    int nr_debug;
    char *prefix, *p, *constructor, *destructor;

    while (*argv)
    {
        if (!parse_debug_channels( srcdir, *argv++ )) exit(1);
    }

    output_standard_file_header( outfile );
    nr_debug = output_debug( outfile );
    if (!nr_debug)
    {
        fprintf( outfile, "/* no debug channels found for this module */\n" );
        return;
    }

    if (output_file_name)
    {
        if ((p = strrchr( output_file_name, '/' ))) p++;
        prefix = xstrdup( p ? p : output_file_name );
        if ((p = strchr( prefix, '.' ))) *p = 0;
        strcpy( p, make_c_identifier(p) );
    }
    else prefix = xstrdup( "_" );

    /* Output the DLL constructor */

    constructor = xmalloc( strlen(prefix) + 17 );
    destructor = xmalloc( strlen(prefix) + 17 );
    sprintf( constructor, "__wine_dbg_%s_init", prefix );
    sprintf( destructor, "__wine_dbg_%s_fini", prefix );
    fprintf( outfile,
             "#ifdef __GNUC__\n"
             "void %s(void) __attribute__((constructor));\n"
             "void %s(void) __attribute__((destructor));\n"
             "#else\n"
             "static void __asm__dummy_dll_init(void) {\n",
             constructor, destructor );
    output_dll_init( outfile, constructor, destructor );
    fprintf( outfile, "}\n#endif /* defined(__GNUC__) */\n\n" );

    fprintf( outfile,
             "void %s(void)\n"
             "{\n"
             "    extern void *__wine_dbg_register( char * const *, int );\n"
             "    if (!debug_registration) debug_registration = __wine_dbg_register( debug_channels, %d );\n"
             "}\n\n", constructor, nr_debug );
    fprintf( outfile,
             "void %s(void)\n"
             "{\n"
             "    extern void __wine_dbg_unregister( void* );\n"
             "    __wine_dbg_unregister( debug_registration );\n"
             "}\n", destructor );

    free( constructor );
    free( destructor );
    free( prefix );
}
