/*
 * DOS-FS
 * NOV 1993 Erik Bos (erik@(trashcan.)hacktic.nl)
 *
 * FindFile by Bob, hacked for dos & unixpaths by Erik.
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#ifdef __linux__
#include <sys/vfs.h>
#endif
#ifdef __NetBSD__
#include <sys/types.h>
#include <sys/mount.h>
#endif
#include <dirent.h>
#include "windows.h"
#include "wine.h"
#include "int21.h"

/*
 #define DEBUG 
*/
#define MAX_OPEN_DIRS 16

extern char WindowsDirectory[256], SystemDirectory[256],TempDirectory[256];

char WindowsPath[256];

void DOS_DeInitFS(void);
int DOS_SetDefaultDrive(int);
char *GetDirectUnixFileName(char *);
void ToDos(char *);
void ToUnix(char *);

int CurrentDrive = 2;

struct DosDriveStruct {			/*  eg: */
	char 		*rootdir;	/*  /usr/windows 	*/
	char 		cwd[256];	/*  /			*/
	char 		label[13];	/*  DRIVE-A		*/		
	unsigned int	serialnumber;	/*  ABCD5678		*/
	int 		disabled;	/*  0			*/
};

struct DosDriveStruct DosDrives[MAX_DOS_DRIVES];

struct dosdirent DosDirs[MAX_OPEN_DIRS];

void DOS_InitFS(void)
{
	int x;
	char drive[2], temp[256], *ptr;

	GetPrivateProfileString("wine", "windows", "c:\\windows", 
		WindowsDirectory, sizeof(WindowsDirectory), WINE_INI);

	GetPrivateProfileString("wine", "system", "c:\\windows\\system", 
		SystemDirectory, sizeof(SystemDirectory), WINE_INI);

	GetPrivateProfileString("wine", "temp", "c:\\windows", 
		TempDirectory, sizeof(TempDirectory), WINE_INI);

	GetPrivateProfileString("wine", "path", "c:\\windows;c:\\windows\\system", 
		WindowsPath, sizeof(WindowsPath), WINE_INI);

	ToDos(WindowsDirectory);
	ToDos(SystemDirectory);
	ToDos(TempDirectory);
	ToDos(WindowsPath);

#ifdef DEBUG
	fprintf(stderr,"wine.ini = %s\n",WINE_INI);
	fprintf(stderr,"win.ini = %s\n",WIN_INI);
	fprintf(stderr,"windir = %s\n",WindowsDirectory);
	fprintf(stderr,"sysdir = %s\n",SystemDirectory);
	fprintf(stderr,"tempdir = %s\n",TempDirectory);
	fprintf(stderr,"path = %s\n",WindowsPath);
#endif

	for (x=0; x!=MAX_DOS_DRIVES; x++) {
		DosDrives[x].serialnumber = (0xEB0500L | x);
		
		drive[0] = 'A' + x;
		drive[1] = '\0';
		GetPrivateProfileString("drives", drive, "*", temp, sizeof(temp), WINE_INI);
		if (!strcmp(temp, "*") || *temp == '\0') {
			DosDrives[x].rootdir = NULL;		
			DosDrives[x].cwd[0] = '\0';
			DosDrives[x].label[0] = '\0';
			DosDrives[x].disabled = 1;
			continue;
		}

		if ((ptr = (char *) malloc(strlen(temp)+1)) == NULL) {
			fprintf(stderr,"DOSFS: can't malloc for drive info!");
			continue;
		}
			if (temp[strlen(temp)-1] == '/')
				temp[strlen(temp)] = '\0';
			DosDrives[x].rootdir = ptr;
			strcpy(DosDrives[x].rootdir, temp);
			strcpy(DosDrives[x].cwd, "/windows/");
			strcpy(DosDrives[x].label, "DRIVE-");
			strcat(DosDrives[x].label, drive);
			DosDrives[x].disabled = 0;
	}

	atexit(DOS_DeInitFS);

	DOS_SetDefaultDrive(2);

	for (x=0; x!=MAX_DOS_DRIVES; x++) {
		if (DosDrives[x].rootdir != NULL) {
#ifdef DEBUG
			fprintf(stderr, "DOSFS: %c: => %-40s %s %s %X %d\n",
			'A'+x,
			DosDrives[x].rootdir,
			DosDrives[x].cwd,
			DosDrives[x].label,
			DosDrives[x].serialnumber,
			DosDrives[x].disabled
			);	
#endif
		}
	}

	for (x=0; x!=MAX_OPEN_DIRS ; x++)
		DosDirs[x].inuse = 0;

}

