/*
 * Copyright 1998 Bertho A. Stultiens (BS)
 *
 * Shared things between parser.l and parser.y and some others
 */

#ifndef __WRC_PARSER_H
#define __WRC_PARSER_H

/* From parser.y */
extern int yydebug;
extern int want_nl;		/* Set when getting line-numers */
extern int want_id;		/* Set when getting the resource name */

int yyparse(void);

/* From parser.l */
extern FILE *yyin;
extern char *yytext;
extern int yy_flex_debug;

int yylex(void);
void set_pp_ignore(int state);
void strip_til_semicolon(void);
void strip_til_parenthesis(void);

#endif

