%{
/*
 *
 * Copyright  Martin von Loewis, 1994
 */

#include <stdio.h>
#include <stdlib.h>
#include "parser.h"
#include "windows.h"

int yylex(void);
int yyerror(const char *s);

%}
%union{
	gen_res *res;
	char *str;
	int num;
	struct rc_style *style;
}
%token <num> NUMBER
%token <str> tSTRING SINGLE_QUOTED IDENT
%token ACCELERATORS ALT ASCII tBEGIN tBITMAP CAPTION CHECKBOX CHECKED 
%token CLASS COMBOBOX CONTROL CTEXT CURSOR DEFPUSHBUTTON DIALOG 
%token DISCARDABLE EDITTEXT tEND tFIXED FONT GRAYED GROUPBOX HELP ICON 
%token IDENT INACTIVE LISTBOX LTEXT MENU MENUBARBREAK MENUBREAK MENUITEM 
%token MOVEABLE LOADONCALL NOINVERT NOT NOT_SUPPORTED POPUP PRELOAD 
%token PURE PUSHBUTTON RADIOBUTTON RCDATA RTEXT SCROLLBAR SHIFT SEPARATOR 
%token SINGLE_QUOTED tSTRING STRINGTABLE STYLE VERSIONINFO VIRTKEY
%type <res> resource_file resource resources resource_definition accelerators
%type <res> events bitmap cursor dialog dlg_attributes controls 
%type <res> generic_control labeled_control control_desc font icon 
%type <res> iconinfo menu menu_body item_definitions rcdata raw_data raw_elements 
%type <res> stringtable strings versioninfo
%type <num> acc_options item_options
%type <style> style style_elm optional_style
%%

resource_file: resources {create_output($1);}

/*resources are put into a linked list*/
resources:	{$$=0;}
		|resource resources {$$=add_resource($1,$2);}
		;

/* get the name for a single resource*/
resource:	NUMBER resource_definition
		{$$=$2;$$->n.i_name=$1;$$->n_type=0;
			if(verbose)fprintf(stderr,"Got %s %d\n",get_typename($2),$1);
		}
		| IDENT resource_definition
		{$$=$2;$$->n.s_name=$1;$$->n_type=1;
			if(verbose)fprintf(stderr,"Got %s %s\n",get_typename($2),$1);
		}
                | stringtable
                {$$=$1; /* <-- should be NULL */
			if(verbose)fprintf(stderr,"Got STRINGTABLE\n");
                }
                ;

/* get the value for a single resource*/
resource_definition:	accelerators {$$=$1;}
		| bitmap {$$=$1;}
		| cursor {$$=$1;}
		| dialog {$$=$1;}
		| font {$$=$1;}
		| icon {$$=$1;}
		| menu {$$=$1;}
		| rcdata {$$=$1;}
		| versioninfo {$$=$1;}
                ;

/* have to use tBEGIN because BEGIN is predefined */
accelerators:	ACCELERATORS load_and_memoption tBEGIN  events tEND {$$=$4;$$->type=acc;}
/* the events are collected in a gen_res, as the accelerator resource is just
   an array of events */
events:		{$$=new_res();}
		| tSTRING ',' NUMBER acc_options  events 
			{$$=add_string_accelerator($1,$3,$4,$5);}
		| NUMBER ',' NUMBER ',' ASCII acc_options  events 
			{$$=add_ascii_accelerator($1,$3,$6,$7);}
		| NUMBER ',' NUMBER ',' VIRTKEY acc_options  events 
			{$$=add_vk_accelerator($1,$3,$6,$7);}
acc_options:	{$$=0;}
		| ',' NOINVERT acc_options {$$=$3|2;}
		| ',' ALT acc_options      {$$=$3|16;}
		| ',' SHIFT acc_options	   {$$=$3|4;}
		| ',' CONTROL acc_options  {$$=$3|8;}

bitmap:		tBITMAP load_and_memoption tSTRING {$$=make_bitmap(load_file($3));}
		| tBITMAP load_and_memoption raw_data {$$=make_bitmap($3);}

/* load and memory options are ignored */
load_and_memoption:	| lamo load_and_memoption
lamo:	PRELOAD | LOADONCALL | tFIXED | MOVEABLE | DISCARDABLE | PURE

cursor:		CURSOR load_and_memoption tSTRING {$$=make_cursor(load_file($3));}
		|CURSOR load_and_memoption raw_data {$$=make_cursor($3);}

dialog:		DIALOG load_and_memoption NUMBER ',' NUMBER ',' NUMBER ',' NUMBER 
		dlg_attributes
		tBEGIN  controls tEND 
		{$$=make_dialog($10,$3,$5,$7,$9,$12);}

dlg_attributes:	{$$=new_dialog();}
		| STYLE style dlg_attributes 
		  {$$=dialog_style($2,$3);}
		| CAPTION tSTRING dlg_attributes
		  {$$=dialog_caption($2,$3);}
		| FONT NUMBER ',' tSTRING dlg_attributes 
		  {$$=dialog_font($2,$4,$5);}
		| CLASS tSTRING dlg_attributes
		  {$$=dialog_class($2,$3);}
		| MENU tSTRING dlg_attributes
		  {$$=dialog_menu_str($2,$3);}
		| MENU NUMBER dlg_attributes
		  {$$=dialog_menu_id($2,$3);}

/* the controls are collected into a gen_res, and finally the dialog header 
   is put at the beginning */
