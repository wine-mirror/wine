/*
 * Atom table functions
 *
 * Copyright 1993 Alexandre Julliard
 */

/*
 * Current limitations:
 *
 * - This code should work fine when called from the emulation library,
 * but probably not when called from the Windows program.  The reason
 * is that everything is allocated on the current local heap, instead
 * of taking into account the DS register.  Correcting this will also
 * require some changes in the local heap management to bring it closer
 * to Windows.
 *
 * - The code assumes that LocalAlloc() returns a block aligned on a
 * 4-bytes boundary (because of the shifting done in HANDLETOATOM).
 * If this is not the case, the allocation code will have to be changed.
 *
 * - Integer atoms created with MAKEINTATOM are not supported.  This is
 * because they can't generally be differentiated from string constants
 * located below 0x10000 in the emulation library.  If you need
 * integer atoms, use the "#1234" form.
 *
 * 13/Feb, miguel
 * Changed the calls to LocalAlloc to LocalAlign. When compiling WINELIB
 * you call a special version of LocalAlloc that would do the alignement.
 * When compiling the emulator we depend on LocalAlloc returning the
 * aligned block. Needed to test the Library.
 */

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "user.h"
#include "atom.h"


#define DEFAULT_ATOMTABLE_SIZE    37
#define MIN_STR_ATOM              0xc000

#define ATOMTOHANDLE(atom)        ((HANDLE)(atom) << 2)
#define HANDLETOATOM(handle)      ((ATOM)(0xc000 | ((handle) >> 2)))


static ATOMTABLE * localTable = NULL;
static ATOMTABLE * globalTable = NULL;


/***********************************************************************
 *           ATOM_InitTable
 */
static BOOL ATOM_InitTable( ATOMTABLE ** table, WORD entries )
{
    int i;
    HANDLE handle;
    
    handle = LocalAlign ( LMEM_MOVEABLE, sizeof(ATOMTABLE) +
			 (entries-1) * sizeof(HANDLE) );
    if (!handle) return FALSE;
    *table = (ATOMTABLE *) LocalLock( handle );
    (*table)->size = entries;
    for (i = 0; i < entries; i++) (*table)->entries[i] = 0;
    return TRUE;
    
}


/***********************************************************************
 *           ATOM_Init
 *
 * Global table initialisation.
 */
BOOL ATOM_Init()
{
    return ATOM_InitTable( &globalTable, DEFAULT_ATOMTABLE_SIZE );
}


/***********************************************************************
 *           ATOM_MakePtr
 *
 * Make an ATOMENTRY pointer from a handle (obtained from GetAtomHandle()).
 * Is is assumed that the atom is in the same segment as the table.
 */
static ATOMENTRY * ATOM_MakePtr( ATOMTABLE * table, HANDLE handle )
{
    return (ATOMENTRY *) (((int)table & 0xffff0000) | (int)handle);
}


/***********************************************************************
 *           ATOM_Hash
 */
static WORD ATOM_Hash( WORD entries, LPCSTR str, WORD len )
{
    WORD i, hash = 0;

    for (i = 0; i < len; i++) hash ^= toupper(str[i]) + i;
    return hash % entries;
}


/***********************************************************************
 *           ATOM_AddAtom
 */
static ATOM ATOM_AddAtom( ATOMTABLE * table, LPCSTR str )
{
    WORD hash;
    HANDLE entry;
    ATOMENTRY * entryPtr;
    int len;
    
    if ((len = strlen( str )) > 255) len = 255;

      /* Check for integer atom */
/*    if (!((int)str & 0xffff0000)) return (ATOM)((int)str & 0xffff); */
    if (str[0] == '#') return atoi( &str[1] );

    hash = ATOM_Hash( table->size, str, len );
    entry = table->entries[hash];
    while (entry)
    {
	entryPtr = ATOM_MakePtr( table, entry );
	if ((entryPtr->length == len) && 
	    (!strncasecmp( entryPtr->str, str, len )))
	{
	    entryPtr->refCount++;
	    return HANDLETOATOM( entry );
	}
	entry = entryPtr->next;
    }
    
    entry = (int)LocalAlign( LMEM_MOVEABLE, sizeof(ATOMENTRY)+len-1 ) & 0xffff;
    if (!entry) return 0;
    entryPtr = ATOM_MakePtr( table, entry );
    entryPtr->next = table->entries[hash];
    entryPtr->refCount = 1;
    entryPtr->length = len;
    memcpy( entryPtr->str, str, len );
    table->entries[hash] = entry;
    return HANDLETOATOM( entry );
}


/***********************************************************************
 *           ATOM_DeleteAtom
 */
