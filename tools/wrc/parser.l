/* -*-C-*-
 *
 * Copyright 1994 Martin von Loewis
 * Copyright 1998 Bertho A. Stultiens (BS)
 *
 * 20-Jun-1998 BS	- Changed the filename conversion. Filenames are
 *			  case-sensitive inder *nix, but not under dos.
 *			  default behaviour is to convert to lower case.
 *			- All backslashes are converted to forward and
 *			  both single and double slash is recognized as
 *			  MS/Borland does.
 *			- Fixed a bug in 'yywf' case that prevented
 *			  double quoted names to be scanned propperly.
 *
 * 19-May-1998 BS	- Started to build a preprocessor.
 *			- Changed keyword processing completely to
 *			  table-lookups.
 *
 * 20-Apr-1998 BS	- Added ';' comment stripping
 *
 * 17-Apr-1998 BS	- Made the win32 keywords optional when compiling in
 *			  16bit mode
 *
 * 15-Apr-1998 BS	- Changed string handling to include escapes
 *			- Added unicode string handling (no codepage
 *			  translation though).
 *			- 'Borrowed' the main idea of string scanning from
 *			  the flex manual pages.
 *			- Added conditional handling of scanning depending
 *			  on the state of the parser. This was mainly required
 *			  to distinguish a file to load or raw data that
 *			  follows. MS's definition of filenames is rather
 *			  complex... It can be unquoted or double quoted. If
 *			  double quoted, then the '\\' char is not automatically
 *			  escaped according to Borland's rc compiler, but it
 *			  accepts both "\\path\\file.rc" and "\path\file.rc".
 *			  This makes life very hard! I go for the escaped
 *			  version, as this seems to be the documented way...
 *			- Single quoted strings are now parsed and converted
 *			  here.
 *			- Added comment stripping. The implementation is
 *			  'borrowed' from the flex manpages.
 *			- Rebuild string processing so that it may contain
 *			  escaped '\0'.
 */

/* Exclusive rules when looking for a filename */
%x yywf
%x yywf_s
/* Exclusive string handling */
%x yystr
/* Exclusive unicode string handling */
%x yylstr
/* Exclusive rcdata single quoted data handling */
%x yyrcd
/* Exclusive comment eating... */
%x comment
/* Preprocessor exclusives */
%x pp_incl
%x pp_def
%x pp_undef
%x pp_if
%x pp_ifdef
%x pp_ifndef
%x pp_elif
%x pp_else
%x pp_endif
%x pp_error
/* Set when accumulating #define's expansion text */
%x pp_def_s
/* Set when processing function type defines */
%x pp_ignore
/* Set when need to strip to eol */
%x pp_ignore_eol
/* Set when handling a false #if case */
%x pp_false
/* Set when stripping c-junk */
%x pp_stripe
%x pp_strips
%x pp_stripp
%x pp_stripp_final

/*%option stack*/
%option never-interactive
/*%option noyywrap */
/* Some shortcut definitions */
ws	[ \f\t\r]
cident	[a-zA-Z_][0-9a-zA-Z_]*

