/*
 * Copyright 1993 Robert J. Amstadt
 * Copyright 1995 Martin von Loewis
 * Copyright 1995, 1996, 1997 Alexandre Julliard
 * Copyright 1997 Eric Youngdale
 * Copyright 1999 Ulrich Weigand
 */

#ifndef __WINE_BUILD_H
#define __WINE_BUILD_H

#include "config.h"
#include <stdio.h>
#include <stdlib.h>

#ifdef NEED_UNDERSCORE_PREFIX
# define PREFIX "_"
#else
# define PREFIX
#endif

#ifdef HAVE_ASM_STRING
# define STRING ".string"
#else
# define STRING ".ascii"
#endif

#if defined(__GNUC__) && !defined(__svr4__)
# define USE_STABS
#else
# undef USE_STABS
#endif

typedef enum
{
    TYPE_VARIABLE,     /* variable */
    TYPE_PASCAL_16,    /* pascal function with 16-bit return (Win16) */
    TYPE_PASCAL,       /* pascal function with 32-bit return (Win16) */
    TYPE_ABS,          /* absolute value (Win16) */
    TYPE_REGISTER,     /* register function */
    TYPE_INTERRUPT,    /* interrupt handler function (Win16) */
    TYPE_STUB,         /* unimplemented stub */
    TYPE_STDCALL,      /* stdcall function (Win32) */
    TYPE_CDECL,        /* cdecl function (Win32) */
    TYPE_VARARGS,      /* varargs function (Win32) */
    TYPE_EXTERN,       /* external symbol (Win32) */
    TYPE_FORWARD,      /* forwarded function (Win32) */
    TYPE_NBTYPES
} ORD_TYPE;

typedef enum
{
    SPEC_INVALID,
    SPEC_WIN16,
    SPEC_WIN32
} SPEC_TYPE;

typedef enum
{
    SPEC_MODE_DLL,
    SPEC_MODE_GUIEXE,
    SPEC_MODE_CUIEXE,
    SPEC_MODE_GUIEXE_NO_MAIN,
    SPEC_MODE_CUIEXE_NO_MAIN
} SPEC_MODE;

typedef struct
{
    int n_values;
    int *values;
} ORD_VARIABLE;

typedef struct
{
    int  n_args;
    char arg_types[17];
    char link_name[80];
} ORD_FUNCTION;

typedef struct
{
    int value;
} ORD_ABS;

typedef struct
{
    char link_name[80];
} ORD_EXTERN;

typedef struct
{
    char link_name[80];
} ORD_FORWARD;

typedef struct
{
    ORD_TYPE    type;
    int         ordinal;
    int         offset;
    int         lineno;
    int         flags;
    char        name[80];
    union
    {
        ORD_VARIABLE   var;
        ORD_FUNCTION   func;
        ORD_ABS        abs;
        ORD_EXTERN     ext;
        ORD_FORWARD    fwd;
    } u;
} ORDDEF;

/* entry point flags */
#define FLAG_NOIMPORT  0x01  /* don't make function available for importing */
#define FLAG_NORELAY   0x02  /* don't use relay debugging for this function */
#define FLAG_RET64     0x04  /* function returns a 64-bit value */
#define FLAG_I386      0x08  /* function is i386 only */

  /* Offset of a structure field relative to the start of the struct */
#define STRUCTOFFSET(type,field) ((int)&((type *)0)->field)

  /* Offset of register relative to the start of the CONTEXT struct */
#define CONTEXTOFFSET(reg)  STRUCTOFFSET(CONTEXT86,reg)

  /* Offset of register relative to the start of the STACK16FRAME struct */
#define STACK16OFFSET(reg)  STRUCTOFFSET(STACK16FRAME,reg)

  /* Offset of register relative to the start of the STACK32FRAME struct */
#define STACK32OFFSET(reg)  STRUCTOFFSET(STACK32FRAME,reg)

  /* Offset of the stack pointer relative to %fs:(0) */
#define STACKOFFSET (STRUCTOFFSET(TEB,cur_stack))


#define MAX_ORDINALS	2048

/* global functions */

extern void *xmalloc (size_t size);
extern void *xrealloc (void *ptr, size_t size);
extern char *xstrdup( const char *str );
extern char *strupper(char *s);
extern void fatal_error( const char *msg, ... );
extern void fatal_perror( const char *msg, ... );
extern void warning( const char *msg, ... );
extern void dump_bytes( FILE *outfile, const unsigned char *data, int len,
                        const char *label, int constant );
extern void add_import_dll( const char *name );
extern void add_ignore_symbol( const char *name );
extern int resolve_imports( FILE *outfile );
extern int output_imports( FILE *outfile );
extern void load_res32_file( const char *name );
extern int output_resources( FILE *outfile );
extern void load_res16_file( const char *name );
extern int output_res16_data( FILE *outfile );
extern int output_res16_directory( unsigned char *buffer );

extern void BuildGlue( FILE *outfile, FILE *infile );
extern void BuildRelays( FILE *outfile );
extern void BuildSpec16File( FILE *outfile );
extern void BuildSpec32File( FILE *outfile );
extern SPEC_TYPE ParseTopLevel( FILE *file );

/* global variables */

extern int current_line;
extern int nb_entry_points;
extern int nb_names;
extern int Base;
extern int Limit;
extern int DLLHeapSize;
extern int UsePIC;
extern int debugging;
extern int nb_debug_channels;
extern int nb_lib_paths;

extern char DLLName[80];
extern char DLLFileName[80];
extern char DLLInitFunc[80];
extern char owner_name[80];
extern const char *input_file_name;
extern const char *output_file_name;
extern char **debug_channels;
extern char **lib_path;

extern ORDDEF EntryPoints[MAX_ORDINALS];
extern ORDDEF *Ordinals[MAX_ORDINALS];
extern ORDDEF *Names[MAX_ORDINALS];
extern SPEC_MODE SpecMode;

#endif  /* __WINE_BUILD_H */
