/*
 * CVDump - Parses through a Visual Studio .DBG file in CodeView 4 format
 * and dumps the info to STDOUT in a human-readable format
 *
 * Copyright 2000 John R. Sheets
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#include "cvdump.h"

DWORD g_dwStartOfCodeView = 0;

int g_exe_mode = TRUE;
IMAGE_DOS_HEADER g_doshdr;
IMAGE_SEPARATE_DEBUG_HEADER g_dbghdr;
IMAGE_NT_HEADERS g_nthdr;

IMAGE_SECTION_HEADER *g_secthdrs = NULL;
int g_numsects;
int g_dbg_dircount;

IMAGE_DEBUG_DIRECTORY *g_debugdirs = NULL;
OMFSignature g_cvSig;
OMFDirHeader g_cvHeader;
OMFDirEntry *g_cvEntries = NULL;
int g_module_count = 0;
OMFModuleFull *g_cvModules = NULL;

void PrintFilePos (FILE *file)
{
#ifdef VERBOSE
    printf (" *** Current file position = %lx\n", ftell (file));
#endif
}

/* Calculate the file offset, based on the RVA.
 */
DWORD GetOffsetFromRVA (DWORD rva)
{
    int i;
    DWORD offset;
    DWORD filepos;
    DWORD sectbegin;

    /* Assumes all RVA's in the section headers are sorted in increasing
     * order (which should be the case).
     */
    for (i = g_dbg_dircount - 1; i >= 0; i--)
    {
	sectbegin = g_secthdrs[i].VirtualAddress;
#ifdef VERBOSE
	printf ("iter = %d, rva = 0x%lx, sectbegin = 0x%lx\n", i, rva, sectbegin);
#endif
	if (rva >= sectbegin)
	    break;
    }

    /* Calculate the difference between the section's RVA and file position.
     */
    offset = g_secthdrs[i].VirtualAddress - g_secthdrs[i].PointerToRawData;

    /* Calculate the actual file position.
     */
    filepos = rva - offset;

#ifdef VERBOSE
    printf (">>> Found RVA 0x%lx in section %d, at 0x%lx (section offset = 0x%lx)\n",
	    rva, i, filepos, offset);
#endif

    return filepos;
}

