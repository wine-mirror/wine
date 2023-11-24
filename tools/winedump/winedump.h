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

#include "../tools.h"
#include "windef.h"
#include "winbase.h"
#include "wine/mscvpdb.h"

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

typedef enum {NONE, DMGL, SPEC, DUMP} Mode;

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
} parsed_symbol;

/* FIXME: Replace with some hash such as GHashTable */
typedef struct __search_symbol
{
  struct __search_symbol *next;
  BOOL found;
  char symbolname[1];    /* static string, be ANSI C compliant by [1] */
} search_symbol;

/* All globals */
typedef struct __globals
{
  Mode  mode;		   /* SPEC, DEMANGLE or DUMP */

  /* Options: generic */
  BOOL  do_quiet;          /* -q */
  BOOL  do_verbose;        /* -v */

  /* Option arguments: generic */
  const char *input_name;  /* */
  const char *input_module; /* input module name generated after input_name according mode */

  /* Options: spec mode */
  BOOL  do_code;           /* -c, -t, -f */
  BOOL  do_trace;          /* -t, -f */
  BOOL  do_cdecl;          /* -C */
  BOOL  do_documentation;  /* -D */

  /* Options: dump mode */
  BOOL  do_demangle;        /* -d */
  BOOL  do_dumpheader;      /* -f */
  BOOL  do_dump_rawdata;    /* -x */
  BOOL  do_debug;           /* -G == 1, -g == 2 */
  BOOL  do_symbol_table;    /* -t */

  /* Option arguments: spec mode */
  int   start_ordinal;     /* -s */
  int   end_ordinal;       /* -e */
  search_symbol *search_symbol; /* -S */
  char *directory;         /* -I */
  const char *forward_dll; /* -f */
  const char *dll_name;    /* -o */
  const char *uc_dll_name;       /* -o */

  /* Option arguments: dump mode */
  const char **dumpsect;   /* -j */
} _globals;

extern _globals globals;
extern void *dump_base;
extern size_t dump_total_len;

BOOL globals_dump_sect(const char*);

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

/* Image functions */
void	dump_file(const char* name);

/* DLL functions */
BOOL  dll_open (const char *dll_name);

BOOL  dll_next_symbol (parsed_symbol * sym);

/* Symbol functions */
void  symbol_init(parsed_symbol* symbol, const char* name);

char *demangle( const char *name );

BOOL  symbol_search (parsed_symbol *symbol);

void  symbol_clear(parsed_symbol *sym);

BOOL  symbol_is_valid_c(const parsed_symbol *sym);

const char *symbol_get_call_convention(const parsed_symbol *sym);

const char *symbol_get_spec_type (const parsed_symbol *sym, size_t arg);

void  symbol_clean_string (char *string);

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

/* Misc functions */
char *str_substring(const char *start, const char *end);

char *str_replace (char *str, const char *oldstr, const char *newstr);

const char *str_match (const char *str, const char *match, BOOL *found);

const char *str_find_set (const char *str, const char *findset);

char *str_toupper (char *str);

const char *get_machine_str(int mach);

/* file dumping functions */
enum FileSig {SIG_UNKNOWN, SIG_DOS, SIG_PE, SIG_DBG, SIG_PDB, SIG_NE, SIG_LE, SIG_MDMP, SIG_COFFLIB, SIG_LNK,
              SIG_EMF, SIG_EMFSPOOL, SIG_MF, SIG_FNT, SIG_TLB, SIG_NLS, SIG_REG};

const void*	PRD(unsigned long prd, unsigned long len);
unsigned long	Offset(const void* ptr);

typedef void (*file_dumper)(void);
BOOL            dump_analysis(const char*, file_dumper, enum FileSig);

void            dump_data_offset( const unsigned char *ptr, unsigned int size, unsigned int offset, const char *prefix );
void            dump_data( const unsigned char *ptr, unsigned int size, const char *prefix );
const char*	get_time_str( unsigned long );
unsigned int    strlenW( const unsigned short *str );
void            dump_unicode_str( const unsigned short *str, int len );
const char*     get_guid_str(const GUID* guid);
const char*     get_unicode_str( const WCHAR *str, int len );
const char*     get_symbol_str(const char* symname);
void            print_fake_dll(void);
void            dump_file_header(const IMAGE_FILE_HEADER *, BOOL);
void            dump_optional_header(const IMAGE_OPTIONAL_HEADER32 *);
void            dump_section(const IMAGE_SECTION_HEADER *, const char* strtable);
void            dump_section_characteristics(DWORD characteristics, const char* sep);

enum FileSig    get_kind_exec(void);
void            dos_dump( void );
void            pe_dump( void );
void            ne_dump( void );
void            le_dump( void );
enum FileSig    get_kind_mdmp(void);
void            mdmp_dump( void );
enum FileSig    get_kind_lib(void);
void            lib_dump( void );
enum FileSig    get_kind_dbg(void);
void	        dbg_dump( void );
enum FileSig    get_kind_lnk(void);
void	        lnk_dump( void );
enum FileSig    get_kind_emf(void);
unsigned long   dump_emfrecord(const char *pfx, unsigned long offset);
void            emf_dump( void );
enum FileSig    get_kind_emfspool(void);
void            emfspool_dump(void);
enum FileSig    get_kind_mf(void);
void            mf_dump(void);
enum FileSig    get_kind_pdb(void);
void            pdb_dump(void);
enum FileSig    get_kind_fnt(void);
void            fnt_dump( void );
enum FileSig    get_kind_tlb(void);
void            tlb_dump(void);
enum FileSig    get_kind_nls(void);
void            nls_dump(void);
enum FileSig    get_kind_reg(void);
void            reg_dump(void);

BOOL            codeview_dump_symbols(const void* root, unsigned long start, unsigned long size);
BOOL            codeview_dump_types_from_offsets(const void* table, const DWORD* offsets, unsigned num_types);
BOOL            codeview_dump_types_from_block(const void* table, unsigned long len);
void            codeview_dump_linetab(const char* linetab, BOOL pascal_str, const char* pfx);
void            codeview_dump_linetab2(const char* linetab, DWORD size, const PDB_STRING_TABLE*, const char* pfx);
const char*     pdb_get_string_table_entry(const PDB_STRING_TABLE* table, unsigned ofs);

void            dump_stabs(const void* pv_stabs, unsigned szstabs, const char* stabstr, unsigned szstr);
void		dump_codeview(unsigned long ptr, unsigned long len);
void		dump_coff(unsigned long coffbase, unsigned long len,
                          const IMAGE_SECTION_HEADER *sectHead);
void            dump_coff_symbol_table(const IMAGE_SYMBOL *coff_symbols, unsigned num_sym,
                                       const IMAGE_SECTION_HEADER *sectHead);
void		dump_frame_pointer_omission(unsigned long base, unsigned long len);

FILE *open_file (const char *name, const char *ext, const char *mode);

#ifdef __GNUC__
void  do_usage (const char *arg) __attribute__ ((noreturn));
void  fatal (const char *message)  __attribute__ ((noreturn));
#else
void  do_usage (const char *arg);
void  fatal (const char *message);
#endif



#endif /* __WINE_WINEDUMP_H */
