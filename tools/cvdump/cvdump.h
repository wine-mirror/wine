/*
 * Includes for cvdump tool.
 *
 * Copyright 2000 John R. Sheets
 */

/* #define VERBOSE */

#include "neexe.h"
#include "cvinclude.h"

typedef enum { CV_NONE, CV_DOS, CV_NT, CV_DBG } CVHeaderType;

/*
 * Function Prototypes
 */

/* From cvload.c */
CVHeaderType GetHeaderType (FILE *debugfile);
int ReadDOSFileHeader (FILE *debugfile, IMAGE_DOS_HEADER *doshdr);
int ReadPEFileHeader (FILE *debugfile, IMAGE_NT_HEADERS *nthdr);
int ReadDBGFileHeader (FILE *debugfile, IMAGE_SEPARATE_DEBUG_HEADER *dbghdr);

int ReadSectionHeaders (FILE *debugfile, int numsects, IMAGE_SECTION_HEADER **secthdrs);
int ReadDebugDir (FILE *debugfile, int numdirs, IMAGE_DEBUG_DIRECTORY **debugdirs);
int ReadCodeViewHeader (FILE *debugfile, OMFSignature *sig, OMFDirHeader *dirhdr);
int ReadCodeViewDirectory (FILE *debugfile, int entrynum, OMFDirEntry **entries);
int ReadModuleData (FILE *debugfile, int entrynum, OMFDirEntry *entries,
		    int *module_count, OMFModuleFull **modules);
int ReadChunk (FILE *debugfile, void *dest, int length, int fileoffset);

/* From cvprint.c */
int PrintSrcModuleInfo (BYTE* rawdata, short *filecount, short *segcount);
int PrintSrcModuleFileInfo (BYTE* rawdata);
int PrintSrcModuleLineInfo (BYTE* rawdata, int tablecount);