int DumpFileHeaders (FILE *debugfile)
{
    CVHeaderType hdrtype;

    hdrtype = GetHeaderType (debugfile);

    if (hdrtype == CV_DOS)
    {
	if (!ReadDOSFileHeader (debugfile, &g_doshdr))
	    return FALSE;

	printf ("\n============================================================\n");
	printf ("                   DOS FILE HEADER\n");
	printf ("============================================================\n");

	printf ("Magic Signature = [0x%4x]\n", g_doshdr.e_magic);
	printf ("e_cblp          = [0x%4x]\n", g_doshdr.e_cblp);
	printf ("e_cp            = [0x%4x]\n", g_doshdr.e_cp);
	printf ("e_cric          = [0x%4x]\n", g_doshdr.e_crlc);
	printf ("e_cparhdr       = [0x%4x]\n", g_doshdr.e_cparhdr);
	printf ("e_minalloc      = [0x%4x]\n", g_doshdr.e_minalloc);
	printf ("e_maxalloc      = [0x%4x]\n", g_doshdr.e_maxalloc);
	printf ("e_ss            = [0x%4x]\n", g_doshdr.e_ss);
	printf ("e_sp            = [0x%4x]\n", g_doshdr.e_sp);
	printf ("e_csum          = [0x%4x]\n", g_doshdr.e_csum);
	printf ("e_ip            = [0x%4x]\n", g_doshdr.e_ip);
	printf ("e_cs            = [0x%4x]\n", g_doshdr.e_cs);
	printf ("e_lfarlc        = [0x%4x]\n", g_doshdr.e_lfarlc);
	printf ("e_ovno          = [0x%4x]\n", g_doshdr.e_ovno);
	printf ("e_res           = [0x%4x ...]\n", g_doshdr.e_res[0]);  /* worth FIXME? */
	printf ("e_oemid         = [0x%4x]\n", g_doshdr.e_oemid);
	printf ("e_oeminfo       = [0x%4x]\n", g_doshdr.e_oeminfo);
	printf ("e_res2          = [0x%4x ...]\n", g_doshdr.e_res2[0]); /* worth FIXME? */
	printf ("e_lfanew        = [0x%8lx]\n", g_doshdr.e_lfanew);

	/* Roll forward to next type */
	hdrtype = GetHeaderType (debugfile);
    }

    if (hdrtype == CV_NT)
    {
	if (!ReadPEFileHeader (debugfile, &g_nthdr))
	    return FALSE;
	
	printf ("\n============================================================\n");
	printf ("                PE EXECUTABLE FILE HEADER\n");
	printf ("============================================================\n");

	printf ("Signature               = [0x%8lx]\n", g_nthdr.Signature);
	printf ("Machine                 = [0x%4x]\n", g_nthdr.FileHeader.Machine);
	printf ("# of Sections           = [0x%4x]\n", g_nthdr.FileHeader.NumberOfSections);
	printf ("Time/Date Stamp         = [0x%08lx]\n", g_nthdr.FileHeader.TimeDateStamp);
	printf ("Pointer to Symbol Table = [0x%8lx]\n", g_nthdr.FileHeader.PointerToSymbolTable);
	printf ("# of Symbols            = [0x%8lx]\n", g_nthdr.FileHeader.NumberOfSymbols);
	printf ("Size of Opt. Hdr        = [0x%4x]\n", g_nthdr.FileHeader.SizeOfOptionalHeader);
	printf ("Characteristics         = [0x%4x]\n", g_nthdr.FileHeader.Characteristics);

	printf ("\n============================================================\n");
	printf ("                   NT FILE HEADER\n");
	printf ("============================================================\n");

	printf ("Magic          = [0x%4x]\n", g_nthdr.OptionalHeader.Magic);
	printf ("Linker Version = %d.%d\n", g_nthdr.OptionalHeader.MajorLinkerVersion,
		g_nthdr.OptionalHeader.MinorLinkerVersion);
	printf ("Size of Code   = [0x%8lx]\n", g_nthdr.OptionalHeader.SizeOfCode);
	printf ("Init. Data     = [0x%8lx]\n", g_nthdr.OptionalHeader.SizeOfInitializedData);
	printf ("Uninit. Data   = [0x%8lx]\n", g_nthdr.OptionalHeader.SizeOfUninitializedData);
	printf ("Entry Point    = [0x%8lx]\n", g_nthdr.OptionalHeader.AddressOfEntryPoint);
	printf ("Base of Code   = [0x%8lx]\n", g_nthdr.OptionalHeader.BaseOfCode);
	printf ("Base of Data   = [0x%8lx]\n", g_nthdr.OptionalHeader.BaseOfData);

	printf ("\n============================================================\n");
	printf ("                 NT OPTIONAL FILE HEADER\n");
	printf ("============================================================\n");

	printf ("Image Base            = [0x%8lx]\n", g_nthdr.OptionalHeader.ImageBase);
	printf ("Section Alignment     = [0x%8lx]\n", g_nthdr.OptionalHeader.SectionAlignment);
	printf ("File Alignment        = [0x%8lx]\n", g_nthdr.OptionalHeader.FileAlignment);
	printf ("OS Version            = %d.%d\n", g_nthdr.OptionalHeader.MajorOperatingSystemVersion,
		g_nthdr.OptionalHeader.MinorOperatingSystemVersion);
	printf ("Image Version         = %d.%d\n", g_nthdr.OptionalHeader.MajorImageVersion,
		g_nthdr.OptionalHeader.MinorImageVersion);
	printf ("Subsystem Version     = %d.%d\n", g_nthdr.OptionalHeader.MajorSubsystemVersion,
		g_nthdr.OptionalHeader.MinorSubsystemVersion);
	printf ("Size of Image         = [0x%8lx]\n", g_nthdr.OptionalHeader.SizeOfImage);
	printf ("Size of Headers       = [0x%8lx]\n", g_nthdr.OptionalHeader.SizeOfHeaders);
	printf ("Checksum              = [0x%8lx]\n", g_nthdr.OptionalHeader.CheckSum);
	printf ("Subsystem             = [0x%4x]\n", g_nthdr.OptionalHeader.Subsystem);
	printf ("DLL Characteristics   = [0x%4x]\n", g_nthdr.OptionalHeader.DllCharacteristics);
	printf ("Size of Stack Reserve = [0x%8lx]\n", g_nthdr.OptionalHeader.SizeOfStackReserve);
	printf ("Size of Stack Commit  = [0x%8lx]\n", g_nthdr.OptionalHeader.SizeOfStackCommit);
	printf ("Size of Heap Reserve  = [0x%8lx]\n", g_nthdr.OptionalHeader.SizeOfHeapReserve);
	printf ("Size of Heap Commit   = [0x%8lx]\n", g_nthdr.OptionalHeader.SizeOfHeapCommit);
	printf ("Loader Flags          = [0x%8lx]\n", g_nthdr.OptionalHeader.LoaderFlags);
	printf ("# of RVA              = [0x%8lx]\n", g_nthdr.OptionalHeader.NumberOfRvaAndSizes);

	printf ("\n============================================================\n");
	printf ("           RVA (RELATIVE VIRTUAL ADDRESS) TABLE\n");
	printf ("============================================================\n");

	printf ("NAME                    RVA         SIZE\n");
	printf ("Export            [0x%8lx] [0x%8lx]\n",
		g_nthdr.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress,
		g_nthdr.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size);
	printf ("Import            [0x%8lx] [0x%8lx]\n", 
		g_nthdr.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress,
		g_nthdr.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size);
	printf ("Resource          [0x%8lx] [0x%8lx]\n", 
		g_nthdr.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress,
		g_nthdr.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].Size);
	printf ("Exception         [0x%8lx] [0x%8lx]\n", 
		g_nthdr.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION].VirtualAddress,
		g_nthdr.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION].Size);
	printf ("Security          [0x%8lx] [0x%8lx]\n", 
		g_nthdr.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY].VirtualAddress,
		g_nthdr.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY].Size);
	printf ("Base Relocations  [0x%8lx] [0x%8lx]\n", 
		g_nthdr.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress,
		g_nthdr.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size);
	printf ("Debug             [0x%8lx] [0x%8lx]\n", 
		g_nthdr.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress,
		g_nthdr.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].Size);
	printf ("Description       [0x%8lx] [0x%8lx]\n", 
		g_nthdr.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COPYRIGHT].VirtualAddress,
		g_nthdr.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COPYRIGHT].Size);
	printf ("Special           [0x%8lx] [0x%8lx]\n", 
		g_nthdr.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_GLOBALPTR].VirtualAddress,
		g_nthdr.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_GLOBALPTR].Size);
	printf ("Thread (TLS)      [0x%8lx] [0x%8lx]\n", 
		g_nthdr.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress,
		g_nthdr.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].Size);
	printf ("Load Config       [0x%8lx] [0x%8lx]\n", 
		g_nthdr.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG].VirtualAddress,
		g_nthdr.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG].Size);
	printf ("Bound Import      [0x%8lx] [0x%8lx]\n", 
		g_nthdr.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT].VirtualAddress,
		g_nthdr.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT].Size);
	printf ("Import Addr Tbl   [0x%8lx] [0x%8lx]\n", 
		g_nthdr.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT].VirtualAddress,
		g_nthdr.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT].Size);
	printf ("Delay Import      [0x%8lx] [0x%8lx]\n", 
		g_nthdr.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT].VirtualAddress,
		g_nthdr.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT].Size);
	printf ("COM Descriptor    [0x%8lx] [0x%8lx]\n", 
		g_nthdr.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR].VirtualAddress,
		g_nthdr.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR].Size);

    }
    else if (hdrtype == CV_DBG)
    {
	if (!ReadDBGFileHeader (debugfile, &g_dbghdr))
	    return FALSE;

	g_exe_mode = FALSE;
#ifdef VERBOSE
	printf ("[ Found DBG header...file is not a PE executable. ]\n");
#endif

	printf ("\n============================================================\n");
	printf ("          STANDALONE DEBUG FILE HEADER (.DBG)\n");
	printf ("============================================================\n");

	printf ("Signature          = [0x%4x]\n", g_dbghdr.Signature);
	printf ("Flags              = [0x%4x]\n", g_dbghdr.Flags);
	printf ("Machine            = [0x%4x]\n", g_dbghdr.Machine);
	printf ("Characteristics    = [0x%4x]\n", g_dbghdr.Characteristics);
	printf ("TimeDateStamp      = [0x%8lx]\n", g_dbghdr.TimeDateStamp);
	printf ("CheckSum           = [0x%8lx]\n", g_dbghdr.CheckSum);
	printf ("ImageBase          = [0x%8lx]\n", g_dbghdr.ImageBase);
	printf ("SizeOfImage        = [0x%8lx]\n", g_dbghdr.SizeOfImage);
	printf ("NumberOfSections   = [0x%8lx]\n", g_dbghdr.NumberOfSections);
	printf ("ExportedNamesSize  = [0x%8lx]\n", g_dbghdr.ExportedNamesSize);
	printf ("DebugDirectorySize = [0x%8lx]\n", g_dbghdr.DebugDirectorySize);

	return TRUE;
    }

    return TRUE;
}

