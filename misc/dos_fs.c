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
#include "task.h"
#include "stddebug.h"
#include "debug.h"

#define WINE_INI_USER "~/.winerc"
#define MAX_DOS_DRIVES	26

extern char WindowsDirectory[256], SystemDirectory[256],TempDirectory[256];

char WindowsPath[256];

static int max_open_dirs = 0;
static int CurrentDrive = 2;

struct DosDriveStruct {			/*  eg: */
	char 		*rootdir;	/*  /usr/windows 	*/
	char 		cwd[256];	/*  /			*/
	char 		label[13];	/*  DRIVE-A		*/
	unsigned int	serialnumber;	/*  ABCD5678		*/
	int 		disabled;	/*  0			*/
};

static struct DosDriveStruct DosDrives[MAX_DOS_DRIVES];
static struct dosdirent *DosDirs=NULL;

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

/* Simplify the path in "name" by removing  "//"'s and
   ".."'s in names like "/usr/bin/../lib/test" */
static void DOS_SimplifyPath(char *name)
{
  char *l,*p;
  BOOL changed;

  dprintf_dosfs(stddeb,"SimplifyPath: Before %s\n",name);
  do {
    changed = FALSE;
    while ((l = strstr(name,"//"))) {
      strcpy(l,l+1); changed = TRUE;
    } 
    while ((l = strstr(name,"/../"))) {
      *l = 0;
      p = strrchr(name,'/');
      if (p == NULL) p = name;
      strcpy(p,l+3);
      changed = TRUE;
    }
  } while (changed);
  dprintf_dosfs(stddeb,"SimplifyPath: After %s\n",name);
}


/* ChopOffSlash takes care to strip directory slashes from the
 * end off the path name, but leaves a single slash. Multiple
 * slashes at the end of a path are all removed.
 */

void ChopOffSlash(char *path)
{
    char *p = path + strlen(path) - 1;
    while ((*p == '\\') && (p > path)) *p-- = '\0';
}

void ToUnix(char *s)
{
    while(*s){
	if (*s == '\\') *s = '/';
	*s=tolower(*s); /* umsdos fs can't read files without :( */
	s++;
    }
}

void ToDos(char *s)
{
    while(*s){
	if (*s == '/') *s = '\\';
	s++;
    }
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
    strcpy(DosDrives[25].label, "UNIX-FS");
    DosDrives[25].serialnumber = 0x12345678;
    DosDrives[25].disabled = 0;
    
    /* Get the startup directory and try to map it to a DOS drive
     * and directory.  (i.e., if we start in /dos/windows/word and
     * drive C is defined as /dos, the starting wd for C will be
     * /windows/word)  Also set the default drive to whatever drive
     * corresponds to the directory we started in.
     */

    for (x=0; x!=MAX_DOS_DRIVES; x++) 
        if (DosDrives[x].rootdir != NULL) 
	    strcpy( DosDrives[x].cwd, "/" );

    getcwd(temp, 254);
    strcat(temp, "/");      /* For DOS_GetDosFileName */
    strcpy(DosDrives[25].cwd, temp );
    strcpy(temp, DOS_GetDosFileName(temp));
    if(temp[0] != 'Z')
    {
	ToUnix(temp + 2);
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
			  DosDrives[x].disabled);	
	}
    }
    
    for (x=0; x!=max_open_dirs ; x++)
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
    
    return equipment;
}

int DOS_ValidDrive(int drive)
{
    dprintf_dosfs(stddeb,"ValidDrive %c (%d)\n",'A'+drive,drive);

    if (drive < 0 || drive >= MAX_DOS_DRIVES) return 0;
    if (DosDrives[drive].rootdir == NULL) return 0;
    if (DosDrives[drive].disabled) return 0;

    dprintf_dosfs(stddeb, " -- valid\n");
    return 1;
}

static void DOS_GetCurrDir_Unix(char *buffer, int drive)
{
    TDB *pTask = (TDB *)GlobalLock(GetCurrentTask());
    
    if (pTask != NULL && (pTask->curdrive & ~0x80) == drive) {
	strcpy(buffer, pTask->curdir);
	ToUnix(buffer);
    } else {
	strcpy(buffer, DosDrives[drive].cwd);
    }
}

