/*
 * Copyright  Martin von Loewis, 1994
 * Copyrignt 1998 Bertho A. Stultiens (BS)
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * History:
 * 30-Apr-2000 BS	- Integrated a new preprocessor (-E and -N)
 * 20-Jun-1998 BS	- Added -L option to prevent case conversion
 *			  of embedded filenames.
 *
 * 08-Jun-1998 BS	- Added -A option to generate autoregister code
 *			  for winelib operation.
 *
 * 21-May-1998 BS	- Removed the CPP option. Its internal now.
 *			- Added implementations for defines and includes
 *			  on the commandline.
 *
 * 30-Apr-1998 BS	- The options now contain nearly the entire alphabet.
 *			  Seems to be a sign for too much freedom. I implemeted
 *			  most of them as a user choice possibility for things
 *			  that I do not know what to put there by default.
 *			- -l and -L options are now known as -t and -T.
 *
 * 23-Apr-1998 BS	- Finally gave up on backward compatibility on the
 *			  commandline (after a blessing from the newsgroup).
 *			  So, I changed the lot.
 *
 * 17-Apr-1998 BS	- Added many new command-line options but took care
 *			  that it would not break old scripts (sigh).
 *
 * 16-Apr-1998 BS	- There is not much left of the original source...
 *			  I had to rewrite most of it because the parser
 *			  changed completely with all the types etc..
 *
 */

#include "config.h"
#include "wine/port.h"

#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <signal.h>
#ifdef HAVE_GETOPT_H
# include <getopt.h>
#endif

#include "wrc.h"
#include "utils.h"
#include "readres.h"
#include "dumpres.h"
#include "genres.h"
#include "newstruc.h"
#include "parser.h"
#include "../wpp/wpp.h"

#ifndef INCLUDEDIR
#define INCLUDEDIR "/usr/local/include/wine"
#endif

#ifdef WORDS_BIGENDIAN
#define ENDIAN	"big"
#else
#define ENDIAN	"little"
#endif

static char usage[] =
	"Usage: wrc [options...] [infile[.rc|.res]] [outfile]\n"
	"   -a n        Alignment of resource (win16 only, default is 4)\n"
	"   -B x        Set output byte-order x={n[ative], l[ittle], b[ig]}\n"
	"               (win32 only; default is " ENDIAN "-endian)\n"
	"   -C cp       Set the resource's codepage to cp (default is 0)\n"
	"   -d n        Set debug level to 'n'\n"
	"   -D id[=val] Define preprocessor identifier id=val\n"
	"   -e          Disable recognition of win32 keywords in 16bit compile\n"
	"   -E          Preprocess only\n"
	"   -F target	Ignored for compatibility with windres\n"
	"   -h		Prints this summary.\n"
	"   -i file	The name of the input file.\n"
	"   -I path     Set include search dir to path (multiple -I allowed)\n"
	"   -J		Do not search the standard include path\n"
	"   -l lan      Set default language to lan (default is neutral {0, 0})\n"
	"   -m          Do not remap numerical resource IDs\n"
	"   -o file     Output to file (default is infile.res)\n"
	"   -O format	The output format (must be `res').\n"
	"   -P program	Specifies the preprocessor to use, including arguments.\n"
	"   -v          Enable verbose mode.\n"
	"   -V          Print version and exit\n"
	"   -w 16|32    Select win16 or win32 output (default is win32)\n"
	"   -W          Enable pedantic warnings\n"
#ifdef HAVE_GETOPT_LONG
	"The following long options are supported:\n"
	"   --input		Synonym for -i.\n"
	"   --output		Synonym for -o.\n"
	"   --target		Synonym for -F.\n"
	"   --format		Synonym for -O.\n"
	"   --include-dir	Synonym for -I.\n"
	"   --nostdinc		Synonym for -J.\n"
	"   --define		Synonym for -D.\n"
	"   --language		Synonym for -l.\n"
	"   --use-temp-file	Ignored for compatibility with windres.\n"
	"   --no-use-temp-file	Ignored for compatibility with windres.\n"
	"   --preprocessor	Synonym for -P.\n"
	"   --help		Synonym for -h.\n"
	"   --version		Synonym for -V.\n"
