static char RCSId[] = "$Id: wine.c,v 1.2 1993/07/04 04:04:21 root Exp root $";
static char Copyright[] = "Copyright  Robert J. Amstadt, 1993";

#include <stdlib.h>
#include <dirent.h>
#include <string.h>

/**********************************************************************
 *					FindFileInPath
 */
char *
FindFileInPath(char *buffer, int buflen, char *rootname, 
	       char **extensions, char *path)
{
    char *workingpath;
    char *dirname;
    DIR *d;
    struct dirent *f;
    char **e;
    int rootnamelen;
    int found = 0;

    if (strchr(rootname, '/') != NULL)
    {
	strncpy(buffer, rootname, buflen);
	return buffer;
    }

    rootnamelen = strlen(rootname);
    workingpath = malloc(strlen(path) + 1);
    if (workingpath == NULL)
	return NULL;
    strcpy(workingpath, path);
    
    for(dirname = strtok(workingpath, ":;"); 
	dirname != NULL;
	dirname = strtok(NULL, ":;"))
    {
	d = opendir(dirname);
	if (d != NULL)
	{
	    while ((f = readdir(d)) != NULL)
	    {
		if (strncasecmp(rootname, f->d_name, rootnamelen) == 0)
		{
		    if (extensions == NULL || 
			strcasecmp(rootname, f->d_name) == 0)
		    {
			found = 1;
		    }
		    else if (f->d_name[rootnamelen] == '.')
		    {
			for (e = extensions; *e != NULL; e++)
			{
			    if (strcasecmp(*e, f->d_name + rootnamelen + 1) 
				== 0)
			    {
				found = 1;
				break;
			    }
			}
		    }
		    
		    if (found)
		    {
			strncpy(buffer, dirname, buflen);
			strncat(buffer, "/", buflen - strlen(buffer));
			strncat(buffer, f->d_name, buflen - strlen(buffer));
			return buffer;
		    }
		}
	    }
	    closedir(d);
	}
    }

    return NULL;
}

/**********************************************************************
 *					GetSystemIniFilename
 */
char *
GetSystemIniFilename()
{
    static char *IniName = NULL;
    char inipath[256];
    
    if (IniName)
	return IniName;

    getcwd(inipath, 256);
    strcat(inipath, ":");
    strcat(inipath, getenv("HOME"));
    strcat(inipath, ":");
    strcat(inipath, getenv("WINEPATH"));
    
    IniName = malloc(1024);
    if (FindFileInPath(IniName, 1024, "wine.ini", NULL, inipath) == NULL)
    {
	free(IniName);
	IniName = NULL;
	return NULL;
    }
    
    IniName = realloc(IniName, strlen(IniName) + 1);
    return IniName;
}
