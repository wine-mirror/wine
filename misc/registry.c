/*
 * 	Registry Functions
 *
 * Copyright 1996 Marcus Meissner
 * Copyright 1998 Matthew Becker
 * Copyright 1999 Sylvain St-Germain
 *
 * December 21, 1997 - Kevin Cozens
 * Fixed bugs in the _w95_loadreg() function. Added extra information
 * regarding the format of the Windows '95 registry files.
 *
 * NOTES
 *    When changing this file, please re-run the regtest program to ensure
 *    the conditions are handled properly.
 *
 * TODO
 *    Security access
 *    Option handling
 *    Time for RegEnumKey*, RegQueryInfoKey*
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#ifdef HAVE_SYS_ERRNO_H
#include <sys/errno.h>
#endif
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <assert.h>
#include <time.h>
#include "windef.h"
#include "winbase.h"
#include "wine/winbase16.h"
#include "wine/winestring.h"
#include "winerror.h"
#include "file.h"
#include "heap.h"
#include "debugtools.h"
#include "xmalloc.h"
#include "options.h"
#include "winreg.h"
#include "server.h"
#include "services.h"

DEFAULT_DEBUG_CHANNEL(reg)

static void REGISTRY_Init(void);
/* FIXME: following defines should be configured global ... */

/* NOTE: do not append a /. linux' mkdir() WILL FAIL if you do that */
#define WINE_PREFIX                 "/.wine"
#define SAVE_USERS_DEFAULT          ETCDIR"/wine.userreg"
#define SAVE_LOCAL_MACHINE_DEFAULT  ETCDIR"/wine.systemreg"

/* relative in ~user/.wine/ : */
#define SAVE_CURRENT_USER           "user.reg"
#define SAVE_LOCAL_USERS_DEFAULT    "wine.userreg"
#define SAVE_LOCAL_MACHINE          "system.reg"

#define KEY_REGISTRY                "Software\\The WINE team\\WINE\\Registry"
#define VAL_SAVEUPDATED             "SaveOnlyUpdatedKeys"


/* what valuetypes do we need to convert? */
#define UNICONVMASK	((1<<REG_SZ)|(1<<REG_MULTI_SZ)|(1<<REG_EXPAND_SZ))



/*
 * QUESTION
 *   Are these doing the same as HEAP_strdupAtoW and HEAP_strdupWtoA?
 *   If so, can we remove them?
 * ANSWER
 *   No, the memory handling functions are called very often in here, 
 *   just replacing them by HeapAlloc(SystemHeap,...) makes registry
 *   loading 100 times slower. -MM
 */
static LPWSTR strdupA2W(LPCSTR src)
{
    if(src) {
	LPWSTR dest=xmalloc(2*strlen(src)+2);
	lstrcpyAtoW(dest,src);
	return dest;
    }
    return NULL;
}

LPWSTR strcvtA2W(LPCSTR src, int nchars)

{
   LPWSTR dest = xmalloc (2 * nchars + 2);

   lstrcpynAtoW(dest,src,nchars+1);
   dest[nchars] = 0;
   return dest;
}



/******************************************************************************
 * REGISTRY_Init [Internal]
 * Registry initialisation, allocates some default keys. 
 */
