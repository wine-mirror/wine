/*
 * Functions to read parts of a .DBG file into their respective struct's
 *
 * Copyright 2000 John R. Sheets
 */

/*
 * .DBG File Layout:
 *
 * IMAGE_SEPARATE_DEBUG_HEADER
 * IMAGE_SECTION_HEADER[]
 * IMAGE_DEBUG_DIRECTORY[]
 * OMFSignature
 * debug data (typical example)
 *   - IMAGE_DEBUG_TYPE_MISC
 *   - IMAGE_DEBUG_TYPE_FPO
 *   - IMAGE_DEBUG_TYPE_CODEVIEW
 * OMFDirHeader
 * OMFDirEntry[]
 */

/*
 * Descriptions:
 *
 * (hdr)  IMAGE_SEPARATE_DEBUG_HEADER - .DBG-specific file header; holds info that 
 *        applies to the file as a whole, including # of COFF sections, file offsets, etc.
 * (hdr)  IMAGE_SECTION_HEADER - list of COFF sections copied verbatim from .EXE;
 *        although this directory contains file offsets, these offsets are meaningless
 *        in the context of the .DBG file, because only the section headers are copied
 *        to the .DBG file...not the binary data it points to.
 * (hdr)  IMAGE_DEBUG_DIRECTORY - list of different formats of debug info contained in file
 *        (see IMAGE_DEBUG_TYPE_* descriptions below); tells where each section starts
 * (hdr)  OMFSignature (CV) - Contains "NBxx" signature, plus file offset telling how far
 *        into the IMAGE_DEBUG_TYPE_CODEVIEW section the OMFDirHeader and OMFDirEntry's sit
 * (data) IMAGE_DEBUG_TYPE_MISC - usually holds name of original .EXE file
 * (data) IMAGE_DEBUG_TYPE_FPO - Frame Pointer Optimization data; used for dealing with
 *        optimized stack frames (optional)
 * (data) IMAGE_DEBUG_TYPE_CODEVIEW - *** THE GOOD STUFF ***
 *        This block of data contains all the symbol tables, line number info, etc.,
 *        that the Visual C++ debugger needs.
 * (hdr)  OMFDirHeader (CV) - 
 * (hdr)  OMFDirEntry (CV) - list of subsections within CodeView debug data section
 */

/*
 * The .DBG file typically has three arrays of directory entries, which tell
 * the OS or debugger where in the file to look for the actual data
 *
 * IMAGE_SECTION_HEADER - number of entries determined by:
 *    (IMAGE_SEPARATE_DEBUG_HEADER.NumberOfSections)
 *
 * IMAGE_DEBUG_DIRECTORY - number of entries determined by:
 *    (IMAGE_SEPARATE_DEBUG_HEADER.DebugDirectorySize / sizeof (IMAGE_DEBUG_DIRECTORY))
 *
 * OMFDirEntry - number of entries determined by:
 *    (OMFDirHeader.cDir)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#include "cvdump.h"

extern DWORD g_dwStartOfCodeView;

/*
 * Extract a generic block of data from debugfile (pass in fileoffset == -1
 * to avoid the fseek()).
 */
int ReadChunk (FILE *debugfile, void *dest, int length, int fileoffset)
{
    size_t bytes_read;

    if (fileoffset >= 0)
	fseek (debugfile, fileoffset, SEEK_SET);

    bytes_read = fread (dest, 1, length, debugfile);
    if (bytes_read < length)
    {
	printf ("ERROR: Only able to read %d bytes of %d-byte chunk!\n",
		bytes_read, length);
	return FALSE;
    }

    return TRUE;
}

/*
 * Scan the next two bytes of a file, and see if they correspond to a file
 * header signature.  Don't forget to put the file pointer back where we
 * found it...
 */
