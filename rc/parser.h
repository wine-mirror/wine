/*
 *
 * Copyright  Martin von Loewis, 1994
 *
 */

/* resource types */
enum rt {acc,bmp,cur,dlg,fnt,ico,men,rdt,str};
/* generic resource
   Bytes can be inserted at arbitrary positions, the data field (res) 
   grows as required. As the dialog header contains the number of 
   controls, this number is generated in num_entries. If n_type if 0,
   the resource name is i_name, and s_name otherwise. Top level
   resources are linked via next. All gen_res objects are linked via
   g_prev, g_next for debugging purposes. space is the length of res,
   size is the used part of res.
   As most bison rules are right recursive, new items are usually 
   inserted at the beginning
*/   
typedef struct gen_res{
	int size,space;
	int num_entries;
	enum rt type;
	union{
		int i_name;
		char* s_name;
	}n;
	int n_type; /*0 - integer, 1 = string*/
	struct gen_res *next;
	struct gen_res *g_prev,*g_next;
	unsigned char res[0];
} gen_res;

/* control/dialog style. or collects styles, and collects NOT styles */
typedef struct rc_style{
	int and, or;
}rc_style;

/* create a new resource */
gen_res *new_res(void);
/* double the space of the resource */
gen_res* grow(gen_res*);
/* insert byte array at the beginning, increase count */
gen_res* insert_at_beginning(gen_res*,char*,int);
/* insert byte array at offset */
gen_res* insert_bytes(gen_res*,char*,int,int);
/* delete bytes at offset */
gen_res* delete_bytes(gen_res*,int,int);
/* create a new style */
rc_style* new_style(void);
/* convert \t to tab etc. */
char* parse_c_string(char*);
/* get the resources type, convert dlg to "DIALOG" and so on */
char* get_typename(gen_res*);

gen_res* add_accelerator(int,int,int,gen_res*);
gen_res* add_string_accelerator(char*,int,int,gen_res*);
gen_res* add_ascii_accelerator(int,int,int,gen_res*);
gen_res* add_vk_accelerator(int,int,int,gen_res*);

gen_res* new_dialog(void);
gen_res* dialog_style(rc_style*,gen_res*);
int dialog_get_menu(gen_res*);
int dialog_get_class(gen_res*);
int dialog_get_caption(gen_res*);
int dialog_get_fontsize(gen_res*);
gen_res* dialog_caption(char*,gen_res*);
gen_res* dialog_font(short,char*,gen_res*);
gen_res* dialog_class(char*,gen_res*);
gen_res* dialog_menu_id(short,gen_res*);
gen_res* dialog_menu_str(char*,gen_res*);
gen_res* create_control_desc(int,int,int,int,int,rc_style*);
gen_res* label_control_desc(char*,gen_res*);
gen_res* create_generic_control(char*,int,char*,rc_style*,int,int,int,int);
gen_res* add_control(int,int,gen_res*,gen_res*);
gen_res* add_icon(char*,int,int,int,gen_res*,gen_res*);
gen_res* add_generic_control(gen_res*,gen_res*);
gen_res* make_dialog(gen_res*,int,int,int,int,gen_res*);

gen_res *hex_to_raw(char*,gen_res*);
gen_res *int_to_raw(int,gen_res*);
gen_res *make_font(gen_res*);
gen_res *make_raw(gen_res*);
gen_res *make_bitmap(gen_res*);
gen_res *make_icon(gen_res*);
gen_res *make_cursor(gen_res*);
gen_res *load_file(char*);

gen_res *add_menuitem(char*,int,int,gen_res*);
gen_res *add_popup(char*,short,gen_res*,gen_res*);
gen_res *make_menu(gen_res*);

gen_res *add_resource(gen_res*,gen_res*);

void add_str_tbl_elm(int,char*);

void create_output(gen_res*);
void set_out_file(char*);

#define CT_BUTTON 	0x80
#define CT_EDIT 	0x81
#define CT_STATIC 	0x82
#define CT_LISTBOX	0x83
#define CT_SCROLLBAR 0x84
#define CT_COMBOBOX	0x85

extern int verbose;

