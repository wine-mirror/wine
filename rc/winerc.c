/*
 *
 * Copyright  Martin von Loewis, 1994
 *
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include "windows.h"
#include "wine/winuser16.h"
#include "parser.h"

char usage[]="winerc -bdvc -p prefix -o outfile < infile \n"
	"   -b            Create a C array from a binary .res file\n"
        "   -c            Add 'const' prefix to C constants\n"
	"   -d            Output debugging information\n"
        "   -h            Also generate a .h file\n"
	"   -p prefix     Give a prefix for the generated names\n"
	"   -v            Show each resource as it is processed\n"
	"   -o file       Output to file.c and file.h\n"
	"   -w 16|32      Select win16 or win32 output\n";

/*might be overwritten by command line*/
char *prefix="_Resource";
int win32=1;
int verbose,constant;
gen_res* g_start;
static FILE *header = NULL, *code = NULL;

int transform_binary_file(void);
int yyparse(void);

static void *xmalloc (size_t size)
{
    void *res;

    res = malloc (size ? size : 1);
    if (res == NULL)
    {
        fprintf (stderr, "Virtual memory exhausted.\n");
        exit (1);
    }
    return res;
}


int main(int argc,char *argv[])
{  
	extern int yydebug;
	extern char* optarg;
	int optc,lose = 0, ret, binary = 0, output_header = 0;
        char output_name[256];

	while((optc=getopt(argc,argv,"bcdhp:vo:w:"))!=EOF)
		switch(optc)
		{
			/* bison will print state transitions on stderr */
			case 'b':binary=1;
					 break;
			case 'd':yydebug=1;
					 setbuf(stdout,0);
					 setbuf(stderr,0);
					break;
                        case 'h':output_header=1; break;
			case 'p':prefix=strdup(optarg); break;
			case 'c':constant=1;break;
			case 'v':verbose=1;
					 setbuf(stderr,0);
					break;
			case 'o':sprintf(output_name,"%s.c",optarg); break;
			case 'w':if(!strcmp(optarg,"16"))win32=0;
				 else if(!strcmp(optarg,"32"))win32=1;
				 else lose++;
				 break;
			default: lose++;break;
		}
	if(lose)return fprintf(stderr,usage),1;

        if (output_name[0])
        {
            code = fopen( output_name, "w" );
            if (output_header)
            {
                output_name[strlen(output_name)-1] = 'h';
                header = fopen( output_name, "w" );
            }
        }
        if (!code) code = stdout;
	if(binary)
		ret=transform_binary_file();
	else
		ret=yyparse();
	if (header) fclose(header);
	fclose(code);
        if (ret) /* There was an error */
        {
            if (header) unlink( output_name );
            output_name[strlen(output_name)-1] = 'c';
            unlink( output_name );
        }
	return ret;
}

int transform_binary_file()
{
	int i,c;
	if (header) fprintf(header,"#define APPLICATION_HAS_RESOURCES 1\n");
	fprintf(code,"char _Application_resources[]={");
	for(i=0;;i++)
	{
		c=getchar();
		if(c==-1)break;
		if(i%16==0)fputc('\n',code);
		fprintf(code,"%3d,",c);
	}
	fprintf(code,"\n  0};\nint _Application_resources_size=%d;\n",i);
	return 0;
}

/* SunOS' memcpy is wrong for overlapping arrays */
char *save_memcpy(char *d,char* s,int l)
{
	if(d<s)
		for(;l;l--)*d++=*s++;
	else
		for(d+=l-1,s+=l-1;l;l--)*d--=*s--;
	return d;
}

/*allow unaligned access*/
void put_WORD(unsigned char* p,WORD w)
{
	*p=w&0xFF;
	*(p+1)=w>>8;
}

void put_DWORD(unsigned char* p,DWORD d)
{
	put_WORD(p,d&0xFFFF);
	put_WORD(p+2,d>>16);
}

WORD get_WORD(unsigned char* p)
{
	return *p|(*(p+1)<<8);
}

DWORD get_DWORD(unsigned char* p)
{
	return get_WORD(p)|(get_WORD(p+2)<<16);
}


/*create a new gen_res, initial size 100*/
gen_res *new_res()
{	gen_res* ret=xmalloc(sizeof(gen_res)+100);
	int i;
	for(i=0;i<sizeof(gen_res)+100;i++)*((char*)ret+i)='\0';
	ret->g_next=g_start;
	ret->g_prev=0;
	g_start=ret;
	ret->space=100;
	return ret;
}

