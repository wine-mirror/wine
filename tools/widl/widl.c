/*
 * IDL Compiler
 *
 * Copyright 2002 Ove Kaaven
 * based on WRC code by Bertho Stultiens
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
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <signal.h>

#define WIDL_FULLVERSION "0.1"

#include "widl.h"
#include "utils.h"
#include "parser.h"
#include "proxy.h"
#include "../wpp/wpp.h"

/* future options to reserve characters for: */
/* a = alignment of structures */
/* A = ACF input filename */
/* c = client stub only? */
/* C = client stub filename */
/* J = do not search standard include path */
/* O = generate interpreted stubs */
/* p = proxy only? */
/* P = proxy filename */
/* s = server stub only? */
/* S = server stub filename */
/* t = typelib only? */
/* T = typelib filename */
/* u = UUID file only? */
/* U = UUID filename */
/* w = select win16/win32 output (?) */

static char usage[] =
"Usage: widl [options...] infile.idl\n"
"   -b          Make headers compatible with ICOM macros\n"
"   -B          Make headers use ICOM macros\n"
"   -d n        Set debug level to 'n'\n"
"   -D id[=val] Define preprocessor identifier id=val\n"
"   -E          Preprocess only\n"
"   -h          Generate headers only\n"
"   -H file     Name of header file (default is infile.h)\n"
"   -I path     Set include search dir to path (multiple -I allowed)\n"
"   -N          Do not preprocess input\n"
"   -V          Print version and exit\n"
"   -W          Enable pedantic warnings\n"
"Debug level 'n' is a bitmask with following meaning:\n"
"    * 0x01 Tell which resource is parsed (verbose mode)\n"
"    * 0x02 Dump internal structures\n"
"    * 0x04 Create a parser trace (yydebug=1)\n"
"    * 0x08 Preprocessor messages\n"
"    * 0x10 Preprocessor lex messages\n"
"    * 0x20 Preprocessor yacc trace\n"
;

char version_string[] = "Wine IDL Compiler Version " WIDL_FULLVERSION "\n"
			"Copyright 2002 Ove Kaaven\n";

int win32 = 1;
int debuglevel = DEBUGLEVEL_NONE;

int pedantic = 0;
int preprocess_only = 0;
int header_only = 0;
int no_preprocess = 0;
int compat_icom = 0;
int use_icom = 0;

char *input_name;
char *header_name;
char *header_token;
char *proxy_name;
char *proxy_token;
char *temp_name;

int line_number = 1;
int char_number = 1;

FILE *header;
FILE *proxy;

time_t now;

int getopt (int argc, char *const *argv, const char *optstring);
static void rm_tempfile(void);
static void segvhandler(int sig);

static char *make_token(const char *name)
{
  char *token;
  char *slash;
  int i;

  slash = strrchr(name, '/');
  if (slash) name = slash + 1;

  token = xstrdup(name);
  for (i=0; token[i]; i++) {
    if (!isalnum(token[i])) token[i] = '_';
    else token[i] = toupper(token[i]);
  }
  return token;
}

int main(int argc,char *argv[])
{
  extern char* optarg;
  extern int   optind;
  int optc;
  int ret;

  signal(SIGSEGV, segvhandler);

  now = time(NULL);

  while((optc = getopt(argc, argv, "bBd:D:EhH:I:NVW")) != EOF) {
    switch(optc) {
    case 'b':
      compat_icom = 1;
      break;
    case 'B':
      use_icom = 1;
      break;
    case 'd':
      debuglevel = strtol(optarg, NULL, 0);
      break;
    case 'D':
      wpp_add_cmdline_define(optarg);
      break;
    case 'E':
      preprocess_only = 1;
      break;
    case 'h':
      header_only = 1;
      break;
    case 'H':
      header_name = strdup(optarg);
      break;
    case 'I':
      wpp_add_include_path(optarg);
      break;
    case 'N':
      no_preprocess = 1;
      break;
    case 'V':
      printf(version_string);
      return 0;
    case 'W':
      pedantic = 1;
      break;
    default:
      fprintf(stderr, usage);
      return 1;
    }
  }

  if(optind < argc) {
    input_name = argv[optind];
  }
  else {
    fprintf(stderr, usage);
    return 1;
  }

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

  if (!header_name) {
    header_name = dup_basename(input_name, ".idl");
    strcat(header_name, ".h");
  }

  if (!proxy_name) {
    proxy_name = dup_basename(input_name, ".idl");
    proxy_token = xstrdup(proxy_name);
    strcat(proxy_name, "_p.c");
  }

  wpp_add_cmdline_define("__WIDL__");

  atexit(rm_tempfile);
  if (!no_preprocess)
  {
    chat("Starting preprocess");

    if (!preprocess_only)
    {
        ret = wpp_parse_temp( input_name, header_name, &temp_name );
    }
    else
    {
        ret = wpp_parse( input_name, stdout );
    }

    if(ret) exit(1);
    if(preprocess_only) exit(0);
    if(!(yyin = fopen(temp_name, "r"))) {
      fprintf(stderr, "Could not open %s for input\n", temp_name);
      return 1;
    }
  }
  else {
    if(!(yyin = fopen(input_name, "r"))) {
      fprintf(stderr, "Could not open %s for input\n", input_name);
      return 1;
    }
  }

  header_token = make_token(header_name);

  header = fopen(header_name, "w");
  fprintf(header, "/*** Autogenerated by WIDL %s from %s - Do not edit ***/\n", WIDL_FULLVERSION, input_name);
  fprintf(header, "#include \"rpc.h\"\n" );
  fprintf(header, "#include \"rpcndr.h\"\n\n" );
  fprintf(header, "#ifndef __WIDL_%s\n", header_token);
  fprintf(header, "#define __WIDL_%s\n", header_token);
  fprintf(header, "#ifdef __cplusplus\n");
  fprintf(header, "extern \"C\" {\n");
  fprintf(header, "#endif\n");

  ret = yyparse();

  finish_proxy();
  fprintf(header, "#ifdef __cplusplus\n");
  fprintf(header, "}\n");
  fprintf(header, "#endif\n");
  fprintf(header, "#endif /* __WIDL_%s */\n", header_token);
  fclose(header);
  fclose(yyin);

  if(ret) {
    exit(1);
  }

  return 0;
}

static void rm_tempfile(void)
{
  abort_import();
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