void DOS_DeInitFS(void)
{
	int x;

	for (x=0; x!=MAX_DOS_DRIVES ; x++)
		if (DosDrives[x].rootdir != NULL) {
#ifdef DEBUG

			fprintf(stderr, "DOSFS: %c: => %s %s %s %X %d\n",
			'A'+x,
			DosDrives[x].rootdir,
			DosDrives[x].cwd,
			DosDrives[x].label,
			DosDrives[x].serialnumber,
			DosDrives[x].disabled
			);	
			free(DosDrives[x].rootdir);
#endif
		}
}

WORD DOS_GetEquipment(void)
{
	WORD equipment;
	int diskdrives = 0;

/* borrowed from Ralph Brown's interrupt lists 

		    bits 15-14: number of parallel devices
		    bit     13: [Conv] Internal modem
		    bit     12: reserved
		    bits 11- 9: number of serial devices
		    bit      8: reserved
		    bits  7- 6: number of diskette drives minus one
		    bits  5- 4: Initial video mode:
				    00b = EGA,VGA,PGA
				    01b = 40 x 25 color
				    10b = 80 x 25 color
				    11b = 80 x 25 mono
		    bit      3: reserved
		    bit      2: [PS] =1 if pointing device
				[non-PS] reserved
		    bit      1: =1 if math co-processor
		    bit      0: =1 if diskette available for boot
*/

	if (DosDrives[0].rootdir != NULL)
		diskdrives++;
	if (DosDrives[1].rootdir != NULL)
		diskdrives++;
	if (diskdrives)
		diskdrives--;

	equipment = diskdrives << 6;

	return (equipment);
}

int DOS_ValidDrive(int drive)
{
/*
#ifdef DEBUG
	fprintf(stderr,"ValidDrive %c (%d)\n",'A'+drive,drive);
#endif
*/
	if (drive >= MAX_DOS_DRIVES)
		return 0;
	if (DosDrives[drive].rootdir == NULL)
		return 0;
	if (DosDrives[drive].disabled)
		return 0;

	return 1;
}

int DOS_GetDefaultDrive(void)
{
#ifdef DEBUG
	fprintf(stderr,"GetDefaultDrive (%c)\n",'A'+CurrentDrive);
#endif

	return( CurrentDrive);
}

int DOS_SetDefaultDrive(int drive)
{
#ifdef DEBUG
	fprintf(stderr,"SetDefaultDrive to %c:\n",'A'+drive);
#endif

	if (!DOS_ValidDrive(drive))
		return 1;
		
	CurrentDrive = drive;
}

void ToUnix(char *s)
{
	while (*s) {
	        if (*s == '/')
		    break;
 		if (*s == '\\')
			*s = '/';		
		if (isupper(*s))
			*s = tolower(*s);
	s++;
	}
}

void ToDos(char *s)
{
	while (*s) {
		if (*s == '/')
			*s = '\\';		
		if (islower(*s))
			*s = toupper(*s);
	s++;
	}
}

int DOS_DisableDrive(int drive)
{
	if (drive >= MAX_DOS_DRIVES)
		return 0;
	if (DosDrives[drive].rootdir == NULL)
		return 0;

	DosDrives[drive].disabled = 1;
	return 1;
}

int DOS_EnableDrive(int drive)
{
	if (drive >= MAX_DOS_DRIVES)
		return 0;
	if (DosDrives[drive].rootdir == NULL)
		return 0;

	DosDrives[drive].disabled = 0;
	return 1;
}

