static char RCSId[] = "$Id: wine.c,v 1.2 1993/07/04 04:04:21 root Exp root $";
static char Copyright[] = "Copyright  Robert J. Amstadt, 1993";

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#ifdef linux
#include <linux/unistd.h>
#include <linux/head.h>
#include <linux/ldt.h>
#include <linux/segment.h>
#endif
#include "neexe.h"
#include "segmem.h"
#include "prototypes.h"
#include "dlls.h"
#include "wine.h"
#include "windows.h"
#include "wineopts.h"
#include "arch.h"
#include "options.h"

/* #define DEBUG_FIXUP */

extern HANDLE CreateNewTask(HINSTANCE hInst);
extern int CallToInit16(unsigned long csip, unsigned long sssp, 
			unsigned short ds);
extern void CallTo32();

char *GetDosFileName(char *unixfilename);
char *GetModuleName(struct w_files * wpnt, int index, char *buffer);
extern unsigned char ran_out;
extern char WindowsPath[256];
char *WIN_ProgramName;

unsigned short WIN_StackSize;
unsigned short WIN_HeapSize;

struct  w_files * wine_files = NULL;

char **Argv;
int Argc;
HINSTANCE hSysRes;

static char *DLL_Extensions[] = { "dll", NULL };
static char *EXE_Extensions[] = { "exe", NULL };

/**********************************************************************
 *					myerror
 */
void
myerror(const char *s)
{
    if (s == NULL)
	perror("wine");
    else
	fprintf(stderr, "wine: %s\n", s);

    exit(1);
}

/**********************************************************************
 *					GetFilenameFromInstance
 */
char *
GetFilenameFromInstance(unsigned short instance)
{
    register struct w_files *w = wine_files;

    while (w && w->hinstance != instance)
	w = w->next;
    
    if (w)
	return w->filename;
    else
	return NULL;
}

struct w_files *
GetFileInfo(unsigned short instance)
{
    register struct w_files *w = wine_files;

    while (w && w->hinstance != instance)
	w = w->next;
    
    return w;
}

/**********************************************************************
 *
 * Load MZ Header
 */
void load_mz_header(int fd, struct mz_header_s *mz_header)
{
    if (read(fd, mz_header, sizeof(struct mz_header_s)) !=
	sizeof(struct mz_header_s))
    {
	myerror("Unable to read MZ header from file");
    }
}

int IsDLLLoaded(char *name)
{
	struct w_files *wpnt;

	if(FindDLLTable(name))
		return 1;

	for(wpnt = wine_files; wpnt; wpnt = wpnt->next)
		if(strcmp(wpnt->name, name) == 0)
			return 1;

	return 0;
}

/**********************************************************************
 *			LoadImage
 * Load one executable into memory
 */
HINSTANCE LoadImage(char *module, int filetype, int change_dir)
{
    unsigned int read_size;
    int i;
    struct w_files * wpnt, *wpnt1;
    unsigned int status;
    char buffer[256], header[2], modulename[64], *fullname;

    ExtractDLLName(module, modulename);

    /* built-in one ? */
    if (FindDLLTable(modulename)) {
	return GetModuleHandle(modulename);
    }
    
    /* already loaded ? */
    for (wpnt = wine_files ; wpnt ; wpnt = wpnt->next)
    	if (strcasecmp(wpnt->name, modulename) == 0)
    		return wpnt->hinstance;

    /*
     * search file
     */
    fullname = FindFile(buffer, sizeof(buffer), module, 
			(filetype == EXE ? EXE_Extensions : DLL_Extensions), 
			WindowsPath);
    if (fullname == NULL)
    {
    	fprintf(stderr, "LoadImage: I can't find %s.dll | %s.exe !\n",
		module, module);
	return 2;
    }

    fullname = GetDosFileName(fullname);
    WIN_ProgramName = strdup(fullname);
    
    fprintf(stderr,"LoadImage: loading %s (%s)\n           [%s]\n", 
	    module, buffer, WIN_ProgramName);

    if (change_dir && fullname)
    {
	char dirname[256];
	char *p;

	strcpy(dirname, fullname);
	p = strrchr(dirname, '\\');
	*p = '\0';

	DOS_SetDefaultDrive(dirname[0] - 'A');
	DOS_ChangeDir(dirname[0] - 'A', dirname + 2);
    }

    /* First allocate a spot to store the info we collect, and add it to
     * our linked list.
     */

    wpnt = (struct w_files *) malloc(sizeof(struct w_files));
    if(wine_files == NULL)
      wine_files = wpnt;
    else {
      wpnt1 = wine_files;
      while(wpnt1->next) wpnt1 =  wpnt1->next;
      wpnt1->next  = wpnt;
    };
    wpnt->next = NULL;
    wpnt->resnamtab = (RESNAMTAB *) -1;

    /*
     * Open file for reading.
     */
    wpnt->fd = open(buffer, O_RDONLY);
    if (wpnt->fd < 0)
	return 2;

    /* 
     * Establish header pointers.
     */
    wpnt->filename = strdup(buffer);
    wpnt->name = strdup(modulename);

/*    if(module) {
    	wpnt->name = strdup(module);
	ToDos(wpnt->name);
    }*/

    /* read mz header */
    wpnt->mz_header = (struct mz_header_s *) malloc(sizeof(struct mz_header_s));;
    status = lseek(wpnt->fd, 0, SEEK_SET);
    load_mz_header (wpnt->fd, wpnt->mz_header);
    if (wpnt->mz_header->must_be_0x40 != 0x40)
	myerror("This is not a Windows program");

    /* read first two bytes to determine filetype */
    status = lseek(wpnt->fd, wpnt->mz_header->ne_offset, SEEK_SET);
    read(wpnt->fd, &header, sizeof(header));

    if (header[0] == 'N' && header[1] == 'E')
	return (LoadNEImage(wpnt));

    if (header[0] == 'P' && header[1] == 'E') {
      printf("win32 applications are not supported");
      return 14;
    }

    fprintf(stderr, "wine: (%s) unknown fileformat !\n", wpnt->filename);

    return 14;
}


