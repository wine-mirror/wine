%{
/*
 * Copyright  Martin von Loewis, 1994
 * Copyright 1998 Bertho A. Stultiens (BS)
 *           1999 Juergen Schmied (JS)
 *
 * 6-Nov-1999 JS        - see CHANGES
 * 
 * 29-Dec-1998 AdH	- Grammar and function extensions.
 *			     grammar: TOOLBAR resources, Named ICONs in 
 *				DIALOGS
 *			     functions: semantic actions for the grammar 
 *				changes, resource files can now be anywhere
 *				on the include path instead of just in the
 *				current directory
 *
 * 20-Jun-1998 BS	- Fixed a bug in load_file() where the name was not
 *			  printed out correctly.
 *
 * 17-Jun-1998 BS	- Fixed a bug in CLASS statement parsing which should
 *			  also accept a tSTRING as argument.
 *
 * 25-May-1998 BS	- Found out that I need to support language, version
 *			  and characteristics in inline resources (bitmap,
 *			  cursor, etc) but they can also be specified with
 *			  a filename. This renders my filename-scanning scheme
 *			  worthless. Need to build newline parsing to solve
 *			  this one.
 *			  It will come with version 1.1.0 (sigh).
 *
 * 19-May-1998 BS	- Started to build a builtin preprocessor
 *
 * 30-Apr-1998 BS	- Redid the stringtable parsing/handling. My previous
 *			  ideas had some serious flaws.
 *
 * 27-Apr-1998 BS	- Removed a lot of dead comments and put it in a doc
 *			  file.
 *
 * 21-Apr-1998 BS	- Added correct behavior for cursors and icons.
 *			- This file is growing too big. It is time to strip
 *			  things and put it in a support file.
 *
 * 19-Apr-1998 BS	- Tagged the stringtable resource so that only one
 *			  resource will be created. This because the table
 *			  has a different layout than other resources. The
 *			  table has to be sorted, and divided into smaller
 *			  resource entries (see comment in source).
 *
 * 17-Apr-1998 BS	- Almost all strings, including identifiers, are parsed
 *			  as string_t which include unicode strings upon
 *			  input.
 *			- Parser now emits a warning when compiling win32
 *			  extensions in win16 mode.
 *
 * 16-Apr-1998 BS	- Raw data elements are now *optionally* seperated
 *			  by commas. Read the comments in file sq2dq.l.
 *			- FIXME: there are instances in the source that rely
 *			  on the fact that int==32bit and pointers are int size.
 *			- Fixed the conflict in menuex by changing a rule
 *			  back into right recursion. See note in source.
 *			- UserType resources cannot have an expression as its
 *			  typeclass. See note in source.
 *
 * 15-Apr-1998 BS	- Changed all right recursion into left recursion to
 *			  get reduction of the parsestack.
 *			  This also helps communication between bison and flex.
 *			  Main advantage is that the Empty rule gets reduced
 *			  first, which is used to allocate/link things.
 *			  It also added a shift/reduce conflict in the menuex
 *			  handling, due to expression/option possibility,
 *			  although not serious.
 *
 * 14-Apr-1998 BS	- Redone almost the entire parser. We're not talking
 *			  about making it more efficient, but readable (for me)
 *			  and slightly easier to expand/change.
 *			  This is done primarily by using more reduce states
 *			  with many (intuitive) types for the various resource
 *			  statements.
 *			- Added expression handling for all resources where a
 *			  number is accepted (not only for win32). Also added
 *			  multiply and division (not MS compatible, but handy).
 *			  Unary minus introduced a shift/reduce conflict, but
 *			  it is not serious.
 *
 * 13-Apr-1998 BS	- Reordered a lot of things
 *			- Made the source more readable
 *			- Added Win32 resource definitions
 *			- Corrected syntax problems with an old yacc (;)
 *			- Added extra comment about grammar
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include <ctype.h>
#include <string.h>
#ifdef HAVE_ALLOCA_H
#include <alloca.h>
#endif

#include "wrc.h"
#include "utils.h"
#include "newstruc.h"
#include "dumpres.h"
#include "preproc.h"
#include "parser.h"
#include "windef.h"
#include "wingdi.h"
#include "winuser.h"

#ifdef __BORLANDC__
#pragma warn -sig
#endif

int indialog = 0;	/* Signal flex that we're parsing a dialog */
int want_rscname = 0;	/* Set when a resource's name is required */
stringtable_t *tagstt;	/* Stringtable tag.
			 * It is set while parsing a stringtable to one of
			 * the stringtables in the sttres list or a new one
			 * if the language was not parsed before.
			 */
stringtable_t *sttres;	/* Stringtable resources. This holds the list of
			 * stringtables with different lanuages
			 */
/* Set to the current options of the currently scanning stringtable */
static int *tagstt_memopt;
static characts_t *tagstt_characts;
static version_t *tagstt_version;

/* Prototypes of here defined functions */
void split_cursors(raw_data_t *rd, cursor_group_t *curg, int *ncur);
void split_icons(raw_data_t *rd, icon_group_t *icog, int *nico);
int alloc_cursor_id(language_t *);
int alloc_icon_id(language_t *);
void ins_stt_entry(stt_entry_t *ste);
int check_stt_entry(stringtable_t *tabs, stt_entry_t *ste);
event_t *get_event_head(event_t *p);
control_t *get_control_head(control_t *p);
ver_value_t *get_ver_value_head(ver_value_t *p);
ver_block_t *get_ver_block_head(ver_block_t *p);
resource_t *get_resource_head(resource_t *p);
menuex_item_t *get_itemex_head(menuex_item_t *p);
menu_item_t *get_item_head(menu_item_t *p);
raw_data_t *merge_raw_data_str(raw_data_t *r1, string_t *str);
raw_data_t *merge_raw_data_int(raw_data_t *r1, int i);
raw_data_t *merge_raw_data_long(raw_data_t *r1, int i);
raw_data_t *merge_raw_data(raw_data_t *r1, raw_data_t *r2);
raw_data_t *str2raw_data(string_t *str);
raw_data_t *int2raw_data(int i);
raw_data_t *long2raw_data(int i);
raw_data_t *load_file(string_t *name);
itemex_opt_t *new_itemex_opt(int id, int type, int state, int helpid);
event_t *add_string_event(string_t *key, int id, int flags, event_t *prev);
event_t *add_event(int key, int id, int flags, event_t *prev);
dialogex_t *dialogex_version(version_t *v, dialogex_t *dlg);
dialogex_t *dialogex_characteristics(characts_t *c, dialogex_t *dlg);
dialogex_t *dialogex_language(language_t *l, dialogex_t *dlg);
dialogex_t *dialogex_menu(name_id_t *m, dialogex_t *dlg);
dialogex_t *dialogex_class(name_id_t *n, dialogex_t *dlg);
dialogex_t *dialogex_font(font_id_t *f, dialogex_t *dlg);
dialogex_t *dialogex_caption(string_t *s, dialogex_t *dlg);
dialogex_t *dialogex_exstyle(style_t *st, dialogex_t *dlg);
dialogex_t *dialogex_style(style_t *st, dialogex_t *dlg);
name_id_t *convert_ctlclass(name_id_t *cls);
control_t *ins_ctrl(int type, int special_style, control_t *ctrl, control_t *prev);
dialog_t *dialog_version(version_t *v, dialog_t *dlg);
dialog_t *dialog_characteristics(characts_t *c, dialog_t *dlg);
dialog_t *dialog_language(language_t *l, dialog_t *dlg);
dialog_t *dialog_menu(name_id_t *m, dialog_t *dlg);
dialog_t *dialog_class(name_id_t *n, dialog_t *dlg);
dialog_t *dialog_font(font_id_t *f, dialog_t *dlg);
dialog_t *dialog_caption(string_t *s, dialog_t *dlg);
dialog_t *dialog_exstyle(style_t * st, dialog_t *dlg);
dialog_t *dialog_style(style_t * st, dialog_t *dlg);
resource_t *build_stt_resources(stringtable_t *stthead);
stringtable_t *find_stringtable(lvc_t *lvc);
toolbar_item_t *ins_tlbr_button(toolbar_item_t *prev, toolbar_item_t *idrec);
toolbar_item_t *get_tlbr_buttons_head(toolbar_item_t *p, int *nitems);

%}
%union{
	string_t	*str;
	int		num;
	int		*iptr;
	resource_t	*res;
	accelerator_t	*acc;
	bitmap_t	*bmp;
	cursor_t	*cur;
	cursor_group_t	*curg;
	dialog_t	*dlg;
	dialogex_t	*dlgex;
	font_t		*fnt;
	icon_t		*ico;
	icon_group_t	*icog;
	menu_t		*men;
	menuex_t	*menex;
	rcdata_t	*rdt;
	stringtable_t	*stt;
	stt_entry_t	*stte;
	user_t		*usr;
	messagetable_t	*msg;
	versioninfo_t	*veri;
	control_t	*ctl;
	name_id_t	*nid;
	font_id_t	*fntid;
	language_t	*lan;
	version_t	*ver;
	characts_t	*chars;
	event_t		*event;
	menu_item_t	*menitm;
	menuex_item_t	*menexitm;
	itemex_opt_t	*exopt;
	raw_data_t	*raw;
	lvc_t		*lvc;
	ver_value_t	*val;
	ver_block_t	*blk;
	ver_words_t	*verw;
	toolbar_t	*tlbar;
	toolbar_item_t	*tlbarItems;
	dlginit_t       *dginit;
	style_pair_t	*styles;
	style_t		*style;
}

%token tIF tIFDEF tIFNDEF tELSE tELIF tENDIF tDEFINED tNL
%token tTYPEDEF tEXTERN
%token <num> NUMBER LNUMBER
%token <str> tSTRING IDENT FILENAME
%token <raw> RAWDATA
%token ACCELERATORS tBITMAP CURSOR DIALOG DIALOGEX MENU MENUEX MESSAGETABLE
%token RCDATA VERSIONINFO STRINGTABLE FONT ICON
%token AUTO3STATE AUTOCHECKBOX AUTORADIOBUTTON CHECKBOX DEFPUSHBUTTON
%token PUSHBUTTON RADIOBUTTON STATE3 /* PUSHBOX */
%token GROUPBOX COMBOBOX LISTBOX SCROLLBAR
%token CONTROL EDITTEXT
%token RTEXT CTEXT LTEXT
%token BLOCK VALUE
%token SHIFT ALT ASCII VIRTKEY GRAYED CHECKED INACTIVE NOINVERT
%token tPURE IMPURE DISCARDABLE LOADONCALL PRELOAD tFIXED MOVEABLE
%token CLASS CAPTION CHARACTERISTICS EXSTYLE STYLE VERSION LANGUAGE
%token FILEVERSION PRODUCTVERSION FILEFLAGSMASK FILEOS FILETYPE FILEFLAGS FILESUBTYPE
%token MENUBARBREAK MENUBREAK MENUITEM POPUP SEPARATOR
%token HELP
%token tSTRING IDENT RAWDATA
%token TOOLBAR BUTTON
%token tBEGIN tEND
%token DLGINIT
%left LOGOR
%left LOGAND
%left '|'
%left '^'
%left '&'
%left EQ NE
%left '<' LTE '>' GTE
%left '+' '-'
%left '*' '/'
%right '~' '!' NOT

