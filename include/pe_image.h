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
extern BOOL32 PE_EnumResourceTypes32A(HMODULE32,ENUMRESTYPEPROC32A,LONG);
extern BOOL32 PE_EnumResourceTypes32W(HMODULE32,ENUMRESTYPEPROC32W,LONG);
extern BOOL32 PE_EnumResourceNames32A(HMODULE32,LPCSTR,ENUMRESNAMEPROC32A,LONG);
extern BOOL32 PE_EnumResourceNames32W(HMODULE32,LPCWSTR,ENUMRESNAMEPROC32W,LONG);
extern BOOL32 PE_EnumResourceLanguages32A(HMODULE32,LPCSTR,LPCSTR,ENUMRESLANGPROC32A,LONG);
extern BOOL32 PE_EnumResourceLanguages32W(HMODULE32,LPCWSTR,LPCWSTR,ENUMRESLANGPROC32W,LONG);
extern HRSRC32 PE_FindResourceEx32W(HINSTANCE32,LPCWSTR,LPCWSTR,WORD);
extern DWORD PE_SizeofResource32(HINSTANCE32,HRSRC32);
extern HGLOBAL32 PE_LoadResource32(HINSTANCE32,HRSRC32);
extern void PE_InitializeDLLs(HMODULE16,DWORD,LPVOID);

#endif /* __WINE_PE_IMAGE_H */
