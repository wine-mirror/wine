/* $Id: prototypes.h,v 1.3 1993/07/04 04:04:21 root Exp root $
 */
/*
 * Copyright  Robert J. Amstadt, 1993
 */
#ifndef _WINE_PROTOTYPES_H
#define _WINE_PROTOTYPES_H

#include <sys/types.h>

#include "neexe.h"
#include "segmem.h"
#include "wine.h"
#include "heap.h"
#include "msdos.h"
#include "windows.h"

#ifndef WINELIB

/* loader/dump.c */

extern void PrintFileHeader(struct ne_header_s *ne_header);
extern void PrintSegmentTable(struct ne_segment_table_entry_s *seg_table, 
				int nentries);
extern void PrintRelocationTable(char *exe_ptr, 
				struct ne_segment_table_entry_s *seg_entry_p, 
				int segment);

/* loader/ldtlib.c */

struct segment_descriptor *
make_sd(unsigned base, unsigned limit, int contents, int read_exec_only, int seg32, int inpgs);
int get_ldt(void *buffer);
int set_ldt_entry(int entry, unsigned long base, unsigned int limit,
	      int seg_32bit_flag, int contents, int read_only_flag,
	      int limit_in_pages_flag);

/* loader/resource.c */

extern int OpenResourceFile(HANDLE instance);
extern HBITMAP ConvertCoreBitmap( HDC hdc, BITMAPCOREHEADER * image );
extern HBITMAP ConvertInfoBitmap( HDC hdc, BITMAPINFO * image );

/* loader/selector.c */

extern int FindUnusedSelectors(int n_selectors);
extern int IPCCopySelector(int i_old, unsigned long new, int swap_type);
extern WORD AllocSelector(WORD old_selector);
extern unsigned int PrestoChangoSelector(unsigned src_selector, unsigned dst_selector);
extern WORD AllocDStoCSAlias(WORD ds_selector);
extern WORD FreeSelector(WORD sel);
extern SEGDESC *CreateNewSegments(int code_flag, int read_only, int length, 
					int n_segments);
extern SEGDESC *GetNextSegment(unsigned int flags, unsigned int limit);
extern unsigned int GetEntryDLLName(char *dll_name, char *function, int *sel,
					int *addr);
extern unsigned int GetEntryDLLOrdinal(char *dll_name, int ordinal, int *sel,
					int *addr);

/* loader/signal.c */

extern int init_wine_signals(void);

/* loader/wine.c */

extern void myerror(const char *s);
extern void load_mz_header (int, struct mz_header_s *);
extern void load_ne_header (int, struct ne_header_s *);

extern char *GetFilenameFromInstance(unsigned short instance);
extern HINSTANCE LoadImage(char *modulename, int filetype, int change_dir);
extern int _WinMain(int argc, char **argv);
extern void InitializeLoadedDLLs();

/* if1632/relay.c */

extern int CallBack16(void *func, int n_args, ...);
extern void *CALLBACK_MakeProcInstance(void *func, int instance);
extern void CallLineDDAProc(FARPROC func, short xPos, short yPos, long lParam);
extern void winestat(void);

/* if1632/callback.c */

extern int DLLRelay(unsigned int func_num, unsigned int seg_off);
extern struct dll_table_entry_s *FindDLLTable(char *dll_name);
extern int FindOrdinalFromName(struct dll_table_entry_s *dll_table, char *func_name);
extern int ReturnArg(int arg);

/* miscemu/int1a.c */

extern do_int1A(struct sigcontext_struct *context);

/* miscemu/int21.c */

extern do_int21(struct sigcontext_struct *context);

/* miscemu/kernel.c */

extern int KERNEL_LockSegment(int segment);
extern int KERNEL_UnlockSegment(int segment);
extern KERNEL_InitTask();
extern int KERNEL_WaitEvent(int task);

/* misc/comm.c */

void Comm_Init(void);
void Comm_DeInit(void);

