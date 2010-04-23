/************************************************
 *
 * Extract fonts from .fnt or Windows DLL files
 * and convert them to the .bdf format.
 *
 * Copyright 1994-1996 Kevin Carothers and Alex Korobka
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
#include "wine/port.h"

#ifdef HAVE_SYS_PARAM_H
# include <sys/param.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <fcntl.h>
#ifdef HAVE_IO_H
# include <io.h>
#endif

#include "windef.h"
#include "wingdi.h"

enum data_types {dfChar, dfShort, dfLong, dfString};

#define ERROR_DATA	1
#define ERROR_VERSION	2
#define ERROR_SIZE	3
#define ERROR_MEMORY	4
#define ERROR_FILE	5

#include "pshpack1.h"

typedef struct
{
    INT16 dfType;
    INT16 dfPoints;
    INT16 dfVertRes;
    INT16 dfHorizRes;
    INT16 dfAscent;
    INT16 dfInternalLeading;
    INT16 dfExternalLeading;
    BYTE  dfItalic;
    BYTE  dfUnderline;
    BYTE  dfStrikeOut;
    INT16 dfWeight;
    BYTE  dfCharSet;
    INT16 dfPixWidth;
    INT16 dfPixHeight;
    BYTE  dfPitchAndFamily;
    INT16 dfAvgWidth;
    INT16 dfMaxWidth;
    BYTE  dfFirstChar;
    BYTE  dfLastChar;
    BYTE  dfDefaultChar;
    BYTE  dfBreakChar;
    INT16 dfWidthBytes;
    LONG  dfDevice;
    LONG  dfFace;
    LONG  dfBitsPointer;
    LONG  dfBitsOffset;
    BYTE  dfReserved;
    LONG  dfFlags;
    INT16 dfAspace;
    INT16 dfBspace;
    INT16 dfCspace;
    LONG  dfColorPointer;
    LONG  dfReserved1[4];
} FONTINFO16;

typedef struct
{
    WORD  offset;
    WORD  length;
    WORD  flags;
    WORD  id;
    WORD  handle;
    WORD  usage;
} NE_NAMEINFO;

typedef struct
{
    WORD  type_id;
    WORD  count;
    DWORD resloader;
} NE_TYPEINFO;

#define NE_FFLAGS_SINGLEDATA    0x0001
#define NE_FFLAGS_MULTIPLEDATA  0x0002
#define NE_FFLAGS_WIN32         0x0010
#define NE_FFLAGS_FRAMEBUF      0x0100
#define NE_FFLAGS_CONSOLE       0x0200
#define NE_FFLAGS_GUI           0x0300
#define NE_FFLAGS_SELFLOAD      0x0800
#define NE_FFLAGS_LINKERROR     0x2000
#define NE_FFLAGS_CALLWEP       0x4000
#define NE_FFLAGS_LIBMODULE     0x8000

#define NE_OSFLAGS_WINDOWS      0x02

#define NE_RSCTYPE_FONTDIR            0x8007
#define NE_RSCTYPE_FONT               0x8008
#define NE_RSCTYPE_SCALABLE_FONTPATH  0x80cc

#define NE_SEGFLAGS_DATA        0x0001
#define NE_SEGFLAGS_ALLOCATED   0x0002
#define NE_SEGFLAGS_LOADED      0x0004
#define NE_SEGFLAGS_ITERATED    0x0008
#define NE_SEGFLAGS_MOVEABLE    0x0010
#define NE_SEGFLAGS_SHAREABLE   0x0020
#define NE_SEGFLAGS_PRELOAD     0x0040
#define NE_SEGFLAGS_EXECUTEONLY 0x0080
#define NE_SEGFLAGS_READONLY    0x0080
#define NE_SEGFLAGS_RELOC_DATA  0x0100
#define NE_SEGFLAGS_SELFLOAD    0x0800
#define NE_SEGFLAGS_DISCARDABLE 0x1000
#define NE_SEGFLAGS_32BIT       0x2000

typedef struct tagFontHeader
{
    SHORT dfVersion;		/* Version */
    LONG dfSize;		/* Total File Size */
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

#define FILE_ERROR	0
#define FILE_DLL	1
#define FILE_FNT	2

/* global options */

static int dump_bdf( fnt_fontS* cpe_font_struct, unsigned char* file_buffer);
static int dump_bdf_hdr(FILE* fs, fnt_fontS* cpe_font_struct, unsigned char* file_buffer);

