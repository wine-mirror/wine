/*
 * Atom table functions
 *
 * Copyright 1993, 1994, 1995 Alexandre Julliard
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "windows.h"

#ifdef CONFIG_IPC
#include "dde_atom.h"
#include "options.h"
#endif

#define MIN_STR_ATOM              0xc000

typedef struct
{
  WORD  refCount;
  BYTE  length;
  char* str;
} ATOMDATA;

typedef struct
{
  void* next;
  ATOMDATA a2h[16];
} ATOMtoHANDLEtable;

static ATOMtoHANDLEtable* GlobalAtomTable = NULL;
static ATOMtoHANDLEtable*  LocalAtomTable = NULL;


/***********************************************************************
 *           ATOM_Init
 *
 * Global table initialisation.
 */
BOOL ATOM_Init(void)
{
  return TRUE;
}


/***********************************************************************
 *           ATOM_AddAtom
 */
static ATOM ATOM_AddAtom( ATOMtoHANDLEtable** tableptr, SEGPTR name )
{
    ATOMDATA* FirstUnused;
    ATOM FirstUnusedIndex;
    ATOM Index;
    ATOMtoHANDLEtable* table;
    int i,len;
    char *str;

    /* Check for integer atom */

    if (!HIWORD(name)) return (ATOM)LOWORD(name);
    str = (char*)name;
    if (str[0] == '#') return atoi( &str[1] );

    if ((len = strlen( str )) > 255) len = 255;
    table = *tableptr;
    FirstUnused = NULL;
    FirstUnusedIndex = 0;
    Index = MIN_STR_ATOM;
    while (table)
    {
      for(i=0; i<16; i++, Index++)
      {
	if (!table->a2h[i].refCount)
	{
	  FirstUnused=&table->a2h[i];
	  FirstUnusedIndex=Index;
	}
	else if ((table->a2h[i].length == len) && 
		 (!strncasecmp( table->a2h[i].str, str, len )))
	{
	    table->a2h[i].refCount++;
	    return Index;
	}
      }
      tableptr = (ATOMtoHANDLEtable**)&table->next;
      table = table->next;
    }
    if(!FirstUnused)
    {
      *tableptr = malloc(sizeof(ATOMtoHANDLEtable));
      (*tableptr)->next = NULL;
      for(i=0; i<16; i++)
      {
	(*tableptr)->a2h[i].str = NULL;
	(*tableptr)->a2h[i].refCount = 0;
      }
      FirstUnused = (*tableptr)->a2h;
      FirstUnusedIndex = Index;
    }
    if((FirstUnused->str = malloc(len+1)))
    {
      memcpy( FirstUnused->str, str, len );
      FirstUnused->str[len] = 0;
    }
    else
      return 0;
    FirstUnused->refCount = 1;
    FirstUnused->length = len;
    return FirstUnusedIndex;
}


/***********************************************************************
 *           ATOM_DeleteAtom
 */
static ATOM ATOM_DeleteAtom( ATOMtoHANDLEtable** tableptr, ATOM atom )
{    
    ATOMtoHANDLEtable* table;
    int i;
    ATOM Index;

    if (atom < MIN_STR_ATOM) return 0;  /* Integer atom */

    Index = MIN_STR_ATOM;
    table = *tableptr;
    while (table)
    {
      if(atom-Index < 16)
      {
	i=atom-Index;

	/* Delete atom */
	if (--table->a2h[i].refCount == 0)
	{
	  free(table->a2h[i].str);
	  table->a2h[i].str=NULL;
	}    
	return 0;
      }
      else
      {
	Index+=16;
	table = table->next;
      }
    }
    return atom;
}


/***********************************************************************
 *           ATOM_FindAtom
 */
static ATOM ATOM_FindAtom( ATOMtoHANDLEtable** tableptr, SEGPTR name )
{
    ATOM Index;
    ATOMtoHANDLEtable* table;
    int i,len;
    char *str;

    /* Check for integer atom */

    if (!HIWORD(name)) return (ATOM)LOWORD(name);
    str = (char*)name;
    if (str[0] == '#') return atoi( &str[1] );

    if ((len = strlen( str )) > 255) len = 255;
    table=*tableptr;
    Index=MIN_STR_ATOM;
    while (table)
    {
      for(i=0; i<16; i++, Index++)
      {
	if ((table->a2h[i].refCount != 0) &&
	    (table->a2h[i].length == len) && 
	    (!strncasecmp( table->a2h[i].str, str, len )))
	  return Index;
      }
      table=table->next;
    }
    return 0;
}


