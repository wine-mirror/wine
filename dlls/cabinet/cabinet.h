/*
 * cabinet.h
 *
 * Copyright 2002 Greg Turner
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifndef __WINE_CABINET_H
#define __WINE_CABINET_H

#include "winnt.h"

#define CAB_SPLITMAX (10)

#define CAB_SEARCH_SIZE (32*1024)

typedef unsigned char cab_UBYTE; /* 8 bits  */
typedef UINT16        cab_UWORD; /* 16 bits */
typedef UINT32        cab_ULONG; /* 32 bits */
typedef INT32         cab_LONG;  /* 32 bits */

typedef UINT32        cab_off_t;

/* number of bits in a ULONG */
#ifndef CHAR_BIT
# define CHAR_BIT (8)
#endif
#define CAB_ULONG_BITS (sizeof(cab_ULONG) * CHAR_BIT)

/* endian-neutral reading of little-endian data */
#define EndGetI32(a)  ((((a)[3])<<24)|(((a)[2])<<16)|(((a)[1])<<8)|((a)[0]))
#define EndGetI16(a)  ((((a)[1])<<8)|((a)[0]))

/* structure offsets */
#define cfhead_Signature         (0x00)
#define cfhead_CabinetSize       (0x08)
#define cfhead_FileOffset        (0x10)
#define cfhead_MinorVersion      (0x18)
#define cfhead_MajorVersion      (0x19)
#define cfhead_NumFolders        (0x1A)
#define cfhead_NumFiles          (0x1C)
#define cfhead_Flags             (0x1E)
#define cfhead_SetID             (0x20)
#define cfhead_CabinetIndex      (0x22)
#define cfhead_SIZEOF            (0x24)
#define cfheadext_HeaderReserved (0x00)
#define cfheadext_FolderReserved (0x02)
#define cfheadext_DataReserved   (0x03)
#define cfheadext_SIZEOF         (0x04)
#define cffold_DataOffset        (0x00)
#define cffold_NumBlocks         (0x04)
#define cffold_CompType          (0x06)
#define cffold_SIZEOF            (0x08)
#define cffile_UncompressedSize  (0x00)
#define cffile_FolderOffset      (0x04)
#define cffile_FolderIndex       (0x08)
#define cffile_Date              (0x0A)
#define cffile_Time              (0x0C)
#define cffile_Attribs           (0x0E)
#define cffile_SIZEOF            (0x10)
#define cfdata_CheckSum          (0x00)
#define cfdata_CompressedSize    (0x04)
#define cfdata_UncompressedSize  (0x06)
#define cfdata_SIZEOF            (0x08)

/* flags */
#define cffoldCOMPTYPE_MASK            (0x000f)
#define cffoldCOMPTYPE_NONE            (0x0000)
#define cffoldCOMPTYPE_MSZIP           (0x0001)
#define cffoldCOMPTYPE_QUANTUM         (0x0002)
#define cffoldCOMPTYPE_LZX             (0x0003)
#define cfheadPREV_CABINET             (0x0001)
#define cfheadNEXT_CABINET             (0x0002)
#define cfheadRESERVE_PRESENT          (0x0004)
#define cffileCONTINUED_FROM_PREV      (0xFFFD)
#define cffileCONTINUED_TO_NEXT        (0xFFFE)
#define cffileCONTINUED_PREV_AND_NEXT  (0xFFFF)
#define cffile_A_RDONLY                (0x01)
#define cffile_A_HIDDEN                (0x02)
#define cffile_A_SYSTEM                (0x04)
#define cffile_A_ARCH                  (0x20)
#define cffile_A_EXEC                  (0x40)
#define cffile_A_NAME_IS_UTF           (0x80)

/****************************************************************************/
/* our archiver information / state */

/* MSZIP stuff */
#define ZIPWSIZE 	0x8000  /* window size */
#define ZIPLBITS	9	/* bits in base literal/length lookup table */
#define ZIPDBITS	6	/* bits in base distance lookup table */
#define ZIPBMAX		16      /* maximum bit length of any code */
#define ZIPN_MAX	288     /* maximum number of codes in any set */

struct Ziphuft {
  cab_UBYTE e;                /* number of extra bits or operation */
  cab_UBYTE b;                /* number of bits in this code or subcode */
  union {
    cab_UWORD n;              /* literal, length base, or distance base */
    struct Ziphuft *t;        /* pointer to next level of table */
  } v;
};

