/*
 * 16-bit spec files
 *
 * Copyright 1993 Robert J. Amstadt
 * Copyright 1995 Martin von Loewis
 * Copyright 1995, 1996, 1997 Alexandre Julliard
 * Copyright 1997 Eric Youngdale
 * Copyright 1999 Ulrich Weigand
 */

#include <assert.h>
#include <ctype.h>

#include "builtin16.h"
#include "module.h"
#include "neexe.h"
#include "stackframe.h"

#include "build.h"


/*******************************************************************
 *         StoreVariableCode
 *
 * Store a list of ints into a byte array.
 */
static int StoreVariableCode( unsigned char *buffer, int size, ORDDEF *odp )
{
    int i;

    switch(size)
    {
    case 1:
        for (i = 0; i < odp->u.var.n_values; i++)
            buffer[i] = odp->u.var.values[i];
        break;
    case 2:
        for (i = 0; i < odp->u.var.n_values; i++)
            ((unsigned short *)buffer)[i] = odp->u.var.values[i];
        break;
    case 4:
        for (i = 0; i < odp->u.var.n_values; i++)
            ((unsigned int *)buffer)[i] = odp->u.var.values[i];
        break;
    }
    return odp->u.var.n_values * size;
}


/*******************************************************************
 *         BuildModule16
 *
 * Build the in-memory representation of a 16-bit NE module, and dump it
 * as a byte stream into the assembly code.
 */
