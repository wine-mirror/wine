/*
 * 	Registry Functions
 *
 * Copyright 1996 Marcus Meissner
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <pwd.h>
#include <time.h>
#include "windows.h"
#include "win.h"
#include "winerror.h"
#include "string32.h"	
#include "stddebug.h"
#include "debug.h"
#include "xmalloc.h"
#include "winreg.h"

/* FIXME: following defines should be configured global ... */

/* NOTE: do not append a /. linux' mkdir() WILL FAIL if you do that */
#define WINE_PREFIX			"/.wine"
#define SAVE_CURRENT_USER_DEFAULT	"/usr/local/etc/wine.userreg"
	/* relative in ~user/.wine/ */
#define SAVE_CURRENT_USER		"user.reg"
#define SAVE_LOCAL_MACHINE_DEFAULT	"/usr/local/etc/wine.systemreg"
	/* relative in ~user/.wine/ */
#define SAVE_LOCAL_MACHINE		"system.reg"

static KEYSTRUCT	*key_classes_root=NULL;	/* windows 3.1 global values */
static KEYSTRUCT	*key_current_user=NULL;	/* user specific values */
static KEYSTRUCT	*key_local_machine=NULL;/* machine specific values */
static KEYSTRUCT	*key_users=NULL;	/* all users? */

/* dynamic, not saved */
static KEYSTRUCT	*key_performance_data=NULL;
static KEYSTRUCT	*key_current_config=NULL;
static KEYSTRUCT	*key_dyn_data=NULL;

/* what valuetypes do we need to convert? */
#define UNICONVMASK	((1<<REG_SZ)|(1<<REG_MULTI_SZ)|(1<<REG_EXPAND_SZ))

#define strdupA2W(x)	STRING32_DupAnsiToUni(x)
#define strdupW2A(x)	STRING32_DupUniToAnsi(x)
#define strdupW(x)	STRING32_strdupW(x)
#define strcmpW(a,b)	STRING32_lstrcmpW(a,b)
#define strcmpniW(a,b)	STRING32_lstrcmpniW(a,b)
#define strchrW(a,c)	STRING32_lstrchrW(a,c)
#define strlenW(a)	STRING32_UniLen(a)
#define strcpyWA(a,b)	STRING32_UniToAnsi(a,b)

static struct openhandle {
	LPKEYSTRUCT	lpkey;
	HKEY		hkey;
	REGSAM		accessmask;
}  *openhandles=NULL;
static int	nrofopenhandles=0;
static int	currenthandle=1;

static void
add_handle(HKEY hkey,LPKEYSTRUCT lpkey,REGSAM accessmask) {
	int	i;

	for (i=0;i<nrofopenhandles;i++) {
		if (openhandles[i].lpkey==lpkey) {
			dprintf_reg(stddeb,"add_handle:Tried to add %p twice!\n",lpkey);
		}
		if (openhandles[i].hkey==hkey) {
			dprintf_reg(stddeb,"add_handle:Tried to add %lx twice!\n",(LONG)hkey);
		}
	}
	openhandles=xrealloc(	openhandles,
				sizeof(struct openhandle)*(nrofopenhandles+1)
		);
	openhandles[i].lpkey	= lpkey;
	openhandles[i].hkey	= hkey;
	openhandles[i].accessmask= accessmask;
	nrofopenhandles++;
}

static LPKEYSTRUCT
get_handle(HKEY hkey) {
	int	i;

	for (i=0;i<nrofopenhandles;i++)
		if (openhandles[i].hkey==hkey)
			return openhandles[i].lpkey;
	dprintf_reg(stddeb,"get_handle:Didn't find handle %lx?\n",(LONG)hkey);
	return NULL;
}

static void
remove_handle(HKEY hkey) {
	int	i;

	for (i=0;i<nrofopenhandles;i++)
		if (openhandles[i].hkey==hkey)
			break;
	if (i==nrofopenhandles) {
		dprintf_reg(stddeb,"remove_handle:Didn't find handle %lx?\n",hkey);
		return;
	}
	memcpy(	openhandles+i,
		openhandles+i+1,
		sizeof(struct openhandle)*(nrofopenhandles-i-1)
	);
	openhandles=xrealloc(openhandles,sizeof(struct openhandle)*(nrofopenhandles-1));
	nrofopenhandles--;
	return;
}


/* debug function, converts a unicode into a static memory area 
 * (sub for using two static strings, in case we need them in a single call)
 */
LPSTR
W2C(LPCWSTR x,int sub) {
	static	LPSTR	unicodedebug[2]={NULL,NULL};
	if (x==NULL)
		return "<NULL>";
	if (sub!=0 && sub!=1)
		return "<W2C:bad sub>";
	if (unicodedebug[sub]) free(unicodedebug[sub]);
	unicodedebug[sub]	= strdupW2A(x);
	return unicodedebug[sub];
}

static LPKEYSTRUCT
lookup_hkey(HKEY hkey) {
	switch (hkey) {
	case 0x00000000:
	case 0x00000001:
	case HKEY_CLASSES_ROOT:
		return key_classes_root;
	case HKEY_CURRENT_USER:
		return key_current_user;
	case HKEY_LOCAL_MACHINE:
		return key_local_machine;
	case HKEY_USERS:
		return key_users;
	case HKEY_PERFORMANCE_DATA:
		return key_performance_data;
	case HKEY_DYN_DATA:
		return key_dyn_data;
	case HKEY_CURRENT_CONFIG:
		return key_current_config;
	default:
		dprintf_reg(stddeb,"lookup_hkey(%lx), special key!\n",
			(LONG)hkey
		);
		return get_handle(hkey);
	}
	/*NOTREACHED*/
}

/* 
 * splits the unicode string 'wp' into an array of strings.
 * the array is allocated by this function. 
 * the number of components will be stored in 'wpc'
 * Free the array using FREE_KEY_PATH
 */
static void
split_keypath(LPCWSTR wp,LPWSTR **wpv,int *wpc) {
	int	i,j,len;
	LPWSTR	ws;

	ws	= strdupW(wp);
	*wpc	= 1;
	for (i=0;ws[i];i++) {
		if (ws[i]=='\\') {
			ws[i]=0;
			(*wpc)++;
		}
	}
	len	= i;
	*wpv	= (LPWSTR*)xmalloc(sizeof(LPWSTR)*(*wpc+2));
	(*wpv)[0]= ws;
	j	= 1;
	for (i=1;i<len;i++)
		if (ws[i-1]==0)
			(*wpv)[j++]=ws+i;
	(*wpv)[j]=NULL;
}
#define FREE_KEY_PATH	free(wps[0]);free(wps);

/*
 * Shell initialisation, allocates keys. 
 */
void
SHELL_Init() {
	struct	passwd	*pwd;

	HKEY	cl_r_hkey,c_u_hkey;
#define ADD_ROOT_KEY(xx) \
	xx = (LPKEYSTRUCT)xmalloc(sizeof(KEYSTRUCT));\
	memset(xx,'\0',sizeof(KEYSTRUCT));\
	xx->keyname= strdupA2W("<should_not_appear_anywhere>");

	ADD_ROOT_KEY(key_local_machine);
	if (RegCreateKey(HKEY_LOCAL_MACHINE,"\\SOFTWARE\\Classes",&cl_r_hkey)!=ERROR_SUCCESS) {
		fprintf(stderr,"couldn't create HKEY_LOCAL_MACHINE\\SOFTWARE\\Classes. This is impossible.\n");
		exit(1);
	}
	key_classes_root = lookup_hkey(cl_r_hkey);

	ADD_ROOT_KEY(key_users);

#if 0
	/* FIXME: load all users and their resp. pwd->pw_dir/.wine/user.reg 
	 *	  (later, when a win32 registry editing tool becomes avail.)
	 */
	while (pwd=getpwent()) {
		if (pwd->pw_name == NULL)
			continue;
		RegCreateKey(HKEY_USERS,pwd->pw_name,&c_u_hkey);
		RegCloseKey(c_u_hkey);
	}
#endif
	pwd=getpwuid(getuid());
	if (pwd && pwd->pw_name) {
		RegCreateKey(HKEY_USERS,pwd->pw_name,&c_u_hkey);
		key_current_user = lookup_hkey(c_u_hkey);
	} else {
		ADD_ROOT_KEY(key_current_user);
	}
	ADD_ROOT_KEY(key_performance_data);
	ADD_ROOT_KEY(key_current_config);
	ADD_ROOT_KEY(key_dyn_data);
#undef ADD_ROOT_KEY
}

/************************ SAVE Registry Function ****************************/

#define REGISTRY_SAVE_VERSION	0x00000001

/* Registry saveformat:
 * If you change it, increase above number by 1, which will flush
 * old registry database files.
 * 
 * Global:
 * 	"WINE REGISTRY Version %d"
 * 	subkeys....
 * Subkeys:
 * 	keyname
 *		valuename=lastmodified,type,data
 *		...
 *		subkeys
 *	...
 * keyname,valuename,stringdata:
 *	the usual ascii characters from 0x00-0xff (well, not 0x00)
 *	and \uXXXX as UNICODE value XXXX with XXXX>0xff
 *	( "=\\\t" escaped in \uXXXX form.)
 * type,lastmodified: 
 *	int
 * 
 * FIXME: doesn't save 'class' (what does it mean anyway?), nor flags.
 */