static char* g_lpstrFileName = NULL;
static char* g_lpstrCharSet = NULL;
static char* g_lpstrInputFile = NULL;
static int   g_outputPoints = 0;

/* info */

static void usage(void)
{
    printf("Usage: fnt2bdf [-t] [-c charset] [-o basename] [input file]\n");
    printf("  -c charset\tcharset name for OEM_CHARSET fonts\n");
    printf("  -f basename\tbasic output filename\n");
    printf("  -t \t\toutput files by point size instead of pixel height\n");
    printf("  input file\tMSWindows .fon, .fnt, .dll, or .exe file.\n");
    printf("\nExample:\n  fnt2bdf -c winsys vgasys.fnt\n\n");
    exit(-1);
}

/* convert little-endian value to the local format */

static int return_data_value(enum data_types dtype, void * pChr)
{
int   ret_val = 0;

    switch(dtype) {
        case (dfChar):
            ret_val = (int) *(unsigned char *)pChr;
            break;

        case(dfShort):
            ret_val = *(unsigned char *)pChr;
            ret_val += (*((unsigned char *)pChr + 1) << 8);
            break;

        case(dfLong): {
            int  i;

            for(i=3; i >= 0; i--)  {
                ret_val += *((unsigned char *)pChr + i) << (8*i);
                }
            break;
            }
		case(dfString):
			break;
        }
    return ret_val;
}

static int make_bdf_filename(char* name, fnt_fontS* cpe_font_struct, unsigned char* file_buffer)
{
long	l_nameoffset = return_data_value(dfLong, &cpe_font_struct->hdr.fi.dfFace);
char*   lpChar;

  if( !g_lpstrFileName )
  {
    if( !l_nameoffset ||
         l_nameoffset > return_data_value(dfLong, &cpe_font_struct->hdr.dfSize) + 1 )
         return ERROR_DATA;
    lpChar =  (char*)(file_buffer + l_nameoffset);
  }
    else lpChar = g_lpstrFileName;

  strcpy( name, lpChar );

  while( (lpChar = strchr( name, ' ')) )
         *lpChar = '-';

  /* construct a filename from the font typeface, slant, weight, and size */

  if( cpe_font_struct->hdr.fi.dfItalic ) strcat(name, "_i" );
  else strcat(name, "_r" );

  lpChar = name + strlen( name );
  sprintf(lpChar, "%d-%d.bdf", return_data_value(dfShort, &cpe_font_struct->hdr.fi.dfWeight),
          (g_outputPoints) ? return_data_value(dfShort, &cpe_font_struct->hdr.fi.dfPoints)
			   : return_data_value(dfShort, &cpe_font_struct->hdr.fi.dfPixHeight) );
  return 0;
}

/* parse FONT resource and write .bdf file */

static int parse_fnt_data(unsigned char* file_buffer, int length)
{
  fnt_fontS	cpe_font_struct;
  int     	ic=0, t;

  memcpy((char *) &cpe_font_struct.hdr, file_buffer, sizeof(fnt_hdrS));

  /* check font header */

  t = return_data_value(dfShort, &cpe_font_struct.hdr.dfVersion);
  if( t != 0x300 && t != 0x200) return ERROR_VERSION;

  t = return_data_value(dfShort, &cpe_font_struct.hdr.fi.dfType);
  if (t & 1)
  {
    fprintf(stderr, "Vector fonts not supported\n");
    return ERROR_DATA;
  }

  t = return_data_value(dfLong, &cpe_font_struct.hdr.dfSize);
  if( t > length ) return ERROR_SIZE;
  else
  {
    /* set up the charWidth/charOffset  structure pairs (dfCharTable)... */

    int l_fchar = return_data_value(dfChar, &cpe_font_struct.hdr.fi.dfFirstChar),
	l_lchar = return_data_value(dfChar, &cpe_font_struct.hdr.fi.dfLastChar);
    int l_len   = l_lchar - l_fchar + 1;
    int l_ptr = sizeof(fnt_hdrS);

    /* some fields were introduced for Windows 3.x fonts */
    if( return_data_value(dfShort, &cpe_font_struct.hdr.dfVersion) == 0x200 )
	l_ptr -= 30;

    /* malloc size = (# chars) * sizeof(WinCharS) */

    if((cpe_font_struct.dfCharTable = calloc(sizeof(WinCharS), l_len)) == NULL)
	return ERROR_MEMORY;

    /* NOW, convert them all to UNIX (lton) notation... */

    for(ic=0; ic < l_len; ic++) {
   	cpe_font_struct.dfCharTable[ic].charWidth = return_data_value(dfShort, &file_buffer[l_ptr]);
	l_ptr += 2;	/* bump by sizeof(short) */


	if( return_data_value(dfShort, &cpe_font_struct.hdr.dfVersion) == 0x200) {
	    cpe_font_struct.dfCharTable[ic].charOffset =
			return_data_value(dfShort, &file_buffer[l_ptr]);
	    l_ptr += 2;	/* bump by sizeof(short) */
	    }
	else { 	/*  Windows Version 3.0 type font */
	    cpe_font_struct.dfCharTable[ic].charOffset =
			return_data_value(dfLong, &file_buffer[l_ptr]);
	    l_ptr += 4;	/* bump by sizeof(long) */
	    }
	}
    t = dump_bdf(&cpe_font_struct, file_buffer);
    free( cpe_font_struct.dfCharTable );
  }
  return t;
}