%{

#if !defined(YY_FLEX_MAJOR_VERSION) || (1000 * YY_FLEX_MAJOR_VERSION + YY_FLEX_MINOR_VERSION < 2005)
#error Must use flex version 2.5.1 or higher (yy_scan_* routines are required).
#endif

/*#define LEX_DEBUG*/

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "wrc.h"
#include "utils.h"
#include "preproc.h"
#include "parser.h"
#include "newstruc.h"

#include "y.tab.h"

#define YY_USE_PROTOS
#define YY_NO_UNPUT

/* Always update the current character position within a line */
#define YY_USER_ACTION	char_number+=yyleng;

raw_data_t *new_raw_data(void);

void addcchar(char c);
void addwchar(short s);
string_t *get_buffered_cstring(void);
string_t *get_buffered_wstring(void);
string_t *make_string(char *s);
string_t *make_filename(char *s, int len);

int line_number = 1;		/* The current line */
int char_number = 1;		/* The current char pos within the line */
static char *cbuffer;		/* Buffers for string collection */
static int cbufidx;
static int cbufalloc = 0;
static short *wbuffer;
static int wbufidx;
static int wbufalloc = 0;
static int want_nl = 0;		/* Set when newline needs to go to parser */
static int want_ident = 0;	/* Set is #ifdef, #ifndef or defined is seen */
static int stripslevel = 0;	/* Count {} during pp_strips/pp_stripe mode */
static int stripplevel = 0;	/* Count () during pp_strips mode */
static char *substtext = NULL;	/* Holds the substition text while getting a define */
static int cjunk_tagline;	/* Where did we start stripping (helps error tracking) */

#ifdef YY_USE_STACK
void push_to(int start)	{ yy_push_state(start); }
void pop_start(void)	{ yy_pop_state(start); }
#else
#define MAXSTARTSTACK	32
static int startstack[MAXSTARTSTACK];
static int startstackidx = 0;

void push_to(int start)
{
	if(yydebug)
		printf("push_to(%d): %d -> %d\n", line_number, YY_START, start);
	if(startstackidx >= MAXSTARTSTACK-1)
		internal_error(__FILE__, __LINE__, "Start condition stack overflow");
	startstack[startstackidx++] = YY_START;
	BEGIN(start);
}

void pop_start(void)
{
	if(yydebug)
		printf("pop_start(%d): %d <- %d\n", line_number, startstack[startstackidx-1], YY_START);
	if(startstackidx <= 0)
		internal_error(__FILE__, __LINE__, "Start condition stack underflow");
	--startstackidx;
	BEGIN(startstack[startstackidx]);
}
#endif


struct bufferstackentry {
	YY_BUFFER_STATE	bufferstate;	/* Buffer to switch back to */
	struct pp_entry	*define;	/* Points to expanding define
					   or NULL if handling includes
					 */
	int line_number;		/* Line that we were handling */
	int char_number;		/* The current position */
	char *filename;			/* Filename that we were handling */
};

#define MAXBUFFERSTACK 128
static struct bufferstackentry bufferstack[MAXBUFFERSTACK];
static int bufferstackidx = 0;

void push_buffer(YY_BUFFER_STATE buf, struct pp_entry *ppp, char *filename)
{
	if(yydebug)
		printf("push_buffer: %p %p %p\n", buf, ppp, filename);
	if(bufferstackidx >= MAXBUFFERSTACK-1)
		internal_error(__FILE__, __LINE__, "Buffer stack overflow");
	memset(&bufferstack[bufferstackidx], 0, sizeof(bufferstack[0]));
	bufferstack[bufferstackidx].bufferstate = buf;
	bufferstack[bufferstackidx].define = ppp;
	if(ppp)
		ppp->expanding = 1;
	else if(filename)
	{
		/* These will track the yyerror to the correct file and line */
		bufferstack[bufferstackidx].line_number = line_number;
		bufferstack[bufferstackidx].char_number = char_number;
		line_number = 1;
		char_number = 1;
		bufferstack[bufferstackidx].filename = input_name;
		input_name = filename;
	}
	else
		internal_error(__FILE__, __LINE__, "Pushing buffer without knowing where to go to");
	bufferstackidx++;
}

YY_BUFFER_STATE pop_buffer(void)
{
	if(bufferstackidx <= 0)
		return (YY_BUFFER_STATE)0;
	bufferstackidx--;
	if(bufferstack[bufferstackidx].define)
		bufferstack[bufferstackidx].define->expanding = 0;
	else
	{
		line_number = bufferstack[bufferstackidx].line_number;
		char_number = bufferstack[bufferstackidx].char_number;
		input_name = bufferstack[bufferstackidx].filename;
		fclose(yyin);
	}
	if(yydebug)
		printf("pop_buffer: %p %p (%d, %d) %p\n",
			bufferstack[bufferstackidx].bufferstate,
			bufferstack[bufferstackidx].define,
			bufferstack[bufferstackidx].line_number,
			bufferstack[bufferstackidx].char_number,
			bufferstack[bufferstackidx].filename);
	yy_switch_to_buffer(bufferstack[bufferstackidx].bufferstate);
	return bufferstack[bufferstackidx].bufferstate;
}

void do_include(char *name, int namelen)
{
	char *cpy = (char *)xmalloc(namelen);
	strcpy(cpy, name+1);	/* strip leading " or < */
	cpy[namelen-2] = '\0';	/* strip trailing " or > */
	if((yyin = open_include(cpy, name[0] == '"')) == NULL)
		yyerror("Unable to open include file %s", cpy);
	push_buffer(YY_CURRENT_BUFFER, NULL, cpy);
	yy_switch_to_buffer(yy_create_buffer(yyin, YY_BUF_SIZE));
}


struct keyword {
	char	*keyword;
	int	token;
	int	isextension;
	int	needcase;
	int	alwayskeyword;
};

static struct keyword keywords[] = {
	{ "ACCELERATORS", ACCELERATORS, 0, 0, 0},
	{ "ALT", ALT, 0, 0, 0},
	{ "ASCII", ASCII, 0, 0, 0},
	{ "AUTO3STATE", AUTO3STATE, 1, 0, 0},
	{ "AUTOCHECKBOX", AUTOCHECKBOX, 1, 0, 0},
	{ "AUTORADIOBUTTON", AUTORADIOBUTTON, 1, 0, 0},
	{ "BEGIN", tBEGIN, 0, 0, 1},
	{ "BITMAP", tBITMAP, 0, 0, 0},
	{ "BLOCK", BLOCK, 0, 0, 1},
	{ "BUTTON", BUTTON, 1, 0, 0},
	{ "CAPTION", CAPTION, 0, 0, 0},
	{ "CHARACTERISTICS", CHARACTERISTICS, 1, 0, 0},
	{ "CHECKBOX", CHECKBOX, 0, 0, 0},
	{ "CHECKED", CHECKED, 0, 0, 0},
	{ "CLASS", CLASS, 0, 0, 0},
	{ "COMBOBOX", COMBOBOX, 0, 0, 0},
	{ "CONTROL", CONTROL, 0, 0, 0},
	{ "CTEXT", CTEXT, 0, 0, 0},
	{ "CURSOR", CURSOR, 0, 0, 0},
	{ "defined", tDEFINED, 0, 1, 1},
	{ "DEFPUSHBUTTON", DEFPUSHBUTTON, 0, 0, 1},
	{ "DIALOG", DIALOG, 0, 0, 0},
	{ "DIALOGEX", DIALOGEX, 1, 0, 0},
	{ "DISCARDABLE", DISCARDABLE, 0, 0, 0},
	{ "DLGINIT", DLGINIT, 0, 0, 0},
	{ "EDITTEXT", EDITTEXT, 0, 0, 0},
	{ "END", tEND, 0, 0, 1},
	{ "EXSTYLE", EXSTYLE, 0, 0, 0},
	{ "extern", tEXTERN, 0, 1, 1},
	{ "FILEFLAGS", FILEFLAGS, 0, 0, 0},
	{ "FILEFLAGSMASK", FILEFLAGSMASK, 0, 0, 0},
	{ "FILEOS", FILEOS, 0, 0, 0},
	{ "FILESUBTYPE", FILESUBTYPE, 0, 0, 0},
	{ "FILETYPE", FILETYPE, 0, 0, 0},
	{ "FILEVERSION", FILEVERSION, 0, 0, 0},
	{ "FIXED", tFIXED, 0, 0, 0},
	{ "FONT", FONT, 0, 0, 0},
	{ "GRAYED", GRAYED, 0, 0, 0},
	{ "GROUPBOX", GROUPBOX, 0, 0, 0},
	{ "HELP", HELP, 0, 0, 0},
	{ "ICON", ICON, 0, 0, 0},
	{ "IMPURE", IMPURE, 0, 0, 0},
	{ "INACTIVE", INACTIVE, 0, 0, 0},
	{ "LANGUAGE", LANGUAGE, 1, 0, 1},
	{ "LISTBOX", LISTBOX, 0, 0, 0},
	{ "LOADONCALL", LOADONCALL, 0, 0, 0},
	{ "LTEXT", LTEXT, 0, 0, 0},
	{ "MENU", MENU, 0, 0, 0},
	{ "MENUBARBREAK", MENUBARBREAK, 0, 0, 0},
	{ "MENUBREAK", MENUBREAK, 0, 0, 0},
	{ "MENUEX", MENUEX, 1, 0, 0},
	{ "MENUITEM", MENUITEM, 0, 0, 0},
	{ "MESSAGETABLE", MESSAGETABLE, 1, 0, 0},
	{ "MOVEABLE", MOVEABLE, 0, 0, 0},
	{ "NOINVERT", NOINVERT, 0, 0, 0},
	{ "NOT", NOT, 0, 0, 0},
	{ "POPUP", POPUP, 0, 0, 0},
	{ "PRELOAD", PRELOAD, 0, 0, 0},
	{ "PRODUCTVERSION", PRODUCTVERSION, 0, 0, 0},
	{ "PURE", tPURE, 0, 0, 0},
	{ "PUSHBUTTON", PUSHBUTTON, 0, 0, 0},
	{ "RADIOBUTTON", RADIOBUTTON, 0, 0, 0},
	{ "RCDATA", RCDATA, 0, 0, 0},
	{ "RTEXT", RTEXT, 0, 0, 0},
	{ "SCROLLBAR", SCROLLBAR, 0, 0, 0},
	{ "SEPARATOR", SEPARATOR, 0, 0, 0},
	{ "SHIFT", SHIFT, 0, 0, 0},
	{ "STATE3", STATE3, 1, 0, 0},
	{ "STRING", tSTRING, 0, 0, 0},
	{ "STRINGTABLE", STRINGTABLE, 0, 0, 1},
	{ "STYLE", STYLE, 0, 0, 0},
	{ "TOOLBAR", TOOLBAR, 1, 0, 0},
	{ "typedef", tTYPEDEF, 0, 1, 1},
	{ "VALUE", VALUE, 0, 0, 0},
	{ "VERSION", VERSION, 1, 0, 0},
	{ "VERSIONINFO", VERSIONINFO, 0, 0, 0},
	{ "VIRTKEY", VIRTKEY, 0, 0, 0}
};

#define NKEYWORDS	(sizeof(keywords)/sizeof(keywords[0]))
#define KWP(p)		((struct keyword *)(p))
int kw_cmp_func(const void *s1, const void *s2)
{
	int ret;
	ret = strcasecmp(KWP(s1)->keyword, KWP(s2)->keyword);
	if(!ret && (KWP(s1)->needcase || KWP(s2)->needcase))
		return strcmp(KWP(s1)->keyword, KWP(s2)->keyword);
	else
		return ret;
}

#define KW_BSEARCH
#define DO_SORT
struct keyword *iskeyword(char *kw)
{
	struct keyword *kwp;
	struct keyword key;
	key.keyword = kw;
	key.needcase = 0;
#ifdef DO_SORT
	{
		/* Make sure that it is sorted for bsearsh */
		static int sorted = 0;
		if(!sorted)
		{
			qsort(keywords, NKEYWORDS, sizeof(keywords[0]), kw_cmp_func);
			sorted = 1;
		}
	}
#endif
#ifdef KW_BSEARCH
	kwp = bsearch(&key, keywords, NKEYWORDS, sizeof(keywords[0]), kw_cmp_func);
#else
	{
		int i;
		for(i = 0; i < NKEYWORDS; i++)
		{
			if(!kw_cmp_func(&key, &keywords[i]))
				break;
		}
		if(i < NKEYWORDS)
			kwp = &keywords[i];
		else
			kwp = NULL;
	}
#endif

#ifdef LEX_DEBUG
	if(kwp && !strcmp(kwp->keyword, "LANGUAGE"))
		printf("Got Language\n");
#endif
	if(kwp == NULL || (kwp->isextension && !extensions))
		return NULL;
	else
		return kwp;
}

void add_to_substtext(char *text, int len)
{
	if(!substtext)
	{
		substtext = xstrdup(text);
	}
	else
	{
		substtext = (char *)xrealloc(substtext, strlen(substtext)+len+1);
		strcat(substtext, text);
	}
}

%}

