/* $Id: dlls.h,v 1.2 1993/07/04 04:04:21 root Exp root $
 */
/*
 * Copyright  Robert J. Amstadt, 1993
 */

#ifndef DLLS_H
#define DLLS_H

#define MAX_NAME_LENGTH		64

typedef struct resource_name_table
{
    struct resource_name_table *next;
    unsigned short type_ord;
    unsigned short id_ord;
    char id[MAX_NAME_LENGTH];
} RESNAMTAB;

struct ne_data {
    struct ne_header_s *ne_header;
    struct ne_segment_table_entry_s *seg_table;
    struct segment_descriptor_s *selector_table;
    char *lookup_table;
    char *nrname_table;
    char *rname_table;
    RESNAMTAB *resnamtab;
};

struct pe_data {
	struct pe_header_s *pe_header;
	struct pe_segment_table *pe_seg;
	struct PE_Import_Directory *pe_import;
	struct PE_Export_Directory *pe_export;
	struct PE_Resource_Directory *pe_resource;
	int resource_offset; /* offset to resource typedirectory in file */
};

struct w_files
{
    struct w_files  * next;
    char * name;   /* Name, as it appears in the windows binaries */
    char * filename; /* Actual name of the unix file that satisfies this */
    int fd;
    unsigned short hinstance;
    int initialised;
    struct mz_header_s *mz_header;
    struct ne_data *ne;
    struct pe_data *pe;
};

extern struct  w_files *wine_files;

#define DLL	0
#define EXE	1

#define DLL_ARGTYPE_SIGNEDWORD	0
#define DLL_ARGTYPE_WORD	1
#define DLL_ARGTYPE_LONG	2
#define DLL_ARGTYPE_FARPTR	3
#define DLL_MAX_ARGS		16

#define DLL_HANDLERTYPE_PASCAL	16
#define DLL_HANDLERTYPE_C	17

struct dll_table_entry_s
{
    /*
     * Relocation data
     */
    unsigned int selector;	/* Selector to access this entry point	  */
    void *address;		/* Offset in segment of entry point	  */

    /*
     * 16->32 bit interface data
     */
    char *export_name;
    void *handler;		/* Address of function to process request */
    char handler_type;		/* C or PASCAL calling convention	  */
    char n_args;			/* Number of arguments passed to function */
    short conv_reference ; /* reference to Argument conversion data  */
#ifdef WINESTAT
    int used;			/* Number of times this function referenced */
#endif
    
};

struct dll_name_table_entry_s
{
    char *dll_name;
    struct dll_table_entry_s *dll_table;
    int dll_table_length;
    int dll_number;
    int dll_is_used;   /* use MS provided if set to zero */
};

extern struct dll_table_entry_s KERNEL_table[];
extern struct dll_table_entry_s USER_table[];
extern struct dll_table_entry_s GDI_table[];
extern struct dll_table_entry_s UNIXLIB_table[];
extern struct dll_table_entry_s WIN87EM_table[];
extern struct dll_table_entry_s MMSYSTEM_table[];
extern struct dll_table_entry_s SHELL_table[];
extern struct dll_table_entry_s SOUND_table[];
extern struct dll_table_entry_s KEYBOARD_table[];
extern struct dll_table_entry_s WINSOCK_table[];
extern struct dll_table_entry_s STRESS_table[];
extern struct dll_table_entry_s SYSTEM_table[];
extern struct dll_table_entry_s TOOLHELP_table[];
extern struct dll_table_entry_s MOUSE_table[];
extern struct dll_table_entry_s COMMDLG_table[];
extern struct dll_table_entry_s OLE2_table[];
extern struct dll_table_entry_s OLE2CONV_table[];
extern struct dll_table_entry_s OLE2DISP_table[];
extern struct dll_table_entry_s OLE2NLS_table[];
extern struct dll_table_entry_s OLE2PROX_table[];
extern struct dll_table_entry_s OLECLI_table[];
extern struct dll_table_entry_s OLESVR_table[];


extern unsigned short KERNEL_offsets[];
extern unsigned short USER_offsets[];
extern unsigned short GDI_offsets[];
extern unsigned short UNIXLIB_offsets[];
extern unsigned short WIN87EM_offsets[];
extern unsigned short MMSYSTEM_offsets[];
extern unsigned short SHELL_offsets[];
extern unsigned short SOUND_offsets[];
extern unsigned short KEYBOARD_offsets[];
extern unsigned short WINSOCK_offsets[];
extern unsigned short STRESS_offsets[];
extern unsigned short SYSTEM_offsets[];
extern unsigned short TOOLHELP_offsets[];
extern unsigned short MOUSE_offsets[];
extern unsigned short COMMDLG_offsets[];
extern unsigned short OLE2_offsets[];
extern unsigned short OLE2CONV_offsets[];
extern unsigned short OLE2DISP_offsets[];
extern unsigned short OLE2NLS_offsets[];
extern unsigned short OLE2PROX_offsets[];
extern unsigned short OLECLI_offsets[];
extern unsigned short OLESVR_offsets[];


extern unsigned char KERNEL_types[];
extern unsigned char USER_types[];
extern unsigned char GDI_types[];
extern unsigned char UNIXLIB_types[];
extern unsigned char WIN87EM_types[];
extern unsigned char MMSYSTEM_types[];
extern unsigned char SHELL_types[];
extern unsigned char SOUND_types[];
extern unsigned char KEYBOARD_types[];
extern unsigned char WINSOCK_types[];
extern unsigned char STRESS_types[];
extern unsigned char SYSTEM_types[];
extern unsigned char TOOLHELP_types[];
extern unsigned char MOUSE_types[];
extern unsigned char COMMDLG_types[];
extern unsigned char OLE2_types[];
extern unsigned char OLE2CONV_types[];
extern unsigned char OLE2DISP_types[];
extern unsigned char OLE2NLS_types[];
extern unsigned char OLE2PROX_types[];
extern unsigned char OLECLI_types[];
extern unsigned char OLESVR_types[];

#define N_BUILTINS	22

#endif /* DLLS_H */


