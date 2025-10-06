/*
 * Copyright 1994 Martin von Loewis
 * Copyright 1998 Bertho A. Stultiens (BS)
 * Copyright 2003 Dimitrie O. Paun
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
 *
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <limits.h>
#include <sys/types.h>

#include "../tools.h"
#include "wrc.h"
#include "utils.h"
#include "newstruc.h"
#include "parser.h"
#include "wpp_private.h"

static const char usage[] =
	"Usage: wrc [options...] [infile[.rc|.res]]\n"
	"   -D, --define id[=val]      Define preprocessor identifier id=val\n"
	"   --debug=nn                 Set debug level to 'nn'\n"
	"   -E                         Preprocess only\n"
	"   -F TARGET                  Ignored, for compatibility with windres\n"
	"   -fo FILE                   Synonym for -o for compatibility with windres\n"
	"   -h, --help                 Prints this summary\n"
	"   -i, --input=FILE           The name of the input file\n"
	"   -I, --include-dir=DIR      Add dir to the include search path (multiple -I allowed)\n"
	"   -J, --input-format=FORMAT  The input format (either `rc' or `rc16')\n"
	"   -l, --language=LANG        Set default language to LANG (default is neutral {0, 0})\n"
	"   -m16                       Build a 16-bit resource file\n"
	"   --nls-dir=DIR              Directory containing the NLS codepage mappings\n"
	"   --no-use-temp-file         Ignored for compatibility with windres\n"
	"   --nostdinc                 Disables searching the standard include path\n"
	"   -o, --output=FILE          Output to file (default is infile.res)\n"
	"   -O, --output-format=FORMAT The output format (`po', `pot', `res', or `res16`)\n"
	"   --pedantic                 Enable pedantic warnings\n"
	"   --po-dir=DIR               Directory containing po files for translations\n"
	"   --preprocessor             Specifies the preprocessor to use, including arguments\n"
	"   -r                         Ignored for compatibility with rc\n"
	"   --sysroot=DIR              Prefix include paths with DIR\n"
	"   -U, --undefine id          Undefine preprocessor identifier id\n"
	"   --use-temp-file            Ignored for compatibility with windres\n"
	"   -v, --verbose              Enable verbose mode\n"
	"   --verify-translations      Check the status of the various translations\n"
	"   --version                  Print version and exit\n"
	"Input is taken from stdin if no sourcefile specified.\n"
	"Debug level 'n' is a bitmask with following meaning:\n"
	"    * 0x01 Tell which resource is parsed (verbose mode)\n"
	"    * 0x02 Dump internal structures\n"
	"    * 0x04 Create a parser trace (yydebug=1)\n"
	"    * 0x08 Preprocessor messages\n"
	"    * 0x10 Preprocessor lex messages\n"
	"    * 0x20 Preprocessor yacc trace\n"
	"If no input filename is given and the output name is not overridden\n"
	"with -o, then the output is written to \"wrc.tab.res\"\n"
	;

static const char version_string[] = "Wine Resource Compiler version " PACKAGE_VERSION "\n"
			"Copyright 1998-2000 Bertho A. Stultiens\n"
			"          1994 Martin von Loewis\n";

/*
 * Set if compiling in 32bit mode (default).
 */
int win32 = 1;

/*
 * debuglevel == DEBUGLEVEL_NONE	Don't bother
 * debuglevel & DEBUGLEVEL_CHAT		Say what's done
 * debuglevel & DEBUGLEVEL_DUMP		Dump internal structures
 * debuglevel & DEBUGLEVEL_TRACE	Create parser trace
 * debuglevel & DEBUGLEVEL_PPMSG	Preprocessor messages
 * debuglevel & DEBUGLEVEL_PPLEX	Preprocessor lex trace
 * debuglevel & DEBUGLEVEL_PPTRACE	Preprocessor yacc trace
 */
int debuglevel = DEBUGLEVEL_NONE;

/*
 * Recognize win32 keywords if set (-w 32 enforces this),
 * otherwise set with -e option.
 */
int extensions = 1;

/*
 * Language setting for resources (-l option)
 */
