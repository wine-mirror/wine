%{
/*
 *
 * Copyright  Martin von Loewis, 1994
 *
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "parser.h"
#include "y.tab.h"
%}
%%
ACCELERATORS	return ACCELERATORS;
ALT		return ALT;
ASCII		return ASCII;
BEGIN		return tBEGIN;
BITMAP		return tBITMAP;
CAPTION		return CAPTION;
CHECKBOX	return CHECKBOX;
CHECKED		return CHECKED;
CLASS		return CLASS;
COMBOBOX	return COMBOBOX;
CONTROL		return CONTROL;
CTEXT		return CTEXT;
CURSOR		return CURSOR;
DEFPUSHBUTTON	return DEFPUSHBUTTON;
DIALOG		return DIALOG;
DISCARDABLE	return DISCARDABLE;
EDITTEXT	return EDITTEXT;
END		return tEND;
FIXED		return FIXED;
FONT		return FONT;
GRAYED		return GRAYED;
GROUPBOX	return GROUPBOX;
HELP		return HELP;
ICON		return ICON;
INACTIVE	return INACTIVE;
LISTBOX		return LISTBOX;
LTEXT		return LTEXT;
MENU		return MENU;
MENUBARBREAK	return MENUBARBREAK;
MENUBREAK	return MENUBREAK;
MENUITEM	return MENUITEM;
MOVEABLE	return MOVEABLE;
LOADONCALL	return LOADONCALL;
NOINVERT	return NOINVERT;
NOT			return NOT;
NUMBER		return NUMBER;
POPUP		return POPUP;
PRELOAD		return PRELOAD;
PUSHBUTTON	return PUSHBUTTON;
PURE		return PURE;
RADIOBUTTON	return RADIOBUTTON;
RCDATA		return RCDATA;
RTEXT		return RTEXT;
SCROLLBAR	return SCROLLBAR;
SHIFT		return SHIFT;
SEPARATOR	return SEPARATOR;
STRING		return STRING;
STRINGTABLE	return STRINGTABLE;
STYLE		return STYLE;
VERSIONINFO	return VERSIONINFO;
VIRTKEY		return VIRTKEY;
\{			return tBEGIN;
\}			return tEND;
[+-]?[0-9]+	yylval.num=atoi(yytext);return NUMBER;
0x[0-9A-Fa-f]+L?	yylval.num=strtoul(yytext,0,16);return NUMBER;
[A-Za-z][A-Za-z_0-9]*	yylval.str=strdup(yytext);return IDENT;
\"[^"]*\"	yylval.str=parse_c_string(yytext);return STRING;
\'[^']*\'   yylval.str=strdup(yytext+1);return SINGLE_QUOTED;
[ \t\n\r]		;
.			return yytext[0];
%%

#ifndef yywrap
int yywrap(void) { return 1; }
#endif

