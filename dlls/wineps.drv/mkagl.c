/*
 * Copyright 2001 Ian Pilcher
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"

#include <sys/types.h>
#include <dirent.h>
#include <string.h>
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

static inline void sort_by_UV(void)
{
    qsort(glyphs, num_glyphs, sizeof(GLYPHINFO), cmp_by_UV);
}

static inline void sort_by_name(void)
{
    qsort(glyphs, num_glyphs, sizeof(GLYPHINFO), cmp_by_name);
}

static inline GLYPHINFO *search_by_name(const char *name)
{
    GLYPHINFO	gi;

    gi.name = name;

    return (GLYPHINFO *)bsearch(&gi, glyphs, num_glyphs, sizeof(GLYPHINFO),
    	    cmp_by_name);
}


/*
 *  Use the 'optimal' combination of tabs and spaces to position the cursor
 */

static inline void fcpto(FILE *f, int newpos, int curpos)
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

/*
 *  Make main() look "purty"
 */

static inline void triple_space(FILE *f)
{
    fputc('\n', f);  fputc('\n', f);
}


/*
 *  Read the Adobe Glyph List from 'glyphlist.txt'
 */

static void read_agl(void)
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

static void read_afms(FILE *f_c, FILE *f_h)
{
    DIR     	    *d = opendir(".");
    struct dirent   *de;

    fputs(  "/*\n"
    	    " *  Built-in font metrics\n"
	    " */\n"
	    "\n"
	    "const AFM *const PSDRV_BuiltinAFMs[] =\n"
	    "{\n", f_c);


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

	cp = strrchr(de->d_name, '.');	    	    	    /* Does it end in   */
	if (cp == NULL || stricmp(cp, ".afm") != 0)   /*   .afm or .AFM?  */
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
	    	fprintf(stderr, "FontName not found in %s\n", de->d_name);
		exit(__LINE__);
	    }

	    if (strncmp(linebuf, "FontName ", 9) == 0)
	    	break;
	}

	sscanf(linebuf, "FontName %[^\r\n]", font_family);

	for (i = 0; font_family[i] != '\0'; ++i)
	    if (font_family[i] == '-')
	    	font_family[i] = '_';

	fprintf(f_h, "extern const AFM PSDRV_%s;\n", font_family);
	fprintf(f_c, "    &PSDRV_%s,\n", font_family);

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

    fputs("    NULL\n};\n", f_c);
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
	    " *\tFont and glyph data for the Wine PostScript driver\n"
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
	    "#include \"psdrv.h\"\n"
	    "#include \"data/agl.h\"\n", f);
}

/*
 *  Write the array of glyph names (also populates indexes)
 */

static void write_glyph_names(FILE *f_c, FILE *f_h)
{
    int i, num_names = 0, index = 0;

    for (i = 0; i < num_glyphs; ++i)
    	if (i == 0 || strcmp(glyphs[i - 1].name, glyphs[i].name) != 0)
	    ++num_names;

    fputs(  "/*\n"
    	    " *  Every glyph name in the AGL and the 35 core PostScript fonts\n"
	    " */\n"
	    "\n", f_c);

    fprintf(f_c, "const INT PSDRV_AGLGlyphNamesSize = %i;\n\n", num_names);

    fprintf(f_c, "GLYPHNAME PSDRV_AGLGlyphNames[%i] =\n{\n", num_names);

    for (i = 0; i < num_glyphs - 1; ++i)
    {
    	int cp = 0;

	if (i == 0 || strcmp(glyphs[i - 1].name, glyphs[i].name) != 0)
	{
	    fcpto(f_h, 32, fprintf(f_h, "#define GN_%s", glyphs[i].name));
	    fprintf(f_h, "(PSDRV_AGLGlyphNames + %i)\n", index);

	    cp = fprintf(f_c, "    { %4i, \"%s\" },", index, glyphs[i].name);
	    glyphs[i].index = index;
	    ++index;
	}
	else
	{
	    glyphs[i].index = glyphs[i - 1].index;
	}

	fcpto(f_c, 40, cp);

	fprintf(f_c, "/* %s */\n", glyphs[i].comment);
    }

    fcpto(f_h, 32, fprintf(f_h, "#define GN_%s", glyphs[i].name));
    fprintf(f_h, "(PSDRV_AGLGlyphNames + %i)\n", index);

    glyphs[i].index = index;
    fcpto(f_c, 40, fprintf(f_c, "    { %4i, \"%s\" }", index, glyphs[i].name));
    fprintf(f_c, "/* %s */\n};\n", glyphs[i].comment);
}


