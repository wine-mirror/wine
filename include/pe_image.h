#ifndef __WINE_PE_IMAGE_H
#define __WINE_PE_IMAGE_H

#include <sys/types.h>
#include "windows.h"

struct pe_data {
	struct pe_header_s *pe_header;
	struct pe_segment_table *pe_seg;
	struct PE_Import_Directory *pe_import;
	struct PE_Export_Directory *pe_export;
	struct PE_Resource_Directory *pe_resource;
	struct PE_Reloc_Block *pe_reloc;
	int base_addr;
	int load_addr;
	int vma_size;
};

typedef struct pe_data PE_MODULE;

extern int PE_unloadImage(HMODULE32 hModule);
extern FARPROC32 PE_FindExportedFunction(struct pe_data *pe, LPCSTR funcName);
extern void my_wcstombs(char * result, u_short * source, int len);

#endif /* __WINE_PE_IMAGE_H */