static language_t defaultlanguage;
language_t currentlanguage = 0;

/*
 * Set when extra warnings should be generated (-W option)
 */
int pedantic = 0;

/*
 * Set when _only_ to run the preprocessor (-E option)
 */
int preprocess_only = 0;

/*
 * Set when _not_ to run the preprocessor (-P cat option)
 */
int no_preprocess = 0;

int utf8_input = 0;

int check_utf8 = 1;  /* whether to check for valid utf8 */

static char *output_name;	/* The name given by the -o option */
const char *input_name = NULL;	/* The name given on the command-line */
static struct strarray input_files;
const char *temp_dir = NULL;
struct strarray temp_files = { 0 };

static int stdinc = 1;
static int po_mode;
static const char *po_dir;
static const char *sysroot = "";
static const char *bindir;
static const char *includedir;
const char *nlsdirs[3] = { NULL, DATADIR "/wine/nls", NULL };

int line_number = 1;		/* The current line */
int char_number = 1;		/* The current char pos within the line */

int parser_debug, yy_flex_debug;

resource_t *resource_top;	/* The top of the parsed resources */

static void cleanup_files(void);

enum long_options_values
{
    LONG_OPT_NOSTDINC = 1,
    LONG_OPT_TMPFILE,
    LONG_OPT_NOTMPFILE,
    LONG_OPT_NLS_DIR,
    LONG_OPT_PO_DIR,
    LONG_OPT_PREPROCESSOR,
    LONG_OPT_SYSROOT,
    LONG_OPT_VERSION,
    LONG_OPT_DEBUG,
    LONG_OPT_PEDANTIC,
};

static const char short_options[] =
	"b:D:Ef:F:hi:I:J:l:m:o:O:ruU:v";
static const struct long_option long_options[] = {
	{ "debug", 1, LONG_OPT_DEBUG },
	{ "define", 1, 'D' },
	{ "help", 0, 'h' },
	{ "include-dir", 1, 'I' },
	{ "input", 1, 'i' },
	{ "input-format", 1, 'J' },
	{ "language", 1, 'l' },
	{ "nls-dir", 1, LONG_OPT_NLS_DIR },
	{ "no-use-temp-file", 0, LONG_OPT_NOTMPFILE },
	{ "nostdinc", 0, LONG_OPT_NOSTDINC },
	{ "output", 1, 'o' },
	{ "output-format", 1, 'O' },
	{ "pedantic", 0, LONG_OPT_PEDANTIC },
	{ "po-dir", 1, LONG_OPT_PO_DIR },
	{ "preprocessor", 1, LONG_OPT_PREPROCESSOR },
	{ "sysroot", 1, LONG_OPT_SYSROOT },
	{ "target", 1, 'F' },
	{ "utf8", 0, 'u' },
	{ "undefine", 1, 'U' },
	{ "use-temp-file", 0, LONG_OPT_TMPFILE },
	{ "verbose", 0, 'v' },
	{ "version", 0, LONG_OPT_VERSION },
	{ NULL }
};

static void set_version_defines(void)
{
    char *version = xstrdup( PACKAGE_VERSION );
    char *major, *minor, *patchlevel;
    char buffer[100];

    if ((minor = strchr( version, '.' )))
    {
        major = version;
        *minor++ = 0;
        if ((patchlevel = strchr( minor, '.' ))) *patchlevel++ = 0;
    }
    else  /* pre 0.9 version */
    {
        major = NULL;
        patchlevel = version;
    }
    snprintf( buffer, sizeof(buffer), "__WRC__=%s", major ? major : "0" );
    wpp_add_cmdline_define(buffer);
    snprintf( buffer, sizeof(buffer), "__WRC_MINOR__=%s", minor ? minor : "0" );
    wpp_add_cmdline_define(buffer);
    snprintf( buffer, sizeof(buffer), "__WRC_PATCHLEVEL__=%s", patchlevel ? patchlevel : "0" );
    wpp_add_cmdline_define(buffer);
    free( version );
}

/* clean things up when aborting on a signal */
static void exit_on_signal( int sig )
{
    exit(1);  /* this will call the atexit functions */
}

