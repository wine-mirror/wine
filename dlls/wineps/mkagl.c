#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <malloc.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>


/*
 *  The array of glyph information
 */
 
typedef struct
{
    int     	UV;
    int     	index;	    	/* in PSDRV_AGLGlyphNames */
    const char	*name;
    const char	*comment;

} GLYPHINFO;

static GLYPHINFO    glyphs[1500];
static int  	    num_glyphs = 0;


/*
 *  Functions to search and sort the array
 */
 
static int cmp_by_UV(const void *a, const void *b)
{
    return ((const GLYPHINFO *)a)->UV - ((const GLYPHINFO *)b)->UV;
}

static int cmp_by_name(const void *a, const void *b)
{
    return strcmp(((const GLYPHINFO *)a)->name, ((const GLYPHINFO *)b)->name);
}

inline static void sort_by_UV()
{
    qsort(glyphs, num_glyphs, sizeof(GLYPHINFO), cmp_by_UV);
}

inline static void sort_by_name()
{
    qsort(glyphs, num_glyphs, sizeof(GLYPHINFO), cmp_by_name);
}

inline static GLYPHINFO *search_by_name(const char *name)
{
    GLYPHINFO	gi;
    
    gi.name = name;
    
    return (GLYPHINFO *)bsearch(&gi, glyphs, num_glyphs, sizeof(GLYPHINFO),
    	    cmp_by_name);
}


/*
 *  Use the 'optimal' combination of tabs and spaces to position the cursor
 */
 
inline static void fcpto(FILE *f, int newpos, int curpos)
{
    int newtpos = newpos & ~7;
    int curtpos = curpos & ~7;
    
    while (curtpos < newtpos)
    {
    	fputc('\t', f);
	curtpos += 8;
	curpos = curtpos;
    }
    
    while (curpos < newpos)
    {
    	fputc(' ', f);
	++curpos;
    }
}

inline static void cpto(int newpos, int curpos)
{
    fcpto(stdout, newpos, curpos);
}


/*
 *  Make main() look "purty"
 */
 
inline static void double_space(FILE *f)
{
    fputc('\n', f);
}

inline static void triple_space(FILE *f)
{
    fputc('\n', f);  fputc('\n', f);
}


/*
 *  Read the Adobe Glyph List from 'glyphlist.txt'
 */
 
static void read_agl()
{
    FILE    *f = fopen("glyphlist.txt", "r");
    char    linebuf[256], namebuf[128], commbuf[128];
    
    if (f == NULL)
    {
    	fprintf(stderr, "Error opening glyphlist.txt\n");
	exit(__LINE__);
    }
    
    while (fgets(linebuf, sizeof(linebuf), f) != NULL)
    {
    	unsigned int	UV;
    
    	if (linebuf[0] == '#')
	    continue;
	    
    	sscanf(linebuf, "%X;%[^;];%[^\n]", &UV, namebuf, commbuf);
	
	glyphs[num_glyphs].UV = (int)UV;
	glyphs[num_glyphs].name = strdup(namebuf);
	glyphs[num_glyphs].comment = strdup(commbuf);
	
	if (glyphs[num_glyphs].name == NULL ||
	    	glyphs[num_glyphs].comment == NULL)
	{
	    fprintf(stderr, "Memory allocation failure\n");
	    exit(__LINE__);
	}
	
	++num_glyphs;
    }
    
    fclose(f);
    
    if (num_glyphs != 1051)
    {
    	fprintf(stderr, "Read %i glyphs\n", num_glyphs);
	exit(__LINE__);
    }
}


/*
 *  Read glyph names from all AFM files in current directory
 */
 
static void read_afms()
{
    DIR     	    *d = opendir(".");
    struct dirent   *de;
    
    if (d == NULL)
    {
    	fprintf(stderr, "Error opening current directory\n");
	exit(__LINE__);
    }
    
    while ((de = readdir(d)) != NULL)
    {
    	FILE   	*f;
	char   	*cp, linebuf[256], font_family[128];
	int	i, num_metrics;
	
	cp = strrchr(de->d_name, '.');	    	    	/* Does it end in   */
	if (cp == NULL || strcasecmp(cp, ".afm") != 0)	/*   .afm or .AFM?  */
	    continue;
	    
	f = fopen(de->d_name, "r");
	if (f == NULL)
	{
	    fprintf(stderr, "Error opening %s\n", de->d_name);
	    exit(__LINE__);
	}
	
	while (1)
	{
	    if (fgets(linebuf, sizeof(linebuf), f) == NULL)
	    {
	    	fprintf(stderr, "FamilyName not found in %s\n", de->d_name);
		exit(__LINE__);
	    }
	    
	    if (strncmp(linebuf, "FamilyName ", 11) == 0)
	    	break;
	}
	
	sscanf(linebuf, "FamilyName %[^\r\n]", font_family);
	
	while (1)
	{
	    if (fgets(linebuf, sizeof(linebuf), f) == NULL)
	    {
	    	fprintf(stderr, "StartCharMetrics not found in %s\n",
		    	de->d_name);
		exit(__LINE__);
	    }
	    
	    if (strncmp(linebuf, "StartCharMetrics ", 17) == 0)
	    	break;
	}
	
	sscanf(linebuf, "StartCharMetrics %i", &num_metrics);
	
	for (i = 0; i < num_metrics; ++i)
	{
	    char    namebuf[128];
	
	    if (fgets(linebuf, sizeof(linebuf), f) == NULL)
	    {
	    	fprintf(stderr, "Unexpected EOF after %i glyphs in %s\n", i,
		    	de->d_name);
		exit(__LINE__);
	    }
	    
	    cp = strchr(linebuf, 'N');
	    if (cp == NULL || strlen(cp) < 3)
	    {
	    	fprintf(stderr, "Parse error after %i glyphs in %s\n", i,
		    	de->d_name);
		exit(__LINE__);
	    }
	    
	    sscanf(cp, "N %s", namebuf);
	    if (search_by_name(namebuf) != NULL)
	    	continue;
		
	    sprintf(linebuf, "FONT FAMILY;%s", font_family);
		
	    glyphs[num_glyphs].UV = -1;
	    glyphs[num_glyphs].name = strdup(namebuf);
	    glyphs[num_glyphs].comment = strdup(linebuf);
	    
	    if (glyphs[num_glyphs].name == NULL ||
	    	    glyphs[num_glyphs].comment == NULL)
	    {
	    	fprintf(stderr, "Memory allocation failure\n");
		exit(__LINE__);
	    }
	    
	    ++num_glyphs;
	    
	    sort_by_name();
	}
	
	fclose(f);
    }
    
    closedir(d);
}


