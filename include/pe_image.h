#ifndef __WINE_PE_IMAGE_H
#define __WINE_PE_IMAGE_H

#include <sys/types.h>
#include "windows.h"
#include "winnt.h"
#include "peexe.h"

/* This struct is used for loaded PE .dlls */
struct pe_data {
	LPIMAGE_NT_HEADERS	pe_header;
	LPIMAGE_SECTION_HEADER	pe_seg;
	HMODULE32		mappeddll;
};

typedef struct pe_data PE_MODULE;

/* modreference used for attached processes
 * all section are calculated here, relocations etc.
 */
struct pe_modref {
	struct pe_modref		*next;
	PE_MODULE			*pe_module;
	unsigned long int		load_addr;
	LPIMAGE_IMPORT_DESCRIPTOR	pe_import;
	LPIMAGE_EXPORT_DIRECTORY	pe_export;
	LPIMAGE_RESOURCE_DIRECTORY	pe_resource;
	LPIMAGE_BASE_RELOCATION		pe_reloc;
	int				flags;
#define PE_MODREF_PROCESS_ATTACHED	0x00000001
#define PE_MODREF_NO_DLL_CALLS		0x00000002
#define PE_MODREF_RELOCS_DONE		0x00000004
};

typedef struct pe_modref PE_MODREF;

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
extern HMODULE32 PE_LoadLibraryEx32A(LPCSTR,HFILE32,DWORD);
extern HGLOBAL32 PE_LoadResource32(HINSTANCE32,HRSRC32);

struct _PDB32; /* forward definition */
extern void PE_InitializeDLLs(struct _PDB32*,DWORD,LPVOID);

#endif /* __WINE_PE_IMAGE_H */