int DumpSectionHeaders (FILE *debugfile)
{
    int i;

    printf ("\n============================================================\n");
    printf ("                 COFF SECTION HEADERS\n");
    printf ("============================================================\n");

    PrintFilePos (debugfile);
    if (!ReadSectionHeaders (debugfile, g_numsects, &g_secthdrs))
	return FALSE;

    /* Print out a quick list of section names
     */
    for (i = 0; i < g_numsects; i++)
	printf ("%8s (0x%08lx bytes long, starts at 0x%08lx)\n", g_secthdrs[i].Name,
		g_secthdrs[i].SizeOfRawData, g_secthdrs[i].PointerToRawData);

    /* Print out bulk of info
     */
    for (i = 0; i < g_numsects; i++)
    {
	printf ("\nContents of IMAGE_SECTION_HEADER %s:\n\n", g_secthdrs[i].Name);

	printf ("Name                 = %s\n", g_secthdrs[i].Name);
	printf ("VirtualSize          = [0x%8lx]\n", g_secthdrs[i].Misc.VirtualSize);
	printf ("VirtualAddress       = [0x%8lx]\n", g_secthdrs[i].VirtualAddress);
	printf ("SizeOfRawData        = [0x%8lx]\n", g_secthdrs[i].SizeOfRawData);
	printf ("PointerToRawData     = [0x%8lx]\n", g_secthdrs[i].PointerToRawData);
	printf ("PointerToRelocations = [0x%8lx]\n", g_secthdrs[i].PointerToRelocations);
	printf ("PointerToLinenumbers = [0x%8lx]\n", g_secthdrs[i].PointerToLinenumbers);
	printf ("NumberOfRelocations  = [0x%4x]\n", g_secthdrs[i].NumberOfRelocations);
	printf ("NumberOfLinenumbers  = [0x%4x]\n", g_secthdrs[i].NumberOfLinenumbers);
	printf ("Characteristics      = [0x%8lx]\n", g_secthdrs[i].Characteristics);
    }

    return TRUE;
}

void PrintDebugDirectoryType (DWORD type)
{
    switch (type)
    {
    case IMAGE_DEBUG_TYPE_UNKNOWN:
	printf ("<Unknown Directory> - %ld\n", type);
	break;
    case IMAGE_DEBUG_TYPE_COFF:
	printf ("COFF Directory:\n");
	break;
    case IMAGE_DEBUG_TYPE_CODEVIEW:
	printf ("CodeView Directory:\n");
	break;
    case IMAGE_DEBUG_TYPE_FPO:
	printf ("FPO Directory:\n");
	break;
    case IMAGE_DEBUG_TYPE_MISC:
	printf ("MISC Directory:\n");
	break;

    default:
	printf ("<Undefined Directory> - %ld\n", type);
    }
}

