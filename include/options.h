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
    int    usePrivateMap;
    int    synchronous;
    short  cmdShow;
};

extern struct options Options;

#endif
