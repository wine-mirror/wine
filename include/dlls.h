/* $Id: dlls.h,v 1.2 1993/07/04 04:04:21 root Exp root $
 */
/*
 * Copyright  Robert J. Amstadt, 1993
 */

#ifndef DLLS_H
#define DLLS_H

typedef struct dll_arg_relocation_s
{
    unsigned short dst_arg;	/* Offset to argument on stack		*/
    unsigned char src_type;	/* Argument type			*/
} DLL_ARG;

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
    int handler_type;		/* C or PASCAL calling convention	  */
#ifdef WINESTAT
    int used;			/* Number of times this function referenced */
#endif
    int n_args;			/* Number of arguments passed to function */
    DLL_ARG args[DLL_MAX_ARGS]; /* Argument conversion data		  */
};

struct dll_name_table_entry_s
{
    char *dll_name;
    struct dll_table_entry_s *dll_table;
    int dll_table_length;
    int dll_number;
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

#endif /* DLLS_H */