static int BuildModule16( FILE *outfile, int max_code_offset,
                          int max_data_offset )
{
    int i;
    char *buffer;
    NE_MODULE *pModule;
    SEGTABLEENTRY *pSegment;
    OFSTRUCT *pFileInfo;
    BYTE *pstr;
    WORD *pword;
    ET_BUNDLE *bundle = 0;
    ET_ENTRY *entry = 0;

    /*   Module layout:
     * NE_MODULE       Module
     * OFSTRUCT        File information
     * SEGTABLEENTRY   Segment 1 (code)
     * SEGTABLEENTRY   Segment 2 (data)
     * WORD[2]         Resource table (empty)
     * BYTE[2]         Imported names (empty)
     * BYTE[n]         Resident names table
     * BYTE[n]         Entry table
     */

    buffer = xmalloc( 0x10000 );

    pModule = (NE_MODULE *)buffer;
    memset( pModule, 0, sizeof(*pModule) );
    pModule->magic = IMAGE_OS2_SIGNATURE;
    pModule->count = 1;
    pModule->next = 0;
    pModule->flags = NE_FFLAGS_SINGLEDATA | NE_FFLAGS_BUILTIN | NE_FFLAGS_LIBMODULE;
    pModule->dgroup = 2;
    pModule->heap_size = DLLHeapSize;
    pModule->stack_size = 0;
    pModule->ip = 0;
    pModule->cs = 0;
    pModule->sp = 0;
    pModule->ss = 0;
    pModule->seg_count = 2;
    pModule->modref_count = 0;
    pModule->nrname_size = 0;
    pModule->modref_table = 0;
    pModule->nrname_fpos = 0;
    pModule->moveable_entries = 0;
    pModule->alignment = 0;
    pModule->truetype = 0;
    pModule->os_flags = NE_OSFLAGS_WINDOWS;
    pModule->misc_flags = 0;
    pModule->dlls_to_init  = 0;
    pModule->nrname_handle = 0;
    pModule->min_swap_area = 0;
    pModule->expected_version = 0;
    pModule->module32 = 0;
    pModule->self = 0;
    pModule->self_loading_sel = 0;

      /* File information */

    pFileInfo = (OFSTRUCT *)(pModule + 1);
    pModule->fileinfo = (int)pFileInfo - (int)pModule;
    memset( pFileInfo, 0, sizeof(*pFileInfo) - sizeof(pFileInfo->szPathName) );
    pFileInfo->cBytes = sizeof(*pFileInfo) - sizeof(pFileInfo->szPathName)
                        + strlen(DLLFileName);
    strcpy( pFileInfo->szPathName, DLLFileName );
    pstr = (char *)pFileInfo + pFileInfo->cBytes + 1;
        
#ifdef __i386__  /* FIXME: Alignment problems! */

      /* Segment table */

    pSegment = (SEGTABLEENTRY *)pstr;
    pModule->seg_table = (int)pSegment - (int)pModule;
    pSegment->filepos = 0;
    pSegment->size = max_code_offset;
    pSegment->flags = 0;
    pSegment->minsize = max_code_offset;
    pSegment->hSeg = 0;
    pSegment++;

    pModule->dgroup_entry = (int)pSegment - (int)pModule;
    pSegment->filepos = 0;
    pSegment->size = max_data_offset;
    pSegment->flags = NE_SEGFLAGS_DATA;
    pSegment->minsize = max_data_offset;
    pSegment->hSeg = 0;
    pSegment++;

      /* Resource table */

    pword = (WORD *)pSegment;
    pModule->res_table = (int)pword - (int)pModule;
    *pword++ = 0;
    *pword++ = 0;

      /* Imported names table */

    pstr = (char *)pword;
    pModule->import_table = (int)pstr - (int)pModule;
    *pstr++ = 0;
    *pstr++ = 0;

      /* Resident names table */

    pModule->name_table = (int)pstr - (int)pModule;
    /* First entry is module name */
    *pstr = strlen(DLLName );
    strcpy( pstr + 1, DLLName );
    pstr += *pstr + 1;
    *(WORD *)pstr = 0;
    pstr += sizeof(WORD);
    /* Store all ordinals */
    for (i = 1; i <= Limit; i++)
    {
        ORDDEF *odp = Ordinals[i];
        if (!odp || !odp->name[0]) continue;
        *pstr = strlen( odp->name );
        strcpy( pstr + 1, odp->name );
        strupper( pstr + 1 );
        pstr += *pstr + 1;
        *(WORD *)pstr = i;
        pstr += sizeof(WORD);
    }
    *pstr++ = 0;

      /* Entry table */

    pModule->entry_table = (int)pstr - (int)pModule;
    for (i = 1; i <= Limit; i++)
    {
        int selector = 0;
        ORDDEF *odp = Ordinals[i];
        if (!odp) continue;

	switch (odp->type)
	{
        case TYPE_CDECL:
        case TYPE_PASCAL:
        case TYPE_PASCAL_16:
        case TYPE_REGISTER:
        case TYPE_INTERRUPT:
        case TYPE_STUB:
            selector = 1;  /* Code selector */
            break;

        case TYPE_BYTE:
        case TYPE_WORD:
        case TYPE_LONG:
            selector = 2;  /* Data selector */
            break;

        case TYPE_ABS:
            selector = 0xfe;  /* Constant selector */
            break;

        default:
            selector = 0;  /* Invalid selector */
            break;
        }

        if ( !selector )
           continue;

        if ( bundle && bundle->last+1 == i )
            bundle->last++;
        else
        {
            if ( bundle )
                bundle->next = (char *)pstr - (char *)pModule;

            bundle = (ET_BUNDLE *)pstr;
            bundle->first = i-1;
            bundle->last = i;
            bundle->next = 0;
            pstr += sizeof(ET_BUNDLE);
        }

	/* FIXME: is this really correct ?? */
	entry = (ET_ENTRY *)pstr;
	entry->type = 0xff;  /* movable */
	entry->flags = 3; /* exported & public data */
	entry->segnum = selector;
	entry->offs = odp->offset;
	pstr += sizeof(ET_ENTRY);
    }
    *pstr++ = 0;
#endif

      /* Dump the module content */

    dump_bytes( outfile, (char *)pModule, (int)pstr - (int)pModule, "Module" );
    return (int)pstr - (int)pModule;
}


