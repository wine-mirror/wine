/*******************************************************************************
 *
 *  Function to write WINEPS AFM data structures as C
 *
 *  Copyright 2001 Ian Pilcher
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
 *
 * NOTES:
 *
 *  PSDRV_AFM2C(AFM *afm) writes the AFM data structure addressed by afm (and
 *  its subsidiary objects) as a C file which can which can then be built in to
 *  the driver.  It creates the file in the current directory with a name of
 *  the form {FontName}.c, where {FontName} is the PostScript font name with
 *  hyphens replaced by underscores.
 *
 *  To use this function, do the following:
 *
 *  	*  Edit dlls/wineps/Makefile (or dlls/wineps/Makefile.in) and add
 *  	   afm2c.c as a source file.
 *
 *  	*  Edit dlls/wineps/afm.c and uncomment the call to PSDRV_AFM2C in
 *  	   PSDRV_DumpFontList() (or wherever it gets moved).  The resulting
 *  	   compiler warning can be safely ignored.
 *
 *  IMPORTANT:  For this to work, all glyph names in the AFM data being
 *  	written *MUST* already be present in PSDRV_AGLGlyphNames in agl.c.
 *  	See mkagl.c in this directory for information on how to generate
 *  	updated glyph name information.  Note, however, that if the glyph
 *  	name information in agl.c is regenerated, *ALL* AFM data must also
 *  	be recreated.
 *
 */

#include <string.h>
#include <stdio.h>
#include <math.h>

#include "wine/debug.h"
#include "psdrv.h"

WINE_DEFAULT_DEBUG_CHANNEL(psdrv);

static inline void cursorto(FILE *of, int np, int cp)
{
    int ntp = np & 0xfffffff8;
    int ctp = cp & 0xfffffff8;

    while (ctp < ntp)
    {
    	fputc('\t', of);
	ctp += 8;
	cp = ctp;
    }

    while (cp < np)
    {
    	fputc(' ', of);
	++cp;
    }
}

static void writeHeader(FILE *of, const AFM *afm, const char *buffer)
{
    int i;

    fputc('/', of);
    for (i = 1; i < 80; ++i)
    	fputc('*', of);
    fprintf(of, "\n"
    	    	" *\n"
		" *\tFont metric data for %S\n"
		" *\n"
		" *\tCopyright 2001 Ian Pilcher\n"
		" *\n"
		" *\n"
		" *\tSee dlls/wineps/data/COPYRIGHTS for font copyright "
		    	"information.\n"
		" *\n"
		" */\n"
		"\n"
		"#include \"psdrv.h\"\n"
		"#include \"data/agl.h\"\n", afm->FullName);
}

static void writeMetrics(FILE *of, const AFM *afm, const char *buffer)
{
    int i;

    fputs("\n\n/*\n *  Glyph metrics\n */\n\n", of);

    fprintf(of, "static const AFMMETRICS metrics[%i] =\n{\n",
    	    afm->NumofMetrics);

    for (i = 0; i < afm->NumofMetrics - 1; ++i)
    {
    	fprintf(of, "    { %3i, 0x%.4lx, %4g, GN_%s },\n", afm->Metrics[i].C,
	    	afm->Metrics[i].UV, afm->Metrics[i].WX, afm->Metrics[i].N->sz);
    }

    fprintf(of, "    { %3i, 0x%.4lx, %4g, GN_%s }\n};\n", afm->Metrics[i].C,
    	    afm->Metrics[i].UV, afm->Metrics[i].WX, afm->Metrics[i].N->sz);
}

