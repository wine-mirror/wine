/*
 *  Code generation functions
 *
 *  Copyright 2000-2002 Jon Griffiths
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

#include "winedump.h"

/* Output files */
static FILE *specfile = NULL;
static FILE *hfile    = NULL;
static FILE *cfile    = NULL;

static void  output_spec_postamble (void);
static void  output_header_postamble (void);
static void  output_c_postamble (void);
static void  output_c_banner (const parsed_symbol *sym);
static const char *get_format_str (int type);
static const char *get_in_or_out (const parsed_symbol *sym, size_t arg);


/*******************************************************************
 *         output_spec_preamble
 *
 * Write the first part of the .spec file
 */
void  output_spec_preamble (void)
{
  specfile = open_file (OUTPUT_DLL_NAME, ".spec", "w");

  atexit (output_spec_postamble);

  if (VERBOSE)
    puts ("Creating .spec preamble");

  fprintf (specfile,
           "# Generated from %s by winedump\n\n", globals.input_name);
}


/*******************************************************************
 *         output_spec_symbol
 *
 * Write a symbol to the .spec file
 */
void  output_spec_symbol (const parsed_symbol *sym)
{
  char ord_spec[20];

  assert (specfile);
  assert (sym && sym->symbol);

  if (sym->ordinal >= 0)
    sprintf(ord_spec, "%d", sym->ordinal);
  else
  {
    ord_spec[0] = '@';
    ord_spec[1] = '\0';
  }
  if (sym->flags & SYM_THISCALL)
    strcat (ord_spec, " -i386"); /* For binary compatibility only */

  if (!globals.do_code || !sym->function_name)
  {
    if (sym->flags & SYM_DATA)
    {
      if (globals.forward_dll)
        fprintf (specfile, "%s forward %s %s.%s #", ord_spec, sym->symbol,
                 globals.forward_dll, sym->symbol);

      fprintf (specfile, "%s extern %s %s\n", ord_spec, sym->symbol,
               sym->arg_name[0]);
      return;
    }

    if (globals.forward_dll)
      fprintf (specfile, "%s forward %s %s.%s\n", ord_spec, sym->symbol,
               globals.forward_dll, sym->symbol);
    else
      fprintf (specfile, "%s stub %s\n", ord_spec, sym->symbol);
  }
  else
  {
    unsigned int i = sym->flags & SYM_THISCALL ? 1 : 0;

    fprintf (specfile, "%s %s %s(", ord_spec, sym->varargs ? "varargs" :
             symbol_get_call_convention(sym), sym->symbol);

    for (; i < sym->argc; i++)
      fprintf (specfile, " %s", symbol_get_spec_type(sym, i));

    if (sym->argc)
      fputc (' ', specfile);
    fputs (") ", specfile);

    if (sym->flags & SYM_THISCALL)
      fputs ("__thiscall_", specfile);
    fprintf (specfile, "%s_%s", OUTPUT_UC_DLL_NAME, sym->function_name);

    fputc ('\n',specfile);
  }
}


/*******************************************************************
 *         output_spec_postamble
 *
 * Write the last part of the .spec file
 */
static void output_spec_postamble (void)
{
  if (specfile)
    fclose (specfile);
  specfile = NULL;
}


/*******************************************************************
 *         output_header_preamble
 *
 * Write the first part of the .h file
 */
void  output_header_preamble (void)
{
  if (!globals.do_code)
      return;

  hfile = open_file (OUTPUT_DLL_NAME, "_dll.h", "w");

  atexit (output_header_postamble);

  fprintf (hfile,
           "/*\n * %s.dll\n *\n * Generated from %s.dll by winedump.\n *\n"
           " * DO NOT SEND GENERATED DLLS FOR INCLUSION INTO WINE !\n *\n */"
           "\n#ifndef __WINE_%s_DLL_H\n#define __WINE_%s_DLL_H\n\n"
           "#include \"windef.h\"\n#include \"wine/debug.h\"\n"
           "#include \"winbase.h\"\n#include \"winnt.h\"\n\n\n",
           OUTPUT_DLL_NAME, OUTPUT_DLL_NAME, OUTPUT_UC_DLL_NAME,
           OUTPUT_UC_DLL_NAME);
}


/*******************************************************************
 *         output_header_symbol
 *
 * Write a symbol to the .h file
 */