/*******************************************************************
 *         BuildCallFrom16Func
 *
 * Build a 16-bit-to-Wine callback glue function. 
 *
 * The generated routines are intended to be used as argument conversion 
 * routines to be called by the CallFrom16... core. Thus, the prototypes of
 * the generated routines are (see also CallFrom16):
 *
 *  extern WORD WINAPI PREFIX_CallFrom16_C_word_xxx( FARPROC func, LPBYTE args );
 *  extern LONG WINAPI PREFIX_CallFrom16_C_long_xxx( FARPROC func, LPBYTE args );
 *  extern void WINAPI PREFIX_CallFrom16_C_regs_xxx( FARPROC func, LPBYTE args, 
 *                                                   CONTEXT86 *context );
 *  extern void WINAPI PREFIX_CallFrom16_C_intr_xxx( FARPROC func, LPBYTE args, 
 *                                                   CONTEXT86 *context );
 *
 * where 'C' is the calling convention ('p' for pascal or 'c' for cdecl), 
 * and each 'x' is an argument  ('w'=word, 's'=signed word, 'l'=long, 
 * 'p'=linear pointer, 't'=linear pointer to null-terminated string,
 * 'T'=segmented pointer to null-terminated string).
 *
 * The generated routines fetch the arguments from the 16-bit stack (pointed
 * to by 'args'); the offsets of the single argument values are computed 
 * according to the calling convention and the argument types.  Then, the
 * 32-bit entry point is called with these arguments.
 * 
 * For register functions, the arguments (if present) are converted just
 * the same as for normal functions, but in addition the CONTEXT86 pointer 
 * filled with the current register values is passed to the 32-bit routine.
 * (An 'intr' interrupt handler routine is treated exactly like a register 
 * routine, except that upon return, the flags word pushed onto the stack 
 * by the interrupt is removed by the 16-bit call stub.)
 *
 */
static void BuildCallFrom16Func( FILE *outfile, char *profile, char *prefix, int local )
{
    int i, pos, argsize = 0;
    int short_ret = 0;
    int reg_func = 0;
    int usecdecl = 0;
    char *args = profile + 7;
    char *ret_type;

    /* Parse function type */

    if (!strncmp( "c_", profile, 2 )) usecdecl = 1;
    else if (strncmp( "p_", profile, 2 ))
    {
        fprintf( stderr, "Invalid function name '%s', ignored\n", profile );
        return;
    }

    if (!strncmp( "word_", profile + 2, 5 )) short_ret = 1;
    else if (!strncmp( "regs_", profile + 2, 5 )) reg_func = 1;
    else if (!strncmp( "intr_", profile + 2, 5 )) reg_func = 2;
    else if (strncmp( "long_", profile + 2, 5 ))
    {
        fprintf( stderr, "Invalid function name '%s', ignored\n", profile );
        return;
    }

    for ( i = 0; args[i]; i++ )
        switch ( args[i] )
        {
        case 'w':  /* word */
        case 's':  /* s_word */
            argsize += 2;
            break;
        
        case 'l':  /* long or segmented pointer */
        case 'T':  /* segmented pointer to null-terminated string */
        case 'p':  /* linear pointer */
        case 't':  /* linear pointer to null-terminated string */
            argsize += 4;
            break;
        }

    ret_type = reg_func? "void" : short_ret? "WORD" : "LONG";

    fprintf( outfile, "typedef %s WINAPI (*proc_%s_t)( ", 
                      ret_type, profile );
    args = profile + 7;
    for ( i = 0; args[i]; i++ )
    {
        if ( i ) fprintf( outfile, ", " );
        switch (args[i])
        {
        case 'w':           fprintf( outfile, "WORD" ); break;
        case 's':           fprintf( outfile, "INT16" ); break;
        case 'l': case 'T': fprintf( outfile, "LONG" ); break;
        case 'p': case 't': fprintf( outfile, "LPVOID" ); break;
        }
    }
    if ( reg_func )
        fprintf( outfile, "%sstruct _CONTEXT86 *", i? ", " : "" );
    else if ( !i )
        fprintf( outfile, "void" );
    fprintf( outfile, " );\n" );
    
    fprintf( outfile, "%s%s WINAPI %s_CallFrom16_%s( FARPROC proc, LPBYTE args%s )\n{\n",
             local? "static " : "", ret_type, prefix, profile,
             reg_func? ", struct _CONTEXT86 *context" : "" );

    fprintf( outfile, "    %s((proc_%s_t) proc) (\n",
             reg_func? "" : "return ", profile );
    args = profile + 7;
    pos = !usecdecl? argsize : 0;
    for ( i = 0; args[i]; i++ )
    {
        if ( i ) fprintf( outfile, ",\n" );
        fprintf( outfile, "        " );
        switch (args[i])
        {
        case 'w':  /* word */
            if ( !usecdecl ) pos -= 2;
            fprintf( outfile, "*(WORD *)(args+%d)", pos );
            if (  usecdecl ) pos += 2;
            break;

        case 's':  /* s_word */
            if ( !usecdecl ) pos -= 2;
            fprintf( outfile, "*(INT16 *)(args+%d)", pos );
            if (  usecdecl ) pos += 2;
            break;

        case 'l':  /* long or segmented pointer */
        case 'T':  /* segmented pointer to null-terminated string */
            if ( !usecdecl ) pos -= 4;
            fprintf( outfile, "*(LONG *)(args+%d)", pos );
            if (  usecdecl ) pos += 4;
            break;

        case 'p':  /* linear pointer */
        case 't':  /* linear pointer to null-terminated string */
            if ( !usecdecl ) pos -= 4;
            fprintf( outfile, "PTR_SEG_TO_LIN( *(SEGPTR *)(args+%d) )", pos );
            if (  usecdecl ) pos += 4;
            break;

        default:
            fprintf( stderr, "Unknown arg type '%c'\n", args[i] );
        }
    }
    if ( reg_func )
        fprintf( outfile, "%s        context", i? ",\n" : "" );
    fprintf( outfile, " );\n}\n\n" );
}