#ifndef WINELIB
/**********************************************************************
 *					main
 */
int _WinMain(int argc, char **argv)
{
	int segment;
	char *p;
	char *sysresname;
	char filename[256];
	HANDLE		hTaskMain;
	HINSTANCE	hInstMain;
#ifdef WINESTAT
	char * cp;
#endif
	struct w_files * wpnt;
	int cs_reg, ds_reg, ss_reg, ip_reg, sp_reg;
	int rv;

	Argc = argc - 1;
	Argv = argv + 1;

	if (strchr(Argv[0], '\\') || strchr(Argv[0],'/')) {
            for (p = Argv[0] + strlen(Argv[0]); *p != '\\' && *p !='/'; p--)
		/* NOTHING */;
		
	    strncpy(filename, Argv[0], p - Argv[0]);
	    filename[p - Argv[0]] = '\0';
	    strcat(WindowsPath, ";");
	    strcat(WindowsPath, filename);
	}
	
	if ((hInstMain = LoadImage(Argv[0], EXE, 1)) < 32) {
		fprintf(stderr, "wine: can't load %s!.\n", Argv[0]);
		exit(1);
	}
	hTaskMain = CreateNewTask(hInstMain);
	printf("_WinMain // hTaskMain=%04X hInstMain=%04X !\n", hTaskMain, hInstMain);

	GetPrivateProfileString("wine", "SystemResources", "sysres.dll", 
				filename, sizeof(filename), WINE_INI);

	hSysRes = LoadImage(filename, DLL, 0);
	if (hSysRes < 32) {
		fprintf(stderr, "wine: can't load %s!.\n", filename);
		exit(1);
	} else
 	    printf("System Resources Loaded // hSysRes='%04X'\n", hSysRes);
	
    /*
     * Fixup references.
     */
/*    wpnt = wine_files;
    for(wpnt = wine_files; wpnt; wpnt = wpnt->next)
	for (segment = 0; segment < wpnt->ne_header->n_segment_tab; segment++)
	    if (FixupSegment(wpnt, segment) < 0)
		myerror("fixup failed.");
*/

#ifdef WINESTAT
    cp = strrchr(argv[0], '/');
    if(!cp) cp = argv[0];
	else cp++;
    if(strcmp(cp,"winestat") == 0) {
	    winestat();
	    exit(0);
    };
#endif

    /*
     * Initialize signal handling.
     */
    init_wine_signals();

    /*
     * Fixup stack and jump to start.
     */
    WIN_StackSize = wine_files->ne_header->stack_length;
    WIN_HeapSize = wine_files->ne_header->local_heap_length;

    ds_reg = (wine_files->
	      selector_table[wine_files->ne_header->auto_data_seg-1].selector);
    cs_reg = wine_files->selector_table[wine_files->ne_header->cs-1].selector;
    ip_reg = wine_files->ne_header->ip;
    ss_reg = wine_files->selector_table[wine_files->ne_header->ss-1].selector;
    sp_reg = wine_files->ne_header->sp;

    if (Options.debug) wine_debug(0, NULL);

    rv = CallToInit16(cs_reg << 16 | ip_reg, ss_reg << 16 | sp_reg, ds_reg);
    printf ("rv = %x\n", rv);
}

void InitDLL(struct w_files *wpnt)
{
	int cs_reg, ds_reg, ip_reg, rv;
	/* 
	 * Is this a library? 
	 */
	if (wpnt->ne_header->format_flags & 0x8000)
	{
	    if (!(wpnt->ne_header->format_flags & 0x0001))
	    {
		/* Not SINGLEDATA */
		fprintf(stderr, "Library is not marked SINGLEDATA\n");
		exit(1);
	    }

	    ds_reg = wpnt->selector_table[wpnt->
					  ne_header->auto_data_seg-1].selector;
	    cs_reg = wpnt->selector_table[wpnt->ne_header->cs-1].selector;
	    ip_reg = wpnt->ne_header->ip;

	    if (cs_reg) {
		fprintf(stderr, "Initializing %s, cs:ip %04x:%04x, ds %04x\n", 
		    wpnt->name, cs_reg, ip_reg, ds_reg);
	    	    
		rv = CallTo16(cs_reg << 16 | ip_reg, ds_reg);
		printf ("rv = %x\n", rv);
	    } else
		printf("%s skipped\n");
	}
}

void InitializeLoadedDLLs(struct w_files *wpnt)
{
    static flagReadyToRun = 0;
    struct w_files *final_wpnt;
    struct w_files * wpnt;

    if (wpnt == NULL)
    {
	flagReadyToRun = 1;
	fprintf(stderr, "Initializing DLLs\n");
    }
    
    if (!flagReadyToRun)
	return;

#if 1
    if (wpnt != NULL)
	fprintf(stderr, "Initializing %s\n", wpnt->name);
#endif

    /*
     * Initialize libraries
     */
    if (!wpnt)
    {
	wpnt = wine_files;
	final_wpnt = NULL;
    }
    else
    {
	final_wpnt = wpnt->next;
    }
    
    for( ; wpnt != final_wpnt; wpnt = wpnt->next)
	InitDLL(wpnt);
}
#endif