int DumpDebugDir (FILE *debugfile)
{
    int i;
    int filepos;

    printf ("\n============================================================\n");
    printf ("                CODEVIEW DEBUG DIRECTORY\n");
    printf ("============================================================\n");

    PrintFilePos (debugfile);

    printf ("Found %d CodeView subsection%c...\n", g_dbg_dircount,
	    (g_dbg_dircount == 1) ? '.' : 's');

    if (g_dbg_dircount == 0)
	return FALSE;

    /* Find the location of the debug directory table.
     */
    if (g_exe_mode)
    {
	/* Convert the RVA to a file offset.
	 */
	filepos = GetOffsetFromRVA (g_nthdr.OptionalHeader.DataDirectory[IMAGE_FILE_DEBUG_DIRECTORY].VirtualAddress);

	fseek (debugfile, filepos, SEEK_SET);
	PrintFilePos (debugfile);
    }
#if 0
    else
    {
	int i;

	/* Find the .rdata section.
	 */
	for (i = 0; i < g_numsects; i++)
	    if (strcmp (g_secthdrs[i].Name, ".rdata") == 0)
		break;

	filepos = g_secthdrs[i].PointerToRawData;
    }
#endif

    if (!ReadDebugDir (debugfile, g_dbg_dircount, &g_debugdirs))
	return FALSE;

    /* Print out the contents of the directories.
     */
    for (i = 0; i < g_dbg_dircount; i++)
    {
	/* Remember start of debug data...for later
	 */
	if (g_debugdirs[i].Type == IMAGE_DEBUG_TYPE_CODEVIEW)
	{
	    g_dwStartOfCodeView = g_debugdirs[i].PointerToRawData;
#ifdef VERBOSE
	    printf ("\n[ Found start of CodeView data, at 0x%lx ]\n\n", g_dwStartOfCodeView);
#endif
	}

	printf ("\n");
	PrintDebugDirectoryType (g_debugdirs[i].Type);
	printf ("  Characteristics  = [0x%8lx]\n", g_debugdirs[i].Characteristics);
	printf ("  TimeDateStamp    = [0x%8lx]\n", g_debugdirs[i].TimeDateStamp);
	printf ("  Version          = %d.%d\n", g_debugdirs[i].MajorVersion, g_debugdirs[i].MinorVersion);
	printf ("  SizeOfData       = [0x%8lx]\n", g_debugdirs[i].SizeOfData);
	printf ("  AddressOfRawData = [0x%8lx]\n", g_debugdirs[i].AddressOfRawData);
	printf ("  PointerToRawData = [0x%8lx]\n", g_debugdirs[i].PointerToRawData);
    }

    free (g_debugdirs);
    return TRUE;
}

void PrintSubsectionName (int ssNum)
{
    switch (ssNum)
    {
    case sstModule:
	printf ("sstModule");
	break;
    case sstAlignSym:
	printf ("sstAlignSym");
	break;
    case sstSrcModule:
	printf ("sstSrcModule");
	break;
    case sstLibraries:
	printf ("sstLibraries");
	break;
    case sstGlobalSym:
	printf ("sstGlobalSym");
	break;
    case sstGlobalPub:
	printf ("sstGlobalPub");
	break;
    case sstGlobalTypes:
	printf ("sstGlobalTypes");
	break;
    case sstSegMap:
	printf ("sstSegMap");
	break;
    case sstFileIndex:
	printf ("sstFileIndex");
	break;
    case sstStaticSym:
	printf ("sstStaticSym");
	break;

    default:
	printf ("<undefined> - %x", ssNum);
    }
}

int DumpCodeViewSummary (OMFDirEntry *entries, long entrycount)
{
    int i;
    int modulecount = 0, alignsymcount = 0, srcmodulecount = 0, librariescount = 0;
    int globalsymcount = 0, globalpubcount = 0, globaltypescount = 0;
    int segmapcount = 0, fileindexcount = 0, staticsymcount = 0;

    if (entries == NULL || entrycount == 0)
	return FALSE;

    for (i = 0; i < entrycount; i++)
    {
	switch ((int)g_cvEntries[i].SubSection)
	{
	case sstModule:
	    modulecount++;
	    break;
	case sstAlignSym:
	    alignsymcount++;
	    break;
	case sstSrcModule:
	    srcmodulecount++;
	    break;
	case sstLibraries:
	    librariescount++;
	    break;
	case sstGlobalSym:
	    globalsymcount++;
	    break;
	case sstGlobalPub:
	    globalpubcount++;
	    break;
	case sstGlobalTypes:
	    globaltypescount++;
	    break;
	case sstSegMap:
	    segmapcount++;
	    break;
	case sstFileIndex:
	    fileindexcount++;
	    break;
	case sstStaticSym:
	    staticsymcount++;
	    break;
	}	
    }

    /* This one has to be > 0
     */
    printf ("\nFound: %d sstModule subsections\n", modulecount);

    if (alignsymcount > 0)    printf ("       %d sstAlignSym subsections\n", alignsymcount);
    if (srcmodulecount > 0)   printf ("       %d sstSrcModule subsections\n", srcmodulecount);
    if (librariescount > 0)   printf ("       %d sstLibraries subsections\n", librariescount);
    if (globalsymcount > 0)   printf ("       %d sstGlobalSym subsections\n", globalsymcount);
    if (globalpubcount > 0)   printf ("       %d sstGlobalPub subsections\n", globalpubcount);
    if (globaltypescount > 0) printf ("       %d sstGlobalTypes subsections\n", globaltypescount);
    if (segmapcount > 0)      printf ("       %d sstSegMap subsections\n", segmapcount);
    if (fileindexcount > 0)   printf ("       %d sstFileIndex subsections\n", fileindexcount);
    if (staticsymcount > 0)   printf ("       %d sstStaticSym subsections\n", staticsymcount);
    
    return TRUE;
}