%type <res> 	resource_file resource resources resource_definition
%type <stt>	stringtable strings
%type <fnt>	font
%type <icog>	icon
%type <acc> 	accelerators
%type <event> 	events
%type <bmp> 	bitmap
%type <curg> 	cursor
%type <dlg> 	dialog dlg_attributes
%type <ctl> 	ctrls gen_ctrl lab_ctrl ctrl_desc iconinfo
%type <iptr>	helpid
%type <dlgex> 	dialogex dlgex_attribs
%type <ctl>	exctrls gen_exctrl lab_exctrl exctrl_desc
%type <rdt> 	rcdata
%type <raw>	raw_data raw_elements opt_data
%type <veri> 	versioninfo fix_version
%type <verw>	ver_words
%type <blk>	ver_blocks ver_block
%type <val>	ver_values ver_value
%type <men> 	menu
%type <menitm>	item_definitions menu_body
%type <menex>	menuex
%type <menexitm> itemex_definitions menuex_body
%type <exopt>	itemex_p_options itemex_options
%type <msg> 	messagetable
%type <usr> 	userres
%type <num> 	item_options
%type <nid> 	nameid nameid_s ctlclass usertype
%type <num> 	acc_opt
%type <iptr>	loadmemopts lamo lama
%type <fntid>	opt_font opt_exfont opt_expr
%type <lvc>	opt_lvc
%type <lan>	opt_language
%type <chars>	opt_characts
%type <ver>	opt_version
%type <num>	expr xpr
%type <iptr>	e_expr
%type <iptr>	pp_expr pp_constant
%type <tlbar>	toolbar
%type <tlbarItems>	toolbar_items
%type <dginit>  dlginit
%type <styles>  optional_style_pair 
%type <num>	any_num
%type <style>   optional_style
%type <style>   style

%%

resource_file
	: resources {
		resource_t *rsc;
		/* First add stringtables to the resource-list */
		rsc = build_stt_resources(sttres);
		/* 'build_stt_resources' returns a head and $1 is a tail */
		if($1)
		{
			$1->next = rsc;
			if(rsc)
				rsc->prev = $1;
		}
		else
			$1 = rsc;
		/* Final statement before were done */
		resource_top = get_resource_head($1);
		}
	;

/* Resources are put into a linked list */
resources
	: /* Empty */		{ $$ = NULL; want_rscname = 1; }
	| resources resource	{
		if($2)
		{
			resource_t *tail = $2;
			resource_t *head = $2;
			while(tail->next)
				tail = tail->next;
			while(head->prev)
				head = head->prev;
			head->prev = $1;
			if($1)
				$1->next = head;
			$$ = tail;
		}
		else if($1)
		{
			resource_t *tail = $1;
			while(tail->next)
				tail = tail->next;
			$$ = tail;
		}
		else
			$$ = NULL;
		want_rscname = 1;
		}
	| resources preprocessor		{ $$ = $1; want_rscname = 1; }
	| resources cjunk			{ $$ = $1; want_rscname = 1; }
	;

/* The buildin preprocessor */
preprocessor
	: tIF pp_expr tNL	{ pop_start(); push_if($2 ? *($2) : 0, 0, 0); if($2) free($2);}
	| tIFDEF IDENT tNL	{ pop_start(); push_if(pp_lookup($2->str.cstr) != NULL, 0, 0); }
	| tIFNDEF IDENT tNL	{ pop_start(); push_if(pp_lookup($2->str.cstr) == NULL, 0, 0); }
	| tELIF pp_expr tNL	{ pop_start(); push_if($2 ? *($2) : 0, pop_if(), 0); if($2) free($2); }
	| tELSE tNL		{ pop_start(); push_if(1, pop_if(), 0); }
	| tENDIF tNL		{ pop_if(); }
	;

pp_expr	: pp_constant			{ $$ = $1; }
	| pp_expr LOGOR pp_expr		{ $$ = new_int($1 && $3 ? (*$1 || *$3) : 0); if($1) free($1); if($3) free($3); }
	| pp_expr LOGAND pp_expr	{ $$ = new_int($1 && $3 ? (*$1 && *$3) : 0); if($1) free($1); if($3) free($3); }
	| pp_expr '+' pp_expr		{ $$ = new_int($1 && $3 ? (*$1  + *$3) : 0); if($1) free($1); if($3) free($3); }
	| pp_expr '-' pp_expr		{ $$ = new_int($1 && $3 ? (*$1  - *$3) : 0); if($1) free($1); if($3) free($3); }
	| pp_expr '^' pp_expr		{ $$ = new_int($1 && $3 ? (*$1  ^ *$3) : 0); if($1) free($1); if($3) free($3); }
	| pp_expr EQ pp_expr		{ $$ = new_int($1 && $3 ? (*$1 == *$3) : 0); if($1) free($1); if($3) free($3); }
	| pp_expr NE pp_expr		{ $$ = new_int($1 && $3 ? (*$1 != *$3) : 0); if($1) free($1); if($3) free($3); }
	| pp_expr '<' pp_expr		{ $$ = new_int($1 && $3 ? (*$1  < *$3) : 0); if($1) free($1); if($3) free($3); }
	| pp_expr '>' pp_expr		{ $$ = new_int($1 && $3 ? (*$1  > *$3) : 0); if($1) free($1); if($3) free($3); }
	| pp_expr LTE pp_expr		{ $$ = new_int($1 && $3 ? (*$1 <= *$3) : 0); if($1) free($1); if($3) free($3); }
	| pp_expr GTE pp_expr		{ $$ = new_int($1 && $3 ? (*$1 >= *$3) : 0); if($1) free($1); if($3) free($3); }
	| '~' pp_expr			{ $$ = $2; if($2) *$2 = ~(*$2); }
	| '+' pp_expr			{ $$ = $2; }
	| '-' pp_expr			{ $$ = $2; if($2) *$2 = -(*$2); }
	| '!' pp_expr			{ $$ = $2; if($2) *$2 = !(*$2); }
	| '(' pp_expr ')'		{ $$ = $2; }
	;

pp_constant
	: any_num			{ $$ = new_int($1); }
	| IDENT				{ $$ = NULL; }
	| tDEFINED IDENT		{ $$ = new_int(pp_lookup($2->str.cstr) != NULL); }
	| tDEFINED '(' IDENT ')'	{ $$ = new_int(pp_lookup($3->str.cstr) != NULL); }
	;

/* C ignore stuff */
cjunk	: tTYPEDEF			{ strip_til_semicolon(); }
	| tEXTERN			{ strip_extern(); }
	| IDENT IDENT			{ strip_til_semicolon(); }
	| IDENT '('			{ strip_til_parenthesis(); }
	| IDENT '*'			{ strip_til_semicolon(); }
	;

/* Parse top level resource definitions etc. */
resource
	: nameid resource_definition {
		$$ = $2;
		if($$)
		{
			$$->name = $1;
			if($1->type == name_ord)
			{
				chat("Got %s (%d)",get_typename($2),$1->name.i_name);
			}
			else if($1->type == name_str)
			{
				chat("Got %s (%s)",get_typename($2),$1->name.s_name->str.cstr);
			}
		}
		}
	| stringtable {
		/* Don't do anything, stringtables are converted to
		 * resource_t structures when we are finished parsing and
		 * the final rule of the parser is reduced (see above)
		 */
		$$ = NULL;
		chat("Got STRINGTABLE");
		}
	| opt_language {
		if(!win32)
			yywarning("LANGUAGE not supported in 16-bit mode");
		if(currentlanguage)
			free(currentlanguage);
		currentlanguage = $1;
		$$ = NULL;
		}
	;

/*
 * Get a valid name/id
 */
nameid	: expr	{
		if($1 > 65535 || $1 < -32768)
			yyerror("Resource's ID out of range (%d)", $1);
		$$ = new_name_id();
		$$->type = name_ord;
		$$->name.i_name = $1;
		want_rscname = 0;
		}
	| IDENT	{
		$$ = new_name_id();
		$$->type = name_str;
		$$->name.s_name = $1;
		want_rscname = 0;
		}
	;

/*
 * Extra string recognition for CLASS statement in dialogs
 */
nameid_s: nameid	{ $$ = $1; }
	| tSTRING	{
		$$ = new_name_id();
		$$->type = name_str;
		$$->name.s_name = $1;
		want_rscname = 0;
		}
	;

/* get the value for a single resource*/
resource_definition
	: accelerators	{ $$ = new_resource(res_acc, $1, $1->memopt, $1->lvc.language); }
	| bitmap	{ $$ = new_resource(res_bmp, $1, $1->memopt, dup_language(currentlanguage)); }
	| cursor {
		resource_t *rsc;
		cursor_t *cur;
		$$ = rsc = new_resource(res_curg, $1, $1->memopt, dup_language(currentlanguage));
		for(cur = $1->cursorlist; cur; cur = cur->next)
		{
			rsc->prev = new_resource(res_cur, cur, $1->memopt, dup_language(currentlanguage));
			rsc->prev->next = rsc;
			rsc = rsc->prev;
			rsc->name = new_name_id();
			rsc->name->type = name_ord;
			rsc->name->name.i_name = cur->id;
		}
		}
	| dialog	{ $$ = new_resource(res_dlg, $1, $1->memopt, $1->lvc.language); }
	| dialogex {
		if(win32)
			$$ = new_resource(res_dlgex, $1, $1->memopt, $1->lvc.language);
		else
			$$ = NULL;
		}
	| dlginit	{ $$ = new_resource(res_dlginit, $1, $1->memopt, $1->lvc.language); }
	| font		{ $$ = new_resource(res_fnt, $1, $1->memopt, dup_language(currentlanguage)); }
	| icon {
		resource_t *rsc;
		icon_t *ico;
		$$ = rsc = new_resource(res_icog, $1, $1->memopt, dup_language(currentlanguage));
		for(ico = $1->iconlist; ico; ico = ico->next)
		{
			rsc->prev = new_resource(res_ico, ico, $1->memopt, dup_language(currentlanguage));
			rsc->prev->next = rsc;
			rsc = rsc->prev;
			rsc->name = new_name_id();
			rsc->name->type = name_ord;
			rsc->name->name.i_name = ico->id;
		}
		}
	| menu		{ $$ = new_resource(res_men, $1, $1->memopt, $1->lvc.language); }
	| menuex {
		if(win32)
			$$ = new_resource(res_menex, $1, $1->memopt, $1->lvc.language);
		else
			$$ = NULL;
		}
	| messagetable	{ $$ = new_resource(res_msg, $1, WRC_MO_MOVEABLE | WRC_MO_DISCARDABLE, dup_language(currentlanguage)); }
	| rcdata	{ $$ = new_resource(res_rdt, $1, $1->memopt, $1->lvc.language); }
	| toolbar	{ $$ = new_resource(res_toolbar, $1, $1->memopt, $1->lvc.language); }
	| userres	{ $$ = new_resource(res_usr, $1, $1->memopt, dup_language(currentlanguage)); }
	| versioninfo	{ $$ = new_resource(res_ver, $1, WRC_MO_MOVEABLE | WRC_MO_DISCARDABLE, dup_language(currentlanguage)); }
	;

