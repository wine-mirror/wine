/*
 * Command-line options.
 *
 * Copyright 1994 Alexandre Julliard
 */

#ifndef OPTIONS_H
#define OPTIONS_H

struct options
{
    char * spyFilename;
    char * desktopGeometry; /* NULL when no desktop */
    char * programName;     /* To use when loading resources */
    int    usePrivateMap;
    int    synchronous;
    short  cmdShow;
    int    relay_debug;
    int    debug;
};

extern struct options Options;

#endif
