/*
 * Made after:
 *	CVDump - Parses through a Visual Studio .DBG file in CodeView 4 format
 * 	and dumps the info to STDOUT in a human-readable format
 *
 * 	Copyright 2000 John R. Sheets
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

#include "config.h"

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <time.h>

#include "../tools.h"
#include "windef.h"
#include "winbase.h"
#include "winedump.h"

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
 *        to the .DBG file... not the binary data it points to.
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

extern const IMAGE_NT_HEADERS*  PE_nt_headers;
static const void*	        cv_base /* = 0 */;

static BOOL dump_cv_sst_module(const OMFDirEntry* omfde)
{
    const OMFModule*	module;
    const OMFSegDesc*	segDesc;
    int		i;

    module = PRD(Offset(cv_base) + omfde->lfo, sizeof(OMFModule));
    if (!module) {printf("Can't get the OMF-Module, aborting\n"); return FALSE;}

    printf("    olvNumber:          %u\n", module->ovlNumber);
    printf("    iLib:               %u\n", module->iLib);
    printf("    cSeg:               %u\n", module->cSeg);
    printf("    Style:              %c%c\n", module->Style[0], module->Style[1]);
    printf("    Name:               %.*s\n",
	   *(const BYTE*)((const char*)(module + 1) + sizeof(OMFSegDesc) * module->cSeg),
	   (const char*)(module + 1) + sizeof(OMFSegDesc) * module->cSeg + 1);

    segDesc = PRD(Offset(module + 1), sizeof(OMFSegDesc) * module->cSeg);
    if (!segDesc) {printf("Can't get the OMF-SegDesc, aborting\n"); return FALSE;}

    for (i = 0; i < module->cSeg; i++)
    {
	printf ("      segment #%2d: offset = [0x%8x], size = [0x%8x]\n",
		segDesc->Seg, segDesc->Off, segDesc->cbSeg);
	segDesc++;
    }
    return TRUE;
}

static BOOL dump_cv_sst_global_pub(const OMFDirEntry* omfde)
{
    long	        fileoffset;
    const OMFSymHash*   header;
    const BYTE*	        symbols;

    fileoffset = Offset(cv_base) + omfde->lfo;
    printf ("    GlobalPub section starts at file offset 0x%lx\n", fileoffset);
    printf ("    Symbol table starts at 0x%lx\n", fileoffset + sizeof (OMFSymHash));

    printf ("\n                           ----- Begin Symbol Table -----\n");

    header = PRD(fileoffset, sizeof(OMFSymHash));
    if (!header) {printf("Can't get OMF-SymHash, aborting\n");return FALSE;}

    symbols = PRD(fileoffset + sizeof(OMFSymHash), header->cbSymbol);
    if (!symbols) {printf("Can't OMF-SymHash details, aborting\n"); return FALSE;}

    codeview_dump_symbols(symbols, 0, header->cbSymbol);

    return TRUE;
}

static BOOL dump_cv_sst_global_sym(const OMFDirEntry* omfde)
{
    /*** NOT YET IMPLEMENTED ***/
    return TRUE;
}

static BOOL dump_cv_sst_static_sym(const OMFDirEntry* omfde)
{
    /*** NOT YET IMPLEMENTED ***/
    return TRUE;
}

static BOOL dump_cv_sst_libraries(const OMFDirEntry* omfde)
{
    /*** NOT YET IMPLEMENTED ***/
    return TRUE;
}

static BOOL dump_cv_sst_global_types(const OMFDirEntry* omfde)
{
    long	        fileoffset;
    const OMFGlobalTypes*types;
    const BYTE*         data;
    unsigned            sz;

    fileoffset = Offset(cv_base) + omfde->lfo;
    printf ("    GlobalTypes section starts at file offset 0x%lx\n", fileoffset);

    printf ("\n                           ----- Begin Global Types Table -----\n");

    types = PRD(fileoffset, sizeof(OMFGlobalTypes));
    if (!types) {printf("Can't get OMF-GlobalTypes, aborting\n");return FALSE;}

    sz = omfde->cb - sizeof(OMFGlobalTypes) - sizeof(DWORD) * types->cTypes;
    data = PRD(fileoffset + sizeof(OMFGlobalTypes) + sizeof(DWORD) * types->cTypes, sz);
    if (!data) {printf("Can't OMF-SymHash details, aborting\n"); return FALSE;}

    /* doc says:
     * - for NB07 & NB08 (that we don't support yet), offsets are from types
     * - for NB09, offsets are from data
     * For now, we only support the latter
     */
    codeview_dump_types_from_offsets(data, (const DWORD*)(types + 1), types->cTypes);

    return TRUE;
}

