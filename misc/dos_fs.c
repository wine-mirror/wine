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
#include <pwd.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>

#if defined(__linux__) || defined(sun)
#include <sys/vfs.h>
#endif
#if defined(__NetBSD__) || defined(__FreeBSD__)
#include <sys/types.h>
#include <sys/mount.h>
#endif

#include "windows.h"
#include "msdos.h"
#include "prototypes.h"
#include "autoconf.h"

/* #define DEBUG */

#define WINE_INI_USER "~/.winerc"
#define MAX_OPEN_DIRS 16
#define MAX_DOS_DRIVES	26

extern char WindowsDirectory[256], SystemDirectory[256],TempDirectory[256];

char WindowsPath[256];

static int CurrentDrive = 2;

struct DosDriveStruct {			/*  eg: */
	char 		*rootdir;	/*  /usr/windows 	*/
	char 		cwd[256];	/*  /			*/
	char 		label[13];	/*  DRIVE-A		*/		
	unsigned int	serialnumber;	/*  ABCD5678		*/
	int 		disabled;	/*  0			*/
};

static struct DosDriveStruct DosDrives[MAX_DOS_DRIVES];
static struct dosdirent DosDirs[MAX_OPEN_DIRS];

static void ExpandTildeString(char *s)
{
	struct passwd *entry;
	char temp[1024], *ptr = temp;
	
	strcpy(temp, s);
	while (*ptr)
	{
		if (*ptr != '~') { 
			*s++ = *ptr++;
			continue;
		}
		ptr++;
		if ( (entry = getpwuid(getuid())) == NULL) {
			continue;
		}
		strcpy(s, entry->pw_dir);
		s += strlen(entry->pw_dir);
	}
}

void ChopOffSlash(char *path)
{
	if (path[strlen(path)-1] == '/' || path[strlen(path)-1] == '\\')
		path[strlen(path)-1] = '\0';
}

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

	ChopOffSlash(WindowsDirectory);
	ToDos(WindowsDirectory);

	ChopOffSlash(SystemDirectory);
	ToDos(SystemDirectory);

	ChopOffSlash(TempDirectory);
	ToDos(TempDirectory);

	ToDos(WindowsPath);
	ExpandTildeString(WindowsPath);

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
		ExpandTildeString(temp);
		if ((ptr = (char *) malloc(strlen(temp)+1)) == NULL) {
			fprintf(stderr,"DOSFS: can't malloc for drive info!");
			continue;
		}
			ChopOffSlash(temp);
			DosDrives[x].rootdir = ptr;
			strcpy(DosDrives[x].rootdir, temp);
			strcpy(DosDrives[x].cwd, "/windows/");
			strcpy(DosDrives[x].label, "DRIVE-");
			strcat(DosDrives[x].label, drive);
			DosDrives[x].disabled = 0;
	}
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

#ifdef DEBUG
	fprintf(stderr,"wine.ini = %s\n",WINE_INI);
	fprintf(stderr,"win.ini = %s\n",WIN_INI);
	fprintf(stderr,"windir = %s\n",WindowsDirectory);
	fprintf(stderr,"sysdir = %s\n",SystemDirectory);
	fprintf(stderr,"tempdir = %s\n",TempDirectory);
	fprintf(stderr,"path = %s\n",WindowsPath);
#endif
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

	equipment = (diskdrives << 6) || 0x02;

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

void DOS_SetDefaultDrive(int drive)
{
#ifdef DEBUG
	fprintf(stderr,"SetDefaultDrive to %c:\n",'A'+drive);
#endif

	if (DOS_ValidDrive(drive))
		CurrentDrive = drive;
}

void ToUnix(char *s)
{
	/*  \WINDOWS\\SYSTEM   =>   /windows/system */

	char *p;

	for (p = s; *p; p++) 
	{
		if (*p != '\\')
			*s++ = tolower(*p);
		else {
			*s++ = '/';
			if (*(p+1) == '/' || *(p+1) == '\\')
				p++;
		}
	}
	*s = '\0';
}