int DumpCodeViewHeaders (FILE *debugfile)
{
    printf ("\n============================================================\n");
    printf ("                    CODEVIEW HEADERS\n");
    printf ("============================================================\n");

    PrintFilePos (debugfile);

    fseek (debugfile, g_dwStartOfCodeView, SEEK_SET);
    printf ("CodeView Directory Table begins at filepos = 0x%lx\n\n", ftell (debugfile));

    if (!ReadCodeViewHeader (debugfile, &g_cvSig, &g_cvHeader))
	return FALSE;

    printf ("Signature   = %.4s\n", g_cvSig.Signature);
    printf ("filepos     = [0x%8lx]\n", g_cvSig.filepos);
    printf ("File Location of debug directories = [0x%8lx]\n\n", g_cvSig.filepos + g_dwStartOfCodeView);

    printf ("Size of header    = [0x%4x]\n", g_cvHeader.cbDirHeader);
    printf ("Size per entry    = [0x%4x]\n", g_cvHeader.cbDirEntry);
    printf ("# of entries      = [0x%8lx] (%ld)\n", g_cvHeader.cDir, g_cvHeader.cDir);
    printf ("Offset to NextDir = [0x%8lx]\n", g_cvHeader.lfoNextDir);
    printf ("Flags             = [0x%8lx]\n", g_cvHeader.flags);

    if (!ReadCodeViewDirectory (debugfile, g_cvHeader.cDir, &g_cvEntries))
	return FALSE;

    DumpCodeViewSummary (g_cvEntries, g_cvHeader.cDir);

    return TRUE;
}

/*
 * Print out the info contained in the sstModule section of a single module
 */
int DumpModuleInfo (int index)
{
    int segnum;

    if (g_cvEntries == NULL || g_cvModules == NULL)
	return FALSE;

    printf ("---------------------- sstModule ----------------------\n");

    /* Print out some juicy module data
     */
    printf ("  '%s' module holds %d segment(s) (style %c%c)\n",
	    g_cvModules[index].Name, g_cvModules[index].cSeg,
	    g_cvModules[index].Style[0], g_cvModules[index].Style[1]);

    /* Print out info from module's OMFDirEntry
     */
    printf ("             file offset = [0x%8lx]\n", g_cvEntries[index].lfo);
    printf ("             size        = [0x%8lx]\n\n", g_cvEntries[index].cb);

    for (segnum = 0; segnum < g_cvModules[index].cSeg; segnum++)
    {
	printf ("  segment #%d: offset = [0x%8lx], size = [0x%8lx]\n",
		g_cvModules[index].SegInfo[segnum].Seg, 
		g_cvModules[index].SegInfo[segnum].Off, 
		g_cvModules[index].SegInfo[segnum].cbSeg);
    }

    return TRUE;
}

int DumpGlobalPubInfo (int index, FILE *debugfile)
{
    long fileoffset;
    unsigned long sectionsize;
    OMFSymHash header;
    BYTE *symbols;
    BYTE *curpos;
    PUBSYM32 *sym;
    char symlen;
    char *symname;
    int recordlen;
    char nametmp[256] = { 0 };  /* Zero out */

    if (g_cvEntries == NULL || debugfile == NULL || 
	g_cvEntries[index].SubSection != sstGlobalPub)
	return FALSE;

    printf ("-------------------- sstGlobalPub --------------------\n");

    sectionsize = g_cvEntries[index].cb;
    printf ("  offset = [0x%8lx]\n  size   = [0x%8lx]\n", g_cvEntries[index].lfo, sectionsize);

    fileoffset = g_dwStartOfCodeView + g_cvEntries[index].lfo;
    printf ("  GlobalPub section starts at file offset 0x%lx\n", fileoffset);
    printf ("  Symbol table starts at 0x%lx\n", fileoffset + sizeof (OMFSymHash));

#ifdef VERBOSE
    printf (" [iMod = %d] [index = %d]\n", g_cvEntries[index].iMod, index);
#endif

    printf ("\n                       ----- Begin Symbol Table -----\n");
    printf ("  (type)      (symbol name)                 (offset)      (len) (seg) (ind)\n");

    /* Read the section header.
     */
    if (!ReadChunk (debugfile, (void*)&header, sizeof (OMFSymHash), fileoffset))
	return FALSE;
    PrintFilePos (debugfile);

    /* Read the entire sstGlobalPub symbol table.
     */
    symbols = malloc (header.cbSymbol);
    if (!ReadChunk (debugfile, (void*)symbols, header.cbSymbol, -1))
	return FALSE;

    /* We don't know how many symbols are in this block of memory...only what
     * the total size of the block is.  Because the symbol's name is tacked
     * on to the end of the PUBSYM32 struct, each symbol may take up a different
     * # of bytes.  This makes it harder to parse through the symbol table,
     * since we won't know the exact location of the following symbol until we've
     * already parsed the current one.
     */
    curpos = symbols;
    while (curpos < symbols + header.cbSymbol)
    {
	/* Point to the next PUBSYM32 in the table.
	 */
	sym = (PUBSYM32*)curpos;

	/* Ugly hack to find the start of the (length-prefixed) name string.
	 * Must be careful about pointer math (i.e. can't use 'sym').
	 *
	 * FIXME: Should take into account the length...this approach hopes
	 * for a coincidental NULL after the string.
	 */
	symlen = *(curpos + sizeof (PUBSYM32));
	symname = curpos + sizeof (PUBSYM32) + 1;

	/* "  (type) (symbol name)         (offset)   (len) (seg) (typind)" */

	snprintf (nametmp, symlen + 1, "%s", symname);
	printf ("  0x%04x  %-30.30s  [0x%8lx]  [0x%4x]  %d     %ld\n",
		sym->rectyp, nametmp, sym->off, sym->reclen, sym->seg, sym->typind);

	/* The entire record is null-padded to the nearest 4-byte
	 * boundary, so we must do a little extra math to keep things straight.
	 */
	recordlen = sym->reclen;
	if (recordlen % 4)
	    recordlen += 4 - (recordlen % 4);
	
	/*  printf ("Padding length of %d bytes to %d\n", sym->reclen, recordlen); */

	curpos += recordlen;
    }

    printf ("  Freeing symbol memory...\n");
    free (symbols);

    return TRUE;
}

