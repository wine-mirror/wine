/*
 * Exported functions of the Wine preprocessor
 *
 * Copyrignt 1998 Bertho A. Stultiens
 * Copyright 2002 Alexandre Julliard
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
#include "wine/port.h"

#include <time.h>
#include <stdlib.h>

#include "wpp_private.h"
#include "wpp.h"

static void add_special_defines(void)
{
    time_t now = time(NULL);
    pp_entry_t *ppp;
    char buf[32];

    strftime(buf, sizeof(buf), "\"%b %d %Y\"", localtime(&now));
    pp_add_define( pp_xstrdup("__DATE__"), pp_xstrdup(buf) );

    strftime(buf, sizeof(buf), "\"%H:%M:%S\"", localtime(&now));
    pp_add_define( pp_xstrdup("__TIME__"), pp_xstrdup(buf) );

    ppp = pp_add_define( pp_xstrdup("__FILE__"), pp_xstrdup("") );
    ppp->type = def_special;

    ppp = pp_add_define( pp_xstrdup("__LINE__"), pp_xstrdup("") );
    ppp->type = def_special;
}


/* add a define to the preprocessor list */
void wpp_add_define( const char *name, const char *value )
{
    if (!value) value = "";
    pp_add_define( pp_xstrdup(name), pp_xstrdup(value) );
}


/* add a command-line define of the form NAME=VALUE */
void wpp_add_cmdline_define( const char *value )
{
    char *str = pp_xstrdup(value);
    char *p = strchr( str, '=' );
    if (p) *p++ = 0;
    else p = "";
    pp_add_define( str, pp_xstrdup(p) );
}


/* set the various debug flags */
void wpp_set_debug( int lex_debug, int parser_debug, int msg_debug )
{
    pp_flex_debug   = lex_debug;
    ppdebug         = parser_debug;
    pp_status.debug = msg_debug;
}


/* set the pedantic mode */
void wpp_set_pedantic( int on )
{
    pp_status.pedantic = on;
}


/* the main preprocessor parsing loop */
int wpp_parse( const char *input, FILE *output )
{
    int ret;

    add_special_defines();

    if (!input) ppin = stdin;
    else if (!(ppin = fopen(input, "rt")))
    {
        fprintf(stderr,"Could not open %s\n", input);
        exit(2);
    }

    pp_status.input = input;

    ppout = output;
    fprintf(ppout, "# 1 \"%s\" 1\n", input ? input : "");

    ret = ppparse();

    if (input) fclose(ppin);
    return ret;
}


/* parse into a temporary file */
int wpp_parse_temp( const char *input, char **output_name )
{
    FILE *output;
    int ret, fd;
    char tmpfn[20], *temp_name;

    strcpy(tmpfn,"/tmp/wpp.XXXXXX");

    if((fd = mkstemp(tmpfn)) == -1)
    {
        fprintf(stderr, "Could not generate a temp-name\n");
        exit(2);
    }
    temp_name = pp_xstrdup(tmpfn);

    if (!(output = fdopen(fd, "wt")))
    {
        fprintf(stderr,"Could not open fd %s for writing\n", temp_name);
        exit(2);
    }

    *output_name = temp_name;
    ret = wpp_parse( input, output );
    fclose( output );
    return ret;
}