/*double the space*/
gen_res* grow(gen_res* res)
{
	res=realloc(res,sizeof(gen_res)+2*res->space);
	if(!res)
		fprintf(stderr,"Out of memory\n"),exit(1);
	if(!res->g_prev)g_start=res;
	else res->g_prev->g_next=res;
	if(res->g_next)res->g_next->g_prev=res;
	res->space=2*res->space;
	return res;
}


/* insert bytes at offset 0, increase num_entries */
gen_res* insert_at_beginning(gen_res* res,char* entry,int size)
{
	while(res->size+size>res->space)res=grow(res);
	save_memcpy(res->res+size,res->res,res->size);
	save_memcpy(res->res,entry,size);
	res->size+=size;
	res->num_entries++;
	return res;
}

/* insert length characters from bytes into res, starting at start */
gen_res* insert_bytes(gen_res* res,char* bytes,int start,int length)
{
	while(res->size+length>res->space)res=grow(res);
	save_memcpy(res->res+start+length,res->res+start,res->size-start);
	save_memcpy(res->res+start,bytes,length);
	res->size+=length;
	return res;
}

/* insert string into res, starting at start */
gen_res* insert_string(gen_res* res,unsigned char* string,int start,int terminating0)
{
	unsigned char* p;
	int lengthA = strlen(string) + (terminating0 ? 1 : 0);
	int length = (win32 ? 2 : 1) * lengthA; 
	while(res->size+length>res->space)res=grow(res);
	save_memcpy(res->res+start+length,res->res+start,res->size-start);
	p=res->res+start;
	while(lengthA--)
		if (win32)
		{
			put_WORD(p, *string++);
			p+=2;
		}
		else
			*p++=*string++;
	res->size+=length;
	return res;
}

/* insert string at offset 0, increase num_entries */
gen_res* insert_string_at_beginning(gen_res* res,char* entry,int terminating0)
{
	res=insert_string(res,entry,0,terminating0);
	res->num_entries++;
	return res;
}

/*delete len bytes from res, starting at start*/
gen_res* delete_bytes(gen_res* res,int start,int len)
{
	save_memcpy(res->res+start,res->res+start+len,res->size-start-len);
	res->size-=len;
	return res;
}

/*create a new style*/
rc_style *new_style()
{
	rc_style *ret=xmalloc(sizeof(rc_style));
	/*initially, no bits have to be reset*/
	ret->and=-1;
	/*initially, no bits are set*/
	ret->or=WS_CHILD | WS_VISIBLE;
	return ret;
}

/* entries are inserted at the beginning, starting from the last one */
gen_res* add_accelerator(int ev, int id, int flags, gen_res* prev)
{
	char accel_entry[5];
	if(prev->num_entries==0)flags|=0x80; /* last entry */
	accel_entry[0]=flags;
	put_WORD(accel_entry+1,ev);
	put_WORD(accel_entry+3,id);
	return insert_at_beginning(prev,accel_entry,5);
}


/* create an integer from the event, taking things as "^c" into account
   add this as new entry */
gen_res* add_string_accelerator(char *ev, int id, int flags, gen_res* prev)
{
	int event;
	if(*ev=='^')
		event=ev[1]-'a';
	else
		event=ev[0];
	return add_accelerator(event,id,flags,prev);
}

/*is there a difference between ASCII and VIRTKEY accelerators? */

gen_res* add_ascii_accelerator(int ev, int id, int flags, gen_res* prev)
{
	return add_accelerator(ev,id,flags,prev);
}

gen_res* add_vk_accelerator(int ev, int id, int flags, gen_res* prev)
{
	return add_accelerator(ev,id,flags,prev);
}

/* create a new dialog header, set all items to 0 */
gen_res* new_dialog()
{	gen_res* ret=new_res();
	ret->size=win32?24:16; /*all strings "\0", no font*/
	return ret;
}

/* the STYLE option was specified */
gen_res* dialog_style(rc_style* style, gen_res* attr)
{
	/* default dialog styles? Do we need style->and? */
	/* DS_SETFONT might have been specified before */
	put_DWORD(attr->res,get_DWORD(attr->res)|style->or);
	return attr;
}

/* menu name is at offset 13 (win32: 18) */
int dialog_get_menu(gen_res* attr)
{
	return win32?18:13;
}

