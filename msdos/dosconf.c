/*
 * DOS CONFIG.SYS parser
 *
 * Copyright 1998 Andreas Mohr 
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "winbase.h"
#include "msdos.h"
#include "debug.h"
#include "options.h"
#include "file.h"


static int DOSCONF_Device(char **confline);
static int DOSCONF_Dos(char **confline);
static int DOSCONF_Fcbs(char **confline);
static int DOSCONF_Break(char **confline);
static int DOSCONF_Files(char **confline);
static int DOSCONF_Install(char **confline);
static int DOSCONF_Lastdrive(char **confline);
static int DOSCONF_Menu(char **confline);
static int DOSCONF_Include(char **confline);
static int DOSCONF_Country(char **confline);
static int DOSCONF_Numlock(char **confline);
static int DOSCONF_Switches(char **confline);
static int DOSCONF_Shell(char **confline);
static int DOSCONF_Stacks(char **confline);
static int DOSCONF_Buffers(char **confline);
static void DOSCONF_Parse(char *menuname);

DOSCONF DOSCONF_config = 
{
    'E',  /* lastdrive */
    0,    /* brk_flag */
    8,    /* files */
    9,    /* stacks_nr */
    256,  /* stacks_sz */
    15,   /* buf */
    1,    /* buf2 */
    4,    /* fcbs */
    0,    /* flags */
    NULL, /* shell */
    NULL  /* country */
};

typedef struct {
    const char *tag_name;
    int (*tag_handler)(char **p);
    void *data;
} TAG_ENTRY;


/*
 * see
 * http://egeria.cm.cf.ac.uk/User/P.L.Poulain/project/internal/allinter.htm
 * or
 * http://www.csulb.edu/~murdock/dosindex.html
 */

static const TAG_ENTRY tag_entries[] =
{
    { ";", NULL },
    { "REM ", NULL },
    { "DEVICE", DOSCONF_Device },
    { "[", DOSCONF_Menu },
    { "SUBMENU", NULL },
    { "MENUDEFAULT", DOSCONF_Menu },
    { "INCLUDE", DOSCONF_Include },

    { "INSTALL", DOSCONF_Install },
    { "DOS", DOSCONF_Dos },
    { "FCBS", DOSCONF_Fcbs },
    { "BREAK", DOSCONF_Break },
    { "FILES", DOSCONF_Files },
    { "SHELL", DOSCONF_Shell },    
    { "STACKS", DOSCONF_Stacks },
    { "BUFFERS", DOSCONF_Buffers },
    { "COUNTRY", DOSCONF_Country },
    { "NUMLOCK", DOSCONF_Numlock },
    { "SWITCHES", DOSCONF_Switches },
    { "LASTDRIVE", DOSCONF_Lastdrive }
};

static FILE *cfg_fd;

static char *menu_default = NULL;
static int menu_in_listing = 0;		/* we are in the [menu] section */
static int menu_skip = 0;				/* the current menu gets skipped */


static void DOSCONF_skip(char **pconfline)
{
    char *p;
    
    p = *pconfline;
    while ( (*p == ' ') || (*p == '\t') ) p++;
    *pconfline = p;
}
		
static int DOSCONF_JumpToEntry(char **pconfline, char separator)
{
    char *p;
    
    p = *pconfline;
    while ( (*p != separator) && (*p != '\0') ) p++;

    if (*p != separator)
	return 0;
    else p++;

    while ( (*p == ' ') || (*p == '\t') ) p++;
    *pconfline = p;
    return 1;
}

static int DOSCONF_Device(char **confline)
{
    int loadhigh = 0;

    *confline += 6; /* strlen("DEVICE") */
    if (!(strncasecmp(*confline, "HIGH", 4)))
    {
	loadhigh = 1;
	*confline += 4;
	/* FIXME: get DEVICEHIGH parameters if avail ? */
    }
    if (!(DOSCONF_JumpToEntry(confline, '='))) return 0;
    TRACE(profile, "Loading device '%s'\n", *confline);
#if 0
    DOSMOD_LoadDevice(*confline, loadhigh);
#endif
    return 1;
}

static int DOSCONF_Dos(char **confline)
{
    *confline += 3; /* strlen("DOS") */
    if (!(DOSCONF_JumpToEntry(confline, '='))) return 0;
    while (**confline != '\0')
    {
	if (!(strncasecmp(*confline, "HIGH", 4)))
	{
	    DOSCONF_config.flags |= DOSCONF_MEM_HIGH;
	    *confline += 4;
	}
	else
	if (!(strncasecmp(*confline, "UMB", 3)))
	{
	    DOSCONF_config.flags |= DOSCONF_MEM_UMB;
	    *confline += 3;
	}
        else (*confline)++;
	DOSCONF_JumpToEntry(confline, ',');
    }
    TRACE(profile, "DOSCONF_Dos: HIGH is %d, UMB is %d\n",
    	(DOSCONF_config.flags & DOSCONF_MEM_HIGH) != 0, (DOSCONF_config.flags & DOSCONF_MEM_UMB) != 0);
    return 1;
}