%%
	/* #include handling */
^{ws}*#{ws}*include{ws}*	push_to(pp_incl);
<pp_incl>\<[^\n\>]+\>		do_include(yytext, yyleng); pop_start();
<pp_incl>\"[^\n\>]+\"		do_include(yytext, yyleng); pop_start();
<pp_incl>.		yyerror("Malformed #include");

	/* #define handling */
^{ws}*#{ws}*define{ws}*	push_to(pp_def);
<pp_def>{cident}	{
				set_define(yytext);
				push_to(pp_def_s);
			}
<pp_def>{cident}\(	push_to(pp_ignore); /* Ignore function-like defines for now*/
<pp_def>.		yyerror("Malformed #define");

<pp_ignore,pp_def_s>[^\/\\\n]*	{
			if(YY_START == pp_def_s)
				add_to_substtext(yytext, yyleng);
		}
<pp_ignore,pp_def_s>\/[^\/\*][^\/\\\n]*	{ /* Comment is handled in normal handling */
			if(YY_START == pp_def_s)
				add_to_substtext(yytext, yyleng);
		}
<pp_ignore,pp_def_s>\\{ws}*\n	line_number++;	char_number = 1; /* Line continuation */
<pp_ignore,pp_def_s>\n		{
			if(YY_START == pp_def_s)
			{
				add_define(substtext ? substtext : "");
				free(substtext);
				substtext = NULL;
			}
			line_number++;
			char_number = 1;
			pop_start();
			pop_start();
		}

	/* #undef handling */
^{ws}*#{ws}*undef{ws}*	push_to(pp_undef);
<pp_undef>{cident}	{
				del_define(yytext);
				pop_start();
				/*push_to(pp_ignore);*/
			}

	/* Conditional handling */
<INITIAL,pp_strips,pp_stripe,pp_stripp,pp_false>^{ws}*#{ws}*if{ws}*	{
			if(YY_START == pp_false)
			{
				if(yydebug)
					printf("(%d)#if ignored\n", line_number);
				push_if(0, 0, 1);
				push_to(pp_ignore_eol);
			}
			else
			{
				push_to(INITIAL);
				want_nl = 1;
				return tIF;
			}
		}