static void
_write_USTRING(FILE *F,LPWSTR wstr,int escapeeq) {
	LPWSTR	s;
	int	doescape;

	if (wstr==NULL) {
		/* FIXME: NULL equals empty string... I hope
		 * the empty string isn't a valid valuename
		 */
		return;
	}
	s=wstr;
	while (*s) {
		doescape=0;
		if (*s>0xff)
			doescape = 1;
		if (*s=='\n')
			doescape = 1;
		if (escapeeq && *s=='=')
			doescape = 1;
		if (*s=='\\')
			fputc(*s,F); /* if \\ than put it twice. */
		if (doescape)
			fprintf(F,"\\u%04x",*((unsigned short*)s));
		else
			fputc(*s,F);
		s++;
	}
}

static int
_do_save_subkey(FILE *F,LPKEYSTRUCT lpkey,int level) {
	LPKEYSTRUCT	lpxkey;
	int		i,tabs,j;

	lpxkey	= lpkey;
	while (lpxkey) {
		if (!(lpxkey->flags & REG_OPTION_VOLATILE)) {
			for (tabs=level;tabs--;)
				fputc('\t',F);
			_write_USTRING(F,lpxkey->keyname,1);
			fputs("\n",F);
			for (i=0;i<lpxkey->nrofvalues;i++) {
				LPKEYVALUE	val=lpxkey->values+i;

				for (tabs=level+1;tabs--;)
					fputc('\t',F);
				_write_USTRING(F,val->name,0);
				fputc('=',F);
				fprintf(F,"%ld,%ld,",val->type,val->lastmodified);
				if ((1<<val->type) & UNICONVMASK)
					_write_USTRING(F,(LPWSTR)val->data,0);
				else
					for (j=0;j<val->len;j++)
						fprintf(F,"%02x",*((unsigned char*)val->data+j));
				fputs("\n",F);
			}
			/* descend recursively */
			if (!_do_save_subkey(F,lpxkey->nextsub,level+1))
				return 0;
		}
		lpxkey=lpxkey->next;
	}
	return 1;
}

static int
_do_savesubreg(FILE *F,LPKEYSTRUCT lpkey) {
	fprintf(F,"WINE REGISTRY Version %d\n",REGISTRY_SAVE_VERSION);
	return _do_save_subkey(F,lpkey->nextsub,0);
}

static void
_SaveSubReg(LPKEYSTRUCT lpkey,char *fn) {
	FILE	*F;

	F=fopen(fn,"w");
	if (F==NULL) {
		fprintf(stddeb,__FILE__":_SaveSubReg:Couldn't open %s for writing: %s\n",
			fn,strerror(errno)
		);
		return;
	}
	if (!_do_savesubreg(F,lpkey)) {
		fclose(F);
		unlink(fn);
		fprintf(stddeb,__FILE__":_SaveSubReg:Failed to save keys, perhaps no more diskspace for %s?\n",fn);
		return;
	}
	fclose(F);
}

void
SHELL_SaveRegistry() {
	char	*fn;
	struct	passwd	*pwd;

	pwd=getpwuid(getuid());
	if (pwd!=NULL && pwd->pw_dir!=NULL) {
		fn=(char*)xmalloc(strlen(pwd->pw_dir)+strlen(WINE_PREFIX)+strlen(SAVE_CURRENT_USER)+2);
		strcpy(fn,pwd->pw_dir);
		strcat(fn,WINE_PREFIX);
		/* create the directory. don't care about errorcodes. */
		mkdir(fn,0755); /* drwxr-xr-x */
		strcat(fn,"/"SAVE_CURRENT_USER);
		_SaveSubReg(key_current_user,fn);
		free(fn);
		fn=(char*)xmalloc(strlen(pwd->pw_dir)+strlen(WINE_PREFIX)+strlen(SAVE_LOCAL_MACHINE)+2);
		strcpy(fn,pwd->pw_dir);
		strcat(fn,WINE_PREFIX"/"SAVE_LOCAL_MACHINE);
		_SaveSubReg(key_local_machine,fn);
		free(fn);
	} else {
		fprintf(stderr,"SHELL_SaveRegistry:failed to get homedirectory of UID %d.\n",getuid());
	}
}

/************************ LOAD Registry Function ****************************/

/* reads a line including dynamically enlarging the readbuffer and throwing
 * away comments
 */
static int 
_read_line(FILE *F,char **buf,int *len) {
	char	*s,*curread;
	int	mylen,curoff;

	curread	= *buf;
	mylen	= *len;
	**buf	= '\0';
	while (1) {
		while (1) {
			s=fgets(curread,mylen,F);
			if (s==NULL)
				return 0; /* EOF */
			if (NULL==(s=strchr(curread,'\n'))) {
				/* buffer wasn't large enough */
				curoff	= strlen(*buf);
				*buf	= xrealloc(*buf,*len*2);
				curread	= *buf + curoff;
				mylen	= *len;	/* we filled up the buffer and 
						 * got new '*len' bytes to fill
						 */
				*len	= *len * 2;
			} else {
				*s='\0';
				break;
			}
		}
		/* throw away comments */
		if (**buf=='#' || **buf==';') {
			curread	= *buf;
			mylen	= *len;
			continue;
		}
		if (s) 	/* got end of line */
			break;
	}
	return 1;
}

/* converts a char* into a UNICODE string (up to a special char)
 * and returns the position exactly after that string
 */
static char*
_read_USTRING(char *buf,LPWSTR *str) {
	char	*s;
	LPWSTR	ws;

	/* read up to "=" or "\0" or "\n" */
	s	= buf;
	if (*s == '=') {
		/* empty string is the win3.1 default value(NULL)*/
		*str	= NULL;
		return s;
	}
	*str	= (LPWSTR)xmalloc(2*strlen(buf)+2);
	ws	= *str;
	while (*s && (*s!='\n') && (*s!='=')) {
		if (*s!='\\')
			*ws++=*((unsigned char*)s++);
		else {
			s++;
			if (*s=='\\') {
				*ws+='\\';
				s++;
				continue;
			}
			if (*s!='u') {
				fprintf(stderr,"_read_USTRING:Non unicode escape sequence \\%c found in |%s|\n",*s,buf);
				*ws++='\\';
				*ws++=*s++;
			} else {
				char	xbuf[5];
				int	wc;

				s++;
				memcpy(xbuf,s,4);xbuf[4]='\0';
				if (!sscanf(xbuf,"%x",&wc))
					fprintf(stderr,"_read_USTRING:strange escape sequence %s found in |%s|\n",xbuf,buf);
				s+=4;
				*ws++	=(unsigned short)wc;
			}
		}
	}
	*ws	= 0;
	ws	= *str;
	*str	= strdupW(*str);
	free(ws);
	return s;
}

static int
_do_load_subkey(FILE *F,LPKEYSTRUCT lpkey,int level,char **buf,int *buflen) {
	LPKEYSTRUCT	lpxkey,*lplpkey;
	int		i;
	char		*s;
	LPWSTR		name;

	/* good. we already got a line here ... so parse it */
	lpxkey	= NULL;
	while (1) {
		i=0;s=*buf;
		while (*s=='\t') {
			s++;
			i++;
		}
		if (i>level) {
			if (lpxkey==NULL) {
				fprintf(stderr,"_do_load_subkey:Got a subhierarchy without resp. key?\n");
				return 0;
			}
			_do_load_subkey(F,lpxkey,level+1,buf,buflen);
			continue;
		}
		/* let the caller handle this line */
		if (i<level || **buf=='\0')
			return 1;
		/* good. this is one line for us.
		 * it can be: a value or a keyname. Parse the name first
		 */
		s=_read_USTRING(s,&name);

		/* switch() default: hack to avoid gotos */
		switch (0) {
		default:
			if (*s=='\0') {
				/* this is a new key 
				 * look for the name in the already existing keys
				 * on this level.
				 */
				 lplpkey= &(lpkey->nextsub);
				 lpxkey	= *lplpkey;
				 while (lpxkey) {
					if (!strcmpW(lpxkey->keyname,name))
						break;
					lplpkey	= &(lpxkey->next);
					lpxkey	= *lplpkey;
				 }
				 if (lpxkey==NULL) {
					/* we have no key with that name yet. allocate
					 * it.
					 */
					*lplpkey = (LPKEYSTRUCT)xmalloc(sizeof(KEYSTRUCT));
					lpxkey	= *lplpkey;
					memset(lpxkey,'\0',sizeof(KEYSTRUCT));
					lpxkey->keyname	= name;
				 } else {
					/* already got it. we just remember it in 
					 * 'lpxkey'
					 */
					free(name);
				 }
			} else {
				LPKEYVALUE	val=NULL;
				LPBYTE		data;
				int		len,lastmodified,type;

				if (*s!='=') {
					fprintf(stderr,"_do_load_subkey:unexpected character: %c\n",*s);
					break;
				}
				/* good. this looks like a value to me */
				s++;
				for (i=0;i<lpkey->nrofvalues;i++) {
					val=lpkey->values+i;
					if (name==NULL) {
						if (val->name==NULL)
							break;
					} else {
						if (	val->name!=NULL && 
							!strcmpW(val->name,name)
						)
							break;
					}
				}
				if (i==lpkey->nrofvalues) {
					lpkey->values = xrealloc(
						lpkey->values,
						(++lpkey->nrofvalues)*sizeof(KEYVALUE)
					);
					val=lpkey->values+i;
					memset(val,'\0',sizeof(KEYVALUE));
					val->name = name;
				} else {
					/* value already exists, free name */
					free(name);
				}
				if (2!=sscanf(s,"%d,%d,",&type,&lastmodified)) {
					fprintf(stderr,"_do_load_subkey: haven't understood possible value in |%s|, skipping.\n",*buf);
					break;
				}
				/* skip the 2 , */
				s=strchr(s,',');s++;
				s=strchr(s,',');s++;
				if ((1<<type) & UNICONVMASK) {
					s=_read_USTRING(s,(LPWSTR*)&data);
					if (data)
						len = strlenW((LPWSTR)data)*2+2;
					else	
						len = 0;
				} else {
					len=strlen(s)/2;
					data = (LPBYTE)xmalloc(len+1);
					for (i=0;i<len;i++) {
						data[i]=0;
						if (*s>='0' && *s<='9')
							data[i]=(*s-'0')<<4;
						if (*s>='a' && *s<='f')
							data[i]=(*s-'a')<<4;
						if (*s>='A' && *s<='F')
							data[i]=(*s-'A')<<4;
						s++;
						if (*s>='0' && *s<='9')
							data[i]|=*s-'0';
						if (*s>='a' && *s<='f')
							data[i]|=*s-'a';
						if (*s>='A' && *s<='F')
							data[i]|=*s-'A';
						s++;
					}
				}
				if (val->lastmodified<lastmodified) {
					val->lastmodified=lastmodified;
					val->type = type;
					val->len  = len;
					if (val->data) 
						free(val->data);
					val->data = data;
				} else {
					free(data);
				}
			}
		}
		/* read the next line */
		if (!_read_line(F,buf,buflen))
			return 1;
	}
	return 1;
}

