/*
 * Module/Library loadorder
 *
 * Copyright 1999 Bertho Stultiens
 */

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "windef.h"
#include "options.h"
#include "debug.h"
#include "loadorder.h"
#include "heap.h"
#include "options.h"

DEFAULT_DEBUG_CHANNEL(module)


/* #define DEBUG_LOADORDER */

#define LOADORDER_ALLOC_CLUSTER	32	/* Allocate with 32 entries at a time */

static module_loadorder_t default_loadorder;
static module_loadorder_t *module_loadorder = NULL;
static int nmodule_loadorder = 0;
static int nmodule_loadorder_alloc = 0;

/***************************************************************************
 *	cmp_sort_func	(internal, static)
 *
 * Sorting and comparing function used in sort and search of loadorder
 * entries.
 */
static int cmp_sort_func(const void *s1, const void *s2)
{
	return strcasecmp(((module_loadorder_t *)s1)->modulename, ((module_loadorder_t *)s2)->modulename);
}


/***************************************************************************
 *	get_tok	(internal, static)
 *
 * strtok wrapper for non-destructive buffer writing.
 * NOTE: strtok is not reentrant and therefore this code is neither.
 */
static char *get_tok(const char *str, const char *delim)
{
	static char *buf = NULL;
	char *cptr;

	if(!str && (!buf || !index))
		return NULL;

	if(str && buf)
	{
		HeapFree(SystemHeap, 0, buf);
		buf = NULL;
	}

	if(str && !buf)
	{
		buf = HEAP_strdupA(SystemHeap, 0, str);
		cptr = strtok(buf, delim);
	}
	else
	{
		cptr = strtok(NULL, delim);
	}

	if(!cptr)
	{
		HeapFree(SystemHeap, 0, buf);
		buf = NULL;
	}
	return cptr;
}


/***************************************************************************
 *	ParseLoadOrder	(internal, static)
 *
 * Parses the loadorder options from the configuration and puts it into
 * a structure.
 */
static BOOL ParseLoadOrder(char *order, module_loadorder_t *mlo)
{
	char *cptr;
	int n = 0;

	memset(mlo->loadorder, 0, sizeof(mlo->loadorder));

	cptr = get_tok(order, ", \t");
	while(cptr)
	{
		char type = MODULE_LOADORDER_INVALID;

		if(n >= MODULE_LOADORDER_NTYPES)
		{
			ERR(module, "More than existing %d module-types specified, rest ignored", MODULE_LOADORDER_NTYPES);
			break;
		}

		switch(*cptr)
		{
		case 'N':	/* Native */
		case 'n': type = MODULE_LOADORDER_DLL; break;

		case 'E':	/* Elfdll */
		case 'e': type = MODULE_LOADORDER_ELFDLL; break;

		case 'S':	/* So */
		case 's': type = MODULE_LOADORDER_SO; break;

		case 'B':	/* Builtin */
		case 'b': type = MODULE_LOADORDER_BI; break;

		default:
			ERR(module, "Invalid load order module-type '%s', ignored\n", cptr);
		}

		if(type != MODULE_LOADORDER_INVALID)
		{
			mlo->loadorder[n++] = type;
		}
		cptr = get_tok(NULL, ", \t");
	}
	return TRUE;
}


/***************************************************************************
 *	AddLoadOrder	(internal, static)
 *
 * Adds an entry in the list of overrides. If the entry exists then the
 * override parameter determines whether it will be overwriten.
 */
static BOOL AddLoadOrder(module_loadorder_t *plo, BOOL override)
{
	int i;

	/* TRACE(module, "'%s' -> %08lx\n", plo->modulename, *(DWORD *)(plo->loadorder)); */

	for(i = 0; i < nmodule_loadorder; i++)
	{
		if(!cmp_sort_func(plo, &module_loadorder[i]))
		{
			if(!override)
				ERR(module, "Module '%s' is already in the list of overrides, using first definition\n", plo->modulename);
			else
				memcpy(module_loadorder[i].loadorder, plo->loadorder, sizeof(plo->loadorder));
			return TRUE;
		}
	}

	if(nmodule_loadorder >= nmodule_loadorder_alloc)
	{
		/* No space in current array, make it larger */
		nmodule_loadorder_alloc += LOADORDER_ALLOC_CLUSTER;
		module_loadorder = (module_loadorder_t *)HeapReAlloc(SystemHeap,
								     0,
								     module_loadorder,
								     nmodule_loadorder_alloc * sizeof(module_loadorder_t));
		if(!module_loadorder)
		{
			MSG("Virtual memory exhausted\n");
			exit(1);
		}
	}
	memcpy(module_loadorder[nmodule_loadorder].loadorder, plo->loadorder, sizeof(plo->loadorder));
	module_loadorder[nmodule_loadorder].modulename = HEAP_strdupA(SystemHeap, 0, plo->modulename);
	nmodule_loadorder++;
	return TRUE;
}


