//
// LIST - list processing functions for orbiter
//
// Version: 1.0  Copyright (C) 1990, Hutchins Software
// Author: Edward Hutchins
// Revisions:
//

#include "orbiter.h"

//
// AddHead - add an object to the head of a list
//

VOID FAR PASCAL AddHead( NPLIST npList, NPNODE npNode )
{
	if (npList->npHead)
	{
		npNode->npNext = npList->npHead;
		npNode->npPrev = NULL;
		npList->npHead = (npList->npHead->npPrev = npNode);
	}
	else // add to an empty list
	{
		npList->npHead = npList->npTail = npNode;
		npNode->npNext = npNode->npPrev = NULL;
	}
}

//
// RemHead - remove the first element in a list
//

NPNODE FAR PASCAL RemHead( NPLIST npList )
{
	if (npList->npHead)
	{
		NPNODE npNode = npList->npHead;
		if (npList->npTail != npNode)
		{
			npList->npHead = npNode->npNext;
			npNode->npNext->npPrev = NULL;
		}
		else npList->npHead = npList->npTail = NULL;
		return( npNode );
	}
	else return( NULL );
}

//
// Remove - remove an arbitrary element from a list
//

VOID FAR PASCAL Remove( NPLIST npList, NPNODE npNode )
{
	if (npNode->npPrev) npNode->npPrev->npNext = npNode->npNext;
	else npList->npHead = npNode->npNext;
	if (npNode->npNext) npNode->npNext->npPrev = npNode->npPrev;
	else npList->npTail = npNode->npPrev;
}