<INITIAL,pp_strips,pp_stripe,pp_stripp,pp_false>^{ws}*#{ws}*ifdef{ws}*	{
			if(YY_START == pp_false)
			{
				if(yydebug)
					printf("(%d)#ifdef ignored\n", line_number);
				push_if(0, 0, 1);
				push_to(pp_ignore_eol);
			}
			else
			{
				push_to(INITIAL);
				want_nl = 1;
				want_ident = 1;
				return tIFDEF;
			}
		}
<INITIAL,pp_strips,pp_stripe,pp_stripp,pp_false>^{ws}*#{ws}*ifndef{ws}*	{
			if(YY_START == pp_false)
			{
				if(yydebug)
					printf("(%d)#ifndef ignored\n", line_number);
				push_if(0, 0, 1);
				push_to(pp_ignore_eol);
			}
			else
			{
				push_to(INITIAL);
				want_nl = 1;
				want_ident = 1;
				return tIFNDEF;
			}
		}
<INITIAL,pp_strips,pp_stripe,pp_stripp,pp_false>^{ws}*#{ws}*elif{ws}*	{
			if(!isnevertrue_if())
			{
				push_to(INITIAL);
				want_nl = 1;
				return tELIF;
			}
			else if(YY_START == pp_false)
				push_to(pp_ignore_eol);
			if(yydebug)
				printf("(%d)#elif ignored\n", line_number);
		}
