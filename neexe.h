/* $Id: neexe.h,v 1.3 1993/06/30 14:24:33 root Exp root $
 */
/*
 * Copyright  Robert J. Amstadt, 1993
 */
#ifndef NEEXE_H
#define NEEXE_H

/*
 * Old MZ header for DOS programs.  Actually just a couple of fields
 * from it, so that we can find the start of the NE header.
 */
struct mz_header_s
{
    u_char dont_care1[0x18];	/* MZ Header stuff			*/
    u_char must_be_0x40;	/* 0x40 for non-MZ program		*/
    u_char dont_care2[0x23];	/* More MZ header stuff			*/
    u_short ne_offset;		/* Offset to extended header		*/
};

/*
 * This is the Windows executable (NE) header.
 */
struct ne_header_s
{
    char    header_type[2];	/* Must contain 'N' 'E'			*/
    u_char  linker_version;	/* Linker version number		*/
    u_char  linker_revision;	/* Linker revision number		*/
    u_short entry_tab_offset;	/* Offset to entry table relative to NE */
    u_short entry_tab_length;	/* Length of etnry table in bytes	*/
    u_long  reserved1;		/* Reserved by Microsoft		*/
    u_short format_flags;	/* Flags that segments in this file	*/
    u_short auto_data_seg;	/* Automatic data segment number	*/
    u_short local_heap_length;	/* Initial size of local heap		*/
    u_short stack_length;	/* Initial size of stack		*/
    u_short ip;			/* Initial IP				*/
    u_short cs;			/* Initial CS				*/
    u_short sp;			/* Initial SP				*/
    u_short ss;			/* Initial SS				*/
    u_short n_segment_tab;	/* # of entries in segment table	*/
    u_short n_mod_ref_tab;	/* # of entries in module reference tab.*/
    u_short nrname_tab_length; 	/* Length of nonresident-name table     */
    u_short segment_tab_offset;	/* Offset to segment table		*/
    u_short resource_tab_offset;/* Offset to resource table		*/
    u_short rname_tab_offset;	/* Offset to resident-name table	*/
    u_short moduleref_tab_offset;/* Offset to module reference table	*/
    u_short iname_tab_offset;	/* Offset to imported name table	*/
    u_long  nrname_tab_offset;	/* Offset to nonresident-name table	*/
    u_short n_mov_entry_points;	/* # of movable entry points		*/
    u_short align_shift_count;	/* Logical sector alignment shift count */
    u_short n_resource_seg;	/* # of resource segments		*/
    u_char  operating_system;	/* Flags indicating target OS		*/
    u_char  additional_flags;	/* Additional information flags		*/
    u_short fastload_offset;	/* Offset to fast load area		*/
    u_short fastload_length;	/* Length of fast load area		*/
    u_short reserved2;		/* Reserved by Microsoft		*/
    u_short expect_version;	/* Expected Windows version number	*/
};

/*
 * NE Header FORMAT FLAGS
 */
#define NE_FFLAGS_SINGLEDATA	0x0001
#define NE_FFLAGS_MULTIPLEDATA	0x0002
#define NE_FFLAGS_SELFLOAD	0x0800
#define NE_FFLAGS_LINKERROR	0x2000
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
    u_short seg_data_offset;	/* Sector offset of segment data	*/
    u_short seg_data_length;	/* Length of segment data		*/
    u_short seg_flags;		/* Flags associated with this segment	*/
    u_short min_alloc;		/* Minimum allocation size for this	*/
};

/*
 * Segment Flags
 */
#define NE_SEGFLAGS_DATA	0x0001
#define NE_SEGFLAGS_ALLOCATED	0x0002
#define NE_SEGFLAGS_LOADED	0x0004
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
    u_char  address_type;	/* Relocation address type		*/
    u_char  relocation_type;	/* Relocation type			*/
    u_short offset;		/* Offset in segment to fixup		*/
    u_short target1;		/* Target specification			*/
    u_short target2;		/* Target specification			*/
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
    unsigned short handle;
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

#endif /* NEEXE_H */
