#ifndef __WINE_LIBRARY_H
#define __WINE_LIBRARY_H

extern HINSTANCE hInstMain;
extern HINSTANCE hSysRes;
extern struct w_files *GetFileInfo(unsigned short instance);
extern int IsDLLLoaded(char *name);
extern void InitDLL(struct w_files *wpnt);
extern void InitializeLoadedDLLs(struct w_files *wpnt);
extern HINSTANCE LoadImage(char *module, int filetype, int change_dir);

extern struct dll_name_table_entry_s dll_builtin_table[];

#endif /* __WINE_LIBRARY_H */