/* the class is after the menu name */
int dialog_get_class(gen_res* attr)
{
	int offs=dialog_get_menu(attr);
	while(attr->res[offs]||(win32&&attr->res[offs+1]))offs+=win32?2:1;
	offs+=win32?2:1;
	return offs;
}

/* the caption is after the class */
int dialog_get_caption(gen_res* attr)
{
	int offs=dialog_get_class(attr);
	while(attr->res[offs]||(win32&&attr->res[offs+1]))offs+=win32?2:1;
	offs+=win32?2:1;
	return offs;
}

/* the fontsize, if present, is after the caption, followed by the font name */
int dialog_get_fontsize(gen_res* attr)
{
	int offs=dialog_get_caption(attr);
	while(attr->res[offs]||(win32&&attr->res[offs+1]))offs+=win32?2:1;
	offs+=win32?2:1;
	return offs;
}


/* the CAPTION option was specified */
gen_res* dialog_caption(char* cap, gen_res*attr)
{
	/* we don't need the terminating 0 as it's already there */
	return insert_string(attr,cap,dialog_get_caption(attr),0);
}


/* the FONT option was specified, set the DS_SETFONT flag */
gen_res* dialog_font(short size,char* font,gen_res *attr)
{
	char c_size[2];
	int offs=dialog_get_fontsize(attr);
	put_DWORD(attr->res,get_DWORD(attr->res)|DS_SETFONT);
	put_WORD(c_size,size);
	attr=insert_bytes(attr,c_size,offs,2);
	offs+=2;
	/* as there is no font name by default, copy the '\0' */
	return insert_string(attr,font,offs,1);
}

gen_res* dialog_class(char* cap, gen_res*attr)
{
	return insert_string(attr,cap,dialog_get_class(attr),0);
}

gen_res* dialog_menu_id(short nr, gen_res*attr)
{
	char c_nr[2];
	int offs=dialog_get_menu(attr);
	attr->res[offs] = 0xff;
	if (win32) attr->res[offs+1] = 0xff;
	put_WORD(c_nr,nr);
	return insert_bytes(attr,c_nr,offs+(win32?2:1),2);
}
gen_res* dialog_menu_str(char* cap, gen_res*attr)
{
	return insert_string(attr,cap,dialog_get_menu(attr),0);
}

/* set the dialogs id, position, extent, and style */
gen_res* create_control_desc(int id,int x,int y,int cx, int cy,rc_style *style)
{	gen_res* ret=new_res();
	int s=WS_VISIBLE|WS_CHILD; /*defaults styles for any control*/
	if(win32)
	{
		if(style)s=(s|style->or)&style->and;
		put_DWORD(ret->res+0,s);
		/* FIXME */
		/* put_WORD(ret->res+4, exStyle); */
		put_WORD(ret->res+8,x);
		put_WORD(ret->res+10,y);
		put_WORD(ret->res+12,cx);
		put_WORD(ret->res+14,cy);
		put_WORD(ret->res+16,id);
		ret->size=24; /*empty strings, unused byte*/
	}
	else
	{
		put_WORD(ret->res+0,x);
		put_WORD(ret->res+2,y);
		put_WORD(ret->res+4,cx);
		put_WORD(ret->res+6,cy);
		put_WORD(ret->res+8,id);
		if(style)s=(s|style->or)&style->and;
		put_DWORD(ret->res+10,s);
		ret->size=17; /*empty strings, unused byte*/
	}
	return ret;
}

/* insert the control's label */
gen_res* label_control_desc(char* label,gen_res* cd)
{
	int offs;
	if(win32) {
		if(get_WORD(cd->res+18)==0xffff)offs=20; /* one-character class */
		else {
			for(offs=18;get_WORD(cd->res+offs);offs+=2);
			offs+=2;
		}
	}
	else {
		if(cd->res[14]&0x80)offs=15; /* one-character class */
		else {
			for(offs=14;cd->res[offs];offs++);
			offs++;
		}
	}
	return insert_string(cd,label,offs,0);
}

