/*
 * CodeView 4 Debug format - declarations
 *
 * (based on cvinfo.h and cvexefmt.h from the Win32 SDK)
 */

#define sstModule           0x120
#define sstAlignSym         0x125
#define sstSrcModule        0x127
#define sstLibraries        0x128
#define sstGlobalSym        0x129
#define sstGlobalPub        0x12a
#define sstGlobalTypes      0x12b
#define sstSegMap           0x12d
#define sstFileIndex        0x133
#define sstStaticSym        0x134

#if 0
/* Old, crusty value */
#define S_PUB32             0x0203
#endif

#define S_PUB32             0x1009

#include "pshpack1.h"

/*
 * CodeView headers
 */

typedef struct OMFSignature 
{
    char        Signature[4];
    long        filepos;
} OMFSignature;

typedef struct OMFDirHeader 
{
    unsigned short  cbDirHeader;
    unsigned short  cbDirEntry;
    unsigned long   cDir;
    long            lfoNextDir;
    unsigned long   flags;
} OMFDirHeader;

typedef struct OMFDirEntry 
{
    unsigned short  SubSection;
    unsigned short  iMod;
    long            lfo;
    unsigned long   cb;
} OMFDirEntry;


/*
 * sstModule subsection
 */

typedef struct OMFSegDesc 
{
    unsigned short  Seg;
    unsigned short  pad;
    unsigned long   Off;
    unsigned long   cbSeg;
} OMFSegDesc;

/* Must chop off the OMFSegDesc* field, because we need to write this
 * struct out to a file.  If we write the whole struct out, we'll end up
 * with (*OMFSegDesc), not (OMFSegDesc).  See OMFModuleFull.
 */
typedef struct OMFModule 
{
    unsigned short  ovlNumber;
    unsigned short  iLib;
    unsigned short  cSeg;
    char            Style[2];
} OMFModule;

/* Full version, with memory pointers, too.  Use OMFModule for writing out to
 * a file, and OMFModuleFull for reading.  If offsetof() were available, we
 * could use that instead.
 */
typedef struct OMFModuleFull
{
    unsigned short  ovlNumber;
    unsigned short  iLib;
    unsigned short  cSeg;
    char            Style[2];
    OMFSegDesc      *SegInfo;
    char            *Name;
} OMFModuleFull;


/*
 * sstGlobalPub section
 */

/* Header for symbol table.
 */
typedef struct OMFSymHash 
{
    unsigned short  symhash;
    unsigned short  addrhash;
    unsigned long   cbSymbol;
    unsigned long   cbHSym;
    unsigned long   cbHAddr;
} OMFSymHash;

/* Symbol table entry.
 */
typedef struct DATASYM32 
{
    unsigned short  reclen;
    unsigned short  rectyp;
    unsigned long   typind;
    unsigned long   off;
    unsigned short  seg;
} DATASYM32;
typedef DATASYM32 PUBSYM32;

/*
 * sstSegMap section
 */

typedef struct OMFSegMapDesc
{
    unsigned short  flags;
    unsigned short  ovl;
    unsigned short  group;
    unsigned short  frame;
    unsigned short  iSegName;
    unsigned short  iClassName;
    unsigned long   offset;
    unsigned long   cbSeg;
} OMFSegMapDesc;

typedef struct OMFSegMap
{
    unsigned short  cSeg;
    unsigned short  cSegLog;
    OMFSegMapDesc   rgDesc[0];
} OMFSegMap;


/*
 * sstSrcModule section
 */

typedef struct OMFSourceLine
{
    unsigned short  Seg;
    unsigned short  cLnOff;
    unsigned long   offset[1];
    unsigned short  lineNbr[1];
} OMFSourceLine;

typedef struct OMFSourceFile
{
    unsigned short  cSeg;
    unsigned short  reserved;
    unsigned long   baseSrcLn[1];
    unsigned short  cFName;
    char            Name;
} OMFSourceFile;

typedef struct OMFSourceModule
{
    unsigned short  cFile;
    unsigned short  cSeg;
    unsigned long   baseSrcFile[1];
} OMFSourceModule;