CVHeaderType GetHeaderType (FILE *debugfile)
{
    WORD hdrtype;
    CVHeaderType ret = CV_NONE;

    int oldpos = ftell (debugfile);

#ifdef VERBOSE
    printf (" *** Current file position = %lx\n", ftell (debugfile));
#endif

    if (!ReadChunk (debugfile, &hdrtype, sizeof (WORD), -1))
    {
	fseek (debugfile, oldpos, SEEK_SET);
	return CV_NONE;
    }

     if (hdrtype == 0x5A4D)      /* "MZ" */
	ret = CV_DOS;
    else if (hdrtype == 0x4550)  /* "PE" */
	ret = CV_NT;
    else if (hdrtype == 0x4944)  /* "DI" */
	ret = CV_DBG;

    fseek (debugfile, oldpos, SEEK_SET);

#ifdef VERBOSE
    printf ("Returning header type = %d [0x%x]\n", ret, hdrtype);
    printf (" *** Current file position = %lx\n", ftell (debugfile));
#endif

    return ret;
}

/*
 * Extract the DOS file headers from an executable
 */
int ReadDOSFileHeader (FILE *debugfile, IMAGE_DOS_HEADER *doshdr)
{
    size_t bytes_read;

    bytes_read = fread (doshdr, 1, sizeof (IMAGE_DOS_HEADER), debugfile);
    if (bytes_read < sizeof (IMAGE_DOS_HEADER))
    {
	printf ("ERROR: Only able to read %d bytes of %d-byte DOS file header!\n",
		bytes_read, sizeof (IMAGE_DOS_HEADER));
	return FALSE;
    }

    /* Skip over stub data, if present
     */
    if (doshdr->e_lfanew)
	fseek (debugfile, doshdr->e_lfanew, SEEK_SET);

    return TRUE;
}

/*
 * Extract the DOS and NT file headers from an executable
 */
int ReadPEFileHeader (FILE *debugfile, IMAGE_NT_HEADERS *nthdr)
{
    size_t bytes_read;

    bytes_read = fread (nthdr, 1, sizeof (IMAGE_NT_HEADERS), debugfile);
    if (bytes_read < sizeof (IMAGE_NT_HEADERS))
    {
	printf ("ERROR: Only able to read %d bytes of %d-byte NT file header!\n",
		bytes_read, sizeof (IMAGE_NT_HEADERS));
	return FALSE;
    }

    return TRUE;
}

/*
 * Extract the DBG file header from debugfile
 */
int ReadDBGFileHeader (FILE *debugfile, IMAGE_SEPARATE_DEBUG_HEADER *dbghdr)
{
    size_t bytes_read;

    bytes_read = fread (dbghdr, 1, sizeof (IMAGE_SEPARATE_DEBUG_HEADER), debugfile);
    if (bytes_read < sizeof (IMAGE_SEPARATE_DEBUG_HEADER))
    {
	printf ("ERROR: Only able to read %d bytes of %d-byte DBG file header!\n",
		bytes_read, sizeof (IMAGE_SEPARATE_DEBUG_HEADER));
	return FALSE;
    }

    return TRUE;
}

/*
 * Extract all of the file's COFF section headers into an array of
 * IMAGE_SECTION_HEADER's.  These COFF sections don't really apply to
 * the .DBG file directly (they contain file offsets into the .EXE file
 * which don't correspond to anything in the .DBG file).  They are
 * copied verbatim into this .DBG file to help make the debugging process
 * more robust.  By referencing these COFF section headers, the debugger
 * can still function in the absence of the original .EXE file!
 *
 * NOTE: Do not bother pre-allocating memory.  This function will
 * allocate it for you.  Don't forget to free() it when you're done,
 * though.
 */
int ReadSectionHeaders (FILE *debugfile, int numsects, IMAGE_SECTION_HEADER **secthdrs)
{
    size_t bytes_read;

    /* Need a double-pointer so we can change the destination of the pointer
     * and return the new allocation back to the caller.
     */
    *secthdrs = calloc (numsects, sizeof (IMAGE_SECTION_HEADER));
    bytes_read = fread (*secthdrs, sizeof (IMAGE_SECTION_HEADER), numsects, debugfile);
    if (bytes_read < numsects)
    {
	printf ("ERROR while reading COFF headers: Only able to "
		"read %d headers out of %d desired!\n",
		bytes_read, sizeof (IMAGE_SECTION_HEADER));
	return FALSE;
    }

    return TRUE;
}

