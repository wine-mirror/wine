#ifndef __WINE_PE_IMAGE_H
#define __WINE_PE_IMAGE_H

#include "windows.h"

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
    int type;        /* DLL or EXE */
    int fd;
    HINSTANCE hinstance;
    HMODULE hModule;
    int initialised;
    struct mz_header_s *mz_header;
    struct pe_data *pe;
	OFSTRUCT ofs;
};


extern int PE_unloadImage(struct w_files *wpnt);
extern int PE_StartProgram(struct w_files *wpnt);
extern void PE_InitDLL(struct w_files *wpnt);
extern HINSTANCE PE_LoadImage(struct w_files *wpnt);
extern void my_wcstombs(char * result, u_short * source, int len);

typedef struct _WIN32_function{
    char *name;
    void *definition;
} WIN32_function;

typedef struct _WIN32_builtin{
    char *name;
    WIN32_function *functions;
    int size;
    struct _WIN32_builtin *next;
} WIN32_builtin;

extern WIN32_builtin *WIN32_builtin_list;

#endif /* __WINE_PE_IMAGE_H */