<INITIAL,pp_strips,pp_stripe,pp_stripp,pp_false>^{ws}*#{ws}*else{ws}*	{
			if(!isnevertrue_if())
			{
				push_to(INITIAL);
				want_nl = 1;
				return tELSE;
			}
			if(yydebug)
				printf("(%d)#else ignored\n", line_number);
		}
<INITIAL,pp_strips,pp_stripe,pp_stripp,pp_false>^{ws}*#{ws}*endif{ws}*	{
			if(!isnevertrue_if())
			{
				want_nl = 1;
				return tENDIF;
			}
			else
			{
				if(yydebug)
					printf("(%d)#endif ignored\n", line_number);
				pop_if();
			}
		}

	/* The error directive */
<INITIAL,pp_strips,pp_stripe,pp_stripp>^{ws}*#{ws}*error{ws}*	push_to(pp_error);
<pp_error>[^\n]* 	yyerror("Error directive: %s", yytext);
<pp_false>^{ws}*#{ws}*error[^\n]* {
			if(yydebug)
				printf("(%d)#error ignored\n", line_number);
		}

	/* preprocessor junk */
<INITIAL,pp_strips,pp_stripe,pp_stripp,pp_false>^{ws}*#{ws}*pragma[^\n]*	;	/* Ignore #pragma */
<INITIAL,pp_strips,pp_stripe,pp_stripp,pp_false>^{ws}*#{ws}*ident[^\n]*		;	/* Ignore #ident */
<INITIAL,pp_strips,pp_stripe,pp_stripp,pp_false>^{ws}*#{ws}*line[^\n]*		;	/* Ignore #line */
 /* We'll get an error on malformed #xxx statements
  * by not recognising '#' at all. This helps tracking
  * preprocessor errors.
  */
 /*^{ws}*#{ws}*			;	 Ignore # */

