/*
 * DOS CONFIG.SYS parser
 *
 * Copyright 1998 Andreas Mohr
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"
#include "wine/port.h"

#include <stdarg.h>
#include <stdio.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "windef.h"
#include "winbase.h"

#include "dosexe.h"

#include "wine/debug.h"
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(profile);


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

static DOSCONF DOSCONF_config =
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

static BOOL DOSCONF_loaded = FALSE;

typedef struct {
    const char *tag_name;
    int (*tag_handler)(char **p);
} TAG_ENTRY;


/*
 * see
 * http://egeria.cm.cf.ac.uk/User/P.L.Poulain/project/internal/allinter.htm
 * or
 * http://www.csulb.edu/~murdock/dosindex.html
 */

static const TAG_ENTRY DOSCONF_tag_entries[] =
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

static FILE *DOSCONF_fd = NULL;

static char *DOSCONF_menu_default = NULL;
static int   DOSCONF_menu_in_listing = 0; /* we are in the [menu] section */
static int   DOSCONF_menu_skip = 0;	  /* the current menu gets skipped */

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
    else 
        p++;

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
    TRACE("Loading device '%s'\n", *confline);
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
	else if (!(strncasecmp(*confline, "UMB", 3)))
	{
	    DOSCONF_config.flags |= DOSCONF_MEM_UMB;
	    *confline += 3;
	}
        else 
        {
            (*confline)++;
        }

	DOSCONF_JumpToEntry(confline, ',');
    }
    TRACE( "DOSCONF_Dos: HIGH is %d, UMB is %d\n",
           (DOSCONF_config.flags & DOSCONF_MEM_HIGH) != 0, 
           (DOSCONF_config.flags & DOSCONF_MEM_UMB) != 0 );
    return 1;
}

static int DOSCONF_Fcbs(char **confline)
{
    *confline += 4; /* strlen("FCBS") */
    if (!(DOSCONF_JumpToEntry(confline, '='))) return 0;
    DOSCONF_config.fcbs = atoi(*confline);
    if (DOSCONF_config.fcbs > 255)
    {
        WARN( "The FCBS value in the config.sys file is too high! Setting to 255.\n" );
        DOSCONF_config.fcbs = 255;
    }
    TRACE( "DOSCONF_Fcbs returning %d\n", DOSCONF_config.fcbs );
    return 1;
}

static int DOSCONF_Break(char **confline)
{
    *confline += 5; /* strlen("BREAK") */
    if (!(DOSCONF_JumpToEntry(confline, '='))) return 0;
    if (!(strcasecmp(*confline, "ON")))
        DOSCONF_config.brk_flag = 1;
    TRACE( "BREAK is %d\n", DOSCONF_config.brk_flag );
    return 1;
}

static int DOSCONF_Files(char **confline)
{
    *confline += 5; /* strlen("FILES") */
    if (!(DOSCONF_JumpToEntry(confline, '='))) return 0;
    DOSCONF_config.files = atoi(*confline);
    if (DOSCONF_config.files > 255)
    {
    	WARN( "The FILES value in the config.sys file is too high! Setting to 255.\n" );
        DOSCONF_config.files = 255;
    }
    if (DOSCONF_config.files < 8)
    {
    	WARN( "The FILES value in the config.sys file is too low! Setting to 8.\n" );
        DOSCONF_config.files = 8;
    }
    TRACE( "DOSCONF_Files returning %d\n", DOSCONF_config.files );
    return 1;
}

static int DOSCONF_Install(char **confline)
{
#if 0
    int loadhigh = 0;
#endif

    *confline += 7; /* strlen("INSTALL") */
    if (!(DOSCONF_JumpToEntry(confline, '='))) return 0;
    TRACE( "Installing '%s'\n", *confline );
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
    TRACE( "Lastdrive %c\n", DOSCONF_config.lastdrive );
    return 1;
}

static int DOSCONF_Country(char **confline)
{
    *confline += 7; /* strlen("COUNTRY") */
    if (!(DOSCONF_JumpToEntry(confline, '='))) return 0;
    TRACE( "Country '%s'\n", *confline );
    if (DOSCONF_config.country == NULL)
        DOSCONF_config.country = HeapAlloc(GetProcessHeap(), 0, strlen(*confline) + 1);
    strcpy(DOSCONF_config.country, *confline);
    return 1;
}

static int DOSCONF_Numlock(char **confline)
{
    *confline += 7; /* strlen("NUMLOCK") */
    if (!(DOSCONF_JumpToEntry(confline, '='))) return 0;
    if (!(strcasecmp(*confline, "ON")))
        DOSCONF_config.flags |= DOSCONF_NUMLOCK;
    TRACE( "NUMLOCK is %d\n", 
           (DOSCONF_config.flags & DOSCONF_NUMLOCK) != 0 );
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
    TRACE( "'Force conventional keyboard' is %d\n",
           (DOSCONF_config.flags & DOSCONF_KEYB_CONV) != 0 );
    return 1;
}

static int DOSCONF_Shell(char **confline)
{
    *confline += 5; /* strlen("SHELL") */
    if (!(DOSCONF_JumpToEntry(confline, '='))) return 0;
    TRACE( "Shell '%s'\n", *confline );
    if (DOSCONF_config.shell == NULL)
        DOSCONF_config.shell = HeapAlloc(GetProcessHeap(), 0, strlen(*confline) + 1);
    strcpy(DOSCONF_config.shell, *confline);
    return 1;
}