char *DOS_GetCurrentDir(int drive)
{ 
    static char temp[256];

    if (!DOS_ValidDrive(drive)) return 0;

    DOS_GetCurrDir_Unix(temp, drive);
    DOS_SimplifyPath( temp );
    ToDos(temp);
    ChopOffSlash(temp);

    dprintf_dosfs(stddeb,"DOS_GetCWD: %c:%s\n", 'A'+drive, temp);
    return temp + 1;
}

char *DOS_GetUnixFileName(const char *dosfilename)
{ 
    /*   a:\windows\system.ini  =>  /dos/windows/system.ini */
    
    /* FIXME: should handle devices here (like LPT: or NUL:) */
    
    static char dostemp[256], temp[256];
    int drive = DOS_GetDefaultDrive();
    
    if (dosfilename[0] && dosfilename[1] == ':')
    {
	drive = toupper(*dosfilename) - 'A';
	dosfilename += 2;
    }    
    if (!DOS_ValidDrive(drive)) return NULL;

    strncpy( dostemp, dosfilename, 255 );
    dostemp[255] = 0;
    ToUnix(dostemp);
    strcpy(temp, DosDrives[drive].rootdir);
    if (dostemp[0] != '/') {
	DOS_GetCurrDir_Unix(temp+strlen(temp), drive);
    }
    strcat(temp, dostemp);
    DOS_SimplifyPath(temp);
	
    dprintf_dosfs(stddeb,"GetUnixFileName: %s => %s\n", dosfilename, temp);
    return temp;
}

/* Note: This function works on directories as well as long as
 * the directory ends in a slash.
 */
char *DOS_GetDosFileName(char *unixfilename)
{ 
    int i;
    static char temp[256], temp2[256];
    /*   /dos/windows/system.ini => c:\windows\system.ini */
    
    dprintf_dosfs(stddeb,"DOS_GetDosFileName: %s\n", unixfilename);
    if (unixfilename[0] == '/') {
	strncpy(temp, unixfilename, 255);
	temp[255] = 0;
    } else {
	/* Expand it if it's a relative name. */
	getcwd(temp, 255);
	if(strncmp(unixfilename, "./", 2) != 0) {
	    strcat(temp, unixfilename + 1);
	} else {	    
	    strcat(temp, "/");
	    strcat(temp, unixfilename);
	}
    }
    for (i = 0 ; i < MAX_DOS_DRIVES; i++) {
	if (DosDrives[i].rootdir != NULL) {
	    int len = strlen(DosDrives[i].rootdir);
	    dprintf_dosfs(stddeb, "  check %c:%s\n", i+'A', DosDrives[i].rootdir);
	    if (strncmp(DosDrives[i].rootdir, temp, len) == 0 && temp[len] == '/')
	    {
		sprintf(temp2, "%c:%s", 'A' + i, temp+len);
		ToDos(temp2+2);
		return temp2;
	    }	
	}
    }
    sprintf(temp, "Z:%s", unixfilename);
    ToDos(temp+2);
    return temp;
}

int DOS_ValidDirectory(int drive, char *name)
{
    char temp[256];
    struct stat s;
    
    strcpy(temp, DosDrives[drive].rootdir);
    strcat(temp, name);
    if (stat(temp, &s)) return 0;
    if (!S_ISDIR(s.st_mode)) return 0;
    dprintf_dosfs(stddeb, "==> OK\n");
    return 1;
}

int DOS_GetDefaultDrive(void)
{
    TDB *pTask = (TDB *)GlobalLock(GetCurrentTask());
    int drive = pTask == NULL ? CurrentDrive : pTask->curdrive & ~0x80;
    
    dprintf_dosfs(stddeb,"GetDefaultDrive (%c)\n",'A'+drive);
    return drive;
}

void DOS_SetDefaultDrive(int drive)
{
    TDB *pTask = (TDB *)GlobalLock(GetCurrentTask());
    
    dprintf_dosfs(stddeb,"SetDefaultDrive to %c:\n",'A'+drive);
    if (DOS_ValidDrive(drive) && drive != DOS_GetDefaultDrive()) {
	if (pTask == NULL) CurrentDrive = drive;
	else {
	    char temp[256];
	    pTask->curdrive = drive | 0x80;
	    strcpy(temp, DosDrives[drive].rootdir);
	    strcat(temp, DosDrives[drive].cwd);
	    strcpy(temp, DOS_GetDosFileName(temp));
	    dprintf_dosfs(stddeb, "  curdir = %s\n", temp);
	    if (strlen(temp)-2 < sizeof(pTask->curdir)) strcpy(pTask->curdir, temp+2);
	    else fprintf(stderr, "dosfs: curdir too long\n");
	}
    }
}