/* a CONTROL was specified */
gen_res* create_generic_control(char* label,int id,char* class,
	rc_style*style,int x,int y,int cx,int cy)
{	gen_res* ret=new_res();
	int s=WS_VISIBLE|WS_CHILD; /*default styles for any control*/
	if(style)s=(s|style->or)&style->and;
	if(win32)
	{
		WORD cl;
		put_DWORD(ret->res+0,s);
		/* FIXME */
		/* put_DWORD(ret->res+4,exstyle->or); */
		put_WORD(ret->res+8,x);
		put_WORD(ret->res+10,y);
		put_WORD(ret->res+12,cx);
		put_WORD(ret->res+14,cy);
		put_WORD(ret->res+16,id);
		ret->size=24;
		ret=insert_string(ret,label,20,0);
		/* is it a predefined class? */
		cl=0;
		if(!strcasecmp(class,"BUTTON"))cl=CT_BUTTON;
		if(!strcasecmp(class,"EDIT"))cl=CT_EDIT;
		if(!strcasecmp(class,"STATIC"))cl=CT_STATIC;
		if(!strcasecmp(class,"LISTBOX"))cl=CT_LISTBOX;
		if(!strcasecmp(class,"SCROLLBAR"))cl=CT_SCROLLBAR;
		if(!strcasecmp(class,"COMBOBOX"))cl=CT_COMBOBOX;
		if(cl) {
			char ffff[2]={0xff, 0xff};
			ret=insert_bytes(ret,ffff,18,2);
			put_WORD(ret->res+20,cl);
		}
		else ret=insert_string(ret,class,18,0);
	}
	else
	{
		char cl;
		put_WORD(ret->res+0,x);
		put_WORD(ret->res+2,y);
		put_WORD(ret->res+4,cx);
		put_WORD(ret->res+6,cy);
		put_WORD(ret->res+8,id);
		put_DWORD(ret->res+10,s);
		ret->size=17;
		ret=insert_string(ret,label,15,0);
		/* is it a predefined class? */
		cl=0;
		if(!strcasecmp(class,"BUTTON"))cl=CT_BUTTON;
		if(!strcasecmp(class,"EDIT"))cl=CT_EDIT;
		if(!strcasecmp(class,"STATIC"))cl=CT_STATIC;
		if(!strcasecmp(class,"LISTBOX"))cl=CT_LISTBOX;
		if(!strcasecmp(class,"SCROLLBAR"))cl=CT_SCROLLBAR;
		if(!strcasecmp(class,"COMBOBOX"))cl=CT_COMBOBOX;
		if(cl)ret->res[14]=cl;
		else ret=insert_string(ret,class,14,0);
	}
	return ret;
}

/* insert cd into rest, set the type, add flags */
gen_res* add_control(int type,int flags,gen_res*cd,gen_res* rest)
{
	char zeros[4]={0,0,0,0};
	if(win32)
	{
		char ffff[2]={0xff, 0xff};
		put_DWORD(cd->res+0,get_DWORD(cd->res+0)|flags);
		cd=insert_bytes(cd,ffff,18,2);
		put_WORD(cd->res+20,type);
	}
	else
	{
		put_DWORD(cd->res+10,get_DWORD(cd->res+10)|flags);
		cd->res[14]=type;
	}
	/* WIN32: First control is on dword boundary */
	if(win32 && cd->size%4)
		cd=insert_bytes(cd,zeros,cd->size,4-cd->size%4);
	return insert_at_beginning(rest,cd->res,cd->size);
}

/* an ICON control was specified, whf contains width, height, and flags */
gen_res* add_icon(char* name,int id,int x,int y,gen_res* whf,gen_res* rest)
{
    if (win32)
    {
	put_WORD(whf->res+8,x);
	put_WORD(whf->res+10,y);
	put_WORD(whf->res+16,id);
    }
    else
    {
	put_WORD(whf->res+0,x);
	put_WORD(whf->res+2,y);
	put_WORD(whf->res+8,id);
    }
	whf=label_control_desc(name,whf);
	return add_control(CT_STATIC,SS_ICON,whf,rest);
}

/* insert the generic control into rest */
gen_res* add_generic_control(gen_res* ctl, gen_res* rest)
{
	char zeros[4]={0,0,0,0};
	/* WIN32: Control is on dword boundary */
	if(win32 && ctl->size%4)
		ctl=insert_bytes(ctl,zeros,ctl->size,4-ctl->size%4);
	return insert_at_beginning(rest,ctl->res,ctl->size);
}

/* create a dialog resource by inserting the header into the controls.
   Set position and extent */