int DumpGlobalSymInfo (int index, FILE *debugfile)
{
    if (g_cvEntries == NULL || debugfile == NULL || 
	g_cvEntries[index].SubSection != sstGlobalSym)
	return FALSE;

    /*** NOT YET IMPLEMENTED ***/
    printf ("---Found section ");
    PrintSubsectionName (g_cvEntries[index].SubSection);
    printf (" [iMod = %d] [index = %d]", g_cvEntries[index].iMod, index);
    printf (" of module #%d---\n", index + 1);

    return TRUE;
}

int DumpStaticSymInfo (int index, FILE *debugfile)
{
    if (g_cvEntries == NULL || debugfile == NULL || 
	g_cvEntries[index].SubSection != sstStaticSym)
	return FALSE;

    /*** NOT YET IMPLEMENTED ***/
    printf ("---Found section ");
    PrintSubsectionName (g_cvEntries[index].SubSection);
    printf (" [iMod = %d] [index = %d]", g_cvEntries[index].iMod, index);
    printf (" of module #%d---\n", index + 1);

    return TRUE;
}

int DumpLibrariesInfo (int index, FILE *debugfile)
{
    if (g_cvEntries == NULL || debugfile == NULL || 
	g_cvEntries[index].SubSection != sstLibraries)
	return FALSE;

    /*** NOT YET IMPLEMENTED ***/
    printf ("---Found section ");
    PrintSubsectionName (g_cvEntries[index].SubSection);
    printf (" [iMod = %d] [index = %d]", g_cvEntries[index].iMod, index);
    printf (" of module #%d---\n", index + 1);

    return TRUE;
}

int DumpGlobalTypesInfo (int index, FILE *debugfile)
{
    if (g_cvEntries == NULL || debugfile == NULL || 
	g_cvEntries[index].SubSection != sstGlobalTypes)
	return FALSE;

    /*** NOT YET IMPLEMENTED ***/
    printf ("---Found section ");
    PrintSubsectionName (g_cvEntries[index].SubSection);
    printf (" [iMod = %d] [index = %d]", g_cvEntries[index].iMod, index);
    printf (" of module #%d---\n", index + 1);

    return TRUE;
}

int DumpSegMapInfo (int index, FILE *debugfile)
{
    if (g_cvEntries == NULL || debugfile == NULL || 
	g_cvEntries[index].SubSection != sstSegMap)
	return FALSE;

    printf ("-------------------- sstSegMap --------------------\n");

    printf ("---Found section ");
    PrintSubsectionName (g_cvEntries[index].SubSection);
    printf (" [iMod = %d] [index = %d]", g_cvEntries[index].iMod, index);
    printf (" of module #%d---\n", index + 1);

    return TRUE;
}

int DumpFileIndexInfo (int index, FILE *debugfile)
{
    if (g_cvEntries == NULL || debugfile == NULL || 
	g_cvEntries[index].SubSection != sstFileIndex)
	return FALSE;

    /*** NOT YET IMPLEMENTED ***/
    printf ("---Found section ");
    PrintSubsectionName (g_cvEntries[index].SubSection);
    printf (" [iMod = %d] [index = %d]", g_cvEntries[index].iMod, index);
    printf (" of module #%d---\n", index + 1);

    return TRUE;
}