int DOS_DisableDrive(int drive)
{
    if (drive >= MAX_DOS_DRIVES) return 0;
    if (DosDrives[drive].rootdir == NULL) return 0;

    DosDrives[drive].disabled = 1;
    return 1;
}

int DOS_EnableDrive(int drive)
{
    if (drive >= MAX_DOS_DRIVES) return 0;
    if (DosDrives[drive].rootdir == NULL) return 0;

    DosDrives[drive].disabled = 0;
    return 1;
}

int DOS_ChangeDir(int drive, char *dirname)
{
    TDB *pTask = (TDB *)GlobalLock(GetCurrentTask());
    char temp[256];
    
    if (!DOS_ValidDrive(drive)) return 0;

    if (dirname[0] == '\\') {
	strcpy(temp, dirname);
    } else {
	DOS_GetCurrDir_Unix(temp, drive);
	strcat(temp, dirname);
    }
    ToUnix(temp);
    strcat(temp, "/");
    DOS_SimplifyPath(temp);
    dprintf_dosfs(stddeb,"DOS_SetCWD: %c: %s ==> %s\n", 'A'+drive, dirname, temp);

    if (!DOS_ValidDirectory(drive, temp)) return 0;
    strcpy(DosDrives[drive].cwd, temp);
    if (pTask != NULL && DOS_GetDefaultDrive() == drive) {
	strcpy(temp, DosDrives[drive].rootdir);
	strcat(temp, DosDrives[drive].cwd);
	strcpy(temp, DOS_GetDosFileName(temp));
	dprintf_dosfs(stddeb, "  curdir = %s\n", temp);
	if (strlen(temp)-2 < sizeof(pTask->curdir)) strcpy(pTask->curdir, temp+2);
	else fprintf(stderr, "dosfs: curdir too long\n");
    }
    return 1;
}