/*
 *  Write the AGL encoding vector, sorted by glyph name
 */

static void write_encoding_by_name(FILE *f)
{
    int i, size = 0, even = 1;

    for (i = 0; i < num_glyphs; ++i)
    	if (glyphs[i].UV != -1 &&
	    	(i == 0 || strcmp(glyphs[i - 1].name, glyphs[i].name) != 0))
	    ++size; 	    	    /* should be 1039 */

    fputs(  "/*\n"
    	    " *  The AGL encoding vector, sorted by glyph name - "
	    	    "duplicates omitted\n"
	    " */\n"
	    "\n", f);

    fprintf(f, "const INT PSDRV_AGLbyNameSize = %i;\n\n", size);
    fprintf(f, "const UNICODEGLYPH PSDRV_AGLbyName[%i] =\n{\n", size);

    for (i = 0; i < num_glyphs - 1; ++i)
    {
    	int cp;

    	if (glyphs[i].UV == -1)
	    continue;

	if (i != 0 && strcmp(glyphs[i - 1].name, glyphs[i].name) == 0)
	    continue;

    	cp = fprintf(f, "    { 0x%.4x, GN_%s },", glyphs[i].UV, glyphs[i].name);

	even = !even;
	if (even)
	    fputc('\n', f);
	else
	    fcpto(f, 40, cp);
    }

    fprintf(f, "    { 0x%.4x, GN_%s }\n};\n", glyphs[i].UV, glyphs[i].name);
}

/*
 *  Write the AGL encoding vector, sorted by Unicode value
 */

static void write_encoding_by_UV(FILE *f)
{
    int i, size = 0, even = 1;

    for (i = 0; i < num_glyphs; ++i)
    	if (glyphs[i].UV != -1)
	    ++size; 	    	    	/* better be 1051! */

    sort_by_UV();

    fputs(  "/*\n"
    	    " *  The AGL encoding vector, sorted by Unicode value - "
	    	    "duplicates included\n"
	    " */\n"
	    "\n", f);

    fprintf(f, "const INT PSDRV_AGLbyUVSize = %i;\n\n", size);
    fprintf(f, "const UNICODEGLYPH PSDRV_AGLbyUV[%i] =\n{\n", size);

    for (i = 0; i < num_glyphs - 1; ++i)
    {
    	int cp;

    	if (glyphs[i].UV == -1)
	    continue;

	cp = fprintf(f, "    { 0x%.4x, GN_%s },", glyphs[i].UV, glyphs[i].name);

	even = !even;
	if (even)
	    fputc('\n', f);
	else
	    fcpto(f, 40, cp);
    }

    fprintf(f, "    { 0x%.4x, GN_%s }\n};\n", glyphs[i].UV, glyphs[i].name);
}


/*
 *  Do it!
 */

int main(int argc, char *argv[])
{
    FILE    *f_c, *f_h;

    if (argc < 3)
    {
    	fprintf(stderr, "Usage: %s <C file> <header file>\n", argv[0]);
	exit(__LINE__);
    }

    f_c = fopen(argv[1], "w");
    if (f_c == NULL)
    {
	fprintf(stderr, "Error opening %s for writing\n", argv[1]);
	exit(__LINE__);
    }

    f_h = fopen(argv[2], "w");
    if (f_h == NULL)
    {
    	fprintf(stderr, "Error opening %s for writing\n", argv[2]);
	exit(__LINE__);
    }

    write_header(f_c);
    triple_space(f_c);
    read_agl();
    read_afms(f_c, f_h);    	    /* also writes font list */
    triple_space(f_c);
    write_glyph_names(f_c, f_h);
    triple_space(f_c);
    write_encoding_by_name(f_c);
    triple_space(f_c);
    write_encoding_by_UV(f_c);

    /* Clean up */
    fclose(f_c);
    fclose(f_h);

    return 0;
}