<pp_strips>\{		stripslevel++;
<pp_strips>\}		stripslevel--;
<pp_strips>;		if(!stripslevel) pop_start();
<pp_strips>\/[^*\n]		; /* To catch comments */
<pp_strips>[^\{\};\n#/]*	; /* Ignore rest */

<pp_stripe>\{		stripslevel++;
<pp_stripe>\}	{
			stripslevel--;
			if(!stripslevel) pop_start();
		}
<pp_stripe>;		if(!stripslevel) pop_start();
<pp_stripe>\/[^*\n]		; /* To catch comments */
<pp_stripe>[^\{\};\n#/]*	; /* Ignore rest */

<pp_stripp>\(		stripplevel++;
<pp_stripp>\)		{
				stripplevel--;
				if(!stripplevel)
				{
					pop_start();
					push_to(pp_stripp_final);
				}
			}
<pp_stripp>\/[^*\n]		; /* To catch comments */
<pp_stripp>[^\(\);\n#/]*	; /* Ignore rest */

<pp_stripp_final>{ws}*	; /* Ignore */
<pp_stripp_final>;	pop_start(); /* Kill the semicolon */
<pp_stripp_final>\n	line_number++; char_number = 1; pop_start();
<pp_stripp_final>.	yyless(0); pop_start();

<pp_false>.		;	/* Ignore everything except #xxx during false #if state */

<pp_ignore_eol>[^\n]*	pop_start();

 /* These are special cases due to filename scanning */
<yywf>[Dd][Ii][Ss][Cc][Aa][Rr][Dd][Aa][Bb][Ll][Ee]	return DISCARDABLE;
<yywf>[Ff][Ii][Xx][Ee][Dd]				return tFIXED;
<yywf>[Ii][Mm][Pp][Uu][Rr][Ee]				return IMPURE;
<yywf>[Mm][Oo][Vv][Ee][Aa][Bb][Ll][Ee]			return MOVEABLE;
<yywf>[Ll][Oo][Aa][Dd][Oo][Nn][Cc][Aa][Ll][Ll]		return LOADONCALL;
<yywf>[Pp][Rr][Ee][Ll][Oo][Aa][Dd]			return PRELOAD;
<yywf>[Pp][Uu][Rr][Ee]					return tPURE;

\{			return tBEGIN;
\}			return tEND;

[0-9]+[lL]?		{ yylval.num = strtoul(yytext,  0, 10); return toupper(yytext[yyleng-1]) == 'L' ? LNUMBER : NUMBER; }
0[xX][0-9A-Fa-f]+[lL]?	{ yylval.num = strtoul(yytext,  0, 16); return toupper(yytext[yyleng-1]) == 'L' ? LNUMBER : NUMBER; }
0[oO][0-7]+[lL]?	{ yylval.num = strtoul(yytext+2, 0, 8); return toupper(yytext[yyleng-1]) == 'L' ? LNUMBER : NUMBER; }
[A-Za-z_0-9]+		{
				struct keyword *token;
				struct pp_entry *ppp;

				want_rscname = 0;
				
				if(want_ident)
				{
					/* Prevent preprocessor subst */
					want_ident = 0;
					yylval.str = make_string(yytext);
				#ifdef LEX_DEBUG
					printf("want IDENT (%s, %d, %d): <%s>\n", input_name, line_number, char_number, yytext);
				#endif
					return IDENT;
				}
				else if((ppp = pp_lookup(yytext)) != NULL)
				{
					/* Do preprocessor substitution,
					 * but expand only if macro is not
					 * already expanding.
					 */
					if(!ppp->expanding)
					{
				#ifdef LEX_DEBUG
						printf("expand IDENT (%s, %d, %d): <%s>\n", input_name, line_number, char_number, yytext);
				#endif
						push_buffer(YY_CURRENT_BUFFER, ppp, NULL);
						yy_scan_string(ppp->subst);
					}
				}
				else if((token = iskeyword(yytext)) != NULL
				  && !(!token->alwayskeyword && want_rscname))
				{
					switch(token->token)
					{
					case tDEFINED:
						want_ident = 1;
						break;
					/*case RCDATA:*/
					case CURSOR:
					case tBITMAP:
					case MESSAGETABLE:
					case DLGINIT:
						push_to(yywf);
						break;
					case FONT:
					case ICON:
						if(!indialog)
							push_to(yywf);
						break;
					case DIALOG:
					case DIALOGEX:
						indialog = 1;
						break;
					}
					return token->token;
				}
				else
				{
					yylval.str = make_string(yytext);
				#ifdef LEX_DEBUG
					printf("%s IDENT (%s, %d, %d): <%s>\n",
						want_rscname ? "rscname" : "just",
						input_name,
						line_number,
						char_number,
						yytext);
				#endif
					return IDENT;
				}
			}
\|\|			return LOGOR;
\&\&			return LOGAND;
\=\=			return EQ;
\!\=			return NE;
\<\=			return LTE;
\>\=			return GTE;

<yywf>[^ \f\t\r\n\"]*	{ pop_start(); yylval.str = make_filename(yytext, yyleng); return FILENAME; }
<yywf>\"		push_to(yywf_s);
<yywf_s>[^\"\n]*\"	{ pop_start(); pop_start(); yylval.str = make_filename(yytext, yyleng-1); return FILENAME; }
<yywf_s>\n		yyerror("Newline in filename");

L\"			{
				push_to(yylstr);
				wbufidx = 0;
				if(!win32)
					yywarning("16bit resource contains unicode strings\n");
			}
<yylstr>\"		{
				pop_start();
				yylval.str = get_buffered_wstring();
				return tSTRING;
			}
<yylstr>\n		yyerror("Unterminated string");
<yylstr>\\[0-7]{1,6}	{ /* octal escape sequence */
				int result;
				result = strtol(yytext+1, 0, 8);
				if ( result > 0xffff )
					yyerror("Character constant out of range");
				addwchar((short)result);
			}
<yylstr>\\x[0-9a-fA-F]{4} {  /* hex escape sequence */
				int result;
				result = strtol(yytext+2, 0, 16);
				addwchar((short)result);
			}
<yylstr>\\[0-9]+	yyerror("Bad escape secuence");
<yylstr>\\a		addwchar('\a');
<yylstr>\\b		addwchar('\b');
<yylstr>\\f		addwchar('\f');
<yylstr>\\n		addwchar('\n');
<yylstr>\\r		addwchar('\r');
<yylstr>\\t		addwchar('\t');
<yylstr>\\v		addwchar('\v');
<yylstr>\\(.|\n)	addwchar(yytext[1]);
<yylstr>\"\"		addcchar('\"');		/* "bla""bla"  -> "bla\"bla" */
<yylstr>\\\"\"		addcchar('\"');		/* "bla\""bla" -> "bla\"bla" */
<yylstr>\"{ws}+\"	;			/* "bla" "bla" -> "blabla" */
<yylstr>[^\\\n\"]+	{
				char *yptr = yytext;
				while(*yptr)	/* FIXME: codepage translation */
					addwchar(*yptr++ & 0xff);
			}

\"			{
				push_to(yystr);
				cbufidx = 0;
			}
<yystr>\"		{
				pop_start();
				yylval.str = get_buffered_cstring();
				return tSTRING;
			}
<yystr>\n		yyerror("Unterminated string");
<yystr>\\[0-7]{1,3}	{ /* octal escape sequence */
				int result;
				result = strtol(yytext+1, 0, 8);
				if ( result > 0xff )
					yyerror("Character constant out of range");
				addcchar((char)result);
			}
<yystr>\\x[0-9a-fA-F]{2} {  /* hex escape sequence */
				int result;
				result = strtol(yytext+2, 0, 16);
				addcchar((char)result);
			}
<yystr>\\[0-9]+		yyerror("Bad escape secuence");
<yystr>\\a		addcchar('\a');
<yystr>\\b		addcchar('\b');
<yystr>\\f		addcchar('\f');
<yystr>\\n		addcchar('\n');
<yystr>\\r		addcchar('\r');
<yystr>\\t		addcchar('\t');
<yystr>\\v		addcchar('\v');
<yystr>\\(.|\n)		addcchar(yytext[1]);
<yystr>[^\\\n\"]+	{
				char *yptr = yytext;
				while(*yptr)
					addcchar(*yptr++);
			}
<yystr>\"\"		addcchar('\"');		/* "bla""bla"   -> "bla\"bla" */
<yystr>\\\"\"		addcchar('\"');		/* "bla\""bla"  -> "bla\"bla" */
<yystr>\"{ws}+\"	;			/* "bla" "bla"  -> "blabla" */
<yystr>.		yywarning("Matched %c", *yytext);


\'			{
				push_to(yyrcd);
				cbufidx = 0;
			}