static int dump_bdf( fnt_fontS* cpe_font_struct, unsigned char* file_buffer)
{
  FILE*   fp;
  int	  ic;
  int	  l_fchar = return_data_value(dfChar, &cpe_font_struct->hdr.fi.dfFirstChar),
	  l_lchar = return_data_value(dfChar, &cpe_font_struct->hdr.fi.dfLastChar);

  int     l_len = l_lchar-l_fchar + 1,
	  l_hgt = return_data_value(dfChar, &cpe_font_struct->hdr.fi.dfPixHeight);
  int	  l_ascent = return_data_value(dfShort, &cpe_font_struct->hdr.fi.dfAscent);
  char    l_filename[256];

    if( (ic = make_bdf_filename(l_filename, cpe_font_struct, file_buffer)) )
	return ic;

    if((fp = fopen(l_filename, "w")) == NULL)
    {
      fprintf(stderr, "Couldn't open \"%s\" for output.\n", l_filename);
      return ERROR_FILE;
    }

    ic = dump_bdf_hdr(fp, cpe_font_struct, file_buffer);
    if (ic) {
      fclose(fp);
      return (ic);
    }

    /* NOW, convert all chars to UNIX (lton) notation... */

    for(ic=0; ic < l_len; ic++) {
	int rowidx, l_span,		/* how many char-cols wide is char? */
	    l_idx = cpe_font_struct->dfCharTable[ic].charOffset;

	l_span = (int) (cpe_font_struct->dfCharTable[ic].charWidth-1)/8;

	fprintf(fp, "STARTCHAR %d\n", ic);
	fprintf(fp, "ENCODING %d\n",   l_fchar);
	fprintf(fp, "SWIDTH    %d    %d\n",
		cpe_font_struct->dfCharTable[ic].charWidth*1000,
		0);

	fprintf(fp, "DWIDTH    %d    %d\n",
		cpe_font_struct->dfCharTable[ic].charWidth, 0);

	fprintf(fp, "BBX  %d  %d  %d   %d\n",
		cpe_font_struct->dfCharTable[ic].charWidth, l_hgt, 0,
		l_ascent - l_hgt);

	fprintf(fp, "BITMAP\n");
	for(rowidx=0; rowidx < l_hgt; rowidx++) {
	    switch(l_span) {
		case(0):	/* 1-7 pixels wide font */
		    {
		    fprintf(fp, "%02X\n", (int) file_buffer[l_idx+rowidx]);
		    break;
		    }

		case(1):	/* 8-15 pixels wide font */
		    {
		    fprintf(fp, "%02X%02X",
			(int) file_buffer[l_idx+rowidx], file_buffer[l_idx+l_hgt+rowidx]);
		    fprintf(fp, "\n");
		    break;
		    }

		case(2):	/* 16-23 pixels wide font */
		    {
		    fprintf(fp, "%02X%02X%02X",
			file_buffer[l_idx+rowidx],
		        file_buffer[l_idx+l_hgt+rowidx],
		        file_buffer[l_idx+(2*l_hgt)+rowidx]);
		    fprintf(fp, "\n");
		    break;
		    }

		case(3):	/* 24-31 pixels wide font */
		    {
		    fprintf(fp, "%02X%02X%02X%02X",
			file_buffer[l_idx+rowidx],
			file_buffer[l_idx+l_hgt+rowidx],
			file_buffer[l_idx+(2*l_hgt)+rowidx],
			file_buffer[l_idx+(3*l_hgt)+rowidx]);
		    fprintf(fp, "\n");
		    break;
		    }
		case(4):	/* 32-39 */
		    {
		    fprintf(fp, "%02X%02X%02X%02X%02X",
			file_buffer[l_idx+rowidx],
			file_buffer[l_idx+l_hgt+rowidx],
                        file_buffer[l_idx+(2*l_hgt)+rowidx],
                        file_buffer[l_idx+(3*l_hgt)+rowidx],
			file_buffer[l_idx+(4*l_hgt)+rowidx]);
		    fprintf(fp, "\n");
                    break;
                    }
		default:
		    fclose(fp);
		    unlink(l_filename);
		    return ERROR_DATA;
		}
	    }
	fprintf(fp, "ENDCHAR\n");

	l_fchar++;	/* Go to next one */
	}
fprintf(fp, "ENDFONT\n");
fclose(fp);
return 0;
}