static int DOSCONF_Fcbs(char **confline)
{
    *confline += 4; /* strlen("FCBS") */
    if (!(DOSCONF_JumpToEntry(confline, '='))) return 0;
    DOSCONF_config.fcbs = atoi(*confline);
    if (DOSCONF_config.fcbs > 255)
    {
		MSG("The FCBS value in the config.sys file is too high ! Setting to 255.\n");
		DOSCONF_config.fcbs = 255;
    }
    TRACE(profile, "DOSCONF_Fcbs returning %d\n", DOSCONF_config.fcbs);
    return 1;
}

static int DOSCONF_Break(char **confline)
{
    *confline += 5; /* strlen("BREAK") */
    if (!(DOSCONF_JumpToEntry(confline, '='))) return 0;
    if (!(strcasecmp(*confline, "ON")))
        DOSCONF_config.brk_flag = 1;
    TRACE(profile, "BREAK is %d\n", DOSCONF_config.brk_flag);
    return 1;
}

static int DOSCONF_Files(char **confline)
{
    *confline += 5; /* strlen("FILES") */
    if (!(DOSCONF_JumpToEntry(confline, '='))) return 0;
    DOSCONF_config.files = atoi(*confline);
    if (DOSCONF_config.files > 255)
    {
    	MSG("The FILES value in the config.sys file is too high ! Setting to 255.\n");
        DOSCONF_config.files = 255;
    }
    if (DOSCONF_config.files < 8)
    {
    	MSG("The FILES value in the config.sys file is too low ! Setting to 8.\n");
        DOSCONF_config.files = 8;
    }
    TRACE(profile, "DOSCONF_Files returning %d\n", DOSCONF_config.files);
    return 1;
}

static int DOSCONF_Install(char **confline)
{
    int loadhigh = 0;

    *confline += 7; /* strlen("INSTALL") */
    if (!(DOSCONF_JumpToEntry(confline, '='))) return 0;
    TRACE(profile, "Installing '%s'\n", *confline);
#if 0
    DOSMOD_Install(*confline, loadhigh);
#endif
    return 1;
}

static int DOSCONF_Lastdrive(char **confline)
{
    *confline += 9; /* strlen("LASTDRIVE") */
    if (!(DOSCONF_JumpToEntry(confline, '='))) return 0;
    DOSCONF_config.lastdrive = toupper(**confline);
    TRACE(profile, "Lastdrive %c\n", DOSCONF_config.lastdrive);
    return 1;
}

static int DOSCONF_Country(char **confline)
{
    *confline += 7; /* strlen("COUNTRY") */
    if (!(DOSCONF_JumpToEntry(confline, '='))) return 0;
    TRACE(profile, "Country '%s'\n", *confline);
    if (DOSCONF_config.country == NULL)
		DOSCONF_config.country = malloc(strlen(*confline) + 1);
    strcpy(DOSCONF_config.country, *confline);
    return 1;
}

static int DOSCONF_Numlock(char **confline)
{
    *confline += 7; /* strlen("NUMLOCK") */
    if (!(DOSCONF_JumpToEntry(confline, '='))) return 0;
    if (!(strcasecmp(*confline, "ON")))
        DOSCONF_config.flags |= DOSCONF_NUMLOCK;
    TRACE(profile, "NUMLOCK is %d\n", (DOSCONF_config.flags & DOSCONF_NUMLOCK) != 0);
    return 1;
}

static int DOSCONF_Switches(char **confline)
{
    char *p;

    *confline += 8; /* strlen("SWITCHES") */
    if (!(DOSCONF_JumpToEntry(confline, '='))) return 0;
    p = strtok(*confline, "/");
    do
    {
	if ( toupper(*p) == 'K')
	    DOSCONF_config.flags |= DOSCONF_KEYB_CONV;
    }
    while ((p = strtok(NULL, "/")));
    TRACE(profile, "'Force conventional keyboard' is %d\n",
		(DOSCONF_config.flags & DOSCONF_KEYB_CONV) != 0);
    return 1;
}

static int DOSCONF_Shell(char **confline)
{
    *confline += 5; /* strlen("SHELL") */
    if (!(DOSCONF_JumpToEntry(confline, '='))) return 0;
    TRACE(profile, "Shell '%s'\n", *confline);
    if (DOSCONF_config.shell == NULL)
		DOSCONF_config.shell = malloc(strlen(*confline) + 1);
    strcpy(DOSCONF_config.shell, *confline);
    return 1;
}

static int DOSCONF_Stacks(char **confline)
{

    *confline += 6; /* strlen("STACKS") */
    if (!(DOSCONF_JumpToEntry(confline, '='))) return 0;
    DOSCONF_config.stacks_nr = atoi(strtok(*confline, ","));
    DOSCONF_config.stacks_sz = atoi((strtok(NULL, ",")));
    TRACE(profile, "%d stacks of size %d\n",
          DOSCONF_config.stacks_nr, DOSCONF_config.stacks_sz);
    return 1;
}