/***********************************************************************
 *           ATOM_GetAtomName
 */
static WORD ATOM_GetAtomName( ATOMtoHANDLEtable** tableptr, ATOM atom,
                              LPSTR buffer, short count )
{
    ATOMtoHANDLEtable* table;
    ATOM Index;
    char * strPtr=NULL;
    int i,len=0;
    char text[8];
    
    if (!count) return 0;
    if (atom < MIN_STR_ATOM)
    {
	sprintf( text, "#%d", atom );
	len = strlen(text);
	strPtr = text;
    }
    else
    {
      Index = MIN_STR_ATOM;
      table = *tableptr;
      while (table)
      {
	if(atom-Index < 16)
	{
	  i=atom-Index;
	  
	  if (table->a2h[i].refCount == 0)
	    table=NULL;
	  else
	  {
	    len = table->a2h[i].length;
	    strPtr = table->a2h[i].str;
	  }
	  break;
	}
	else
	{
	  Index+=16;
	  table = table->next;
	}
      }
      if(!table)return 0;
    }
    if (len >= count) len = count-1;
    memcpy( buffer, strPtr, len );
    buffer[len] = '\0';
    return len;
}


/***********************************************************************
 *           InitAtomTable   (KERNEL.68)
 */
WORD InitAtomTable( WORD entries )
{
    return entries;
}


/***********************************************************************
 *           GetAtomHandle   (KERNEL.73)
 */
HANDLE GetAtomHandle( ATOM atom )
{
  fprintf(stderr,"JBP: GetAtomHandle() called (obsolete).\n");
  return 0;
}


/***********************************************************************
 *           AddAtom   (KERNEL.70)
 */
ATOM AddAtom( SEGPTR str )
{
    return ATOM_AddAtom( &LocalAtomTable, (SEGPTR)str );
}


/***********************************************************************
 *           DeleteAtom   (KERNEL.71)
 */
ATOM DeleteAtom( ATOM atom )
{
    return ATOM_DeleteAtom( &LocalAtomTable, atom );
}


/***********************************************************************
 *           FindAtom   (KERNEL.69)
 */
ATOM FindAtom( SEGPTR str )
{
    return ATOM_FindAtom( &LocalAtomTable, str );
}


/***********************************************************************
 *           GetAtomName   (KERNEL.72)
 */
WORD GetAtomName( ATOM atom, LPSTR buffer, short count )
{
    return ATOM_GetAtomName( &LocalAtomTable, atom, buffer, count );
}


/***********************************************************************
 *           GlobalAddAtom   (USER.268)
 */
ATOM GlobalAddAtom( SEGPTR str )
{
#ifdef CONFIG_IPC
    if (Options.ipc) return DDE_GlobalAddAtom( str );
#endif
    return ATOM_AddAtom( &GlobalAtomTable, str );
}


/***********************************************************************
 *           GlobalDeleteAtom   (USER.269)
 */
ATOM GlobalDeleteAtom( ATOM atom )
{
#ifdef CONFIG_IPC
    if (Options.ipc) return DDE_GlobalDeleteAtom( atom );
#endif
    return ATOM_DeleteAtom( &GlobalAtomTable, atom );
}


/***********************************************************************
 *           GlobalFindAtom   (USER.270)
 */
ATOM GlobalFindAtom( SEGPTR str )
{
#ifdef CONFIG_IPC
    if (Options.ipc) return DDE_GlobalFindAtom( str );
#endif
    return ATOM_FindAtom( &GlobalAtomTable, str );
}


/***********************************************************************
 *           GlobalGetAtomName   (USER.271)
 */
WORD GlobalGetAtomName( ATOM atom, LPSTR buffer, short count )
{
#ifdef CONFIG_IPC
    if (Options.ipc) return DDE_GlobalGetAtomName( atom, buffer, count );
#endif
    return ATOM_GetAtomName( &GlobalAtomTable, atom, buffer, count );
}