/*******************************************************************
 *         BuildCallTo16Func
 *
 * Build a Wine-to-16-bit callback glue function. 
 *
 * Prototypes for the CallTo16 functions:
 *   extern WORD CALLBACK PREFIX_CallTo16_word_xxx( FARPROC16 func, args... );
 *   extern LONG CALLBACK PREFIX_CallTo16_long_xxx( FARPROC16 func, args... );
 * 
 * These routines are provided solely for convenience; they simply
 * write the arguments onto the 16-bit stack, and call the appropriate
 * CallTo16... core routine.
 *
 * If you have more sophisticated argument conversion requirements than
 * are provided by these routines, you might as well call the core 
 * routines by yourself.
 *
 */
static void BuildCallTo16Func( FILE *outfile, char *profile, char *prefix )
{
    char *args = profile + 5;
    int i, argsize = 0, short_ret = 0;

    if (!strncmp( "word_", profile, 5 )) short_ret = 1;
    else if (strncmp( "long_", profile, 5 ))
    {
        fprintf( stderr, "Invalid function name '%s'.\n", profile );
        exit(1);
    }

    fprintf( outfile, "%s %s_CallTo16_%s( FARPROC16 proc",
             short_ret? "WORD" : "LONG", prefix, profile );
    args = profile + 5;
    for ( i = 0; args[i]; i++ )
    {
        fprintf( outfile, ", " );
        switch (args[i])
        {
        case 'w': fprintf( outfile, "WORD" ); argsize += 2; break;
        case 'l': fprintf( outfile, "LONG" ); argsize += 4; break;
        }
        fprintf( outfile, " arg%d", i+1 );
    }
    fprintf( outfile, " )\n{\n" );

    if ( argsize > 0 )
        fprintf( outfile, "    LPBYTE args = (LPBYTE)CURRENT_STACK16;\n" );

    args = profile + 5;
    for ( i = 0; args[i]; i++ )
    {
        switch (args[i])
        {
        case 'w': fprintf( outfile, "    args -= sizeof(WORD); *(WORD" ); break;
        case 'l': fprintf( outfile, "    args -= sizeof(LONG); *(LONG" ); break;
        default:  fprintf( stderr, "Unexpected case '%c' in BuildCallTo16Func\n",
                                   args[i] );
        }
        fprintf( outfile, " *)args = arg%d;\n", i+1 );
    }

    fprintf( outfile, "    return CallTo16%s( proc, %d );\n}\n\n",
             short_ret? "Word" : "Long", argsize );
}