static int DOSCONF_Buffers(char **confline)
{
    char *p;

    *confline += 7; /* strlen("BUFFERS") */
    if (!(DOSCONF_JumpToEntry(confline, '='))) return 0;
    p = strtok(*confline, ",");
    DOSCONF_config.buf = atoi(p);
    if ((p = strtok(NULL, ",")))
        DOSCONF_config.buf2 = atoi(p);
    TRACE(profile, "%d primary buffers, %d secondary buffers\n",
          DOSCONF_config.buf, DOSCONF_config.buf2);
    return 1;
}

static int DOSCONF_Menu(char **confline)
{
    if (!(strncasecmp(*confline, "[MENU]", 6)))
	menu_in_listing = 1;
    else
    if ((!(strncasecmp(*confline, "[COMMON]", 8)))
    || (!(strncasecmp(*confline, "[WINE]", 6))))
	menu_skip = 0;
    else
    if (**confline == '[')
    {
	(*confline)++;
	if ((menu_default)
	&& (!(strncasecmp(*confline, menu_default, strlen(menu_default)))))
    {
		free(menu_default);
		menu_default = NULL;
	    menu_skip = 0;
    }
	else
	    menu_skip = 1;
	menu_in_listing = 0;
    }
    else
    if (!(strncasecmp(*confline, "menudefault", 11)) && (menu_in_listing))
    {
	if (!(DOSCONF_JumpToEntry(confline, '='))) return 0;
    *confline = strtok(*confline, ",");
    menu_default = malloc(strlen(*confline) + 1);
	strcpy(menu_default, *confline);
    }
    return 1;
}

static int DOSCONF_Include(char **confline)
{
    fpos_t oldpos;
    char *temp;
    
    *confline += 7; /* strlen("INCLUDE") */
    if (!(DOSCONF_JumpToEntry(confline, '='))) return 0;
    fgetpos(cfg_fd, &oldpos);
    fseek(cfg_fd, 0, SEEK_SET);
    TRACE(profile, "Including menu '%s'\n", *confline);
    temp = malloc(strlen(*confline) + 1);
    strcpy(temp, *confline);
    DOSCONF_Parse(temp);
	free(temp);
    fsetpos(cfg_fd, &oldpos);
    return 1;
}

static void DOSCONF_Parse(char *menuname)
{
   char confline[256];
   char *p, *trail;
   int i;
    
    if (menuname != NULL) /* we need to jump to a certain sub menu */
    {
	while (fgets(confline, 255, cfg_fd))
	{
	     p = confline;
	     DOSCONF_skip(&p);
	     if (*p == '[')
	     {
		p++;
		if (!(trail = strrchr(p, ']')))
		    return;
		if (!(strncasecmp(p, menuname, (int)trail - (int)p)))
		    break;
	     }
	}
    }

    while (fgets(confline, 255, cfg_fd))
    {
	p = confline;
	DOSCONF_skip(&p);

	if ((menuname) && (*p == '['))
	    /* we were handling a specific sub menu, but now next menu begins */
	    break;

	if ((trail = strrchr(confline, '\n')))
		*trail = '\0';
	if ((trail = strrchr(confline, '\r')))
		*trail = '\0';
	if (!(menu_skip))
	{
	    for (i = 0; i < sizeof(tag_entries) / sizeof(TAG_ENTRY); i++)
		if (!(strncasecmp(p, tag_entries[i].tag_name,
		   strlen(tag_entries[i].tag_name))))
		{
		    TRACE(profile, "tag '%s'\n", tag_entries[i].tag_name);
		    if (tag_entries[i].tag_handler != NULL)
			    tag_entries[i].tag_handler(&p);
			break;
		}
	}
	else /* the current menu gets skipped */
	DOSCONF_Menu(&p);
    }
}

int DOSCONF_ReadConfig(void)
{
    char buffer[256];
    DOS_FULL_NAME fullname;
    char *filename, *menuname;
    int ret = 1;

    PROFILE_GetWineIniString( "wine", "config.sys", "", buffer, sizeof(buffer) );
    filename = strtok(buffer, ",");
    menuname = strtok(NULL,   ",");
    if (!filename) return ret;

    DOSFS_GetFullName(filename, FALSE, &fullname);
    if (menuname) menu_default = strdup(menuname);
    if ((cfg_fd = fopen(fullname.long_name, "r")))
    {
        DOSCONF_Parse(NULL);
        fclose(cfg_fd);
    }
    else
    {
        MSG("Couldn't open config.sys file given as \"%s\" in" \
            " wine.conf or .winerc, section [wine] !\n", filename);
        ret = 0;
    }
    if (menu_default) free(menu_default);
    return ret;
}