/* ------------------------------ Bitmap ------------------------------ */
bitmap	: tBITMAP loadmemopts FILENAME	{ $$ = new_bitmap(load_file($3), $2); }
	| tBITMAP loadmemopts raw_data	{ $$ = new_bitmap($3, $2); }
	;

/* ------------------------------ Cursor ------------------------------ */
cursor	: CURSOR loadmemopts FILENAME	{ $$ = new_cursor_group(load_file($3), $2); }
	| CURSOR loadmemopts raw_data	{ $$ = new_cursor_group($3, $2); }
	;

/* ------------------------------ Font ------------------------------ */
/* FIXME: Should we allow raw_data here? */
font	: FONT loadmemopts FILENAME	{ $$ = new_font(load_file($3), $2); }
	;

/* ------------------------------ Icon ------------------------------ */
icon	: ICON loadmemopts FILENAME	{ $$ = new_icon_group(load_file($3), $2); }
	| ICON loadmemopts raw_data	{ $$ = new_icon_group($3, $2); }
	;

/* ------------------------------ MessageTable ------------------------------ */
/* It might be interesting to implement the MS Message compiler here as well
 * to get everything in one source. Might be a future project.
 */
messagetable
	: MESSAGETABLE FILENAME	{
		if(!win32)
			yywarning("MESSAGETABLE not supported in 16-bit mode");
		$$ = new_messagetable(load_file($2));
		}
	;

/* ------------------------------ RCData ------------------------------ */
rcdata	: RCDATA loadmemopts opt_lvc raw_data {
		$$ = new_rcdata($4, $2);
		if($3)
		{
			$$->lvc = *($3);
			free($3);
		}
		if(!$$->lvc.language)
			$$->lvc.language = dup_language(currentlanguage);
		}
	;

/* ------------------------------ DLGINIT ------------------------------ */
dlginit	: DLGINIT loadmemopts opt_lvc raw_data {
		$$ = new_dlginit($4, $2);
		if($3)
		{
			$$->lvc = *($3);
			free($3);
		}
		if(!$$->lvc.language)
			$$->lvc.language = dup_language(currentlanguage);
		}
	;	  

/* ------------------------------ UserType ------------------------------ */
userres	: usertype loadmemopts FILENAME	{ $$ = new_user($1, load_file($3), $2); }
	| usertype loadmemopts raw_data	{ $$ = new_user($1, $3, $2); }
	;

/* NOTE: This here is an exception where I do not allow an expression.
 * Reason for this is that it is not possible to set the 'yywf' condition
 * for flex if loadmemopts is empty. Reading an expression requires a
 * lookahead to determine its end. In this case here, that would mean that
 * the filename has been read as IDENT or tSTRING, which is incorrect.
 * Note also that IDENT cannot be used as a file-name because it is lacking
 * the '.'.
 */

/* I also allow string identifiers as classtypes. Not MS implemented, but
 * seems to be reasonable to implement.
 */
/* Allowing anything else than NUMBER makes it very hard to get rid of
 * prototypes. So, I remove IDENT to be able to get prototypes out of the
 * world.
 */
usertype: NUMBER {
		$$ = new_name_id();
		$$->type = name_ord;
		$$->name.i_name = $1;
		set_yywf();
		}
/*	| IDENT {
		$$ = new_name_id();
		$$->type = name_str;
		$$->name.s_name = $1;
		set_yywf();
		}
*/	| tSTRING {
		$$ = new_name_id();
		$$->type = name_str;
		$$->name.s_name = $1;
		set_yywf();
		}
	;

/* ------------------------------ Accelerator ------------------------------ */
accelerators
	: ACCELERATORS loadmemopts opt_lvc tBEGIN events tEND {
		$$ = new_accelerator();
		if($2)
		{
			$$->memopt = *($2);
			free($2);
		}
		else
		{
			$$->memopt = WRC_MO_MOVEABLE | WRC_MO_PURE;
		}
		if(!$5)
			yyerror("Accelerator table must have at least one entry");
		$$->events = get_event_head($5);
		if($3)
		{
			$$->lvc = *($3);
			free($3);
		}
		if(!$$->lvc.language)
			$$->lvc.language = dup_language(currentlanguage);
		}
	;

events	: /* Empty */ 				{ $$=NULL; }
	| events tSTRING ',' expr acc_opt	{ $$=add_string_event($2, $4, $5, $1); }
	| events expr ',' expr acc_opt		{ $$=add_event($2, $4, $5, $1); }
	;

acc_opt	: /* Empty */		{ $$=0; }
	| acc_opt ',' NOINVERT 	{ $$=$1 | WRC_AF_NOINVERT; }
	| acc_opt ',' SHIFT	{ $$=$1 | WRC_AF_SHIFT; }
	| acc_opt ',' CONTROL	{ $$=$1 | WRC_AF_CONTROL; }
	| acc_opt ',' ALT	{ $$=$1 | WRC_AF_ALT; }
	| acc_opt ',' ASCII	{ $$=$1 | WRC_AF_ASCII; }
	| acc_opt ',' VIRTKEY	{ $$=$1 | WRC_AF_VIRTKEY; }
	;

/* ------------------------------ Dialog ------------------------------ */
/* FIXME: Support EXSTYLE in the dialog line itself */
dialog	: DIALOG loadmemopts expr ',' expr ',' expr ',' expr dlg_attributes
	  tBEGIN  ctrls tEND {
		if($2)
		{
			$10->memopt = *($2);
			free($2);
		}
		else
			$10->memopt = WRC_MO_MOVEABLE | WRC_MO_PURE | WRC_MO_DISCARDABLE;
		$10->x = $3;
		$10->y = $5;
		$10->width = $7;
		$10->height = $9;
		$10->controls = get_control_head($12);
		$$ = $10;
		if(!$$->gotstyle)
		{
			$$->style->or_mask = WS_POPUP;
			$$->gotstyle = TRUE;
		}
		if($$->title)
			$$->style->or_mask |= WS_CAPTION;
		if($$->font)
			$$->style->or_mask |= DS_SETFONT;

		$$->style->or_mask &= ~($$->style->and_mask);
		$$->style->and_mask = 0;

		indialog = FALSE;
		if(!$$->lvc.language)
			$$->lvc.language = dup_language(currentlanguage);
		}
	;

dlg_attributes
	: /* Empty */			{ $$=new_dialog(); }
	| dlg_attributes STYLE style	{ $$=dialog_style($3,$1); }
	| dlg_attributes EXSTYLE style	{ $$=dialog_exstyle($3,$1); }
	| dlg_attributes CAPTION tSTRING { $$=dialog_caption($3,$1); }
	| dlg_attributes opt_font	{ $$=dialog_font($2,$1); }
	| dlg_attributes CLASS nameid_s	{ $$=dialog_class($3,$1); }
	| dlg_attributes MENU nameid	{ $$=dialog_menu($3,$1); }
	| dlg_attributes opt_language	{ $$=dialog_language($2,$1); }
	| dlg_attributes opt_characts	{ $$=dialog_characteristics($2,$1); }
	| dlg_attributes opt_version	{ $$=dialog_version($2,$1); }
	;

ctrls	: /* Empty */				{ $$ = NULL; }
	| ctrls CONTROL		gen_ctrl	{ $$=ins_ctrl(-1, 0, $3, $1); }
	| ctrls EDITTEXT	ctrl_desc	{ $$=ins_ctrl(CT_EDIT, 0, $3, $1); }
	| ctrls LISTBOX		ctrl_desc	{ $$=ins_ctrl(CT_LISTBOX, 0, $3, $1); }
	| ctrls COMBOBOX	ctrl_desc	{ $$=ins_ctrl(CT_COMBOBOX, 0, $3, $1); }
	| ctrls SCROLLBAR	ctrl_desc	{ $$=ins_ctrl(CT_SCROLLBAR, 0, $3, $1); }
	| ctrls CHECKBOX	lab_ctrl	{ $$=ins_ctrl(CT_BUTTON, BS_CHECKBOX, $3, $1); }
	| ctrls DEFPUSHBUTTON	lab_ctrl	{ $$=ins_ctrl(CT_BUTTON, BS_DEFPUSHBUTTON, $3, $1); }
	| ctrls GROUPBOX	lab_ctrl	{ $$=ins_ctrl(CT_BUTTON, BS_GROUPBOX, $3, $1);}
	| ctrls PUSHBUTTON	lab_ctrl	{ $$=ins_ctrl(CT_BUTTON, BS_PUSHBUTTON, $3, $1); }
/*	| ctrls PUSHBOX		lab_ctrl	{ $$=ins_ctrl(CT_BUTTON, BS_PUSHBOX, $3, $1); } */
	| ctrls RADIOBUTTON	lab_ctrl	{ $$=ins_ctrl(CT_BUTTON, BS_RADIOBUTTON, $3, $1); }
	| ctrls AUTO3STATE	lab_ctrl	{ $$=ins_ctrl(CT_BUTTON, BS_AUTO3STATE, $3, $1); }
	| ctrls STATE3		lab_ctrl	{ $$=ins_ctrl(CT_BUTTON, BS_3STATE, $3, $1); }
	| ctrls AUTOCHECKBOX	lab_ctrl	{ $$=ins_ctrl(CT_BUTTON, BS_AUTOCHECKBOX, $3, $1); }
	| ctrls AUTORADIOBUTTON lab_ctrl	{ $$=ins_ctrl(CT_BUTTON, BS_AUTORADIOBUTTON, $3, $1); }
	| ctrls LTEXT		lab_ctrl	{ $$=ins_ctrl(CT_STATIC, SS_LEFT, $3, $1); }
	| ctrls CTEXT		lab_ctrl	{ $$=ins_ctrl(CT_STATIC, SS_CENTER, $3, $1); }
	| ctrls RTEXT		lab_ctrl	{ $$=ins_ctrl(CT_STATIC, SS_RIGHT, $3, $1); }
	/* special treatment for icons, as the extent is optional */
	| ctrls ICON nameid_s opt_comma expr ',' expr ',' expr iconinfo {
		$10->title = $3;
		$10->id = $5;
		$10->x = $7;
		$10->y = $9;
		$$ = ins_ctrl(CT_STATIC, SS_ICON, $10, $1);
		}
	;

lab_ctrl
	: tSTRING opt_comma expr ',' expr ',' expr ',' expr ',' expr optional_style {
		$$=new_control();
		$$->title = new_name_id();
		$$->title->type = name_str;
		$$->title->name.s_name = $1;
		$$->id = $3;
		$$->x = $5;
		$$->y = $7;
		$$->width = $9;
		$$->height = $11;
		if($12)
		{
			$$->style = $12;
			$$->gotstyle = TRUE;
		}
		}
	;

ctrl_desc
	: expr ',' expr ',' expr ',' expr ',' expr optional_style {
		$$ = new_control();
		$$->id = $1;
		$$->x = $3;
		$$->y = $5;
		$$->width = $7;
		$$->height = $9;
		if($10)
		{
			$$->style = $10;
			$$->gotstyle = TRUE;
		}
		}
	;