/*******************************************************************
 *         Spec16TypeCompare
 */
static int Spec16TypeCompare( const void *e1, const void *e2 )
{
    const ORDDEF *odp1 = *(const ORDDEF **)e1;
    const ORDDEF *odp2 = *(const ORDDEF **)e2;

    int type1 = (odp1->type == TYPE_CDECL) ? 0
              : (odp1->type == TYPE_REGISTER) ? 3
              : (odp1->type == TYPE_INTERRUPT) ? 4
              : (odp1->type == TYPE_PASCAL_16) ? 1 : 2;

    int type2 = (odp2->type == TYPE_CDECL) ? 0
              : (odp2->type == TYPE_REGISTER) ? 3
              : (odp2->type == TYPE_INTERRUPT) ? 4
              : (odp2->type == TYPE_PASCAL_16) ? 1 : 2;

    int retval = type1 - type2;
    if ( !retval )
        retval = strcmp( odp1->u.func.arg_types, odp2->u.func.arg_types );

    return retval;
}


/*******************************************************************
 *         BuildSpec16File
 *
 * Build a Win16 assembly file from a spec file.
 */
void BuildSpec16File( FILE *outfile )
{
    ORDDEF **type, **typelist;
    int i, nFuncs, nTypes;
    int code_offset, data_offset, module_size;
    unsigned char *data;

    /* File header */

    fprintf( outfile, "/* File generated automatically from %s; do not edit! */\n\n",
             input_file_name );
    fprintf( outfile, "#define __FLATCS__ 0x%04x\n", code_selector );
    fprintf( outfile, "#include \"builtin16.h\"\n\n" );

    data = (unsigned char *)xmalloc( 0x10000 );
    memset( data, 0, 16 );
    data_offset = 16;
    strupper( DLLName );

    /* Build sorted list of all argument types, without duplicates */

    typelist = (ORDDEF **)calloc( Limit+1, sizeof(ORDDEF *) );

    for (i = nFuncs = 0; i <= Limit; i++)
    {
        ORDDEF *odp = Ordinals[i];
        if (!odp) continue;
        switch (odp->type)
        {
          case TYPE_REGISTER:
          case TYPE_INTERRUPT:
          case TYPE_CDECL:
          case TYPE_PASCAL:
          case TYPE_PASCAL_16:
          case TYPE_STUB:
            typelist[nFuncs++] = odp;

          default:
            break;
        }
    }

    qsort( typelist, nFuncs, sizeof(ORDDEF *), Spec16TypeCompare );

    i = nTypes = 0;
    while ( i < nFuncs )
    {
        typelist[nTypes++] = typelist[i++];
        while ( i < nFuncs && Spec16TypeCompare( typelist + i, typelist + nTypes-1 ) == 0 )
            i++;
    }

    /* Output CallFrom16 routines needed by this .spec file */

    for ( i = 0; i < nTypes; i++ )
    {
        char profile[101];

        sprintf( profile, "%s_%s_%s",
                 (typelist[i]->type == TYPE_CDECL) ? "c" : "p",
                 (typelist[i]->type == TYPE_REGISTER) ? "regs" :
                 (typelist[i]->type == TYPE_INTERRUPT) ? "intr" :
                 (typelist[i]->type == TYPE_PASCAL_16) ? "word" : "long",
                 typelist[i]->u.func.arg_types );

        BuildCallFrom16Func( outfile, profile, DLLName, TRUE );
    }

    /* Output the DLL functions prototypes */

    for (i = 0; i <= Limit; i++)
    {
        ORDDEF *odp = Ordinals[i];
        if (!odp) continue;
        switch(odp->type)
        {
        case TYPE_REGISTER:
        case TYPE_INTERRUPT:
        case TYPE_CDECL:
        case TYPE_PASCAL:
        case TYPE_PASCAL_16:
            fprintf( outfile, "extern void %s();\n", odp->u.func.link_name );
            break;
        default:
            break;
        }
    }

    /* Output code segment */

    fprintf( outfile, "\nstatic struct\n{\n    CALLFROM16   call[%d];\n"
                      "    ENTRYPOINT16 entry[%d];\n} Code_Segment = \n{\n    {\n",
                      nTypes, nFuncs );
    code_offset = 0;

    for ( i = 0; i < nTypes; i++ )
    {
        char profile[101], *arg;
        int argsize = 0;

        sprintf( profile, "%s_%s_%s", 
                          (typelist[i]->type == TYPE_CDECL) ? "c" : "p",
                          (typelist[i]->type == TYPE_REGISTER) ? "regs" :
                          (typelist[i]->type == TYPE_INTERRUPT) ? "intr" :
                          (typelist[i]->type == TYPE_PASCAL_16) ? "word" : "long",
                          typelist[i]->u.func.arg_types );

        if ( typelist[i]->type != TYPE_CDECL )
            for ( arg = typelist[i]->u.func.arg_types; *arg; arg++ )
                switch ( *arg )
                {
                case 'w':  /* word */
                case 's':  /* s_word */
                    argsize += 2;
                    break;
        
                case 'l':  /* long or segmented pointer */
                case 'T':  /* segmented pointer to null-terminated string */
                case 'p':  /* linear pointer */
                case 't':  /* linear pointer to null-terminated string */
                    argsize += 4;
                    break;
                }

        if ( typelist[i]->type == TYPE_INTERRUPT )
            argsize += 2;

        fprintf( outfile, "        CF16_%s( %s_CallFrom16_%s, %d, \"%s\" ),\n",
                 (   typelist[i]->type == TYPE_REGISTER 
                  || typelist[i]->type == TYPE_INTERRUPT)? "REGS":
                 typelist[i]->type == TYPE_PASCAL_16? "WORD" : "LONG",
                 DLLName, profile, argsize, profile );

        code_offset += sizeof(CALLFROM16);
    }
    fprintf( outfile, "    },\n    {\n" );

    for (i = 0; i <= Limit; i++)
    {
        ORDDEF *odp = Ordinals[i];
        if (!odp) continue;
        switch (odp->type)
        {
          case TYPE_ABS:
            odp->offset = LOWORD(odp->u.abs.value);
            break;

          case TYPE_BYTE:
            odp->offset = data_offset;
            data_offset += StoreVariableCode( data + data_offset, 1, odp);
            break;

          case TYPE_WORD:
            odp->offset = data_offset;
            data_offset += StoreVariableCode( data + data_offset, 2, odp);
            break;

          case TYPE_LONG:
            odp->offset = data_offset;
            data_offset += StoreVariableCode( data + data_offset, 4, odp);
            break;

          case TYPE_REGISTER:
          case TYPE_INTERRUPT:
          case TYPE_CDECL:
          case TYPE_PASCAL:
          case TYPE_PASCAL_16:
          case TYPE_STUB:
            type = bsearch( &odp, typelist, nTypes, sizeof(ORDDEF *), Spec16TypeCompare );
            assert( type );

            fprintf( outfile, "        /* %s.%d */ ", DLLName, i );
            fprintf( outfile, "EP( %s, %d /* %s_%s_%s */ ),\n",
                              odp->u.func.link_name,
                              (type-typelist)*sizeof(CALLFROM16) - 
                              (code_offset + sizeof(ENTRYPOINT16)),
                              (odp->type == TYPE_CDECL) ? "c" : "p",
                              (odp->type == TYPE_REGISTER) ? "regs" :
                              (odp->type == TYPE_INTERRUPT) ? "intr" :
                              (odp->type == TYPE_PASCAL_16) ? "word" : "long",
                              odp->u.func.arg_types );
                                 
            odp->offset = code_offset;
            code_offset += sizeof(ENTRYPOINT16);
            break;
		
          default:
            fprintf(stderr,"build: function type %d not available for Win16\n",
                    odp->type);
            exit(1);
	}
    }

    fprintf( outfile, "    }\n};\n" );

    /* Output data segment */

    dump_bytes( outfile, data, data_offset, "Data_Segment" );

    /* Build the module */

    module_size = BuildModule16( outfile, code_offset, data_offset );

    /* Output the DLL descriptor */

    if (rsrc_name[0]) fprintf( outfile, "extern const char %s[];\n\n", rsrc_name );

    fprintf( outfile, "\nstatic const BUILTIN16_DESCRIPTOR descriptor = \n{\n" );
    fprintf( outfile, "    \"%s\",\n", DLLName );
    fprintf( outfile, "    Module,\n" );
    fprintf( outfile, "    sizeof(Module),\n" );
    fprintf( outfile, "    (BYTE *)&Code_Segment,\n" );
    fprintf( outfile, "    (BYTE *)Data_Segment,\n" );
    fprintf( outfile, "    \"%s\",\n", owner_name );
    fprintf( outfile, "    %s\n", rsrc_name[0] ? rsrc_name : "0" );
    fprintf( outfile, "};\n" );
    
    /* Output the DLL constructor */

    fprintf( outfile, "#ifdef __GNUC__\n" );
    fprintf( outfile, "static void %s_init(void) __attribute__((constructor));\n", DLLName );
    fprintf( outfile, "#else /* defined(__GNUC__) */\n" );
    fprintf( outfile, "static void __asm__dummy_dll_init(void) {\n" );
    fprintf( outfile, "asm(\"\\t.section\t.init ,\\\"ax\\\"\\n\"\n" );
    fprintf( outfile, "    \"\\tcall %s_init\\n\"\n", DLLName );
    fprintf( outfile, "    \"\\t.previous\\n\");\n" );
    fprintf( outfile, "}\n" );
    fprintf( outfile, "#endif /* defined(__GNUC__) */\n" );
    fprintf( outfile, "static void %s_init(void) { BUILTIN_RegisterDLL( &descriptor ); }\n",
             DLLName );
}