static int
_do_loadsubreg(FILE *F,LPKEYSTRUCT lpkey) {
	int	ver;
	char	*buf;
	int	buflen;

	buf=xmalloc(10);buflen=10;
	if (!_read_line(F,&buf,&buflen)) {
		free(buf);
		return 0;
	}
	if (!sscanf(buf,"WINE REGISTRY Version %d",&ver)) {
		free(buf);
		return 0;
	}
	if (ver!=REGISTRY_SAVE_VERSION) {
		dprintf_reg(stddeb,__FILE__":_do_loadsubreg:Old format (%d) registry found, ignoring it. (buf was %s).\n",ver,buf);
		free(buf);
		return 0;
	}
	if (!_read_line(F,&buf,&buflen)) {
		free(buf);
		return 0;
	}
	if (!_do_load_subkey(F,lpkey,0,&buf,&buflen)) {
		free(buf);
		/* FIXME: memory leak on failure to read registry ... 
		 * But this won't happen very often.
		 */
		lpkey->nextsub=NULL;
		return 0;
	}
	free(buf);
	return 1;
}

static void
_LoadSubReg(LPKEYSTRUCT lpkey,char *fn) {
	FILE	*F;

	F=fopen(fn,"rb");
	if (F==NULL) {
		dprintf_reg(stddeb,__FILE__":Couldn't open %s for reading: %s\n",
			fn,strerror(errno)
		);
		return;
	}
	if (!_do_loadsubreg(F,lpkey)) {
		fclose(F);
		unlink(fn);
		return;
	}
	fclose(F);
}

void
SHELL_LoadRegistry() {
	char	*fn;
	struct	passwd	*pwd;

	if (key_classes_root==NULL)
		SHELL_Init();
	/* load the machine-wide defaults first */
	_LoadSubReg(key_current_user,SAVE_CURRENT_USER_DEFAULT);
	_LoadSubReg(key_local_machine,SAVE_LOCAL_MACHINE_DEFAULT);

	/* load the user saved registry. overwriting only newer entries */
	pwd=getpwuid(getuid());
	if (pwd!=NULL && pwd->pw_dir!=NULL) {
		fn=(char*)xmalloc(strlen(pwd->pw_dir)+strlen(WINE_PREFIX)+strlen(SAVE_CURRENT_USER)+2);
		strcpy(fn,pwd->pw_dir);
		strcat(fn,WINE_PREFIX"/"SAVE_CURRENT_USER);
		_LoadSubReg(key_current_user,fn);
		free(fn);
		fn=(char*)xmalloc(strlen(pwd->pw_dir)+strlen(WINE_PREFIX)+strlen(SAVE_LOCAL_MACHINE)+2);
		strcpy(fn,pwd->pw_dir);
		strcat(fn,WINE_PREFIX"/"SAVE_LOCAL_MACHINE);
		_LoadSubReg(key_local_machine,fn);
		free(fn);
	} else {
		fprintf(stderr,"SHELL_LoadRegistry:failed to get homedirectory of UID %d.\n",getuid());
	}
	/* FIXME: load all users and their resp. pwd->pw_dir/.wine/user.reg 
	 *	  (later, when a win32 registry editing tool becomes avail.)
	 */
}

/********************* API FUNCTIONS ***************************************/

/*
 * Open Keys.
 *
 * All functions are stubs to RegOpenKeyExW where all the
 * magic happens. 
 *
 * FIXME: security,options,desiredaccess,...
 *
 * Callpath:
 * RegOpenKey -> RegOpenKeyA -> RegOpenKeyExA \
 *                              RegOpenKeyW   -> RegOpenKeyExW 
 */

/* RegOpenKeyExW		[ADVAPI32.150] */
WINAPI DWORD
RegOpenKeyExW(
	HKEY	hkey,
	LPCWSTR	lpszSubKey,
	DWORD	dwReserved,
	REGSAM	samDesired,
	LPHKEY	retkey
) {
	LPKEYSTRUCT	lpNextKey,lpxkey;
	LPWSTR		*wps;
	int		wpc,i;
	dprintf_reg(stddeb,"RegOpenKeyExW(%lx,%s,%ld,%lx,%p)\n",
		(LONG)hkey,W2C(lpszSubKey,0),dwReserved,samDesired,retkey
	);

	lpNextKey	= lookup_hkey(hkey);
	if (!lpNextKey)
		return SHELL_ERROR_BADKEY;
	if (!lpszSubKey || !*lpszSubKey) {
		add_handle(++currenthandle,lpNextKey,samDesired);
		*retkey=currenthandle;
		return SHELL_ERROR_SUCCESS;
	}
	split_keypath(lpszSubKey,&wps,&wpc);
	i 	= 0;
	while ((i<wpc) && (wps[i][0]=='\0')) i++;
	lpxkey	= lpNextKey;
	while (i<wpc) {
		lpxkey=lpNextKey->nextsub;
		while (lpxkey) {
			if (!strcmpW(wps[i],lpxkey->keyname))
				break;
			lpxkey=lpxkey->next;
		}
		if (!lpxkey) {
			FREE_KEY_PATH;
			return SHELL_ERROR_BADKEY;
		}
		i++;
		lpNextKey	= lpxkey;
	}
	add_handle(++currenthandle,lpxkey,samDesired);
	*retkey	= currenthandle;
	FREE_KEY_PATH;
	return	SHELL_ERROR_SUCCESS;
}

/* RegOpenKeyW			[ADVAPI32.151] */
WINAPI DWORD
RegOpenKeyW(
	HKEY	hkey,
	LPCWSTR	lpszSubKey,
	LPHKEY	retkey
) {
	dprintf_reg(stddeb,"RegOpenKeyW(%lx,%s,%p)\n",
		(LONG)hkey,W2C(lpszSubKey,0),retkey
	);
	return RegOpenKeyExW(hkey,lpszSubKey,0,KEY_ALL_ACCESS,retkey);
}


/* RegOpenKeyExA		[ADVAPI32.149] */
WINAPI DWORD
RegOpenKeyExA(
	HKEY	hkey,
	LPCSTR	lpszSubKey,
	DWORD	dwReserved,
	REGSAM	samDesired,
	LPHKEY	retkey
) {
	LPWSTR	lpszSubKeyW;
	DWORD	ret;

	dprintf_reg(stddeb,"RegOpenKeyExA(%lx,%s,%ld,%lx,%p)\n",
		(LONG)hkey,lpszSubKey,dwReserved,samDesired,retkey
	);
	if (lpszSubKey)
		lpszSubKeyW=strdupA2W(lpszSubKey);
	else
		lpszSubKeyW=NULL;
	ret=RegOpenKeyExW(hkey,lpszSubKeyW,dwReserved,samDesired,retkey);
	if (lpszSubKeyW)
		free(lpszSubKeyW);
	return ret;
}

/* RegOpenKeyA			[ADVAPI32.148] */
WINAPI DWORD
RegOpenKeyA(
	HKEY	hkey,
	LPCSTR	lpszSubKey,
	LPHKEY	retkey
) {
	dprintf_reg(stddeb,"RegOpenKeyA(%lx,%s,%p)\n",
		(LONG)hkey,lpszSubKey,retkey
	);
	return	RegOpenKeyExA(hkey,lpszSubKey,0,KEY_ALL_ACCESS,retkey);
}