controls:	{$$=new_res();}
		| CHECKBOX  labeled_control controls 
		  {$$=add_control(CT_BUTTON, BS_CHECKBOX, $2, $3);}
		| COMBOBOX control_desc controls 
		  {$$=add_control(CT_COMBOBOX, 0, $2, $3);}
		| CONTROL generic_control controls
		  {$$=add_generic_control($2, $3);}
		| CTEXT labeled_control controls 
		  {$$=add_control(CT_STATIC, SS_CENTER, $2, $3);}
		| DEFPUSHBUTTON labeled_control controls 
		  {$$=add_control(CT_BUTTON, BS_DEFPUSHBUTTON, $2, $3);}
		| EDITTEXT control_desc controls 
		  {$$=add_control(CT_EDIT, 0, $2, $3);}
		| GROUPBOX labeled_control controls 
		  {$$=add_control(CT_BUTTON, BS_GROUPBOX, $2, $3);}
		/*special treatment for icons, as the extent is optional*/
		| ICON tSTRING ',' NUMBER ',' NUMBER ',' NUMBER iconinfo controls
		  {$$=add_icon($2, $4, $6, $8, $9, $10);}
		| LISTBOX control_desc controls 
		  {$$=add_control(CT_LISTBOX, 0, $2, $3);}
		| LTEXT labeled_control controls 
		  {$$=add_control(CT_STATIC, SS_LEFT, $2, $3);}
		| PUSHBUTTON labeled_control controls 
		  {$$=add_control(CT_BUTTON, BS_PUSHBUTTON, $2, $3);}
		| RADIOBUTTON labeled_control controls 
		  {$$=add_control(CT_BUTTON, BS_RADIOBUTTON, $2, $3);}
		| RTEXT labeled_control controls 
		  {$$=add_control(CT_STATIC, SS_RIGHT, $2, $3);}
		| SCROLLBAR control_desc controls		
		  {$$=add_control(CT_SCROLLBAR, 0, $2, $3);}


labeled_control: tSTRING ',' control_desc {$$=label_control_desc($1,$3);}
control_desc:	NUMBER ',' NUMBER ',' NUMBER ',' NUMBER ',' NUMBER optional_style 
		{$$=create_control_desc($1,$3,$5,$7,$9,$10);}

optional_style: {$$=0;}|
		',' style {$$=$2;}

iconinfo:	/*set extent and style to 0 if they are not provided */
		{$$=create_control_desc(0,0,0,0,0,0);} 
		/* x and y are overwritten later */
		| ',' NUMBER ',' NUMBER optional_style
        {$$=create_control_desc(0,0,0,$2,$4,$5);}

generic_control:	tSTRING ',' NUMBER ',' tSTRING ',' style ',' NUMBER
		',' NUMBER ',' NUMBER ',' NUMBER
		{$$=create_generic_control($1,$3,$5,$7,$9,$11,$13,$15);}

font:		FONT load_and_memoption tSTRING {$$=make_font(load_file($3));}

icon:		ICON load_and_memoption tSTRING {$$=make_icon(load_file($3));}
		| ICON load_and_memoption raw_data {$$=make_icon($3);}

menu:		MENU load_and_memoption menu_body {$$=make_menu($3);}
/* menu items are collected in a gen_res and prefixed with the menu header*/
menu_body:	tBEGIN item_definitions tEND {$$=$2;}
item_definitions:	{$$=new_res();}
		| MENUITEM tSTRING ',' NUMBER item_options item_definitions
		  {$$=add_menuitem($2,$4,$5,$6);}
		| MENUITEM SEPARATOR item_definitions
		  {$$=add_menuitem("",0,0,$3);}
		| POPUP tSTRING item_options menu_body item_definitions
		  {$$=add_popup($2,$3,$4,$5);}
item_options:	{$$=0;}
		| ',' CHECKED item_options {$$=$3|MF_CHECKED;}
		| ',' GRAYED item_options {$$=$3|MF_GRAYED;}
		| ',' HELP item_options {$$=$3|MF_HELP;}
		| ',' INACTIVE item_options {$$=$3|MF_DISABLED;}
		| ',' MENUBARBREAK item_options {$$=$3|MF_MENUBARBREAK;}
		| ',' MENUBREAK item_options {$$=$3|MF_MENUBREAK;}

rcdata:		RCDATA load_and_memoption raw_data {$$=make_raw($3);}

raw_data:	tBEGIN raw_elements tEND {$$=$2;}
raw_elements:	SINGLE_QUOTED {$$=hex_to_raw($1,new_res());}
		| NUMBER {$$=int_to_raw($1,new_res());}
		| SINGLE_QUOTED raw_elements {$$=hex_to_raw($1,$2);}
		| NUMBER ',' raw_elements {$$=int_to_raw($1,$3);}

stringtable:	STRINGTABLE load_and_memoption tBEGIN strings tEND
			{$$=$4;}
strings:	{$$=0;}|
		NUMBER tSTRING strings {$$=0;add_str_tbl_elm($1,$2);}

versioninfo:	VERSIONINFO NOT_SUPPORTED {$$=0;}

/* NOT x | NOT y | a | b means (a|b)& ~x & ~y
   NOT is used to disable default styles */
style:	        {$$=new_style();}
                | style_elm {$$=$1;}
		| style_elm '|' style 
                {$$=$1;$$->or|=$3->or;$$->and&=$3->and;free($3);}

style_elm:      NUMBER {$$=new_style();$$->or=$1;}
		| NOT NUMBER {$$=new_style();$$->and=~($2);}
		| '(' style ')' {$$=$2;}
%%
extern int line_number;
extern char* yytext;

int yyerror( const char *s )
{
	fprintf(stderr,"stdin:%d: %s before '%s'\n",line_number,s,yytext);
	return 0;
}