/* load a single input file */
static int load_file( const char *input_name, const char *output_name )
{
    int ret;

    /* Run the preprocessor on the input */
    if(!no_preprocess)
    {
        FILE *output;
        int ret;
        char *name;

        /*
         * Preprocess the input to a temp-file, or stdout if
         * no output was given.
         */

        if (preprocess_only)
        {
            if (output_name)
            {
                if (!(output = fopen( output_name, "w" )))
                    fatal_perror( "Could not open %s for writing", output_name );
                ret = wpp_parse( input_name, output );
                fclose( output );
            }
            else ret = wpp_parse( input_name, stdout );

            if (ret) return ret;
            output_name = NULL;
            exit(0);
        }

        name = make_temp_file( output_name, "" );
        if (!(output = fopen(name, "wt")))
            error("Could not open fd %s for writing\n", name);

        ret = wpp_parse( input_name, output );
        fclose( output );
        if (ret) return ret;
        input_name = name;
    }

    /* Reset the language */
    currentlanguage = defaultlanguage;
    check_utf8 = 1;

    /* Go from .rc to .res */
    chat("Starting parse\n");

    if(!(parser_in = fopen(input_name, "rb")))
        fatal_perror("Could not open %s for input", input_name);

    ret = parser_parse();
    fclose(parser_in);
    parser_lex_destroy();
    return ret;
}

static void option_callback( int optc, char *optarg )
{
    switch(optc)
    {
    case LONG_OPT_NOSTDINC:
        stdinc = 0;
        break;
    case LONG_OPT_TMPFILE:
        if (debuglevel) warning("--use-temp-file option not yet supported, ignored.\n");
        break;
    case LONG_OPT_NOTMPFILE:
        if (debuglevel) warning("--no-use-temp-file option not yet supported, ignored.\n");
        break;
    case LONG_OPT_NLS_DIR:
        nlsdirs[0] = xstrdup( optarg );
        break;
    case LONG_OPT_PO_DIR:
        po_dir = xstrdup( optarg );
        break;
    case LONG_OPT_PREPROCESSOR:
        if (strcmp(optarg, "cat") == 0) no_preprocess = 1;
        else fprintf(stderr, "-P option not yet supported, ignored.\n");
        break;
    case LONG_OPT_SYSROOT:
        sysroot = xstrdup( optarg );
        break;
    case LONG_OPT_VERSION:
        printf(version_string);
        exit(0);
        break;
    case LONG_OPT_DEBUG:
        debuglevel = strtol(optarg, NULL, 0);
        break;
    case LONG_OPT_PEDANTIC:
        pedantic = 1;
        break;
    case 'D':
        wpp_add_cmdline_define(optarg);
        break;
    case 'E':
        preprocess_only = 1;
        break;
    case 'b':
    case 'F':
        break;
    case 'h':
        printf(usage);
        exit(0);
    case 'i':
        strarray_add( &input_files, optarg );
        break;
    case 'I':
        wpp_add_include_path(optarg);
        break;
    case 'J':
        if (strcmp(optarg, "rc16") == 0)  extensions = 0;
        else if (strcmp(optarg, "rc")) error("Output format %s not supported.\n", optarg);
        break;
    case 'l':
        defaultlanguage = strtol(optarg, NULL, 0);
        if (get_language_codepage( defaultlanguage ) == -1)
            error("Language %04x is not supported\n", defaultlanguage);
        break;
    case 'm':
        if (!strcmp( optarg, "16" )) win32 = 0;
        else win32 = 1;
        break;
    case 'f':
        if (*optarg != 'o') error("Unknown option: -f%s\n",  optarg);
        optarg++;
        /* fall through */
    case 'o':
        if (!output_name) output_name = xstrdup(optarg);
        else error("Too many output files.\n");
        break;
    case 'O':
        if (strcmp(optarg, "po") == 0) po_mode = 1;
        else if (strcmp(optarg, "pot") == 0) po_mode = 2;
        else if (strcmp(optarg, "res16") == 0) win32 = 0;
        else if (strcmp(optarg, "res")) warning("Output format %s not supported.\n", optarg);
        break;
    case 'r':
        /* ignored for compatibility with rc */
        break;
    case 'u':
        utf8_input = 1;
        break;
    case 'U':
        wpp_del_define(optarg);
        break;
    case 'v':
        debuglevel = DEBUGLEVEL_CHAT;
        break;
    case '?':
        fprintf(stderr, "wrc: %s\n\n%s", optarg, usage);
        exit(1);
    }
}