/* RegOpenKey			[SHELL.1] [KERNEL.217] */
WINAPI DWORD
RegOpenKey(
	HKEY	hkey,
	LPCSTR	lpszSubKey,
	LPHKEY	retkey
) {
	dprintf_reg(stddeb,"RegOpenKey(%lx,%s,%p)\n",
		(LONG)hkey,lpszSubKey,retkey
	);
	return RegOpenKeyA(hkey,lpszSubKey,retkey);
}

/* 
 * Create keys
 * 
 * All those functions convert their respective 
 * arguments and call RegCreateKeyExW at the end.
 *
 * FIXME: no security,no access attrib,no optionhandling yet.
 *
 * Callpath:
 * RegCreateKey -> RegCreateKeyA -> RegCreateKeyExA \
 *                                  RegCreateKeyW   -> RegCreateKeyExW
 */

/* RegCreateKeyExW		[ADVAPI32.131] */
WINAPI DWORD
RegCreateKeyExW(
	HKEY	hkey,
	LPCWSTR	lpszSubKey,
	DWORD	dwReserved,
	LPWSTR	lpszClass,
	DWORD	fdwOptions,
	REGSAM	samDesired,
	LPSECURITY_ATTRIBUTES lpSecAttribs,
	LPHKEY	retkey,
	LPDWORD	lpDispos
) {
	LPKEYSTRUCT	*lplpPrevKey,lpNextKey,lpxkey;
	LPWSTR		*wps;
	int		wpc,i;

/*FIXME: handle security/access/whatever */
	dprintf_reg(stddeb,"RegCreateKeyExW(%lx,%s,%ld,%s,%lx,%lx,%p,%p,%p)\n",
		(LONG)hkey,
		W2C(lpszSubKey,0),
		dwReserved,
		W2C(lpszClass,1),
		fdwOptions,
		samDesired,
		lpSecAttribs,
		retkey,
		lpDispos
	);

	lpNextKey	= lookup_hkey(hkey);
	if (!lpNextKey)
		return SHELL_ERROR_BADKEY;
	if (!lpszSubKey || !*lpszSubKey) {
		add_handle(++currenthandle,lpNextKey,samDesired);
		*retkey=currenthandle;
		return SHELL_ERROR_SUCCESS;
	}
	split_keypath(lpszSubKey,&wps,&wpc);
	i 	= 0;
	while ((i<wpc) && (wps[i][0]=='\0')) i++;
	lpxkey	= lpNextKey;
	while (i<wpc) {
		lpxkey=lpNextKey->nextsub;
		while (lpxkey) {
			if (!strcmpW(wps[i],lpxkey->keyname))
				break;
			lpxkey=lpxkey->next;
		}
		if (!lpxkey)
			break;
		i++;
		lpNextKey	= lpxkey;
	}
	if (lpxkey) {
		add_handle(++currenthandle,lpxkey,samDesired);
		*retkey		= currenthandle;
		*lpDispos	= REG_OPENED_EXISTING_KEY;
		FREE_KEY_PATH;
		return	SHELL_ERROR_SUCCESS;
	}
	/* good. now the hard part */
	while (i<wpc) {
		lplpPrevKey	= &(lpNextKey->nextsub);
		lpxkey		= *lplpPrevKey;
		while (lpxkey) {
			lplpPrevKey	= &(lpxkey->next);
			lpxkey		= *lplpPrevKey;
		}
		*lplpPrevKey=malloc(sizeof(KEYSTRUCT));
		if (!*lplpPrevKey) {
			FREE_KEY_PATH;
			return SHELL_ERROR_OUTOFMEMORY;
		}
		memset(*lplpPrevKey,'\0',sizeof(KEYSTRUCT));
		(*lplpPrevKey)->keyname	= strdupW(wps[i]);
		(*lplpPrevKey)->next	= NULL;
		(*lplpPrevKey)->nextsub	= NULL;
		(*lplpPrevKey)->values	= NULL;
		(*lplpPrevKey)->nrofvalues = 0;
		if (lpszClass)
			(*lplpPrevKey)->class = strdupW(lpszClass);
		else
			(*lplpPrevKey)->class = NULL;
		lpNextKey	= *lplpPrevKey;
		i++;
	}
	add_handle(++currenthandle,lpNextKey,samDesired);

	/*FIXME: flag handling correct? */
	lpNextKey->flags= fdwOptions;
	if (lpszClass)
		lpNextKey->class = strdupW(lpszClass);
	else
		lpNextKey->class = NULL;
	*retkey		= currenthandle;
	*lpDispos	= REG_CREATED_NEW_KEY;
	FREE_KEY_PATH;
	return SHELL_ERROR_SUCCESS;
}

/* RegCreateKeyW		[ADVAPI32.132] */
WINAPI DWORD
RegCreateKeyW(
	HKEY	hkey,
	LPCWSTR	lpszSubKey,
	LPHKEY	retkey
) {
	DWORD	junk,ret;

	dprintf_reg(stddeb,"RegCreateKeyW(%lx,%s,%p)\n",
		(LONG)hkey,W2C(lpszSubKey,0),retkey
	);
	ret=RegCreateKeyExW(
		hkey,		/* key handle */
		lpszSubKey,	/* subkey name */
		0,		/* reserved = 0 */
		NULL,		/* lpszClass? FIXME: ? */
		REG_OPTION_NON_VOLATILE,	/* options */
		KEY_ALL_ACCESS,	/* desired access attribs */
		NULL,		/* lpsecurity attributes */
		retkey,		/* lpretkey */
		&junk		/* disposition value */
	);
	return	ret;
}

/* RegCreateKeyExA		[ADVAPI32.130] */
WINAPI DWORD
RegCreateKeyExA(
	HKEY	hkey,
	LPCSTR	lpszSubKey,
	DWORD	dwReserved,
	LPSTR	lpszClass,
	DWORD	fdwOptions,
	REGSAM	samDesired,
	LPSECURITY_ATTRIBUTES lpSecAttribs,
	LPHKEY	retkey,
	LPDWORD	lpDispos
) {
	LPWSTR	lpszSubKeyW,lpszClassW;
	DWORD	ret;

	dprintf_reg(stddeb,"RegCreateKeyExA(%lx,%s,%ld,%s,%lx,%lx,%p,%p,%p)\n",
		(LONG)hkey,
		lpszSubKey,
		dwReserved,
		lpszClass,
		fdwOptions,
		samDesired,
		lpSecAttribs,
		retkey,
		lpDispos
	);
	if (lpszSubKey)
		lpszSubKeyW=strdupA2W(lpszSubKey);
	else
		lpszSubKeyW=NULL;
	if (lpszClass)
		lpszClassW=strdupA2W(lpszClass);
	else
		lpszClassW=NULL;
	ret=RegCreateKeyExW(
		hkey,
		lpszSubKeyW,
		dwReserved,
		lpszClassW,
		fdwOptions,
		samDesired,
		lpSecAttribs,
		retkey,
		lpDispos
	);
	if (lpszSubKeyW)
		free(lpszSubKeyW);
	if (lpszClassW)
		free(lpszClassW);
	return ret;
}

/* RegCreateKeyA		[ADVAPI32.129] */
WINAPI DWORD
RegCreateKeyA(
	HKEY	hkey,
	LPCSTR	lpszSubKey,
	LPHKEY	retkey
) {
	DWORD	junk;

	dprintf_reg(stddeb,"RegCreateKeyA(%lx,%s,%p)\n",
		(LONG)hkey,lpszSubKey,retkey
	);
	return	RegCreateKeyExA(
		hkey,		/* key handle */
		lpszSubKey,	/* subkey name */
		0,		/* reserved = 0 */
		NULL,		/* lpszClass? FIXME: ? */
		REG_OPTION_NON_VOLATILE,/* options */
		KEY_ALL_ACCESS,	/* desired access attribs */
		NULL,		/* lpsecurity attributes */
		retkey,		/* lpretkey */
		&junk		/* disposition value */
	);
}

/* RegCreateKey			[SHELL.2] [KERNEL.218] */
WINAPI DWORD
RegCreateKey(
	HKEY	hkey,
	LPCSTR	lpszSubKey,
	LPHKEY	retkey
) {
	dprintf_reg(stddeb,"RegCreateKey(%lx,%s,%p)\n",
		(LONG)hkey,lpszSubKey,retkey
	);
	return RegCreateKeyA(hkey,lpszSubKey,retkey);
}

/* 
 * Query Value Functions
 * Win32 differs between keynames and valuenames. 
 * multiple values may belong to one key, the special value
 * with name NULL is the default value used by the win31
 * compat functions.
 *
 * Callpath:
 * RegQueryValue -> RegQueryValueA -> RegQueryValueExA \
 *                                    RegQueryValueW   -> RegQueryValueExW
 */