/*
 * Load in the debug directory table.  This directory describes the various
 * blocks of debug data that reside at the end of the file (after the COFF
 * sections), including FPO data, COFF-style debug info, and the CodeView
 * we are *really* after.
 */
int ReadDebugDir (FILE *debugfile, int numdirs, IMAGE_DEBUG_DIRECTORY **debugdirs)
{
    size_t bytes_read;

    /* Need a double-pointer so we can change the destination of the pointer
     * and return the new allocation back to the caller.
     */
    *debugdirs = calloc (numdirs, sizeof (IMAGE_DEBUG_DIRECTORY));
    bytes_read = fread (*debugdirs, sizeof (IMAGE_DEBUG_DIRECTORY), numdirs, debugfile);
    if (bytes_read < numdirs)
    {
	printf ("ERROR while reading Debug Directory: Only able to "
		"read %d headers out of %d desired!\n",
		bytes_read, numdirs);
	return FALSE;
    }

    return TRUE;
}

/*
 * Load in the CodeView-style headers inside the CodeView debug section.
 * The 'sig' and 'dirhdr' parameters must point to already-allocated
 * data structures.
 */
int ReadCodeViewHeader (FILE *debugfile, OMFSignature *sig, OMFDirHeader *dirhdr)
{
    size_t bytes_read;

    bytes_read = fread (sig, 1, sizeof (OMFSignature), debugfile);
    if (bytes_read < sizeof (OMFSignature))
    {
	printf ("ERROR while reading CodeView Header Signature: Only "
		"able to read %d bytes out of %d desired!\n",
		bytes_read, sizeof (OMFSignature));
	return FALSE;
    }

    /* Must perform a massive jump, almost to the end of the file, to find the
     * CodeView Directory Header (OMFDirHeader), which is immediately followed
     * by the array of entries (OMFDirEntry).  We calculate the jump based on
     * the beginning of the CodeView debug section (from the CodeView entry in
     * the IMAGE_DEBUG_DIRECTORY array), with the added offset from OMGSignature.
     */
    fseek (debugfile, sig->filepos + g_dwStartOfCodeView, SEEK_SET);
    bytes_read = fread (dirhdr, 1, sizeof (OMFDirHeader), debugfile);
    if (bytes_read < sizeof (OMFDirHeader))
    {
	printf ("ERROR while reading CodeView Directory Header: Only "
		"able to read %d bytes out of %d desired!\n",
		bytes_read, sizeof (OMFDirHeader));
	return FALSE;
    }

    /* File pointer is now at first OMGDirEntry, so we can begin reading those now,
     * with an immediate call to ReadCodeViewDirectory ().
     */

    return TRUE;
}

/*
 * Load in the CodeView directory entries, which each point to a CodeView
 * subsection (e.g. sstModules, sstGlobalPub).  The number of entries in
 * this table is determined by OMFDirEntry.cDir.
 *
 * Strangely enough, this particular section comes immediately *after* 
 * the debug data (as opposed to immediately *before* the data as is the
 * standard with the COFF headers).
 */
int ReadCodeViewDirectory (FILE *debugfile, int entrynum, OMFDirEntry **entries)
{
    size_t bytes_read;

    /* Need a double-pointer so we can change the destination of the pointer
     * and return the new allocation back to the caller.
     */
    /*  printf ("Allocating space for %d entries\n", entrynum); */
    *entries = calloc (entrynum, sizeof (OMFDirEntry));
    /*  printf ("Allocated memory at %p (%p)\n", *entries, entries); */
    bytes_read = fread (*entries, sizeof (OMFDirEntry), entrynum, debugfile);
    if (bytes_read < entrynum)
    {
	printf ("ERROR while reading CodeView Debug Directories: Only "
		"able to read %d entries out of %d desired!\n",
		bytes_read, entrynum);
	return FALSE;
    }

    return TRUE;
}

