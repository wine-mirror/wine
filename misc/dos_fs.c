/*
 * DOS-FS
 * NOV 1993 Erik Bos <erik@xs4all.nl>
 *
 * FindFile by Bob, hacked for dos & unixpaths by Erik.
 *
 * Bugfix by dash@ifi.uio.no: ToUnix() was called to often
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
#include <sys/param.h>
#include <sys/mount.h>
#endif

#include "wine.h"
#include "windows.h"
#include "msdos.h"
#include "dos_fs.h"
#include "autoconf.h"
#include "comm.h"
#include "stddebug.h"
#include "debug.h"

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
	if (*ptr != '~') 
	{ 
	    *s++ = *ptr++;
	    continue;
	}

	ptr++;

	if ( (entry = getpwuid(getuid())) == NULL) 
	{
	    continue;
	}

	strcpy(s, entry->pw_dir);
	s += strlen(entry->pw_dir);
    }
    *s = 0;
}

void ChopOffSlash(char *path)
{
	if (path[strlen(path)-1] == '/' || path[strlen(path)-1] == '\\')
		path[strlen(path)-1] = '\0';
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
			if (*(p+1) == '/' || *(p+1) == '\\') 
			    p++;
		}
	}
	*s = '\0';
}

void DOS_InitFS(void)
{
	int x;
	char drive[2], temp[256];

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
		ChopOffSlash(temp);
		DosDrives[x].rootdir = strdup(temp);
		strcpy(DosDrives[x].rootdir, temp);
		strcpy(DosDrives[x].cwd, "/windows/");
		strcpy(DosDrives[x].label, "DRIVE-");
		strcat(DosDrives[x].label, drive);
		DosDrives[x].disabled = 0;
	}
	DosDrives[25].rootdir = "/";
	strcpy(DosDrives[25].cwd, "/");
	strcpy(DosDrives[25].label, "UNIX-FS");
	DosDrives[25].serialnumber = 0x12345678;
	DosDrives[25].disabled = 0;

        /* Get the startup directory and try to map it to a DOS drive
         * and directory.  (i.e., if we start in /dos/windows/word and
         * drive C is defined as /dos, the starting wd for C will be
         * /windows/word)  Also set the default drive to whatever drive
         * corresponds to the directory we started in.
         */
        getcwd(temp, 254);
        strcat(temp, "/");      /* For DOS_GetDosFileName */
        strcpy(temp, DOS_GetDosFileName(temp));
        if(temp[0] != 'Z')
        {
            ToUnix(&temp[2]);
            strcpy(DosDrives[temp[0] - 'A'].cwd, &temp[2]);
            DOS_SetDefaultDrive(temp[0] - 'A');
        }
        else
        {
	    DOS_SetDefaultDrive(2);
        }

	for (x=0; x!=MAX_DOS_DRIVES; x++) {
		if (DosDrives[x].rootdir != NULL) {
            dprintf_dosfs(stddeb, "DOSFS: %c: => %-40s %s %s %X %d\n",
			'A'+x,
			DosDrives[x].rootdir,
			DosDrives[x].cwd,
			DosDrives[x].label,
			DosDrives[x].serialnumber,
			DosDrives[x].disabled
			);	
		}
	}

	for (x=0; x!=MAX_OPEN_DIRS ; x++)
		DosDirs[x].inuse = 0;

    dprintf_dosfs(stddeb,"wine.ini = %s\n",WINE_INI);
    dprintf_dosfs(stddeb,"win.ini = %s\n",WIN_INI);
    dprintf_dosfs(stddeb,"windir = %s\n",WindowsDirectory);
    dprintf_dosfs(stddeb,"sysdir = %s\n",SystemDirectory);
    dprintf_dosfs(stddeb,"tempdir = %s\n",TempDirectory);
    dprintf_dosfs(stddeb,"path = %s\n",WindowsPath);
}

WORD DOS_GetEquipment(void)
{
	WORD equipment;
	int diskdrives = 0;
	int parallelports = 0;
	int serialports = 0;
	int x;

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
/*  Currently the only of these bits correctly set are:
		bits 15-14 		} Added by William Owen Smith, 
		bits 11-9		} wos@dcs.warwick.ac.uk
		bits 7-6
		bit  2			(always set)
*/

	if (DosDrives[0].rootdir != NULL)
		diskdrives++;
	if (DosDrives[1].rootdir != NULL)
		diskdrives++;
	if (diskdrives)
		diskdrives--;
	
	for (x=0; x!=MAX_PORTS; x++) {
		if (COM[x].devicename)
			serialports++;
		if (LPT[x].devicename)
			parallelports++;
	}
	if (serialports > 7)		/* 3 bits -- maximum value = 7 */
		serialports=7;
	if (parallelports > 3)		/* 2 bits -- maximum value = 3 */
		parallelports=3;

	equipment = (diskdrives << 6) | (serialports << 9) | 
		    (parallelports << 14) | 0x02;

    dprintf_dosfs(stddeb, "DOS_GetEquipment : diskdrives = %d serialports = %d "
			"parallelports = %d\n"
			"DOS_GetEquipment : equipment = %d\n",
			diskdrives, serialports, parallelports, equipment);

	return (equipment);
}