/* RegQueryValueExW		[ADVAPI32.158] */
WINAPI DWORD
RegQueryValueExW(
	HKEY	hkey,
	LPWSTR	lpszValueName,
	LPDWORD	lpdwReserved,
	LPDWORD	lpdwType,
	LPBYTE	lpbData,
	LPDWORD	lpcbData
) {
	LPKEYSTRUCT	lpkey;
	int		i;

	dprintf_reg(stddeb,"RegQueryValueExW(%lx,%s,%p,%p,%p,%p)\n",
		hkey,W2C(lpszValueName,0),lpdwReserved,lpdwType,lpbData,lpcbData
	);

	lpkey	= lookup_hkey(hkey);
	if (!lpkey)
		return SHELL_ERROR_BADKEY;
	if (lpszValueName==NULL) {
		for (i=0;i<lpkey->nrofvalues;i++)
			if (lpkey->values[i].name==NULL)
				break;
	} else {
		for (i=0;i<lpkey->nrofvalues;i++)
			if (!strcmpW(lpszValueName,lpkey->values[i].name))
				break;
	}
	if (i==lpkey->nrofvalues) {
		if (lpszValueName==NULL) {
			*(WCHAR*)lpbData = 0;
			*lpcbData	= 2;
			*lpdwType	= REG_SZ;
			return SHELL_ERROR_SUCCESS;
		}
		return SHELL_ERROR_BADKEY;/*FIXME: correct return? */
	}
	if (lpdwType)
		*lpdwType	= lpkey->values[i].type;
	if (lpbData==NULL) {
		if (lpcbData==NULL)
			return SHELL_ERROR_SUCCESS;
		*lpcbData	= lpkey->values[i].len;
		return SHELL_ERROR_SUCCESS;
	}
	if (*lpcbData<lpkey->values[i].len) {
		*(WCHAR*)lpbData
			= 0;
		*lpcbData	= lpkey->values[i].len;
		return ERROR_MORE_DATA;
	}
	memcpy(lpbData,lpkey->values[i].data,lpkey->values[i].len);
	*lpcbData	= lpkey->values[i].len;
	return SHELL_ERROR_SUCCESS;
}

/* RegQueryValueW		[ADVAPI32.159] */
WINAPI DWORD
RegQueryValueW(
	HKEY	hkey,
	LPWSTR	lpszSubKey,
	LPWSTR	lpszData,
	LPDWORD	lpcbData
) {
	HKEY	xhkey;
	DWORD	ret,lpdwType;

	dprintf_reg(stddeb,"RegQueryValueW(%lx,%s,%p,%p)\n->",
		hkey,W2C(lpszSubKey,0),lpszData,lpcbData
	);

	/* only open subkey, if we really do descend */
	if (lpszSubKey && *lpszSubKey) {
		ret	= RegOpenKeyW(hkey,lpszSubKey,&xhkey);
		if (ret!=ERROR_SUCCESS)
			return ret;
	} else
		xhkey	= hkey;

	lpdwType	= REG_SZ;
	ret	= RegQueryValueExW(
		xhkey,
		NULL,		/* varname NULL -> compat */
		NULL,		/* lpdwReserved, must be NULL */
		&lpdwType,
		(LPBYTE)lpszData,
		lpcbData
	);
	if (xhkey!=hkey)
		RegCloseKey(xhkey);
	return ret;
}

/* RegQueryValueExA		[ADVAPI32.157] */
WINAPI DWORD
RegQueryValueExA(
	HKEY	hkey,
	LPSTR	lpszValueName,
	LPDWORD	lpdwReserved,
	LPDWORD	lpdwType,
	LPBYTE	lpbData,
	LPDWORD	lpcbData
) {
	LPWSTR	lpszValueNameW;
	LPBYTE	buf;
	DWORD	ret,myxlen;
	DWORD	*mylen;

	dprintf_reg(stddeb,"RegQueryValueExA(%lx,%s,%p,%p,%p,%p)\n->",
		hkey,lpszValueName,lpdwReserved,lpdwType,lpbData,lpcbData
	);
	if (lpbData) {
		/* double buffer */
		buf	= (LPBYTE)xmalloc((*lpcbData)*2);
		myxlen	= *lpcbData*2;
		mylen	= &myxlen;
	} else {
		buf=NULL;
		if (lpcbData) {
			myxlen	= *lpcbData*2;
			mylen	= &myxlen;
		}
			mylen	= NULL;
	}
	if (lpszValueName)
		lpszValueNameW=strdupA2W(lpszValueName);
	else 
		lpszValueNameW=NULL;

	ret=RegQueryValueExW(
		hkey,
		lpszValueNameW,
		lpdwReserved,
		lpdwType,
		buf,
		mylen
	);

	if (ret==ERROR_SUCCESS) {
		if (buf) {
			if (UNICONVMASK & (1<<(*lpdwType))) {
				/* convert UNICODE to ASCII */
				strcpyWA(lpbData,(LPWSTR)buf);
				*lpcbData	= myxlen/2;
			} else {
				if (myxlen>*lpcbData)
					ret	= ERROR_MORE_DATA;
				else
					memcpy(lpbData,buf,myxlen);

				*lpcbData	= myxlen;
			}
		} else {
			if ((UNICONVMASK & (1<<(*lpdwType))) && lpcbData)
				*lpcbData	= myxlen/2;
		}
	} else {
		if ((UNICONVMASK & (1<<(*lpdwType))) && lpcbData)
			*lpcbData	= myxlen/2;
	}
	if (buf)
		free(buf);
	return ret;
}

/* RegQueryValueEx		[KERNEL.225] */
WINAPI DWORD
RegQueryValueEx(
	HKEY	hkey,
	LPSTR	lpszValueName,
	LPDWORD	lpdwReserved,
	LPDWORD	lpdwType,
	LPBYTE	lpbData,
	LPDWORD	lpcbData
) {
	dprintf_reg(stddeb,"RegQueryValueEx(%lx,%s,%p,%p,%p,%p)\n",
		hkey,lpszValueName,lpdwReserved,lpdwType,lpbData,lpcbData
	);
	return RegQueryValueExA(	
		hkey,
		lpszValueName,
		lpdwReserved,
		lpdwType,
		lpbData,
		lpcbData
	);
}

/* RegQueryValueA		[ADVAPI32.156] */
WINAPI DWORD
RegQueryValueA(
	HKEY	hkey,
	LPSTR	lpszSubKey,
	LPSTR	lpszData,
	LPDWORD	lpcbData
) {
	HKEY	xhkey;
	DWORD	ret,lpdwType;

	dprintf_reg(stddeb,"RegQueryValueA(%lx,%s,%p,%p)\n",
		hkey,lpszSubKey,lpszData,lpcbData
	);

	/* only open subkey, if we really do descend */
	if (lpszSubKey && *lpszSubKey) {
		ret	= RegOpenKey(hkey,lpszSubKey,&xhkey);
		if (ret!=ERROR_SUCCESS)
			return ret;
	} else
		xhkey	= hkey;

	lpdwType	= REG_SZ;
	ret	= RegQueryValueExA(
		xhkey,
		NULL,		/* lpszValueName NULL -> compat */
		NULL,		/* lpdwReserved, must be NULL */
		&lpdwType,
		(LPBYTE)lpszData,
		lpcbData
	);
	if (xhkey!=hkey)
		RegCloseKey(xhkey);
	return ret;
}

/* RegQueryValue		[SHELL.6] [KERNEL.224] */
WINAPI DWORD
RegQueryValue(
	HKEY	hkey,
	LPSTR	lpszSubKey,
	LPSTR	lpszData,
	LPDWORD	lpcbData
) {
	dprintf_reg(stddeb,"RegQueryValueA(%lx,%s,%p,%p)\n",
		hkey,lpszSubKey,lpszData,lpcbData
	);
	return RegQueryValueA(hkey,lpszSubKey,lpszData,lpcbData);
}

/*
 * Setting values of Registry keys
 *
 * Callpath:
 * RegSetValue -> RegSetValueA -> RegSetValueExA \
 *                                RegSetValueW   -> RegSetValueExW
 */

/* RegSetValueExW		[ADVAPI32.170] */
WINAPI DWORD
RegSetValueExW(
	HKEY	hkey,
	LPWSTR	lpszValueName,
	DWORD	dwReserved,
	DWORD	dwType,
	LPBYTE	lpbData,
	DWORD	cbData
) {
	LPKEYSTRUCT	lpkey;
	int		i;

	dprintf_reg(stddeb,"RegSetValueExW(%lx,%s,%ld,%ld,%p,%ld)\n",
		hkey,W2C(lpszValueName,0),dwReserved,dwType,lpbData,cbData
	);
	/* we no longer care about the lpbData dwType here... */
	lpkey	= lookup_hkey(hkey);
	if (!lpkey)
		return SHELL_ERROR_BADKEY;
	if (lpszValueName==NULL) {
		for (i=0;i<lpkey->nrofvalues;i++)
			if (lpkey->values[i].name==NULL)
				break;
	} else {
		for (i=0;i<lpkey->nrofvalues;i++)
			if (!strcmpW(lpszValueName,lpkey->values[i].name))
				break;
	}
	if (i==lpkey->nrofvalues) {
		lpkey->values = (LPKEYVALUE)xrealloc(
					lpkey->values,
					(lpkey->nrofvalues+1)*sizeof(KEYVALUE)
				);
		lpkey->nrofvalues++;
		memset(lpkey->values+i,'\0',sizeof(KEYVALUE));
	}
	if (lpkey->values[i].name==NULL)
		if (lpszValueName)
			lpkey->values[i].name = strdupW(lpszValueName);
		else
			lpkey->values[i].name = NULL;
	lpkey->values[i].len	= cbData;
	lpkey->values[i].type	= dwType;
	if (lpkey->values[i].data !=NULL)
		free(lpkey->values[i].data);
	lpkey->values[i].data	= (LPBYTE)xmalloc(cbData);
	lpkey->values[i].lastmodified = time(NULL);
	memcpy(lpkey->values[i].data,lpbData,cbData);
	return SHELL_ERROR_SUCCESS;
}