gen_res* make_dialog(gen_res* header,int x,int y,int cx,int cy,gen_res* ctls)
{
	char zeros[4]={0,0,0,0};
	if(win32)
	{
		put_WORD(header->res+8, ctls->num_entries);
		header->type=dlg;
		put_WORD(header->res+10,x);
		put_WORD(header->res+12,y);
		put_WORD(header->res+14,cx);
		put_WORD(header->res+16,cy);
	}
	else
	{
		header->res[4]=ctls->num_entries;
		header->type=dlg;
		put_WORD(header->res+5,x);
		put_WORD(header->res+7,y);
		put_WORD(header->res+9,cx);
		put_WORD(header->res+11,cy);
	}
	/* WIN32: First control is on dword boundary */
	if(win32 && header->size%4)
		header=insert_bytes(header,zeros,header->size,4-header->size%4);
	return insert_bytes(header,ctls->res,header->size,ctls->size);
}

/* create {0x15,0x16,0xFF} from '15 16 FF' */
gen_res *hex_to_raw(char *hex, gen_res*rest)
{
	char r2[16];
    int i;
	for(i=0;*hex!='\'';i++)r2[i]=strtoul(hex,&hex,16);
	return insert_bytes(rest,r2,0,i);
}

/* create a bitmap resource */
gen_res *make_bitmap(gen_res* res)
{
	res=delete_bytes(res,0,14); /* skip bitmap file header*/
	res->type=bmp;
	return res;
}

gen_res *make_icon(gen_res* res)
{
	res->type=ico;
	return res;
}

gen_res *make_cursor(gen_res* res)
{
	res->type=cur;
	return res;
}

/* load resource bytes from the file name */
gen_res *load_file(char* name)
{
	gen_res *res;
	struct stat st;
	int f=open(name,O_RDONLY);
	if(f<0)
	{
	  perror(name);
	  exit(1);
	}
	fstat(f,&st);
	res=new_res();
	while(res->space<st.st_size)res=grow(res);
	read(f,res->res,st.st_size);
	res->size=st.st_size;
	close(f);
	return res;
}

/* insert a normal menu item into res, starting from the last item */
gen_res *add_menuitem(char* name,int id,int flags,gen_res *res)
{
	char item[4];
	if(res->num_entries==0)flags|=MF_END;
	put_WORD(item,flags);
	put_WORD(item+2,id);
	res=insert_string_at_beginning(res,name,1);
	res=insert_bytes(res,item,0,4);
	return res;
}

/* insert a popup item into res */
gen_res *add_popup(char *name,short flags, gen_res* body, gen_res*res)
{
	char c_flags[2];
	flags|=MF_POPUP;
	if(res->num_entries==0)flags|=MF_END;
	put_WORD(c_flags,flags);
	res=insert_at_beginning(res,body->res,body->size);
	res=insert_string(res,name,0,1);
	res=insert_bytes(res,c_flags,0,2);
	return res;
}

/* prefix the menu header into res */
gen_res *make_menu(gen_res* res)
{
	static char header[4]={0,0,0,0};
	res=insert_at_beginning(res,header,4);
	res->type=men;
	return res;
}

/* link top-level resources */
gen_res *add_resource(gen_res* first,gen_res *rest)
{
    if(first)
    {
	first->next=rest;
	return first;
    }
    else
        return rest;
}

typedef struct str_tbl_elm{
	int group;
	struct str_tbl_elm *next;
	char* strings[16];
} str_tbl_elm;

str_tbl_elm* string_table=NULL; /* sorted by group */

void add_str_tbl_elm(int id,char* str)
{
  int group=(id>>4)+1;
  int idx=id & 0x000f;

  str_tbl_elm** elm=&string_table;
  while(*elm && (*elm)->group<group) elm=&(*elm)->next;
  if(!*elm || (*elm)->group!=group)
  {
    int i;
    str_tbl_elm* new=xmalloc(sizeof(str_tbl_elm));
    for(i=0; i<16; i++) new->strings[i] = NULL;
    new->group=group;
    new->next=*elm;
    *elm=new;
  }
  (*elm)->strings[idx]=str;
}