struct ZIPstate {
    cab_ULONG window_posn;      /* current offset within the window        */
    cab_ULONG bb;               /* bit buffer */
    cab_ULONG bk;               /* bits in bit buffer */
    cab_ULONG ll[288+32];       /* literal/length and distance code lengths */
    cab_ULONG c[ZIPBMAX+1];     /* bit length count table */
    cab_LONG  lx[ZIPBMAX+1];    /* memory for l[-1..ZIPBMAX-1] */
    struct Ziphuft *u[ZIPBMAX];	/* table stack */
    cab_ULONG v[ZIPN_MAX];      /* values in order of bit length */
    cab_ULONG x[ZIPBMAX+1];     /* bit offsets, then code stack */
    cab_UBYTE *inpos;
};
  
/* Quantum stuff */

struct QTMmodelsym {
  cab_UWORD sym, cumfreq;
};

struct QTMmodel {
  int shiftsleft, entries; 
  struct QTMmodelsym *syms;
  cab_UWORD tabloc[256];
};

struct QTMstate {
    cab_UBYTE *window;         /* the actual decoding window              */
    cab_ULONG window_size;     /* window size (1Kb through 2Mb)           */
    cab_ULONG actual_size;     /* window size when it was first allocated */
    cab_ULONG window_posn;     /* current offset within the window        */

    struct QTMmodel model7;
    struct QTMmodelsym m7sym[7+1];

    struct QTMmodel model4, model5, model6pos, model6len;
    struct QTMmodelsym m4sym[0x18 + 1];
    struct QTMmodelsym m5sym[0x24 + 1];
    struct QTMmodelsym m6psym[0x2a + 1], m6lsym[0x1b + 1];

    struct QTMmodel model00, model40, model80, modelC0;
    struct QTMmodelsym m00sym[0x40 + 1], m40sym[0x40 + 1];
    struct QTMmodelsym m80sym[0x40 + 1], mC0sym[0x40 + 1];
};

/* LZX stuff */

/* some constants defined by the LZX specification */
#define LZX_MIN_MATCH                (2)
#define LZX_MAX_MATCH                (257)
#define LZX_NUM_CHARS                (256)
#define LZX_BLOCKTYPE_INVALID        (0)   /* also blocktypes 4-7 invalid */
#define LZX_BLOCKTYPE_VERBATIM       (1)
#define LZX_BLOCKTYPE_ALIGNED        (2)
#define LZX_BLOCKTYPE_UNCOMPRESSED   (3)
#define LZX_PRETREE_NUM_ELEMENTS     (20)
#define LZX_ALIGNED_NUM_ELEMENTS     (8)   /* aligned offset tree #elements */
#define LZX_NUM_PRIMARY_LENGTHS      (7)   /* this one missing from spec! */
#define LZX_NUM_SECONDARY_LENGTHS    (249) /* length tree #elements */

/* LZX huffman defines: tweak tablebits as desired */
#define LZX_PRETREE_MAXSYMBOLS  (LZX_PRETREE_NUM_ELEMENTS)
#define LZX_PRETREE_TABLEBITS   (6)
#define LZX_MAINTREE_MAXSYMBOLS (LZX_NUM_CHARS + 50*8)
#define LZX_MAINTREE_TABLEBITS  (12)
#define LZX_LENGTH_MAXSYMBOLS   (LZX_NUM_SECONDARY_LENGTHS+1)
#define LZX_LENGTH_TABLEBITS    (12)
#define LZX_ALIGNED_MAXSYMBOLS  (LZX_ALIGNED_NUM_ELEMENTS)
#define LZX_ALIGNED_TABLEBITS   (7)

#define LZX_LENTABLE_SAFETY (64) /* we allow length table decoding overruns */

#define LZX_DECLARE_TABLE(tbl) \
  cab_UWORD tbl##_table[(1<<LZX_##tbl##_TABLEBITS) + (LZX_##tbl##_MAXSYMBOLS<<1)];\
  cab_UBYTE tbl##_len  [LZX_##tbl##_MAXSYMBOLS + LZX_LENTABLE_SAFETY]

struct LZXstate {
    cab_UBYTE *window;         /* the actual decoding window              */
    cab_ULONG window_size;     /* window size (32Kb through 2Mb)          */
    cab_ULONG actual_size;     /* window size when it was first allocated */
    cab_ULONG window_posn;     /* current offset within the window        */
    cab_ULONG R0, R1, R2;      /* for the LRU offset system               */
    cab_UWORD main_elements;   /* number of main tree elements            */
    int   header_read;         /* have we started decoding at all yet?    */
    cab_UWORD block_type;      /* type of this block                      */
    cab_ULONG block_length;    /* uncompressed length of this block       */
    cab_ULONG block_remaining; /* uncompressed bytes still left to decode */
    cab_ULONG frames_read;     /* the number of CFDATA blocks processed   */
    cab_LONG  intel_filesize;  /* magic header value used for transform   */
    cab_LONG  intel_curpos;    /* current offset in transform space       */
    int   intel_started;       /* have we seen any translatable data yet? */

