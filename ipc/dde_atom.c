/***************************************************************************
 * Copyright 1995, Technion, Israel Institute of Technology
 * Electrical Eng, Software Lab.
 * Author:    Michael Veksler.
 ***************************************************************************
 * File:      dde_atom.c
 * Purpose :  atom functionality for DDE
 */

#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "dde_atom.h"
#include "shm_main_blk.h"
#include "shm_fragment.h"
#include "ldt.h"
#include "stddebug.h"
#include "debug.h"

typedef struct
{
	WORD        count;
	BYTE        str[1];
} AtomData, *AtomData_ptr;

#define EMPTY   0		   /* empty hash entry */
#define DELETED -1		   /* deleted hash entry */
#define MIN_STR_ATOM 0xfc00

/* OFS2AtomData_ptr: extract AtomData_ptr from ofs */
#define OFS2AtomData_ptr(ofs) ((AtomData*)((int)&main_block->block+(ofs)))

/* OFS2AtomStr: find the string of the atom */
#define OFS2AtomStr(ofs)      (OFS2AtomData_ptr(atom_ofs)->str)

/* offset of an atom according to index */
#define ATOM_OFS(idx) (main_block->atoms[idx])

/* rot_left: rotate (with wrap-around) */
static __inline__ int rot_left(unsigned var,int count)
{
  return (var<<count) | (var>> (sizeof(var)-count));
}
/* find the entry in the atom table for this string */
static int FindHash(LPCSTR str)	   /* ignore str case */
{
  int i,j;
  unsigned hash1,hash2;
  int deleted=-1;		   /* hash for deleted entry */
  int atom_ofs;

  /* get basic hash parameters */
  for (i= hash1= hash2= 0; str[i] ; i++) {
     hash1= rot_left(hash1,5) ^ toupper(str[i]);
     hash2= rot_left(hash2,4) ^ toupper(str[i]);
  }

  hash1%= DDE_ATOMS;
  atom_ofs=ATOM_OFS(hash1);
  switch (atom_ofs) {
    case EMPTY:			   /* empty atom entry */
      return hash1;		   
    case DELETED:		   /* deleted atom entry */
      deleted=hash1;
      break;
    default :			   /* non empty atom entry */
      if (  strcasecmp( OFS2AtomStr(atom_ofs) , str) == 0)
	 return hash1;		   /* found string in atom table */
  }
  hash2%= DDE_ATOMS-1 ;		   /* hash2=0..(DDE_ATOMS-2) */
  hash2++;			   /* hash2=1..(DDE_ATOMS-1) */

  /* make jumps in the hash table by hash2 steps */
  for (i=hash1+hash2 ; ; i+=hash2) {
     /* i wraps around into j */
     j=i-DDE_ATOMS;
     if (j >= 0)
	i=j;			   /* i wraps around */
     
     if (i==hash1) 
	/* here if covered all hash locations, and got back to beginning */
	return deleted;		   /* return first empty entry - if any */
     atom_ofs=ATOM_OFS(i);
     switch (atom_ofs) {
       case EMPTY:		   /* empty atom entry */
	 return i;		   
       case DELETED:		   /* deleted atom entry */
	 if (deleted < 0)
	    /* consider only the first deleted entry */
	    deleted= i;
	 break;
       default :		   /* nonempty atom entry */
	 if (  strcasecmp( OFS2AtomStr(atom_ofs) , str) == 0)
	    return i;	   /* found string in atom table */
     }
  }
}

void ATOM_GlobalInit(void)
{
  int i;
  
  for (i=0 ; i < DDE_ATOMS ; i++)
     ATOM_OFS(i)=EMPTY;
}

/***********************************************************************
 *           GlobalAddAtom   (USER.268)
 */

/* important! don't forget to unlock semaphores before return */
ATOM GlobalAddAtom( SEGPTR name )
{
  int atom_idx;
  int atom_ofs;
  AtomData_ptr ptr;
  ATOM atom;
  char *str;

  /* First check for integer atom */

  if (!HIWORD(name)) return (ATOM)LOWORD(name);

  str = (char *)PTR_SEG_TO_LIN( name );
  if (str[0] == '#')
  {
     ATOM atom= (ATOM) atoi(&str[1]);
     return (atom<MIN_STR_ATOM) ? atom : 0;
  }

  dprintf_atom(stddeb,"GlobalAddAtom(\"%s\")\n",str);

  DDE_IPC_init();		/* will initialize only if needed */
  
  shm_write_wait(main_block->sem);

  atom_idx=FindHash(str);
  atom=(ATOM)0;

  /* use "return" only at the end so semaphore handling is done only once */
  if (atom_idx>=0) {
     /* unless table full and item not found */
     switch (atom_ofs= ATOM_OFS(atom_idx)) {
       case DELETED:
       case EMPTY:		   /* need to allocate new atom */
	 atom_ofs= shm_FragmentAlloc(&main_block->block,
				     strlen(str)+sizeof(AtomData));
	 if (atom_ofs==NIL)
	    break;		   /* no more memory (atom==0) */
	 ATOM_OFS(atom_idx)=atom_ofs;
	 ptr=OFS2AtomData_ptr(atom_ofs);
	 strcpy(ptr->str,str);
	 ptr->count=1;
	 atom=(ATOM)(atom_idx+MIN_STR_ATOM);
	 break;
       default :		   /* has to update existing atom */
	 OFS2AtomData_ptr(atom_ofs)->count++;
	 atom=(ATOM)(atom_idx+MIN_STR_ATOM);
     } /* end of switch */
  } /* end of if */
  shm_write_signal(main_block->sem);
  return atom;
}