iconinfo: /* Empty */
		{ $$ = new_control(); }

	| ',' expr ',' expr {
		$$ = new_control();
		$$->width = $2;
		$$->height = $4;
		}
	| ',' expr ',' expr ',' style {
		$$ = new_control();
		$$->width = $2;
		$$->height = $4;
		$$->style = $6;
		$$->gotstyle = TRUE;
		}
	| ',' expr ',' expr ',' style ',' style {
		$$ = new_control();
		$$->width = $2;
		$$->height = $4;
		$$->style = $6;
		$$->gotstyle = TRUE;
		$$->exstyle = $8;
		$$->gotexstyle = TRUE;
		}
	;

gen_ctrl: nameid_s opt_comma expr ',' ctlclass ',' style ',' expr ',' expr ',' expr ',' expr ',' style {
		$$=new_control();
		$$->title = $1;
		$$->id = $3;
		$$->ctlclass = convert_ctlclass($5);
		$$->style = $7;
		$$->gotstyle = TRUE;
		$$->x = $9;
		$$->y = $11;
		$$->width = $13;
		$$->height = $15;
		$$->exstyle = $17;
		$$->gotexstyle = TRUE;
		}
	| nameid_s opt_comma expr ',' ctlclass ',' style ',' expr ',' expr ',' expr ',' expr {
		$$=new_control();
		$$->title = $1;
		$$->id = $3;
		$$->ctlclass = convert_ctlclass($5);
		$$->style = $7;
		$$->gotstyle = TRUE;
		$$->x = $9;
		$$->y = $11;
		$$->width = $13;
		$$->height = $15;
		}
	;

opt_font
	: FONT expr ',' tSTRING	{ $$ = new_font_id($2, $4, 0, 0); }
	;

/* ------------------------------ style flags ------------------------------ */
optional_style		/* Abbused once to get optional ExStyle */
	: /* Empty */	{ $$ = NULL; }
	| ',' style	{ $$ = $2; }
	;

optional_style_pair
	: /* Empty */		{ $$ = NULL; }
	| ',' style		{ $$ = new_style_pair($2, 0); }
	| ',' style ',' style 	{ $$ = new_style_pair($2, $4); }
	;

style
	: style '|' style	{ $$ = new_style($1->or_mask | $3->or_mask, $1->and_mask | $3->and_mask); free($1); free($3);}
	| '(' style ')'		{ $$ = $2; }
        | any_num       	{ $$ = new_style($1, 0); }
        | NOT any_num		{ $$ = new_style(0, $2); }
        ;   

ctlclass
	: expr	{
		$$ = new_name_id();
		$$->type = name_ord;
		$$->name.i_name = $1;
		}
	| tSTRING {
		$$ = new_name_id();
		$$->type = name_str;
		$$->name.s_name = $1;
		}
	;

/* ------------------------------ DialogEx ------------------------------ */
dialogex: DIALOGEX loadmemopts expr ',' expr ',' expr ',' expr helpid dlgex_attribs
	  tBEGIN  exctrls tEND {
		if(!win32)
			yywarning("DIALOGEX not supported in 16-bit mode");
		if($2)
		{
			$11->memopt = *($2);
			free($2);
		}
		else
			$11->memopt = WRC_MO_MOVEABLE | WRC_MO_PURE | WRC_MO_DISCARDABLE;
		$11->x = $3;
		$11->y = $5;
		$11->width = $7;
		$11->height = $9;
		if($10)
		{
			$11->helpid = *($10);
			$11->gothelpid = TRUE;
			free($10);
		}
		$11->controls = get_control_head($13);
		$$ = $11;

		assert($$->style != NULL);
		if(!$$->gotstyle)
		{
			$$->style->or_mask = WS_POPUP;
			$$->gotstyle = TRUE;
		}
		if($$->title)
			$$->style->or_mask |= WS_CAPTION;
		if($$->font)
			$$->style->or_mask |= DS_SETFONT;

		$$->style->or_mask &= ~($$->style->and_mask);
		$$->style->and_mask = 0;

		indialog = FALSE;
		if(!$$->lvc.language)
			$$->lvc.language = dup_language(currentlanguage);
		}
	;

dlgex_attribs
	: /* Empty */			{ $$=new_dialogex(); }
	| dlgex_attribs STYLE style	{ $$=dialogex_style($3,$1); }
	| dlgex_attribs EXSTYLE style	{ $$=dialogex_exstyle($3,$1); }
	| dlgex_attribs CAPTION tSTRING { $$=dialogex_caption($3,$1); }
	| dlgex_attribs opt_font	{ $$=dialogex_font($2,$1); }
	| dlgex_attribs opt_exfont	{ $$=dialogex_font($2,$1); }
	| dlgex_attribs CLASS nameid_s	{ $$=dialogex_class($3,$1); }
	| dlgex_attribs MENU nameid	{ $$=dialogex_menu($3,$1); }
	| dlgex_attribs opt_language	{ $$=dialogex_language($2,$1); }
	| dlgex_attribs opt_characts	{ $$=dialogex_characteristics($2,$1); }
	| dlgex_attribs opt_version	{ $$=dialogex_version($2,$1); }
	;

exctrls	: /* Empty */				{ $$ = NULL; }
	| exctrls CONTROL	gen_exctrl	{ $$=ins_ctrl(-1, 0, $3, $1); }
	| exctrls EDITTEXT	exctrl_desc	{ $$=ins_ctrl(CT_EDIT, 0, $3, $1); }
	| exctrls LISTBOX	exctrl_desc	{ $$=ins_ctrl(CT_LISTBOX, 0, $3, $1); }
	| exctrls COMBOBOX	exctrl_desc	{ $$=ins_ctrl(CT_COMBOBOX, 0, $3, $1); }
	| exctrls SCROLLBAR	exctrl_desc	{ $$=ins_ctrl(CT_SCROLLBAR, 0, $3, $1); }
	| exctrls CHECKBOX	lab_exctrl	{ $$=ins_ctrl(CT_BUTTON, BS_CHECKBOX, $3, $1); }
	| exctrls DEFPUSHBUTTON	lab_exctrl	{ $$=ins_ctrl(CT_BUTTON, BS_DEFPUSHBUTTON, $3, $1); }
	| exctrls GROUPBOX	lab_exctrl	{ $$=ins_ctrl(CT_BUTTON, BS_GROUPBOX, $3, $1);}
	| exctrls PUSHBUTTON	lab_exctrl	{ $$=ins_ctrl(CT_BUTTON, BS_PUSHBUTTON, $3, $1); }
/*	| exctrls PUSHBOX	lab_exctrl	{ $$=ins_ctrl(CT_BUTTON, BS_PUSHBOX, $3, $1); } */
	| exctrls RADIOBUTTON	lab_exctrl	{ $$=ins_ctrl(CT_BUTTON, BS_RADIOBUTTON, $3, $1); }
	| exctrls AUTO3STATE	lab_exctrl	{ $$=ins_ctrl(CT_BUTTON, BS_AUTO3STATE, $3, $1); }
	| exctrls STATE3	lab_exctrl	{ $$=ins_ctrl(CT_BUTTON, BS_3STATE, $3, $1); }
	| exctrls AUTOCHECKBOX	lab_exctrl	{ $$=ins_ctrl(CT_BUTTON, BS_AUTOCHECKBOX, $3, $1); }
	| exctrls AUTORADIOBUTTON lab_exctrl	{ $$=ins_ctrl(CT_BUTTON, BS_AUTORADIOBUTTON, $3, $1); }
	| exctrls LTEXT		lab_exctrl	{ $$=ins_ctrl(CT_STATIC, SS_LEFT, $3, $1); }
	| exctrls CTEXT		lab_exctrl	{ $$=ins_ctrl(CT_STATIC, SS_CENTER, $3, $1); }
	| exctrls RTEXT		lab_exctrl	{ $$=ins_ctrl(CT_STATIC, SS_RIGHT, $3, $1); }
	/* special treatment for icons, as the extent is optional */
	| exctrls ICON nameid_s opt_comma expr ',' expr ',' expr iconinfo {
		$10->title = $3;
		$10->id = $5;
		$10->x = $7;
		$10->y = $9;
		$$ = ins_ctrl(CT_STATIC, SS_ICON, $10, $1);
		}
	;

gen_exctrl
	: nameid_s opt_comma expr ',' ctlclass ',' style ',' expr ',' expr ',' expr ','
	  expr ',' style helpid opt_data {
		$$=new_control();
		$$->title = $1;
		$$->id = $3;
		$$->ctlclass = convert_ctlclass($5);
		$$->style = $7;
		$$->gotstyle = TRUE;
		$$->x = $9;
		$$->y = $11;
		$$->width = $13;
		$$->height = $15;
		if($17)
		{
			$$->exstyle = $17;
			$$->gotexstyle = TRUE;
		}
		if($18)
		{
			$$->helpid = *($18);
			$$->gothelpid = TRUE;
			free($18);
		}
		$$->extra = $19;
		}
	| nameid_s opt_comma expr ',' ctlclass ',' style ',' expr ',' expr ',' expr ',' expr opt_data {
		$$=new_control();
		$$->title = $1;
		$$->id = $3;
		$$->style = $7;
		$$->gotstyle = TRUE;
		$$->ctlclass = convert_ctlclass($5);
		$$->x = $9;
		$$->y = $11;
		$$->width = $13;
		$$->height = $15;
		$$->extra = $16;
		}
	;

lab_exctrl
	: tSTRING opt_comma expr ',' expr ',' expr ',' expr ',' expr optional_style_pair opt_data {
		$$=new_control();
		$$->title = new_name_id();
		$$->title->type = name_str;
		$$->title->name.s_name = $1;
		$$->id = $3;
		$$->x = $5;
		$$->y = $7;
		$$->width = $9;
		$$->height = $11;
		if($12)
		{
			$$->style = $12->style;
			$$->gotstyle = TRUE;

			if ($12->exstyle)
			{
			    $$->exstyle = $12->exstyle;
			    $$->gotexstyle = TRUE;
			}
			free($12);
		}

		$$->extra = $13;
		}
	;

exctrl_desc
	: expr ',' expr ',' expr ',' expr ',' expr optional_style_pair opt_data {
		$$ = new_control();
		$$->id = $1;
		$$->x = $3;
		$$->y = $5;
		$$->width = $7;
		$$->height = $9;
		if($10)
		{
			$$->style = $10->style;
			$$->gotstyle = TRUE;

			if ($10->exstyle)
			{
			    $$->exstyle = $10->exstyle;
			    $$->gotexstyle = TRUE;
			}
			free($10);
		}
		$$->extra = $11;
		}
	;

opt_data: /* Empty */	{ $$ = NULL; }
	| raw_data	{ $$ = $1; }
	;

helpid	: /* Empty */	{ $$ = NULL; }
	| ',' expr	{ $$ = new_int($2); }
	;

opt_exfont
	: FONT expr ',' tSTRING ',' expr ',' expr  opt_expr { $$ = new_font_id($2, $4, $6, $8); }
	;

/*
 * FIXME: This odd expression is here to nullify an extra token found 
 * in some appstudio produced resources which appear to do nothing.
 */
opt_expr: /* Empty */	{ $$ = NULL; }
	| ',' expr	{ $$ = NULL; }
	;

