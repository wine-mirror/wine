#include <stdio.h>

enum data_types {dfChar, dfShort, dfLong, dfString};

#define ERROR_DATA	1
#define ERROR_VERSION	2
#define ERROR_SIZE	3
#define ERROR_MEMORY	4
#define ERROR_FILE	5

typedef struct tagFontHeader 
{
    unsigned char dfVersion[2]; 	/* Version (always 0x3000) */ 
    unsigned char dfSize[4];            /* Total File Size */
    unsigned char dfCopyright[60];      /* Copyright notice */
    unsigned char dfType[2];            /* Vector or bitmap font */
    unsigned char dfPoints[2];          /* Nominal point size */
    unsigned char dfVertRes[2]; 	/* Vertical Resolution */
    unsigned char dfHorizRes[2];        /* Horizontal Resolutionchar */  
    unsigned char dfAscent[2];          /* Character ascent in pixels */ 
    unsigned char dfInternalLeading[2]; /* Leading included in character defn */
    unsigned char dfExternalLeading[2]; /* Leading to be added by Windows */
    unsigned char dfItalic[1];             /* 1=Italic font */
    unsigned char dfUnderline[1];          /* 1=underlined font */
    unsigned char dfStrikeOut[1];          /* 1=strike-out font */
    unsigned char dfWeight[2];          /* Weight: 400=normal */
    unsigned char dfCharSet[1];            /* Character Set for this font */
    unsigned char dfPixWidth[2];        /* Character width (0 for proportional) */
    unsigned char dfPixHeight[2];       /* Character height */
    unsigned char dfPitchAndFamily[1];     /* Font Pitch and family */
    unsigned char dfAvgWidth[2];        /* Average character width */
    unsigned char dfMaxWidth[2];        /* Maximum character width */
    unsigned char dfFirstChar[1];          /* Firwst character of the font */
    unsigned char dfLastChar[1];           /* Last character of the font */
    unsigned char dfDefaultChar[1];        /* Missing character */
    unsigned char dfBreakChar[1];          /* Character to indicate word breaks */
    unsigned char dfWidthBytes[2];      /* Number of bytes in each row */
    unsigned char dfDevice[4];          /* Offset to device name */
    unsigned char dfFace[4];            /* Offset to type face name */
    unsigned char dfBitsPointer[4];
    unsigned char dfBitsOffset[4];      /* Offset to bitmaps */
    unsigned char dfReserved[1];
    unsigned char dfFlags[4];           /* Bitmapped flags */
    unsigned char dfAspace[2];
    unsigned char dfBspace[2];
    unsigned char dfCspace[2];
    unsigned char dfColorTable[2];      /* Offset to Color table */
    unsigned char dfReserved1[4];
} fnt_hdrS;

typedef struct WinCharStruct
{
    unsigned int charWidth;
    unsigned int charOffset;
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

extern	int return_data_value(enum data_types, void *);
 
extern	int dump_bdf(fnt_fontS*, unsigned char* );
extern	int dump_bdf_hdr(FILE* fp,fnt_fontS*, unsigned char* );

extern	int parse_fnt_data(unsigned char* file_buffer, int length);