static ATOM ATOM_DeleteAtom( ATOMTABLE * table, ATOM atom )
{
    ATOMENTRY * entryPtr;
    HANDLE entry, *prevEntry;
    WORD hash;
    
    if (atom < MIN_STR_ATOM) return 0;  /* Integer atom */

    entry = ATOMTOHANDLE( atom );
    entryPtr = ATOM_MakePtr( table, entry );

      /* Find previous atom */
    hash = ATOM_Hash( table->size, entryPtr->str, entryPtr->length );
    prevEntry = &table->entries[hash];
    while (*prevEntry && *prevEntry != entry)
    {
	ATOMENTRY * prevEntryPtr = ATOM_MakePtr( table, *prevEntry );
	prevEntry = &prevEntryPtr->next;
    }    
    if (!*prevEntry) return atom;

      /* Delete atom */
    if (--entryPtr->refCount == 0)
    {
	*prevEntry = entryPtr->next;	
	USER_HEAP_FREE( entry );
    }    
    return 0;
}


/***********************************************************************
 *           ATOM_FindAtom
 */
static ATOM ATOM_FindAtom( ATOMTABLE * table, LPCSTR str )
{
    WORD hash;
    HANDLE entry;
    int len;
    
    if ((len = strlen( str )) > 255) len = 255;
    
      /* Check for integer atom */
/*    if (!((int)str & 0xffff0000)) return (ATOM)((int)str & 0xffff); */
    if (str[0] == '#') return atoi( &str[1] );

    hash = ATOM_Hash( table->size, str, len );
    entry = table->entries[hash];
    while (entry)
    {
	ATOMENTRY * entryPtr = ATOM_MakePtr( table, entry );
	if ((entryPtr->length == len) && 
	    (!strncasecmp( entryPtr->str, str, len )))
	    return HANDLETOATOM( entry );
	entry = entryPtr->next;
    }
    return 0;
}


/***********************************************************************
 *           ATOM_GetAtomName
 */
static WORD ATOM_GetAtomName( ATOMTABLE * table, ATOM atom,
			      LPSTR buffer, short count )
{
    ATOMENTRY * entryPtr;
    HANDLE entry;
    char * strPtr;
    int len;
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
	entry = ATOMTOHANDLE( atom );
	entryPtr = ATOM_MakePtr( table, entry );
	len = entryPtr->length;
	strPtr = entryPtr->str;
    }
    if (len >= count) len = count-1;
    memcpy( buffer, strPtr, len );
    buffer[len] = '\0';
    return len;
}


/***********************************************************************
 *           InitAtomTable   (KERNEL.68)
 */
BOOL InitAtomTable( WORD entries )
{
    return ATOM_InitTable( &localTable, entries );
}


/***********************************************************************
 *           GetAtomHandle   (KERNEL.73)
 */
HANDLE GetAtomHandle( ATOM atom )
{
    if (atom < MIN_STR_ATOM) return 0;
    return ATOMTOHANDLE( atom );
}


/***********************************************************************
 *           AddAtom   (KERNEL.70)
 */
ATOM AddAtom( LPCSTR str )
{
    if (!localTable) InitAtomTable( DEFAULT_ATOMTABLE_SIZE );
    return ATOM_AddAtom( localTable, str );
}


/***********************************************************************
 *           DeleteAtom   (KERNEL.71)
 */
ATOM DeleteAtom( ATOM atom )
{
    if (!localTable) InitAtomTable( DEFAULT_ATOMTABLE_SIZE );
    return ATOM_DeleteAtom( localTable, atom );
}


/***********************************************************************
 *           FindAtom   (KERNEL.69)
 */
ATOM FindAtom( LPCSTR str )
{
    if (!localTable) InitAtomTable( DEFAULT_ATOMTABLE_SIZE );
    return ATOM_FindAtom( localTable, str );
}


/***********************************************************************
 *           GetAtomName   (KERNEL.72)
 */
WORD GetAtomName( ATOM atom, LPSTR buffer, short count )
{
    if (!localTable) InitAtomTable( DEFAULT_ATOMTABLE_SIZE );
    return ATOM_GetAtomName( localTable, atom, buffer, count );
}


/***********************************************************************
 *           GlobalAddAtom   (USER.268)
 */
ATOM GlobalAddAtom( LPCSTR str )
{
    return ATOM_AddAtom( globalTable, str );
}


/***********************************************************************
 *           GlobalDeleteAtom   (USER.269)
 */
ATOM GlobalDeleteAtom( ATOM atom )
{
    return ATOM_DeleteAtom( globalTable, atom );
}


/***********************************************************************
 *           GlobalFindAtom   (USER.270)
 */
ATOM GlobalFindAtom( LPCSTR str )
{
    return ATOM_FindAtom( globalTable, str );
}


/***********************************************************************
 *           GlobalGetAtomName   (USER.271)
 */
WORD GlobalGetAtomName( ATOM atom, LPSTR buffer, short count )
{
    return ATOM_GetAtomName( globalTable, atom, buffer, count );
}