/*
 * Load in the data contents of all CodeView sstModule sub-sections in the file (likely a
 * large array, as there is one sub-section for every code module... > 100 modules is normal).
 * 'entrynum' should hold the total number of CV sub-sections, not the number of sstModule
 * subsections.  The function will ignore anything that isn't a sstModule.
 *
 * NOTE: 'debugfile' must already be pointing to the correct location.
 */
int ReadModuleData (FILE *debugfile, int entrynum, OMFDirEntry *entries,
		    int *module_count, OMFModuleFull **modules)
{
    int i;
    int segnum;
    size_t bytes_read;
    OMFSegDesc *segarray;
    char namelen;
    OMFModuleFull *module;
    int pad;

    /* How much of the OMFModuleFull struct can we pull directly from the file?
     * (Kind of a hack, but not much else we can do...the 'SegInfo' and 'Name'
     * fields will hold memory pointers, not the actual data from the file.)
     */
    int module_bytes = (sizeof (unsigned short) * 3) + (sizeof (char) * 2);

    if (entries == NULL)
	return FALSE;

    /* Find out how many sstModule sub-sections we have in 'entries'
     */
    *module_count = 0;
    for (i = 0; i < entrynum; i++)
    {
	if (entries[i].SubSection == sstModule)
	    (*module_count)++;
    }

    /* Need a double-pointer so we can change the destination of the pointer
     * and return the new allocation back to the caller.
     */
    *modules = calloc (*module_count, sizeof (OMFModuleFull));
    for (i = 0; i < *module_count; i++)
    {
	/* Convenience pointer to current module
	 */
	module = &(*modules)[i];

	/* Must extract each OMFModuleFull separately from file, because the 'SegInfo'
	 * and 'Name' fields also require separate allocations; the data for these
	 * fields is interspersed in the file, between OMFModuleFull blocks.
	 */
	bytes_read = fread (module, sizeof (char), module_bytes, debugfile);
	if (bytes_read < module_bytes)
	{
	    printf ("ERROR while reading CodeView Module Sub-section Data: "
		    "Only able to read %d bytes from entry %d!\n",
		    bytes_read, i);
	    return FALSE;
	}
	
	/* Allocate space for, and grab the entire 'SegInfo' array.
	 */
	segnum = module->cSeg;
	segarray = calloc (segnum, sizeof (OMFSegDesc));

	bytes_read = fread (segarray, sizeof (OMFSegDesc), segnum, debugfile);
	if (bytes_read < segnum)
	{
	    printf ("ERROR while reading CodeView Module SegInfo Data: "
		    "Only able to read %d segments from module %d!\n",
		    bytes_read, i);
	    return FALSE;
	}
	module->SegInfo = segarray;

	/* Allocate space for the (length-prefixed) 'Name' field.
	 */
	bytes_read = fread (&namelen, sizeof (char), 1, debugfile);
	if (bytes_read < 1)
	{
	    printf ("ERROR while reading CodeView Module Name length!\n");
	    return FALSE;
	}

	/* Read 'Name' field from file.  'Name' must be aligned on a 4-byte
	 * boundary, so we must do a little extra math on the string length.
	 * (NOTE: Must include namelen byte in total padding length, too.)
	 */
	pad = ((namelen + 1) % 4);
	if (pad)
	    namelen += (4 - pad);

	module->Name = calloc (namelen + 1, sizeof (char));
	bytes_read = fread (module->Name, sizeof (char), namelen, debugfile);
	if (bytes_read < namelen)
	{
	    printf ("ERROR while reading CodeView Module Name: "
		    "Only able to read %d chars from module %d!\n",
		    bytes_read, i);
	    return FALSE;
	}
	/*  printf ("%s\n", module->Name); */
    }

#ifdef VERBOSE
    printf ("Done reading %d modules\n", *module_count);
#endif

    return TRUE;
}