/* ------------------------------ Menu ------------------------------ */
menu	: MENU loadmemopts opt_lvc menu_body {
		if(!$4)
			yyerror("Menu must contain items");
		$$ = new_menu();
		if($2)
		{
			$$->memopt = *($2);
			free($2);
		}
		else
			$$->memopt = WRC_MO_MOVEABLE | WRC_MO_PURE | WRC_MO_DISCARDABLE;
		$$->items = get_item_head($4);
		if($3)
		{
			$$->lvc = *($3);
			free($3);
		}
		if(!$$->lvc.language)
			$$->lvc.language = dup_language(currentlanguage);
		}
	;

menu_body
	: tBEGIN item_definitions tEND	{ $$ = $2; }
	;

item_definitions
	: /* Empty */	{$$ = NULL;}
	| item_definitions MENUITEM tSTRING opt_comma expr item_options {
		$$=new_menu_item();
		$$->prev = $1;
		if($1)
			$1->next = $$;
		$$->id =  $5;
		$$->state = $6;
		$$->name = $3;
		}
	| item_definitions MENUITEM SEPARATOR {
		$$=new_menu_item();
		$$->prev = $1;
		if($1)
			$1->next = $$;
		}
	| item_definitions POPUP tSTRING item_options menu_body {
		$$ = new_menu_item();
		$$->prev = $1;
		if($1)
			$1->next = $$;
		$$->popup = get_item_head($5);
		$$->name = $3;
		}
	;

/* NOTE: item_options is right recursive because it would introduce
 * a shift/reduce conflict on ',' in itemex_options due to the
 * empty rule here. The parser is now forced to look beyond the ','
 * before reducing (force shift).
 * Right recursion here is not a problem because we cannot expect
 * more than 7 parserstack places to be occupied while parsing this
 * (who would want to specify a MF_x flag twice?).
 */
item_options
	: /* Empty */			{ $$ = 0; }
	| ',' CHECKED	item_options	{ $$ = $3 | MF_CHECKED; }
	| ',' GRAYED	item_options	{ $$ = $3 | MF_GRAYED; }
	| ',' HELP	item_options	{ $$ = $3 | MF_HELP; }
	| ',' INACTIVE	item_options	{ $$ = $3 | MF_DISABLED; }
	| ',' MENUBARBREAK item_options	{ $$ = $3 | MF_MENUBARBREAK; }
	| ',' MENUBREAK	item_options	{ $$ = $3 | MF_MENUBREAK; }
	;

/* ------------------------------ MenuEx ------------------------------ */
menuex	: MENUEX loadmemopts opt_lvc menuex_body	{
		if(!win32)
			yywarning("MENUEX not supported in 16-bit mode");
		if(!$4)
			yyerror("MenuEx must contain items");
		$$ = new_menuex();
		if($2)
		{
			$$->memopt = *($2);
			free($2);
		}
		else
			$$->memopt = WRC_MO_MOVEABLE | WRC_MO_PURE | WRC_MO_DISCARDABLE;
		$$->items = get_itemex_head($4);
		if($3)
		{
			$$->lvc = *($3);
			free($3);
		}
		if(!$$->lvc.language)
			$$->lvc.language = dup_language(currentlanguage);
		}
	;

menuex_body
	: tBEGIN itemex_definitions tEND { $$ = $2; }
	;

itemex_definitions
	: /* Empty */	{$$ = NULL; }
	| itemex_definitions MENUITEM tSTRING itemex_options {
		$$ = new_menuex_item();
		$$->prev = $1;
		if($1)
			$1->next = $$;
		$$->name = $3;
		$$->id = $4->id;
		$$->type = $4->type;
		$$->state = $4->state;
		$$->helpid = $4->helpid;
		$$->gotid = $4->gotid;
		$$->gottype = $4->gottype;
		$$->gotstate = $4->gotstate;
		$$->gothelpid = $4->gothelpid;
		free($4);
		}
	| itemex_definitions MENUITEM SEPARATOR {
		$$ = new_menuex_item();
		$$->prev = $1;
		if($1)
			$1->next = $$;
		}
	| itemex_definitions POPUP tSTRING itemex_p_options menuex_body {
		$$ = new_menuex_item();
		$$->prev = $1;
		if($1)
			$1->next = $$;
		$$->popup = get_itemex_head($5);
		$$->name = $3;
		$$->id = $4->id;
		$$->type = $4->type;
		$$->state = $4->state;
		$$->helpid = $4->helpid;
		$$->gotid = $4->gotid;
		$$->gottype = $4->gottype;
		$$->gotstate = $4->gotstate;
		$$->gothelpid = $4->gothelpid;
		free($4);
		}
	;

itemex_options
	: /* Empty */			{ $$ = new_itemex_opt(0, 0, 0, 0); }
	| ',' expr {
		$$ = new_itemex_opt($2, 0, 0, 0);
		$$->gotid = TRUE;
		}
	| ',' e_expr ',' e_expr item_options {
		$$ = new_itemex_opt($2 ? *($2) : 0, $4 ? *($4) : 0, $5, 0);
		$$->gotid = TRUE;
		$$->gottype = TRUE;
		$$->gotstate = TRUE;
		if($2) free($2);
		if($4) free($4);
		}
	| ',' e_expr ',' e_expr ',' expr {
		$$ = new_itemex_opt($2 ? *($2) : 0, $4 ? *($4) : 0, $6, 0);
		$$->gotid = TRUE;
		$$->gottype = TRUE;
		$$->gotstate = TRUE;
		if($2) free($2);
		if($4) free($4);
		}
	;

itemex_p_options
	: /* Empty */			{ $$ = new_itemex_opt(0, 0, 0, 0); }
	| ',' expr {
		$$ = new_itemex_opt($2, 0, 0, 0);
		$$->gotid = TRUE;
		}
	| ',' e_expr ',' expr {
		$$ = new_itemex_opt($2 ? *($2) : 0, $4, 0, 0);
		if($2) free($2);
		$$->gotid = TRUE;
		$$->gottype = TRUE;
		}
	| ',' e_expr ',' e_expr ',' expr {
		$$ = new_itemex_opt($2 ? *($2) : 0, $4 ? *($4) : 0, $6, 0);
		if($2) free($2);
		if($4) free($4);
		$$->gotid = TRUE;
		$$->gottype = TRUE;
		$$->gotstate = TRUE;
		}
	| ',' e_expr ',' e_expr ',' e_expr ',' expr {
		$$ = new_itemex_opt($2 ? *($2) : 0, $4 ? *($4) : 0, $6 ? *($6) : 0, $8);
		if($2) free($2);
		if($4) free($4);
		if($6) free($6);
		$$->gotid = TRUE;
		$$->gottype = TRUE;
		$$->gotstate = TRUE;
		$$->gothelpid = TRUE;
		}
	;

/* ------------------------------ StringTable ------------------------------ */
/* Stringtables are parsed differently than other resources because their
 * layout is substantially different from other resources.
 * The table is parsed through a _global_ variable 'tagstt' which holds the
 * current stringtable descriptor (stringtable_t *) and 'sttres' that holds a
 * list of stringtables of different languages.
 */
stringtable
	: stt_head tBEGIN strings tEND {
		if(!$3)
		{
			yyerror("Stringtable must have at least one entry");
		}
		else
		{
			stringtable_t *stt;
			/* Check if we added to a language table or created
			 * a new one.
			 */
			 for(stt = sttres; stt; stt = stt->next)
			 {
				if(stt == tagstt)
					break;
			 }
			 if(!stt)
			 {
				/* It is a new one */
				if(sttres)
				{
					sttres->prev = tagstt;
					tagstt->next = sttres;
					sttres = tagstt;
				}
				else
					sttres = tagstt;
			 }
			 /* Else were done */
		}
		if(tagstt_memopt)
		{
			free(tagstt_memopt);
			tagstt_memopt = NULL;
		}

		$$ = tagstt;
		}
	;

/* This is to get the language of the currently parsed stringtable */
stt_head: STRINGTABLE loadmemopts opt_lvc {
		if((tagstt = find_stringtable($3)) == NULL)
			tagstt = new_stringtable($3);
		tagstt_memopt = $2;
		tagstt_version = $3->version;
		tagstt_characts = $3->characts;
		if($3)
			free($3);
		}
	;

strings	: /* Empty */	{ $$ = NULL; }
	| strings expr opt_comma tSTRING {
		int i;
		assert(tagstt != NULL);
		if($2 > 65535 || $2 < -32768)
			yyerror("Stringtable entry's ID out of range (%d)", $2);
		/* Search for the ID */
		for(i = 0; i < tagstt->nentries; i++)
		{
			if(tagstt->entries[i].id == $2)
				yyerror("Stringtable ID %d already in use", $2);
		}
		/* If we get here, then we have a new unique entry */
		tagstt->nentries++;
		tagstt->entries = xrealloc(tagstt->entries, sizeof(tagstt->entries[0]) * tagstt->nentries);
		tagstt->entries[tagstt->nentries-1].id = $2;
		tagstt->entries[tagstt->nentries-1].str = $4;
		if(tagstt_memopt)
			tagstt->entries[tagstt->nentries-1].memopt = *tagstt_memopt;
		else
			tagstt->entries[tagstt->nentries-1].memopt = WRC_MO_MOVEABLE | WRC_MO_DISCARDABLE | WRC_MO_PURE;
		tagstt->entries[tagstt->nentries-1].version = tagstt_version;
		tagstt->entries[tagstt->nentries-1].characts = tagstt_characts;

		if(!win32 && $4->size > 254)
			yyerror("Stringtable entry more than 254 characters");
		if(win32 && $4->size > 65534) /* Hmm..., does this happen? */
			yyerror("Stringtable entry more than 65534 characters (probably something else that went wrong)");
		$$ = tagstt;
		}
	;

opt_comma	/* There seem to be two ways to specify a stringtable... */
	: /* Empty */
	| ','
	;

/* ------------------------------ VersionInfo ------------------------------ */
versioninfo
	: VERSIONINFO fix_version tBEGIN ver_blocks tEND {
		$$ = $2;
		$2->blocks = get_ver_block_head($4);
		}
	;

fix_version
	: /* Empty */			{ $$ = new_versioninfo(); }
	| fix_version FILEVERSION expr ',' expr ',' expr ',' expr {
		if($1->gotit.fv)
			yyerror("FILEVERSION already defined");
		$$ = $1;
		$$->filever_maj1 = $3;
		$$->filever_maj2 = $5;
		$$->filever_min1 = $7;
		$$->filever_min2 = $9;
		$$->gotit.fv = 1;
		}
	| fix_version PRODUCTVERSION expr ',' expr ',' expr ',' expr {
		if($1->gotit.pv)
			yyerror("PRODUCTVERSION already defined");
		$$ = $1;
		$$->prodver_maj1 = $3;
		$$->prodver_maj2 = $5;
		$$->prodver_min1 = $7;
		$$->prodver_min2 = $9;
		$$->gotit.pv = 1;
		}
	| fix_version FILEFLAGS expr {
		if($1->gotit.ff)
			yyerror("FILEFLAGS already defined");
		$$ = $1;
		$$->fileflags = $3;
		$$->gotit.ff = 1;
		}
	| fix_version FILEFLAGSMASK expr {
		if($1->gotit.ffm)
			yyerror("FILEFLAGSMASK already defined");
		$$ = $1;
		$$->fileflagsmask = $3;
		$$->gotit.ffm = 1;
		}
	| fix_version FILEOS expr {
		if($1->gotit.fo)
			yyerror("FILEOS already defined");
		$$ = $1;
		$$->fileos = $3;
		$$->gotit.fo = 1;
		}
	| fix_version FILETYPE expr {
		if($1->gotit.ft)
			yyerror("FILETYPE already defined");
		$$ = $1;
		$$->filetype = $3;
		$$->gotit.ft = 1;
		}
	| fix_version FILESUBTYPE expr {
		if($1->gotit.fst)
			yyerror("FILESUBTYPE already defined");
		$$ = $1;
		$$->filesubtype = $3;
		$$->gotit.fst = 1;
		}
	;