void ToDos(char *s)
{
	/* /windows//system   =>   \WINDOWS\SYSTEM */

	char *p;
	for (p = s; *p; p++) 
	{
		if (*p != '/')
			*s++ = toupper(*p);
		else {
			*s++ = '\\';
			if (*s == '/' || *s == '\\') 
			    p++;
		}
	}
	*s = '\0';
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

static void GetUnixDirName(char *rootdir, char *name)
{
	int filename = 1;
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

char *GetUnixFileName(char *dosfilename)
{ 
	/*   a:\windows\system.ini  =>  /dos/windows/system.ini */
	
	char temp[256];
	int drive;

	if (dosfilename[1] == ':') 
	{
		drive = (islower(*dosfilename) ? toupper(*dosfilename) : *dosfilename) - 'A';
		
		if (!DOS_ValidDrive(drive))		
			return NULL;
		else
			dosfilename+=2;
	} else
		drive = CurrentDrive;

	strcpy(temp, DosDrives[drive].rootdir);
	strcat(temp, DosDrives[drive].cwd);
	GetUnixDirName(temp + strlen(DosDrives[drive].rootdir), dosfilename);

	ToUnix(temp);

#ifdef DEBUG
	fprintf(stderr,"GetUnixFileName: %s => %s\n", dosfilename, temp);
#endif

	return(temp);
}

char *DOS_GetCurrentDir(int drive)
{ 
	/* should return 'WINDOWS\SYSTEM' */

	char temp[256];

	if (!DOS_ValidDrive(drive)) 
		return 0;
	
	strcpy(temp, DosDrives[drive].cwd);
	ToDos(temp);
	fprintf(stderr, "2 %s\n", temp);
	ChopOffSlash(temp);

#ifdef DEBUG
	fprintf(stderr,"DOS_GetCWD: %c: %s\n",'A'+drive, temp + 1);
#endif
	return (temp + 1);
}

int DOS_ChangeDir(int drive, char *dirname)
{
	char temp[256];

	if (!DOS_ValidDrive(drive)) 
		return 0;

	strcpy(temp, dirname);
	ToUnix(temp);
	
	GetUnixDirName(DosDrives[drive].cwd, temp);
	strcat(DosDrives[drive].cwd,"/");
#ifdef DEBUG
	fprintf(stderr,"DOS_SetCWD: %c: %s\n",'A'+drive, DosDrives[drive].cwd);
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
	return 1;
}

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

char *DOS_GetVolumeLabel(int drive)
{
	if (!DOS_ValidDrive(drive)) 
		return NULL;

	return (DosDrives[drive].label);
}

int DOS_SetVolumeLabel(int drive, char *label)
{
	if (!DOS_ValidDrive(drive)) 
		return 0;

	strncpy(DosDrives[drive].label, label, 8);
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

char *FindFile(char *buffer, int buflen, char *filename, char **extensions, 
		char *path)
{
    char *workingpath, *dirname, *rootname, **e;
    DIR *d;
    struct dirent *f;
    int rootnamelen, found = 0;
    struct stat filestat;

    if (strchr(filename, '\\') != NULL)
    {
	strncpy(buffer, GetUnixFileName(filename), buflen);
	ToUnix(buffer);
	return buffer;
    }

    if (strchr(filename, '/') != NULL)
    {
	strncpy(buffer, filename, buflen);
	return buffer;
    }

#ifdef DEBUG
fprintf(stderr,"FindFile: looking for %s\n", filename);
#endif

    rootnamelen = strlen(filename);
    if ((rootname = malloc(rootnamelen + 1)) == NULL)
    	return NULL;
    strcpy(rootname, filename);
    ToUnix(rootname);

    if ((workingpath = malloc(strlen(path) + 1)) == NULL)
	return NULL;
    strcpy(workingpath, path);

    for(dirname = strtok(workingpath, ";"); 
	dirname != NULL;
	dirname = strtok(NULL, ";"))
    {
	if (strchr(dirname, '\\') != NULL)
		d = opendir( GetUnixFileName(dirname) );
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
				found = 1;
		    else 
		    if (f->d_name[rootnamelen] == '.')
			for (e = extensions; *e != NULL; e++)
			    if (strcasecmp(*e, f->d_name + rootnamelen + 1) 
				== 0)
			    {
				found = 1;
				break;
			    }

		    if (found)
		    {
			if (strchr(dirname, '\\') != NULL)
				strncpy(buffer, GetUnixFileName(dirname), buflen);
			else
				strncpy(buffer, dirname, buflen);

			strncat(buffer, "/", buflen - strlen(buffer));
			strncat(buffer, f->d_name, buflen - strlen(buffer));

			stat(buffer, &filestat);
			if (S_ISREG(filestat.st_mode)) {
				closedir(d);
				free(rootname);
				ToUnix(buffer);
				return buffer;
		    	} else 
		    		found = 0; 
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
	int fd;
	static char *filename = NULL;
	char name[256];

	if (filename)
		return filename;

	strcpy(name, WINE_INI_USER);
	ExpandTildeString(name);
	if ((fd = open(name, O_RDONLY)) != -1) {
		close(fd);
		filename = malloc(strlen(name) + 1);
		strcpy(filename, name);
		return(filename);
	}
	if ((fd = open(WINE_INI_GLOBAL, O_RDONLY)) != -1) {
		close(fd);
		filename = malloc(strlen(WINE_INI_GLOBAL) + 1);
		strcpy(filename, WINE_INI_GLOBAL);
		return(filename);
	}
	fprintf(stderr,"wine: can't open configuration file %s or %s !\n", 
				WINE_INI_GLOBAL, WINE_INI_USER);
	exit(1);
}

char *WinIniFileName(void)
{
	static char *name = NULL;
	
	if (name)
		return name;
		
	name = malloc(1024);

	strcpy(name, GetUnixFileName(WindowsDirectory));
	strcat(name, "/");
	strcat(name, "win.ini");
	ToUnix(name);

	name = realloc(name, strlen(name) + 1);
	
	return name;
}

static int match(char *filename, char *filemask)
{
	int x, masklength = strlen(filemask);

#ifdef DEBUG
	fprintf(stderr, "match: %s, %s\n", filename, filemask);
#endif

	for (x = 0; x != masklength ; x++) {
/*		printf("(%c%c) ", *filename, filemask[x]); 
*/
		if (!*filename)
			/* stop if EOFname */
			return 1;

		if (filemask[x] == '?') {
			/* skip the next char */
			filename++;
			continue;
		}

		if (filemask[x] == '*') {
			/* skip each char until '.' or EOFname */
			while (*filename && *filename !='.')
				filename++;
			continue;
		}
		if (filemask[x] != *filename)
			return 0;

		filename++;
	}
	return 1;
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

	if ((unixdirname = GetUnixFileName(dosdirname)) == NULL)
		return NULL;

	strcpy(temp, unixdirname);
	y = strlen(temp);
	while (y--)
	{
		if (temp[y] == '/') 
		{
			temp[y++] = '\0';
			strcpy(DosDirs[x].filemask, temp +y);
			ToDos(DosDirs[x].filemask);
			break;
		}
	}

#ifdef DEBUG
	fprintf(stderr,"DOS_opendir: %s -> %s\n", unixdirname, temp);
#endif

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
	
	do {
		if ((d = readdir(de->ds)) == NULL) 
		{
			closedir(de->ds);
			de->inuse = 0;
			return de;
		}

		strcpy(de->filename, d->d_name);
		if (d->d_reclen > 12)
			de->filename[12] = '\0';
		
		ToDos(de->filename);
	} while ( !match(de->filename, de->filemask) );

	strcpy(temp,de->unixpath);
	strcat(temp,"/");
	strcat(temp,de->filename);
	ToUnix(temp);

	stat (temp, &st);
	de->attribute = 0x0;
	if S_ISDIR(st.st_mode)
		de->attribute |= FA_DIREC;
	
	de->filesize = st.st_size;
	de->filetime = st.st_mtime;

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
