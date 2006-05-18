/*
 *  Winedump - A Wine DLL tool
 *
 *  Copyright 2000 Jon Griffiths
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
 *
 *  References:
 *  DLL symbol extraction based on file format from alib (anthonyw.cjb.net).
 *
 *  Option processing shamelessly cadged from winebuild.
 *
 *  All the cool functionality (prototyping, call tracing, forwarding)
 *  relies on Patrik Stridvall's 'function_grep.pl' script to work.
 *
 *  http://msdn.microsoft.com/library/periodic/period96/msj/S330.htm
 *  This article provides both a description and freely downloadable
 *  implementation, in source code form, of how to extract symbols
 *  from Win32 PE executables/DLLs.
 *
 *  http://www.kegel.com/mangle.html
 *  Gives information on the name mangling scheme used by MS compilers,
 *  used as the starting point for the code here. Contains a few
 *  mistakes and some incorrect assumptions, but the lists of types
 *  are pure gold.
 */
#ifndef __WINE_WINEDUMP_H
#define __WINE_WINEDUMP_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <assert.h>
#include <stdarg.h>

/* Argument type constants */
#define MAX_FUNCTION_ARGS   32

#define ARG_VOID            0x0
#define ARG_STRING          0x1
#define ARG_WIDE_STRING     0x2
#define ARG_POINTER         0x3
#define ARG_LONG            0x4
#define ARG_DOUBLE          0x5
#define ARG_STRUCT          0x6 /* By value */
#define ARG_FLOAT           0x7
#define ARG_VARARGS         0x8

/* Compound type flags */
#define CT_BY_REFERENCE     0x1
#define CT_VOLATILE         0x2
#define CT_CONST            0x4
#define CT_EXTENDED         0x8

/* symbol flags */
#define SYM_CDECL           0x1
#define SYM_STDCALL         0x2
#define SYM_THISCALL        0x4
#define SYM_DATA            0x8 /* Data, not a function */

typedef enum {NONE, DMGL, SPEC, DUMP, EMF, LNK} Mode;

/* Structure holding a parsed symbol */
typedef struct __parsed_symbol
{
  char *symbol;
  int   ordinal;
  char *return_text;
  char  return_type;
  char *function_name;
  int varargs;
  unsigned int argc;
  unsigned int flags;
  char  arg_type [MAX_FUNCTION_ARGS];
  char  arg_flag [MAX_FUNCTION_ARGS];
  char *arg_text [MAX_FUNCTION_ARGS];
  char *arg_name [MAX_FUNCTION_ARGS];
  unsigned int n_u_refs;
  char *u_ref    [MAX_FUNCTION_ARGS];
} parsed_symbol;

/* FIXME: Replace with some hash such as GHashTable */
typedef struct __search_symbol
{
  struct __search_symbol *next;
  int found;
  char symbolname[1];    /* static string, be ANSI C compliant by [1] */
} search_symbol;

/* All globals */
typedef struct __globals
{
  Mode  mode;		   /* SPEC, DEMANGLE or DUMP */

  /* Options: generic */
  int   do_quiet;          /* -q */
  int   do_verbose;        /* -v */

  /* Option arguments: generic */
  const char *input_name;  /* */
  const char *input_module; /* input module name generated after input_name according mode */

  /* Options: spec mode */
  int   do_code;           /* -c, -t, -f */
  int   do_trace;          /* -t, -f */
  int   do_cdecl;          /* -C */
  int   do_documentation;  /* -D */

  /* Options: dump mode */
  int   do_demangle;        /* -d */
  int   do_dumpheader;      /* -f */

  /* Option arguments: spec mode */
  int   start_ordinal;     /* -s */
  int   end_ordinal;       /* -e */
  search_symbol *search_symbol; /* -S */
  char *directory;         /* -I */
  const char *forward_dll; /* -f */
  const char *dll_name;    /* -o */
  const char *uc_dll_name;       /* -o */

  /* Option arguments: dump mode */
  const char *dumpsect;    /* -j */

  /* internal options */
  int   do_ordinals;
} _globals;

extern _globals globals;

/* Names to use for output DLL */
#define OUTPUT_DLL_NAME \
          (globals.dll_name ? globals.dll_name : (globals.input_module ? globals.input_module : globals.input_name))
#define OUTPUT_UC_DLL_NAME globals.uc_dll_name

/* Verbosity levels */
#define QUIET   (globals.do_quiet)
#define NORMAL  (!QUIET)
#define VERBOSE (globals.do_verbose)

/* Default calling convention */
#define CALLING_CONVENTION (globals.do_cdecl ? SYM_CDECL : SYM_STDCALL)

/* EMF functions */
int   dump_emf (const char *emf);

/* LNK functions */
int   dump_lnk (const char *lnk);

/* Image functions */
void	dump_file(const char* name);

/* DLL functions */
int   dll_open (const char *dll_name);

int   dll_next_symbol (parsed_symbol * sym);

/* Symbol functions */
int   symbol_init(parsed_symbol* symbol, const char* name);

int   symbol_demangle (parsed_symbol *symbol);

int   symbol_search (parsed_symbol *symbol);

void  symbol_clear(parsed_symbol *sym);

int   symbol_is_valid_c(const parsed_symbol *sym);

const char *symbol_get_call_convention(const parsed_symbol *sym);

const char *symbol_get_spec_type (const parsed_symbol *sym, size_t arg);

void  symbol_clean_string (const char *string);

int   symbol_get_type (const char *string);

/* Output functions */
void  output_spec_preamble (void);

void  output_spec_symbol (const parsed_symbol *sym);

void  output_header_preamble (void);

void  output_header_symbol (const parsed_symbol *sym);

void  output_c_preamble (void);

void  output_c_symbol (const parsed_symbol *sym);

void  output_prototype (FILE *file, const parsed_symbol *sym);

void  output_makefile (void);

void  output_install_script (void);

/* Misc functions */
char *str_create (size_t num_str, ...);

char *str_create_num (size_t num_str, int num, ...);

char *str_substring(const char *start, const char *end);

char *str_replace (char *str, const char *oldstr, const char *newstr);

const char *str_match (const char *str, const char *match, int *found);

const char *str_find_set (const char *str, const char *findset);

char *str_toupper (char *str);

/* file dumping functions */
enum FileSig {SIG_UNKNOWN, SIG_DOS, SIG_PE, SIG_DBG, SIG_NE, SIG_LE, SIG_MDMP};

const void*	PRD(unsigned long prd, unsigned long len);
unsigned long	Offset(const void* ptr);

typedef void (*file_dumper)(enum FileSig, const void*);
int             dump_analysis(const char*, file_dumper, enum FileSig);

void            dump_data( const unsigned char *ptr, unsigned int size, const char *prefix );
const char*	get_time_str( unsigned long );
unsigned int    strlenW( const unsigned short *str );
void            dump_unicode_str( const unsigned short *str, int len );

void            ne_dump( const void *exe, size_t exe_size );
void            le_dump( const void *exe, size_t exe_size );
void            mdmp_dump( void );

FILE *open_file (const char *name, const char *ext, const char *mode);

#ifdef __GNUC__
void  do_usage (void) __attribute__ ((noreturn));
void  fatal (const char *message)  __attribute__ ((noreturn));
#else
void  do_usage (void);
void  fatal (const char *message);
#endif



#endif /* __WINE_WINEDUMP_H */
