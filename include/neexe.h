/* $Id: neexe.h,v 1.4 1993/07/04 04:04:21 root Exp root $
 */
/*
 * Copyright  Robert J. Amstadt, 1993
 */
#ifndef NEEXE_H
#define NEEXE_H

#include "wintypes.h"

/*
 * Old MZ header for DOS programs.  Actually just a couple of fields
 * from it, so that we can find the start of the NE header.
 */
struct mz_header_s
{
    WORD mz_magic;         /* MZ Header signature */
    BYTE  dont_care[0x3a];  /* MZ Header stuff */
    WORD ne_offset;        /* Offset to extended header */
};

#define MZ_SIGNATURE  ('M' | ('Z' << 8))

/*
 * This is the Windows executable (NE) header.
 */
struct ne_header_s
{
    WORD  ne_magic;             /* 00 NE signature 'NE' */
    BYTE  linker_version;	/* 02 Linker version number */
    BYTE  linker_revision;	/* 03 Linker revision number */
    WORD  entry_tab_offset;	/* 04 Offset to entry table relative to NE */
    WORD  entry_tab_length;	/* 06 Length of entry table in bytes */
    DWORD reserved1;		/* 08 Reserved by Microsoft */
    WORD  format_flags;         /* 0c Flags about segments in this file */
    WORD  auto_data_seg;	/* 0e Automatic data segment number */
    WORD  local_heap_length;	/* 10 Initial size of local heap */
    WORD  stack_length;         /* 12 Initial size of stack */
    WORD  ip;			/* 14 Initial IP */
    WORD  cs;			/* 16 Initial CS */
    WORD  sp;			/* 18 Initial SP */
    WORD  ss;			/* 1a Initial SS */
    WORD  n_segment_tab;	/* 1c # of entries in segment table */
    WORD  n_mod_ref_tab;	/* 1e # of entries in module reference tab. */
    WORD  nrname_tab_length; 	/* 20 Length of nonresident-name table     */
    WORD  segment_tab_offset;	/* 22 Offset to segment table */
    WORD  resource_tab_offset;  /* 24 Offset to resource table */
    WORD  rname_tab_offset;	/* 26 Offset to resident-name table */
    WORD  moduleref_tab_offset; /* 28 Offset to module reference table */
    WORD  iname_tab_offset;	/* 2a Offset to imported name table */
    DWORD nrname_tab_offset;	/* 2c Offset to nonresident-name table */
    WORD  n_mov_entry_points;	/* 30 # of movable entry points */
    WORD  align_shift_count;	/* 32 Logical sector alignment shift count */
    WORD  n_resource_seg;	/* 34 # of resource segments */
    BYTE  operating_system;	/* 36 Flags indicating target OS */
    BYTE  additional_flags;	/* 37 Additional information flags */
    WORD  fastload_offset;	/* 38 Offset to fast load area */
    WORD  fastload_length;	/* 3a Length of fast load area */
    WORD  reserved2;		/* 3c Reserved by Microsoft */
    WORD  expect_version;	/* 3e Expected Windows version number */
};

#define NE_SIGNATURE  ('N' | ('E' << 8))
#define PE_SIGNATURE  ('P' | ('E' << 8))

/*
 * NE Header FORMAT FLAGS
 */
#define NE_FFLAGS_SINGLEDATA	0x0001
#define NE_FFLAGS_MULTIPLEDATA	0x0002
#define NE_FFLAGS_WIN32         0x0010
#define NE_FFLAGS_BUILTIN       0x0020  /* Wine built-in module */
#define NE_FFLAGS_SELFLOAD	0x0800
#define NE_FFLAGS_LINKERROR	0x2000
#define NE_FFLAGS_CALLWEP       0x4000
#define NE_FFLAGS_LIBMODULE	0x8000

/*
 * NE Header OPERATING SYSTEM
 */
#define NE_OSFLAGS_UNKNOWN	0x01
#define NE_OSFLAGS_WINDOWS	0x04

/*
 * NE Header ADDITIONAL FLAGS
 */
#define NE_AFLAGS_WIN2_PROTMODE	0x02
#define NE_AFLAGS_WIN2_PROFONTS	0x04
#define NE_AFLAGS_FASTLOAD	0x08

/*
 * Segment table entry
 */
