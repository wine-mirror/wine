/***************************************************************************
 * Copyright 1995, Technion, Israel Institute of Technology
 * Electrical Eng, Software Lab.
 * Author:    Michael Veksler.
 ***************************************************************************
 * File:      dde_atom.h
 * Purpose :  atom functionality for DDE
 ***************************************************************************
 */
#ifndef __WINE_DDE_ATOM_H
#define __WINE_DDE_ATOM_H

#ifdef CONFIG_IPC

#include "wintypes.h"

#define DDE_ATOMS 157		   /* a prime number for hashing */

void ATOM_GlobalInit(void);

ATOM DDE_GlobalAddAtom( SEGPTR str );
ATOM DDE_GlobalDeleteAtom( ATOM atom );
ATOM DDE_GlobalFindAtom( SEGPTR str );
WORD DDE_GlobalGetAtomName( ATOM atom, LPSTR buffer, short count );

#endif  /* CONFIG_IPC */

#endif __WINE_DDE_ATOM_H