/* RegSetValueExA		[ADVAPI32.169] */
WINAPI DWORD
RegSetValueExA(
	HKEY	hkey,
	LPSTR	lpszValueName,
	DWORD	dwReserved,
	DWORD	dwType,
	LPBYTE	lpbData,
	DWORD	cbData
) {
	LPBYTE	buf;
	LPWSTR	lpszValueNameW;
	DWORD	ret;

	dprintf_reg(stddeb,"RegSetValueExA(%lx,%s,%ld,%ld,%p,%ld)\n->",
		hkey,lpszValueName,dwReserved,dwType,lpbData,cbData
	);
	if ((1<<dwType) & UNICONVMASK) {
		buf=(LPBYTE)strdupA2W(lpbData);
		cbData=2*strlen(lpbData)+2;
	} else
		buf=lpbData;
	if (lpszValueName)
		lpszValueNameW = strdupA2W(lpszValueName);
	else
		lpszValueNameW = NULL;
	ret=RegSetValueExW(hkey,lpszValueNameW,dwReserved,dwType,buf,cbData);
	if (lpszValueNameW)
		free(lpszValueNameW);
	if (buf!=lpbData)
		free(buf);
	return ret;
}

/* RegSetValueEx		[KERNEL.226] */
WINAPI DWORD
RegSetValueEx(
	HKEY	hkey,
	LPSTR	lpszValueName,
	DWORD	dwReserved,
	DWORD	dwType,
	LPBYTE	lpbData,
	DWORD	cbData
) {
	dprintf_reg(stddeb,"RegSetValueEx(%lx,%s,%ld,%ld,%p,%ld)\n->",
		hkey,lpszValueName,dwReserved,dwType,lpbData,cbData
	);
	return RegSetValueExA(hkey,lpszValueName,dwReserved,dwType,lpbData,cbData);
}

/* RegSetValueW			[ADVAPI32.171] */
WINAPI DWORD
RegSetValueW(
	HKEY	hkey,
	LPCWSTR	lpszSubKey,
	DWORD	dwType,
	LPCWSTR	lpszData,
	DWORD	cbData
) {
	HKEY	xhkey;
	DWORD	ret;

	dprintf_reg(stddeb,"RegSetValueW(%lx,%s,%ld,%s,%ld)\n->",
		hkey,W2C(lpszSubKey,0),dwType,W2C(lpszData,0),cbData
	);
	if (lpszSubKey && *lpszSubKey) {
		ret=RegCreateKeyW(hkey,lpszSubKey,&xhkey);
		if (ret!=ERROR_SUCCESS)
			return ret;
	} else
		xhkey=hkey;
	if (dwType!=REG_SZ) {
		fprintf(stddeb,"RegSetValueX called with dwType=%ld!\n",dwType);
		dwType=REG_SZ;
	}
	if (cbData!=2*strlenW(lpszData)+2) {
		dprintf_reg(stddeb,"RegSetValueX called with len=%ld != strlen(%s)+1=%d!\n",
			cbData,W2C(lpszData,0),2*strlenW(lpszData)+2
		);
		cbData=2*strlenW(lpszData)+2;
	}
	ret=RegSetValueExW(xhkey,NULL,0,dwType,(LPBYTE)lpszData,cbData);
	if (hkey!=xhkey)
		RegCloseKey(xhkey);
	return ret;

}
/* RegSetValueA			[ADVAPI32.168] */
WINAPI DWORD
RegSetValueA(
	HKEY	hkey,
	LPCSTR	lpszSubKey,
	DWORD	dwType,
	LPCSTR	lpszData,
	DWORD	cbData
) {
	DWORD	ret;
	HKEY	xhkey;

	dprintf_reg(stddeb,"RegSetValueA(%lx,%s,%ld,%s,%ld)\n->",
		hkey,lpszSubKey,dwType,lpszData,cbData
	);
	if (lpszSubKey && *lpszSubKey) {
		ret=RegCreateKey(hkey,lpszSubKey,&xhkey);
		if (ret!=ERROR_SUCCESS)
			return ret;
	} else
		xhkey=hkey;

	if (dwType!=REG_SZ) {
		dprintf_reg(stddeb,"RegSetValueA called with dwType=%ld!\n",dwType);
		dwType=REG_SZ;
	}
	if (cbData!=strlen(lpszData)+1)
		cbData=strlen(lpszData)+1;
	ret=RegSetValueExA(xhkey,NULL,0,dwType,(LPBYTE)lpszData,cbData);
	if (xhkey!=hkey)
		RegCloseKey(xhkey);
	return ret;
}

/* RegSetValue			[KERNEL.221] [SHELL.5] */
WINAPI DWORD
RegSetValue(
	HKEY	hkey,
	LPCSTR	lpszSubKey,
	DWORD	dwType,
	LPCSTR	lpszData,
	DWORD	cbData
) {
	DWORD	ret;
	dprintf_reg(stddeb,"RegSetValue(%lx,%s,%ld,%s,%ld)\n->",
		hkey,lpszSubKey,dwType,lpszData,cbData
	);
	ret=RegSetValueA(hkey,lpszSubKey,dwType,lpszData,cbData);
	return ret;
}

/* 
 * Key Enumeration
 *
 * Callpath:
 * RegEnumKey -> RegEnumKeyA -> RegEnumKeyExA \
 *                              RegEnumKeyW   -> RegEnumKeyExW
 */

/* RegEnumKeyExW		[ADVAPI32.139] */
WINAPI DWORD
RegEnumKeyExW(
	HKEY	hkey,
	DWORD	iSubkey,
	LPWSTR	lpszName,
	LPDWORD	lpcchName,
	LPDWORD	lpdwReserved,
	LPWSTR	lpszClass,
	LPDWORD	lpcchClass,
	FILETIME	*ft
) {
	LPKEYSTRUCT	lpkey,lpxkey;

	dprintf_reg(stddeb,"RegEnumKeyExW(%lx,%ld,%p,%ld,%p,%p,%p,%p)\n",
		hkey,iSubkey,lpszName,*lpcchName,lpdwReserved,lpszClass,lpcchClass,ft
	);
	lpkey=lookup_hkey(hkey);
	if (!lpkey)
		return SHELL_ERROR_BADKEY;
	if (!lpkey->nextsub)
		return ERROR_NO_MORE_ITEMS;
	lpxkey=lpkey->nextsub;
	while (iSubkey && lpxkey) {
		iSubkey--;
		lpxkey=lpxkey->next;
	}
	if (iSubkey || !lpxkey)
		return ERROR_NO_MORE_ITEMS;
	if (2*strlenW(lpxkey->keyname)+2>*lpcchName)
		return ERROR_MORE_DATA;
	memcpy(lpszName,lpxkey->keyname,strlenW(lpxkey->keyname)*2+2);
	if (lpszClass) {
		/* what should we write into it? */
		*lpszClass		= 0;
		*lpcchClass	= 2;
	}
	return ERROR_SUCCESS;

}

/* RegEnumKeyW			[ADVAPI32.140] */
WINAPI DWORD
RegEnumKeyW(
	HKEY	hkey,
	DWORD	iSubkey,
	LPWSTR	lpszName,
	DWORD	lpcchName
) {
	FILETIME	ft;

	dprintf_reg(stddeb,"RegEnumKeyW(%lx,%ld,%p,%ld)\n->",
		hkey,iSubkey,lpszName,lpcchName
	);
	return RegEnumKeyExW(hkey,iSubkey,lpszName,&lpcchName,NULL,NULL,NULL,&ft);
}
/* RegEnumKeyExA		[ADVAPI32.138] */
WINAPI DWORD
RegEnumKeyExA(
	HKEY	hkey,
	DWORD	iSubkey,
	LPSTR	lpszName,
	LPDWORD	lpcchName,
	LPDWORD	lpdwReserved,
	LPSTR	lpszClass,
	LPDWORD	lpcchClass,
	FILETIME	*ft
) {
	DWORD	ret,lpcchNameW,lpcchClassW;
	LPWSTR	lpszNameW,lpszClassW;


	dprintf_reg(stddeb,"RegEnumKeyExA(%lx,%ld,%p,%ld,%p,%p,%p,%p)\n->",
		hkey,iSubkey,lpszName,*lpcchName,lpdwReserved,lpszClass,lpcchClass,ft
	);
	if (lpszName) {
		lpszNameW	= (LPWSTR)xmalloc(*lpcchName*2);
		lpcchNameW	= *lpcchName*2;
	} else {
		lpszNameW	= NULL;
		lpcchNameW 	= 0;
	}
	if (lpszClass) {
		lpszClassW		= (LPWSTR)xmalloc(*lpcchClass*2);
		lpcchClassW	= *lpcchClass*2;
	} else {
		lpszClassW	=0;
		lpcchClassW=0;
	}
	ret=RegEnumKeyExW(
		hkey,
		iSubkey,
		lpszNameW,
		&lpcchNameW,
		lpdwReserved,
		lpszClassW,
		&lpcchClassW,
		ft
	);
	if (ret==ERROR_SUCCESS) {
		strcpyWA(lpszName,lpszNameW);
		*lpcchName=strlen(lpszName);
		if (lpszClassW) {
			strcpyWA(lpszClass,lpszClassW);
			*lpcchClass=strlen(lpszClass);
		}
	}
	if (lpszNameW)
		free(lpszNameW);
	if (lpszClassW)
		free(lpszClassW);
	return ret;
}

