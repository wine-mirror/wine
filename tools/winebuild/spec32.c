/*
 * 32-bit spec files
 *
 * Copyright 1993 Robert J. Amstadt
 * Copyright 1995 Martin von Loewis
 * Copyright 1995, 1996, 1997 Alexandre Julliard
 * Copyright 1997 Eric Youngdale
 * Copyright 1999 Ulrich Weigand
 */

#include <assert.h>
#include <ctype.h>
#include <unistd.h>

#include "config.h"
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
static const char *make_internal_name( const ORDDEF *odp, const char *prefix )
{
    static char buffer[256];
    if (odp->name[0]) sprintf( buffer, "__wine_%s_%s_%s", prefix, DLLName, odp->name );
    else sprintf( buffer, "__wine_%s_%s_%d", prefix, DLLName, odp->ordinal );
    return buffer;
}

/*******************************************************************
 *         AssignOrdinals
 *
 * Assign ordinals to all entry points.
 */
static void AssignOrdinals(void)
{
    int i, ordinal;

    if ( !nb_names ) return;

    /* start assigning from Base, or from 1 if no ordinal defined yet */
    if (Base == MAX_ORDINALS) Base = 1;
    for (i = 0, ordinal = Base; i < nb_names; i++)
    {
        if (Names[i]->ordinal != -1) continue;  /* already has an ordinal */
        while (Ordinals[ordinal]) ordinal++;
        if (ordinal >= MAX_ORDINALS)
        {
            current_line = Names[i]->lineno;
            fatal_error( "Too many functions defined (max %d)\n", MAX_ORDINALS );
        }
        Names[i]->ordinal = ordinal;
        Ordinals[ordinal] = Names[i];
    }
    if (ordinal > Limit) Limit = ordinal;
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
static int output_exports( FILE *outfile, int nr_exports )
{
    int i, fwd_size = 0, total_size = 0;
    char *p;
    ORDDEF *odp;

    if (!nr_exports) return 0;

    fprintf( outfile, "asm(\".data\\n\"\n" );
    fprintf( outfile, "    \"\\t.align 4\\n\"\n" );
    fprintf( outfile, "    \"" PREFIX "__wine_spec_exports:\\n\"\n" );

    /* export directory header */

    fprintf( outfile, "    \"\\t.long 0\\n\"\n" );                 /* Characteristics */
    fprintf( outfile, "    \"\\t.long 0\\n\"\n" );                 /* TimeDateStamp */
    fprintf( outfile, "    \"\\t.long 0\\n\"\n" );                 /* MajorVersion/MinorVersion */
    fprintf( outfile, "    \"\\t.long " PREFIX "dllname\\n\"\n" ); /* Name */
    fprintf( outfile, "    \"\\t.long %d\\n\"\n", Base );          /* Base */
    fprintf( outfile, "    \"\\t.long %d\\n\"\n", nr_exports );    /* NumberOfFunctions */
    fprintf( outfile, "    \"\\t.long %d\\n\"\n", nb_names );      /* NumberOfNames */
    fprintf( outfile, "    \"\\t.long __wine_spec_exports_funcs\\n\"\n" ); /* AddressOfFunctions */
    if (nb_names)
    {
        fprintf( outfile, "    \"\\t.long __wine_spec_exp_name_ptrs\\n\"\n" );     /* AddressOfNames */
        fprintf( outfile, "    \"\\t.long __wine_spec_exp_ordinals\\n\"\n" );  /* AddressOfNameOrdinals */
    }
    else
    {
        fprintf( outfile, "    \"\\t.long 0\\n\"\n" );  /* AddressOfNames */
        fprintf( outfile, "    \"\\t.long 0\\n\"\n" );  /* AddressOfNameOrdinals */
    }
    total_size += 10 * sizeof(int);

    /* output the function pointers */

    fprintf( outfile, "    \"__wine_spec_exports_funcs:\\n\"\n" );
    for (i = Base; i <= Limit; i++)
    {
        ORDDEF *odp = Ordinals[i];
        if (!odp) fprintf( outfile, "    \"\\t.long 0\\n\"\n" );
        else switch(odp->type)
        {
        case TYPE_EXTERN:
            fprintf( outfile, "    \"\\t.long " PREFIX "%s\\n\"\n", odp->u.ext.link_name );
            break;
        case TYPE_STDCALL:
        case TYPE_VARARGS:
        case TYPE_CDECL:
            fprintf( outfile, "    \"\\t.long " PREFIX "%s\\n\"\n", odp->u.func.link_name);
            break;
        case TYPE_STUB:
            fprintf( outfile, "    \"\\t.long " PREFIX "%s\\n\"\n", make_internal_name( odp, "stub" ) );
            break;
        case TYPE_REGISTER:
            fprintf( outfile, "    \"\\t.long " PREFIX "%s\\n\"\n", make_internal_name( odp, "regs" ) );
            break;
        case TYPE_VARIABLE:
            fprintf( outfile, "    \"\\t.long " PREFIX "%s\\n\"\n", make_internal_name( odp, "var" ) );
            break;
        case TYPE_FORWARD:
            fprintf( outfile, "    \"\\t.long __wine_spec_forwards+%d\\n\"\n", fwd_size );
            fwd_size += strlen(odp->u.fwd.link_name) + 1;
            break;
        default:
            assert(0);
        }
    }
    total_size += (Limit - Base + 1) * sizeof(int);

    if (nb_names)
    {
        /* output the function name pointers */

        int namepos = 0;

        fprintf( outfile, "    \"__wine_spec_exp_name_ptrs:\\n\"\n" );
        for (i = 0; i < nb_names; i++)
        {
            fprintf( outfile, "    \"\\t.long __wine_spec_exp_names+%d\\n\"\n", namepos );
            namepos += strlen(Names[i]->name) + 1;
        }
        total_size += nb_names * sizeof(int);

        /* output the function names */

        fprintf( outfile, "    \"\\t.text\\n\"\n" );
        fprintf( outfile, "    \"__wine_spec_exp_names:\\n\"\n" );
        for (i = 0; i < nb_names; i++)
            fprintf( outfile, "    \"\\t" STRING " \\\"%s\\\"\\n\"\n", Names[i]->name );
        fprintf( outfile, "    \"\\t.data\\n\"\n" );

        /* output the function ordinals */

        fprintf( outfile, "    \"__wine_spec_exp_ordinals:\\n\"\n" );
        for (i = 0; i < nb_names; i++)
        {
            fprintf( outfile, "    \"\\t.short %d\\n\"\n", Names[i]->ordinal - Base );
        }
        total_size += nb_names * sizeof(short);
        if (nb_names % 2)
        {
            fprintf( outfile, "    \"\\t.short 0\\n\"\n" );
            total_size += sizeof(short);
        }
    }

    /* output forward strings */

    if (fwd_size)
    {
        fprintf( outfile, "    \"__wine_spec_forwards:\\n\"\n" );
        for (i = Base; i <= Limit; i++)
        {
            ORDDEF *odp = Ordinals[i];
            if (odp && odp->type == TYPE_FORWARD)
                fprintf( outfile, "    \"\\t" STRING " \\\"%s\\\"\\n\"\n", odp->u.fwd.link_name );
        }
        fprintf( outfile, "    \"\\t.align 4\\n\"\n" );
        total_size += (fwd_size + 3) & ~3;
    }

    /* output relays */

    if (debugging)
    {
        for (i = Base; i <= Limit; i++)
        {
            ORDDEF *odp = Ordinals[i];
            unsigned int j, mask = 0;

            /* skip non-existent entry points */
            if (!odp) goto ignore;
            /* skip non-functions */
            if ((odp->type != TYPE_STDCALL) &&
                (odp->type != TYPE_CDECL) &&
                (odp->type != TYPE_REGISTER)) goto ignore;
            /* skip norelay entry points */
            if (odp->flags & FLAG_NORELAY) goto ignore;

            for (j = 0; odp->u.func.arg_types[j]; j++)
            {
                if (odp->u.func.arg_types[j] == 't') mask |= 1<< (j*2);
                if (odp->u.func.arg_types[j] == 'W') mask |= 2<< (j*2);
            }
            if ((odp->flags & FLAG_RET64) && (j < 16)) mask |= 0x80000000;

            switch(odp->type)
            {
            case TYPE_STDCALL:
                fprintf( outfile, "    \"\\tjmp " PREFIX "%s\\n\"\n", odp->u.func.link_name );
                fprintf( outfile, "    \"\\tret $%d\\n\"\n",
                         strlen(odp->u.func.arg_types) * sizeof(int) );
                fprintf( outfile, "    \"\\t.long " PREFIX "%s,0x%08x\\n\"\n",
                         odp->u.func.link_name, mask );
                break;
            case TYPE_CDECL:
                fprintf( outfile, "    \"\\tjmp " PREFIX "%s\\n\"\n", odp->u.func.link_name );
                fprintf( outfile, "    \"\\tret\\n\"\n" );
                fprintf( outfile, "    \"\\t.short %d\\n\"\n",
                         strlen(odp->u.func.arg_types) * sizeof(int) );
                fprintf( outfile, "    \"\\t.long " PREFIX "%s,0x%08x\\n\"\n",
                         odp->u.func.link_name, mask );
                break;
            case TYPE_REGISTER:
                fprintf( outfile, "    \"\\tjmp " PREFIX "%s\\n\"\n",
                         make_internal_name( odp, "regs" ) );
                fprintf( outfile, "    \"\\tret\\n\"\n" );
                fprintf( outfile, "    \"\\t.short 0x%04x\\n\"\n",
                         0x8000 | strlen(odp->u.func.arg_types) * sizeof(int) );
                fprintf( outfile, "    \"\\t.long " PREFIX "%s,0x%08x\\n\"\n",
                         make_internal_name( odp, "regs" ), mask );
                break;
            default:
                assert(0);
            }
            continue;

        ignore:
            fprintf( outfile, "    \"\\t.long 0,0,0,0\\n\"\n" );
        }
    }


    /* output __wine_dllexport symbols */

    for (i = 0; i < nb_names; i++)
    {
        if (Names[i]->flags & FLAG_NOIMPORT) continue;
        /* check for invalid characters in the name */
        for (p = Names[i]->name; *p; p++)
            if (!isalnum(*p) && *p != '_' && *p != '.') goto next;
        fprintf( outfile, "    \"\\t.globl " PREFIX "__wine_dllexport_%s_%s\\n\"\n",
                 DLLName, Names[i]->name );
        fprintf( outfile, "    \"" PREFIX "__wine_dllexport_%s_%s:\\n\"\n",
                 DLLName, Names[i]->name );
    next:
    }
    fprintf( outfile, "    \"\\t.long 0xffffffff\\n\"\n" );

    /* output variables */

    for (i = 0, odp = EntryPoints; i < nb_entry_points; i++, odp++)
    {
        if (odp->type == TYPE_VARIABLE)
        {
            int j;
            fprintf( outfile, "    \"%s:\\n\"\n", make_internal_name( odp, "var" ) );
            fprintf( outfile, "    \"\\t.long " );
            for (j = 0; j < odp->u.var.n_values; j++)
            {
                fprintf( outfile, "0x%08x", odp->u.var.values[j] );
                if (j < odp->u.var.n_values-1) fputc( ',', outfile );
            }
            fprintf( outfile, "\\n\"\n" );
        }
    }

    fprintf( outfile, ");\n\n" );

    return total_size;
}


/*******************************************************************
 *         output_stub_funcs
 *
 * Output the functions for stub entry points
*/
static void output_stub_funcs( FILE *outfile )
{
    int i;
    ORDDEF *odp;

    for (i = 0, odp = EntryPoints; i < nb_entry_points; i++, odp++)
    {
        if (odp->type != TYPE_STUB) continue;
        fprintf( outfile, "#ifdef __GNUC__\n" );
        fprintf( outfile, "static void __wine_unimplemented( const char *func ) __attribute__((noreturn));\n" );
        fprintf( outfile, "#endif\n\n" );
        fprintf( outfile, "struct exc_record {\n" );
        fprintf( outfile, "  unsigned int code, flags;\n" );
        fprintf( outfile, "  void *rec, *addr;\n" );
        fprintf( outfile, "  unsigned int params;\n" );
        fprintf( outfile, "  const void *info[15];\n" );
        fprintf( outfile, "};\n\n" );
        fprintf( outfile, "extern void __stdcall RtlRaiseException( struct exc_record * );\n\n" );
        fprintf( outfile, "static void __wine_unimplemented( const char *func )\n{\n" );
        fprintf( outfile, "  struct exc_record rec;\n" );
        fprintf( outfile, "  rec.code    = 0x%08x;\n", EXCEPTION_WINE_STUB );
        fprintf( outfile, "  rec.flags   = %d;\n", EH_NONCONTINUABLE );
        fprintf( outfile, "  rec.rec     = 0;\n" );
        fprintf( outfile, "  rec.params  = 2;\n" );
        fprintf( outfile, "  rec.info[0] = dllname;\n" );
        fprintf( outfile, "  rec.info[1] = func;\n" );
        fprintf( outfile, "#ifdef __GNUC__\n" );
        fprintf( outfile, "  rec.addr = __builtin_return_address(1);\n" );
        fprintf( outfile, "#else\n" );
        fprintf( outfile, "  rec.addr = 0;\n" );
        fprintf( outfile, "#endif\n" );
        fprintf( outfile, "  for (;;) RtlRaiseException( &rec );\n}\n\n" );
        break;
    }

    for (i = 0, odp = EntryPoints; i < nb_entry_points; i++, odp++)
    {
        if (odp->type != TYPE_STUB) continue;
        fprintf( outfile, "void %s(void) ", make_internal_name( odp, "stub" ) );
        if (odp->name[0])
            fprintf( outfile, "{ __wine_unimplemented(\"%s\"); }\n", odp->name );
        else
            fprintf( outfile, "{ __wine_unimplemented(\"%d\"); }\n", odp->ordinal );
    }
}


/*******************************************************************
 *         output_register_funcs
 *
 * Output the functions for register entry points
 */
static void output_register_funcs( FILE *outfile )
{
    ORDDEF *odp;
    const char *name;
    int i;

    for (i = 0, odp = EntryPoints; i < nb_entry_points; i++, odp++)
    {
        if (odp->type != TYPE_REGISTER) continue;
        name = make_internal_name( odp, "regs" );
        fprintf( outfile,
                 "asm(\".align 4\\n\\t\"\n"
                 "    \"" __ASM_FUNC("%s") "\\n\\t\"\n"
                 "    \"" PREFIX "%s:\\n\\t\"\n"
                 "    \"call " PREFIX "CALL32_Regs\\n\\t\"\n"
                 "    \".long " PREFIX "%s\\n\\t\"\n"
                 "    \".byte %d,%d\");\n",
                 name, name, odp->u.func.link_name,
                 4 * strlen(odp->u.func.arg_types), 4 * strlen(odp->u.func.arg_types) );
    }
}


/*******************************************************************
 *         BuildSpec32File
 *
 * Build a Win32 C file from a spec file.
 */
void BuildSpec32File( FILE *outfile )
{
    int exports_size = 0;
    int nr_exports, nr_imports, nr_resources, nr_debug;
    int characteristics, subsystem;
    const char *init_func;
    DWORD page_size;

#ifdef HAVE_GETPAGESIZE
    page_size = getpagesize();
#else
# ifdef __svr4__
    page_size = sysconf(_SC_PAGESIZE);
# else
#   error Cannot get the page size on this platform
# endif
#endif

    AssignOrdinals();
    nr_exports = Base <= Limit ? Limit - Base + 1 : 0;

    resolve_imports( outfile );

    fprintf( outfile, "/* File generated automatically from %s; do not edit! */\n\n",
             input_file_name );

    /* Reserve some space for the PE header */

    fprintf( outfile, "extern char pe_header[];\n" );
    fprintf( outfile, "asm(\".section .text\\n\\t\"\n" );
    fprintf( outfile, "    \".align %ld\\n\"\n", page_size );
    fprintf( outfile, "    \"pe_header:\\t.fill %ld,1,0\\n\\t\");\n", page_size );

    fprintf( outfile, "static const char dllname[] = \"%s\";\n\n", DLLName );
    fprintf( outfile, "extern int __wine_spec_exports[];\n\n" );

#ifdef __i386__
    fprintf( outfile, "#define __stdcall __attribute__((__stdcall__))\n\n" );
#else
    fprintf( outfile, "#define __stdcall\n\n" );
#endif

    if (nr_exports)
    {
        /* Output the stub functions */

        output_stub_funcs( outfile );

        fprintf( outfile, "#ifndef __GNUC__\n" );
        fprintf( outfile, "static void __asm__dummy(void) {\n" );
        fprintf( outfile, "#endif /* !defined(__GNUC__) */\n" );

        /* Output code for all register functions */

        output_register_funcs( outfile );

        /* Output the exports and relay entry points */

        exports_size = output_exports( outfile, nr_exports );

        fprintf( outfile, "#ifndef __GNUC__\n" );
        fprintf( outfile, "}\n" );
        fprintf( outfile, "#endif /* !defined(__GNUC__) */\n" );
    }

    /* Output the DLL imports */

    nr_imports = output_imports( outfile );

    /* Output the resources */

    nr_resources = output_resources( outfile );

    /* Output the debug channels */

    nr_debug = output_debug( outfile );

    /* Output LibMain function */

    init_func = DLLInitFunc[0] ? DLLInitFunc : NULL;
    characteristics = subsystem = 0;
    switch(SpecMode)
    {
    case SPEC_MODE_DLL:
        if (init_func) fprintf( outfile, "extern void %s();\n", init_func );
        characteristics = IMAGE_FILE_DLL;
        break;
    case SPEC_MODE_GUIEXE:
        if (!init_func) init_func = "WinMain";
        fprintf( outfile,
                 "\n#include <winbase.h>\n"
                 "int _ARGC;\n"
                 "char **_ARGV;\n"
                 "extern int __stdcall %s(HINSTANCE,HINSTANCE,LPSTR,INT);\n"
                 "static void __wine_exe_main(void)\n"
                 "{\n"
                 "    extern int __wine_get_main_args( char ***argv );\n"
                 "    STARTUPINFOA info;\n"
                 "    LPSTR cmdline = GetCommandLineA();\n"
                 "    while (*cmdline && *cmdline != ' ') cmdline++;\n"
                 "    if (*cmdline) cmdline++;\n"
                 "    GetStartupInfoA( &info );\n"
                 "    if (!(info.dwFlags & STARTF_USESHOWWINDOW)) info.wShowWindow = 1;\n"
                 "    _ARGC = __wine_get_main_args( &_ARGV );\n"
                 "    ExitProcess( %s( GetModuleHandleA(0), 0, cmdline, info.wShowWindow ) );\n"
                 "}\n\n", init_func, init_func );
        init_func = "__wine_exe_main";
        subsystem = IMAGE_SUBSYSTEM_WINDOWS_GUI;
        break;
    case SPEC_MODE_GUIEXE_UNICODE:
        if (!init_func) init_func = "WinMain";
        fprintf( outfile,
                 "\n#include <winbase.h>\n"
                 "int _ARGC;\n"
                 "WCHAR **_ARGV;\n"
                 "extern int __stdcall %s(HINSTANCE,HINSTANCE,LPSTR,INT);\n"
                 "static void __wine_exe_main(void)\n"
                 "{\n"
                 "    extern int __wine_get_wmain_args( WCHAR ***argv );\n"
                 "    STARTUPINFOA info;\n"
                 "    LPSTR cmdline = GetCommandLineA();\n"
                 "    while (*cmdline && *cmdline != ' ') cmdline++;\n"
                 "    if (*cmdline) cmdline++;\n"
                 "    GetStartupInfoA( &info );\n"
                 "    if (!(info.dwFlags & STARTF_USESHOWWINDOW)) info.wShowWindow = 1;\n"
                 "    _ARGC = __wine_get_wmain_args( &_ARGV );\n"
                 "    ExitProcess( %s( GetModuleHandleA(0), 0, cmdline, info.wShowWindow ) );\n"
                 "}\n\n", init_func, init_func );
        init_func = "__wine_exe_main";
        subsystem = IMAGE_SUBSYSTEM_WINDOWS_GUI;
        break;
    case SPEC_MODE_CUIEXE:
        if (!init_func) init_func = "main";
        fprintf( outfile,
                 "\nint _ARGC;\n"
                 "char **_ARGV;\n"
                 "extern void __stdcall ExitProcess(int);\n"
                 "static void __wine_exe_main(void)\n"
                 "{\n"
                 "    extern int %s( int argc, char *argv[] );\n"
                 "    extern int __wine_get_main_args( char ***argv );\n"
                 "    _ARGC = __wine_get_main_args( &_ARGV );\n"
                 "    ExitProcess( %s( _ARGC, _ARGV ) );\n"
                 "}\n\n", init_func, init_func );
        init_func = "__wine_exe_main";
        subsystem = IMAGE_SUBSYSTEM_WINDOWS_CUI;
        break;
    case SPEC_MODE_CUIEXE_UNICODE:
        if (!init_func) init_func = "wmain";
        fprintf( outfile,
                 "\ntypedef unsigned short WCHAR;\n"
                 "int _ARGC;\n"
                 "WCHAR **_ARGV;\n"
                 "extern void __stdcall ExitProcess(int);\n"
                 "static void __wine_exe_main(void)\n"
                 "{\n"
                 "    extern int %s( int argc, WCHAR *argv[] );\n"
                 "    extern int __wine_get_wmain_args( WCHAR ***argv );\n"
                 "    _ARGC = __wine_get_wmain_args( &_ARGV );\n"
                 "    ExitProcess( %s( _ARGC, _ARGV ) );\n"
                 "}\n\n", init_func, init_func );
        init_func = "__wine_exe_main";
        subsystem = IMAGE_SUBSYSTEM_WINDOWS_CUI;
        break;
    }

    /* Output the NT header */

    /* this is the IMAGE_NT_HEADERS structure, but we cannot include winnt.h here */
    fprintf( outfile, "static const struct image_nt_headers\n{\n" );
    fprintf( outfile, "  int Signature;\n" );
    fprintf( outfile, "  struct file_header {\n" );
    fprintf( outfile, "    short Machine;\n" );
    fprintf( outfile, "    short NumberOfSections;\n" );
    fprintf( outfile, "    int   TimeDateStamp;\n" );
    fprintf( outfile, "    void *PointerToSymbolTable;\n" );
    fprintf( outfile, "    int   NumberOfSymbols;\n" );
    fprintf( outfile, "    short SizeOfOptionalHeader;\n" );
    fprintf( outfile, "    short Characteristics;\n" );
    fprintf( outfile, "  } FileHeader;\n" );
    fprintf( outfile, "  struct opt_header {\n" );
    fprintf( outfile, "    short Magic;\n" );
    fprintf( outfile, "    char  MajorLinkerVersion, MinorLinkerVersion;\n" );
    fprintf( outfile, "    int   SizeOfCode;\n" );
    fprintf( outfile, "    int   SizeOfInitializedData;\n" );
    fprintf( outfile, "    int   SizeOfUninitializedData;\n" );
    fprintf( outfile, "    void *AddressOfEntryPoint;\n" );
    fprintf( outfile, "    void *BaseOfCode;\n" );
    fprintf( outfile, "    void *BaseOfData;\n" );
    fprintf( outfile, "    void *ImageBase;\n" );
    fprintf( outfile, "    int   SectionAlignment;\n" );
    fprintf( outfile, "    int   FileAlignment;\n" );
    fprintf( outfile, "    short MajorOperatingSystemVersion;\n" );
    fprintf( outfile, "    short MinorOperatingSystemVersion;\n" );
    fprintf( outfile, "    short MajorImageVersion;\n" );
    fprintf( outfile, "    short MinorImageVersion;\n" );
    fprintf( outfile, "    short MajorSubsystemVersion;\n" );
    fprintf( outfile, "    short MinorSubsystemVersion;\n" );
    fprintf( outfile, "    int   Win32VersionValue;\n" );
    fprintf( outfile, "    int   SizeOfImage;\n" );
    fprintf( outfile, "    int   SizeOfHeaders;\n" );
    fprintf( outfile, "    int   CheckSum;\n" );
    fprintf( outfile, "    short Subsystem;\n" );
    fprintf( outfile, "    short DllCharacteristics;\n" );
    fprintf( outfile, "    int   SizeOfStackReserve;\n" );
    fprintf( outfile, "    int   SizeOfStackCommit;\n" );
    fprintf( outfile, "    int   SizeOfHeapReserve;\n" );
    fprintf( outfile, "    int   SizeOfHeapCommit;\n" );
    fprintf( outfile, "    int   LoaderFlags;\n" );
    fprintf( outfile, "    int   NumberOfRvaAndSizes;\n" );
    fprintf( outfile, "    struct { const void *VirtualAddress; int Size; } DataDirectory[%d];\n",
             IMAGE_NUMBEROF_DIRECTORY_ENTRIES );
    fprintf( outfile, "  } OptionalHeader;\n" );
    fprintf( outfile, "} nt_header = {\n" );
    fprintf( outfile, "  0x%04x,\n", IMAGE_NT_SIGNATURE );   /* Signature */

    fprintf( outfile, "  { 0x%04x,\n", IMAGE_FILE_MACHINE_I386 );  /* Machine */
    fprintf( outfile, "    0, 0, 0, 0,\n" );
    fprintf( outfile, "    sizeof(nt_header.OptionalHeader),\n" ); /* SizeOfOptionalHeader */
    fprintf( outfile, "    0x%04x },\n", characteristics );        /* Characteristics */

    fprintf( outfile, "  { 0x%04x,\n", IMAGE_NT_OPTIONAL_HDR_MAGIC );  /* Magic */
    fprintf( outfile, "    0, 0,\n" );                   /* Major/MinorLinkerVersion */
    fprintf( outfile, "    0, 0, 0,\n" );                /* SizeOfCode/Data */
    fprintf( outfile, "    %s,\n", init_func ? init_func : "0" );  /* AddressOfEntryPoint */
    fprintf( outfile, "    0, 0,\n" );                   /* BaseOfCode/Data */
    fprintf( outfile, "    pe_header,\n" );              /* ImageBase */
    fprintf( outfile, "    %ld,\n", page_size );         /* SectionAlignment */
    fprintf( outfile, "    %ld,\n", page_size );         /* FileAlignment */
    fprintf( outfile, "    1, 0,\n" );                   /* Major/MinorOperatingSystemVersion */
    fprintf( outfile, "    0, 0,\n" );                   /* Major/MinorImageVersion */
    fprintf( outfile, "    4, 0,\n" );                   /* Major/MinorSubsystemVersion */
    fprintf( outfile, "    0,\n" );                      /* Win32VersionValue */
    fprintf( outfile, "    %ld,\n", page_size );         /* SizeOfImage */
    fprintf( outfile, "    %ld,\n", page_size );         /* SizeOfHeaders */
    fprintf( outfile, "    0,\n" );                      /* CheckSum */
    fprintf( outfile, "    0x%04x,\n", subsystem );      /* Subsystem */
    fprintf( outfile, "    0, 0, 0, 0, 0, 0,\n" );
    fprintf( outfile, "    %d,\n", IMAGE_NUMBEROF_DIRECTORY_ENTRIES );  /* NumberOfRvaAndSizes */
    fprintf( outfile, "    {\n" );
    fprintf( outfile, "      { %s, %d },\n",  /* IMAGE_DIRECTORY_ENTRY_EXPORT */
             exports_size ? "__wine_spec_exports" : "0", exports_size );
    fprintf( outfile, "      { %s, %s },\n",  /* IMAGE_DIRECTORY_ENTRY_IMPORT */
             nr_imports ? "&imports" : "0", nr_imports ? "sizeof(imports)" : "0" );
    fprintf( outfile, "      { %s, %s },\n",   /* IMAGE_DIRECTORY_ENTRY_RESOURCE */
             nr_resources ? "&resources" : "0", nr_resources ? "sizeof(resources)" : "0" );
    fprintf( outfile, "    }\n  }\n};\n\n" );

    /* Output the DLL constructor */

    fprintf( outfile, "#ifndef __GNUC__\n" );
    fprintf( outfile, "static void __asm__dummy_dll_init(void) {\n" );
    fprintf( outfile, "#endif /* defined(__GNUC__) */\n" );
    fprintf( outfile, "asm(\"\\t.section\t.init ,\\\"ax\\\"\\n\"\n" );
    fprintf( outfile, "    \"\\tcall " PREFIX "__wine_spec_%s_init\\n\"\n", DLLName );
    fprintf( outfile, "    \"\\t.previous\\n\");\n" );
    if (nr_debug)
    {
        fprintf( outfile, "asm(\"\\t.section\t.fini ,\\\"ax\\\"\\n\"\n" );
        fprintf( outfile, "    \"\\tcall " PREFIX "__wine_spec_%s_fini\\n\"\n", DLLName );
        fprintf( outfile, "    \"\\t.previous\\n\");\n" );
    }
    fprintf( outfile, "#ifndef __GNUC__\n" );
    fprintf( outfile, "}\n" );
    fprintf( outfile, "#endif /* defined(__GNUC__) */\n\n" );

    fprintf( outfile,
             "void __wine_spec_%s_init(void)\n"
             "{\n"
             "    extern void __wine_dll_register( const struct image_nt_headers *, const char * );\n"
             "    extern void *__wine_dbg_register( char * const *, int );\n"
             "    __wine_dll_register( &nt_header, \"%s\" );\n",
             DLLName, DLLFileName );
    if (nr_debug)
        fprintf( outfile, "    debug_registration = __wine_dbg_register( debug_channels, %d );\n",
                 nr_debug );
    fprintf( outfile, "}\n" );
    if (nr_debug)
    {
        fprintf( outfile,
                 "\nvoid __wine_spec_%s_fini(void)\n"
                 "{\n"
                 "    extern void __wine_dbg_unregister( void* );\n"
                 "    __wine_dbg_unregister( debug_registration );\n"
                 "}\n", DLLName );
    }
}