    LZX_DECLARE_TABLE(PRETREE);
    LZX_DECLARE_TABLE(MAINTREE);
    LZX_DECLARE_TABLE(LENGTH);
    LZX_DECLARE_TABLE(ALIGNED);
};

/* generic stuff */
#define CAB(x) (decomp_state->x)
#define ZIP(x) (decomp_state->methods.zip.x)
#define QTM(x) (decomp_state->methods.qtm.x)
#define LZX(x) (decomp_state->methods.lzx.x)
#define DECR_OK           (0)
#define DECR_DATAFORMAT   (1)
#define DECR_ILLEGALDATA  (2)
#define DECR_NOMEMORY     (3)
#define DECR_CHECKSUM     (4)
#define DECR_INPUT        (5)
#define DECR_OUTPUT       (6)

/* CAB data blocks are <= 32768 bytes in uncompressed form. Uncompressed
 * blocks have zero growth. MSZIP guarantees that it won't grow above
 * uncompressed size by more than 12 bytes. LZX guarantees it won't grow
 * more than 6144 bytes.
 */
#define CAB_BLOCKMAX (32768)
#define CAB_INPUTMAX (CAB_BLOCKMAX+6144)

struct cab_file {
  struct cab_file *next;               /* next file in sequence          */
  struct cab_folder *folder;           /* folder that contains this file */
  LPCSTR filename;                    /* output name of file            */
  HANDLE fh;                           /* open file handle or NULL       */
  cab_ULONG length;                    /* uncompressed length of file    */
  cab_ULONG offset;                    /* uncompressed offset in folder  */
  cab_UWORD index;                     /* magic index number of folder   */
  cab_UWORD time, date, attribs;       /* MS-DOS time/date/attributes    */
};


struct cab_folder {
  struct cab_folder *next;
  struct cabinet *cab[CAB_SPLITMAX];   /* cabinet(s) this folder spans   */
  cab_off_t offset[CAB_SPLITMAX];      /* offset to data blocks          */
  cab_UWORD comp_type;                 /* compression format/window size */
  cab_ULONG comp_size;                 /* compressed size of folder      */
  cab_UBYTE num_splits;                /* number of split blocks + 1     */
  cab_UWORD num_blocks;                /* total number of blocks         */
  struct cab_file *contfile;           /* the first split file           */
};

struct cabinet {
  struct cabinet *next;                /* for making a list of cabinets  */
  LPCSTR filename;                    /* input name of cabinet          */
  HANDLE *fh;                          /* open file handle or NULL       */
  cab_off_t filelen;                   /* length of cabinet file         */
  cab_off_t blocks_off;                /* offset to data blocks in file  */
  struct cabinet *prevcab, *nextcab;   /* multipart cabinet chains       */
  char *prevname, *nextname;           /* and their filenames            */
  char *previnfo, *nextinfo;           /* and their visible names        */
  struct cab_folder *folders;           /* first folder in this cabinet   */
  struct cab_file *files;               /* first file in this cabinet     */
  cab_UBYTE block_resv;                /* reserved space in datablocks   */
  cab_UBYTE flags;                     /* header flags                   */
};

typedef struct cds_forward {
  struct cab_folder *current;      /* current folder we're extracting from  */
  cab_ULONG offset;                /* uncompressed offset within folder     */
  cab_UBYTE *outpos;               /* (high level) start of data to use up  */
  cab_UWORD outlen;                /* (high level) amount of data to use up */
  cab_UWORD split;                 /* at which split in current folder?     */
  int (*decompress)(int, int, struct cds_forward *);     /* the chosen compression func      */
  cab_UBYTE inbuf[CAB_INPUTMAX+2]; /* +2 for lzx bitbuffer overflows!  */
  cab_UBYTE outbuf[CAB_BLOCKMAX];
  union {
    struct ZIPstate zip;
    struct QTMstate qtm;
    struct LZXstate lzx;
  } methods;
} cab_decomp_state;

/* from cabextract.c */
BOOL process_cabinet(LPCSTR cabname, LPCSTR dir, BOOL fix, BOOL lower);

#endif /* __WINE_CABINET_H */