static BOOL dump_cv_sst_seg_map(const OMFDirEntry* omfde)
{
    const OMFSegMap*		segMap;
    const OMFSegMapDesc*	segMapDesc;
    int		i;

    segMap = PRD(Offset(cv_base) + omfde->lfo, sizeof(OMFSegMap));
    if (!segMap) {printf("Can't get SegMap, aborting\n");return FALSE;}

    printf("    cSeg:          %u\n", segMap->cSeg);
    printf("    cSegLog:       %u\n", segMap->cSegLog);

    segMapDesc = PRD(Offset(segMap + 1), segMap->cSeg * sizeof(OMFSegDesc));
    if (!segMapDesc) {printf("Can't get SegDescr array, aborting\n");return FALSE;}

    for (i = 0; i < segMap->cSeg; i++)
    {
	printf("    SegDescr #%2d\n", i + 1);
	printf("      flags:         %04X\n", segMapDesc[i].flags);
	printf("      ovl:           %u\n", segMapDesc[i].ovl);
	printf("      group:         %u\n", segMapDesc[i].group);
	printf("      frame:         %u\n", segMapDesc[i].frame);
	printf("      iSegName:      %u\n", segMapDesc[i].iSegName);
	printf("      iClassName:    %u\n", segMapDesc[i].iClassName);
	printf("      offset:        %u\n", segMapDesc[i].offset);
	printf("      cbSeg:         %u\n", segMapDesc[i].cbSeg);
    }

    return TRUE;
}

static BOOL dump_cv_sst_file_index(const OMFDirEntry* omfde)
{
    /*** NOT YET IMPLEMENTED ***/
    return TRUE;
}

static BOOL dump_cv_sst_src_module(const OMFDirEntry* omfde)
{
    int 		        i, j;
    const BYTE*		        rawdata;
    const unsigned int *seg_info_dw;
    const unsigned short*	seg_info_w;
    unsigned		        ofs;
    const OMFSourceModule*	sourceModule;
    const OMFSourceFile*	sourceFile;
    const OMFSourceLine*	sourceLine;

    rawdata = PRD(Offset(cv_base) + omfde->lfo, omfde->cb);
    if (!rawdata) {printf("Can't get srcModule subsection details, aborting\n");return FALSE;}

    /* FIXME: check ptr validity */
    sourceModule = (const void*)rawdata;
    printf ("    Module table: Found %d file(s) and %d segment(s)\n",
	    sourceModule->cFile, sourceModule->cSeg);
    for (i = 0; i < sourceModule->cFile; i++)
    {
	printf ("      File #%2d begins at an offset of 0x%x in this section\n",
		i + 1, sourceModule->baseSrcFile[i]);
    }

    /* FIXME: check ptr validity */
    seg_info_dw = (const void*)((const char*)(sourceModule + 1) +
			  sizeof(unsigned int) * (sourceModule->cFile - 1));
    seg_info_w = (const unsigned short*)(&seg_info_dw[sourceModule->cSeg * 2]);
    for (i = 0; i <  sourceModule->cSeg; i++)
    {
	printf ("      Segment #%2d start = 0x%x, end = 0x%x, seg index = %u\n",
		i + 1, seg_info_dw[i * 2], seg_info_dw[(i * 2) + 1],
		seg_info_w[i]);
    }
    ofs = sizeof(OMFSourceModule) + sizeof(unsigned int) * (sourceModule->cFile - 1) +
	sourceModule->cSeg * (2 * sizeof(unsigned int) + sizeof(unsigned short));
    ofs = (ofs + 3) & ~3;

    /* the OMFSourceFile is quite unpleasant to use:
     * we have first:
     * 	unsigned short	number of segments
     *	unsigned short	reserved
     *	unsigned int	baseSrcLn[# segments]
     *  unsigned int	offset[2 * #segments]
     *				odd indices are start offsets
     *				even indices are end offsets
     * 	unsigned char	string length for file name
     *	char		file name (length is previous field)
     */
    /* FIXME: check ptr validity */
    sourceFile = (const void*)(rawdata + ofs);
    seg_info_dw = (const void*)((const char*)sourceFile + 2 * sizeof(unsigned short) +
			  sourceFile->cSeg * sizeof(unsigned int));

    ofs += 2 * sizeof(unsigned short) + 3 * sourceFile->cSeg * sizeof(unsigned int);

    printf("    File table: %.*s\n",
	   *(const BYTE*)((const char*)sourceModule + ofs), (const char*)sourceModule + ofs + 1);

    for (i = 0; i < sourceFile->cSeg; i++)
    {
	printf ("      Segment #%2d start = 0x%x, end = 0x%x, offset = 0x%x\n",
		i + 1, seg_info_dw[i * 2], seg_info_dw[(i * 2) + 1], sourceFile->baseSrcLn[i]);
    }
    /* add file name length */
    ofs += *(const BYTE*)((const char*)sourceModule + ofs) + 1;
    ofs = (ofs + 3) & ~3;

    for (i = 0; i < sourceModule->cSeg; i++)
    {
        sourceLine = (const void*)(rawdata + ofs);
        seg_info_dw = (const void*)((const char*)sourceLine + 2 * sizeof(unsigned short));
        seg_info_w = (const void*)(&seg_info_dw[sourceLine->cLnOff]);

	printf ("    Line table #%2d: Found %d line numbers for segment index %d\n",
		i, sourceLine->cLnOff, sourceLine->Seg);

	for (j = 0; j < sourceLine->cLnOff; j++)
	{
	    printf ("      Pair #%2d: offset = [0x%8x], linenumber = %d\n",
		    j + 1, seg_info_dw[j], seg_info_w[j]);
	}
	ofs += 2 * sizeof(unsigned short) +
	    sourceLine->cLnOff * (sizeof(unsigned int) + sizeof(unsigned short));
	ofs = (ofs + 3) & ~3;
    }

    return TRUE;
}