int DOS_ValidDrive(int drive)
{
    dprintf_dosfs(stddeb,"ValidDrive %c (%d)\n",'A'+drive,drive);
	if (drive >= MAX_DOS_DRIVES)
		return 0;
	if (DosDrives[drive].rootdir == NULL)
		return 0;
	if (DosDrives[drive].disabled)
		return 0;

	return 1;
}


int DOS_ValidDirectory(char *name)
{
	char *dirname;
	struct stat s;
	dprintf_dosfs(stddeb, "DOS_ValidDirectory: '%s'\n", name);
	if ((dirname = DOS_GetUnixFileName(name)) == NULL)
		return 0;
	if (stat(dirname,&s))
		return 0;
	if (!S_ISDIR(s.st_mode))
		return 0;
	dprintf_dosfs(stddeb, "==> OK\n");
	return 1;
}



int DOS_GetDefaultDrive(void)
{
    dprintf_dosfs(stddeb,"GetDefaultDrive (%c)\n",'A'+CurrentDrive);
	return( CurrentDrive);
}

void DOS_SetDefaultDrive(int drive)
{
    dprintf_dosfs(stddeb,"SetDefaultDrive to %c:\n",'A'+drive);
	if (DOS_ValidDrive(drive))
		CurrentDrive = drive;
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

	dprintf_dosfs(stddeb,"GetUnixDirName: %s <=> %s => ",rootdir, name);

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

	dprintf_dosfs(stddeb,"%s\n", rootdir);

}

char *DOS_GetUnixFileName(char *dosfilename)
{ 
	/*   a:\windows\system.ini  =>  /dos/windows/system.ini */
	
	static char temp[256];
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

        /* Expand the filename to it's full path if it doesn't
         * start from the root.
         */
        DOS_ExpandToFullPath(dosfilename, drive);

	strcpy(temp, DosDrives[drive].rootdir);
	strcat(temp, DosDrives[drive].cwd);
	GetUnixDirName(temp + strlen(DosDrives[drive].rootdir), dosfilename);

	dprintf_dosfs(stddeb,"GetUnixFileName: %s => %s\n", dosfilename, temp);
	return(temp);
}

/* Note: This function works on directories as well as long as
 * the directory ends in a slash.
 */
char *DOS_GetDosFileName(char *unixfilename)
{ 
	int i;
	static char temp[256], rootdir[256];
	/*   /dos/windows/system.ini => c:\windows\system.ini */
	
        /* Expand it if it's a relative name.
         */
        DOS_ExpandToFullUnixPath(unixfilename);

	for (i = 0 ; i != MAX_DOS_DRIVES; i++) {
		if (DosDrives[i].rootdir != NULL) {
 			strcpy(rootdir, DosDrives[i].rootdir);
 			strcat(rootdir, "/");
 			if (strncmp(rootdir, unixfilename, strlen(rootdir)) == 0) {
 				sprintf(temp, "%c:\\%s", 'A' + i, unixfilename + strlen(rootdir));
				ToDos(temp);
				return temp;
			}	
		}
	}
	sprintf(temp, "Z:%s", unixfilename);
	ToDos(temp);
	return(temp);
}

char *DOS_GetCurrentDir(int drive)
{ 
	/* should return 'WINDOWS\SYSTEM' */

	static char temp[256];

	if (!DOS_ValidDrive(drive)) 
		return 0;
	
	strcpy(temp, DosDrives[drive].cwd);
	ToDos(temp);
	ChopOffSlash(temp);

    	dprintf_dosfs(stddeb,"DOS_GetCWD: %c: %s\n",'A'+drive, temp + 1);
	return (temp + 1);
}

int DOS_ChangeDir(int drive, char *dirname)
{
	char temp[256],old[256];

	if (!DOS_ValidDrive(drive)) 
		return 0;

	strcpy(temp, dirname);
	ToUnix(temp);
	strcpy(old, DosDrives[drive].cwd);
	
	GetUnixDirName(DosDrives[drive].cwd, temp);
	strcat(DosDrives[drive].cwd,"/");

	dprintf_dosfs(stddeb,"DOS_SetCWD: %c: %s\n",'A'+drive,
                      DosDrives[drive].cwd);

	if (!DOS_ValidDirectory(DosDrives[drive].cwd))
	{
		strcpy(DosDrives[drive].cwd, old);
		return 0;
	}
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

	ToUnix(temp + strlen(DosDrives[drive].cwd));
	mkdir(temp,0);	

    	dprintf_dosfs(stddeb,
		"DOS_MakeDir: %c:\%s => %s",'A'+drive, dirname, temp);
	return 1;
}