struct ne_segment_table_entry_s
{
    WORD seg_data_offset;	/* Sector offset of segment data	*/
    WORD seg_data_length;	/* Length of segment data		*/
    WORD seg_flags;		/* Flags associated with this segment	*/
    WORD min_alloc;		/* Minimum allocation size for this	*/
};

/*
 * Segment Flags
 */
#define NE_SEGFLAGS_DATA	0x0001
#define NE_SEGFLAGS_ALLOCATED	0x0002
#define NE_SEGFLAGS_LOADED	0x0004
#define NE_SEGFLAGS_ITERATED	0x0008
#define NE_SEGFLAGS_MOVEABLE	0x0010
#define NE_SEGFLAGS_SHAREABLE	0x0020
#define NE_SEGFLAGS_PRELOAD	0x0040
#define NE_SEGFLAGS_EXECUTEONLY	0x0080
#define NE_SEGFLAGS_READONLY	0x0080
#define NE_SEGFLAGS_RELOC_DATA	0x0100
#define NE_SEGFLAGS_DISCARDABLE	0x1000

/*
 * Relocation table entry
 */
struct relocation_entry_s
{
    BYTE address_type;	/* Relocation address type		*/
    BYTE relocation_type;	/* Relocation type			*/
    WORD offset;		/* Offset in segment to fixup		*/
    WORD target1;		/* Target specification			*/
    WORD target2;		/* Target specification			*/
};

/*
 * Relocation address types
 */
#define NE_RADDR_LOWBYTE	0
#define NE_RADDR_SELECTOR	2
#define NE_RADDR_POINTER32	3
#define NE_RADDR_OFFSET16	5
#define NE_RADDR_POINTER48	11
#define NE_RADDR_OFFSET32	13

/*
 * Relocation types
 */
#define NE_RELTYPE_INTERNAL	0
#define NE_RELTYPE_ORDINAL	1
#define NE_RELTYPE_NAME		2
#define NE_RELTYPE_OSFIXUP	3
#define NE_RELFLAG_ADDITIVE	4

/*
 * DOS PSP
 */
struct dos_psp_s
{
    unsigned short pspInt20;
    unsigned short pspNextParagraph;
    unsigned char  pspReserved1;
    unsigned char  pspDispatcher[5];
    unsigned short pspTerminateVector[2];
    unsigned short pspControlCVector[2];
    unsigned short pspCritErrorVector[2];
    unsigned short pspReserved2[11];
    unsigned short pspEnvironment;
    unsigned short pspReserved3[23];
    unsigned char  pspFCB_1[16];
    unsigned char  pspFCB_2[16];
    unsigned char  pspReserved4[4];
    unsigned char  pspCommandTailCount;
    unsigned char  pspCommandTail[128];
};

/*
 * Entry table structures.
 */
struct entry_tab_header_s
{
    unsigned char n_entries;
    unsigned char seg_number;
};

struct entry_tab_movable_s
{
    unsigned char flags;
    unsigned char int3f[2];
    unsigned char seg_number;
    unsigned short offset;
};

struct entry_tab_fixed_s
{
    unsigned char flags;
    unsigned char offset[2];
};

/*
 * Resource table structures.
 */
struct resource_nameinfo_s
{
    unsigned short offset;
    unsigned short length;
    unsigned short flags;
    unsigned short id;
    HANDLE16 handle;
    unsigned short usage;
};

struct resource_typeinfo_s
{
    unsigned short type_id;	/* Type identifier */
    unsigned short count;	/* Number of resources of this type */
    unsigned long  reserved;
    /*
     * Name info array.
     */
};

#define NE_RSCTYPE_ACCELERATOR		0x8009
#define NE_RSCTYPE_BITMAP		0x8002
#define NE_RSCTYPE_CURSOR		0x8001
#define NE_RSCTYPE_DIALOG		0x8005
#define NE_RSCTYPE_FONT			0x8008
#define NE_RSCTYPE_FONTDIR		0x8007
#define NE_RSCTYPE_GROUP_CURSOR		0x800c
#define NE_RSCTYPE_GROUP_ICON		0x800e
#define NE_RSCTYPE_ICON			0x8003
#define NE_RSCTYPE_MENU			0x8004
#define NE_RSCTYPE_RCDATA		0x800a
#define NE_RSCTYPE_STRING		0x8006

int  load_typeinfo  (int, struct resource_typeinfo_s *);
int  load_nameinfo  (int, struct resource_nameinfo_s *);

#endif /* NEEXE_H */
