/*
 *
 * Copyright  Martin von Loewis, 1994
 *
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <windows.h>
/* #include <neexe.h> */
#include "parser.h"
#include "y.tab.h"

char usage[]="winerc -bdvc -p prefix -o outfile < infile \n"
	"   -b            Create a C array from a binary .res file\n"
        "   -c            Add 'const' prefix to C constants\n"
	"   -d            Output debugging information\n"
	"   -p prefix     Give a prefix for the generated names\n"
	"   -v            Show each resource as it is processed\n"
	"   -o file       Output to file.c and file.h\n";


/*might be overwritten by command line*/
char *prefix="_Resource";
int verbose,constant;
gen_res* g_start;
FILE *header,*code;
char hname[256],sname[256];

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
	char* tmpc;
	int optc,lose,ret,binary;
	lose=binary=0;
	while((optc=getopt(argc,argv,"bcdp:vo:"))!=EOF)
		switch(optc)
		{
			/* bison will print state transitions on stderr */
			case 'b':binary=1;
					 break;
			case 'd':yydebug=1;
					 setbuf(stdout,0);
					 setbuf(stderr,0);
					break;
			case 'p':prefix=strdup(optarg);
					 if(!isalpha(*prefix))*prefix='_';
					 for(tmpc=prefix;*tmpc;tmpc++)
					  if( !isalnum(*tmpc) && *tmpc!='_')
					   *tmpc='_';
					break;
			case 'c':constant=1;break;
			case 'v':verbose=1;
					 setbuf(stderr,0);
					break;
			case 'o':set_out_file(optarg);break;
			default: lose++;break;
		}
	if(lose)return fprintf(stderr,usage),1;
	if(!header)header=stdout;
	if(!code)code=stdout;
	if(binary)
		ret=transform_binary_file();
	else
		ret=yyparse();
	fclose(header);
	fclose(code);
	return ret;
}

void set_out_file(char *prefix)
{
	sprintf(sname,"%s.c",prefix);
	code=fopen(sname,"w");
	sprintf(hname,"%s.h",prefix);
	header=fopen(hname,"w");
}