void GetUnixDirName(char *rootdir, char *name)
{
	int filename;
	char *nameptr, *cwdptr;
	
	cwdptr = rootdir + strlen(rootdir);
	nameptr = name;
/*
#ifdef DEBUG
	fprintf(stderr,"GetUnixDirName: %s <=> %s => ",rootdir, name);
#endif
*/	
	while (*nameptr) {
		if (*nameptr == '.' & !filename) {
			nameptr++;
			if (*nameptr == '\0') {
				cwdptr--;
				break;
			}
			if (*nameptr == '.') {
				cwdptr--;
				while (cwdptr != rootdir) {
					cwdptr--;
					if (*cwdptr == '/') {
						*(cwdptr+1) = '\0';
						goto next;
					}
					
				}
				goto next;
			}
			if (*nameptr == '\\' || *nameptr == '/') {
			next:	nameptr++;		
				filename = 0;
				continue;
			}
		}
		if (*nameptr == '\\' || *nameptr == '/') {
			filename = 0;
			if (nameptr == name)
				cwdptr = rootdir;
			*cwdptr++='/';		
			nameptr++;
			continue;
		}
		filename = 1;
		*cwdptr++ = *nameptr++;
	}
	*cwdptr = '\0';

	ToUnix(rootdir);
/*
#ifdef DEBUG
	fprintf(stderr,"%s\n", rootdir);
#endif
*/
}

char *GetDirectUnixFileName(char *dosfilename)
{ 
	/*   a:\windows\system.ini  =>  /dos/windows/system.ini   */
	
	int drive;
	char x, temp[256];

	if (dosfilename[1] == ':') 
	{
		drive = (islower(*dosfilename) ? toupper(*dosfilename) : *dosfilename) - 'A';
		
		if (!DOS_ValidDrive(drive))		
			return NULL;
		else
			dosfilename+=2;
	} else
		drive = CurrentDrive;

	strcpy(temp,DosDrives[drive].rootdir);
	strcat(temp, DosDrives[drive].cwd);
	GetUnixDirName(temp + strlen(DosDrives[drive].rootdir), dosfilename);

	ToUnix(temp);

#ifdef DEBUG
	fprintf(stderr,"GetDirectUnixFileName: %c:%s => %s\n",'A'+ drive, dosfilename, temp);
#endif

	return(temp);
}

char *GetUnixFileName(char *dosfilename)
{
	char *dirname, *unixname, workingpath[256], temp[256];

	/* check if program specified a path */

	if (*dosfilename == '.' || *dosfilename == '\\')
		return( GetDirectUnixFileName(dosfilename) );	

	/* nope, lets find it */

#ifdef DEBUG
	fprintf(stderr,"GetUnixFileName: %s\n",dosfilename);
#endif

    	strcpy(workingpath, WindowsPath);
    
    	for(dirname = strtok(workingpath, ";") ;
    		dirname != NULL;
		dirname = strtok(NULL, ";"))
    	{
		strcpy(temp,dirname);	
		if (temp[strlen(temp)-1] != '\\')
			strcat(temp,"\\");
		strcat(temp,dosfilename);

#ifdef DEBUG
		fprintf(stderr,"trying %s\n",temp);
#endif

		if ( (unixname = GetDirectUnixFileName(temp)) != NULL)
			return unixname;
	}
	puts("FAILED!");
	return NULL;
}

char *DOS_GetCurrentDir(int drive, char *dirname)
{ 
	/* should return 'windows\system' */

	char temp[256];

	if (!DOS_ValidDrive(drive)) 
		return 0;
	
	strcpy(temp, DosDrives[drive].cwd);
	ToDos(temp);

	if (temp[strlen(temp)-1] == '\\')
		temp[strlen(temp)] = '\0';	

#ifdef DEBUG
	fprintf(stderr,"DOS_GetCWD: %c:\%s",'A'+drive, temp+1);
#endif
	return (temp+1);
}