/* DOS_ExpandToFullPath takes a dos-style filename and converts it
 * into a full path based on the current working directory.
 * (e.g., "foo.bar" => "d:\\moo\\foo.bar")
 */
void DOS_ExpandToFullPath(char *filename, int drive)
{
        char temp[256];

        dprintf_dosfs(stddeb, "DOS_ExpandToFullPath: Original = %s\n", filename);

        /* If the filename starts with '/' or '\',
         * don't bother -- we're already at the root.
         */
        if(filename[0] == '/' || filename[0] == '\\')
            return;

        strcpy(temp, DosDrives[drive].cwd);
        strcat(temp, filename);
        strcpy(filename, temp);

        dprintf_dosfs(stddeb, "                      Expanded = %s\n", temp); 
}

/* DOS_ExpandToFullUnixPath takes a unix filename and converts it
 * into a full path based on the current working directory.  Thus,
 * it's probably not a good idea to get a relative name, change the
 * working directory, and then convert it...
 */
void DOS_ExpandToFullUnixPath(char *filename)
{
        char temp[256];

        if(filename[0] == '/')
            return;

        getcwd(temp, 255);
        if(strncmp(filename, "./", 2))
                strcat(temp, filename + 1);
        else
        {
                strcat(temp, "/");
                strcat(temp, filename);
        }
        dprintf_dosfs(stddeb, "DOS_ExpandToFullUnixPath: %s => %s\n", filename, temp);
        strcpy(filename, temp);
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
		fprintf(stderr,"dosfs: cannot do statfs(%s)\n",
			DosDrives[drive].rootdir);
		return 0;
	}

	*size = info.f_bsize * info.f_blocks;
	*available = info.f_bavail * info.f_bsize;

	return 1;
}

char *DOS_FindFile(char *buffer, int buflen, char *filename, char **extensions, 
		char *path)
{
    char *workingpath, *dirname, *rootname, **e;
    DIR *d;
    struct dirent *f;
    int rootnamelen, found = 0;
    struct stat filestat;

    if (strchr(filename, '\\') != NULL)
    {
	strncpy(buffer, DOS_GetUnixFileName(filename), buflen);
	stat( buffer, &filestat);
	if (S_ISREG(filestat.st_mode))
	    return buffer;
	else
	    return NULL;
    }

    if (strchr(filename, '/') != NULL)
    {
	strncpy(buffer, filename, buflen);
	return buffer;
    }

    dprintf_dosfs(stddeb,"DOS_FindFile: looking for %s\n", filename);
    rootnamelen = strlen(filename);
    rootname = strdup(filename);
    ToUnix(rootname);
    workingpath = strdup(path);

    for(dirname = strtok(workingpath, ";"); 
	dirname != NULL;
	dirname = strtok(NULL, ";"))
    {
	if (strchr(dirname, '\\') != NULL)
		d = opendir( DOS_GetUnixFileName(dirname) );
	else
		d = opendir( dirname );

    	dprintf_dosfs(stddeb,"in %s\n",dirname);
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
				strncpy(buffer, DOS_GetUnixFileName(dirname), buflen);
			else
				strncpy(buffer, dirname, buflen);

			strncat(buffer, "/", buflen - strlen(buffer));
			strncat(buffer, f->d_name, buflen - strlen(buffer));

			stat(buffer, &filestat);
			if (S_ISREG(filestat.st_mode)) {
				closedir(d);
				free(rootname);
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
	static char name[256];

	if (filename)
		return filename;

	strcpy(name, WINE_INI_USER);
	ExpandTildeString(name);
	if ((fd = open(name, O_RDONLY)) != -1) {
		close(fd);
		filename = name;
		return(filename);
	}
	if ((fd = open(WINE_INI_GLOBAL, O_RDONLY)) != -1) {
		close(fd);
		filename = WINE_INI_GLOBAL;
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

	strcpy(name, DOS_GetUnixFileName(WindowsDirectory));
	strcat(name, "/");
	strcat(name, "win.ini");

	name = realloc(name, strlen(name) + 1);
	
	return name;
}

static int match(char *filename, char *filemask)
{
	int x, masklength = strlen(filemask);

    	dprintf_dosfs(stddeb, "match: %s, %s\n", filename, filemask);
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

	if ((unixdirname = DOS_GetUnixFileName(dosdirname)) == NULL)
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

    	dprintf_dosfs(stddeb,"DOS_opendir: %s -> %s\n", unixdirname, temp);

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
			return NULL;

		strcpy(de->filename, d->d_name);
		if (d->d_reclen > 12)
			de->filename[12] = '\0';
		
		ToDos(de->filename);
	} while ( !match(de->filename, de->filemask) );

	strcpy(temp,de->unixpath);
	strcat(temp,"/");
	strcat(temp,de->filename);
	ToUnix(temp + strlen(de->unixpath));

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
	if (de && de->inuse)
	{
		closedir(de->ds);
		de->inuse = 0;
	}
}