<yyrcd>\'		{
				pop_start();
				yylval.raw = new_raw_data();
				yylval.raw->size = cbufidx;
				yylval.raw->data = xmalloc(yylval.raw->size);
				memcpy(yylval.raw->data, cbuffer, yylval.raw->size);
				return RAWDATA;
			}
<yyrcd>[0-9a-fA-F]{2}	{
				int result;
				result = strtol(yytext, 0, 16);
				addcchar((char)result);
			}
<yyrcd>{ws}+		;	/* Ignore space */
<yyrcd>\n		line_number++; char_number = 1;
<yyrcd>.		yyerror("Malformed data-line");

<INITIAL,pp_ignore,pp_def_s,pp_strips,pp_stripe,pp_stripp>"/*"	push_to(comment);	/* Eat comment */
<comment>[^*\n]*	;
<comment>"*"+[^*/\n]*	;
<comment>\n		line_number++; char_number = 1;
<comment>"*"+"/"	pop_start();

;[^\n]*			; /* Eat comment */
<INITIAL,pp_ignore,pp_def_s>"//"[^\n]*		; /* Eat comment */

<INITIAL,yywf,pp_false,pp_strips,pp_stripe,pp_stripp>\n	{
				if(YY_START == yywf)
					pop_start();
				line_number++;
				char_number = 1;
				if(want_nl)
				{
					want_nl = 0;
					return tNL;
				}
			}
<INITIAL,yywf>{ws}+	;	/* Eat whitespace */

<INITIAL>.		return yytext[0];
<<EOF>>			{
				YY_BUFFER_STATE b = YY_CURRENT_BUFFER;
				if(!pop_buffer())
                                {
					if(YY_START == pp_strips || YY_START == pp_stripe || YY_START == pp_stripp || YY_START == pp_stripp_final)
						yyerror("Unexpected end of file during c-junk scanning (started at %d)", cjunk_tagline);
					else
						yyterminate();
                                }
				yy_delete_buffer(b);
			}