int DumpSrcModuleInfo (int index, FILE *debugfile)
{
    int i;
    int fileoffset;
    BYTE *rawdata;
    BYTE *curpos;
    short filecount;
    short segcount;

    int moduledatalen;
    int filedatalen;
    int linedatalen;

    if (g_cvEntries == NULL || debugfile == NULL || 
	g_cvEntries[index].SubSection != sstSrcModule)
	return FALSE;

    printf ("--------------------- sstSrcModule --------------------\n");
    printf ("             file offset = [0x%8lx]\n", g_dwStartOfCodeView + g_cvEntries[index].lfo);
    printf ("             size        = [0x%8lx]\n", g_cvEntries[index].cb);

    /* Where in the .DBG file should we start reading?
     */
    fileoffset = g_dwStartOfCodeView + g_cvEntries[index].lfo;

    /* Allocate a chunk of memory for the entire sstSrcModule
     */
    rawdata = malloc (g_cvEntries[index].cb);
    if (!rawdata)
    {
	printf ("ERROR - Unable to allocate %ld bytes for DumpSrcModuleInfo()\n",
		g_cvEntries[index].cb);
	return FALSE;
    }

    /* Read in the entire sstSrcModule from the .DBG file.  We'll process it
     * bit by bit, but passing memory pointers into the various functions in
     * cvprint.c.
     */
    if (!ReadChunk (debugfile, (void*)rawdata, g_cvEntries[index].cb, fileoffset))
	return FALSE;

    moduledatalen = PrintSrcModuleInfo (rawdata, &filecount, &segcount);
#ifdef VERBOSE
    printf ("*** PrintSrcModuleInfo() returned %d\n", moduledatalen);
#endif

    curpos = rawdata + moduledatalen;
    filedatalen = PrintSrcModuleFileInfo (curpos);
#ifdef VERBOSE
    printf ("*** PrintSrcModuleFileInfo() returned %d\n", filedatalen);
#endif

    curpos += filedatalen;
    for (i = 0; i < segcount; i++)
    {
	linedatalen = PrintSrcModuleLineInfo (curpos, i);
#ifdef VERBOSE
	printf ("*** PrintSrcModuleLineInfo() returned %d\n", linedatalen);
#endif

	curpos += linedatalen;
    }

    free (rawdata);

    return TRUE;
}

int DumpAlignSymInfo (int index, FILE *debugfile)
{
    if (g_cvEntries == NULL || debugfile == NULL || 
	g_cvEntries[index].SubSection != sstAlignSym)
	return FALSE;

    /*** NOT YET IMPLEMENTED ***/
    printf ("--------------------- sstAlignSym ---------------------\n");
    printf ("  [iMod = %d] [index = %d]", g_cvEntries[index].iMod, index);
    printf (" of module #%d\n", index + 1);

    return TRUE;
}

/*
 * Print out the info of all related modules (e.g. sstAlignSym, sstSrcModule)
 * for the desired sub-section (i.e. sstModule).
 */
int DumpRelatedSections (int index, FILE *debugfile)
{
    int i;

    if (g_cvEntries == NULL)
	return FALSE;

    /*  printf ("...Scanning %ld entries for matches on module #%d\n", g_cvHeader.cDir, module_num); */

    for (i = 0; i < g_cvHeader.cDir; i++)
    {
	if (g_cvEntries[i].iMod != (index + 1) ||
	    g_cvEntries[i].SubSection == sstModule)
	    continue;

	/* Pass in index of entry in g_cvEntries array to individual sub-section
	 * dumping functions.  Each function will figure out where in the file its
	 * sub-section lies and seek the file position itself, before parsing out
	 * its data.
	 */
	switch (g_cvEntries[i].SubSection)
	{
	case sstAlignSym:
	    DumpAlignSymInfo (i, debugfile);
	    break;
	case sstSrcModule:
	    DumpSrcModuleInfo (i, debugfile);
	    break;

	default:
	    printf ("---Found section ");
	    PrintSubsectionName (g_cvEntries[i].SubSection);
	    printf (" [iMod = %d] [i = %d]", g_cvEntries[i].iMod, i);
	    printf (" of module #%d---\n", index + 1);
	}
    }

    return TRUE;
}

int DumpMiscSections (int index, FILE *debugfile)
{
    /* The module # 65535 is reserved for all free-standing modules, not
     * associated with a sstModule sub-section.  These are the only sections
     * we wish to process here.
     */
    if (g_cvEntries == NULL || g_cvEntries[index].iMod != 65535)
	return FALSE;

    /* Pass in index of entry in g_cvEntries array to individual sub-section
     * dumping functions.  Each function will figure out where in the file its
     * sub-section lies and seek the file position itself, before parsing out
     * its data.
     */
    switch (g_cvEntries[index].SubSection)
    {
    case sstGlobalPub:
	DumpGlobalPubInfo (index, debugfile);
	break;
    case sstGlobalSym:
	DumpGlobalSymInfo (index, debugfile);
	break;
    case sstStaticSym:
	DumpStaticSymInfo (index, debugfile);
	break;
    case sstLibraries:
	DumpLibrariesInfo (index, debugfile);
	break;
    case sstGlobalTypes:
	DumpGlobalTypesInfo (index, debugfile);
	break;
    case sstSegMap:
	DumpSegMapInfo (index, debugfile);
	break;
    case sstFileIndex:
	DumpFileIndexInfo (index, debugfile);
	break;

    default:
	printf ("---Found section ");
	PrintSubsectionName (g_cvEntries[index].SubSection);
	printf (" [iMod = %d] [index = %d]", g_cvEntries[index].iMod, index);
	printf (" of module #%d---\n", index + 1);
    }

    return TRUE;
}