/***************************************************************************
 *	AddLoadOrderSet	(internal, static)
 *
 * Adds an set of entries in the list of overrides from the key parameter.
 * If the entry exists then the override parameter determines whether it
 * will be overwriten.
 */
static BOOL AddLoadOrderSet(char *key, char *order, BOOL override)
{
	module_loadorder_t ldo;
	char *cptr;

	/* Parse the loadorder before the rest because strtok is not reentrant */
	if(!ParseLoadOrder(order, &ldo))
		return FALSE;

	cptr = get_tok(key, ", \t");
	while(cptr)
	{
		if(strchr(cptr, '.'))
			MSG("Warning: Loadorder override '%s' contains an extension and might not be found during lookup\n", cptr);

		ldo.modulename = cptr;
		if(!AddLoadOrder(&ldo, override))
			return FALSE;
		cptr = get_tok(NULL, ", \t");
	}
	return TRUE;
}


/***************************************************************************
 *	ParseCommandlineOverrides	(internal, static)
 *
 * The commandline is in the form:
 * name[,name,...]=native[,b,...][:...]
 */
static BOOL ParseCommandlineOverrides(void)
{
	char *cpy;
	char *key;
	char *next;
	char *value;
	BOOL retval = TRUE;

	if(!Options.dllFlags)
		return TRUE;

	cpy = HEAP_strdupA(SystemHeap, 0, Options.dllFlags);
	key = cpy;
	next = key;
	for(; next; key = next)
	{
		next = strchr(key, ':');
		if(next)
		{
			*next = '\0';
			next++;
		}
		value = strchr(key, '=');
		if(!value)
		{
			retval = FALSE;
			goto endit;
		}
		*value = '\0';
		value++;

		TRACE(module, "Commandline override '%s' = '%s'\n", key, value);
		
		if(!AddLoadOrderSet(key, value, TRUE))
		{
			retval = FALSE;
			goto endit;
		}
	}
endit:
	HeapFree(SystemHeap, 0, cpy);
	return retval;;
}


/***************************************************************************
 *	MODULE_InitLoadOrder	(internal)
 *
 * Initialize the load order from the wine.conf file.
 * The section has tyhe following format:
 * Section:
 *	[DllDefaults]
 *
 * Keys:
 *	EXTRA_LD_LIBRARY_PATH=/usr/local/lib/wine[:/more/path/to/search[:...]]
 * The path will be appended to any existing LD_LIBRARY_PATH from the 
 * environment (see note in code below).
 *
 *	DefaultLoadOrder=native,elfdll,so,builtin
 * A comma seperated list of module-types to try to load in that specific
 * order. The DefaultLoadOrder key is used as a fallback when a module is
 * not specified explicitely. If the DefaultLoadOrder key is not found, 
 * then the order "dll,elfdll,so,bi" is used
 * The possible module-types are:
 *	- native	Native windows dll files
 *	- elfdll	Dlls encapsulated in .so libraries
 *	- so		Native .so libraries mapped to dlls
 *	- builtin	Built-in modules
 *
 * Case is not important and only the first letter of each type is enough to
 * identify the type n[ative], e[lfdll], s[o], b[uiltin]. Also whitespace is
 * ignored.
 * E.g.:
 * 	n,el	,s , b
 * is equal to:
 *	native,elfdll,so,builtin
 *
 * Section:
 *	[DllOverrides]
 *
 * Keys:
 * There are no explicit keys defined other than module/library names. A comma
 * separated list of modules is followed by an assignment of the load-order
 * for these specific modules. See above for possible types. You should not
 * specify an extension.
 * Examples:
 * kernel32, gdi32, user32 = builtin
 * kernel, gdi, user = builtin
 * comdlg32 = elfdll, native, builtin
 * commdlg = native, builtin
 * version, ver = elfdll, native, builtin
 *
 * Section:
 *	[DllPairs]
 *
 * Keys:
 * This is a simple pairing in the form 'name1 = name2'. It is supposed to
 * identify the dlls that cannot live without eachother unless they are
 * loaded in the same format. Examples are common dialogs and controls,
 * shell, kernel, gdi, user, etc...
 * The code will issue a warning if the loadorder of these pairs are different
 * and might cause hard-to-find bugs due to incompatible pairs loaded at
 * run-time. Note that this pairing gives *no* guarantee that the pairs
 * actually get loaded as the same type, nor that the correct versions are
 * loaded (might be implemented later). It merely notes obvious trouble.
 * Examples:
 * kernel = kernel32
 * commdlg = comdlg32
 *
 */

