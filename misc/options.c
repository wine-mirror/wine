/*
 * Option parsing
 *
 * Copyright 2000 Alexandre Julliard
 */

#include "config.h"
#include <string.h>
#include <stdlib.h>

#include "winbase.h"
#include "main.h"
#include "options.h"
#include "version.h"
#include "debugtools.h"

struct option
{
    const char *longname;
    char        shortname;
    int         has_arg;
    int         inherit;
    void      (*func)( const char *arg );
    const char *usage;
};

/* Most Windows C/C++ compilers use something like this to */
/* access argc and argv globally: */
int _ARGC;
char **_ARGV;

/* default options */
struct options Options =
{
    NULL,           /* desktopGeometry */
    NULL,           /* display */
    NULL,           /* dllFlags */
    FALSE,          /* synchronous */
    FALSE,          /* Managed windows */
    NULL            /* Alternate config file name */
};

const char *argv0;       /* the original argv[0] */
const char *full_argv0;  /* the full path of argv[0] (if known) */

static char *inherit_str;  /* options to pass to child processes */

static void out_of_memory(void) WINE_NORETURN;
static void out_of_memory(void)
{
    MESSAGE( "Virtual memory exhausted\n" );
    ExitProcess(1);
}

static char *xstrdup( const char *str )
{
    char *ret = strdup( str );
    if (!ret) out_of_memory();
    return ret;
}

static void do_config( const char *arg );
static void do_debugmsg( const char *arg );
static void do_desktop( const char *arg );
static void do_display( const char *arg );
static void do_dll( const char *arg );
static void do_help( const char *arg );
static void do_language( const char *arg );
static void do_managed( const char *arg );
static void do_synchronous( const char *arg );
static void do_version( const char *arg );

static const struct option option_table[] =
{
    { "config",       0, 1, 0, do_config,
      "--config name    Specify config file to use" },
    { "debugmsg",     0, 1, 1, do_debugmsg,
      "--debugmsg name  Turn debugging-messages on or off" },
    { "desktop",      0, 1, 1, do_desktop,
      "--desktop geom   Use a desktop window of the given geometry" },
    { "display",      0, 1, 0, do_display,
      "--display name   Use the specified display" },
    { "dll",          0, 1, 1, do_dll,
      "--dll name       Enable or disable built-in DLLs" },
    { "dosver",       0, 1, 1, VERSION_ParseDosVersion,
      "--dosver x.xx    DOS version to imitate (e.g. 6.22). Only valid with --winver win31" },
    { "help",       'h', 0, 0, do_help,
      "--help,-h        Show this help message" },
    { "language",     0, 1, 1, do_language,
      "--language xx    Set the language (one of Br,Ca,Cs,Cy,Da,De,En,Eo,Es,Fi,Fr,Ga,Gd,Gv,\n"
      "                    Hr,Hu,It,Ja,Ko,Kw,Nl,No,Pl,Pt,Sk,Sv,Ru,Wa)" },
    { "managed",      0, 0, 0, do_managed,
      "--managed        Allow the window manager to manage created windows" },
    { "synchronous",  0, 0, 1, do_synchronous,
      "--synchronous    Turn on synchronous display mode" },
    { "version",    'v', 0, 0, do_version,
      "--version,-v     Display the Wine version" },
    { "winver",       0, 1, 1, VERSION_ParseWinVersion,
      "--winver         Version to imitate (one of win31,win95,nt351,nt40)" },
    { NULL,           0, 0, 0, NULL, NULL }  /* terminator */
};


static void do_help( const char *arg )
{
    OPTIONS_Usage();
}

static void do_version( const char *arg )
{
    MESSAGE( "%s\n", WINE_RELEASE_INFO );
    ExitProcess(0);
}

static void do_synchronous( const char *arg )
{
    Options.synchronous = TRUE;
}

static void do_desktop( const char *arg )
{
    Options.desktopGeometry = xstrdup( arg );
}

static void do_display( const char *arg )
{
    Options.display = xstrdup( arg );
}

static void do_dll( const char *arg )
{
    if (Options.dllFlags)
    {
        Options.dllFlags = (char *) realloc ( Options.dllFlags, 
					    strlen ( Options.dllFlags ) + strlen ( arg ) + 2 );
	if ( !Options.dllFlags ) out_of_memory(); 
	strcat ( Options.dllFlags, "+" );
	strcat ( Options.dllFlags, arg );
    }
    else 
    {
        Options.dllFlags = xstrdup( arg );
    }
}

static void do_language( const char *arg )
{
    SetEnvironmentVariableA( "LANGUAGE", arg );
}

static void do_managed( const char *arg )
{
    Options.managed = TRUE;
}

static void do_config( const char *arg )
{
    Options.configFileName = xstrdup( arg );
}