static BOOL dump_cv_sst_align_sym(const OMFDirEntry* omfde)
{
    const char* rawdata = PRD(Offset(cv_base) + omfde->lfo, omfde->cb);

    if (!rawdata) {printf("Can't get srcAlignSym subsection details, aborting\n");return FALSE;}
    if (omfde->cb < sizeof(DWORD)) return TRUE;
    codeview_dump_symbols(rawdata, sizeof(DWORD), omfde->cb);

    return TRUE;
}

static void dump_codeview_all_modules(const OMFDirHeader *omfdh)
{
    unsigned 		i;
    const OMFDirEntry*  dirEntry;
    const char*		str;

    if (!omfdh || !omfdh->cDir) return;

    dirEntry = PRD(Offset(omfdh + 1), omfdh->cDir * sizeof(OMFDirEntry));
    if (!dirEntry) {printf("Can't read DirEntry array, aborting\n"); return;}

    for (i = 0; i < omfdh->cDir; i++)
    {
	switch (dirEntry[i].SubSection)
	{
	case sstModule:		str = "sstModule";	break;
	case sstAlignSym:	str = "sstAlignSym";	break;
	case sstSrcModule:	str = "sstSrcModule";	break;
	case sstLibraries:	str = "sstLibraries";	break;
	case sstGlobalSym:	str = "sstGlobalSym";	break;
	case sstGlobalPub:	str = "sstGlobalPub";	break;
	case sstGlobalTypes:	str = "sstGlobalTypes";	break;
	case sstSegMap:		str = "sstSegMap";	break;
	case sstFileIndex:	str = "sstFileIndex";	break;
	case sstStaticSym:	str = "sstStaticSym";	break;
	default:		str = "<undefined>";	break;
	}
	printf("Module #%2d (%p)\n", i + 1, &dirEntry[i]);
	printf("  SubSection:            %04X (%s)\n", dirEntry[i].SubSection, str);
	printf("  iMod:                  %d\n", dirEntry[i].iMod);
	printf("  lfo:                   %d\n", dirEntry[i].lfo);
	printf("  cb:                    %u\n", dirEntry[i].cb);

	switch (dirEntry[i].SubSection)
	{
	case sstModule:		dump_cv_sst_module(&dirEntry[i]);	break;
	case sstAlignSym:	dump_cv_sst_align_sym(&dirEntry[i]);	break;
	case sstSrcModule:	dump_cv_sst_src_module(&dirEntry[i]);	break;
	case sstLibraries:	dump_cv_sst_libraries(&dirEntry[i]);	break;
	case sstGlobalSym:	dump_cv_sst_global_sym(&dirEntry[i]);	break;
	case sstGlobalPub:	dump_cv_sst_global_pub(&dirEntry[i]);	break;
	case sstGlobalTypes:	dump_cv_sst_global_types(&dirEntry[i]);	break;
	case sstSegMap:		dump_cv_sst_seg_map(&dirEntry[i]);	break;
	case sstFileIndex:	dump_cv_sst_file_index(&dirEntry[i]);	break;
	case sstStaticSym:	dump_cv_sst_static_sym(&dirEntry[i]);	break;
	default:		printf("unsupported type %x\n", dirEntry[i].SubSection); break;
	}
	printf("\n");
    }

    return;
}

