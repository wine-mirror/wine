/*
 * System metrics definitions
 *
 * Copyright 1994 Alexandre Julliard
 */

#ifndef __WINE_SYSMETRICS_H
#define __WINE_SYSMETRICS_H

extern void SYSMETRICS_Init(void);  /* sysmetrics.c */
extern void SYSCOLOR_Init(void);  /* syscolor.c */

/* Wine extensions */
#define SM_WINE_BPP (SM_CMETRICS+1)  /* screen bpp */
#define SM_WINE_CMETRICS SM_WINE_BPP

#endif  /* __WINE_SYSMETRICS_H */