gen_res* add_string_table(gen_res* t)
{
  str_tbl_elm* ste;
  int size,i;
  gen_res* res;
  unsigned char* p;
  unsigned char* q;

  if(!string_table) return t;
  for(ste=string_table; ste; ste=ste->next)
  {
    for(size=0,i=0; i<16; i++)
      size += (win32 ? 2 : 1) * (ste->strings[i] ? strlen(ste->strings[i])+1 : 1);
    res=new_res();
    while(res->space<size)res=grow(res);
    res->type=str;
    res->n.i_name=ste->group;
    res->n_type=0;
    res->size=size;
    if (win32)
      for(p=res->res,i=0; i<16; i++)
        if((q=ste->strings[i])==NULL)
        {
	  put_WORD(p, 0);
	  p+=2;
	}
	else
	{
	  put_WORD(p, strlen(q));
	  p+=2;
	  while(*q)
	  {
	    put_WORD(p, *q++);
	    p+=2;
	  }
	}
    else
      for(p=res->res,i=0; i<16; i++)
	if((q=ste->strings[i])==NULL)
	  *p++ = 0;
	else
	{
	  *p++ = strlen(q);
	  while(*q) *p++ = *q++;
	}
    t=add_resource(res,t);
  }
  return t;
}

char *get_typename(gen_res* t)
{
	switch(t->type){
	case acc:return "ACCELERATOR";
	case bmp:return "BITMAP";
	case cur:return "CURSOR";
	case dlg:return "DIALOG";
	case fnt:return "FONT";
	case ico:return "ICON";
	case men:return "MENU";
	case rdt:return "RCDATA";
	case str:return "STRINGTABLE";
	default: return "UNKNOWN";
	}
}

/* create strings like _Sysres_DIALOG_2 */
char *get_resource_name(gen_res*it)
{
	static char buf[1000];
	if(it->n_type)
		sprintf(buf,"%s_%s_%s",prefix,get_typename(it),it->n.s_name);
	else
		sprintf(buf,"%s_%s_%d",prefix,get_typename(it),it->n.i_name);
	return buf;
}

#define ISCONSTANT	(constant ? "const " : "")

/* create the final output */
void create_output(gen_res* top)
{
    gen_res *it;

    top=add_string_table(top);

    /* Generate the header */

    if (header)
    {
        fprintf( header,
                 "/*\n"
                 " * This file is automatically generated. Do not edit!\n"
                 " */\n\n"
                 "#ifndef __%s_H\n"
                 "#define __%s_H\n\n"
                 "struct resource;\n\n",
                 prefix, prefix );

        /* Declare the resources */
        for (it=top;it;it=it->next)
            fprintf( header,"extern %sstruct resource %s;\n",
                     ISCONSTANT, get_resource_name(it) );
        fprintf( header,"\nextern %sstruct resource * %s%s_Table[];\n\n",
                 ISCONSTANT, ISCONSTANT, prefix );
        fprintf( header, "#endif  /* __%s_H */\n", prefix );
    }

    /* Print the resources bytes */

    fprintf( code, "/*\n"
                   " * This file is automatically generated. Do not edit!\n"
                   " */\n\n"
                   "struct resource {\n"
                   "\tint id;\n"
                   "\tint type;\n"
                   "\tconst char *name;\n"
                   "\tconst unsigned char* bytes;\n"
                   "\tunsigned size;\n"
                   "};\n\n" );

    for(it=top;it;it=it->next)
    {
        int i;
        fprintf( code, "static %sunsigned char %s__bytes[]%s = {\n",
                 ISCONSTANT, get_resource_name(it),
		 win32?"\n__attribute__ ((aligned (4)))":"");
        for (i=0;i<it->size-1;i++)
        {
            fprintf(code,"0x%02x, ",it->res[i]);
            if ((i&7)==7)fputc('\n',code);
        }
        fprintf(code,"0x%02x };\n\n",it->res[i]);
    }

    /* Print the resources names */

    if (win32)
        for(it=top;it;it=it->next)
        {
            int i;
	    char s_buffer[20], *s_name=s_buffer;
	    if(it->n_type) s_name=it->n.s_name;
	    else sprintf(s_name,"@%d",it->n.i_name);
	    fprintf( code, "static %sunsigned char %s__name[] = {\n",
		     ISCONSTANT, get_resource_name(it) );
	    for (i=0;*s_name;i++,s_name++)
	      {
		fprintf(code,"0x%02x, 0x00, ",*s_name);
		if ((i&3)==3)fputc('\n',code);
	      }
	    fprintf(code,"0x00, 0x00};\n\n");
	}

    /* Print the resources */
    for (it=top;it;it=it->next)
    {
        int type;
        switch(it->type)
        {
        case acc:type=(int)RT_ACCELERATOR16;break;
        case bmp:type=(int)RT_BITMAP16;break;
        case cur:type=(int)RT_CURSOR16;break;
        case dlg:type=(int)RT_DIALOG16;break;
        case fnt:type=(int)RT_FONT16;break;
        case ico:type=(int)RT_ICON16;break;
        case men:type=(int)RT_MENU16;break;
        case rdt:type=(int)RT_RCDATA16;break;
        case str:type=(int)RT_STRING16;break;
        default:fprintf(stderr,"Unknown restype\n");type=-1;break;
        }
	if(win32)
	{
            if(it->n_type)
                fprintf(code,"%sstruct resource %s = {0,%d,%s__name,%s__bytes,%d};\n",
			ISCONSTANT, get_resource_name(it), type, get_resource_name(it),
			get_resource_name(it), it->size );
	    else
	        fprintf(code,"%sstruct resource %s = {%d,%d,%s__name,%s__bytes,%d};\n",
			ISCONSTANT, get_resource_name(it), it->n.i_name, type,
			get_resource_name(it), get_resource_name(it), it->size );
	}
	else
	{
            if(it->n_type)
                fprintf(code,"%sstruct resource %s = {0,%d,\"%s\",%s__bytes,%d};\n",
			ISCONSTANT, get_resource_name(it), type, it->n.s_name,
			get_resource_name(it), it->size );
	    else
	        fprintf(code,"%sstruct resource %s = {%d,%d,\"@%d\",%s__bytes,%d};\n",
			ISCONSTANT, get_resource_name(it), it->n.i_name, type,
			it->n.i_name, get_resource_name(it), it->size );
	}
    }

    /* Print the resource table (NULL terminated) */

    fprintf(code,"\n%sstruct resource * %s%s_Table[] = {\n",
	    ISCONSTANT, ISCONSTANT, prefix);
    for (it=top;it;it=it->next)
        fprintf( code, "  &%s,\n", get_resource_name(it) );
    fprintf( code, "  0\n};\n\n\n" );

    /* Perform autoregistration */
    fprintf( code, 
             "#ifndef __WINE__\n"
             "#if defined(__GNUC__) && ((__GNUC__ > 2) || ((__GNUC__ == 2) && (__GNUC_MINOR__ >= 7)))\n"
             "static void DoIt(void) __attribute__((constructor));\n"
             "#else\n"
             "static void DoIt(void);\n"
             "void LIBWINE_Register_%s(void) { DoIt(); }\n"
             "#endif\n"
             "static void DoIt(void)\n"
             "{\n"
             "\textern void LIBRES_RegisterResources(const struct resource* const * Res);\n"
             "\tLIBRES_RegisterResources(%s_Table);\n"
             "}\n\n"
             "#endif /* __WINE__ */\n"
             ,prefix,prefix);
}