void  output_header_symbol (const parsed_symbol *sym)
{
  if (!globals.do_code)
      return;

  assert (hfile);
  assert (sym && sym->symbol);

  if (!globals.do_code)
    return;

  if (sym->flags & SYM_DATA)
    return;

  if (!sym->function_name)
    fprintf (hfile, "/* __%s %s_%s(); */\n", symbol_get_call_convention(sym),
             OUTPUT_UC_DLL_NAME, sym->symbol);
  else
  {
    output_prototype (hfile, sym);
    fputs (";\n", hfile);
  }
}


/*******************************************************************
 *         output_header_postamble
 *
 * Write the last part of the .h file
 */
static void output_header_postamble (void)
{
  if (hfile)
  {
    fprintf (hfile, "\n\n\n#endif\t/* __WINE_%s_DLL_H */\n",
             OUTPUT_UC_DLL_NAME);
    fclose (hfile);
    hfile = NULL;
  }
}


/*******************************************************************
 *         output_c_preamble
 *
 * Write the first part of the .c file
 */
void  output_c_preamble (void)
{
  cfile = open_file (OUTPUT_DLL_NAME, "_main.c", "w");

  atexit (output_c_postamble);

  fprintf (cfile,
           "/*\n * %s.dll\n *\n * Generated from %s by winedump.\n *\n"
           " * DO NOT SUBMIT GENERATED DLLS FOR INCLUSION INTO WINE!\n *\n */"
           "\n\n#include \"config.h\"\n\n#include <stdarg.h>\n\n"
           "#include \"windef.h\"\n#include \"winbase.h\"\n",
           OUTPUT_DLL_NAME, globals.input_name);

  if (globals.do_code)
    fprintf (cfile, "#include \"%s_dll.h\"\n", OUTPUT_DLL_NAME);

  fprintf (cfile,"#include \"wine/debug.h\"\n\nWINE_DEFAULT_DEBUG_CHANNEL(%s);\n\n",
           OUTPUT_DLL_NAME);

  if (globals.forward_dll)
  {
    if (VERBOSE)
      puts ("Creating a forwarding DLL");

    fputs ("\nHMODULE hDLL=0;    /* DLL to call */\n\n", cfile);

    fprintf (cfile,
           "BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, void *reserved)\n{\n"
           "    TRACE(\"(%%p, %%u, %%p)\\n\", instance, reason, reserved);\n\n"
           "    switch (reason)\n"
           "    {\n"
           "        case DLL_PROCESS_ATTACH:\n"
           "            hDLL = LoadLibraryA(\"%s\");\n"
           "            TRACE(\"Forwarding DLL (%s) loaded (%%p)\\n\", hDLL);\n"
           "            DisableThreadLibraryCalls(instance);\n"
           "            break;\n"
           "        case DLL_PROCESS_DETACH:\n"
           "            FreeLibrary(hDLL);\n"
           "            TRACE(\"Forwarding DLL (%s) freed\\n\");\n"
           "            break;\n"
           "    }\n\n"
           "    return TRUE;\n}\n",
           globals.forward_dll, globals.forward_dll, globals.forward_dll);
  }
}


/*******************************************************************
 *         output_c_symbol
 *
 * Write a symbol to the .c file
 */
