/***************************************************************************
 * Copyright 1995, Technion, Israel Institute of Technology
 * Electrical Eng, Software Lab.
 * Author:    Michael Veksler.
 ***************************************************************************
 * File:      dde_atom_test.c
 * Purpose :  tests for dde_atom object
 ***************************************************************************
 */
#include <stdio.h>
#include <stdlib.h>
#include <win.h>
#include "dde_atom.h"
#include "shm_main_blk.h"
#include <debug.h>
#define TOGETHER (DDE_ATOMS/5)


/* run random sequences */
int main()
{
  ATOM atom_list[TOGETHER];
  char str[TOGETHER][80];
  int i,j,atom_n;
  int atom_len[TOGETHER];
  
  TRACE_ON(shm)=1;
  TRACE_ON(atom)=0;
  TRACE_ON(sem)=0;
  
  for (i=0 ; i<=10000/TOGETHER ; i++) {
     for (atom_n=0 ; atom_n<TOGETHER ; atom_n++) {
	atom_len[atom_n]=rand()%64+1;
	for (j=atom_len[atom_n]-1; j>=0; j--) 
	   do {
	      str[atom_n][j]=(char)(rand()%255+1);
	   } while (j==0 && str[atom_n][j]=='#');

	str[atom_n][ atom_len[atom_n] ]='\0';

	atom_list[atom_n]=GlobalAddAtom(str[atom_n]);

	if (atom_list[atom_n]==0) {
	   fprintf(stderr,"failed i=%d, atom_n=%d\n",i,atom_n);
	   return 1;
	}
	if (atom_list[atom_n]!=GlobalAddAtom(str[atom_n])) {
	   fprintf(stderr,
		   "wrong second GlobalAddAtom(\"%s\")\n", str[atom_n]);
	   return 1;
	}
     } /* for */
     for (atom_n=0 ; atom_n<TOGETHER ; atom_n++) {
	char buf[80];
	int len;

	len=GlobalGetAtomName( atom_list[atom_n], buf, 79);
	if (atom_len[atom_n] != len) {
	   fprintf(stderr, "i=%d, atom_n=%d; ", i, atom_n);
	   fprintf(stderr,
		   "wrong length of GlobalGetAtomName(\"%s\")\n",
		   str[atom_n]);

	   return 1;
	}
	   
     }
     for (atom_n=0 ; atom_n<TOGETHER ; atom_n++) {
	GlobalDeleteAtom(atom_list[atom_n]);
	if (atom_list[atom_n]!=GlobalAddAtom(str[atom_n])) {
	   fprintf(stderr, "i=%d, atom_n=%d; ", i, atom_n);
	   fprintf(stderr,
		   "wrong third GlobalAddAtom(\"%s\")\n", str[atom_n]);
	   return 1;
	}
	GlobalDeleteAtom(atom_list[atom_n]);
	GlobalDeleteAtom(atom_list[atom_n]);
	
	atom_list[atom_n]=GlobalAddAtom(str[atom_n]);
	if (atom_list[atom_n]!=GlobalAddAtom(str[atom_n])) {
	   fprintf(stderr,
		   "i=%d, atom_n=%d wrong fifth GlobalAddAtom(\"%s\")\n",
		   i, atom_n,
		   str[atom_n]);
	   return 1;
	}
	GlobalDeleteAtom(atom_list[atom_n]);
	if (atom_list[atom_n]!=GlobalFindAtom(str[atom_n])) {
	   fprintf(stderr,
		   "i=%d, atom_n=%d wrong GlobalFindAtom(\"%s\")\n",
		   i, atom_n,
		   str[atom_n]);
	   return 1;
	}
	GlobalDeleteAtom(atom_list[atom_n]);
     }     
  }
  return 0;
}
