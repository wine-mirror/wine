#ifndef __WINE_PE_IMAGE_H
#define __WINE_PE_IMAGE_H

#include <sys/types.h>
#include "windows.h"
#include "dlls.h"

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
	int resource_offset; /* offset to resource typedirectory in file */
};

extern int PE_unloadImage(HMODULE hModule);
extern void my_wcstombs(char * result, u_short * source, int len);

#endif /* __WINE_PE_IMAGE_H */
