/*
 * *DeferWindowPos() structure and definitions
 *
 * Copyright 1994 Alexandre Julliard
 */

#ifndef WINPOS_H
#define WINPOS_H

#define DWP_MAGIC  0x5057  /* 'WP' */

typedef struct
{
    WORD        actualCount;
    WORD        suggestedCount;
    WORD        valid;
    WORD        wMagic;
    WINDOWPOS   winPos[1];
} DWP;

#endif  /* WINPOS_H */