ver_blocks
	: /* Empty */			{ $$ = NULL; }
	| ver_blocks ver_block {
		$$ = $2;
		$$->prev = $1;
		if($1)
			$1->next = $$;
		}
	;

ver_block
	: BLOCK tSTRING tBEGIN ver_values tEND {
		$$ = new_ver_block();
		$$->name = $2;
		$$->values = get_ver_value_head($4);
		}
	;

ver_values
	: /* Empty */			{ $$ = NULL; }
	| ver_values ver_value {
		$$ = $2;
		$$->prev = $1;
		if($1)
			$1->next = $$;
		}
	;

ver_value
	: ver_block {
		$$ = new_ver_value();
		$$->type = val_block;
		$$->value.block = $1;
		}
	| VALUE tSTRING ',' tSTRING {
		$$ = new_ver_value();
		$$->type = val_str;
		$$->key = $2;
		$$->value.str = $4;
		}
	| VALUE tSTRING ',' ver_words {
		$$ = new_ver_value();
		$$->type = val_words;
		$$->key = $2;
		$$->value.words = $4;
		}
	;

ver_words
	: expr			{ $$ = new_ver_words($1); }
	| ver_words ',' expr	{ $$ = add_ver_words($1, $3); }
	;

/* ------------------------------ Toolbar ------------------------------ */
toolbar: TOOLBAR loadmemopts expr ',' expr opt_lvc tBEGIN toolbar_items tEND {
		int nitems;
		toolbar_item_t *items = get_tlbr_buttons_head($8, &nitems);
		$$ = new_toolbar($3, $5, items, nitems);
		if($2)
		{
			$$->memopt = *($2);
			free($2); 
		}
		else
		{
			$$->memopt = WRC_MO_MOVEABLE | WRC_MO_PURE;
		}
		if($6)
		{
			$$->lvc = *($6);
			free($6);
		}
		if(!$$->lvc.language)
		{
			$$->lvc.language = dup_language(currentlanguage);
		}
		}
	;

toolbar_items
	:  /* Empty */			{ $$ = NULL; }
	| toolbar_items BUTTON expr	{         
		toolbar_item_t *idrec = new_toolbar_item();
		idrec->id = $3;
		$$ = ins_tlbr_button($1, idrec); 
		}
	| toolbar_items SEPARATOR	{         
		toolbar_item_t *idrec = new_toolbar_item();
		idrec->id = 0;
		$$ = ins_tlbr_button($1, idrec); 
	}
	;

/* ------------------------------ Memory options ------------------------------ */
loadmemopts
	: /* Empty */		{ $$ = NULL; }
	| loadmemopts lamo {
		if($1)
		{
			*($1) |= *($2);
			$$ = $1;
			free($2);
		}
		else
			$$ = $2;
		}
	| loadmemopts lama {
		if($1)
		{
			*($1) &= *($2);
			$$ = $1;
			free($2);
		}
		else
		{
			*$2 &= WRC_MO_MOVEABLE | WRC_MO_DISCARDABLE | WRC_MO_PURE;
			$$ = $2;
		}
		}
	;

lamo	: PRELOAD	{ $$ = new_int(WRC_MO_PRELOAD); }
	| MOVEABLE	{ $$ = new_int(WRC_MO_MOVEABLE); }
	| DISCARDABLE	{ $$ = new_int(WRC_MO_DISCARDABLE); }
	| tPURE		{ $$ = new_int(WRC_MO_PURE); }
	;

lama	: LOADONCALL	{ $$ = new_int(~WRC_MO_PRELOAD); }
	| tFIXED	{ $$ = new_int(~WRC_MO_MOVEABLE); }
	| IMPURE	{ $$ = new_int(~WRC_MO_PURE); }
	;

/* ------------------------------ Win32 options ------------------------------ */
opt_lvc	: /* Empty */		{ $$ = new_lvc(); }
	| opt_lvc opt_language {
		if(!win32)
			yywarning("LANGUAGE not supported in 16-bit mode");
		if($1->language)
			yyerror("Language already defined");
		$$ = $1;
		$1->language = $2;
		}
	| opt_lvc opt_characts {
		if(!win32)
			yywarning("CHARACTERISTICS not supported in 16-bit mode");
		if($1->characts)
			yyerror("Characteristics already defined");
		$$ = $1;
		$1->characts = $2;
		}
	| opt_lvc opt_version {
		if(!win32)
			yywarning("VERSION not supported in 16-bit mode");
		if($1->version)
			yyerror("Version already defined");
		$$ = $1;
		$1->version = $2;
		}
	;

opt_language
	: LANGUAGE expr ',' expr	{ $$ = new_language($2, $4); }
	;

opt_characts
	: CHARACTERISTICS expr		{ $$ = new_characts($2); }
	;

opt_version
	: VERSION expr			{ $$ = new_version($2); }
	;

/* ------------------------------ Raw data handking ------------------------------ */
raw_data: tBEGIN raw_elements tEND	{ $$ = $2; }
	;

raw_elements
	: RAWDATA			{ $$ = $1; }
	| NUMBER			{ $$ = int2raw_data($1); }
	| LNUMBER			{ $$ = long2raw_data($1); }
	| tSTRING			{ $$ = str2raw_data($1); }
	| raw_elements opt_comma RAWDATA   { $$ = merge_raw_data($1, $3); free($3->data); free($3); }
	| raw_elements opt_comma NUMBER    { $$ = merge_raw_data_int($1, $3); }
	| raw_elements opt_comma LNUMBER   { $$ = merge_raw_data_long($1, $3); }
	| raw_elements opt_comma tSTRING   { $$ = merge_raw_data_str($1, $3); }
	;

/* ------------------------------ Win32 expressions ------------------------------ */
/* All win16 numbers are also handled here. This is inconsistent with MS'
 * resource compiler, but what the heck, its just handy to have.
 */
e_expr	: /* Empty */	{ $$ = 0; }
	| expr		{ $$ = new_int($1); }
	;
expr	: xpr	{ $$ = ($1); }
	;

xpr	: xpr '+' xpr	{ $$ = ($1) + ($3); }
	| xpr '-' xpr	{ $$ = ($1) - ($3); }
	| xpr '|' xpr	{ $$ = ($1) | ($3); }
	| xpr '&' xpr	{ $$ = ($1) & ($3); }
	| xpr '*' xpr	{ $$ = ($1) * ($3); }
	| xpr '/' xpr	{ $$ = ($1) / ($3); }
	| '~' xpr	{ $$ = ~($2); }
	| '-' xpr	{ $$ = -($2); }		/* FIXME: shift/reduce conflict */
/*	| '+' xpr	{ $$ = $2; } */
	| '(' xpr ')'	{ $$ = $2; }
	| any_num	{ $$ = $1; want_rscname = 0; }
	| NOT any_num	{ $$ = ~($2); }
	;

any_num	: NUMBER	{ $$ = $1; }
	| LNUMBER	{ $$ = $1; }
	;

%%
/* Dialog specific functions */
dialog_t *dialog_style(style_t * st, dialog_t *dlg)
{
	assert(dlg != NULL);
	if(dlg->style == NULL)
	{
		dlg->style = new_style(0,0);
	}

	if(dlg->gotstyle)
	{
		yywarning("Style already defined, or-ing together");
	}
	else
	{
		dlg->style->or_mask = 0;
		dlg->style->and_mask = 0;
	}
	dlg->style->or_mask |= st->or_mask;
	dlg->style->and_mask |= st->and_mask;
	dlg->gotstyle = TRUE;
	free(st);
	return dlg;
}

dialog_t *dialog_exstyle(style_t *st, dialog_t *dlg)
{
	assert(dlg != NULL);
	if(dlg->exstyle == NULL)
	{
		dlg->exstyle = new_style(0,0);
	}

	if(dlg->gotexstyle)
	{
		yywarning("ExStyle already defined, or-ing together");
	}
	else
	{
		dlg->exstyle->or_mask = 0;
		dlg->exstyle->and_mask = 0;
	}
	dlg->exstyle->or_mask |= st->or_mask;
	dlg->exstyle->and_mask |= st->and_mask;
	dlg->gotexstyle = TRUE;
	free(st);
	return dlg;
}

dialog_t *dialog_caption(string_t *s, dialog_t *dlg)
{
	assert(dlg != NULL);
	if(dlg->title)
		yyerror("Caption already defined");
	dlg->title = s;
	return dlg;
}

dialog_t *dialog_font(font_id_t *f, dialog_t *dlg)
{
	assert(dlg != NULL);
	if(dlg->font)
		yyerror("Font already defined");
	dlg->font = f;
	return dlg;
}

dialog_t *dialog_class(name_id_t *n, dialog_t *dlg)
{
	assert(dlg != NULL);
	if(dlg->dlgclass)
		yyerror("Class already defined");
	dlg->dlgclass = n;
	return dlg;
}

dialog_t *dialog_menu(name_id_t *m, dialog_t *dlg)
{
	assert(dlg != NULL);
	if(dlg->menu)
		yyerror("Menu already defined");
	dlg->menu = m;
	return dlg;
}

dialog_t *dialog_language(language_t *l, dialog_t *dlg)
{
	assert(dlg != NULL);
	if(dlg->lvc.language)
		yyerror("Language already defined");
	dlg->lvc.language = l;
	return dlg;
}

dialog_t *dialog_characteristics(characts_t *c, dialog_t *dlg)
{
	assert(dlg != NULL);
	if(dlg->lvc.characts)
		yyerror("Characteristics already defined");
	dlg->lvc.characts = c;
	return dlg;
}

dialog_t *dialog_version(version_t *v, dialog_t *dlg)
{
	assert(dlg != NULL);
	if(dlg->lvc.version)
		yyerror("Version already defined");
	dlg->lvc.version = v;
	return dlg;
}