static int dump_bdf_hdr(FILE* fs, fnt_fontS* cpe_font_struct, unsigned char* file_buffer)
{
int	l_fchar = return_data_value(dfChar, &cpe_font_struct->hdr.fi.dfFirstChar),
	l_lchar = return_data_value(dfChar, &cpe_font_struct->hdr.fi.dfLastChar);
int     l_len = l_lchar - l_fchar + 1;
long	l_nameoffset = return_data_value(dfLong, &cpe_font_struct->hdr.fi.dfFace);
int	l_cellheight = return_data_value(dfShort, &cpe_font_struct->hdr.fi.dfPixHeight);
int	l_ascent = return_data_value(dfShort, &cpe_font_struct->hdr.fi.dfAscent);

    fprintf(fs, "STARTFONT   2.1\n");

    /* Compose font name */

    if( l_nameoffset &&
	l_nameoffset < return_data_value(dfLong, &cpe_font_struct->hdr.dfSize) )
    {
      int     point_size;
      char*   lpFace = (char*)(file_buffer + l_nameoffset), *lpChar;
      short   tmWeight = return_data_value(dfShort, &cpe_font_struct->hdr.fi.dfWeight);

      while((lpChar = strchr(lpFace, '-')) )
	    *lpChar = ' ';

      fprintf(fs, "FONT -windows-%s-", lpFace );

      if( tmWeight == 0 )			/* weight */
	  fputs("medium-", fs);
      else if( tmWeight <= FW_LIGHT )
	  fputs("light-", fs);
      else if( tmWeight <= FW_MEDIUM )
	  fputs("medium-", fs);
      else if( tmWeight <= FW_DEMIBOLD )
	  fputs("demibold-", fs);
      else if( tmWeight <= FW_BOLD )
	  fputs("bold-", fs);
      else fputs("black-", fs);

      if( cpe_font_struct->hdr.fi.dfItalic )	/* slant */
	  fputs("i-", fs);
      else fputs("r-", fs);

      /* style */

      if( (cpe_font_struct->hdr.fi.dfPitchAndFamily & 0xF0) == FF_SWISS )
	  fputs("normal-sans-", fs);
      else fputs("normal--", fs);	/* still can be -sans */

      /* y extents */

      point_size = 10 * return_data_value(dfShort, &cpe_font_struct->hdr.fi.dfPoints );

      fprintf(fs, "%d-%d-%d-%d-",l_cellheight, point_size,
            return_data_value (dfShort, &cpe_font_struct->hdr.fi.dfHorizRes),
            return_data_value (dfShort, &cpe_font_struct->hdr.fi.dfVertRes));

      /* spacing */

      if( return_data_value(dfShort, &cpe_font_struct->hdr.fi.dfPixWidth) ) fputs("c-", fs);
      else fputs("p-", fs);

      /* average width */

      fprintf( fs, "%d-", 10 * return_data_value(dfShort, &cpe_font_struct->hdr.fi.dfAvgWidth) );

      /* charset */

     if( g_lpstrCharSet ) fprintf(fs, "%s\n", g_lpstrCharSet);
     else
      switch( cpe_font_struct->hdr.fi.dfCharSet )
      {
	/* Microsoft just had to invent its own charsets! */

	case ANSI_CHARSET: 	fputs("microsoft-cp1252\n", fs); break;
	case GREEK_CHARSET: 	fputs("microsoft-cp1253\n", fs); break;
	case TURKISH_CHARSET: 	fputs("microsoft-cp1254\n", fs); break;
	case HEBREW_CHARSET: 	fputs("microsoft-cp1255\n", fs); break;
	case ARABIC_CHARSET: 	fputs("microsoft-cp1256\n", fs); break;
	case BALTIC_CHARSET: 	fputs("microsoft-cp1257\n", fs); break;
	case RUSSIAN_CHARSET: 	fputs("microsoft-cp1251\n", fs); break;
	case EE_CHARSET: 	fputs("microsoft-cp1250\n", fs); break;
	case SYMBOL_CHARSET: 	fputs("microsoft-symbol\n", fs); break;
	case SHIFTJIS_CHARSET: 	fputs("jisx0208.1983-0\n", fs); break;
	case DEFAULT_CHARSET:	fputs("iso8859-1\n", fs); break;

	default:
	case OEM_CHARSET:
		    fputs("Undefined charset, use -c option.\n", stderr);
		    return ERROR_DATA;
      }
    }
    else return ERROR_DATA;

    fprintf(fs, "SIZE  %d  %d   %d\n",
        return_data_value(dfShort, &cpe_font_struct->hdr.fi.dfPoints ),
	return_data_value(dfShort, &cpe_font_struct->hdr.fi.dfHorizRes),
	return_data_value(dfShort, &cpe_font_struct->hdr.fi.dfVertRes));   /* dfVertRes[2] */

    fprintf(fs, "FONTBOUNDINGBOX %d  %d  %d  %d\n",
	return_data_value(dfShort, &cpe_font_struct->hdr.fi.dfMaxWidth),
	return_data_value(dfChar, &cpe_font_struct->hdr.fi.dfPixHeight),
   	0, l_ascent - l_cellheight );

    fprintf(fs, "STARTPROPERTIES  4\n");

    fprintf(fs, "FONT_ASCENT %d\n", l_ascent );                       /*  dfAscent[2] */
    fprintf(fs, "FONT_DESCENT %d\n", l_cellheight - l_ascent );
    fprintf(fs, "CAP_HEIGHT %d\n", l_ascent -
                                   return_data_value(dfShort, &cpe_font_struct->hdr.fi.dfInternalLeading));
    fprintf(fs, "DEFAULT_CHAR %d\n", return_data_value(dfShort, &cpe_font_struct->hdr.fi.dfDefaultChar));

    fprintf(fs, "ENDPROPERTIES\n");

    fprintf(fs, "CHARS  %d\n",  l_len);
    return 0;
}