/***********************************************************************
 *           GlobalDeleteAtom   (USER.269)
 */

ATOM GlobalDeleteAtom( ATOM atom )
{
  int atom_idx;
  int atom_ofs;
  AtomData_ptr atom_ptr;
  ATOM retval=(ATOM) 0;
  
  dprintf_atom(stddeb,"GlobalDeleteAtom(\"%d\")\n",(int)atom);
  atom_idx=(int)atom - MIN_STR_ATOM;
  
  if (atom_idx < 0 )
     return 0;

  DDE_IPC_init();	   /* will initialize only if needed */

  shm_write_wait(main_block->sem);
  /* return used only once from here on -- for semaphore simplicity */
  switch (atom_ofs=ATOM_OFS(atom_idx)) {
    case DELETED:
    case EMPTY:
      fprintf(stderr,"trying to free unallocated atom %d\n", atom);
      retval=atom;
      break;
    default :
      atom_ptr=OFS2AtomData_ptr(atom_ofs);
      if ( --atom_ptr->count == 0) {
	 shm_FragmentFree(&main_block->block,atom_ofs);
	 ATOM_OFS(atom_idx)=DELETED;
      }
  }
  
  shm_write_signal(main_block->sem);
  return retval;
}

/***********************************************************************
 *           GlobalFindAtom   (USER.270)
 */
ATOM GlobalFindAtom( SEGPTR name )
{
  int atom_idx;
  int atom_ofs;
  char *str;

  dprintf_atom(stddeb,"GlobalFindAtom(%08lx)\n", name );

  /* First check for integer atom */

  if (!HIWORD(name)) return (ATOM)LOWORD(name);

  str = (char *)PTR_SEG_TO_LIN( name );
  if (str[0] == '#')
  {
     ATOM atom= (ATOM) atoi(&str[1]);
     return (atom<MIN_STR_ATOM) ? atom : 0;
  }
  dprintf_atom(stddeb,"GlobalFindAtom(\"%s\")\n",str);

  DDE_IPC_init();		/* will initialize only if needed */

  shm_read_wait(main_block->sem);
  atom_idx=FindHash(str);
  if (atom_idx>=0)		   
     atom_ofs=ATOM_OFS(atom_idx);  /* is it free ? */
  else
     atom_ofs=EMPTY;
  shm_read_signal(main_block->sem);

  if (atom_ofs==EMPTY || atom_ofs==DELETED)
     return 0;
  else
     return (ATOM)(atom_idx+MIN_STR_ATOM);
}

/***********************************************************************
 *           GlobalGetAtomName   (USER.271)
 */
WORD GlobalGetAtomName( ATOM atom, LPSTR buffer, short count )
{
  int atom_idx, atom_ofs;
  int size;
  /* temporary buffer to hold maximum "#65535\0" */
  char str_num[7];
  
  if (count<2)			   /* no sense to go on */
     return 0;
  atom_idx=(int)atom - MIN_STR_ATOM;
  
  if (atom_idx < 0) {		   /* word atom */
     /* use wine convention... */
     sprintf(str_num,"#%d%n",(int)atom,&size);
     if (size+1>count) {	   /* overflow ? */
	/* truncate the string */
	size=count-1;
	str_num[size]='\0';
     }
     strcpy(buffer,str_num);
     return size;
  }

  DDE_IPC_init();		/* will initialize only if needed */

  /* string atom */
  shm_read_wait(main_block->sem);
  atom_ofs=ATOM_OFS(atom_idx);
  if (atom_ofs==EMPTY || atom_ofs==DELETED) {
     fprintf(stderr,"GlobalGetAtomName: illegal atom=%d\n",(int)atom);
     size=0;
  } else {			   /* non empty entry */
     /* string length will be at most count-1, find actual size */
     sprintf(buffer,"%.*s%n",count-1, OFS2AtomStr(atom_ofs), &size);
  }
  shm_read_signal(main_block->sem);
  return size;
}