/* Controls specific functions */
control_t *ins_ctrl(int type, int special_style, control_t *ctrl, control_t *prev)
{
	/* Hm... this seems to be jammed in at all time... */
	int defaultstyle = WS_CHILD | WS_VISIBLE;

	assert(ctrl != NULL);
	ctrl->prev = prev;

	if(prev)
		prev->next = ctrl;

	if(type != -1)
	{
		ctrl->ctlclass = new_name_id();
		ctrl->ctlclass->type = name_ord;
		ctrl->ctlclass->name.i_name = type;
	}

	switch(type)
	{
	case CT_BUTTON:
		if(special_style != BS_GROUPBOX && special_style != BS_RADIOBUTTON)
			defaultstyle |= WS_TABSTOP;
		break;
	case CT_EDIT:
		defaultstyle |= WS_TABSTOP | WS_BORDER;
		break;
	case CT_LISTBOX:
		defaultstyle |= LBS_NOTIFY | WS_BORDER;
		break;
	case CT_COMBOBOX:
		defaultstyle |= CBS_SIMPLE;
		break;
	case CT_STATIC:
		if(special_style == SS_CENTER || special_style == SS_LEFT || special_style == SS_RIGHT)
			defaultstyle |= WS_GROUP;
		break;
	}

	if(!ctrl->gotstyle)	/* Handle default style setting */
	{
		switch(type)
		{
		case CT_EDIT:
			defaultstyle |= ES_LEFT;
			break;
		case CT_LISTBOX:
			defaultstyle |= LBS_NOTIFY;
			break;
		case CT_COMBOBOX:
			defaultstyle |= CBS_SIMPLE | WS_TABSTOP;
			break;
		case CT_SCROLLBAR:
			defaultstyle |= SBS_HORZ;
			break;
		case CT_BUTTON:
			switch(special_style)
			{
			case BS_CHECKBOX:
			case BS_DEFPUSHBUTTON:
			case BS_PUSHBUTTON:
			case BS_GROUPBOX:
/*			case BS_PUSHBOX:	*/
			case BS_AUTORADIOBUTTON:
			case BS_AUTO3STATE:
			case BS_3STATE:
			case BS_AUTOCHECKBOX:
				defaultstyle |= WS_TABSTOP;
				break;
			default:
				yywarning("Unknown default button control-style 0x%08x", special_style);
			case BS_RADIOBUTTON:
				break;
			}
			break;

		case CT_STATIC:
			switch(special_style)
			{
			case SS_LEFT:
			case SS_RIGHT:
			case SS_CENTER:
				defaultstyle |= WS_GROUP;
				break;
			case SS_ICON:	/* Special case */
				break;
			default:
				yywarning("Unknown default static control-style 0x%08x", special_style);
				break;
			}
			break;
		case -1:	/* Generic control */
			goto byebye;

		default:
			yyerror("Internal error (report this): Got weird control type 0x%08x", type);
		}
	}

	/* The SS_ICON flag is always forced in for icon controls */
	if(type == CT_STATIC && special_style == SS_ICON)
		defaultstyle |= SS_ICON;

	if (!ctrl->gotstyle)
		ctrl->style = new_style(0,0);

	/* combine all styles */
	ctrl->style->or_mask = ctrl->style->or_mask | defaultstyle | special_style;
	ctrl->gotstyle = TRUE;
byebye:
	/* combine with NOT mask */
	if (ctrl->gotstyle)
	{
		ctrl->style->or_mask &= ~(ctrl->style->and_mask);
		ctrl->style->and_mask = 0;
	}
	if (ctrl->gotexstyle)
	{
		ctrl->exstyle->or_mask &= ~(ctrl->exstyle->and_mask);
		ctrl->exstyle->and_mask = 0;
	}
	return ctrl;
}

name_id_t *convert_ctlclass(name_id_t *cls)
{
	char *cc;
	int iclass;

	if(cls->type == name_ord)
		return cls;
	assert(cls->type == name_str);
	if(cls->type == str_unicode)
	{
		yyerror("Don't yet support unicode class comparison");
	}
	else
		cc = cls->name.s_name->str.cstr;

	if(!strcasecmp("BUTTON", cc))
		iclass = CT_BUTTON;
	else if(!strcasecmp("COMBOBOX", cc))
		iclass = CT_COMBOBOX;
	else if(!strcasecmp("LISTBOX", cc))
		iclass = CT_LISTBOX;
	else if(!strcasecmp("EDIT", cc))
		iclass = CT_EDIT;
	else if(!strcasecmp("STATIC", cc))
		iclass = CT_STATIC;
	else if(!strcasecmp("SCROLLBAR", cc))
		iclass = CT_SCROLLBAR;
	else
		return cls;	/* No default, return user controlclass */

	free(cls->name.s_name->str.cstr);
	free(cls->name.s_name);
	cls->type = name_ord;
	cls->name.i_name = iclass;
	return cls;
}

/* DialogEx specific functions */
dialogex_t *dialogex_style(style_t * st, dialogex_t *dlg)
{
	assert(dlg != NULL);
	if(dlg->style == NULL)
	{
		dlg->style = new_style(0,0);
	}

	if(dlg->gotstyle)
	{
		yywarning("Style already defined, or-ing together");
	}
	else
	{
		dlg->style->or_mask = 0;
		dlg->style->and_mask = 0;
	}
	dlg->style->or_mask |= st->or_mask;
	dlg->style->and_mask |= st->and_mask;
	dlg->gotstyle = TRUE;
	free(st);
	return dlg;
}

dialogex_t *dialogex_exstyle(style_t * st, dialogex_t *dlg)
{
	assert(dlg != NULL);
	if(dlg->exstyle == NULL)
	{
		dlg->exstyle = new_style(0,0);
	}

	if(dlg->gotexstyle)
	{
		yywarning("ExStyle already defined, or-ing together");
	}
	else
	{
		dlg->exstyle->or_mask = 0;
		dlg->exstyle->and_mask = 0;
	}
	dlg->exstyle->or_mask |= st->or_mask;
	dlg->exstyle->and_mask |= st->and_mask;
	dlg->gotexstyle = TRUE;
	free(st);
	return dlg;
}

dialogex_t *dialogex_caption(string_t *s, dialogex_t *dlg)
{
	assert(dlg != NULL);
	if(dlg->title)
		yyerror("Caption already defined");
	dlg->title = s;
	return dlg;
}

dialogex_t *dialogex_font(font_id_t *f, dialogex_t *dlg)
{
	assert(dlg != NULL);
	if(dlg->font)
		yyerror("Font already defined");
	dlg->font = f;
	return dlg;
}

dialogex_t *dialogex_class(name_id_t *n, dialogex_t *dlg)
{
	assert(dlg != NULL);
	if(dlg->dlgclass)
		yyerror("Class already defined");
	dlg->dlgclass = n;
	return dlg;
}

dialogex_t *dialogex_menu(name_id_t *m, dialogex_t *dlg)
{
	assert(dlg != NULL);
	if(dlg->menu)
		yyerror("Menu already defined");
	dlg->menu = m;
	return dlg;
}

dialogex_t *dialogex_language(language_t *l, dialogex_t *dlg)
{
	assert(dlg != NULL);
	if(dlg->lvc.language)
		yyerror("Language already defined");
	dlg->lvc.language = l;
	return dlg;
}

dialogex_t *dialogex_characteristics(characts_t *c, dialogex_t *dlg)
{
	assert(dlg != NULL);
	if(dlg->lvc.characts)
		yyerror("Characteristics already defined");
	dlg->lvc.characts = c;
	return dlg;
}

dialogex_t *dialogex_version(version_t *v, dialogex_t *dlg)
{
	assert(dlg != NULL);
	if(dlg->lvc.version)
		yyerror("Version already defined");
	dlg->lvc.version = v;
	return dlg;
}

/* Accelerator specific functions */
event_t *add_event(int key, int id, int flags, event_t *prev)
{
	event_t *ev = new_event();

	if((flags & (WRC_AF_VIRTKEY | WRC_AF_ASCII)) == (WRC_AF_VIRTKEY | WRC_AF_ASCII))
		yyerror("Cannot use both ASCII and VIRTKEY");

	ev->key = key;
	ev->id = id;
	ev->flags = flags & ~WRC_AF_ASCII;
	ev->prev = prev;
	if(prev)
		prev->next = ev;
	return ev;
}

event_t *add_string_event(string_t *key, int id, int flags, event_t *prev)
{
	int keycode = 0;
	event_t *ev = new_event();

	if(key->type != str_char)
		yyerror("Key code must be an ascii string");

	if((flags & WRC_AF_VIRTKEY) && (!isupper(key->str.cstr[0]) && !isdigit(key->str.cstr[0])))
		yyerror("VIRTKEY code is not equal to ascii value");

	if(key->str.cstr[0] == '^' && (flags & WRC_AF_CONTROL) != 0)
	{
		yyerror("Cannot use both '^' and CONTROL modifier");
	}
	else if(key->str.cstr[0] == '^')
	{
		keycode = toupper(key->str.cstr[1]) - '@';
		if(keycode >= ' ')
			yyerror("Control-code out of range");
	}
	else
		keycode = key->str.cstr[0];
	ev->key = keycode;
	ev->id = id;
	ev->flags = flags & ~WRC_AF_ASCII;
	ev->prev = prev;
	if(prev)
		prev->next = ev;
	return ev;
}

/* MenuEx specific functions */
itemex_opt_t *new_itemex_opt(int id, int type, int state, int helpid)
{
	itemex_opt_t *opt = (itemex_opt_t *)xmalloc(sizeof(itemex_opt_t));
	opt->id = id;
	opt->type = type;
	opt->state = state;
	opt->helpid = helpid;
	return opt;
}

/* Raw data functions */
raw_data_t *load_file(string_t *name)
{
	FILE *fp;
	raw_data_t *rd;
	if(name->type != str_char)
		yyerror("Filename must be ASCII string");
		
	fp = open_include(name->str.cstr, 1);
	if(!fp)
		yyerror("Cannot open file %s", name->str.cstr);
	rd = new_raw_data();
	fseek(fp, 0, SEEK_END);
	rd->size = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	rd->data = (char *)xmalloc(rd->size);
	fread(rd->data, rd->size, 1, fp);
	fclose(fp);
	HEAPCHECK();
	return rd;
}

raw_data_t *int2raw_data(int i)
{
	raw_data_t *rd;

	if((int)((short)i) != i)
		yywarning("Integer constant out of 16bit range (%d), truncated to %d\n", i, (short)i);

	rd = new_raw_data();
	rd->size = sizeof(short);
	rd->data = (char *)xmalloc(rd->size);
	*(short *)(rd->data) = (short)i;
	return rd;
}

raw_data_t *long2raw_data(int i)
{
	raw_data_t *rd;
	rd = new_raw_data();
	rd->size = sizeof(int);
	rd->data = (char *)xmalloc(rd->size);
	*(int *)(rd->data) = i;
	return rd;
}

raw_data_t *str2raw_data(string_t *str)
{
	raw_data_t *rd;
	rd = new_raw_data();
	rd->size = str->size * (str->type == str_char ? 1 : 2);
	rd->data = (char *)xmalloc(rd->size);
	memcpy(rd->data, str->str.cstr, rd->size);
	return rd;
}

raw_data_t *merge_raw_data(raw_data_t *r1, raw_data_t *r2)
{
	r1->data = xrealloc(r1->data, r1->size + r2->size);
	memcpy(r1->data + r1->size, r2->data, r2->size);
	r1->size += r2->size;
	return r1;
}

raw_data_t *merge_raw_data_int(raw_data_t *r1, int i)
{
	raw_data_t *t = int2raw_data(i);
	merge_raw_data(r1, t);
	free(t->data);
	free(t);
	return r1;
}

raw_data_t *merge_raw_data_long(raw_data_t *r1, int i)
{
	raw_data_t *t = long2raw_data(i);
	merge_raw_data(r1, t);
	free(t->data);
	free(t);
	return r1;
}

raw_data_t *merge_raw_data_str(raw_data_t *r1, string_t *str)
{
	raw_data_t *t = str2raw_data(str);
	merge_raw_data(r1, t);
	free(t->data);
	free(t);
	return r1;
}