int transform_binary_file()
{
	int i,c;
	fprintf(header,"#define APPLICATION_HAS_RESOURCES 1\n");
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
	ret->or=0;
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
	ret->size=16; /*all strings "\0", no font*/
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

/* menu name is at offset 13 */
int dialog_get_menu(gen_res* attr)
{
	return 13;
}

/* the class is after the menu name */
int dialog_get_class(gen_res* attr)
{
	int offs=dialog_get_menu(attr);
	while(attr->res[offs])offs++;
	offs++;
	return offs;
}

/* the caption is after the class */
int dialog_get_caption(gen_res* attr)
{
	int offs=dialog_get_class(attr);
	while(attr->res[offs])offs++;
	offs++;
	return offs;
}

/* the fontsize, if present, is after the caption, followed by the font name */
int dialog_get_fontsize(gen_res* attr)
{
	int offs=dialog_get_caption(attr);
	while(attr->res[offs])offs++;
	offs++;
	return offs;
}


/* the CAPTION option was specified */
gen_res* dialog_caption(char* cap, gen_res*attr)
{
	/* we don't need the terminating 0 as it's already there */
	return insert_bytes(attr,cap,dialog_get_caption(attr),strlen(cap));
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
	return insert_bytes(attr,font,offs,strlen(font)+1);
}

gen_res* dialog_class(char* cap, gen_res*attr)
{
	return insert_bytes(attr,cap,dialog_get_class(attr),strlen(cap));
}

gen_res* dialog_menu(char* cap, gen_res*attr)
{
	return insert_bytes(attr,cap,dialog_get_menu(attr),strlen(cap));
}

/* set the dialogs id, position, extent, and style */
gen_res* create_control_desc(int id,int x,int y,int cx, int cy,rc_style *style)
{	gen_res* ret=new_res();
	int s=WS_VISIBLE|WS_CHILD; /*defaults styles for any control*/
	put_WORD(ret->res+0,x);
	put_WORD(ret->res+2,y);
	put_WORD(ret->res+4,cx);
	put_WORD(ret->res+6,cy);
	put_WORD(ret->res+8,id);
	if(style)s=(s|style->or)&style->and;
	put_DWORD(ret->res+10,s);
	ret->size=17; /*empty strings, unused byte*/
	return ret;
}

/* insert the control's label */
gen_res* label_control_desc(char* label,gen_res* cd)
{
	int offs;
	if(cd->res[14]&0x80)offs=15; /* one-character class */
	else {
		for(offs=14;cd->res[offs];offs++);
		offs++;
	}
	return insert_bytes(cd,label,offs,strlen(label));
}

/* a CONTROL was specified */
gen_res* create_generic_control(char* label,int id,char* class,
	rc_style*style,int x,int y,int cx,int cy)
{	char cl;
	gen_res* ret=new_res();
	put_WORD(ret->res+0,x);
    put_WORD(ret->res+2,y);
    put_WORD(ret->res+4,cx);
    put_WORD(ret->res+6,cy);
    put_WORD(ret->res+8,id);
	put_DWORD(ret->res+10,style->or);
	ret->size=17;
	ret=insert_bytes(ret,label,15,strlen(label));
	/* is it a predefined class? */
	cl=0;
	if(!strcmp(class,"BUTTON"))cl=CT_BUTTON;
	if(!strcmp(class,"EDIT"))cl=CT_EDIT;
	if(!strcmp(class,"STATIC"))cl=CT_STATIC;
	if(!strcmp(class,"LISTBOX"))cl=CT_LISTBOX;
	if(!strcmp(class,"SCROLLBAR"))cl=CT_SCROLLBAR;
	if(!strcmp(class,"COMBOBOX"))cl=CT_COMBOBOX;
	if(cl)ret->res[14]=cl;
	else ret=insert_bytes(ret,class,14,strlen(class));
	return ret;
}

/* insert cd into rest, set the type, add flags */
gen_res* add_control(int type,int flags,gen_res*cd,gen_res* rest)
{
	put_DWORD(cd->res+10,get_DWORD(cd->res+10)|flags);
	cd->res[14]=type;
	return insert_at_beginning(rest,cd->res,cd->size);
}

/* an ICON control was specified, whf contains width, height, and flags */
gen_res* add_icon(char* name,int id,int x,int y,gen_res* whf,gen_res* rest)
{
	put_WORD(whf->res+0,x);
	put_WORD(whf->res+2,y);
	put_WORD(whf->res+8,id);
	whf=label_control_desc(name,whf);
	return add_control(CT_STATIC,SS_ICON,whf,rest);
}

/* insert the generic control into rest */
gen_res* add_generic_control(gen_res* ctl, gen_res* rest)
{
	return insert_at_beginning(rest,ctl->res,ctl->size);
}

/* create a dialog resource by inserting the header into the controls.
   Set position and extent */
gen_res* make_dialog(gen_res* header,int x,int y,int cx,int cy,gen_res* ctls)
{
	header->res[4]=ctls->num_entries;
	header->type=dlg;
	put_WORD(header->res+5,x);
	put_WORD(header->res+7,y);
	put_WORD(header->res+9,cx);
	put_WORD(header->res+11,cy);
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
	if(!f)perror(name);
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
	res=insert_at_beginning(res,name,strlen(name)+1);
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
	res=insert_bytes(res,name,0,strlen(name)+1);
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
	first->next=rest;
	return first;
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


    fprintf( header, "/* %s\n"
                     " * This file is automatically generated. Do not edit!\n"
                     " */\n\n"
                     "#include \"resource.h\"\n", hname );

    /* Declare the resources */

    for (it=top;it;it=it->next)
        fprintf( header,"extern %sstruct resource %s;\n",
                 ISCONSTANT, get_resource_name(it) );
    fprintf( header,"\nextern %sstruct resource * %s%s_Table[];\n",
             ISCONSTANT, ISCONSTANT, prefix );

    /* Print the resources bytes */

    fprintf( code, "/* %s\n"
                   " * This file is automatically generated. Do not edit!\n"
                   " */\n\n"
                   "#include \"%s\"\n", sname, hname );
    for(it=top;it;it=it->next)
    {
        int i;
        fprintf( code, "static %sunsigned char %s__bytes[] = {\n",
                 ISCONSTANT, get_resource_name(it) );
        for (i=0;i<it->size-1;i++)
        {
            fprintf(code,"0x%02x, ",it->res[i]);
            if ((i&7)==7)fputc('\n',code);
        }
        fprintf(code,"0x%02x };\n\n",it->res[i]);
    }

    /* Print the resources */
    for (it=top;it;it=it->next)
    {
        int type;
        switch(it->type)
        {
        case acc:type=(int)RT_ACCELERATOR;break;
        case bmp:type=(int)RT_BITMAP;break;
        case cur:type=(int)RT_CURSOR;break;
        case dlg:type=(int)RT_DIALOG;break;
        case fnt:type=(int)RT_FONT;break;
        case ico:type=(int)RT_ICON;break;
        case men:type=(int)RT_MENU;break;
        case rdt:type=(int)RT_RCDATA;break;
        case str:type=(int)RT_STRING;break;
        default:fprintf(stderr,"Unknown restype\n");type=-1;break;
        }
        if(it->n_type)
            fprintf(code,"%sstruct resource %s = {0,%d,\"%s\",%s__bytes,%d};\n",
                    ISCONSTANT, get_resource_name(it), type, it->n.s_name,
                    get_resource_name(it), it->size );
        else
            fprintf(code,"%sstruct resource %s = {%d,%d,\"@%d\",%s__bytes,%d};\n",
                    ISCONSTANT, get_resource_name(it), it->n.i_name, type,
                    it->n.i_name, get_resource_name(it), it->size );
    }

    /* Print the resource table (NULL terminated) */

    fprintf(code,"\n%sstruct resource * %s%s_Table[] = {\n",
	    ISCONSTANT, ISCONSTANT, prefix);
    for (it=top;it;it=it->next)
        fprintf( code, "  &%s,\n", get_resource_name(it) );
    fprintf( code, "  0\n};\n" );

        /* Perform autoregistration */
        fprintf( code, 
                "#ifdef WINELIB\n"
                "static void DoIt() WINE_CONSTRUCTOR;\n"
                "static void DoIt()\n"
                "{\n"
                "\tLIBRES_RegisterResources(%s_Table);\n"
                "}\n\n"
                "#ifndef HAVE_WINE_CONSTRUCTOR\n"
                "void LIBWINE_Register_%s(){\n"
                "\tDoIt();\n"
                "}\n"
                "#endif\n"
                "#endif /*WINELIB*/\n"
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