<*>.|\n			{
				/* Catch all rule to find any unmatched text */
				if(*yytext == '\n')
				{
					line_number++;
					char_number = 1;
				}
				yywarning("Unmatched text '%c' (0x%02x) YY_START=%d stripslevel=%d",
					isprint(*yytext) ? *yytext : '.', *yytext, YY_START,stripslevel);
			}

%%

#ifndef yywrap
int yywrap(void)
{
#if 0
	if(bufferstackidx > 0)
	{
		return 0;
 	}
#endif
	return 1;
}
#endif

/* These dup functions copy the enclosed '\0' from
 * the resource string.
 */
void addcchar(char c)
{
	if(cbufidx >= cbufalloc)
	{
		cbufalloc += 1024;
		cbuffer = xrealloc(cbuffer, cbufalloc * sizeof(cbuffer[0]));
		if(cbufalloc > 65536)
			yywarning("Reallocating string buffer larger than 64kB");
	}
	cbuffer[cbufidx++] = c;
}

void addwchar(short s)
{
	if(wbufidx >= wbufalloc)
	{
		wbufalloc += 1024;
		wbuffer = xrealloc(wbuffer, wbufalloc * sizeof(wbuffer[0]));
		if(wbufalloc > 65536)
			yywarning("Reallocating wide string buffer larger than 64kB");
	}

	/*
	 * BS 08-Aug-1999 FIXME: The '& 0xff' is probably a bug, but I have
	 * not experienced it yet and I seem to remember that this was for
	 * a reason. But, as so many things you tend to forget why.
	 * I guess that there were problems due to the sign extension of
	 * shorts WRT chars (e.g. 0x80 becomes 0xff80 instead of 0x0080).
	 * This should then be fixed in the lexer calling the function.
	 */
	wbuffer[wbufidx++] = (short)(s & 0xff);
}

string_t *get_buffered_cstring(void)
{
	string_t *str = new_string();
	str->size = cbufidx;
	str->type = str_char;
	str->str.cstr = (char *)xmalloc(cbufidx+1);
	memcpy(str->str.cstr, cbuffer, cbufidx);
	str->str.cstr[cbufidx] = '\0';
/*	printf("got cstring \"%s\"\n", str->str.cstr); */
	return str;
}

string_t *get_buffered_wstring(void)
{
	string_t *str = new_string();
	str->size = wbufidx;
	str->type = str_unicode;
	str->str.wstr = (short *)xmalloc(2*(wbufidx+1));
	memcpy(str->str.wstr, wbuffer, wbufidx);
	str->str.wstr[wbufidx] = 0;
	return str;
}

string_t *make_string(char *s)
{
	string_t *str = new_string();
	str->size = strlen(s);
	str->type = str_char;
	str->str.cstr = (char *)xmalloc(str->size+1);
	memcpy(str->str.cstr, s, str->size+1);
	return str;
}

string_t *make_filename(char *s, int len)
{
	char *cptr;
	string_t *str = new_string();

	str->size = len;
	str->type = str_char;
	str->str.cstr = (char *)xmalloc(str->size+1);
	memcpy(str->str.cstr, s, str->size);
	str->str.cstr[str->size] = '\0';

	/* Remove escaped backslash and convert to forward */
	cptr = str->str.cstr;
	for(cptr = str->str.cstr; (cptr = strchr(cptr, '\\')) != NULL; cptr++)
	{
		if(cptr[1] == '\\')
		{
			memmove(cptr, cptr+1, strlen(cptr));
			str->size--;
		}
		*cptr = '/';
	}

	/* Convert to lower case. Seems to be reasonable to do */
	for(cptr = str->str.cstr; !leave_case && *cptr; cptr++)
	{
		*cptr = tolower(*cptr);
	}
	return str;
}

/* Called from the parser to signal filename request */
void set_yywf(void)
{
	push_to(yywf);
}

/* Called from the parser to signal preprocessor if case */
void set_pp_ignore(int state)
{
	if(state)
		push_to(pp_false);
	else
		pop_start();
}

/* Called from the parser to kill c-junk */
void strip_extern(void)
{
	cjunk_tagline = line_number;
	push_to(pp_stripe);
}

void strip_til_semicolon(void)
{
	cjunk_tagline = line_number;
	push_to(pp_strips);
}

void strip_til_parenthesis(void)
{
	cjunk_tagline = line_number;
	stripplevel = 1;	/* One scanned already */
	push_to(pp_stripp);
}

