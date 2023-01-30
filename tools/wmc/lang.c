/*
 * Wine Message Compiler language and codepage support
 *
 * Copyright 2000 Bertho A. Stultiens (BS)
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "wmc.h"
#include "lang.h"


/*
 * Languages supported
 *
 * MUST be sorting ascending on language ID
 */
static const struct language languages[] = {

	{0x0000, 437, "Neutral", NULL},
	{0x0002, 866, "Bulgarian", NULL},
	{0x0003, 850, "Catalan", NULL},
	{0x0005, 852, "Czech", NULL},
	{0x0006, 850, "Danish", NULL},
	{0x0007, 850, "German", NULL},
	{0x0008, 737, "Greek", NULL},
	{0x0009, 437, "English", NULL},
	{0x000A, 850, "Spanish - Traditional Sort", NULL},
	{0x000B, 850, "Finnish", NULL},
	{0x000C, 850, "French", NULL},
	{0x000E, 852, "Hungarian", NULL},
	{0x000F, 850, "Icelandic", NULL},
	{0x0010, 850, "Italian", NULL},
	{0x0011, 932, "Japanese", NULL},
	{0x0012, 949, "Korean", NULL},
	{0x0013, 850, "Dutch", NULL},
	{0x0014, 850, "Norwegian (Bokmål)", NULL},
	{0x0015, 852, "Polish", NULL},
	{0x0016, 850, "Portuguese", NULL},
	{0x0018, 852, "Romanian", NULL},
	{0x0019, 866, "Russian", NULL},
	{0x001A, 852, "Serbian", NULL},
	{0x001B, 852, "Slovak", NULL},
	{0x001C, 852, "Albanian", NULL},
	{0x001D, 850, "Swedish", NULL},
	{0x001F, 857, "Turkish", NULL},
	{0x0021, 850, "Indonesian", NULL},
	{0x0022, 866, "Ukrainian", NULL},
	{0x0023, 866, "Belarusian", NULL},
	{0x0024, 852, "Slovene", NULL},
	{0x0025, 775, "Estonian", NULL},
	{0x0026, 775, "Latvian", NULL},
	{0x0027, 775, "Lithuanian", NULL},
	{0x002A,1258, "Vietnamese", NULL},
	{0x002D, 850, "Basque", NULL},
	{0x002F, 866, "Macedonian", NULL},
	{0x0036, 850, "Afrikaans", NULL},
	{0x0038, 852, "Faroese", NULL},
	{0x003C, 437, "Irish", NULL},
	{0x003E, 850, "Malay", NULL},
	{0x0402, 866, "Bulgarian", "Bulgaria"},
	{0x0403, 850, "Catalan", "Spain"},
	{0x0405, 852, "Czech", "Czech Republic"},
	{0x0406, 850, "Danish", "Denmark"},
	{0x0407, 850, "German", "Germany"},
	{0x0408, 737, "Greek", "Greece"},
	{0x0409, 437, "English", "United States"},
	{0x040A, 850, "Spanish - Traditional Sort", "Spain"},
	{0x040B, 850, "Finnish", "Finland"},
	{0x040C, 850, "French", "France"},
	{0x040E, 852, "Hungarian", "Hungary"},
	{0x040F, 850, "Icelandic", "Iceland"},
	{0x0410, 850, "Italian", "Italy"},
	{0x0411, 932, "Japanese", "Japan"},
	{0x0412, 949, "Korean", "Korea (south)"},
	{0x0413, 850, "Dutch", "Netherlands"},
	{0x0414, 850, "Norwegian (Bokmål)", "Norway"},
	{0x0415, 852, "Polish", "Poland"},
	{0x0416, 850, "Portuguese", "Brazil"},
	{0x0418, 852, "Romanian", "Romania"},
	{0x0419, 866, "Russian", "Russia"},
	{0x041A, 852, "Croatian", "Croatia"},
	{0x041B, 852, "Slovak", "Slovakia"},
	{0x041C, 852, "Albanian", "Albania"},
	{0x041D, 850, "Swedish", "Sweden"},
	{0x041F, 857, "Turkish", "Turkey"},
	{0x0421, 850, "Indonesian", "Indonesia"},
	{0x0422, 866, "Ukrainian", "Ukraine"},
	{0x0423, 866, "Belarusian", "Belarus"},
	{0x0424, 852, "Slovene", "Slovenia"},
	{0x0425, 775, "Estonian", "Estonia"},
	{0x0426, 775, "Latvian", "Latvia"},
	{0x0427, 775, "Lithuanian", "Lithuania"},
	{0x042A,1258, "Vietnamese", "Vietnam"},
	{0x042D, 850, "Basque", "Spain"},
	{0x042F, 866, "Macedonian", "Former Yugoslav Republic of Macedonia"},
	{0x0436, 850, "Afrikaans", "South Africa"},
	{0x0438, 852, "Faroese", "Faroe Islands"},
	{0x043C, 437, "Irish", "Ireland"},
	{0x043E, 850, "Malay", "Malaysia"},
/*	{0x048F,   ?, "Esperanto", "<none>"},*/
	{0x0804, 936, "Chinese (People's republic of China)", "People's republic of China"},
	{0x0807, 850, "German", "Switzerland"},
	{0x0809, 850, "English", "United Kingdom"},
	{0x080A, 850, "Spanish", "Mexico"},
	{0x080C, 850, "French", "Belgium"},
	{0x0810, 850, "Italian", "Switzerland"},
	{0x0813, 850, "Dutch", "Belgium"},
	{0x0814, 850, "Norwegian (Nynorsk)", "Norway"},
	{0x0816, 850, "Portuguese", "Portugal"},
	{0x081A, 852, "Serbian (latin)", "Yugoslavia"},
	{0x081D, 850, "Swedish (Finland)", "Finland"},
	{0x0C07, 850, "German", "Austria"},
	{0x0C09, 850, "English", "Australia"},
	{0x0C0A, 850, "Spanish - International Sort", "Spain"},
	{0x0C0C, 850, "French", "Canada"},
	{0x0C1A, 855, "Serbian (Cyrillic)", "Serbia"},
	{0x1007, 850, "German", "Luxembourg"},
	{0x1009, 850, "English", "Canada"},
	{0x100A, 850, "Spanish", "Guatemala"},
	{0x100C, 850, "French", "Switzerland"},
	{0x1407, 850, "German", "Liechtenstein"},
	{0x1409, 850, "English", "New Zealand"},
	{0x140A, 850, "Spanish", "Costa Rica"},
	{0x140C, 850, "French", "Luxembourg"},
	{0x1809, 850, "English", "Ireland"},
	{0x180A, 850, "Spanish", "Panama"},
	{0x1C09, 437, "English", "South Africa"},
	{0x1C0A, 850, "Spanish", "Dominican Republic"},
	{0x2009, 850, "English", "Jamaica"},
	{0x200A, 850, "Spanish", "Venezuela"},
	{0x2409, 850, "English", "Caribbean"},
	{0x240A, 850, "Spanish", "Colombia"},
	{0x2809, 850, "English", "Belize"},
	{0x280A, 850, "Spanish", "Peru"},
	{0x2C09, 437, "English", "Trinidad & Tobago"},
	{0x2C0A, 850, "Spanish", "Argentina"},
	{0x300A, 850, "Spanish", "Ecuador"},
	{0x340A, 850, "Spanish", "Chile"},
	{0x380A, 850, "Spanish", "Uruguay"},
	{0x3C0A, 850, "Spanish", "Paraguay"},
	{0x400A, 850, "Spanish", "Bolivia"},
	{0x440A, 850, "Spanish", "El Salvador"},
	{0x480A, 850, "Spanish", "Honduras"},
	{0x4C0A, 850, "Spanish", "Nicaragua"},
	{0x500A, 850, "Spanish", "Puerto Rico"}
};

void show_languages(void)
{
	unsigned int i;
	printf(" Code  | DOS-cp |   Language   | Country\n");
	printf("-------+--------+--------------+---------\n");
	for(i = 0; i < ARRAY_SIZE(languages); i++)
		printf("0x%04x | %5d  | %-12s | %s\n",
			languages[i].id,
			languages[i].doscp,
			languages[i].name,
			languages[i].country ? languages[i].country : "Neutral");
}

static int langcmp(const void *p1, const void *p2)
{
	return *(const unsigned *)p1 - ((const struct language *)p2)->id;
}

const struct language *find_language(unsigned id)
{
	return (const struct language *)bsearch(&id, languages, ARRAY_SIZE(languages),
		sizeof(languages[0]), langcmp);
}
