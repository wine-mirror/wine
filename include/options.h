/*
 * Command-line options.
 *
 * Copyright 1994 Alexandre Julliard
 */

#ifndef OPTIONS_H
#define OPTIONS_H

  /* Supported languages */
typedef enum
{
    LANG_En,  /* English */
    LANG_Es,  /* Spanish */
    LANG_De,  /* German */
    LANG_No,  /* Norwegian */
    LANG_Fr,  /* French */
    LANG_Fi,  /* Finnish */
    LANG_Da   /* Danish */
} WINE_LANGUAGE;

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
    int    allowReadOnly;   /* Opening a read only file will succeed even
			       if write access is requested */
    int    enhanced;        /* Start Wine in enhanced mode */
    int    ipc;             /* Use IPC mechanisms */
    WINE_LANGUAGE language; /* Current language */
    int    managed;	    /* Managed windows */
};

extern struct options Options;

#endif