static void parse_options(int argc, char **argv)
{
  int i;

  switch( argc )
  {
    case 2:
	 g_lpstrInputFile = argv[1];
	 break;

    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
	 for( i = 1; i < argc - 1; i++ )
	 {
	   if( argv[i][0] != '-' ||
	       strlen(argv[i]) != 2 ) break;

	   if( argv[i][1] == 'c' )
	       g_lpstrCharSet = argv[i+1];
	   else
	   if( argv[i][1] == 'f' )
	       g_lpstrFileName = argv[i+1];
	   else
	   if( argv[i][1] == 't' )
	   {
	      g_outputPoints = 1;
	      continue;
	   }
	   else
	   usage();

	   i++;
	 }
	 if( i == argc - 1 )
	 {
	   g_lpstrInputFile = argv[i];
	   break;
	 }
    default: usage();
  }

}

/* read file data and return file type */

static int get_resource_table(int fd, unsigned char** lpdata, int fsize)
{
  IMAGE_DOS_HEADER mz_header;
  IMAGE_OS2_HEADER ne_header;
  long s, offset, size;
  int retval;

  lseek( fd, 0, SEEK_SET );

  if( read(fd, &mz_header, sizeof(mz_header)) != sizeof(mz_header) )
      return FILE_ERROR;

  s = return_data_value(dfShort, &mz_header.e_magic);

  if( s == IMAGE_DOS_SIGNATURE)		/* looks like .dll file so far... */
  {
    s = return_data_value(dfShort, &mz_header.e_lfanew);
    lseek( fd, s, SEEK_SET );

    if( read(fd, &ne_header, sizeof(ne_header)) != sizeof(ne_header) )
	return FILE_ERROR;

    s = return_data_value(dfShort, &ne_header.ne_magic);

    if( s == IMAGE_NT_SIGNATURE)
    {
       fprintf( stderr, "Do not know how to handle 32-bit Windows DLLs.\n");
       return FILE_ERROR;
    }
    else if ( s != IMAGE_OS2_SIGNATURE) return FILE_ERROR;

    s = return_data_value(dfShort, &ne_header.ne_rsrctab);
    size = return_data_value(dfShort, &ne_header.ne_restab);

    if( size > fsize ) return FILE_ERROR;

    size -= s;
    offset = s + return_data_value(dfShort, &mz_header.e_lfanew);

    if( size <= sizeof(NE_TYPEINFO) ) return FILE_ERROR;
    retval = FILE_DLL;
  }
  else if( s == 0x300 || s == 0x200 )		/* maybe .fnt ? */
  {
    size = return_data_value(dfLong, &((fnt_hdrS *)&mz_header)->dfSize);

    if( size != fsize ) return FILE_ERROR;
    offset  = 0;
    retval = FILE_FNT;
  }
  else return FILE_ERROR;

  *lpdata = malloc(size);

  if( *lpdata )
  {
    lseek( fd, offset, SEEK_SET );
    if( read(fd, *lpdata, size) != size )
           { free( *lpdata ); *lpdata = NULL; }
  }
  return retval;
}


