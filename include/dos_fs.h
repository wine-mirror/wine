#ifndef __WINE_DOS_FS_H
#define __WINE_DOS_FS_H

#include <wintypes.h>

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
extern char *DOS_GetUnixFileName(char *dosfilename);
extern char *DOS_GetDosFileName(char *unixfilename);
extern char *DOS_GetCurrentDir(int drive);
extern int DOS_ChangeDir(int drive, char *dirname);
extern int DOS_MakeDir(int drive, char *dirname);
extern int DOS_GetSerialNumber(int drive, unsigned long *serialnumber); 
extern int DOS_SetSerialNumber(int drive, unsigned long serialnumber); 
extern char *DOS_GetVolumeLabel(int drive);
extern int DOS_SetVolumeLabel(int drive, char *label);
extern int DOS_GetFreeSpace(int drive, long *size, long *available);
extern char *DOS_FindFile(char *buffer, int buflen, char *rootname, char **extensions, char *path);
extern char *WineIniFileName(void);
extern char *WinIniFileName(void); 
extern struct dosdirent *DOS_opendir(char *dosdirname); 
extern struct dosdirent *DOS_readdir(struct dosdirent *de);
extern void DOS_closedir(struct dosdirent *de);
extern void DOS_ExpandToFullPath(char *filename, int drive);
extern void DOS_ExpandToFullUnixPath(char *filename);
extern char *DOS_GetRedirectedDir(int drive);
extern void errno_to_doserr(void);

extern char WindowsPath[256];

#endif /* __WINE_DOS_FS_H */
