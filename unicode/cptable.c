/*
 * Codepage tables
 *
 * Copyright 2000 Alexandre Julliard
 */

#include <stdlib.h>

#include "wine/unicode.h"

/* Everything below this line is generated automatically by cpmap.pl */
/* ### cpmap begin ### */
extern union cptable cptable_037;
extern union cptable cptable_424;
extern union cptable cptable_437;
extern union cptable cptable_500;
extern union cptable cptable_737;
extern union cptable cptable_775;
extern union cptable cptable_850;
extern union cptable cptable_852;
extern union cptable cptable_855;
extern union cptable cptable_856;
extern union cptable cptable_857;
extern union cptable cptable_860;
extern union cptable cptable_861;
extern union cptable cptable_862;
extern union cptable cptable_863;
extern union cptable cptable_864;
extern union cptable cptable_865;
extern union cptable cptable_866;
extern union cptable cptable_869;
extern union cptable cptable_874;
extern union cptable cptable_875;
extern union cptable cptable_878;
extern union cptable cptable_932;
extern union cptable cptable_936;
extern union cptable cptable_949;
extern union cptable cptable_950;
extern union cptable cptable_1006;
extern union cptable cptable_1026;
extern union cptable cptable_1250;
extern union cptable cptable_1251;
extern union cptable cptable_1252;
extern union cptable cptable_1253;
extern union cptable cptable_1254;
extern union cptable cptable_1255;
extern union cptable cptable_1256;
extern union cptable cptable_1257;
extern union cptable cptable_1258;
extern union cptable cptable_10000;
extern union cptable cptable_10006;
extern union cptable cptable_10007;
extern union cptable cptable_10029;
extern union cptable cptable_10079;
extern union cptable cptable_10081;
extern union cptable cptable_20866;
extern union cptable cptable_28591;
extern union cptable cptable_28592;
extern union cptable cptable_28593;
extern union cptable cptable_28594;
extern union cptable cptable_28595;
extern union cptable cptable_28596;
extern union cptable cptable_28597;
extern union cptable cptable_28598;
extern union cptable cptable_28599;

static const union cptable * const cptables[53] =
{
    &cptable_037,
    &cptable_424,
    &cptable_437,
    &cptable_500,
    &cptable_737,
    &cptable_775,
    &cptable_850,
    &cptable_852,
    &cptable_855,
    &cptable_856,
    &cptable_857,
    &cptable_860,
    &cptable_861,
    &cptable_862,
    &cptable_863,
    &cptable_864,
    &cptable_865,
    &cptable_866,
    &cptable_869,
    &cptable_874,
    &cptable_875,
    &cptable_878,
    &cptable_932,
    &cptable_936,
    &cptable_949,
    &cptable_950,
    &cptable_1006,
    &cptable_1026,
    &cptable_1250,
    &cptable_1251,
    &cptable_1252,
    &cptable_1253,
    &cptable_1254,
    &cptable_1255,
    &cptable_1256,
    &cptable_1257,
    &cptable_1258,
    &cptable_10000,
    &cptable_10006,
    &cptable_10007,
    &cptable_10029,
    &cptable_10079,
    &cptable_10081,
    &cptable_20866,
    &cptable_28591,
    &cptable_28592,
    &cptable_28593,
    &cptable_28594,
    &cptable_28595,
    &cptable_28596,
    &cptable_28597,
    &cptable_28598,
    &cptable_28599,
};
/* ### cpmap end ### */
/* Everything above this line is generated automatically by cpmap.pl */

#define NB_CODEPAGES  (sizeof(cptables)/sizeof(cptables[0]))


static int cmp_codepage( const void *codepage, const void *entry )
{
    return (unsigned int)codepage - (*(union cptable **)entry)->info.codepage;
}


/* get the table of a given code page */
const union cptable *cp_get_table( unsigned int codepage )
{
    const union cptable **res;

    if (!(res = bsearch( (void *)codepage, cptables, NB_CODEPAGES,
                         sizeof(cptables[0]), cmp_codepage ))) return NULL;
    return *res;
}


/* enum valid codepages */
const union cptable *cp_enum_table( unsigned int index )
{
    if (index >= NB_CODEPAGES) return NULL;
    return cptables[index];
}