void  output_c_symbol (const parsed_symbol *sym)
{
  unsigned int i, start = sym->flags & SYM_THISCALL ? 1 : 0;
  BOOL is_void;
  static BOOL has_thiscall = FALSE;

  assert (cfile);
  assert (sym && sym->symbol);

  if (!globals.do_code)
    return;

  if (sym->flags & SYM_DATA)
  {
    fprintf (cfile, "/* FIXME: Move to top of file */\n%s;\n\n",
             sym->arg_text[0]);
    return;
  }

  if (sym->flags & SYM_THISCALL && !has_thiscall)
  {
    fputs ("#ifdef __i386__  /* thiscall functions are i386-specific */\n\n"
           "#define THISCALL(func) __thiscall_ ## func\n"
           "#define THISCALL_NAME(func) __ASM_NAME(\"__thiscall_\" #func)\n"
           "#define DEFINE_THISCALL_WRAPPER(func) \\\n"
           "\textern void THISCALL(func)(); \\\n"
           "\t__ASM_GLOBAL_FUNC(__thiscall_ ## func, \\\n"
           "\t\t\t\"popl %eax\\n\\t\" \\\n"
           "\t\t\t\"pushl %ecx\\n\\t\" \\\n"
           "\t\t\t\"pushl %eax\\n\\t\" \\\n"
           "\t\t\t\"jmp \" __ASM_NAME(#func) )\n"
           "#else /* __i386__ */\n\n"
           "#define THISCALL(func) func\n"
           "#define THISCALL_NAME(func) __ASM_NAME(#func)\n"
           "#define DEFINE_THISCALL_WRAPPER(func) /* nothing */\n\n"
           "#endif /* __i386__ */\n\n", cfile);
    has_thiscall = TRUE;
  }

  output_c_banner(sym);

  if (sym->flags & SYM_THISCALL)
    fprintf(cfile, "DEFINE_THISCALL_WRAPPER(%s)\n", sym->function_name);

  if (!sym->function_name)
  {
    /* #ifdef'd dummy */
    fprintf (cfile, "#if 0\n__%s %s_%s()\n{\n\t/* %s in .spec */\n}\n#endif\n",
             symbol_get_call_convention(sym), OUTPUT_UC_DLL_NAME, sym->symbol,
             globals.forward_dll ? "@forward" : "@stub");
    return;
  }

  is_void = !strcasecmp (sym->return_text, "void");

  output_prototype (cfile, sym);
  fputs ("\n{\n", cfile);

  if (!globals.do_trace)
  {
    fputs ("\tFIXME(\":stub\\n\");\n", cfile);
    if (!is_void)
        fprintf (cfile, "\treturn (%s) 0;\n", sym->return_text);
    fputs ("}\n", cfile);
    return;
  }

  /* Tracing, maybe forwarding as well */
  if (globals.forward_dll)
  {
    /* Write variables for calling */
    if (sym->varargs)
      fputs("\tva_list valist;\n", cfile);

    fprintf (cfile, "\t%s (__%s *pFunc)(", sym->return_text,
             symbol_get_call_convention(sym));

    for (i = start; i < sym->argc; i++)
      fprintf (cfile, "%s%s", i > start ? ", " : "", sym->arg_text [i]);

    fprintf (cfile, "%s);\n", sym->varargs ? ",..." : sym->argc == 1 &&
             sym->flags & SYM_THISCALL ? "" : sym->argc ? "" : "void");

    if (!is_void)
      fprintf (cfile, "\t%s retVal;\n", sym->return_text);

    fprintf (cfile, "\tpFunc=(void*)GetProcAddress(hDLL,\"%s\");\n",
             sym->symbol);
  }

  /* TRACE input arguments */
  fprintf (cfile, "\tTRACE(\"(%s", !sym->argc ? "void" : "");

  for (i = 0; i < sym->argc; i++)
    fprintf (cfile, "%s(%s)%s", i ? "," : "", sym->arg_text [i],
             get_format_str (sym->arg_type [i]));

  fprintf (cfile, "%s): %s\\n\"", sym->varargs ? ",..." : "",
           globals.forward_dll ? "forward" : "stub");

  for (i = 0; i < sym->argc; i++)
    if (sym->arg_type[i] != ARG_STRUCT)
      fprintf(cfile, ",%s%s%s%s", sym->arg_type[i] == ARG_LONG ? "(LONG)" : "",
              sym->arg_type[i] == ARG_WIDE_STRING ? "debugstr_w(" : "",
              sym->arg_name[i],
              sym->arg_type[i] == ARG_WIDE_STRING ? ")" : "");

  fputs (");\n", cfile);

  if (!globals.forward_dll)
  {
    if (!is_void)
      fprintf (cfile, "\treturn (%s) 0;\n", sym->return_text);
    fputs ("}\n", cfile);
    return;
  }

  /* Call the DLL */
  if (sym->varargs)
    fprintf (cfile, "\tva_start(valist,%s);\n", sym->arg_name[sym->argc-1]);

  fprintf (cfile, "\t%spFunc(", !is_void ? "retVal = " : "");

  for (i = 0; i < sym->argc; i++)
    fprintf (cfile, "%s%s", i ? "," : "", sym->arg_name [i]);

  fputs (sym->varargs ? ",valist);\n\tva_end(valist);" : ");", cfile);

  /* TRACE return value */
  fprintf (cfile, "\n\tTRACE(\"Returned (%s)\\n\"",
           get_format_str (sym->return_type));

  if (!is_void)
  {
    if (sym->return_type == ARG_WIDE_STRING)
      fputs (",debugstr_w(retVal)", cfile);
    else
      fprintf (cfile, ",%s%s", sym->return_type == ARG_LONG ? "(LONG)" : "",
               sym->return_type == ARG_STRUCT ? "" : "retVal");
    fputs (");\n\treturn retVal;\n", cfile);
  }
  else
    fputs (");\n", cfile);

  fputs ("}\n", cfile);
}


/*******************************************************************
 *         output_c_postamble
 *
 * Write the last part of the .c file
 */