int DOS_ChangeDir(int drive, char *dirname)
{
	if (!DOS_ValidDrive(drive)) 
		return 0;

	GetUnixDirName(DosDrives[drive].cwd, dirname);
	strcat(DosDrives[drive].cwd,"/");
#ifdef DEBUG
	fprintf(stderr,"DOS_SetCWD: %c:\%s",'A'+drive, DosDrives[drive].cwd);
#endif
	return 1;
}

int DOS_MakeDir(int drive, char *dirname)
{
	char temp[256];
	
	if (!DOS_ValidDrive(drive))
		return 0;	

	strcpy(temp, DosDrives[drive].cwd);
	GetUnixDirName(temp, dirname);
	strcat(DosDrives[drive].cwd,"/");

	ToUnix(temp);
	mkdir(temp,0);	

#ifdef DEBUG
	fprintf(stderr,"DOS_MakeDir: %c:\%s => %s",'A'+drive, dirname, temp);
#endif
}

/*
void main(void)
{
	strcpy(DosDrives[0].cwd, "1/2/3/");	
	
	puts(DosDrives[0].cwd);
	ChangeDir(0,"..");
	puts(DosDrives[0].cwd);
	
	ChangeDir(0,"..\\..");
	puts(DosDrives[0].cwd);

	ChangeDir(0,".");
	puts(DosDrives[0].cwd);

	ChangeDir(0,"test");
	puts(DosDrives[0].cwd);

	ChangeDir(0,"\\qwerty\\ab");
	puts(DosDrives[0].cwd);

	ChangeDir(0,"erik\\.\\bos\\..\\24");
	puts(DosDrives[0].cwd);

}
*/

int DOS_GetSerialNumber(int drive, unsigned long *serialnumber)
{
	if (!DOS_ValidDrive(drive)) 
		return 0;

	*serialnumber = DosDrives[drive].serialnumber;
	return 1;
}

int DOS_SetSerialNumber(int drive, unsigned long serialnumber)
{
	if (!DOS_ValidDrive(drive)) 
		return 0;

	DosDrives[drive].serialnumber = serialnumber;
	return 1;
}

int DOS_GetFreeSpace(int drive, long *size, long *available)
{
	struct statfs info;

	if (!DOS_ValidDrive(drive))
		return 0;

	if (statfs(DosDrives[drive].rootdir, &info) < 0) {
		fprintf(stderr,"dosfs: cannot do statfs(%s)\n",DosDrives[drive].rootdir);
		return 0;
	}

	*size = info.f_bsize * info.f_blocks / 1024;
	*available = info.f_bavail * info.f_bsize / 1024;
	
	return 1;
}

char *FindFile(char *buffer, int buflen, char *rootname, char **extensions, 
		char *path)
{
    char *workingpath;
    char *dirname;
    DIR *d;
    struct dirent *f;
    char **e;
    int rootnamelen;
    int found = 0;


    if (strchr(rootname, '\\') != NULL)
    {
	strncpy(buffer, GetDirectUnixFileName(rootname), buflen);
	ToUnix(buffer);
	
#ifdef DEBUG
fprintf(stderr,"FindFile: %s -> %s\n",rootname,buffer);
#endif

	return buffer;
    }

    if (strchr(rootname, '/') != NULL)
    {
	strncpy(buffer, rootname, buflen);

#ifdef DEBUG
fprintf(stderr,"FindFile: %s -> %s\n",rootname,buffer);
#endif

	return buffer;
    }

#ifdef DEBUG
fprintf(stderr,"FindFile: looking for %s\n",rootname);
#endif

    ToUnix(rootname);

    rootnamelen = strlen(rootname);
    workingpath = malloc(strlen(path) + 1);
    if (workingpath == NULL)
	return NULL;
    strcpy(workingpath, path);

    for(dirname = strtok(workingpath, ";"); 
	dirname != NULL;
	dirname = strtok(NULL, ";"))
    {
	if (strchr(dirname, '\\')!=NULL)
		d = opendir( GetDirectUnixFileName(dirname) );
	else
		d = opendir( dirname );

#ifdef DEBUG
	fprintf(stderr,"in %s\n",dirname);
#endif
	
	if (d != NULL)
	{
	    while ((f = readdir(d)) != NULL)
	    {
		if (strncasecmp(rootname, f->d_name, rootnamelen) == 0)
		{
		    if (extensions == NULL || 
			strcasecmp(rootname, f->d_name) == 0)
		    {
			found = 1;
		    }
		    else if (f->d_name[rootnamelen] == '.')
		    {
			for (e = extensions; *e != NULL; e++)
			{
			    if (strcasecmp(*e, f->d_name + rootnamelen + 1) 
				== 0)
			    {
				found = 1;
				break;
			    }
			}
		    }
		    
		    if (found)
		    {
			if (strchr(dirname, '\\')!=NULL)
				strncpy(buffer, GetDirectUnixFileName(dirname), buflen);
			 else
				strncpy(buffer, dirname, buflen);

			if (buffer[strlen(buffer)-1]!='/')			
				strncat(buffer, "/", buflen - strlen(buffer));
			
			strncat(buffer, f->d_name, buflen - strlen(buffer));
			closedir(d);

			ToUnix(buffer);

			return buffer;
		    }
		}
	    }
	    closedir(d);
	}
    }
    return NULL;
}