static void dump_codeview_headers(unsigned long base, unsigned long len)
{
    const OMFDirHeader* dirHeader;
    const char*         signature;
    const OMFDirEntry*  dirEntry;
    const OMFSignature* sig;
    unsigned		i;
    int modulecount = 0, alignsymcount = 0, srcmodulecount = 0, librariescount = 0;
    int globalsymcount = 0, globalpubcount = 0, globaltypescount = 0;
    int segmapcount = 0, fileindexcount = 0, staticsymcount = 0;

    cv_base = PRD(base, len);
    if (!cv_base) {printf("Can't get full debug content, aborting\n");return;}

    signature = cv_base;

    printf("    CodeView Data\n");
    printf("      Signature:         %.4s\n", signature);

    if (memcmp(signature, "NB10", 4) == 0)
    {
	const CODEVIEW_PDB_DATA* pdb_data;
	pdb_data = cv_base;

        printf("      Filepos:           0x%08X\n", pdb_data->filepos);
	printf("      TimeStamp:         %08X (%s)\n",
	       pdb_data->timestamp, get_time_str(pdb_data->timestamp));
	printf("      Age:               %08X\n", pdb_data->age);
	printf("      Filename:          %s\n", pdb_data->name);
	return;
    }
    if (memcmp(signature, "RSDS", 4) == 0)
    {
	const OMFSignatureRSDS* rsds_data;

	rsds_data = cv_base;
	printf("      Guid:              %s\n", get_guid_str(&rsds_data->guid));
	printf("      Age:               %08X\n", rsds_data->age);
	printf("      Filename:          %s\n", rsds_data->name);
	return;
    }

    if (memcmp(signature, "NB09", 4) != 0 && memcmp(signature, "NB11", 4) != 0)
    {
	printf("Unsupported signature (%.4s), aborting\n", signature);
	return;
    }

    sig = cv_base;

    printf("      Filepos:           0x%08X\n", sig->filepos);

    dirHeader = PRD(Offset(cv_base) + sig->filepos, sizeof(OMFDirHeader));
    if (!dirHeader) {printf("Can't get debug header, aborting\n"); return;}

    printf("      Size of header:    0x%4X\n", dirHeader->cbDirHeader);
    printf("      Size per entry:    0x%4X\n", dirHeader->cbDirEntry);
    printf("      # of entries:      0x%8X (%d)\n", dirHeader->cDir, dirHeader->cDir);
    printf("      Offset to NextDir: 0x%8X\n", dirHeader->lfoNextDir);
    printf("      Flags:             0x%8X\n", dirHeader->flags);

    if (!dirHeader->cDir) return;

    dirEntry = PRD(Offset(dirHeader + 1), sizeof(OMFDirEntry) * dirHeader->cDir);
    if (!dirEntry) {printf("Can't get DirEntry array, aborting\n");return;}

    for (i = 0; i < dirHeader->cDir; i++)
    {
	switch (dirEntry[i].SubSection)
	{
	case sstModule:		modulecount++;		break;
	case sstAlignSym:	alignsymcount++;	break;
	case sstSrcModule:	srcmodulecount++;	break;
	case sstLibraries:	librariescount++;	break;
	case sstGlobalSym:	globalsymcount++;	break;
	case sstGlobalPub:	globalpubcount++;	break;
	case sstGlobalTypes:	globaltypescount++;	break;
	case sstSegMap:		segmapcount++;		break;
	case sstFileIndex:	fileindexcount++;	break;
	case sstStaticSym:	staticsymcount++;	break;
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

    dump_codeview_all_modules(dirHeader);
}

static const char *get_coff_name( const IMAGE_SYMBOL *coff_sym, const char *coff_strtab )
{
   static       char    namebuff[9];
   const char*          nampnt;

   if( coff_sym->N.Name.Short )
      {
         memcpy(namebuff, coff_sym->N.ShortName, 8);
         namebuff[8] = '\0';
         nampnt = namebuff;
      }
   else
      {
         nampnt = coff_strtab + coff_sym->N.Name.Long;
      }

   if( nampnt[0] == '_' )
      nampnt++;
   return nampnt;
}

static const char* storage_class(BYTE sc)
{
    static char tmp[7];
    switch (sc)
    {
    case IMAGE_SYM_CLASS_STATIC:        return "static";
    case IMAGE_SYM_CLASS_EXTERNAL:      return "extrnl";
    case IMAGE_SYM_CLASS_LABEL:         return "label ";
    }
    sprintf(tmp, "#%d", sc);
    return tmp;
}

void    dump_coff_symbol_table(const IMAGE_SYMBOL *coff_symbols, unsigned num_sym,
                               const IMAGE_SECTION_HEADER *sectHead)
{
    const IMAGE_SYMBOL              *coff_sym;
    const char                      *coff_strtab = (const char *) (coff_symbols + num_sym);
    unsigned int                    i;
    const char                      *nampnt;
    int                             naux;

    printf("\nDebug table: COFF format.\n");
    printf("  ID  | seg:offs    [  abs   ] |  Kind  | symbol/function name\n");
    for (i=0; i < num_sym; i++)
    {
        coff_sym = coff_symbols + i;
        naux = coff_sym->NumberOfAuxSymbols;

        switch (coff_sym->StorageClass)
        {
        case IMAGE_SYM_CLASS_FILE:
            printf("file %s\n", (const char *) (coff_sym + 1));
            break;
        case IMAGE_SYM_CLASS_STATIC:
        case IMAGE_SYM_CLASS_EXTERNAL:
        case IMAGE_SYM_CLASS_LABEL:
            if (coff_sym->SectionNumber > 0)
            {
                UINT base = sectHead[coff_sym->SectionNumber - 1].VirtualAddress;
                nampnt = get_coff_name( coff_sym, coff_strtab );

                printf("%05d | %02d:%08x [%08x] | %s | %s\n",
                       i, coff_sym->SectionNumber - 1, (UINT)coff_sym->Value,
                       base + (UINT)coff_sym->Value,
                       storage_class(coff_sym->StorageClass), nampnt);
            }
            break;
        default:
            printf("%05d | %s\n", i, storage_class(coff_sym->StorageClass));
        }
        /*
         * For now, skip past the aux entries.
         */
        i += naux;
    }
}

void	dump_coff(unsigned long coffbase, unsigned long len, const IMAGE_SECTION_HEADER* sectHead)
{
    const IMAGE_COFF_SYMBOLS_HEADER *coff = PRD(coffbase, len);
    const IMAGE_SYMBOL              *coff_symbols =
                                        (const IMAGE_SYMBOL *) ((const char *)coff + coff->LvaToFirstSymbol);

    dump_coff_symbol_table(coff_symbols, coff->NumberOfSymbols, sectHead);
}

void	dump_codeview(unsigned long base, unsigned long len)
{
    dump_codeview_headers(base, len);
}

void	dump_frame_pointer_omission(unsigned long base, unsigned long len)
{
    const FPO_DATA*     fpo;
    const FPO_DATA*     last;
    const char*         x;
    /* FPO is used to describe nonstandard stack frames */
    printf("Range             #loc #pmt Prlg #reg Info\n"
           "-----------------+----+----+----+----+------------\n");

    fpo = PRD(base, len);
    if (!fpo) {printf("Couldn't get FPO blob\n"); return;}
    last = (const FPO_DATA*)((const char*)fpo + len);

    while (fpo < last && fpo->ulOffStart)
    {
        switch (fpo->cbFrame)
        {
        case FRAME_FPO: x = "FRAME_FPO"; break;
        case FRAME_NONFPO: x = "FRAME_NONFPO"; break;
        case FRAME_TRAP: x = "FRAME_TRAP"; break;
        case FRAME_TSS: x = "case FRAME_TSS"; break;
        default: x = NULL; break;
        }
        printf("%08x-%08x %4u %4u %4u %4u %s%s%s\n",
               (UINT)fpo->ulOffStart, (UINT)(fpo->ulOffStart + fpo->cbProcSize),
               (UINT)fpo->cdwLocals, fpo->cdwParams, fpo->cbProlog, fpo->cbRegs,
               x, fpo->fHasSEH ? " SEH" : "", fpo->fUseBP ? " UseBP" : "");
        fpo++;
    }
}

struct stab_nlist
{
    union
    {
        char*                   n_name;
        struct stab_nlist*      n_next;
        int                     n_strx;
    } n_un;
    unsigned char       n_type;
    char                n_other;
    short               n_desc;
    unsigned int        n_value;
};

static const char * const stabs_defs[] = {
    NULL,NULL,NULL,NULL,                /* 00 */
    NULL,NULL,NULL,NULL,                /* 08 */
    NULL,NULL,NULL,NULL,                /* 10 */
    NULL,NULL,NULL,NULL,                /* 18 */
    "GSYM","FNAME","FUN","STSYM",       /* 20 */
    "LCSYM","MAIN","ROSYM","PC",        /* 28 */
    NULL,"NSYMS","NOMAP",NULL,          /* 30 */
    "OBJ",NULL,"OPT",NULL,              /* 38 */
    "RSYM","M2C","SLINE","DSLINE",      /* 40 */
    "BSLINE","DEFD","FLINE",NULL,       /* 48 */
    "EHDECL",NULL,"CATCH",NULL,         /* 50 */
    NULL,NULL,NULL,NULL,                /* 58 */
    "SSYM","ENDM","SO",NULL,            /* 60 */
    NULL,NULL,NULL,NULL,                /* 68 */
    NULL,NULL,NULL,NULL,                /* 70 */
    NULL,NULL,NULL,NULL,                /* 78 */
    "LSYM","BINCL","SOL",NULL,          /* 80 */
    NULL,NULL,NULL,NULL,                /* 88 */
    NULL,NULL,NULL,NULL,                /* 90 */
    NULL,NULL,NULL,NULL,                /* 98 */
    "PSYM","EINCL","ENTRY",NULL,        /* a0 */
    NULL,NULL,NULL,NULL,                /* a8 */
    NULL,NULL,NULL,NULL,                /* b0 */
    NULL,NULL,NULL,NULL,                /* b8 */
    "LBRAC","EXCL","SCOPE",NULL,        /* c0 */
    NULL,NULL,NULL,NULL,                /* c8 */
    NULL,NULL,NULL,NULL,                /* d0 */
    NULL,NULL,NULL,NULL,                /* d8 */
    "RBRAC","BCOMM","ECOMM",NULL,       /* e0 */
    "ECOML","WITH",NULL,NULL,           /* e8 */
    "NBTEXT","NBDATA","NBBSS","NBSTS",  /* f0 */
    "NBLCS",NULL,NULL,NULL              /* f8 */
};

void    dump_stabs(const void* pv_stabs, unsigned szstabs, const char* stabstr, unsigned szstr)
{
    int                         i;
    int                         nstab;
    const char*                 ptr;
    char*                       stabbuff;
    unsigned int                stabbufflen;
    const struct stab_nlist*    stab_ptr = pv_stabs;
    const char*                 strs_end;
    char                        n_buffer[16];

    nstab = szstabs / sizeof(struct stab_nlist);
    strs_end = stabstr + szstr;

    /*
     * Allocate a buffer into which we can build stab strings for cases
     * where the stab is continued over multiple lines.
     */
    stabbufflen = 65536;
    stabbuff = xmalloc(stabbufflen);

    stabbuff[0] = '\0';

    printf("#Sym n_type n_othr   n_desc n_value  n_strx String\n");

    for (i = 0; i < nstab; i++, stab_ptr++)
    {
        ptr = stabstr + stab_ptr->n_un.n_strx;
        if ((ptr > strs_end) || (ptr + strlen(ptr) > strs_end))
        {
            ptr = "[[*** bad string ***]]";
        }
        else if (ptr[strlen(ptr) - 1] == '\\')
        {
            /*
             * Indicates continuation.  Append this to the buffer, and go onto the
             * next record.  Repeat the process until we find a stab without the
             * '/' character, as this indicates we have the whole thing.
             */
            unsigned    len = strlen(ptr);
            if (strlen(stabbuff) + len > stabbufflen)
            {
                stabbufflen += 65536;
                stabbuff = xrealloc(stabbuff, stabbufflen);
            }
            strcat(stabbuff, ptr);
            continue;
        }
        else if (stabbuff[0] != '\0')
        {
            strcat(stabbuff, ptr);
            ptr = stabbuff;
        }
        if ((stab_ptr->n_type & 1) || !stabs_defs[stab_ptr->n_type / 2])
            sprintf(n_buffer, "<0x%02x>", stab_ptr->n_type);
        else
            sprintf(n_buffer, "%-6s", stabs_defs[stab_ptr->n_type / 2]);
        printf("%4d %s %-8x % 6d %-8x %-6x %s\n",
               i, n_buffer, stab_ptr->n_other, stab_ptr->n_desc, stab_ptr->n_value,
               stab_ptr->n_un.n_strx, ptr);
    }
    free(stabbuff);
}