static int DOSCONF_Stacks(char **confline)
{

    *confline += 6; /* strlen("STACKS") */
    if (!(DOSCONF_JumpToEntry(confline, '='))) return 0;
    DOSCONF_config.stacks_nr = atoi(strtok(*confline, ","));
    DOSCONF_config.stacks_sz = atoi((strtok(NULL, ",")));
    TRACE( "%d stacks of size %d\n",
           DOSCONF_config.stacks_nr, DOSCONF_config.stacks_sz );
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
    TRACE( "%d primary buffers, %d secondary buffers\n",
           DOSCONF_config.buf, DOSCONF_config.buf2 );
    return 1;
}

static int DOSCONF_Menu(char **confline)
{
    if (!(strncasecmp(*confline, "[MENU]", 6)))
    {
	DOSCONF_menu_in_listing = 1;
    }
    else if ((!(strncasecmp(*confline, "[COMMON]", 8)))
             || (!(strncasecmp(*confline, "[WINE]", 6))))
    {
	DOSCONF_menu_skip = 0;
    }
    else if (**confline == '[')
    {
	(*confline)++;
	if ((DOSCONF_menu_default)
            && (!(strncasecmp(*confline, DOSCONF_menu_default, 
                              strlen(DOSCONF_menu_default)))))
        {
            HeapFree(GetProcessHeap(), 0, DOSCONF_menu_default);
            DOSCONF_menu_default = NULL;
            DOSCONF_menu_skip = 0;
        }
        else
	    DOSCONF_menu_skip = 1;
	DOSCONF_menu_in_listing = 0;
    }
    else if (!(strncasecmp(*confline, "menudefault", 11)) 
             && (DOSCONF_menu_in_listing))
    {
	if (!(DOSCONF_JumpToEntry(confline, '='))) return 0;
        *confline = strtok(*confline, ",");
        DOSCONF_menu_default = HeapAlloc(GetProcessHeap(), 0, strlen(*confline) + 1);
	strcpy(DOSCONF_menu_default, *confline);
    }

    return 1;
}

static int DOSCONF_Include(char **confline)
{
    fpos_t oldpos;
    char *temp;

    *confline += 7; /* strlen("INCLUDE") */
    if (!(DOSCONF_JumpToEntry(confline, '='))) return 0;
    fgetpos(DOSCONF_fd, &oldpos);
    fseek(DOSCONF_fd, 0, SEEK_SET);
    TRACE( "Including menu '%s'\n", *confline );
    temp = HeapAlloc(GetProcessHeap(), 0, strlen(*confline) + 1);
    strcpy(temp, *confline);
    DOSCONF_Parse(temp);
    HeapFree(GetProcessHeap(), 0, temp);
    fsetpos(DOSCONF_fd, &oldpos);
    return 1;
}

static void DOSCONF_Parse(char *menuname)
{
    char confline[256];
    char *p, *trail;
    unsigned int i;

    if (menuname != NULL) /* we need to jump to a certain sub menu */
    {
	while (fgets(confline, 255, DOSCONF_fd))
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

    while (fgets(confline, 255, DOSCONF_fd))
    {
	p = confline;
	DOSCONF_skip(&p);

	if ((menuname) && (*p == '['))
	    /*
             * we were handling a specific sub menu, 
             * but now next menu begins 
             */
	    break;

	if ((trail = strrchr(confline, '\n')))
            *trail = '\0';
	if ((trail = strrchr(confline, '\r')))
            *trail = '\0';
	if (!(DOSCONF_menu_skip))
	{
	    for (i = 0; i < sizeof(DOSCONF_tag_entries) / sizeof(TAG_ENTRY); 
                 i++)
		if (!(strncasecmp(p, DOSCONF_tag_entries[i].tag_name,
                                  strlen(DOSCONF_tag_entries[i].tag_name))))
                {
		    TRACE( "tag '%s'\n", DOSCONF_tag_entries[i].tag_name );
		    if (DOSCONF_tag_entries[i].tag_handler != NULL)
                        DOSCONF_tag_entries[i].tag_handler(&p);
                    break;
		}
	}
	else
        { 
            /* the current menu gets skipped */
            DOSCONF_Menu(&p);
        }
    }
}

DOSCONF *DOSCONF_GetConfig(void)
{
    char *fullname;
    WCHAR filename[MAX_PATH];
    static const WCHAR configW[] = {'c','o','n','f','i','g','.','s','y','s',0};

    if (DOSCONF_loaded)
        return &DOSCONF_config;

    /* look for config.sys at the root of the drive containing the windows dir */
    GetWindowsDirectoryW( filename, MAX_PATH );
    strcpyW( filename + 3, configW );

    if ((fullname = wine_get_unix_file_name(filename)))
    {
        DOSCONF_fd = fopen(fullname, "r");
        HeapFree( GetProcessHeap(), 0, fullname );
    }

    if (DOSCONF_fd)
    {
        DOSCONF_Parse(NULL);
        fclose(DOSCONF_fd);
        DOSCONF_fd = NULL;
    }
    else WARN( "Couldn't open %s\n", debugstr_w(filename) );

    DOSCONF_loaded = TRUE;
    return &DOSCONF_config;
}