/**********************************************************************
 *		WineIniFileName
 */
char *WineIniFileName(void)
{
    static char *IniName = NULL;
    char inipath[256];
    
    if (IniName)
	return IniName;

    getcwd(inipath, 256);
    strcat(inipath, ";");
    strcat(inipath, getenv("HOME"));
    strcat(inipath, ";");
    strcat(inipath, getenv("WINEPATH"));

    IniName = malloc(1024);
    if (FindFile(IniName, 1024, "wine.ini", NULL, inipath) == NULL)
    {
	free(IniName);
	IniName = NULL;
	return NULL;
    }
    
    IniName = realloc(IniName, strlen(IniName) + 1);

    ToUnix(IniName);
	
    return IniName;
}

char *WinIniFileName()
{
	char name[256];
	
	strcpy(name,GetDirectUnixFileName(WindowsDirectory));	
	strcat(name,"win.ini");

	ToUnix(name);
	
	return name;
}

struct dosdirent *DOS_opendir(char *dosdirname)
{
	int x,y;
	char *unixdirname;
	char temp[256];
	
	for (x=0; x != MAX_OPEN_DIRS && DosDirs[x].inuse; x++)
		;

	if (x == MAX_OPEN_DIRS)
		return NULL;

	if ((unixdirname = GetDirectUnixFileName(dosdirname)) == NULL)
		return NULL;

	strcpy(temp,unixdirname);


	y = strlen(temp);

	while (y--)
	{
		if (temp[y] == '/') 
		{
			temp[y] = '\0';
			break;
		}
	}

	fprintf(stderr,"%s -> %s\n",unixdirname,temp);

	DosDirs[x].inuse = 1;
	strcpy(DosDirs[x].unixpath, temp);

	if ((DosDirs[x].ds = opendir(temp)) == NULL)
		return NULL;

	return &DosDirs[x];
}


struct dosdirent *DOS_readdir(struct dosdirent *de)
{
	char temp[256];
	struct dirent *d;
	struct stat st;

	if (!de->inuse)
		return NULL;
	
	if ((d = readdir(de->ds)) == NULL) 
	{
		closedir(de->ds);
		de->inuse = 0;
		return de;
	}

	strcpy(de->filename, d->d_name);
	if (d->d_reclen > 12)
		de->filename[12] = '\0';
	ToDos (de->filename);

	de->attribute = 0x0;

	strcpy(temp,de->unixpath);
	strcat(temp,"/");
	strcat(temp,de->filename);
	ToUnix(temp);
	stat (temp, &st);
	if S_ISDIR(st.st_mode)
		de->attribute |= 0x08;

	return de;
}

void DOS_closedir(struct dosdirent *de)
{
	if (de->inuse)
	{
		closedir(de->ds);
		de->inuse = 0;
	}
}