/*
 *  Write opening comments, etc.
 */
 
static void write_header(FILE *f)
{
    int i;

    fputc('/', f);
    for (i = 0; i < 79; ++i)
    	fputc('*', f);
    fputs("\n"
    	    " *\n"
	    " *\tAdobe Glyph List data for the Wine PostScript driver\n"
    	    " *\n"
	    " *\tCopyright 2001 Ian Pilcher\n"
	    " *\n"
	    " *\n"
	    " *\tThis data is derived from the Adobe Glyph list at\n"
    	    " *\n"
	    " *\t    "
	    "http://partners.adobe.com/asn/developer/type/glyphlist.txt\n"
	    " *\n"
	    " *\tand the Adobe Font Metrics files at\n"
	    " *\n"
	    " *\t    "
	    "ftp://ftp.adobe.com/pub/adobe/type/win/all/afmfiles/base35/\n"
	    " *\n"
	    " *\twhich are Copyright 1985-1998 Adobe Systems Incorporated.\n"
	    " *\n"
	    " */\n"
	    "\n"
	    "#include \"psdrv.h\"\n", f);
}

/*
 *  Write the array of GLYPHNAME structures (also populates indexes)
 */
 
static void write_glyph_names(FILE *f)
{
    int i, num_names = 0, index = 0;
    
    for (i = 0; i < num_glyphs; ++i)
    	if (i == 0 || strcmp(glyphs[i - 1].name, glyphs[i].name) != 0)
	    ++num_names;
    
    fputs(  "/*\n"
    	    " *  Every glyph name in the AGL and the 39 core PostScript fonts\n"
	    " */\n"
	    "\n", f);
	    
    fprintf(f, "const INT PSDRV_AGLGlyphNamesSize = %i;\n\n", num_names);
    
    fprintf(f, "GLYPHNAME PSDRV_AGLGlyphNames[%i] =\n{\n", num_names);
    
    for (i = 0; i < num_glyphs - 1; ++i)
    {
    	int cp = 0;
	
	if (i == 0 || strcmp(glyphs[i - 1].name, glyphs[i].name) != 0)
	{
	    cp = fprintf(f, "    { -1, \"%s\" },", glyphs[i].name);
	    glyphs[i].index = index;
	    ++index;
	}
	else
	{
	    glyphs[i].index = index - 1;
	}
	
	fcpto(f, 36, cp);
	
	fprintf(f, "/* %s */\n", glyphs[i].comment);
    }
    
    glyphs[i].index = index;
    fcpto(f, 36, fprintf(f, "    { -1, \"%s\" }", glyphs[i].name));
    fprintf(f, "/* %s */\n};\n", glyphs[i].comment);
}


/*
 *  Write the AGL encoding vector
 */
 
static void write_encoding(FILE *f)
{
    int i, size = 0;
    
    for (i = 0; i < num_glyphs; ++i)
    	if (glyphs[i].UV != -1)
	    ++size; 	    	    	/* better be 1051! */
	    
    sort_by_UV();
	    
    fputs(  "/*\n"
    	    " *  The AGL encoding vector, sorted by Unicode value\n"
	    " */\n"
	    "\n", f);
	    
    fprintf(f, "static const UNICODEGLYPH encoding[%i] = \n{\n", size);
    
    for (i = 0; i < num_glyphs - 1; ++i)
    {
    	if (glyphs[i].UV == -1)
	    continue;
	    
	fprintf(f, "    { 0x%.4x, PSDRV_AGLGlyphNames + %4i },\t/* %s */\n",
	    	glyphs[i].UV, glyphs[i].index, glyphs[i].name);
    }
    
    fprintf(f, "    { 0x%.4x, PSDRV_AGLGlyphNames + %4i }\t/* %s */\n};\n\n",
    	    glyphs[i].UV, glyphs[i].index, glyphs[i].name);
	    
    fprintf(f, "UNICODEVECTOR PSDRV_AdobeGlyphList = { %i, encoding };\n",
    	    size);
}
    

/*
 *  Do it!
 */
 
int main(int argc, char *argv[])
{
    FILE    *f;

    read_agl();
    read_afms();
    
    if (argc < 2)
    {
    	f = stdout;
    }
    else
    {
    	f = fopen(argv[1], "w");
	if (f == NULL)
	{
	    fprintf(stderr, "Error opening %s for writing\n", argv[1]);
	    exit(__LINE__);
	}
    }
    
    write_header(f);
    triple_space(f);
    write_glyph_names(f);
    triple_space(f);
    write_encoding(f);
    
    return 0;
}
