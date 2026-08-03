/*******************************************************************************
 *
 *	Functions and data structures used to maintain a single list of glyph
 *	names.  The list is sorted alphabetically and each name appears only
 *	once.  After all font information has been read, the 'index' field of
 *	each GLYPHNAME structure is initialized, so future sorts/searches can
 *	be done without comparing strings.
 *
 * Copyright 2001 Ian Pilcher
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <string.h>
#include "psdrv.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(psdrv);

#define	GLYPHLIST_ALLOCSIZE	1024

static GLYPHNAME    **glyphList = NULL;
static INT	    glyphListSize = 0;
static BOOL 	    glyphNamesIndexed = TRUE;

/*******************************************************************************
 *	PSDRV_GlyphListInit
 *
 *  Allocates initial block of memory for the glyph list and copies pointers to
 *  the AGL glyph names into it; returns 0 on success, 1 on failure
 *
 */
INT PSDRV_GlyphListInit(void)
{
    INT i;

    /*
     *	Compute the smallest multiple of GLYPHLIST_ALLOCSIZE that is
     *	greater than or equal to PSDRV_AGLGlyphNamesSize
     *
     */
    glyphListSize = PSDRV_AGLGlyphNamesSize;
    i = ((glyphListSize + GLYPHLIST_ALLOCSIZE - 1) / GLYPHLIST_ALLOCSIZE) *
	    GLYPHLIST_ALLOCSIZE;

    TRACE("glyphList will initially hold %i glyph names\n", i);

    glyphList = HeapAlloc(PSDRV_Heap, 0, i * sizeof(GLYPHNAME *));
    if (glyphList == NULL) return 1;

    for (i = 0; i < glyphListSize; ++i)
	glyphList[i] = PSDRV_AGLGlyphNames + i;

    return 0;
}

/*******************************************************************************
 *	GlyphListInsert
 *
 *  Inserts a copy of the  glyph name into the list at the index, growing the
 *  list if necessary; returns index on success (-1 on failure)
 *
 */
static inline INT GlyphListInsert(LPCSTR szName, INT index)
{
    GLYPHNAME *g;

    g = HeapAlloc(PSDRV_Heap, 0, sizeof(GLYPHNAME) + strlen(szName) + 1);
    if (g == NULL) return -1;

    g->index = -1;
    g->sz = (LPSTR)(g + 1);
    strcpy((LPSTR)g->sz, szName);

    if (glyphListSize % GLYPHLIST_ALLOCSIZE == 0)	/* grow the list? */
    {
	GLYPHNAME   **newGlyphList;

	newGlyphList = HeapReAlloc(PSDRV_Heap, 0, glyphList,
		(glyphListSize + GLYPHLIST_ALLOCSIZE) * sizeof(GLYPHNAME *));
	if (newGlyphList == NULL)
	{
	    HeapFree(PSDRV_Heap, 0, g);
	    return -1;
	}

	glyphList = newGlyphList;

	TRACE("glyphList will now hold %i glyph names\n",
		glyphListSize + GLYPHLIST_ALLOCSIZE);
    }

    if (index < glyphListSize)
    {
	memmove(glyphList + (index + 1), glyphList + index,
		(glyphListSize - index) * sizeof(GLYPHNAME *));
    }

    glyphList[index] = g;
    ++glyphListSize;
    glyphNamesIndexed = FALSE;

#if 0
    TRACE("Added '%s' at glyphList[%i] (glyphListSize now %i)\n",
	    glyphList[index]->sz, index, glyphListSize);
#endif
    return index;
}

/*******************************************************************************
 *	GlyphListSearch
 *
 *  Searches the specified portion of the list for the glyph name and inserts it
 *  in the list if necessary; returns the index at which the name (now) resides
 *  (-1 if unable to insert it)
 *
 */
static INT GlyphListSearch(LPCSTR szName, INT loIndex, INT hiIndex)
{
    INT midIndex, cmpResult;

    while (1)
    {
	if (loIndex > hiIndex)
	    return GlyphListInsert(szName, loIndex);

	midIndex = (loIndex + hiIndex) >> 1;

	cmpResult = strcmp(szName, glyphList[midIndex]->sz);

	if (cmpResult == 0)
	{
#if 0
	    TRACE("Found '%s' at glyphList[%i]\n", glyphList[midIndex]->sz,
		    midIndex);
#endif
	    return midIndex;
	}

	if (cmpResult < 0)
	    hiIndex = midIndex - 1;
	else
	    loIndex = midIndex + 1;
    }
}

/*******************************************************************************
 *	PSDRV_GlyphName
 *
 *  Searches the glyph name list for the provided name, adds it to the list if
 *  necessary, and returns a pointer to it (NULL if unable to add it)
 *
 */
const GLYPHNAME *PSDRV_GlyphName(LPCSTR szName)
{
    INT index;

    index = GlyphListSearch(szName, 0, glyphListSize - 1);
    if (index < 0)
	return NULL;

    return glyphList[index];
}

/*******************************************************************************
 *	PSDRV_IndexGlyphList
 *
 *  Initializes index member of all GLYPHNAME structures
 *
 */
VOID PSDRV_IndexGlyphList(void)
{
    INT i;

    if (glyphNamesIndexed)
    	return;

    TRACE("%i glyph names:\n", glyphListSize);

    for (i = 0; i < glyphListSize; ++i)
    {
    	glyphList[i]->index = i;
#if 0
	TRACE("  glyphList[%i] -> '%s'\n", i, glyphList[i]->sz);
#endif
    }

    glyphNamesIndexed = TRUE;
}