static void output_c_postamble (void)
{
  if (cfile)
    fclose (cfile);
  cfile = NULL;
}


/*******************************************************************
 *         output_makefile
 *
 * Write a Wine compatible makefile.in
 */
void  output_makefile (void)
{
  FILE *makefile = open_file ("Makefile", ".in", "w");

  if (VERBOSE)
    puts ("Creating makefile");

  fprintf (makefile,
           "# Generated from %s by winedump.\n"
           "MODULE    = %s.dll\n", globals.input_name, OUTPUT_DLL_NAME);

  if (globals.forward_dll)
    fprintf (makefile, "IMPORTS   = %s", globals.forward_dll);

  fprintf (makefile, "\n\nC_SRCS = \\\n\t%s_main.c\n", OUTPUT_DLL_NAME);

  if (globals.forward_dll)
    fprintf (specfile,"#import %s.dll\n", globals.forward_dll);

  fclose (makefile);
}


/*******************************************************************
 *         output_prototype
 *
 * Write a C prototype for a parsed symbol
 */
void  output_prototype (FILE *file, const parsed_symbol *sym)
{
  unsigned int i, start = sym->flags & SYM_THISCALL ? 1 : 0;

  fprintf (file, "%s __%s %s_%s(", sym->return_text, symbol_get_call_convention(sym),
           OUTPUT_UC_DLL_NAME, sym->function_name);

  if (!sym->argc || (sym->argc == 1 && sym->flags & SYM_THISCALL))
    fputs ("void", file);
  else
    for (i = start; i < sym->argc; i++)
      fprintf (file, "%s%s %s", i > start ? ", " : "", sym->arg_text [i],
               sym->arg_name [i]);
  if (sym->varargs)
    fputs (", ...", file);
  fputc (')', file);
}


/*******************************************************************
 *         output_c_banner
 *
 * Write a function banner to the .c file
 */
void  output_c_banner (const parsed_symbol *sym)
{
  char ord_spec[16];
  size_t i;

  if (sym->ordinal >= 0)
    sprintf(ord_spec, "%d", sym->ordinal);
  else
  {
    ord_spec[0] = '@';
    ord_spec[1] = '\0';
  }

  fprintf (cfile, "/*********************************************************"
           "*********\n *\t\t%s (%s.%s)\n *\n", sym->symbol,
           OUTPUT_UC_DLL_NAME, ord_spec);

  if (globals.do_documentation && sym->function_name)
  {
    fputs (" *\n * PARAMS\n *\n", cfile);

    if (!sym->argc)
      fputs (" *  None.\n *\n", cfile);
    else
    {
      for (i = 0; i < sym->argc; i++)
        fprintf (cfile, " *  %s [%s]%s\n", sym->arg_name [i],
                 get_in_or_out(sym, i),
                 strcmp (sym->arg_name [i], "_this") ? "" :
                 "     Pointer to the class object (in ECX)");

      if (sym->varargs)
        fputs (" *  ...[I]\n", cfile);
      fputs (" *\n", cfile);
    }

    fputs (" * RETURNS\n *\n", cfile);

    if (sym->return_text && !strcmp (sym->return_text, "void"))
      fputs (" *  Nothing.\n", cfile);
    else
      fprintf (cfile, " *  %s\n", sym->return_text);
  }
  fputs (" *\n */\n", cfile);
}


/*******************************************************************
 *         get_format_str
 *
 * Get a string containing the correct format string for a type
 */
static const char *get_format_str (int type)
{
  switch (type)
  {
  case ARG_VOID:        return "void";
  case ARG_FLOAT:       return "%f";
  case ARG_DOUBLE:      return "%g";
  case ARG_POINTER:     return "%p";
  case ARG_WIDE_STRING:
  case ARG_STRING:      return "%s";
  case ARG_LONG:        return "%ld";
  case ARG_STRUCT:      return "struct";
  }
  assert (0);
  return "";
}


/*******************************************************************
 *         get_in_or_out
 *
 * Determine if a parameter is In or In/Out
 */
static const char *get_in_or_out (const parsed_symbol *sym, size_t arg)
{
  assert (sym && arg < sym->argc);
  assert (globals.do_documentation);

  if (sym->arg_flag [arg] & CT_CONST)
    return "In";

  switch (sym->arg_type [arg])
  {
  case ARG_FLOAT:
  case ARG_DOUBLE:
  case ARG_LONG:
  case ARG_STRUCT:      return "In";
  case ARG_POINTER:
  case ARG_WIDE_STRING:
  case ARG_STRING:      return "In/Out";
  }
  assert (0);
  return "";
}
