/*
 * Win32 built-in DLLs definitions
 *
 * Copyright 1997 Alexandre Julliard
 */

#ifndef __WINE_BUILTIN32_H
#define __WINE_BUILTIN32_H

typedef void (*ENTRYPOINT32)();

typedef struct
{
    const char           *name;         /* DLL name */
    int                   base;         /* Ordinal base */
    int                   nb_funcs;     /* Number of functions */
    int                   nb_names;     /* Number of function names */
    int                   nb_reg_funcs; /* Number of register functions */
    const ENTRYPOINT32   *functions;    /* Pointer to function table */
    const char * const   *names;        /* Pointer to names table */
    const unsigned short *ordinals;     /* Pointer to ordinals table */
    const unsigned char  *args;         /* Pointer to argument lengths */
    const unsigned int   *argtypes;     /* Pointer to argument types bitmask */
} BUILTIN32_DESCRIPTOR;

extern ENTRYPOINT32 BUILTIN32_GetEntryPoint( char *buffer, void *relay,
                                             unsigned int *typemask );
extern void BUILTIN32_Unimplemented( const BUILTIN32_DESCRIPTOR *descr,
                                     int ordinal );
extern void BUILTIN32_PrintDLLs(void);
extern int BUILTIN32_EnableDLL( const char *name, int len, int enable );

#endif /* __WINE_BUILTIN32_H */