int main(int argc,char *argv[])
{
	int i;

        init_signals( exit_on_signal );
        bindir = get_bindir( argv[0] );
        includedir = get_includedir( bindir );
        nlsdirs[0] = get_nlsdir( bindir, "/tools/wrc" );

	/* Set the default defined stuff */
        set_version_defines();
	wpp_add_cmdline_define("RC_INVOKED=1");
	/* Microsoft RC always searches current directory */
	wpp_add_include_path(".");

        strarray_addall( &input_files,
                         parse_options( argc, argv, short_options, long_options, 0, option_callback ));

	if (win32) wpp_add_cmdline_define("_WIN32=1");

	/* If we do need to search standard includes, add them to the path */
	if (stdinc)
	{
            static const char *incl_dirs[] = { INCLUDEDIR, "/usr/include", "/usr/local/include" };

            if (includedir)
            {
                wpp_add_include_path( strmake( "%s/wine/msvcrt", includedir ));
                wpp_add_include_path( strmake( "%s/wine/windows", includedir ));
            }
            for (i = 0; i < ARRAY_SIZE(incl_dirs); i++)
            {
                if (i && !strcmp( incl_dirs[i], incl_dirs[0] )) continue;
                wpp_add_include_path( strmake( "%s%s/wine/msvcrt", sysroot, incl_dirs[i] ));
                wpp_add_include_path( strmake( "%s%s/wine/windows", sysroot, incl_dirs[i] ));
            }
	}

	/* Kill io buffering when some kind of debuglevel is enabled */
	if(debuglevel)
	{
		setbuf(stdout, NULL);
		setbuf(stderr, NULL);
	}

	parser_debug = debuglevel & DEBUGLEVEL_TRACE ? 1 : 0;
	yy_flex_debug = debuglevel & DEBUGLEVEL_TRACE ? 1 : 0;

        wpp_set_debug( (debuglevel & DEBUGLEVEL_PPLEX) != 0,
                       (debuglevel & DEBUGLEVEL_PPTRACE) != 0,
                       (debuglevel & DEBUGLEVEL_PPMSG) != 0 );

	atexit(cleanup_files);

        STRARRAY_FOR_EACH( file, &input_files )
        {
            input_name = file;
            if (load_file( input_name, output_name )) exit(1);
        }
	/* stdin special case. NULL means "stdin" for wpp. */
        if (input_files.count == 0 && load_file( NULL, output_name )) exit(1);

        if (!po_mode && output_name)
        {
            if (strendswith( output_name, ".po" )) po_mode = 1;
            else if (strendswith( output_name, ".pot" )) po_mode = 2;
        }
	if (po_mode)
	{
            if (!win32) error( "PO files are not supported in 16-bit mode\n" );
            if (po_mode == 2)  /* pot file */
            {
                if (!output_name)
                {
                    const char *name = input_files.count ? get_basename(input_files.str[0]) : "wrc.tab";
                    output_name = replace_extension( name, ".rc", ".pot" );
                }
                write_pot_file( output_name );
            }
            else write_po_files( output_name );
            output_name = NULL;
            exit(0);
	}
        if (win32) add_translations( po_dir );

	chat("Writing .res-file\n");
        if (!output_name)
        {
            const char *name = input_files.count ? get_basename(input_files.str[0]) : "wrc.tab";
            output_name = replace_extension( name, ".rc", ".res" );
        }
	write_resfile(output_name, resource_top);
	output_name = NULL;

	return 0;
}


static void cleanup_files(void)
{
	if (output_name) unlink(output_name);
        remove_temp_files();
}