static void writeAFM(FILE *of, const AFM *afm, const char *buffer)
{
    fputs("\n\n/*\n *  Font metrics\n */\n\n", of);

    fprintf(of, "const AFM PSDRV_%s =\n{\n", buffer);
    cursorto(of, 44, fprintf(of, "    \"%s\",", afm->FontName));
    fputs("/* FontName */\n", of);
    cursorto(of, 44, fprintf(of, "    L\"%S\",", afm->FullName));
    fputs("/* FullName */\n", of);
    cursorto(of, 44, fprintf(of, "    L\"%S\",", afm->FamilyName));
    fputs("/* FamilyName */\n", of);
    cursorto(of, 44, fprintf(of, "    L\"%S\",", afm->EncodingScheme));
    fputs("/* EncodingScheme */\n", of);
    cursorto(of, 44, fprintf(of, "    %s,",
    	    (afm->Weight > 550) ? "FW_BOLD" : "FW_NORMAL"));
    fputs("/* Weight */\n", of);
    cursorto(of, 44, fprintf(of, "    %g,", afm->ItalicAngle));
    fputs("/* ItalicAngle */\n", of);
    cursorto(of, 44, fprintf(of, "    %s,",
    	    afm->IsFixedPitch ? "TRUE" : "FALSE"));
    fputs("/* IsFixedPitch */\n", of);
    cursorto(of, 44, fprintf(of, "    %g,", afm->UnderlinePosition));
    fputs("/* UnderlinePosition */\n", of);
    cursorto(of, 44, fprintf(of, "    %g,", afm->UnderlineThickness));
    fputs("/* UnderlineThickness */\n", of);
    cursorto(of, 44, fprintf(of, "    { %g, %g, %g, %g },", afm->FontBBox.llx,
    	    afm->FontBBox.lly, afm->FontBBox.urx, afm->FontBBox.ury));
    fputs("/* FontBBox */\n", of);
    cursorto(of, 44, fprintf(of, "    %g,", afm->Ascender));
    fputs("/* Ascender */\n", of);
    cursorto(of, 44, fprintf(of, "    %g,", afm->Descender));
    fputs("/* Descender */\n", of);
    fputs("    {\n", of);
    cursorto(of, 44, 7 + fprintf(of, "\t%u,",
    	    (unsigned int)(afm->WinMetrics.usUnitsPerEm)));
    fputs("/* WinMetrics.usUnitsPerEm */\n", of);
    cursorto(of, 44, 7 + fprintf(of, "\t%i,",
    	    (int)(afm->WinMetrics.sAscender)));
    fputs("/* WinMetrics.sAscender */\n", of);
    cursorto(of, 44, 7 + fprintf(of, "\t%i,",
    	    (int)(afm->WinMetrics.sDescender)));
    fputs("/* WinMetrics.sDescender */\n", of);
    cursorto(of, 44, 7 + fprintf(of, "\t%i,",
    	    (int)(afm->WinMetrics.sLineGap)));
    fputs("/* WinMetrics.sLineGap */\n", of);
    cursorto(of, 44, 7 + fprintf(of, "\t%i,",
    	    (int)(afm->WinMetrics.sAvgCharWidth)));
    fputs("/* WinMetrics.sAvgCharWidth */\n", of);
    cursorto(of, 44, 7 + fprintf(of, "\t%i,",
    	    (int)(afm->WinMetrics.sTypoAscender)));
    fputs("/* WinMetrics.sTypoAscender */\n", of);
    cursorto(of, 44, 7 + fprintf(of, "\t%i,",
    	    (int)(afm->WinMetrics.sTypoDescender)));
    fputs("/* WinMetrics.sTypoDescender */\n", of);
    cursorto(of, 44, 7 + fprintf(of, "\t%i,",
    	    (int)(afm->WinMetrics.sTypoLineGap)));
    fputs("/* WinMetrics.sTypoLineGap */\n", of);
    cursorto(of, 44, 7 + fprintf(of, "\t%u,",
    	    (unsigned int)(afm->WinMetrics.usWinAscent)));
    fputs("/* WinMetrics.usWinAscent */\n", of);
    cursorto(of, 44, 7 + fprintf(of, "\t%u",
    	    (unsigned int)(afm->WinMetrics.usWinDescent)));
    fputs("/* WinMetrics.usWinDescent */\n",of);
    fputs("    },\n", of);
    cursorto(of, 44, fprintf(of, "    %i,", afm->NumofMetrics));
    fputs("/* NumofMetrics */\n", of);
    fputs("    metrics\t\t\t\t    /* Metrics */\n};\n", of);
}

void PSDRV_AFM2C(const AFM *afm)
{
    char    buffer[256];
    FILE    *of;
    unsigned int i;

    lstrcpynA(buffer, afm->FontName, sizeof(buffer) - 2);

    for (i = 0; i < strlen(buffer); ++i)
    	if (buffer[i] == '-')
	    buffer[i] = '_';

    buffer[i] = '.';  buffer[i + 1] = 'c';  buffer[i + 2] = '\0';

    MESSAGE("writing '%s'\n", buffer);

    of = fopen(buffer, "w");
    if (of == NULL)
    {
    	ERR("error opening '%s' for writing\n", buffer);
	return;
    }

    buffer[i] = '\0';

    writeHeader(of, afm, buffer);
    writeMetrics(of, afm, buffer);
    writeAFM(of, afm, buffer);

    fclose(of);
}
