/*
 * Command-line options.
 *
 * Copyright 1994 Alexandre Julliard
 */

#ifndef __WINE_OPTIONS_H
#define __WINE_OPTIONS_H

#include "wintypes.h"

  /* Supported languages */
typedef enum
{
    LANG_En,  /* English */
    LANG_Es,  /* Spanish */
    LANG_De,  /* German */
    LANG_No,  /* Norwegian */
    LANG_Fr,  /* French */
    LANG_Fi,  /* Finnish */
    LANG_Da,  /* Danish */
    LANG_Cz,  /* Czech */
    LANG_Eo,  /* Esperanto */
    LANG_It,  /* Italian */
    LANG_Ko   /* Korean */
} WINE_LANGUAGE;

typedef struct
{
    const char *name;
    WORD        langid;
} WINE_LANGUAGE_DEF;

extern const WINE_LANGUAGE_DEF Languages[];

/* Supported modes */
typedef enum
{
    MODE_STANDARD,
    MODE_ENHANCED
} WINE_MODE;

struct options
{
    char * desktopGeometry; /* NULL when no desktop */
    char * programName;     /* To use when loading resources */
    int    usePrivateMap;
    int    useFixedMap;
    int    synchronous;     /* X synchronous mode */
    int    backingstore;    /* Use backing store */
    short  cmdShow;
    int    debug;
    int    failReadOnly;    /* Opening a read only file will fail
			       if write access is requested */
    WINE_MODE mode;         /* Start Wine in selected mode
			       (standard/enhanced) */
    int    ipc;             /* Use IPC mechanisms */
    WINE_LANGUAGE language; /* Current language */
    int    managed;	    /* Managed windows */
    int    perfectGraphics; /* Favor correctness over speed for graphics */
};

extern struct options Options;

/* Profile functions */

extern int PROFILE_LoadWineIni(void);
extern int PROFILE_GetWineIniString( const char *section, const char *key_name,
                                     const char *def, char *buffer, int len );
extern int PROFILE_GetWineIniInt( const char *section, const char *key_name,
                                  int def );

#endif  /* __WINE_OPTIONS_H */
