/*
 * Win32 built-in DLLs definitions
 *
 * Copyright 1997 Alexandre Julliard
 */

#ifndef __WINE_BUILTIN32_H
#define __WINE_BUILTIN32_H

typedef struct
{
    const char*           filename;     /* DLL file name */
    int                   nb_imports;   /* Number of imported DLLs */
    void                 *pe_header;    /* Buffer for PE header */
    void                 *exports;      /* Pointer to export directory */
    unsigned int          exports_size; /* Total size of export directory */
    const char * const   *imports;      /* Pointer to imports */
    void                (*dllentrypoint)(); /* Pointer to entry point function */
    int                   characteristics;
    void                 *rsrc;         /* Resource descriptor */
} BUILTIN32_DESCRIPTOR;

extern void BUILTIN32_RegisterDLL( const BUILTIN32_DESCRIPTOR *descr );
extern void BUILTIN32_Unimplemented( const char *dllname, const char *funcname );
extern void RELAY_SetupDLL( const char *module );

#endif /* __WINE_BUILTIN32_H */