/*******************************************************************
 *         BuildGlue
 *
 * Build the 16-bit-to-Wine/Wine-to-16-bit callback glue code
 */
void BuildGlue( FILE *outfile, FILE *infile )
{
    char buffer[1024];

    /* File header */

    fprintf( outfile, "/* File generated automatically from %s; do not edit! */\n\n",
             input_file_name );
    fprintf( outfile, "#include \"builtin16.h\"\n" );
    fprintf( outfile, "#include \"stackframe.h\"\n\n" );

    /* Build the callback glue functions */

    while (fgets( buffer, sizeof(buffer), infile ))
    {
        if (strstr( buffer, "### start build ###" )) break;
    }
    while (fgets( buffer, sizeof(buffer), infile ))
    {
        char *p; 
        if ( (p = strstr( buffer, "CallFrom16_" )) != NULL )
        {
            char *q, *profile = p + strlen( "CallFrom16_" );
            for (q = profile; (*q == '_') || isalpha(*q); q++ )
                ;
            *q = '\0';
            for (q = p-1; q > buffer && ((*q == '_') || isalnum(*q)); q-- )
                ;
            if ( ++q < p ) p[-1] = '\0'; else q = "";
            BuildCallFrom16Func( outfile, profile, q, FALSE );
        }
        if ( (p = strstr( buffer, "CallTo16_" )) != NULL )
        {
            char *q, *profile = p + strlen( "CallTo16_" );
            for (q = profile; (*q == '_') || isalpha(*q); q++ )
                ;
            *q = '\0';
            for (q = p-1; q > buffer && ((*q == '_') || isalnum(*q)); q-- )
                ;
            if ( ++q < p ) p[-1] = '\0'; else q = "";
            BuildCallTo16Func( outfile, profile, q );
        }
        if (strstr( buffer, "### stop build ###" )) break;
    }

    fclose( infile );
}