#define BUFFERSIZE	1024

BOOL MODULE_InitLoadOrder(void)
{
	char buffer[BUFFERSIZE];
	int nbuffer;

	/* Get/set the new LD_LIBRARY_PATH */
	nbuffer = PROFILE_GetWineIniString("DllDefaults", "EXTRA_LD_LIBRARY_PATH", "", buffer, sizeof(buffer));

	if(nbuffer)
	{
		extern char *_dl_library_path;
		char *ld_lib_path = getenv("LD_LIBRARY_PATH");
		if(ld_lib_path)
		{
			/*
			 * Append new path to current
			 */
			char *tmp = HEAP_strdupA(SystemHeap, 0, buffer);
			sprintf(buffer, "%s:%s", ld_lib_path, tmp);
			HeapFree( SystemHeap, 0, tmp );
		}

		TRACE(module, "Setting new LD_LIBRARY_PATH=%s\n", buffer);

		setenv("LD_LIBRARY_PATH", buffer, 1);

		/*
		 * This is a cruel hack required to have libdl check this path.
		 * The problem is that libdl caches the environment variable
		 * and we won't get our modifications applied. We ensure the
		 * the correct search path by explicitely modifying the libdl
		 * internal variable which holds the path.
		 */
		_dl_library_path = HEAP_strdupA(SystemHeap, 0, buffer);
	}

	/* Get the default load order */
	nbuffer = PROFILE_GetWineIniString("DllDefaults", "DefaultLoadOrder", "n,e,s,b", buffer, sizeof(buffer));
	if(!nbuffer)
	{
		MSG("MODULE_InitLoadOrder: misteriously read nothing from default loadorder\n");
		return FALSE;
	}

	TRACE(module, "Setting default loadorder=%s\n", buffer);

	if(!ParseLoadOrder(buffer, &default_loadorder))
		return FALSE;
	default_loadorder.modulename = "<none>";

	/* Read the explicitely defined orders for specific modules as an entire section */
	nbuffer = PROFILE_GetWineIniString("DllOverrides", NULL, "", buffer, sizeof(buffer));
	if(nbuffer == BUFFERSIZE-2)
	{
		ERR(module, "BUFFERSIZE %d is too small to read [DllOverrides]. Needs to grow in the source\n", BUFFERSIZE);
		return FALSE;
	}
	if(nbuffer)
	{
		/* We only have the keys in the buffer, not the values */
		char *key;
		char value[BUFFERSIZE];
		char *next;

		for(key = buffer; *key; key = next)
		{
			next = key + strlen(key) + 1;

			nbuffer = PROFILE_GetWineIniString("DllOverrides", key, "", value, sizeof(value));
			if(!nbuffer)
			{
				ERR(module, "Module(s) '%s' will always fail to load. Are you sure you want this?\n", key);
				value[0] = '\0';	/* Just in case */
			}
			if(nbuffer == BUFFERSIZE-2)
			{
				ERR(module, "BUFFERSIZE %d is too small to read [DllOverrides] key '%s'. Needs to grow in the source\n", BUFFERSIZE, key);
				return FALSE;
			}

			TRACE(module, "Key '%s' uses override '%s'\n", key, value);

			if(!AddLoadOrderSet(key, value, FALSE))
				return FALSE;
		}
	}

	/* Add the commandline overrides to the pool */
	if(!ParseCommandlineOverrides())
	{
		MSG(	"Syntax: -dll name[,name[,...]]={native|elfdll|so|builtin}[,{n|e|s|b}[,...]][:...]\n"
			"    - 'name' is the name of any dll without extension\n"
			"    - the order of loading (native, elfdll, so and builtin) can be abbreviated\n"
			"      with the first letter\n"
			"    - different loadorders for different dlls can be specified by seperating the\n"
			"      commandline entries with a ':'\n"
			"    Example:\n"
			"    -dll comdlg32,commdlg=n:shell,shell32=b\n"
		   );
		return FALSE;
	}

	/* Sort the array for quick lookup */
	qsort(module_loadorder, nmodule_loadorder, sizeof(module_loadorder[0]), cmp_sort_func);

	/* Check the pairs of dlls */
	nbuffer = PROFILE_GetWineIniString("DllPairs", NULL, "", buffer, sizeof(buffer));
	if(nbuffer == BUFFERSIZE-2)
	{
		ERR(module, "BUFFERSIZE %d is too small to read [DllPairs]. Needs to grow in the source\n", BUFFERSIZE);
		return FALSE;
	}
	if(nbuffer)
	{
		/* We only have the keys in the buffer, not the values */
		char *key;
		char value[BUFFERSIZE];
		char *next;

		for(key = buffer; *key; key = next)
		{
			module_loadorder_t *plo1, *plo2;

			next = key + strlen(key) + 1;

			nbuffer = PROFILE_GetWineIniString("DllPairs", key, "", value, sizeof(value));
			if(!nbuffer)
			{
				ERR(module, "Module pair '%s' is not associated with another module?\n", key);
				continue;
			}
			if(nbuffer == BUFFERSIZE-2)
			{
				ERR(module, "BUFFERSIZE %d is too small to read [DllPairs] key '%s'. Needs to grow in the source\n", BUFFERSIZE, key);
				return FALSE;
			}

			plo1 = MODULE_GetLoadOrder(key);
			plo2 = MODULE_GetLoadOrder(value);
			assert(plo1 && plo2);

			if(memcmp(plo1->loadorder, plo2->loadorder, sizeof(plo1->loadorder)))
				MSG("Warning: Modules '%s' and '%s' have different loadorder which may cause trouble\n", key, value);
		}
	}

	if(TRACE_ON(module))
	{
		int i, j;
		static char types[6] = "-NESB";

		for(i = 0; i < nmodule_loadorder; i++)
		{
			DPRINTF("%3d: %-12s:", i, module_loadorder[i].modulename);
			for(j = 0; j < MODULE_LOADORDER_NTYPES; j++)
				DPRINTF(" %c", types[module_loadorder[i].loadorder[j] % (MODULE_LOADORDER_NTYPES+1)]);
			DPRINTF("\n");
		}
	}

	return TRUE;
}


