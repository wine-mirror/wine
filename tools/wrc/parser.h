/*
 * Copyright 1998 Bertho A. Stultiens (BS)
 *
 * Shared things between parser.l and parser.y and some others
 */

#ifndef __WRC_PARSER_H
#define __WRC_PARSER_H

/* From parser.y */
extern int yydebug;
extern int indialog;		/* Set when parsing dialogs */
extern int want_rscname;	/* Set when a resource's name is required */

int yyparse(void);
void split_icons(raw_data_t *rd, icon_group_t *icog, int *nico);
void split_cursors(raw_data_t *rd, cursor_group_t *curg, int *ncur);

/* From parser.l */
extern FILE *yyin;
extern char *yytext;
extern int line_number;
extern int char_number;

int yylex(void);
void set_yywf(void);
void set_pp_ignore(int state);
void push_to(int start);
void pop_start(void);
void strip_extern(void);
void strip_til_semicolon(void);
void strip_til_parenthesis(void);

#endif