#endif
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

char version_string[] = "Wine Resource Compiler Version " WRC_FULLVERSION "\n"
			"Copyright 1998-2000 Bertho A. Stultiens\n"
			"          1994 Martin von Loewis\n";

/*
 * External functions
 */
void write_resfile(char *outname, resource_t *top);

/*
 * Set if compiling in 32bit mode (default).
 */
int win32 = 1;

/*
 * debuglevel == DEBUGLEVEL_NONE	Don't bother
 * debuglevel & DEBUGLEVEL_CHAT		Say whats done
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
 * NE segment resource aligment (-a option)
 */
int alignment = 4;
int alignment_pwr;

/*
 * Language setting for resources (-l option)
 */
language_t *currentlanguage = NULL;

/*
 * The codepage to write in win32 PE resource segment (-C option)
 */
DWORD codepage = 0;

/*
 * Set when extra warnings should be generated (-W option)
 */
int pedantic = 0;

/*
 * The output byte-order of resources (set with -B)
 */
int byteorder = WRC_BO_NATIVE;

/*
 * Set when _only_ to run the preprocessor (-E option)
 */
int preprocess_only = 0;

/*
 * Set when _not_ to run the preprocessor (-P cat option)
 */
int no_preprocess = 0;

/*
 * Cleared when _not_ to remap resource types (-m option)
 */
int remap = 1;

char *output_name;		/* The name given by the -o option */
char *input_name;		/* The name given on the command-line */
char *temp_name;		/* Temporary file for preprocess pipe */

int line_number = 1;		/* The current line */
int char_number = 1;		/* The current char pos within the line */

char *cmdline;			/* The entire commandline */
time_t now;			/* The time of start of wrc */

resource_t *resource_top;	/* The top of the parsed resources */

int getopt (int argc, char *const *argv, const char *optstring);
static void rm_tempfile(void);
static void segvhandler(int sig);

static const char* short_options = 
	"a:AB:cC:d:D:eEF:hH:i:I:Jl:LmnNo:O:P:rtTvVw:W";
#ifdef HAVE_GETOPT_LONG
static struct option long_options[] = {
	{ "input", 1, 0, 'i' },
	{ "output", 1, 0, 'o' },
	{ "target", 1, 0, 'F' },
	{ "format", 1, 0, 'O' },
	{ "include-dir", 1, 0, 'I' },
	{ "nostdinc", 0, 0, 'J' },
	{ "define", 1, 0, 'D' },
	{ "language", 1, 0, 'l' },
	{ "version", 0, 0, 'V' },
	{ "help", 0, 0, 'h' },
	{ "preprocessor", 1, 0, 'P' },
	{ "use-temp-file", 0, 0, 2 },
	{ "no-use-temp-file", 0, 0, 3 },
	{ 0, 0, 0, 0 }
};
#endif