/***************************************************************************
 *	MODULE_GetLoadOrder	(internal)
 *
 * Locate the loadorder of a module.
 * Any path is stripped from the path-argument and so are the extension
 * '.dll' and '.exe'. A lookup in the table can yield an override for the
 * specific dll. Otherwise the default load order is returned.
 */
module_loadorder_t *MODULE_GetLoadOrder(const char *path)
{
	module_loadorder_t lo, *tmp;
	char fname[256];
	char *cptr;
	char *name;
	int len;

	assert(path != NULL);

	/* Strip path information */
	cptr = strrchr(path, '\\');
	if(!cptr)
		name = strrchr(path, '/');
	else
		name = strrchr(cptr, '/');

	if(!name)
		name = cptr ? cptr+1 : (char *)path;
	else
		name++;

	if((cptr = strchr(name, ':')) != NULL)	/* Also strip drive if in format 'C:MODULE.DLL' */
		name = cptr+1;

	len = strlen(name);
	if(len >= sizeof(fname) || len <= 0)
	{
		ERR(module, "Path '%s' -> '%s' reduces to zilch or just too large...\n", path, name);
		return &default_loadorder;
	}

	strcpy(fname, name);
	if(len >= 4 && (!lstrcmpiA(fname+len-4, ".dll") || !lstrcmpiA(fname+len-4, ".exe")))
		fname[len-4] = '\0';

	lo.modulename = fname;
	tmp = bsearch(&lo, module_loadorder, nmodule_loadorder, sizeof(module_loadorder[0]), cmp_sort_func);

	TRACE(module, "Looking for '%s' (%s), found '%s'\n", path, fname, tmp ? tmp->modulename : "<nothing>");

	if(!tmp)
		return &default_loadorder;
	return tmp;
}

