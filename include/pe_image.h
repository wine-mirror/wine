#ifndef __WINE_PE_IMAGE_H
#define __WINE_PE_IMAGE_H

#include <sys/types.h>
#include "windows.h"
#include "winnt.h"
#include "peexe.h"

struct pe_data {
	LPIMAGE_NT_HEADERS			pe_header;
	LPIMAGE_SECTION_HEADER			pe_seg;
	LPIMAGE_IMPORT_DESCRIPTOR		pe_import;
	LPIMAGE_EXPORT_DIRECTORY		pe_export;
	LPIMAGE_RESOURCE_DIRECTORY		pe_resource;
	LPIMAGE_BASE_RELOCATION			pe_reloc;
	int base_addr;
	int load_addr;
	int vma_size;
};

typedef struct pe_data PE_MODULE;

extern int PE_unloadImage(HMODULE32 hModule);
extern FARPROC32 PE_FindExportedFunction(struct pe_data *pe, LPCSTR funcName);
extern void my_wcstombs(char * result, u_short * source, int len);

#endif /* __WINE_PE_IMAGE_H */