int main(int argc,char *argv[])
{
	extern char* optarg;
	extern int   optind;
	int optc;
#ifdef HAVE_GETOPT_LONG
	int opti = 0;
#endif
	int stdinc = 1;
	int lose = 0;
	int ret;
	int a;
	int i;
	int cmdlen;

	signal(SIGSEGV, segvhandler);

	now = time(NULL);

	/* First rebuild the commandline to put in destination */
	/* Could be done through env[], but not all OS-es support it */
	cmdlen = 4; /* for "wrc " */
	for(i = 1; i < argc; i++)
		cmdlen += strlen(argv[i]) + 1;
	cmdline = (char *)xmalloc(cmdlen);
	strcpy(cmdline, "wrc ");
	for(i = 1; i < argc; i++)
	{
		strcat(cmdline, argv[i]);
		if(i < argc-1)
			strcat(cmdline, " ");
	}

#ifdef HAVE_GETOPT_LONG
	while((optc = getopt_long(argc, argv, short_options, long_options, &opti)) != EOF)
#else
	while((optc = getopt(argc, argv, short_options)) != EOF)
#endif
	{
		switch(optc)
		{
		case 2:
			fprintf(stderr, "--use-temp-file option not yet supported, ignored.\n");
			break;
		case 3:
			fprintf(stderr, "--no-use-temp-file option not yet supported, ignored.\n");
			break;
		case 'a':
			alignment = atoi(optarg);
			break;
		case 'B':
			switch(optarg[0])
			{
			case 'n':
			case 'N':
				byteorder = WRC_BO_NATIVE;
				break;
			case 'l':
			case 'L':
				byteorder = WRC_BO_LITTLE;
				break;
			case 'b':
			case 'B':
				byteorder = WRC_BO_BIG;
				break;
			default:
				fprintf(stderr, "Byte ordering must be n[ative], l[ittle] or b[ig]\n");
				lose++;
			}
			break;
		case 'C':
			codepage = strtol(optarg, NULL, 0);
			break;
		case 'd':
			debuglevel = strtol(optarg, NULL, 0);
			break;
		case 'D':
			wpp_add_cmdline_define(optarg);
			break;
		case 'e':
			extensions = 0;
			break;
		case 'E':
			preprocess_only = 1;
			break;
		case 'F':
			/* ignored for compatibility with windres */
			break;
		case 'h':
			printf(usage);
			exit(0);
		case 'i':
			if (!input_name) input_name = strdup(optarg);
			else error("Too many input files.\n");
			break;
		case 'I':
			wpp_add_include_path(optarg);
			break;
		case 'J':
			stdinc = 0;
			break;
		case 'l':
			{
				int lan;
				lan = strtol(optarg, NULL, 0);
				if (get_language_codepage(PRIMARYLANGID(lan), SUBLANGID(lan)) == -1)
					error("Language %04x is not supported",lan);
				currentlanguage = new_language(PRIMARYLANGID(lan), SUBLANGID(lan));
			}
			break;
		case 'm':
			remap = 0;
			break;
		case 'o':
			if (!output_name) output_name = strdup(optarg);
			else error("Too many output files.\n");
			break;
		case 'O':
			if (strcmp(optarg, "res")) 
				error("Output format %s not supported.", optarg);
			break;
		case 'P':
			if (strcmp(optarg, "cat") == 0) no_preprocess = 1;
			else fprintf(stderr, "-P option not yet supported, ignored.\n");
			break;
		case 'v':
			debuglevel = DEBUGLEVEL_CHAT;
			break;
		case 'V':
			printf(version_string);
			exit(0);
			break;
		case 'w':
			if(!strcmp(optarg, "16"))
				win32 = 0;
			else if(!strcmp(optarg, "32"))
				win32 = 1;
			else
				lose++;
			break;
		case 'W':
			pedantic = 1;
			wpp_set_pedantic(1);
			break;
		default:
			lose++;
			break;
		}
	}

	if(lose)
	{
		fprintf(stderr, usage);
		return 1;
	}

	/* If we do need to search standard includes, add them to the path */
	if (stdinc)
	{
		wpp_add_include_path(INCLUDEDIR"/msvcrt");
		wpp_add_include_path(INCLUDEDIR"/windows");
	}
	
	/* Check for input file on command-line */
	if(optind < argc)
	{
		if (!input_name) input_name = argv[optind++];
		else error("Too many input files.\n");
	}

	/* Check for output file on command-line */
	if(optind < argc)
	{
		if (!output_name) output_name = argv[optind++];
		else error("Too many output files.\n");
	}

	/* Check the command line options for invalid combinations */
	if(win32)
	{
		if(!extensions)
		{
			warning("Option -e ignored with 32bit compile\n");
			extensions = 1;
		}
	}

	/* Set alignment power */
	a = alignment;
	for(alignment_pwr = 0; alignment_pwr < 10 && a > 1; alignment_pwr++)
	{
		a >>= 1;
	}
	if(a != 1)
	{
		error("Alignment must be between 1 and 1024");
	}
	if((1 << alignment_pwr) != alignment)
	{
		error("Alignment must be a power of 2");
	}

	/* Kill io buffering when some kind of debuglevel is enabled */
	if(debuglevel)
	{
		setbuf(stdout,0);
		setbuf(stderr,0);
	}

	yydebug = debuglevel & DEBUGLEVEL_TRACE ? 1 : 0;
	yy_flex_debug = debuglevel & DEBUGLEVEL_TRACE ? 1 : 0;

        wpp_set_debug( (debuglevel & DEBUGLEVEL_PPLEX) != 0,
                       (debuglevel & DEBUGLEVEL_PPTRACE) != 0,
                       (debuglevel & DEBUGLEVEL_PPMSG) != 0 );

	/* Set the default defined stuff */
	wpp_add_cmdline_define("__WRC__=" WRC_EXP_STRINGIZE(WRC_MAJOR_VERSION));
	wpp_add_cmdline_define("__WRC_MINOR__=" WRC_EXP_STRINGIZE(WRC_MINOR_VERSION));
	wpp_add_cmdline_define("__WRC_MICRO__=" WRC_EXP_STRINGIZE(WRC_MICRO_VERSION));
	wpp_add_cmdline_define("__WRC_PATCH__=" WRC_EXP_STRINGIZE(WRC_MICRO_VERSION));

	wpp_add_cmdline_define("RC_INVOKED=1");

	if(win32)
	{
		wpp_add_cmdline_define("__WIN32__=1");
		wpp_add_cmdline_define("__FLAT__=1");
	}

	/* Check if the user set a language, else set default */
	if(!currentlanguage)
		currentlanguage = new_language(0, 0);

	/* Generate appropriate outfile names */
	if(!output_name && !preprocess_only)
	{
		output_name = dup_basename(input_name, ".rc");
		strcat(output_name, ".res");
	}

	/* Run the preprocessor on the input */
	if(!no_preprocess)
	{
		/*
		 * Preprocess the input to a temp-file, or stdout if
		 * no output was given.
		 */

		chat("Starting preprocess");

                if (!preprocess_only)
                {
                    atexit(rm_tempfile);
                    ret = wpp_parse_temp( input_name, output_name, &temp_name );
                }
                else if (output_name)
                {
                    FILE *output;

                    if (!(output = fopen( output_name, "w" )))
                        error( "Could not open %s for writing\n", output_name );
                    ret = wpp_parse( input_name, output );
                    fclose( output );
                }
                else
                {
                    ret = wpp_parse( input_name, stdout );
                }

		if(ret)
			exit(1);	/* Error during preprocess */

		if(preprocess_only)
			exit(0);

		input_name = temp_name;
	}

	/* Go from .rc to .res */
	chat("Starting parse");

	if(!(yyin = fopen(input_name, "rb")))
		error("Could not open %s for input\n", input_name);

	ret = yyparse();

	if(input_name) fclose(yyin);

	if(ret) exit(1); /* Error during parse */

	if(debuglevel & DEBUGLEVEL_DUMP)
		dump_resources(resource_top);

	/* Convert the internal lists to binary data */
	resources2res(resource_top);

	chat("Writing .res-file");
	write_resfile(output_name, resource_top);

	return 0;
}


static void rm_tempfile(void)
{
	if(temp_name)
		unlink(temp_name);
}

static void segvhandler(int sig)
{
	fprintf(stderr, "\n%s:%d: Oops, segment violation\n", input_name, line_number);
	fflush(stdout);
	fflush(stderr);
	abort();
}