/* RegEnumKeyA			[ADVAPI32.137] */
WINAPI DWORD
RegEnumKeyA(
	HKEY	hkey,
	DWORD	iSubkey,
	LPSTR	lpszName,
	DWORD	lpcchName
) {
	FILETIME	ft;

	dprintf_reg(stddeb,"RegEnumKeyA(%lx,%ld,%p,%ld)\n->",
		hkey,iSubkey,lpszName,lpcchName
	);
	return	RegEnumKeyExA(
		hkey,
		iSubkey,
		lpszName,
		&lpcchName,
		NULL,
		NULL,
		NULL,
		&ft
	);
}

/* RegEnumKey			[SHELL.7] [KERNEL.216] */
WINAPI DWORD
RegEnumKey(
	HKEY	hkey,
	DWORD	iSubkey,
	LPSTR	lpszName,
	DWORD	lpcchName
) {
	dprintf_reg(stddeb,"RegEnumKey(%lx,%ld,%p,%ld)\n->",
		hkey,iSubkey,lpszName,lpcchName
	);
	return RegEnumKeyA(hkey,iSubkey,lpszName,lpcchName);
}

/* 
 * Enumerate Registry Values
 *
 * Callpath:
 * RegEnumValue -> RegEnumValueA -> RegEnumValueW
 */

/* RegEnumValueW		[ADVAPI32.142] */
WINAPI DWORD
RegEnumValueW(
	HKEY	hkey,
	DWORD	iValue,
	LPWSTR	lpszValue,
	LPDWORD	lpcchValue,
	LPDWORD	lpdReserved,
	LPDWORD	lpdwType,
	LPBYTE	lpbData,
	LPDWORD	lpcbData
) {
	LPKEYSTRUCT	lpkey;
	LPKEYVALUE	val;

	dprintf_reg(stddeb,"RegEnumValueW(%ld,%ld,%p,%p,%p,%p,%p,%p)\n",
		hkey,iValue,lpszValue,lpcchValue,lpdReserved,lpdwType,lpbData,lpcbData
	);
	lpkey = lookup_hkey(hkey);
	if (!lpkey)
		return SHELL_ERROR_BADKEY;
	if (lpkey->nrofvalues<=iValue)
		return ERROR_NO_MORE_ITEMS;
	val	= lpkey->values+iValue;

	if (val->name) {
		if (strlenW(val->name)*2+2>*lpcchValue) {
			*lpcchValue = strlenW(val->name)*2+2;
			return ERROR_MORE_DATA;
		}
		memcpy(lpszValue,val->name,2*strlenW(val->name)+2);
		*lpcchValue=strlenW(val->name)*2+2;
	} else {
		/* how to handle NULL value? */
		*lpszValue	= 0;
		*lpcchValue	= 2;
	}
	*lpdwType=val->type;
	if (lpbData) {
		if (val->len>*lpcbData)
			return ERROR_MORE_DATA;
		memcpy(lpbData,val->data,val->len);
		*lpcbData = val->len;
	}
	return SHELL_ERROR_SUCCESS;
}

/* RegEnumValueA		[ADVAPI32.141] */
WINAPI DWORD
RegEnumValueA(
	HKEY	hkey,
	DWORD	iValue,
	LPSTR	lpszValue,
	LPDWORD	lpcchValue,
	LPDWORD	lpdReserved,
	LPDWORD	lpdwType,
	LPBYTE	lpbData,
	LPDWORD	lpcbData
) {
	LPWSTR	lpszValueW;
	LPBYTE	lpbDataW;
	DWORD	ret,lpcbDataW;

	dprintf_reg(stddeb,"RegEnumValueA(%ld,%ld,%p,%p,%p,%p,%p,%p)\n",
		hkey,iValue,lpszValue,lpcchValue,lpdReserved,lpdwType,lpbData,lpcbData
	);

	lpszValueW = (LPWSTR)xmalloc(*lpcchValue*2);
	if (lpbData) {
		lpbDataW = (LPBYTE)xmalloc(*lpcbData*2);
		lpcbDataW = *lpcbData*2;
	} else
		lpbDataW = NULL;
	ret=RegEnumValueW(
		hkey,
		iValue,
		lpszValueW,
		lpcchValue,
		lpdReserved,
		lpdwType,
		lpbDataW,
		&lpcbDataW
	);

	if (ret==ERROR_SUCCESS) {
		strcpyWA(lpszValue,lpszValueW);
		if (lpbData) {
			if ((1<<*lpdwType) & UNICONVMASK) {
				strcpyWA(lpbData,(LPWSTR)lpbDataW);
			} else {
				if (lpcbDataW > *lpcbData)
					ret	= ERROR_MORE_DATA;
				else
					memcpy(lpbData,lpbDataW,lpcbDataW);
			}
			*lpcbData = lpcbDataW;
		}
	}
	if (lpbDataW)
		free(lpbDataW);
	if (lpszValueW)
		free(lpszValueW);
	return ret;
}

/* RegEnumValue			[KERNEL.223] */
WINAPI DWORD
RegEnumValue(
	HKEY	hkey,
	DWORD	iValue,
	LPSTR	lpszValue,
	LPDWORD	lpcchValue,
	LPDWORD	lpdReserved,
	LPDWORD	lpdwType,
	LPBYTE	lpbData,
	LPDWORD	lpcbData
) {
	dprintf_reg(stddeb,"RegEnumValue(%ld,%ld,%p,%p,%p,%p,%p,%p)\n",
		hkey,iValue,lpszValue,lpcchValue,lpdReserved,lpdwType,lpbData,lpcbData
	);
	return RegEnumValueA(
		hkey,
		iValue,
		lpszValue,
		lpcchValue,
		lpdReserved,
		lpdwType,
		lpbData,
		lpcbData
	);
}

/* 
 *  Close registry key
 */
/* RegCloseKey			[SHELL.3] [KERNEL.220] [ADVAPI32.126] */
WINAPI DWORD
RegCloseKey(HKEY hkey) {
	dprintf_reg(stddeb,"RegCloseKey(%ld)\n",hkey);
	remove_handle(hkey);
	return ERROR_SUCCESS;
}
/* 
 * Delete registry key
 *
 * Callpath:
 * RegDeleteKey -> RegDeleteKeyA -> RegDeleteKeyW
 */
/* RegDeleteKeyW		[ADVAPI32.134] */
WINAPI DWORD
RegDeleteKeyW(HKEY hkey,LPWSTR lpszSubKey) {
	LPKEYSTRUCT	*lplpPrevKey,lpNextKey,lpxkey;
	LPWSTR		*wps;
	int		wpc,i;

	dprintf_reg(stddeb,"RegDeleteKeyW(%ld,%s)\n",
		hkey,W2C(lpszSubKey,0)
	);
	lpNextKey	= lookup_hkey(hkey);
	if (!lpNextKey)
		return SHELL_ERROR_BADKEY;
	/* we need to know the previous key in the hier. */
	if (!lpszSubKey || !*lpszSubKey)
		return SHELL_ERROR_BADKEY;
	split_keypath(lpszSubKey,&wps,&wpc);
	i 	= 0;
	lpxkey	= lpNextKey;
	while (i<wpc-1) {
		lpxkey=lpNextKey->nextsub;
		while (lpxkey) {
			if (!strcmpW(wps[i],lpxkey->keyname))
				break;
			lpxkey=lpxkey->next;
		}
		if (!lpxkey) {
			FREE_KEY_PATH;
			/* not found is success */
			return SHELL_ERROR_SUCCESS;
		}
		i++;
		lpNextKey	= lpxkey;
	}
	lpxkey	= lpNextKey->nextsub;
	lplpPrevKey = &(lpNextKey->nextsub);
	while (lpxkey) {
		if (!strcmpW(wps[i],lpxkey->keyname))
			break;
		lplpPrevKey	= &(lpxkey->next);
		lpxkey		= lpxkey->next;
	}
	if (!lpxkey)
		return SHELL_ERROR_SUCCESS;
	if (lpxkey->nextsub)
		return SHELL_ERROR_CANTWRITE;
	*lplpPrevKey	= lpxkey->next;
	free(lpxkey->keyname);
	if (lpxkey->class)
		free(lpxkey->class);
	if (lpxkey->values)
		free(lpxkey->values);
	free(lpxkey);
	FREE_KEY_PATH;
	return	SHELL_ERROR_SUCCESS;
}