/* entry point */

int main(int argc, char **argv)
{
  unsigned char* lpdata = NULL;
  int 		 fd;

  parse_options( argc, argv);

  if( (fd = open( g_lpstrInputFile, O_RDONLY | O_BINARY)) != -1 )
  {
    int    i;
    struct stat file_stat;

    fstat( fd, &file_stat);
    i = get_resource_table( fd, &lpdata, file_stat.st_size );

    switch(i)
    {
	case FILE_DLL:
	     if( lpdata )
	     {
	       int	    j, count = 0;
	       NE_TYPEINFO* pTInfo = (NE_TYPEINFO*)(lpdata + 2);
	       NE_NAMEINFO* pFontStorage = NULL;

	       while( (i = return_data_value(dfShort, &pTInfo->type_id)) )
	       {
		  j = return_data_value(dfShort, &pTInfo->count);
		  if( i == NE_RSCTYPE_FONT )
		  {
		    count = j;
		    pFontStorage = (NE_NAMEINFO*)(pTInfo + 1);
		    break; /* found one */
		  }

		  pTInfo = (NE_TYPEINFO *)((char*)(pTInfo+1) + j*sizeof(NE_NAMEINFO));
	       }
	       if( pFontStorage && count )
	       {
		 unsigned short		size_shift = return_data_value(dfShort, lpdata);
		 unsigned char*		lpfont = NULL;
		 unsigned		offset;
		 int			length;

		 for( j = 0; j < count; j++, pFontStorage++ )
		 {
		    length = return_data_value(dfShort, &pFontStorage->length) << size_shift;
		    offset = return_data_value(dfShort, &pFontStorage->offset) << size_shift;

		    if( !(lpfont = realloc( lpfont, length )) )
		    {
			fprintf(stderr, "Memory allocation error.\n" );
			exit(1);
		    }

		    lseek( fd, offset, SEEK_SET );
		    if( read(fd, lpfont, length) != length )
		    {
			fprintf(stderr, "Unable to read Windows DLL.\n" );
			exit(1);
		    }

		    if( (i = parse_fnt_data( lpfont, length )) )
		    {
			fprintf(stderr, "Unable to parse font data: Error %d\n", i );
			exit(1);
		    }
		 }
		 free(lpfont); free(lpdata);
		 exit(0);
	       }
	       else
	       {
                 fprintf(stderr, "No fonts found.\n" );
		 exit(1);
	       }
	       free( lpdata );
	     }
	     else
	     {
               fprintf(stderr, "Unable to read Windows DLL.\n" );
	       exit(1);
	     }
	     break;

	case FILE_FNT:
	     if( lpdata )
	     {
	       if( (i = parse_fnt_data( lpdata, file_stat.st_size )) )
	       {
                   fprintf(stderr, "Unable to parse font data: Error %d\n", i );
		   exit(1);
	       }
	       free( lpdata );
	     }
	     else
	     {
               fprintf(stderr, "Unable to read .FNT file.\n" );
	       exit(1);
	     }
	     break;

	case FILE_ERROR:
             fprintf(stderr, "Corrupt or invalid file.\n" );
	     exit(1);
    }
    close(fd);
    exit(0);
  }
  else
  {
    fprintf(stderr, "Unable to open '%s'.\n", g_lpstrInputFile );
    exit(1);
  }
}