static void do_debugmsg( const char *arg )
{
    extern void wine_dbg_add_option( const char *name, unsigned char set, unsigned char clear );
    static const char * const debug_class_names[__DBCL_COUNT] = { "fixme", "err", "warn", "trace" };

    char *opt, *options = strdup(arg);
    int i;

    if (!(opt = strtok( options, "," ))) goto error;
    do
    {
        unsigned char set = 0, clear = 0;
        char *p = strchr( opt, '+' );
        if (!p) p = strchr( opt, '-' );
        if (!p || !p[1]) goto error;
        if (p > opt)
        {
            for (i = 0; i < __DBCL_COUNT; i++)
            {
                int len = strlen(debug_class_names[i]);
                if (len != (p - opt)) continue;
                if (!memcmp( opt, debug_class_names[i], len ))  /* found it */
                {
                    if (*p == '+') set |= 1 << i;
                    else clear |= 1 << i;
                    break;
                }
            }
            if (i == __DBCL_COUNT) goto error;  /* class name not found */
        }
        else
        {
            if (*p == '+') set = ~0;
            else clear = ~0;
        }
        p++;
        if (!strcmp( p, "all" )) p = "";  /* empty string means all */
        wine_dbg_add_option( p, set, clear );
        opt = strtok( NULL, "," );
    } while(opt);

    free( options );
    return;

 error:
    MESSAGE("wine: Syntax: --debugmsg [class]+xxx,...  or "
            "-debugmsg [class]-xxx,...\n");
    MESSAGE("Example: --debugmsg +all,warn-heap\n"
            "  turn on all messages except warning heap messages\n");
    MESSAGE("Available message classes:\n");
    for( i = 0; i < __DBCL_COUNT; i++) MESSAGE( "%-9s", debug_class_names[i] );
    MESSAGE("\n\n");
    ExitProcess(1);
}


static void remove_options( char *argv[], int pos, int count, int inherit )
{
    if (inherit)
    {
        int i, len = 0;
        for (i = 0; i < count; i++) len += strlen(argv[pos+i]) + 1;
        if (inherit_str)
        {
            if (!(inherit_str = realloc( inherit_str, strlen(inherit_str) + 1 + len )))
                out_of_memory();
            strcat( inherit_str, " " );
        }
        else
        {
            if (!(inherit_str = malloc( len ))) out_of_memory();
            inherit_str[0] = 0;
        }
        for (i = 0; i < count; i++)
        {
            strcat( inherit_str, argv[pos+i] );
            if (i < count-1) strcat( inherit_str, " " );
        }
    }
    while ((argv[pos] = argv[pos+count])) pos++;
}

/* parse options from the argv array and remove all the recognized ones */
static void parse_options( char *argv[] )
{
    const struct option *opt;
    int i;

    for (i = 0; argv[i]; i++)
    {
        char *p = argv[i];
        if (*p++ != '-') continue;  /* not an option */
        if (*p && !p[1]) /* short name */
        {
            if (*p == '-') break; /* "--" option */
            for (opt = option_table; opt->longname; opt++) if (opt->shortname == *p) break;
        }
        else  /* long name */
        {
            if (*p == '-') p++;
            /* check for the long name */
            for (opt = option_table; opt->longname; opt++)
                if (!strcmp( p, opt->longname )) break;
        }
        if (!opt->longname) continue;

        if (opt->has_arg && argv[i+1])
        {
            opt->func( argv[i+1] );
            remove_options( argv, i, 2, opt->inherit );
        }
        else
        {
            opt->func( "" );
            remove_options( argv, i, 1, opt->inherit );
        }
        i--;
    }
}

/* inherit options from WINEOPTIONS variable */
static void inherit_options( char *buffer )
{
    char *argv[256];
    unsigned int n;

    char *p = strtok( buffer, " \t" );
    for (n = 0; n < sizeof(argv)/sizeof(argv[0])-1 && p; n++)
    {
        argv[n] = p;
        p = strtok( NULL, " \t" );
    }
    argv[n] = NULL;
    parse_options( argv );
    if (argv[0])  /* an option remains */
    {
        MESSAGE( "Unknown option '%s' in WINEOPTIONS variable\n\n", argv[0] );
        OPTIONS_Usage();
    }
}

/***********************************************************************
 *              OPTIONS_Usage
 */
void OPTIONS_Usage(void)
{
    const struct option *opt;
    MESSAGE( "Usage: %s [options] program_name [arguments]\n\n", argv0 );
    MESSAGE( "Options:\n" );
    for (opt = option_table; opt->longname; opt++) MESSAGE( "   %s\n", opt->usage );
    ExitProcess(0);
}

/***********************************************************************
 *              OPTIONS_ParseOptions
 */
void OPTIONS_ParseOptions( char *argv[] )
{
    char buffer[1024];
    int i;

    if (GetEnvironmentVariableA( "WINEOPTIONS", buffer, sizeof(buffer) ) && buffer[0])
        inherit_options( buffer );

    parse_options( argv + 1 );

    SetEnvironmentVariableA( "WINEOPTIONS", inherit_str );

    /* check if any option remains */
    for (i = 1; argv[i]; i++)
    {
        if (!strcmp( argv[i], "--" ))
        {
            remove_options( argv, i, 1, 0 );
            break;
        }
        if (argv[i][0] == '-')
        {
            MESSAGE( "Unknown option '%s'\n\n", argv[i] );
            OPTIONS_Usage();
        }
    }

    /* count the resulting arguments */
    _ARGV = argv;
    _ARGC = 0;
    while (argv[_ARGC]) _ARGC++;
}