gen_res* make_font(gen_res* res)
{
	fprintf(stderr,"Fonts not supported\n");
	return NULL;
}

gen_res* make_raw(gen_res* res)
{
	fprintf(stderr,"RCData not supported\n");
	return NULL;
}

gen_res* int_to_raw(int i,gen_res* res)
{
	fprintf(stderr,"IntToRaw not supported\n");
	return NULL;
}

/* translate "Hello,\\tworld!\\10" to "Hello,\tworld!\n" */
char *parse_c_string(char *in)
{
	char *out=xmalloc(strlen(in)-1);
	char *it;
	char tmp[5],*tend;
	for(it=out,in++;*in;in++)
	if(*in=='\\')
		switch(*++in)
		{case 't':*it++='\t';break;
		 case 'r':*it++='\r';break;
		 case 'n':*it++='\n';break;
		 case 'a':*it++='\a';break;
		 case '0':
			memset(tmp,0,5);/*make sure it doesn't use more than 4 chars*/
			memcpy(tmp,in,4);
			*it++=strtoul(tmp,&tend,0);
			in+=tend-tmp-1;
			break;
		 case '1':case '2':case '3':case '4':case '5':
		 case '6':case '7':case '8':case '9':
			memset(tmp,0,5);
			memcpy(tmp,in,3);
			*it++=strtoul(tmp,&tend,10);
			in+=tend-tmp-1;
			break;
		 case 'x':
			memset(tmp,0,5);
			memcpy(tmp,++in,2);
			*it++=strtoul(tmp,&tend,16);
			in+=tend-tmp-1;
			break;
		 default:*it++=*in;
		}
	else
		*it++=*in;
	*(it-1)='\0';
	return out;
}