/* RegDeleteKeyA		[ADVAPI32.133] */
WINAPI DWORD
RegDeleteKeyA(HKEY hkey,LPCSTR lpszSubKey) {
	LPWSTR	lpszSubKeyW;
	DWORD	ret;

	dprintf_reg(stddeb,"RegDeleteKeyA(%ld,%s)\n",
		hkey,lpszSubKey
	);
	lpszSubKeyW=strdupA2W(lpszSubKey);
	ret=RegDeleteKeyW(hkey,lpszSubKeyW);
	free(lpszSubKeyW);
	return ret;
}

/* RegDeleteKey			[SHELL.4] [KERNEL.219] */
WINAPI DWORD
RegDeleteKey(HKEY hkey,LPCSTR lpszSubKey) {
	dprintf_reg(stddeb,"RegDeleteKey(%ld,%s)\n",
		hkey,lpszSubKey
	);
	return RegDeleteKeyA(hkey,lpszSubKey);
}

/* 
 * Delete registry value
 *
 * Callpath:
 * RegDeleteValue -> RegDeleteValueA -> RegDeleteValueW
 */
/* RegDeleteValueW		[ADVAPI32.136] */
WINAPI DWORD
RegDeleteValueW(HKEY hkey,LPWSTR lpszValue) {
	DWORD		i;
	LPKEYSTRUCT	lpkey;
	LPKEYVALUE	val;

	dprintf_reg(stddeb,"RegDeleteValueW(%ld,%s)\n",
		hkey,W2C(lpszValue,0)
	);
	lpkey=lookup_hkey(hkey);
	if (!lpkey)
		return SHELL_ERROR_BADKEY;
	if (lpszValue) {
		for (i=0;i<lpkey->nrofvalues;i++)
			if (!strcmpW(lpkey->values[i].name,lpszValue))
				break;
	} else {
		for (i=0;i<lpkey->nrofvalues;i++)
			if (lpkey->values[i].name==NULL)
				break;
	}
	if (i==lpkey->nrofvalues)
		return SHELL_ERROR_BADKEY;/*FIXME: correct errorcode? */
	val	= lpkey->values+i;
	if (val->name) free(val->name);
	if (val->data) free(val->data);
	memcpy(	
		lpkey->values+i,
		lpkey->values+i+1,
		sizeof(KEYVALUE)*(lpkey->nrofvalues-i-1)
	);
	lpkey->values	= (LPKEYVALUE)xrealloc(
				lpkey->values,
				(lpkey->nrofvalues-1)*sizeof(KEYVALUE)
			);
	lpkey->nrofvalues--;
	return SHELL_ERROR_SUCCESS;
}

/* RegDeleteValueA		[ADVAPI32.135] */
WINAPI DWORD
RegDeleteValueA(HKEY hkey,LPSTR lpszValue) {
	LPWSTR	lpszValueW;
	DWORD	ret;

	dprintf_reg(stddeb,"RegDeleteValueA(%ld,%s)\n",
		hkey,lpszValue
	);
	if (lpszValue)
		lpszValueW=strdupA2W(lpszValue);
	else
		lpszValueW=NULL;
	ret=RegDeleteValueW(hkey,lpszValueW);
	if (lpszValueW)
		free(lpszValueW);
	return ret;
}

/* RegDeleteValue		[KERNEL.222] */
WINAPI DWORD
RegDeleteValue(HKEY hkey,LPSTR lpszValue) {
	dprintf_reg(stddeb,"RegDeleteValue(%ld,%s)\n",
		hkey,lpszValue
	);
	return RegDeleteValueA(hkey,lpszValue);
}

/* RegFlushKey			[ADVAPI32.143] [KERNEL.227] */
WINAPI DWORD
RegFlushKey(HKEY hkey) {
	dprintf_reg(stddeb,"RegFlushKey(%ld), STUB.\n",hkey);
	return SHELL_ERROR_SUCCESS;
}

/* FIXME: lpcchXXXX ... is this counting in WCHARS or in BYTEs ?? */

/* RegQueryInfoKeyW		[ADVAPI32.153] */
WINAPI DWORD
RegQueryInfoKeyW(
	HKEY	hkey,
	LPWSTR	lpszClass,
	LPDWORD	lpcchClass,
	LPDWORD	lpdwReserved,
	LPDWORD	lpcSubKeys,
	LPDWORD	lpcchMaxSubkey,
	LPDWORD	lpcchMaxClass,
	LPDWORD	lpcValues,
	LPDWORD	lpcchMaxValueName,
	LPDWORD	lpccbMaxValueData,
	LPDWORD	lpcbSecurityDescriptor,
	FILETIME	*ft
) {
	LPKEYSTRUCT	lpkey,lpxkey;
	int		nrofkeys,maxsubkey,maxclass,maxvalues,maxvname,maxvdata;
	int		i;

	dprintf_reg(stddeb,"RegQueryInfoKeyW(%lx,......)\n",hkey);
	lpkey=lookup_hkey(hkey);
	if (!lpkey)
		return SHELL_ERROR_BADKEY;
	if (lpszClass) {
		if (lpkey->class) {
			if (strlenW(lpkey->class)*2+2>*lpcchClass) {
				*lpcchClass=strlenW(lpkey->class)*2;
				return ERROR_MORE_DATA;
			}
			*lpcchClass=strlenW(lpkey->class)*2;
			memcpy(lpszClass,lpkey->class,strlenW(lpkey->class));
		} else {
			*lpszClass	= 0;
			*lpcchClass	= 0;
		}
	} else {
		if (lpcchClass)
			*lpcchClass	= strlenW(lpkey->class)*2;
	}
	lpxkey=lpkey->nextsub;
	nrofkeys=maxsubkey=maxclass=maxvalues=maxvname=maxvdata=0;
	while (lpxkey) {
		nrofkeys++;
		if (strlenW(lpxkey->keyname)>maxsubkey)
			maxsubkey=strlenW(lpxkey->keyname);
		if (lpxkey->class && strlenW(lpxkey->class)>maxclass)
			maxclass=strlenW(lpxkey->class);
		if (lpxkey->nrofvalues>maxvalues)
			maxvalues=lpxkey->nrofvalues;
		for (i=0;i<lpxkey->nrofvalues;i++) {
			LPKEYVALUE	val=lpxkey->values+i;

			if (val->name && strlenW(val->name)>maxvname)
				maxvname=strlenW(val->name);
			if (val->len>maxvdata)
				maxvdata=val->len;
		}
		lpxkey=lpxkey->next;
	}
	if (!maxclass) maxclass	= 1;
	if (!maxvname) maxvname	= 1;
	if (lpcSubKeys)
		*lpcSubKeys	= nrofkeys;
	if (lpcchMaxSubkey)
		*lpcchMaxSubkey	= maxsubkey*2;
	if (lpcchMaxClass)
		*lpcchMaxClass	= maxclass*2;
	if (lpcValues)
		*lpcValues	= maxvalues;
	if (lpcchMaxValueName)
		*lpcchMaxValueName= maxvname;
	if (lpccbMaxValueData)
		*lpccbMaxValueData= maxvdata;
	return SHELL_ERROR_SUCCESS;
}

/* RegQueryInfoKeyA		[ADVAPI32.152] */
WINAPI DWORD
RegQueryInfoKeyA(
	HKEY	hkey,
	LPSTR	lpszClass,
	LPDWORD	lpcchClass,
	LPDWORD	lpdwReserved,
	LPDWORD	lpcSubKeys,
	LPDWORD	lpcchMaxSubkey,
	LPDWORD	lpcchMaxClass,
	LPDWORD	lpcValues,
	LPDWORD	lpcchMaxValueName,
	LPDWORD	lpccbMaxValueData,
	LPDWORD	lpcbSecurityDescriptor,
	FILETIME	*ft
) {
	LPWSTR		lpszClassW;
	DWORD		ret;

	dprintf_reg(stddeb,"RegQueryInfoKeyA(%lx,......)\n",hkey);
	if (lpszClass) {
		*lpcchClass*= 2;
		lpszClassW  = (LPWSTR)xmalloc(*lpcchClass);

	} else
		lpszClassW  = NULL;
	ret=RegQueryInfoKeyW(
		hkey,
		lpszClassW,
		lpcchClass,
		lpdwReserved,
		lpcSubKeys,
		lpcchMaxSubkey,
		lpcchMaxClass,
		lpcValues,
		lpcchMaxValueName,
		lpccbMaxValueData,
		lpcbSecurityDescriptor,
		ft
	);
	if (ret==ERROR_SUCCESS)
		strcpyWA(lpszClass,lpszClassW);
	if (lpcchClass)
		*lpcchClass/=2;
	if (lpcchMaxSubkey)
		*lpcchMaxSubkey/=2;
	if (lpcchMaxClass)
		*lpcchMaxClass/=2;
	if (lpcchMaxValueName)
		*lpcchMaxValueName/=2;
	if (lpszClassW)
		free(lpszClassW);
	return ret;
}
