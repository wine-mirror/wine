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
#include "windows.h"

#define DDE_ATOMS 157		   /* a prime number for hashing */

void ATOM_GlobalInit(void);
/*
ATOM GlobalAddAtom( LPCSTR str );
ATOM GlobalDeleteAtom( ATOM atom );
ATOM GlobalFindAtom( LPCSTR str );
WORD GlobalGetAtomName( ATOM atom, LPSTR buffer, short count )
*/
#endif __WINE_DDE_ATOM_H