int DOS_MakeDir(int drive, char *dirname)
{
    char temp[256], currdir[256];
    
    if (!DOS_ValidDrive(drive)) return 0;	

    strcpy(temp, DosDrives[drive].rootdir);
    DOS_GetCurrDir_Unix(currdir, drive);
    strcat(temp, currdir);
    strcat(temp, dirname);
    ToUnix(temp);
    DOS_SimplifyPath(temp);
    mkdir(temp,0);	
    
    dprintf_dosfs(stddeb, "DOS_MakeDir: %c:\%s => %s",'A'+drive, dirname, temp);
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

	return DosDrives[drive].label;
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
    int rootnamelen;
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
		if (strcasecmp(rootname, f->d_name) != 0) {
		    if (strncasecmp(rootname, f->d_name, rootnamelen) != 0
		      || extensions == NULL 
		      || f->d_name[rootnamelen] != '.')
		        continue;

		    for (e = extensions; *e != NULL; e++) {
			if (strcasecmp(*e, f->d_name + rootnamelen + 1) == 0)
			    break;
		    }
		    if (*e == NULL) continue;
		}

		if (strchr(dirname, '\\') != NULL) {
		    strncpy(buffer, DOS_GetUnixFileName(dirname), buflen);
		} else {
		    strncpy(buffer, dirname, buflen);
		}

		strncat(buffer, "/", buflen - strlen(buffer));
		strncat(buffer, f->d_name, buflen - strlen(buffer));
		
		stat(buffer, &filestat);
		if (S_ISREG(filestat.st_mode)) {
		    closedir(d);
		    free(rootname);
		    DOS_SimplifyPath(buffer);
		    return buffer;
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
		return filename;
	}
	if ((fd = open(WINE_INI_GLOBAL, O_RDONLY)) != -1) {
		close(fd);
		filename = WINE_INI_GLOBAL;
		return filename;
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
        char name[12], mask[12];
        int i;

        dprintf_dosfs(stddeb, "match: %s, %s\n", filename, filemask);
 
	for( i=0; i<11; i++ ) {
	  name[i] = ' ';
	  mask[i] = ' ';
	}
	name[11] = 0;
	mask[11] = 0; 
 
	for( i=0; i<8; i++ )
	  if( !(*filename) || *filename == '.' )
  	    break;
	  else
            name[i] = toupper( *filename++ );
        while( *filename && *filename != '.' )
	  filename++;
        if( *filename )
	  filename++;
        for( i=8; i<11; i++ )
	  if( !(*filename) )
	    break;
	  else
	    name[i] = toupper( *filename++ );

	for( i=0; i<8; i++ )
	  if( !(*filemask) || *filemask == '.' )
	    break;
	  else if( *filemask == '*' ) {
	    int j;
	    for( j=i; j<8; j++ )
	      mask[j] = '?';
	    break;
          }
	  else
	    mask[i] = toupper( *filemask++ );
	while( *filemask && *filemask != '.' )
	  filemask++;	   
	if( *filemask )
	  filemask++;
	for( i=8; i<11; i++ )
	  if( !(*filemask) )
	    break;
	  else if (*filemask == '*' ) {
	    int j;
	    for( j=i; j<11; j++ )
	      mask[j] = '?';
	    break;
	  }
	  else
	    mask[i] = toupper( *filemask++ );
	
    	dprintf_dosfs(stddeb, "changed to: %s, %s\n", name, mask);

	for( i=0; i<11; i++ )
	  if( ( name[i] != mask[i] ) && ( mask[i] != '?' ) )
	    return 0;

	return 1;
}

struct dosdirent *DOS_opendir(char *dosdirname)
{
    int x, len;
    char *unixdirname;
    char dirname[256];
    DIR  *ds;
    
    if ((unixdirname = DOS_GetUnixFileName(dosdirname)) == NULL) return NULL;
    
    len = strrchr(unixdirname, '/') - unixdirname + 1;
    strncpy(dirname, unixdirname, len);
    dirname[len] = 0;
    unixdirname = strrchr(unixdirname, '/') + 1;

    for (x=0; x <= max_open_dirs; x++) {
	if (x == max_open_dirs) {
	    if (DosDirs) {
		DosDirs=(struct dosdirent*)realloc(DosDirs,(++max_open_dirs)*sizeof(DosDirs[0]));
	    } else {
		DosDirs=(struct dosdirent*)malloc(sizeof(DosDirs[0]));
		max_open_dirs=1;
	    }
	    break; /* this one is definitely not in use */
	}
	if (!DosDirs[x].inuse) break;
	if (strcmp(DosDirs[x].unixpath, dirname) == 0) break;
    }
    
    strncpy(DosDirs[x].filemask, unixdirname, 12);
    DosDirs[x].filemask[12] = 0;
    dprintf_dosfs(stddeb,"DOS_opendir: %s / %s\n", unixdirname, dirname);

    DosDirs[x].inuse = 1;
    strcpy(DosDirs[x].unixpath, dirname);
    DosDirs[x].entnum = 0;

    if ((ds = opendir(dirname)) == NULL) return NULL;
    if (-1==(DosDirs[x].telldirnum=telldir(ds))) {
	closedir(ds);
	return NULL;
    }
    if (-1==closedir(ds)) return NULL;

    return &DosDirs[x];
}


struct dosdirent *DOS_readdir(struct dosdirent *de)
{
	char temp[256];
	struct dirent *d;
	struct stat st;
	DIR	*ds;

	if (!de->inuse)
		return NULL;
	if (!(ds=opendir(de->unixpath))) return NULL;
	seekdir(ds,de->telldirnum); /* returns no error value. strange */
   
        if (de->search_attribute & FA_LABEL)  {	
	    int drive;
	    de->search_attribute &= ~FA_LABEL; /* don't find it again */
	    for(drive = 0; drive < MAX_DOS_DRIVES; drive++) {
		if (DosDrives[drive].rootdir != NULL &&
		    strcmp(DosDrives[drive].rootdir, de->unixpath) == 0)
		{
		    strcpy(de->filename, DOS_GetVolumeLabel(drive));
		    de->attribute = FA_LABEL;
		    return de;
		}
	    }
	}
    
	do {
	    if ((d = readdir(ds)) == NULL)  {
		de->telldirnum=telldir(ds);
		closedir(ds);
		return NULL;
	    }

	    de->entnum++;   /* Increment the directory entry number */
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

	de->telldirnum = telldir(ds);
	closedir(ds);
	return de;
}

void DOS_closedir(struct dosdirent *de)
{
	if (de && de->inuse)
		de->inuse = 0;
}

char *DOS_GetRedirectedDir(int drive)
{
    if(DOS_ValidDrive(drive))
        return (DosDrives[drive].rootdir);
    else
        return ("/");
}