static void REGISTRY_Init(void) {
	HKEY	hkey;
	char	buf[200];

	TRACE("(void)\n");

	RegCreateKeyA(HKEY_DYN_DATA,"PerfStats\\StatData",&hkey);
	RegCloseKey(hkey);

        /* This was an Open, but since it is called before the real registries
           are loaded, it was changed to a Create - MTB 980507*/
	RegCreateKeyA(HKEY_LOCAL_MACHINE,"HARDWARE\\DESCRIPTION\\System",&hkey);
	RegSetValueExA(hkey,"Identifier",0,REG_SZ,"SystemType WINE",strlen("SystemType WINE"));
	RegCloseKey(hkey);

	/* \\SOFTWARE\\Microsoft\\Window NT\\CurrentVersion
	 *						CurrentVersion
	 *						CurrentBuildNumber
	 *						CurrentType
	 *					string	RegisteredOwner
	 *					string	RegisteredOrganization
	 *
	 */
	/* System\\CurrentControlSet\\Services\\SNMP\\Parameters\\RFC1156Agent
	 * 					string	SysContact
	 * 					string	SysLocation
	 * 						SysServices
	 */
	if (-1!=gethostname(buf,200)) {
		RegCreateKeyA(HKEY_LOCAL_MACHINE,"System\\CurrentControlSet\\Control\\ComputerName\\ComputerName",&hkey);
		RegSetValueExA(hkey,"ComputerName",0,REG_SZ,buf,strlen(buf)+1);
		RegCloseKey(hkey);
	}
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
 *
 * [HKEY_CURRENT_USER\\Software\\The WINE team\\WINE\\Registry]
 * SaveOnlyUpdatedKeys=yes
 */

/* Same as RegSaveKey but with Unix pathnames */
static void save_key( HKEY hkey, const char *filename )
{
    struct save_registry_request *req = get_req_buffer();
    int count = 0;
    DWORD ret;
    HANDLE handle;
    char *p;
    char *name = HeapAlloc( GetProcessHeap(), 0, strlen(filename) + 20 );

    if (!name) return;
    strcpy( name, filename );
    if ((p = strrchr( name, '/' ))) p++;
    else p = name;

    for (;;)
    {
        sprintf( p, "reg%04x.tmp", count++ );
        handle = FILE_CreateFile( name, GENERIC_WRITE, 0, NULL,
                                  CREATE_NEW, FILE_ATTRIBUTE_NORMAL, -1 );
        if (handle != INVALID_HANDLE_VALUE) break;
        if ((ret = GetLastError()) != ERROR_FILE_EXISTS) break;
    }

    if (handle != INVALID_HANDLE_VALUE)
    {
        req->hkey = hkey;
        req->file = handle;
        ret = server_call_noerr( REQ_SAVE_REGISTRY );
        CloseHandle( handle );
        if (ret) unlink( name );
        else if (rename( name, filename ) == -1)
        {
            ERR( "Failed to move %s to %s: ", name, filename );
            perror( "rename" );
            unlink( name );
        }
    }
    HeapFree( GetProcessHeap(), 0, name );
}


/******************************************************************************
 * SHELL_SaveRegistryBranch [Internal]
 *
 * Saves main registry branch specified by hkey.
 */
static void SHELL_SaveRegistryBranch(HKEY hkey)
{
    char   *fn, *home;

    /* Find out what to save to, get from config file */
    BOOL writeToHome = PROFILE_GetWineIniBool("registry","WritetoHomeRegistries",1);
    BOOL writeToAlt = PROFILE_GetWineIniBool("registry","WritetoAltRegistries",1);

    /* FIXME: does this check apply to all keys written below ? */
    if (!(home = getenv( "HOME" )))
        ERR("Failed to get homedirectory of UID %ld.\n",(long) getuid());

    /* HKEY_LOCAL_MACHINE contains the HKEY_CLASSES_ROOT branch */
    if (hkey == HKEY_CLASSES_ROOT) hkey = HKEY_LOCAL_MACHINE;

    switch (hkey)
    {
    case HKEY_CURRENT_USER:
        fn = xmalloc( MAX_PATHNAME_LEN ); 
        if (writeToAlt && PROFILE_GetWineIniString( "registry", "AltCurrentUserFile", "",
                                                    fn, MAX_PATHNAME_LEN - 1))
            save_key( HKEY_CURRENT_USER, fn );
        free (fn);

        if (home && writeToHome)
        {
            fn=(char*)xmalloc( strlen(home) + strlen(WINE_PREFIX) +
                               strlen(SAVE_CURRENT_USER) + 2 );
            strcpy(fn,home);
            strcat(fn,WINE_PREFIX);
  
            /* create the directory. don't care about errorcodes. */
            mkdir(fn,0755); /* drwxr-xr-x */
            strcat(fn,"/"SAVE_CURRENT_USER);
            save_key( HKEY_CURRENT_USER, fn );
            free(fn);
        }
        break;
    case HKEY_LOCAL_MACHINE:
        /* Try first saving according to the defined location in .winerc */
        fn = xmalloc ( MAX_PATHNAME_LEN);
        if (writeToAlt && PROFILE_GetWineIniString( "Registry", "AltLocalMachineFile", "", 
                                                    fn, MAX_PATHNAME_LEN - 1))
            save_key( HKEY_LOCAL_MACHINE, fn );
        free (fn);

        if (home && writeToHome)
        {
            fn=(char*)xmalloc( strlen(home) + strlen(WINE_PREFIX) +
                               strlen(SAVE_LOCAL_MACHINE) + 2);
            strcpy(fn,home);
            strcat(fn,WINE_PREFIX"/"SAVE_LOCAL_MACHINE);
            save_key( HKEY_LOCAL_MACHINE, fn );
            free(fn);
        }
        break;
    case HKEY_USERS:
        fn = xmalloc( MAX_PATHNAME_LEN );
        if (writeToAlt && PROFILE_GetWineIniString( "Registry", "AltUserFile", "", 
                                                    fn, MAX_PATHNAME_LEN - 1))
            save_key( HKEY_USERS, fn );
        free (fn);

        if (home && writeToHome)
        {
            fn=(char*)xmalloc( strlen(home) + strlen(WINE_PREFIX) +
                               strlen(SAVE_LOCAL_USERS_DEFAULT) + 2);
            strcpy(fn,home);
            strcat(fn,WINE_PREFIX"/"SAVE_LOCAL_USERS_DEFAULT);
            save_key( HKEY_USERS, fn );
            free(fn);
        }
        break;
    default:
        ERR("unknown/invalid key handle !\n");
        break;
    }
}


/******************************************************************************
 * SHELL_SaveRegistry [Internal]
 */
void SHELL_SaveRegistry( void )
{
    struct set_registry_levels_request *req = get_req_buffer();
    char   buf[4];
    HKEY   hkey;
    int    all;

    TRACE("(void)\n");

    all=0;
    if (RegOpenKeyA(HKEY_CURRENT_USER,KEY_REGISTRY,&hkey)!=ERROR_SUCCESS) 
    {
        strcpy(buf,"yes");
    } 
    else 
    {
        DWORD len,junk,type;

        len=4;
        if ((ERROR_SUCCESS!=RegQueryValueExA( hkey,
                                              VAL_SAVEUPDATED,
                                              &junk,
                                              &type,
                                              buf,
                                              &len)) || (type!=REG_SZ))
        {
            strcpy(buf,"yes");
        }
        RegCloseKey(hkey);
    }

    if (lstrcmpiA(buf,"yes")) all = 1;

    /* set saving level (0 for saving everything, 1 for saving only modified keys) */
    req->current = 1;
    req->saving  = !all;
    req->version = PROFILE_GetWineIniBool( "registry", "UseNewFormat", 0 ) ? 2 : 1;
    server_call( REQ_SET_REGISTRY_LEVELS );

    SHELL_SaveRegistryBranch(HKEY_CURRENT_USER);
    SHELL_SaveRegistryBranch(HKEY_LOCAL_MACHINE);
    SHELL_SaveRegistryBranch(HKEY_USERS);
}

/* Periodic save callback */
static void CALLBACK periodic_save( ULONG_PTR dummy )
{
    SHELL_SaveRegistry();
}

/************************ LOAD Registry Function ****************************/



/******************************************************************************
 * _find_or_add_key [Internal]
 */
static inline HKEY _find_or_add_key( HKEY hkey, LPWSTR keyname )
{
    HKEY subkey;
    if (RegCreateKeyW( hkey, keyname, &subkey ) != ERROR_SUCCESS) subkey = 0;
    if (keyname) free( keyname );
    return subkey;
}

/******************************************************************************
 * _find_or_add_value [Internal]
 */
static void _find_or_add_value( HKEY hkey, LPWSTR name, DWORD type, LPBYTE data, DWORD len )
{
    RegSetValueExW( hkey, name, 0, type, data, len );
    if (name) free( name );
    if (data) free( data );
}


/******************************************************************************
 * _wine_read_line [Internal]
 *
 * reads a line including dynamically enlarging the readbuffer and throwing
 * away comments
 */
static int _wine_read_line( FILE *F, char **buf, int *len )
{
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


/******************************************************************************
 * _wine_read_USTRING [Internal]
 *
 * converts a char* into a UNICODE string (up to a special char)
 * and returns the position exactly after that string
 */
static char* _wine_read_USTRING( char *buf, LPWSTR *str )
{
	char	*s;
	LPWSTR	ws;

	/* read up to "=" or "\0" or "\n" */
	s	= buf;
	*str	= (LPWSTR)xmalloc(2*strlen(buf)+2);
	ws	= *str;
	while (*s && (*s!='\n') && (*s!='=')) {
		if (*s!='\\')
			*ws++=*((unsigned char*)s++);
		else {
			s++;
			if (!*s) {
				/* Dangling \ ... may only happen if a registry
				 * write was short. FIXME: What do to?
				 */
				 break;
			}
			if (*s=='\\') {
				*ws++='\\';
				s++;
				continue;
			}
			if (*s!='u') {
				WARN("Non unicode escape sequence \\%c found in |%s|\n",*s,buf);
				*ws++='\\';
				*ws++=*s++;
			} else {
				char	xbuf[5];
				int	wc;

				s++;
				memcpy(xbuf,s,4);xbuf[4]='\0';
				if (!sscanf(xbuf,"%x",&wc))
					WARN("Strange escape sequence %s found in |%s|\n",xbuf,buf);
				s+=4;
				*ws++	=(unsigned short)wc;
			}
		}
	}
	*ws	= 0;
	return s;
}


/******************************************************************************
 * _wine_loadsubkey [Internal]
 *
 * NOTES
 *    It seems like this is returning a boolean.  Should it?
 *
 * RETURNS
 *    Success: 1
 *    Failure: 0
 */
static int _wine_loadsubkey( FILE *F, HKEY hkey, int level, char **buf, int *buflen )
{
    	HKEY subkey;
	int		i;
	char		*s;
	LPWSTR		name;

    TRACE("(%p,%x,%d,%s,%d)\n", F, hkey, level, debugstr_a(*buf), *buflen);

    /* Good.  We already got a line here ... so parse it */
    subkey = 0;
    while (1) {
        i=0;s=*buf;
        while (*s=='\t') {
            s++;
            i++;
        }
        if (i>level) {
            if (!subkey) {
                WARN("Got a subhierarchy without resp. key?\n");
                return 0;
            }
	    if (!_wine_loadsubkey(F,subkey,level+1,buf,buflen))
	       if (!_wine_read_line(F,buf,buflen))
		  goto done;
            continue;
        }

		/* let the caller handle this line */
		if (i<level || **buf=='\0')
			goto done;

		/* it can be: a value or a keyname. Parse the name first */
		s=_wine_read_USTRING(s,&name);

		/* switch() default: hack to avoid gotos */
		switch (0) {
		default:
			if (*s=='\0') {
                                if (subkey) RegCloseKey( subkey );
				subkey=_find_or_add_key(hkey,name);
			} else {
				LPBYTE		data;
				int		len,lastmodified,type;

				if (*s!='=') {
					WARN("Unexpected character: %c\n",*s);
					break;
				}
				s++;
				if (2!=sscanf(s,"%d,%d,",&type,&lastmodified)) {
					WARN("Haven't understood possible value in |%s|, skipping.\n",*buf);
					break;
				}
				/* skip the 2 , */
				s=strchr(s,',');s++;
				s=strchr(s,',');
				if (!s++) {
					WARN("Haven't understood possible value in |%s|, skipping.\n",*buf);
					break;
				}
				if (type == REG_SZ || type == REG_EXPAND_SZ) {
					s=_wine_read_USTRING(s,(LPWSTR*)&data);
                                        len = lstrlenW((LPWSTR)data)*2+2;
				} else {
					len=strlen(s)/2;
					data = (LPBYTE)xmalloc(len+1);
					for (i=0;i<len;i++) {
						data[i]=0;
						if (*s>='0' && *s<='9')
							data[i]=(*s-'0')<<4;
						if (*s>='a' && *s<='f')
							data[i]=(*s-'a'+'\xa')<<4;
						if (*s>='A' && *s<='F')
							data[i]=(*s-'A'+'\xa')<<4;
						s++;
						if (*s>='0' && *s<='9')
							data[i]|=*s-'0';
						if (*s>='a' && *s<='f')
							data[i]|=*s-'a'+'\xa';
						if (*s>='A' && *s<='F')
							data[i]|=*s-'A'+'\xa';
						s++;
					}
				}
				_find_or_add_value(hkey,name,type,data,len);
			}
		}
		/* read the next line */
		if (!_wine_read_line(F,buf,buflen))
			goto done;
    }
 done:
    if (subkey) RegCloseKey( subkey );
    return 1;
}


/******************************************************************************
 * _wine_loadsubreg [Internal]
 */
static int _wine_loadsubreg( FILE *F, HKEY hkey, const char *fn )
{
	int	ver;
	char	*buf;
	int	buflen;

	buf=xmalloc(10);buflen=10;
	if (!_wine_read_line(F,&buf,&buflen)) {
		free(buf);
		return 0;
	}
	if (!sscanf(buf,"WINE REGISTRY Version %d",&ver)) {
		free(buf);
		return 0;
	}
	if (ver!=REGISTRY_SAVE_VERSION) {
            if (ver == 2)  /* new version */
            {
                HANDLE file;
                if ((file = FILE_CreateFile( fn, GENERIC_READ, 0, NULL, OPEN_EXISTING,
                                             FILE_ATTRIBUTE_NORMAL, -1 )) != INVALID_HANDLE_VALUE)
                {
                    struct load_registry_request *req = get_req_buffer();
                    req->hkey    = hkey;
                    req->file    = file;
                    req->name[0] = 0;
                    server_call( REQ_LOAD_REGISTRY );
                    CloseHandle( file );
                }
                free( buf );
                return 1;
            }
            else
            {
		TRACE("Old format (%d) registry found, ignoring it. (buf was %s).\n",ver,buf);
		free(buf);
		return 0;
            }
	}
	if (!_wine_read_line(F,&buf,&buflen)) {
		free(buf);
		return 0;
	}
	if (!_wine_loadsubkey(F,hkey,0,&buf,&buflen)) {
		free(buf);
		return 0;
	}
	free(buf);
	return 1;
}


/******************************************************************************
 * _wine_loadreg [Internal]
 */
static void _wine_loadreg( HKEY hkey, char *fn )
{
    FILE *F;

    TRACE("(%x,%s)\n",hkey,debugstr_a(fn));

    F = fopen(fn,"rb");
    if (F==NULL) {
        WARN("Couldn't open %s for reading: %s\n",fn,strerror(errno) );
        return;
    }
    _wine_loadsubreg(F,hkey,fn);
    fclose(F);
}

/******************************************************************************
 * _flush_registry [Internal]
 * 
 * This function allow to flush section of the internal registry.  It is mainly
 * implements to fix a problem with the global HKU and the local HKU.
 * Those two files are read to build the HKU\.Default branch to finaly copy
 * this branch onto HKCU hive, once this is done, if we keep the HKU hive as is, 
 * all the global HKU are saved onto the user's personal version of HKU hive.
 * which is bad...
 */

static void _flush_registry( HKEY hkey )
{
    WCHAR name[MAX_PATH];

    for (;;)
    {
        HKEY subkey;
        /* FIXME: we assume that deleting a key will move the other ones up, */
        /* so that we can always use index 0 until there are no more keys    */
        if (RegEnumKeyW( hkey, 0, name, sizeof(name) ) != ERROR_SUCCESS) break;
        if (RegOpenKeyW( hkey, name, &subkey ) != ERROR_SUCCESS) break;
        _flush_registry( subkey );
        if (RegDeleteKeyW( subkey, NULL ) != ERROR_SUCCESS) break;
        RegCloseKey( subkey );
    }
}


/******************************************************************************
 * _copy_registry [Internal]
 */
static void _copy_registry( HKEY from, HKEY to )
{
    int index;
    HKEY subkey;
    FILETIME ft;
    DWORD type, name_len, len;
    static WCHAR name[MAX_PATH];
    static BYTE data[2048];

    /* copy values */
    index = 0;
    for (;;)
    {
        len = sizeof(data);
        name_len = sizeof(name);
        if (RegEnumValueW( from, index++, name, &name_len,
                           NULL, &type, data, &len ) != ERROR_SUCCESS) break;
        RegSetValueExW( to, name, 0, type, data, len );
    }

    /* copy subkeys */
    index = 0;
    for (;;)
    {
        name_len = sizeof(name);
        if (RegEnumKeyExW( from, index++, name, &name_len,
                           NULL, NULL, 0, &ft ) != ERROR_SUCCESS)
            break;
        if (RegOpenKeyW( from, name, &subkey ) == ERROR_SUCCESS)
        {
            HKEY newsub;
            if (RegCreateKeyW( to, name, &newsub ) == ERROR_SUCCESS)
            {
                _copy_registry( subkey, newsub );
                RegCloseKey( newsub );
            }
            RegCloseKey( subkey );
        }
    }
}
/* NT REGISTRY LOADER */
#include <sys/mman.h>

#ifndef MAP_FAILED
#define MAP_FAILED ((LPVOID)-1)
#endif

#define LONG_DUMP 1

#define  REG_BLOCK_SIZE		0x1000

#define  REG_HEADER_BLOCK_ID	0x66676572	/* regf */
#define  REG_POOL_BLOCK_ID	0x6E696268	/* hbin */
#define  REG_KEY_BLOCK_ID	0x6b6e
#define	 REG_VALUE_BLOCK_ID	0x6b76
#define	 REG_HASH_BLOCK_ID	0x666c
#define  REG_NOHASH_BLOCK_ID	0x696c
#define  REG_KEY_BLOCK_TYPE	0x20
#define  REG_ROOT_KEY_BLOCK_TYPE	0x2c

typedef struct 
{
	DWORD	id;		/* 0x66676572 'regf'*/
	DWORD	uk1;		/* 0x04 */
	DWORD	uk2;		/* 0x08 */
	FILETIME	DateModified;	/* 0x0c */
	DWORD	uk3;		/* 0x14 */
	DWORD	uk4;		/* 0x18 */
	DWORD	uk5;		/* 0x1c */
	DWORD	uk6;		/* 0x20 */
	DWORD	RootKeyBlock;	/* 0x24 */
	DWORD	BlockSize;	/* 0x28 */
	DWORD   uk7[116];	
	DWORD	Checksum; /* at offset 0x1FC */
} nt_regf;

typedef struct
{
	DWORD	blocksize;
	BYTE	data[1];
} nt_hbin_sub;

typedef struct
{
	DWORD	id;		/* 0x6E696268 'hbin' */
	DWORD	off_prev;
	DWORD	off_next;
	DWORD	uk1;
	DWORD	uk2;		/* 0x10 */
	DWORD	uk3;		/* 0x14 */
	DWORD	uk4;		/* 0x18 */
	DWORD	size;		/* 0x1C */
	nt_hbin_sub	hbin_sub;	/* 0x20 */
} nt_hbin;

/*
 * the value_list consists of offsets to the values (vk)
 */
typedef struct
{
	WORD	SubBlockId;		/* 0x00 0x6B6E */
	WORD	Type;			/* 0x02 for the root-key: 0x2C, otherwise 0x20*/
	FILETIME	writetime;	/* 0x04 */
	DWORD	uk1;			/* 0x0C */
	DWORD	parent_off;		/* 0x10 Offset of Owner/Parent key */
	DWORD	nr_subkeys;		/* 0x14 number of sub-Keys */
	DWORD	uk8;			/* 0x18 */
	DWORD	lf_off;			/* 0x1C Offset of the sub-key lf-Records */
	DWORD	uk2;			/* 0x20 */
	DWORD	nr_values;		/* 0x24 number of values */
	DWORD	valuelist_off;		/* 0x28 Offset of the Value-List */
	DWORD	off_sk;			/* 0x2c Offset of the sk-Record */
	DWORD	off_class;		/* 0x30 Offset of the Class-Name */
	DWORD	uk3;			/* 0x34 */
	DWORD	uk4;			/* 0x38 */
	DWORD	uk5;			/* 0x3c */
	DWORD	uk6;			/* 0x40 */
	DWORD	uk7;			/* 0x44 */
	WORD	name_len;		/* 0x48 name-length */
	WORD	class_len;		/* 0x4a class-name length */
	char	name[1];		/* 0x4c key-name */
} nt_nk;

typedef struct
{
	DWORD	off_nk;	/* 0x00 */
	DWORD	name;	/* 0x04 */
} hash_rec;

typedef struct
{
	WORD	id;		/* 0x00 0x666c */
	WORD	nr_keys;	/* 0x06 */
	hash_rec	hash_rec[1];
} nt_lf;

typedef struct
{
	WORD	id;		/* 0x00 0x696c */
	WORD	nr_keys;
	DWORD	off_nk[1];
} nt_il;

typedef struct
{
	WORD	id;		/* 0x00 'vk' */
	WORD	nam_len;
	DWORD	data_len;
	DWORD	data_off;
	DWORD	type;
	WORD	flag;
	WORD	uk1;
	char	name[1];
} nt_vk;

#define vk_sz		0x0001
#define	vk_expsz	0x0002
#define	vk_bin		0x0003
#define vk_dword	0x0004
#define vk_multisz	0x0007
#define vk_u2		0x0008
#define vk_u1		0x000a

LPSTR _strdupnA( LPCSTR str, int len )
{
    LPSTR ret;

    if (!str) return NULL;
    ret = malloc( len + 1 );
    lstrcpynA( ret, str, len );
    ret[len] = 0x00;
    return ret;
}

int _nt_parse_nk(HKEY hkey, char * base, nt_nk * nk, int level);
int _nt_parse_vk(HKEY hkey, char * base, nt_vk * vk, int level);
int _nt_parse_lf(HKEY hkey, char * base, nt_lf * lf, int level);


/*
 * gets a value
 *
 * vk->flag:
 *  0 value is a default value
 *  1 the value has a name
 *
 * vk->data_len
 *  len of the whole data block
 *  - reg_sz (unicode)
 *    bytes including the terminating \0 = 2*(number_of_chars+1)
 *  - reg_dword, reg_binary:
 *    if highest bit of data_len is set data_off contains the value
 */
int _nt_parse_vk(HKEY hkey, char * base, nt_vk * vk, int level)
{
	WCHAR name [256];
	BYTE * pdata = (BYTE *)(base+vk->data_off+4); /* start of data */

	if(vk->id != REG_VALUE_BLOCK_ID) goto error;

	lstrcpynAtoW(name, vk->name, vk->nam_len+1);

	if (RegSetValueExW( hkey, (vk->flag & 0x00000001) ? name : NULL, 0, vk->type,
			(vk->data_len & 0x80000000) ? (LPBYTE)&(vk->data_off): pdata,
			(vk->data_len & 0x7fffffff) )) goto error;
	return TRUE;
error:
	ERR_(reg)("vk block invalid\n");
	return FALSE;
}

/*
 * get the subkeys
 *
 * this structure contains the hash of a keyname and points to all
 * subkeys
 *
 * exception: if the id is 'il' there are no hash values and every 
 * dword is a offset
 */
int _nt_parse_lf(HKEY hkey, char * base, nt_lf * lf, int level)
{
	int i;

	if (lf->id == REG_HASH_BLOCK_ID)
	{
	  for (i=0; i<lf->nr_keys; i++)
	  {
	    if (!_nt_parse_nk(hkey, base, (nt_nk*)(base+lf->hash_rec[i].off_nk+4), level)) goto error;
	  }
	  
	}
	else if (lf->id == REG_NOHASH_BLOCK_ID)
	{
	  for (i=0; i<lf->nr_keys; i++)
	  {
	    if (!_nt_parse_nk(hkey, base, (nt_nk*)(base+((nt_il*)lf)->off_nk[i]+4), level)) goto error;
	  }
	}
	return TRUE;
	
error:	ERR_(reg)("error reading lf block\n");
	return FALSE;
}

int _nt_parse_nk(HKEY hkey, char * base, nt_nk * nk, int level)
{
	char * name;
	int i;
	DWORD * vl;
	HKEY subkey;

	if(nk->SubBlockId != REG_KEY_BLOCK_ID) goto error;
	if((nk->Type!=REG_ROOT_KEY_BLOCK_TYPE) &&
	   (((nt_nk*)(base+nk->parent_off+4))->SubBlockId != REG_KEY_BLOCK_ID)) goto error;

	/* create the new key */
	name = _strdupnA( nk->name, nk->name_len+1);
	if(RegCreateKeyA( hkey, name, &subkey )) { free(name); goto error; }
	free(name);

	/* loop through the subkeys */
	if (nk->nr_subkeys)
	{
	  nt_lf * lf = (nt_lf*)(base+nk->lf_off+4);
	  if (nk->nr_subkeys != lf->nr_keys) goto error1;
	  if (!_nt_parse_lf(subkey, base, lf, level+1)) goto error1;
	}

	/* loop trough the value list */
	vl = (DWORD *)(base+nk->valuelist_off+4);
	for (i=0; i<nk->nr_values; i++)
	{
	  nt_vk * vk = (nt_vk*)(base+vl[i]+4);
	  if (!_nt_parse_vk(subkey, base, vk, level+1 )) goto error1;
	}

	RegCloseKey(subkey);
	return TRUE;
	
error1:	RegCloseKey(subkey);
error:	ERR_(reg)("error reading nk block\n");
	return FALSE;
}

/*
 * this function intentionally uses unix file functions to make it possible
 * to move it to a seperate registry helper programm
 */
static int _nt_loadreg( HKEY hkey, char* fn )
{
	void * base;
	int len, fd;
	struct stat st;
	nt_regf * regf;
	nt_hbin * hbin;
	nt_hbin_sub * hbin_sub;
	nt_nk* nk;
	DOS_FULL_NAME full_name;
	
	if (!DOSFS_GetFullName( fn, 0, &full_name ));

	TRACE_(reg)("Loading NT registry database '%s' '%s'\n",fn, full_name.long_name);

	if ((fd = open(full_name.long_name, O_RDONLY | O_NONBLOCK)) == -1) return FALSE;
	if (fstat(fd, &st ) == -1) goto error1;
	len = st.st_size;
	if ((base=mmap(NULL, len, PROT_READ, MAP_PRIVATE, fd, 0)) == MAP_FAILED) goto error1;

	/* start block */
	regf = base;
	if(regf->id != REG_HEADER_BLOCK_ID)	/* 'regf' */
	{
	  ERR( "%s is not a nt-registry\n", fn);
	  goto error;
	}
	TRACE_(reg)( "%p [regf] offset=%lx size=%lx\n", regf, regf->RootKeyBlock, regf->BlockSize);
	
	/* hbin block */
	hbin = base + 0x1000;
	if (hbin->id != REG_POOL_BLOCK_ID)
	{
	  ERR_(reg)( "%s hbin block invalid\n", fn);
	  goto error;
	}
	TRACE_(reg)( "%p [hbin]  prev=%lx next=%lx size=%lx\n", hbin, hbin->off_prev, hbin->off_next, hbin->size);

	/* hbin_sub block */
	hbin_sub = (nt_hbin_sub*)&(hbin->hbin_sub);
	if ((hbin_sub->data[0] != 'n') || (hbin_sub->data[1] != 'k'))
	{
	  ERR_(reg)( "%s hbin_sub block invalid\n", fn);
	  goto error;
	}
 	TRACE_(reg)( "%p [hbin sub] size=%lx\n", hbin_sub, hbin_sub->blocksize);
            	
	/* nk block */
	nk = (nt_nk*)&(hbin_sub->data[0]);
	if (nk->Type != REG_ROOT_KEY_BLOCK_TYPE)
	{
	  ERR_(reg)( "%s special nk block not found\n", fn);
	  goto error;
	}

	_nt_parse_nk (hkey, base+0x1000, nk, 0);

	munmap(base, len);
	close(fd);
	return 1;

error:	munmap(base, len);
error1:	close(fd);
	ERR_(reg)("error reading registry file\n");
	return 0;
}
/* end nt loader */

/* WINDOWS 95 REGISTRY LOADER */
/* 
 * Structure of a win95 registry database.
 * main header:
 * 0 :	"CREG"	- magic
 * 4 :	DWORD version
 * 8 :	DWORD offset_of_RGDB_part
 * 0C..0F:	? (someone fill in please)
 * 10:  WORD	number of RGDB blocks
 * 12:  WORD	?
 * 14:  WORD	always 0000?
 * 16:  WORD	always 0001?
 * 18..1F:	? (someone fill in please)
 *
 * 20: RGKN_section:
 *   header:
 * 	0 :		"RGKN"	- magic
 *      4 : DWORD	offset to first RGDB section
 *      8 : DWORD	offset to the root record
 * 	C..0x1B: 	? (fill in)
 *      0x20 ... offset_of_RGDB_part: Disk Key Entry structures
 *
 *   Disk Key Entry Structure:
 *	00: DWORD	- Free entry indicator(?)
 *	04: DWORD	- Hash = sum of bytes of keyname
 *	08: DWORD	- Root key indicator? unknown, but usually 0xFFFFFFFF on win95 systems
 *	0C: DWORD	- disk address of PreviousLevel Key.
 *	10: DWORD	- disk address of Next Sublevel Key.
 *	14: DWORD	- disk address of Next Key (on same level).
 * DKEP>18: WORD	- Nr, Low Significant part.
 *	1A: WORD	- Nr, High Significant part.
 *
 * The disk address always points to the nr part of the previous key entry 
 * of the referenced key. Don't ask me why, or even if I got this correct
 * from staring at 1kg of hexdumps. (DKEP)
 *
 * The High significant part of the structure seems to equal the number
 * of the RGDB section. The low significant part is a unique ID within
 * that RGDB section
 *
 * There are two minor corrections to the position of that structure.
 * 1. If the address is xxx014 or xxx018 it will be aligned to xxx01c AND 
 *    the DKE reread from there.
 * 2. If the address is xxxFFx it will be aligned to (xxx+1)000.
 * CPS - I have not experienced the above phenomenon in my registry files
 *
 * RGDB_section:
 * 	00:		"RGDB"	- magic
 *	04: DWORD	offset to next RGDB section
 *	08: DWORD	?
 *	0C: WORD	always 000d?
 *	0E: WORD	RGDB block number
 *	10:	DWORD	? (equals value at offset 4 - value at offset 8)
 *	14..1F:		?
 *	20.....:	disk keys
 *
 * disk key:
 * 	00: 	DWORD	nextkeyoffset	- offset to the next disk key structure
 *	08: 	WORD	nrLS		- low significant part of NR
 *	0A: 	WORD	nrHS		- high significant part of NR
 *	0C: 	DWORD	bytesused	- bytes used in this structure.
 *	10: 	WORD	name_len	- length of name in bytes. without \0
 *	12: 	WORD	nr_of_values	- number of values.
 *	14: 	char	name[name_len]	- name string. No \0.
 *	14+name_len: disk values
 *	nextkeyoffset: ... next disk key
 *
 * disk value:
 *	00:	DWORD	type		- value type (hmm, could be WORD too)
 *	04:	DWORD			- unknown, usually 0
 *	08:	WORD	namelen		- length of Name. 0 means name=NULL
 *	0C:	WORD	datalen		- length of Data.
 *	10:	char	name[namelen]	- name, no \0
 *	10+namelen: BYTE	data[datalen] - data, without \0 if string
 *	10+namelen+datalen: next values or disk key
 *
 * Disk keys are layed out flat ... But, sometimes, nrLS and nrHS are both
 * 0xFFFF, which means skipping over nextkeyoffset bytes (including this
 * structure) and reading another RGDB_section.
 * repeat until end of file.
 *
 * An interesting relationship exists in RGDB_section. The value at offset
 * 10 equals the value at offset 4 minus the value at offset 8. I have no
 * idea at the moment what this means.  (Kevin Cozens)
 *
 * FIXME: this description needs some serious help, yes.
 */

struct	_w95keyvalue {
	unsigned long		type;
	unsigned short		datalen;
	char			*name;
	unsigned char		*data;
	unsigned long		x1;
	int			lastmodified;
};

struct 	_w95key {
	char			*name;
	int			nrofvals;
	struct	_w95keyvalue	*values;
	struct _w95key		*prevlvl;
	struct _w95key		*nextsub;
	struct _w95key		*next;
};


struct _w95_info {
  char *rgknbuffer;
  int  rgknsize;
  char *rgdbbuffer;
  int  rgdbsize;
  int  depth;
  int  lastmodified;
};


/******************************************************************************
 * _w95_processKey [Internal]
 */
static HKEY _w95_processKey ( HKEY hkey, int nrLS, int nrMS, struct _w95_info *info )

{
  /* Disk Key Header structure (RGDB part) */
	struct	dkh {
                unsigned long		nextkeyoff; 
		unsigned short		nrLS;
		unsigned short		nrMS;
		unsigned long		bytesused;
		unsigned short		keynamelen;
		unsigned short		values;
		unsigned long		xx1;
		/* keyname */
		/* disk key values or nothing */
	};
	/* Disk Key Value structure */
	struct	dkv {
		unsigned long		type;
		unsigned long		x1;
		unsigned short		valnamelen;
		unsigned short		valdatalen;
		/* valname, valdata */
	};

	
	struct	dkh dkh;
	int	bytesread = 0;
	char    *rgdbdata = info->rgdbbuffer;
	int     nbytes = info->rgdbsize;
	char    *curdata = rgdbdata;
	char    *end = rgdbdata + nbytes;
	int     off_next_rgdb;
	char    *next = rgdbdata;
	int     nrgdb, i;
        HKEY subkey;
	
	do {
	  curdata = next;
	  if (strncmp(curdata, "RGDB", 4)) return 0;
	    
	  memcpy(&off_next_rgdb,curdata+4,4);
	  next = curdata + off_next_rgdb;
	  nrgdb = (int) *((short *)curdata + 7);

	} while (nrgdb != nrMS && (next < end));

	/* curdata now points to the start of the right RGDB section */
	curdata += 0x20;

#define XREAD(whereto,len) \
	if ((curdata + len) <= end) {\
		memcpy(whereto,curdata,len);\
		curdata+=len;\
		bytesread+=len;\
	}

	while (curdata < next) {
	  struct	dkh *xdkh = (struct dkh*)curdata;

	  bytesread += sizeof(dkh); /* FIXME... nextkeyoff? */
	  if (xdkh->nrLS == nrLS) {
	  	memcpy(&dkh,xdkh,sizeof(dkh));
	  	curdata += sizeof(dkh);
	  	break;
	  }
	  curdata += xdkh->nextkeyoff;
	};

	if (dkh.nrLS != nrLS) return 0;

	if (nrgdb != dkh.nrMS)
	  return 0;

        assert((dkh.keynamelen<2) || curdata[0]);
	subkey=_find_or_add_key(hkey,strcvtA2W(curdata, dkh.keynamelen));
	curdata += dkh.keynamelen;

	for (i=0;i< dkh.values; i++) {
	  struct dkv dkv;
	  LPBYTE data;
	  int len;
	  LPWSTR name;

	  XREAD(&dkv,sizeof(dkv));

	  name = strcvtA2W(curdata, dkv.valnamelen);
	  curdata += dkv.valnamelen;

	  if ((1 << dkv.type) & UNICONVMASK) {
	    data = (LPBYTE) strcvtA2W(curdata, dkv.valdatalen);
	    len = 2*(dkv.valdatalen + 1);
	  } else {
	    /* I don't think we want to NULL terminate all data */
	    data = xmalloc(dkv.valdatalen);
	    memcpy (data, curdata, dkv.valdatalen);
	    len = dkv.valdatalen;
	  }

	  curdata += dkv.valdatalen;
	  
	  _find_or_add_value( subkey, name, dkv.type, data, len );
	}
	return subkey;
}

/******************************************************************************
 * _w95_walkrgkn [Internal]
 */
static void _w95_walkrgkn( HKEY prevkey, char *off, 
                           struct _w95_info *info )

{
  /* Disk Key Entry structure (RGKN part) */
  struct	dke {
    unsigned long		x1;
    unsigned long		x2;
    unsigned long		x3;/*usually 0xFFFFFFFF */
    unsigned long		prevlvl;
    unsigned long		nextsub;
    unsigned long		next;
    unsigned short		nrLS;
    unsigned short		nrMS;
  } *dke = (struct dke *)off;
  HKEY subkey;

  if (dke == NULL) {
    dke = (struct dke *) ((char *)info->rgknbuffer);
  }

  subkey = _w95_processKey(prevkey, dke->nrLS, dke->nrMS, info);

  if (dke->nextsub != -1 && 
      ((dke->nextsub - 0x20) < info->rgknsize) 
      && (dke->nextsub > 0x20)) {
    
    _w95_walkrgkn(subkey ? subkey : prevkey, /* XXX <-- This is a hack*/
		  info->rgknbuffer + dke->nextsub - 0x20, 
		  info);
  }
  if (subkey) RegCloseKey( subkey );

  if (dke->next != -1 && 
      ((dke->next - 0x20) < info->rgknsize) && 
      (dke->next > 0x20)) {
    _w95_walkrgkn(prevkey,  
		  info->rgknbuffer + dke->next - 0x20,
		  info);
  }
}


/******************************************************************************
 * _w95_loadreg [Internal]
 */
static void _w95_loadreg( char* fn, HKEY hkey )
{
	HFILE		hfd;
	char		magic[5];
	unsigned long	where,version,rgdbsection,end;
	struct          _w95_info info;
	OFSTRUCT	ofs;
	BY_HANDLE_FILE_INFORMATION hfdinfo;

	TRACE("Loading Win95 registry database '%s'\n",fn);
	hfd=OpenFile(fn,&ofs,OF_READ);
	if (hfd==HFILE_ERROR)
		return;
	magic[4]=0;
	if (4!=_lread(hfd,magic,4))
		return;
	if (strcmp(magic,"CREG")) {
		WARN("%s is not a w95 registry.\n",fn);
		return;
	}
	if (4!=_lread(hfd,&version,4))
		return;
	if (4!=_lread(hfd,&rgdbsection,4))
		return;
	if (-1==_llseek(hfd,0x20,SEEK_SET))
		return;
	if (4!=_lread(hfd,magic,4))
		return;
	if (strcmp(magic,"RGKN")) {
		WARN("second IFF header not RGKN, but %s\n", magic);
		return;
	}

	/* STEP 1: Keylink structures */
	if (-1==_llseek(hfd,0x40,SEEK_SET))
		return;
	where	= 0x40;
	end	= rgdbsection;

	info.rgknsize = end - where;
	info.rgknbuffer = (char*)xmalloc(info.rgknsize);
	if (info.rgknsize != _lread(hfd,info.rgknbuffer,info.rgknsize))
		return;

	if (!GetFileInformationByHandle(hfd,&hfdinfo))
		return;

	end = hfdinfo.nFileSizeLow;
	info.lastmodified = DOSFS_FileTimeToUnixTime(&hfdinfo.ftLastWriteTime,NULL);

	if (-1==_llseek(hfd,rgdbsection,SEEK_SET))
		return;

	info.rgdbbuffer = (char*)xmalloc(end-rgdbsection);
	info.rgdbsize = end - rgdbsection;

	if (info.rgdbsize !=_lread(hfd,info.rgdbbuffer,info.rgdbsize))
		return;
	_lclose(hfd);

	_w95_walkrgkn(hkey, NULL, &info);

	free (info.rgdbbuffer);
	free (info.rgknbuffer);
}


/* WINDOWS 31 REGISTRY LOADER, supplied by Tor Sjøwall, tor@sn.no */

/*
    reghack - windows 3.11 registry data format demo program.

    The reg.dat file has 3 parts, a header, a table of 8-byte entries that is
    a combined hash table and tree description, and finally a text table.

    The header is obvious from the struct header. The taboff1 and taboff2
    fields are always 0x20, and their usage is unknown.

    The 8-byte entry table has various entry types.

    tabent[0] is a root index. The second word has the index of the root of
            the directory.
    tabent[1..hashsize] is a hash table. The first word in the hash entry is
            the index of the key/value that has that hash. Data with the same
            hash value are on a circular list. The other three words in the
            hash entry are always zero.
    tabent[hashsize..tabcnt] is the tree structure. There are two kinds of
            entry: dirent and keyent/valent. They are identified by context.
    tabent[freeidx] is the first free entry. The first word in a free entry
            is the index of the next free entry. The last has 0 as a link.
            The other three words in the free list are probably irrelevant.

    Entries in text table are preceeded by a word at offset-2. This word
    has the value (2*index)+1, where index is the referring keyent/valent
    entry in the table. I have no suggestion for the 2* and the +1.
    Following the word, there are N bytes of data, as per the keyent/valent
    entry length. The offset of the keyent/valent entry is from the start
    of the text table to the first data byte.

    This information is not available from Microsoft. The data format is
    deduced from the reg.dat file by me. Mistakes may
    have been made. I claim no rights and give no guarantees for this program.

    Tor Sjøwall, tor@sn.no
*/

/* reg.dat header format */
struct _w31_header {
	char		cookie[8];	/* 'SHCC3.10' */
	unsigned long	taboff1;	/* offset of hash table (??) = 0x20 */
	unsigned long	taboff2;	/* offset of index table (??) = 0x20 */
	unsigned long	tabcnt;		/* number of entries in index table */
	unsigned long	textoff;	/* offset of text part */
	unsigned long	textsize;	/* byte size of text part */
	unsigned short	hashsize;	/* hash size */
	unsigned short	freeidx;	/* free index */
};

/* generic format of table entries */
struct _w31_tabent {
	unsigned short w0, w1, w2, w3;
};

/* directory tabent: */
struct _w31_dirent {
	unsigned short	sibling_idx;	/* table index of sibling dirent */
	unsigned short	child_idx;	/* table index of child dirent */
	unsigned short	key_idx;	/* table index of key keyent */
	unsigned short	value_idx;	/* table index of value valent */
};

/* key tabent: */
struct _w31_keyent {
	unsigned short	hash_idx;	/* hash chain index for string */
	unsigned short	refcnt;		/* reference count */
	unsigned short	length;		/* length of string */
	unsigned short	string_off;	/* offset of string in text table */
};

/* value tabent: */
struct _w31_valent {
	unsigned short	hash_idx;	/* hash chain index for string */
	unsigned short	refcnt;		/* reference count */
	unsigned short	length;		/* length of string */
	unsigned short	string_off;	/* offset of string in text table */
};

/* recursive helper function to display a directory tree */
void
__w31_dumptree(	unsigned short idx,
		unsigned char *txt,
		struct _w31_tabent *tab,
		struct _w31_header *head,
		HKEY hkey,
		time_t		lastmodified,
		int		level
) {
	struct _w31_dirent	*dir;
	struct _w31_keyent	*key;
	struct _w31_valent	*val;
        HKEY subkey = 0;
	static char		tail[400];

	while (idx!=0) {
		dir=(struct _w31_dirent*)&tab[idx];

		if (dir->key_idx) {
			key = (struct _w31_keyent*)&tab[dir->key_idx];

			memcpy(tail,&txt[key->string_off],key->length);
			tail[key->length]='\0';
			/* all toplevel entries AND the entries in the 
			 * toplevel subdirectory belong to \SOFTWARE\Classes
			 */
			if (!level && !lstrcmpA(tail,".classes")) {
				__w31_dumptree(dir->child_idx,txt,tab,head,hkey,lastmodified,level+1);
				idx=dir->sibling_idx;
				continue;
			}
                        if (subkey) RegCloseKey( subkey );
                        if (RegCreateKeyA( hkey, tail, &subkey ) != ERROR_SUCCESS) subkey = 0;
			/* only add if leaf node or valued node */
			if (dir->value_idx!=0||dir->child_idx==0) {
				if (dir->value_idx) {
					val=(struct _w31_valent*)&tab[dir->value_idx];
					memcpy(tail,&txt[val->string_off],val->length);
					tail[val->length]='\0';
                                        RegSetValueA( subkey, NULL, REG_SZ, tail, 0 );
				}
			}
		} else {
			TRACE("strange: no directory key name, idx=%04x\n", idx);
		}
		__w31_dumptree(dir->child_idx,txt,tab,head,subkey,lastmodified,level+1);
		idx=dir->sibling_idx;
	}
        if (subkey) RegCloseKey( subkey );
}


/******************************************************************************
 * _w31_loadreg [Internal]
 */
void _w31_loadreg(void) {
	HFILE			hf;
	struct _w31_header	head;
	struct _w31_tabent	*tab;
	unsigned char		*txt;
	int			len;
	OFSTRUCT		ofs;
	BY_HANDLE_FILE_INFORMATION hfinfo;
	time_t			lastmodified;

	TRACE("(void)\n");

	hf = OpenFile("reg.dat",&ofs,OF_READ);
	if (hf==HFILE_ERROR)
		return;

	/* read & dump header */
	if (sizeof(head)!=_lread(hf,&head,sizeof(head))) {
		ERR("reg.dat is too short.\n");
		_lclose(hf);
		return;
	}
	if (memcmp(head.cookie, "SHCC3.10", sizeof(head.cookie))!=0) {
		ERR("reg.dat has bad signature.\n");
		_lclose(hf);
		return;
	}

	len = head.tabcnt * sizeof(struct _w31_tabent);
	/* read and dump index table */
	tab = xmalloc(len);
	if (len!=_lread(hf,tab,len)) {
		ERR("couldn't read %d bytes.\n",len); 
		free(tab);
		_lclose(hf);
		return;
	}

	/* read text */
	txt = xmalloc(head.textsize);
	if (-1==_llseek(hf,head.textoff,SEEK_SET)) {
		ERR("couldn't seek to textblock.\n"); 
		free(tab);
		free(txt);
		_lclose(hf);
		return;
	}
	if (head.textsize!=_lread(hf,txt,head.textsize)) {
		ERR("textblock too short (%d instead of %ld).\n",len,head.textsize); 
		free(tab);
		free(txt);
		_lclose(hf);
		return;
	}

	if (!GetFileInformationByHandle(hf,&hfinfo)) {
		ERR("GetFileInformationByHandle failed?.\n"); 
		free(tab);
		free(txt);
		_lclose(hf);
		return;
	}
	lastmodified = DOSFS_FileTimeToUnixTime(&hfinfo.ftLastWriteTime,NULL);
	__w31_dumptree(tab[0].w1,txt,tab,&head,HKEY_CLASSES_ROOT,lastmodified,0);
	free(tab);
	free(txt);
	_lclose(hf);
	return;
}


/**********************************************************************************
 * SHELL_LoadRegistry [Internal]
 */
void SHELL_LoadRegistry( void )
{
  struct set_registry_levels_request *req = get_req_buffer();
  int save_timeout;
  char	      *fn, *home;
  HKEY		    hkey;

  TRACE("(void)\n");

  REGISTRY_Init();

  /* set level to 0 for loading system files */
  req->current = 0;
  req->saving  = 0;
  req->version = 1;
  server_call( REQ_SET_REGISTRY_LEVELS );

  if (PROFILE_GetWineIniBool ("registry", "LoadWin311RegistryFiles", 1)) 
  { 
      /* Load windows 3.1 entries */
      _w31_loadreg();
  }
  if (PROFILE_GetWineIniBool ("registry", "LoadWin95RegistryFiles", 1)) 
  { 
      /* Load windows 95 entries */
      _w95_loadreg("C:\\system.1st", HKEY_LOCAL_MACHINE);
      _w95_loadreg("system.dat", HKEY_LOCAL_MACHINE);
      _w95_loadreg("user.dat", HKEY_USERS);
  }
  if (PROFILE_GetWineIniBool ("registry", "LoadWinNTRegistryFiles", 1)) 
  { 
      fn = xmalloc( MAX_PATHNAME_LEN ); 
      home = xmalloc ( MAX_PATHNAME_LEN );
      if ( PROFILE_GetWineIniString( "registry", "NTUser", "", home, MAX_PATHNAME_LEN - 1)) 
      {
         GetWindowsDirectoryA( fn, MAX_PATHNAME_LEN );
	 strncat(fn, "\\Profiles\\", MAX_PATHNAME_LEN - strlen(fn) - 1);
	 strncat(fn, home, MAX_PATHNAME_LEN - strlen(fn) - 1);
	 strncat(fn, "\\ntuser.dat", MAX_PATHNAME_LEN - strlen(fn) - 1);
         _nt_loadreg( HKEY_USERS, fn );
      }     
      /*
      * FIXME
      *  map HLM\System\ControlSet001 to HLM\System\CurrentControlSet
      */
      GetSystemDirectoryA( fn, MAX_PATHNAME_LEN );

      strcpy(home, fn);
      strncat(home, "\\config\\system", MAX_PATHNAME_LEN - strlen(home) - 1);
      _nt_loadreg(HKEY_LOCAL_MACHINE, home);

      strcpy(home, fn);
      strncat(home, "\\config\\software", MAX_PATHNAME_LEN - strlen(home) - 1);
      _nt_loadreg(HKEY_LOCAL_MACHINE, home);

      strcpy(home, fn);
      strncat(home, "\\config\\sam", MAX_PATHNAME_LEN - strlen(home) - 1);
      _nt_loadreg(HKEY_LOCAL_MACHINE, home);

      strcpy(home, fn);
      strncat(home, "\\config\\security", MAX_PATHNAME_LEN - strlen(home) - 1);
      _nt_loadreg(HKEY_LOCAL_MACHINE, home);

      free (home);
      free (fn);
  }

  if (PROFILE_GetWineIniBool ("registry","LoadGlobalRegistryFiles", 1))
  {
      /* 
       * Load the global HKU hive directly from sysconfdir
       */ 
      _wine_loadreg( HKEY_USERS, SAVE_USERS_DEFAULT );

      /* 
       * Load the global machine defaults directly form sysconfdir
       */
      _wine_loadreg( HKEY_LOCAL_MACHINE, SAVE_LOCAL_MACHINE_DEFAULT );
  }

  /* set level to 1 for loading user files */
  req->current = 1;
  req->saving  = 0;
  req->version = 1;
  server_call( REQ_SET_REGISTRY_LEVELS );

  /*
   * Load the user saved registries 
   */
  if (!(home = getenv( "HOME" )))
      WARN("Failed to get homedirectory of UID %ld.\n",(long) getuid());
  else if (PROFILE_GetWineIniBool("registry", "LoadHomeRegistryFiles", 1))
  {
      /* 
       * Load user's personal versions of global HKU/.Default keys
       */
      fn=(char*)xmalloc( strlen(home)+ strlen(WINE_PREFIX) +
                         strlen(SAVE_LOCAL_USERS_DEFAULT)+2);
      strcpy(fn, home);
      strcat(fn, WINE_PREFIX"/"SAVE_LOCAL_USERS_DEFAULT);
      _wine_loadreg( HKEY_USERS, fn ); 
      free(fn);

      fn=(char*)xmalloc( strlen(home) + strlen(WINE_PREFIX) + strlen(SAVE_CURRENT_USER)+2);
      strcpy(fn, home);
      strcat(fn, WINE_PREFIX"/"SAVE_CURRENT_USER);
      _wine_loadreg( HKEY_CURRENT_USER, fn );
      free(fn);

      /* 
       * Load HKLM, attempt to get the registry location from the config 
       * file first, if exist, load and keep going.
       */
      fn=(char*)xmalloc( strlen(home)+ strlen(WINE_PREFIX)+ strlen(SAVE_LOCAL_MACHINE)+2);
      strcpy(fn,home);
      strcat(fn,WINE_PREFIX"/"SAVE_LOCAL_MACHINE);
      _wine_loadreg( HKEY_LOCAL_MACHINE, fn );
      free(fn);
  }
  
  /* 
   * Load HKCU, get the registry location from the config 
   * file, if exist, load and keep going.
   */      
  if (PROFILE_GetWineIniBool ( "registry", "LoadAltRegistryFiles", 1))
  {
      fn = xmalloc( MAX_PATHNAME_LEN ); 
      if ( PROFILE_GetWineIniString( "registry", "AltCurrentUserFile", "", 
                                     fn, MAX_PATHNAME_LEN - 1)) 
       {
         _wine_loadreg( HKEY_CURRENT_USER, fn );
       }
      free (fn);
      /*
       * Load HKU, get the registry location from the config
       * file, if exist, load and keep going.
       */
      fn = xmalloc ( MAX_PATHNAME_LEN );
      if ( PROFILE_GetWineIniString ( "registry", "AltUserFile", "",
                                      fn, MAX_PATHNAME_LEN - 1))
       {
         _wine_loadreg( HKEY_USERS, fn );
       }
      free (fn);
      /*
       * Load HKLM, get the registry location from the config
       * file, if exist, load and keep going.
       */
      fn = xmalloc ( MAX_PATHNAME_LEN );
      if (PROFILE_GetWineIniString ( "registry", "AltLocalMachineFile", "",
                                     fn, MAX_PATHNAME_LEN - 1))
       {
         _wine_loadreg( HKEY_LOCAL_MACHINE, fn );
       }
      free (fn);
    }
  
  /* 
   * Obtain the handle of the HKU\.Default key.
   * in order to copy HKU\.Default\* onto HKEY_CURRENT_USER 
   */
  if (RegCreateKeyA(HKEY_USERS,".Default",&hkey) != ERROR_SUCCESS)
      WARN("Could not create global user default key\n");
  else
    _copy_registry( hkey, HKEY_CURRENT_USER );
  RegCloseKey(hkey);

  /* 
   * Since HKU is built from the global HKU and the local user HKU file we must
   * flush the HKU tree we have built at this point otherwise the part brought
   * in from the global HKU is saved into the local HKU.  To avoid this 
   * useless dupplication of HKU keys we reread the local HKU key.
   */

  /* Allways flush the HKU hive and reload it only with user's personal HKU */
  _flush_registry( HKEY_USERS ); 

  /* Reload user's local HKU hive */
  if (home && PROFILE_GetWineIniBool ("registry","LoadHomeRegistryFiles",1))
  {
      fn=(char*)xmalloc( strlen(home) + strlen(WINE_PREFIX)
                         + strlen(SAVE_LOCAL_USERS_DEFAULT) + 2);
      
      strcpy(fn,home);
      strcat(fn,WINE_PREFIX"/"SAVE_LOCAL_USERS_DEFAULT);

      _wine_loadreg( HKEY_USERS, fn );

      free(fn);
  }

  /* 
   * Make sure the update mode is there
   */
  if (ERROR_SUCCESS==RegCreateKey16(HKEY_CURRENT_USER,KEY_REGISTRY,&hkey)) 
  {
    DWORD	junk,type,len;
    char	data[5];

    len=4;
    if ((	RegQueryValueExA(
            hkey,
            VAL_SAVEUPDATED,
            &junk,
            &type,
            data,
            &len) != ERROR_SUCCESS) || (type != REG_SZ))
    {
      RegSetValueExA(hkey,VAL_SAVEUPDATED,0,REG_SZ,"yes",4);
    }

    RegCloseKey(hkey);
  }

  if ((save_timeout = PROFILE_GetWineIniInt( "registry", "PeriodicSave", 0 )))
  {
      SERVICE_AddTimer( save_timeout * 1000000, periodic_save, 0 );
  }
}


/********************* API FUNCTIONS ***************************************/




/******************************************************************************
 * RegFlushKey [KERNEL.227] [ADVAPI32.143]
 * Immediately writes key to registry.
 * Only returns after data has been written to disk.
 *
 * FIXME: does it really wait until data is written ?
 *
 * PARAMS
 *    hkey [I] Handle of key to write
 *
 * RETURNS
 *    Success: ERROR_SUCCESS
 *    Failure: Error code
 */
DWORD WINAPI RegFlushKey( HKEY hkey )
{
    FIXME( "(%x): stub\n", hkey );
    return ERROR_SUCCESS;
}

/******************************************************************************
 * RegConnectRegistry32W [ADVAPI32.128]
 *
 * PARAMS
 *    lpMachineName [I] Address of name of remote computer
 *    hHey          [I] Predefined registry handle
 *    phkResult     [I] Address of buffer for remote registry handle
 */
LONG WINAPI RegConnectRegistryW( LPCWSTR lpMachineName, HKEY hKey, 
                                   LPHKEY phkResult )
{
    TRACE("(%s,%x,%p): stub\n",debugstr_w(lpMachineName),hKey,phkResult);

    if (!lpMachineName || !*lpMachineName) {
        /* Use the local machine name */
        return RegOpenKey16( hKey, "", phkResult );
    }

    FIXME("Cannot connect to %s\n",debugstr_w(lpMachineName));
    return ERROR_BAD_NETPATH;
}


/******************************************************************************
 * RegConnectRegistry32A [ADVAPI32.127]
 */
LONG WINAPI RegConnectRegistryA( LPCSTR machine, HKEY hkey, LPHKEY reskey )
{
    DWORD ret;
    LPWSTR machineW = strdupA2W(machine);
    ret = RegConnectRegistryW( machineW, hkey, reskey );
    free(machineW);
    return ret;
}


/******************************************************************************
 * RegGetKeySecurity [ADVAPI32.144]
 * Retrieves a copy of security descriptor protecting the registry key
 *
 * PARAMS
 *    hkey                   [I]   Open handle of key to set
 *    SecurityInformation    [I]   Descriptor contents
 *    pSecurityDescriptor    [O]   Address of descriptor for key
 *    lpcbSecurityDescriptor [I/O] Address of size of buffer and description
 *
 * RETURNS
 *    Success: ERROR_SUCCESS
 *    Failure: Error code
 */
LONG WINAPI RegGetKeySecurity( HKEY hkey, 
                               SECURITY_INFORMATION SecurityInformation,
                               PSECURITY_DESCRIPTOR pSecurityDescriptor,
                               LPDWORD lpcbSecurityDescriptor )
{
    TRACE("(%x,%ld,%p,%ld)\n",hkey,SecurityInformation,pSecurityDescriptor,
          lpcbSecurityDescriptor?*lpcbSecurityDescriptor:0);

    /* FIXME: Check for valid SecurityInformation values */

    if (*lpcbSecurityDescriptor < sizeof(SECURITY_DESCRIPTOR))
        return ERROR_INSUFFICIENT_BUFFER;

    FIXME("(%x,%ld,%p,%ld): stub\n",hkey,SecurityInformation,
          pSecurityDescriptor,lpcbSecurityDescriptor?*lpcbSecurityDescriptor:0);

    return ERROR_SUCCESS;
}


/******************************************************************************
 * RegNotifyChangeKeyValue [ADVAPI32.???]
 *
 * PARAMS
 *    hkey            [I] Handle of key to watch
 *    fWatchSubTree   [I] Flag for subkey notification
 *    fdwNotifyFilter [I] Changes to be reported
 *    hEvent          [I] Handle of signaled event
 *    fAsync          [I] Flag for asynchronous reporting
 */
LONG WINAPI RegNotifyChangeKeyValue( HKEY hkey, BOOL fWatchSubTree, 
                                     DWORD fdwNotifyFilter, HANDLE hEvent,
                                     BOOL fAsync )
{
    FIXME("(%x,%i,%ld,%x,%i): stub\n",hkey,fWatchSubTree,fdwNotifyFilter,
          hEvent,fAsync);
    return ERROR_SUCCESS;
}


/******************************************************************************
 * RegUnLoadKey32W [ADVAPI32.173]
 *
 * PARAMS
 *    hkey     [I] Handle of open key
 *    lpSubKey [I] Address of name of subkey to unload
 */
LONG WINAPI RegUnLoadKeyW( HKEY hkey, LPCWSTR lpSubKey )
{
    FIXME("(%x,%s): stub\n",hkey, debugstr_w(lpSubKey));
    return ERROR_SUCCESS;
}


/******************************************************************************
 * RegUnLoadKey32A [ADVAPI32.172]
 */
LONG WINAPI RegUnLoadKeyA( HKEY hkey, LPCSTR lpSubKey )
{
    LONG ret;
    LPWSTR lpSubKeyW = strdupA2W(lpSubKey);
    ret = RegUnLoadKeyW( hkey, lpSubKeyW );
    if(lpSubKeyW) free(lpSubKeyW);
    return ret;
}


/******************************************************************************
 * RegSetKeySecurity [ADVAPI32.167]
 *
 * PARAMS
 *    hkey          [I] Open handle of key to set
 *    SecurityInfo  [I] Descriptor contents
 *    pSecurityDesc [I] Address of descriptor for key
 */
LONG WINAPI RegSetKeySecurity( HKEY hkey, SECURITY_INFORMATION SecurityInfo,
                               PSECURITY_DESCRIPTOR pSecurityDesc )
{
    TRACE("(%x,%ld,%p)\n",hkey,SecurityInfo,pSecurityDesc);

    /* It seems to perform this check before the hkey check */
    if ((SecurityInfo & OWNER_SECURITY_INFORMATION) ||
        (SecurityInfo & GROUP_SECURITY_INFORMATION) ||
        (SecurityInfo & DACL_SECURITY_INFORMATION) ||
        (SecurityInfo & SACL_SECURITY_INFORMATION)) {
        /* Param OK */
    } else
        return ERROR_INVALID_PARAMETER;

    if (!pSecurityDesc)
        return ERROR_INVALID_PARAMETER;

    FIXME(":(%x,%ld,%p): stub\n",hkey,SecurityInfo,pSecurityDesc);

    return ERROR_SUCCESS;
}


/******************************************************************************
 * RegRestoreKey32W [ADVAPI32.164]
 *
 * PARAMS
 *    hkey    [I] Handle of key where restore begins
 *    lpFile  [I] Address of filename containing saved tree
 *    dwFlags [I] Optional flags
 */
LONG WINAPI RegRestoreKeyW( HKEY hkey, LPCWSTR lpFile, DWORD dwFlags )
{
    TRACE("(%x,%s,%ld)\n",hkey,debugstr_w(lpFile),dwFlags);

    /* It seems to do this check before the hkey check */
    if (!lpFile || !*lpFile)
        return ERROR_INVALID_PARAMETER;

    FIXME("(%x,%s,%ld): stub\n",hkey,debugstr_w(lpFile),dwFlags);

    /* Check for file existence */

    return ERROR_SUCCESS;
}


/******************************************************************************
 * RegRestoreKey32A [ADVAPI32.163]
 */
LONG WINAPI RegRestoreKeyA( HKEY hkey, LPCSTR lpFile, DWORD dwFlags )
{
    LONG ret;
    LPWSTR lpFileW = strdupA2W(lpFile);
    ret = RegRestoreKeyW( hkey, lpFileW, dwFlags );
    if(lpFileW) free(lpFileW);
    return ret;
}


/******************************************************************************
 * RegReplaceKey32W [ADVAPI32.162]
 *
 * PARAMS
 *    hkey      [I] Handle of open key
 *    lpSubKey  [I] Address of name of subkey
 *    lpNewFile [I] Address of filename for file with new data
 *    lpOldFile [I] Address of filename for backup file
 */
LONG WINAPI RegReplaceKeyW( HKEY hkey, LPCWSTR lpSubKey, LPCWSTR lpNewFile,
                              LPCWSTR lpOldFile )
{
    FIXME("(%x,%s,%s,%s): stub\n", hkey, debugstr_w(lpSubKey), 
          debugstr_w(lpNewFile),debugstr_w(lpOldFile));
    return ERROR_SUCCESS;
}


/******************************************************************************
 * RegReplaceKey32A [ADVAPI32.161]
 */
LONG WINAPI RegReplaceKeyA( HKEY hkey, LPCSTR lpSubKey, LPCSTR lpNewFile,
                              LPCSTR lpOldFile )
{
    LONG ret;
    LPWSTR lpSubKeyW = strdupA2W(lpSubKey);
    LPWSTR lpNewFileW = strdupA2W(lpNewFile);
    LPWSTR lpOldFileW = strdupA2W(lpOldFile);
    ret = RegReplaceKeyW( hkey, lpSubKeyW, lpNewFileW, lpOldFileW );
    free(lpOldFileW);
    free(lpNewFileW);
    free(lpSubKeyW);
    return ret;
}






/* 16-bit functions */

/* 0 and 1 are valid rootkeys in win16 shell.dll and are used by
 * some programs. Do not remove those cases. -MM
 */
static inline void fix_win16_hkey( HKEY *hkey )
{
    if (*hkey == 0 || *hkey == 1) *hkey = HKEY_CLASSES_ROOT;
}

/******************************************************************************
 *           RegEnumKey16   [KERNEL.216] [SHELL.7]
 */
DWORD WINAPI RegEnumKey16( HKEY hkey, DWORD index, LPSTR name, DWORD name_len )
{
    fix_win16_hkey( &hkey );
    return RegEnumKeyA( hkey, index, name, name_len );
}

/******************************************************************************
 *           RegOpenKey16   [KERNEL.217] [SHELL.1]
 */
DWORD WINAPI RegOpenKey16( HKEY hkey, LPCSTR name, LPHKEY retkey )
{
    fix_win16_hkey( &hkey );
    return RegOpenKeyA( hkey, name, retkey );
}

/******************************************************************************
 *           RegCreateKey16   [KERNEL.218] [SHELL.2]
 */
DWORD WINAPI RegCreateKey16( HKEY hkey, LPCSTR name, LPHKEY retkey )
{
    fix_win16_hkey( &hkey );
    return RegCreateKeyA( hkey, name, retkey );
}

/******************************************************************************
 *           RegDeleteKey16   [KERNEL.219] [SHELL.4]
 */
DWORD WINAPI RegDeleteKey16( HKEY hkey, LPCSTR name )
{
    fix_win16_hkey( &hkey );
    return RegDeleteKeyA( hkey, name );
}

/******************************************************************************
 *           RegCloseKey16   [KERNEL.220] [SHELL.3]
 */
DWORD WINAPI RegCloseKey16( HKEY hkey )
{
    fix_win16_hkey( &hkey );
    return RegCloseKey( hkey );
}

/******************************************************************************
 *           RegSetValue16   [KERNEL.221] [SHELL.5]
 */
DWORD WINAPI RegSetValue16( HKEY hkey, LPCSTR name, DWORD type, LPCSTR data, DWORD count )
{
    fix_win16_hkey( &hkey );
    return RegSetValueA( hkey, name, type, data, count );
}

/******************************************************************************
 *           RegDeleteValue16  [KERNEL.222]
 */
DWORD WINAPI RegDeleteValue16( HKEY hkey, LPSTR name )
{
    fix_win16_hkey( &hkey );
    return RegDeleteValueA( hkey, name );
}

/******************************************************************************
 *           RegEnumValue16   [KERNEL.223]
 */
DWORD WINAPI RegEnumValue16( HKEY hkey, DWORD index, LPSTR value, LPDWORD val_count,
                             LPDWORD reserved, LPDWORD type, LPBYTE data, LPDWORD count )
{
    fix_win16_hkey( &hkey );
    return RegEnumValueA( hkey, index, value, val_count, reserved, type, data, count );
}

/******************************************************************************
 *           RegQueryValue16   [KERNEL.224] [SHELL.6]
 *
 * NOTES
 *    Is this HACK still applicable?
 *
 * HACK
 *    The 16bit RegQueryValue doesn't handle selectorblocks anyway, so we just
 *    mask out the high 16 bit.  This (not so much incidently) hopefully fixes
 *    Aldus FH4)
 */
DWORD WINAPI RegQueryValue16( HKEY hkey, LPCSTR name, LPSTR data, LPDWORD count )
{
    fix_win16_hkey( &hkey );
    if (count) *count &= 0xffff;
    return RegQueryValueA( hkey, name, data, count );
}

/******************************************************************************
 *           RegQueryValueEx16   [KERNEL.225]
 */
DWORD WINAPI RegQueryValueEx16( HKEY hkey, LPCSTR name, LPDWORD reserved, LPDWORD type,
                                LPBYTE data, LPDWORD count )
{
    fix_win16_hkey( &hkey );
    return RegQueryValueExA( hkey, name, reserved, type, data, count );
}

/******************************************************************************
 *           RegSetValueEx16   [KERNEL.226]
 */
DWORD WINAPI RegSetValueEx16( HKEY hkey, LPCSTR name, DWORD reserved, DWORD type,
                              CONST BYTE *data, DWORD count )
{
    fix_win16_hkey( &hkey );
    return RegSetValueExA( hkey, name, reserved, type, data, count );
}
