/*
 * Functions to process in-memory arrays of CodeView data sections
 * (currently only contains sstSrcModule).
 *
 * Copyright 2000 John R. Sheets
 */

/* FIXME - Change to cvprint.c */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#include "cvinclude.h"

/************************ sstSrcModule ************************/

/* Print out stuff in OMFSourceModule block.  Rather than using the supplied
 * OMFSourceModule struct, we'll extract each piece of data separately from
 * the block of memory (rawdata).  This struct (and the others used in
 * sstSrcModule sections) is pretty useless.  We can't use sizeof() on it
 * because it contains the first element of the file offset array (i.e. baseSrcFile),
 * which we need to parse separately anyway.  See below for problems with the
 * other structs.
 *
 * The contents of this section look like this (the first two fields are
 * already extracted and passed in as parameters):
 *
 * unsigned short cFile
 * unsigned short cSeg
 * unsigned long baseSrcFile[cFile]
 * unsigned long segarray[cSeg * 2]
 * unsigned short segindexarray[cSeg]
 */
int PrintSrcModuleInfo (BYTE* rawdata, short *filecount, short *segcount)
{
    int i;
    int datalen;

    unsigned short cFile;
    unsigned short cSeg;
    unsigned long *baseSrcFile;
    unsigned long *segarray;
    unsigned short *segindexarray;

    /* Get all our pointers straightened out
     */
    cFile = *(short*)rawdata;
    cSeg = *(short*)(rawdata + 2);
    baseSrcFile = (long*)(rawdata + 4);
    segarray = &baseSrcFile[cFile];
    segindexarray = (short*)(&segarray[cSeg * 2]);
    
    /* Pass # of segments and files back to calling function
     */
    *filecount = cFile;
    *segcount = cSeg;

    printf ("\n  Module table: Found %d file(s) and %d segment(s)\n", cFile, cSeg);
    for (i = 0; i < cFile; i++)
    {
	printf ("    File #%d begins at an offset of 0x%lx in this section\n",
		i + 1, baseSrcFile[i]);
    }

    for (i = 0; i < cSeg; i++)
    {
	printf ("    Segment #%d start = 0x%lx, end = 0x%lx, seg index = %d\n",
		i + 1, segarray[i * 2], segarray[(i * 2) + 1], segindexarray[i]);
    }

    /* Return the total length of the data (in bytes) that we used, so
     * we'll know how far to jump ahead for the next part of the sstSrcModule.
     */
    datalen = ((BYTE*)(&segindexarray[cSeg]) - rawdata);
    /*  printf ("datalen before padding = %d\n", datalen); */
    if (datalen % 4)
	datalen += 4 - (datalen % 4);
    /*  printf ("datalen after padding = %d\n", datalen); */

    return datalen;
}

/* Print out the contents of a OMFSourceFile block.  Unfortunately, the official
 * version of this struct (probably quite outdated) claims that the 'cFName' field
 * is a short.  Based on experimentation with MSVC 5.0 .DBG files, this field is
 * quite clearly only a single byte.  Yet another reason to do it all by hand
 * and avoid the "official" structs.
 *
 * The contents of this section look like this (the first field is
 * pre-extracted, and 'pad' is ignored):
 *
 * unsigned short cSeg
 * unsigned short pad
 * unsigned long baseSrcLn[cSeg]
 * unsigned long segarray[cSeg * 2]
 * char cFName
 * char Name[cFName]
 */
int PrintSrcModuleFileInfo (BYTE* rawdata)
{
    int i;
    int datalen;

    unsigned short cSeg;
    unsigned long *baseSrcLn;
    unsigned long *segarray;
    unsigned char cFName;
    char Name[256];

    /* Get all our pointers straightened out
     */
    cSeg = *(short*)(rawdata);
    /* Skip the 'pad' field */
    baseSrcLn = (long*)(rawdata + 4);
    segarray = &baseSrcLn[cSeg];
    cFName = *((char*)&segarray[cSeg * 2]);
    snprintf (Name, cFName + 1, "%s", (char*)&segarray[cSeg * 2] + 1);

    /*  printf ("cSeg = %d\n", cSeg); */
    printf ("\n  File table: '%s'\n", Name);

    for (i = 0; i < cSeg; i++)
    {
	printf ("    Segment #%d start = 0x%lx, end = 0x%lx, offset = 0x%lx\n",
		i + 1, segarray[i * 2], segarray[(i * 2) + 1], baseSrcLn[i]);
    }

    /* Return the total length of the data (in bytes) that we used, so
     * we'll know how far to jump ahead for the next part of the sstSrcModule.
     */
    datalen = ((BYTE*)(&segarray[cSeg * 2]) + cFName  + 1 - rawdata);
    /*  printf ("datalen before padding = %d\n", datalen); */
    if (datalen % 4)
	datalen += 4 - (datalen % 4);
    /*  printf ("datalen after padding = %d\n", datalen); */

    return datalen;
}

/* Print out the contents of a OMFSourceLine block.  The contents of this section
 * look like this:
 *
 * unsigned short Seg
 * unsigned short cPair
 * unsigned long offset[cPair]
 * unsigned long linenumber[cPair]
 */
int PrintSrcModuleLineInfo (BYTE* rawdata, int tablecount)
{
    int i;
    int datalen;

    unsigned short Seg;
    unsigned short cPair;
    unsigned long *offset;
    unsigned short *linenumber;

    Seg = *(short*)rawdata;
    cPair = *(short*)(rawdata + 2);
    offset = (long*)(rawdata + 4);
    linenumber = (short*)&offset[cPair];

    printf ("\n  Line table #%d: Found %d line numbers for segment index %d\n",
	    tablecount, cPair, Seg);

    for (i = 0; i < cPair; i++)
    {
	printf ("    Pair #%2d: offset = [0x%8lx], linenumber = %d\n",
		i + 1, offset[i], linenumber[i]);
    }

    /* Return the total length of the data (in bytes) that we used, so
     * we'll know how far to jump ahead for the next part of the sstSrcModule.
     */
    datalen = ((BYTE*)(&linenumber[cPair]) - rawdata);
    /*  printf ("datalen before padding = %d\n", datalen); */
    if (datalen % 4)
	datalen += 4 - (datalen % 4);
    /*  printf ("datalen after padding = %d\n", datalen); */

    return datalen;
}