int DumpAllModules (FILE *debugfile)
{
    int i;

    if (g_cvHeader.cDir == 0 || g_cvEntries == NULL)
    {
	printf ("ERROR: Bailing out of Module Data Dump\n");
	printf ("%ld %p\n", g_cvHeader.cDir, g_cvEntries);
	return FALSE;
    }

    printf ("\n============================================================\n");
    printf ("                    MODULE LISTING\n");
    printf ("============================================================\n");

    /* Seek to beginning of debug data
     */
    fseek (debugfile, g_dwStartOfCodeView + g_cvEntries[0].lfo, SEEK_SET);
#ifdef VERBOSE
    printf ("[ Moving to filepos = 0x%lx to read in CodeView module info ]\n",
	    ftell (debugfile));
#endif

    /* Load all OMFModuleFull data from file into memory
     */
    if (!ReadModuleData (debugfile, g_cvHeader.cDir, g_cvEntries,
			 &g_module_count, &g_cvModules))
    {
	PrintFilePos (debugfile);
	return FALSE;
    }

    /* Print out bulk of info (depends on the fact that all sstModule's
     * are packed at the beginning of the array).
     */
    printf ("Found %d modules\n", g_module_count);
    for (i = 0; i < g_module_count; i++)
    {
	printf ("\n====================== Module #%d ======================\n", i + 1);
	DumpModuleInfo (i);
	DumpRelatedSections (i, debugfile);
	printf ("=======================================================\n");
    }

    printf ("\n============================================================\n");
    printf ("                  MISCELLANEOUS MODULES\n");
    printf ("============================================================\n");

    for (i = 0; i < g_cvHeader.cDir; i++)
    {
	DumpMiscSections (i, debugfile);
    }

    return TRUE;
}


/*
 * Free Global data used by OMFModuleFull structs.  Can't just use free() because
 * the 'SegInfo' and 'Name' fields also have allocated memory.
 */
void FreeCVModules ()
{
    int i;
    OMFModuleFull *module;

    for (i = 0; i < g_module_count; i++)
    {
	module = &(g_cvModules[i]);

	free (module->SegInfo);
	free (module->Name);
	free (module);
    }
}

int DumpCVFile (LPSTR filename)
{
    FILE *debugfile;

    if (strlen (filename) == 0)
	return (-1);

    debugfile = fopen (filename, "r");
    if (debugfile == NULL)
    {
	printf ("============================================================\n");
	printf ("   ERROR: Unable to open file [%s]\n", filename);
	printf ("============================================================\n");
	return (-1);
    }

    printf ("============================================================\n");
    printf ("        Performing bindump on file %s\n", filename);
    printf ("============================================================\n\n");

    if (!DumpFileHeaders (debugfile))
    {
	printf ("============================================================\n");
	printf ("   ERROR: Bailed out while printing file headers!\n");
	printf ("============================================================\n");
	return (-1);
    }

    if (g_exe_mode)
	g_numsects = g_nthdr.FileHeader.NumberOfSections;
    else
	g_numsects = g_dbghdr.NumberOfSections;

    if (!DumpSectionHeaders (debugfile))
    {
	printf ("============================================================\n");
	printf ("   ERROR: Bailed out while printing section headers\n");
	printf ("============================================================\n");
	return (-1);
    }

    if (g_exe_mode)
	g_dbg_dircount = g_nthdr.OptionalHeader.DataDirectory[IMAGE_FILE_DEBUG_DIRECTORY].Size /
	    sizeof (IMAGE_DEBUG_DIRECTORY);
    else
	g_dbg_dircount = g_dbghdr.DebugDirectorySize / sizeof (IMAGE_DEBUG_DIRECTORY);

#ifdef VERBOSE
    printf ("\n[ Found %d debug directories in %s file. ]\n", g_dbg_dircount,
	    g_exe_mode ? "PE" : "DBG");
#endif

    if (!DumpDebugDir (debugfile))
    {
	printf ("============================================================\n");
	printf ("   ERROR: Bailed out while printing Debug Directories\n");
	printf ("============================================================\n");
	return (-1);
    }

    /* Only dump CodeView data if we know where it is!
     */
    if (g_dwStartOfCodeView == 0)
    {
	printf ("============================================================\n");
	printf ("   ERROR: Unable to find CodeView info!\n");
	printf ("============================================================\n");
	return (-1);
    }

    if (!DumpCodeViewHeaders (debugfile))
    {
	printf ("============================================================\n");
	printf ("   ERROR: Bailed out while printing CodeView headers\n");
	printf ("============================================================\n");
	return (-1);
    }

    if (!DumpAllModules (debugfile))
    {
	printf ("============================================================\n");
	printf ("   ERROR: Bailed out while printing CodeView debug info\n");
	printf ("============================================================\n");
	return (-1);
    }

    /* Clean up our trash
     */
    printf ("Shutting down...\n");

    free (g_debugdirs);
    free (g_secthdrs);

    /* FIXME: For some reason, this call segfaults...check it out later */
    /*  free (g_cvEntries); */

    /*  printf ("Freeing module data..."); */
    /*  FreeCVModules (); */

    return 0;
}

int main(int argc, char *argv[])
{
    int i;

    if (argc == 1)
    {
        printf ("Usage:\n\tcvdump FILE [FILES...]\n");
        return (-1);
    }

    for (i = 1; i < argc; i++)
       DumpCVFile (argv[i]);
    return 0;
}