/* Function the go back in a list to get the head */
menu_item_t *get_item_head(menu_item_t *p)
{
	if(!p)
		return NULL;
	while(p->prev)
		p = p->prev;
	return p;
}

menuex_item_t *get_itemex_head(menuex_item_t *p)
{
	if(!p)
		return NULL;
	while(p->prev)
		p = p->prev;
	return p;
}

resource_t *get_resource_head(resource_t *p)
{
	if(!p)
		return NULL;
	while(p->prev)
		p = p->prev;
	return p;
}

ver_block_t *get_ver_block_head(ver_block_t *p)
{
	if(!p)
		return NULL;
	while(p->prev)
		p = p->prev;
	return p;
}

ver_value_t *get_ver_value_head(ver_value_t *p)
{
	if(!p)
		return NULL;
	while(p->prev)
		p = p->prev;
	return p;
}

control_t *get_control_head(control_t *p)
{
	if(!p)
		return NULL;
	while(p->prev)
		p = p->prev;
	return p;
}

event_t *get_event_head(event_t *p)
{
	if(!p)
		return NULL;
	while(p->prev)
		p = p->prev;
	return p;
}

/* Find a stringtable with given language */
stringtable_t *find_stringtable(lvc_t *lvc)
{
	stringtable_t *stt;

	assert(lvc != NULL);

	if(!lvc->language)
		lvc->language = dup_language(currentlanguage);

	for(stt = sttres; stt; stt = stt->next)
	{
		if(stt->lvc.language->id == lvc->language->id
		&& stt->lvc.language->id == lvc->language->id)
		{
			/* Found a table with the same language */
			/* The version and characteristics are now handled
			 * in the generation of the individual stringtables.
			 * This enables localized analysis.
			if((stt->lvc.version && lvc->version && *(stt->lvc.version) != *(lvc->version))
			|| (!stt->lvc.version && lvc->version)
			|| (stt->lvc.version && !lvc->version))
				yywarning("Stringtable's versions are not the same, using first definition");

			if((stt->lvc.characts && lvc->characts && *(stt->lvc.characts) != *(lvc->characts))
			|| (!stt->lvc.characts && lvc->characts)
			|| (stt->lvc.characts && !lvc->characts))
				yywarning("Stringtable's characteristics are not the same, using first definition");
			*/
			return stt;
		}
	}
	return NULL;
}

/* qsort sorting function for string table entries */
#define STE(p)	((stt_entry_t *)(p))
int sort_stt_entry(const void *e1, const void *e2)
{
	return STE(e1)->id - STE(e2)->id;
}
#undef STE

resource_t *build_stt_resources(stringtable_t *stthead)
{
	stringtable_t *stt;
	stringtable_t *newstt;
	resource_t *rsc;
	resource_t *rsclist = NULL;
	resource_t *rsctail = NULL;
	int i;
	int j;
	DWORD andsum;
	DWORD orsum;
	characts_t *characts;
	version_t *version;

	if(!stthead)
		return NULL;

	/* For all languages defined */
	for(stt = stthead; stt; stt = stt->next)
	{
		assert(stt->nentries > 0);

		/* Sort the entries */
		if(stt->nentries > 1)
			qsort(stt->entries, stt->nentries, sizeof(stt->entries[0]), sort_stt_entry);

		for(i = 0; i < stt->nentries; )
		{
			newstt = new_stringtable(&stt->lvc);
			newstt->entries = (stt_entry_t *)xmalloc(16 * sizeof(stt_entry_t));
			newstt->nentries = 16;
			newstt->idbase = stt->entries[i].id & ~0xf;
			for(j = 0; j < 16 && i < stt->nentries; j++)
			{
				if(stt->entries[i].id - newstt->idbase == j)
				{
					newstt->entries[j] = stt->entries[i];
					i++;
				}
			}
			andsum = ~0;
			orsum = 0;
			characts = NULL;
			version = NULL;
			/* Check individual memory options and get
			 * the first characteristics/version
			 */
			for(j = 0; j < 16; j++)
			{
				if(!newstt->entries[j].str)
					continue;
				andsum &= newstt->entries[j].memopt;
				orsum |= newstt->entries[j].memopt;
				if(!characts)
					characts = newstt->entries[j].characts;
				if(!version)
					version = newstt->entries[j].version;
			}
			if(andsum != orsum)
			{
				warning("Stringtable's memory options are not equal (idbase: %d)", newstt->idbase);
			}
			/* Check version and characteristics */
			for(j = 0; j < 16; j++)
			{
				if(characts
				&& newstt->entries[j].characts
				&& *newstt->entries[j].characts != *characts)
					warning("Stringtable's characteristics are not the same (idbase: %d)", newstt->idbase);
				if(version
				&& newstt->entries[j].version
				&& *newstt->entries[j].version != *version)
					warning("Stringtable's versions are not the same (idbase: %d)", newstt->idbase);
			}
			rsc = new_resource(res_stt, newstt, newstt->memopt, newstt->lvc.language);
			rsc->name = new_name_id();
			rsc->name->type = name_ord;
			rsc->name->name.i_name = (newstt->idbase >> 4) + 1;
			rsc->memopt = andsum; /* Set to least common denominator */
			newstt->memopt = andsum;
			newstt->lvc.characts = characts;
			newstt->lvc.version = version;
			if(!rsclist)
			{
				rsclist = rsc;
				rsctail = rsc;
			}
			else
			{
				rsctail->next = rsc;
				rsc->prev = rsctail;
				rsctail = rsc;
			}
		}
	}
	return rsclist;
}

/* Cursor and icon splitter functions */
typedef struct {
	language_t	lan;
	int		id;
} id_alloc_t;

static int get_new_id(id_alloc_t **list, int *n, language_t *lan)
{
	int i;
	assert(lan != NULL);
	assert(list != NULL);
	assert(n != NULL);

	if(!*list)
	{
		*list = (id_alloc_t *)xmalloc(sizeof(id_alloc_t));
		*n = 1;
		(*list)[0].lan = *lan;
		(*list)[0].id = 1;
		return 1;
	}

	for(i = 0; i < *n; i++)
	{
		if((*list)[i].lan.id == lan->id && (*list)[i].lan.sub == lan->sub)
			return ++((*list)[i].id);
	}

	*list = (id_alloc_t *)xrealloc(*list, sizeof(id_alloc_t) * (*n+1));
	(*list)[*n].lan = *lan;
	(*list)[*n].id = 1;
	*n += 1;
	return 1;
}

int alloc_icon_id(language_t *lan)
{
	static id_alloc_t *idlist = NULL;
	static int nid = 0;

	return get_new_id(&idlist, &nid, lan);
}

int alloc_cursor_id(language_t *lan)
{
	static id_alloc_t *idlist = NULL;
	static int nid = 0;

	return get_new_id(&idlist, &nid, lan);
}

#define BPTR(base)	((char *)(rd->data + (base)))
#define WPTR(base)	((WORD *)(rd->data + (base)))
#define DPTR(base)	((DWORD *)(rd->data + (base)))
void split_icons(raw_data_t *rd, icon_group_t *icog, int *nico)
{
	int cnt;
	int i;
	icon_dir_entry_t *ide;
	icon_t *ico;
	icon_t *list = NULL;

	/* FIXME: Distinguish between normal and animated icons (RIFF format) */
	if(WPTR(0)[1] != 1)
		yyerror("Icon resource data has invalid type id %d", WPTR(0)[1]);
	cnt = WPTR(0)[2];
	ide = (icon_dir_entry_t *)&(WPTR(0)[3]);
	for(i = 0; i < cnt; i++)
	{
		ico = new_icon();
		ico->id = alloc_icon_id(icog->lvc.language);
		ico->lvc.language = dup_language(icog->lvc.language);
		if(ide[i].offset > rd->size
		|| ide[i].offset + ide[i].ressize > rd->size)
			yyerror("Icon resource data corrupt");
		ico->width = ide[i].width;
		ico->height = ide[i].height;
		ico->nclr = ide[i].nclr;
		ico->planes = ide[i].planes;
		ico->bits = ide[i].bits;
		if(!ico->planes)
		{
			/* Argh! They did not fill out the resdir structure */
			ico->planes = ((BITMAPINFOHEADER *)BPTR(ide[i].offset))->biPlanes;
		}
		if(!ico->bits)
		{
			/* Argh! They did not fill out the resdir structure */
			ico->bits = ((BITMAPINFOHEADER *)BPTR(ide[i].offset))->biBitCount;
		}
		ico->data = new_raw_data();
		copy_raw_data(ico->data, rd, ide[i].offset, ide[i].ressize);
		if(!list)
		{
			list = ico;
		}
		else
		{
			ico->next = list;
			list->prev = ico;
			list = ico;
		}
	}
	icog->iconlist = list;
	*nico = cnt;
}

void split_cursors(raw_data_t *rd, cursor_group_t *curg, int *ncur)
{
	int cnt;
	int i;
	cursor_dir_entry_t *cde;
	cursor_t *cur;
	cursor_t *list = NULL;

	/* FIXME: Distinguish between normal and animated cursors (RIFF format)*/
	if(WPTR(0)[1] != 2)
		yyerror("Cursor resource data has invalid type id %d", WPTR(0)[1]);
	cnt = WPTR(0)[2];
	cde = (cursor_dir_entry_t *)&(WPTR(0)[3]);
	for(i = 0; i < cnt; i++)
	{
		cur = new_cursor();
		cur->id = alloc_cursor_id(curg->lvc.language);
		cur->lvc.language = dup_language(curg->lvc.language);
		if(cde[i].offset > rd->size
		|| cde[i].offset + cde[i].ressize > rd->size)
			yyerror("Cursor resource data corrupt");
		cur->width = cde[i].width;
		cur->height = cde[i].height;
		cur->nclr = cde[i].nclr;
		/* The next two are to support color cursors */
		cur->planes = ((BITMAPINFOHEADER *)BPTR(cde[i].offset))->biPlanes;
		cur->bits = ((BITMAPINFOHEADER *)BPTR(cde[i].offset))->biBitCount;
		if(!win32 && (cur->planes != 1 || cur->bits != 1))
			yywarning("Win16 cursor contains colors");
		cur->xhot = cde[i].xhot;
		cur->yhot = cde[i].yhot;
		cur->data = new_raw_data();
		copy_raw_data(cur->data, rd, cde[i].offset, cde[i].ressize);
		if(!list)
		{
			list = cur;
		}
		else
		{
			cur->next = list;
			list->prev = cur;
			list = cur;
		}
	}
	curg->cursorlist = list;
	*ncur = cnt;
}

#undef	BPTR
#undef	WPTR
#undef	DPTR


toolbar_item_t *ins_tlbr_button(toolbar_item_t *prev, toolbar_item_t *idrec)
{
	idrec->prev = prev;
	if(prev)
		prev->next = idrec;

	return idrec;
}

toolbar_item_t *get_tlbr_buttons_head(toolbar_item_t *p, int *nitems)
{
	if(!p)
	{
		*nitems = 0;
		return NULL;
	} 

	*nitems = 1;

	while(p->prev)
	{
		(*nitems)++;
		p = p->prev;
	}

	return p;
}

