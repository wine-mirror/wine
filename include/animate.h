/*
 * Animation class extra info
 *
 * Copyright 1998 Eric Kohl
 */

#ifndef __WINE_ANIMATE_H
#define __WINE_ANIMATE_H


typedef struct tagANIMATE_INFO
{
    LPVOID  lpAvi;   /* pointer to avi data */
    HFILE32 hFile;   /* handle to avi file */
} ANIMATE_INFO;


extern void ANIMATE_Register (void);

#endif  /* __WINE_ANIMATE_H */
