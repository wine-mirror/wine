#include "wine/wingdi16.h"

#include "pshpack1.h"

enum data_types {dfChar, dfShort, dfLong, dfString};

#define ERROR_DATA	1
#define ERROR_VERSION	2
#define ERROR_SIZE	3
#define ERROR_MEMORY	4
#define ERROR_FILE	5

typedef struct tagFontHeader 
{
    short dfVersion;		/* Version */
    long dfSize;		/* Total File Size */
    char dfCopyright[60];	/* Copyright notice */
    FONTINFO16 fi;		/* FONTINFO structure */
} fnt_hdrS;

typedef struct WinCharStruct
{
    unsigned int charWidth;
    long charOffset;
} WinCharS;
 
typedef struct fntFontStruct
{
    fnt_hdrS 	 	hdr;
    WinCharS 		*dfCharTable;
    unsigned char	*dfDeviceP;
    unsigned char 	*dfFaceP;
    unsigned char 	*dfBitsPointerP;
    unsigned char 	*dfBitsOffsetP;
    short 		*dfColorTableP;
} fnt_fontS;

#include "poppack.h"