/* misc/dos_fs.c */

extern void DOS_InitFS(void);
extern WORD DOS_GetEquipment(void);
extern int DOS_ValidDrive(int drive);
extern int DOS_GetDefaultDrive(void); 
extern void DOS_SetDefaultDrive(int drive);
extern void ToUnix(char *s); 
extern void ToDos(char *s); 
extern void ChopOffSlash(char *string);
extern int DOS_DisableDrive(int drive);
extern int DOS_EnableDrive(int drive); 
extern char *GetUnixFileName(char *dosfilename);
extern char *DOS_GetCurrentDir(int drive);
extern int DOS_ChangeDir(int drive, char *dirname);
extern int DOS_MakeDir(int drive, char *dirname);
extern int DOS_GetSerialNumber(int drive, unsigned long *serialnumber); 
extern int DOS_SetSerialNumber(int drive, unsigned long serialnumber); 
extern char *DOS_GetVolumeLabel(int drive);
extern int DOS_SetVolumeLabel(int drive, char *label);
extern int DOS_GetFreeSpace(int drive, long *size, long *available);
extern char *FindFile(char *buffer, int buflen, char *rootname, char **extensions, char *path);
extern char *WineIniFileName(void);
extern char *WinIniFileName(void); 
extern struct dosdirent *DOS_opendir(char *dosdirname); 
extern struct dosdirent *DOS_readdir(struct dosdirent *de);
extern void DOS_closedir(struct dosdirent *de);

/* misc/profile.c */

extern void sync_profiles(void);

/* misc/spy.c */

extern void SpyInit(void);

/* controls/desktop.c */

extern BOOL DESKTOP_SetPattern(char *pattern);

/* controls/widget.c */

extern BOOL WIDGETS_Init(void);

/* memory/heap.c */

extern void HEAP_Init(MDESC **free_list, void *start, int length);
extern void *HEAP_Alloc(MDESC **free_list, int flags, int bytes);
extern void *HEAP_ReAlloc(MDESC **free_list, void *old_block, int new_size, unsigned int flags);
extern int HEAP_Free(MDESC **free_list, void *block);
extern LHEAP *HEAP_LocalFindHeap(unsigned short owner);
extern void HEAP_LocalInit(unsigned short owner, void *start, int length);
extern void *WIN16_LocalAlloc(int flags, int bytes);
extern int WIN16_LocalCompact(int min_free);
extern unsigned int WIN16_LocalFlags(unsigned int handle);
extern unsigned int WIN16_LocalFree(unsigned int handle);
extern unsigned int WIN16_LocalInit(unsigned int segment, unsigned int start, unsigned int end);
extern void *WIN16_LocalLock(unsigned int handle);
extern void *WIN16_LocalReAlloc(unsigned int handle, int flags, int bytes);
extern unsigned int WIN16_LocalSize(unsigned int handle);
extern unsigned int WIN16_LocalUnlock(unsigned int handle);

/* objects/bitmaps.c */

extern BOOL BITMAP_Init(void);

/* objects/color.c */

extern BOOL COLOR_Init(void);

/* objects/dib.c */

extern int DIB_BitmapInfoSize(BITMAPINFO *info, WORD coloruse);

/* objects/gdiobj.c */

extern BOOL GDI_Init(void);

/* objects/palette.c */

extern BOOL PALETTE_Init(void);

/* objects/region.c */

extern BOOL REGION_Init(void);

/* windows/graphic.c */

extern void DrawReliefRect(HDC hdc, RECT rect, int thickness, BOOL pressed);

/* windows/dce.c */

extern void DCE_Init(void);

/* windows/dialog.c */

extern BOOL DIALOG_Init(void);

/* windows/syscolor.c */

extern void SYSCOLOR_Init(void);

/* windows/sysmetrics.c */

extern void SYSMETRICS_Init(void);

#endif /* WINELIB */
#endif /* _WINE_PROTOTYPES_H */
