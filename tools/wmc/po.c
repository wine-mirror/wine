/*
 * Support for po files
 *
 * Copyright 2010, 2011 Alexandre Julliard
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
#include <stdarg.h>
#include <assert.h>
#include <ctype.h>
#ifdef HAVE_GETTEXT_PO_H
#include <gettext-po.h>
#endif

#include "wmc.h"
#include "utils.h"
#include "lang.h"
#include "write.h"
#include "windef.h"

#ifdef HAVE_LIBGETTEXTPO

static const struct
{
    unsigned int id, sub;
    const char *name;
} languages[] =
{
    { LANG_ARABIC,         SUBLANG_NEUTRAL,                     "ar" },
    { LANG_ARABIC,         SUBLANG_ARABIC_SAUDI_ARABIA,         "ar_SA" },
    { LANG_ARABIC,         SUBLANG_ARABIC_IRAQ,                 "ar_IQ" },
    { LANG_ARABIC,         SUBLANG_ARABIC_EGYPT,                "ar_EG" },
    { LANG_ARABIC,         SUBLANG_ARABIC_LIBYA,                "ar_LY" },
    { LANG_ARABIC,         SUBLANG_ARABIC_ALGERIA,              "ar_DZ" },
    { LANG_ARABIC,         SUBLANG_ARABIC_MOROCCO,              "ar_MA" },
    { LANG_ARABIC,         SUBLANG_ARABIC_TUNISIA,              "ar_TN" },
    { LANG_ARABIC,         SUBLANG_ARABIC_OMAN,                 "ar_OM" },
    { LANG_ARABIC,         SUBLANG_ARABIC_YEMEN,                "ar_YE" },
    { LANG_ARABIC,         SUBLANG_ARABIC_SYRIA,                "ar_SY" },
    { LANG_ARABIC,         SUBLANG_ARABIC_JORDAN,               "ar_JO" },
    { LANG_ARABIC,         SUBLANG_ARABIC_LEBANON,              "ar_LB" },
    { LANG_ARABIC,         SUBLANG_ARABIC_KUWAIT,               "ar_KW" },
    { LANG_ARABIC,         SUBLANG_ARABIC_UAE,                  "ar_AE" },
    { LANG_ARABIC,         SUBLANG_ARABIC_BAHRAIN,              "ar_BH" },
    { LANG_ARABIC,         SUBLANG_ARABIC_QATAR,                "ar_QA" },
    { LANG_BULGARIAN,      SUBLANG_NEUTRAL,                     "bg" },
    { LANG_BULGARIAN,      SUBLANG_BULGARIAN_BULGARIA,          "bg_BG" },
    { LANG_CATALAN,        SUBLANG_NEUTRAL,                     "ca" },
    { LANG_CATALAN,        SUBLANG_CATALAN_CATALAN,             "ca_ES" },
    { LANG_CHINESE,        SUBLANG_NEUTRAL,                     "zh" },
    { LANG_CHINESE,        SUBLANG_CHINESE_TRADITIONAL,         "zh_TW" },
    { LANG_CHINESE,        SUBLANG_CHINESE_SIMPLIFIED,          "zh_CN" },
    { LANG_CHINESE,        SUBLANG_CHINESE_HONGKONG,            "zh_HK" },
    { LANG_CHINESE,        SUBLANG_CHINESE_SINGAPORE,           "zh_SG" },
    { LANG_CHINESE,        SUBLANG_CHINESE_MACAU,               "zh_MO" },
    { LANG_CZECH,          SUBLANG_NEUTRAL,                     "cs" },
    { LANG_CZECH,          SUBLANG_CZECH_CZECH_REPUBLIC,        "cs_CZ" },
    { LANG_DANISH,         SUBLANG_NEUTRAL,                     "da" },
    { LANG_DANISH,         SUBLANG_DANISH_DENMARK,              "da_DK" },
    { LANG_GERMAN,         SUBLANG_NEUTRAL,                     "de" },
    { LANG_GERMAN,         SUBLANG_GERMAN,                      "de_DE" },
    { LANG_GERMAN,         SUBLANG_GERMAN_SWISS,                "de_CH" },
    { LANG_GERMAN,         SUBLANG_GERMAN_AUSTRIAN,             "de_AT" },
    { LANG_GERMAN,         SUBLANG_GERMAN_LUXEMBOURG,           "de_LU" },
    { LANG_GERMAN,         SUBLANG_GERMAN_LIECHTENSTEIN,        "de_LI" },
    { LANG_GREEK,          SUBLANG_NEUTRAL,                     "el" },
    { LANG_GREEK,          SUBLANG_GREEK_GREECE,                "el_GR" },
    { LANG_ENGLISH,        SUBLANG_NEUTRAL,                     "en" },
    { LANG_ENGLISH,        SUBLANG_ENGLISH_US,                  "en_US" },
    { LANG_ENGLISH,        SUBLANG_ENGLISH_UK,                  "en_GB" },
    { LANG_ENGLISH,        SUBLANG_ENGLISH_AUS,                 "en_AU" },
    { LANG_ENGLISH,        SUBLANG_ENGLISH_CAN,                 "en_CA" },
    { LANG_ENGLISH,        SUBLANG_ENGLISH_NZ,                  "en_NZ" },
    { LANG_ENGLISH,        SUBLANG_ENGLISH_EIRE,                "en_IE" },
    { LANG_ENGLISH,        SUBLANG_ENGLISH_SOUTH_AFRICA,        "en_ZA" },
    { LANG_ENGLISH,        SUBLANG_ENGLISH_JAMAICA,             "en_JM" },
    { LANG_ENGLISH,        SUBLANG_ENGLISH_CARIBBEAN,           "en_CB" },
    { LANG_ENGLISH,        SUBLANG_ENGLISH_BELIZE,              "en_BZ" },
    { LANG_ENGLISH,        SUBLANG_ENGLISH_TRINIDAD,            "en_TT" },
    { LANG_ENGLISH,        SUBLANG_ENGLISH_ZIMBABWE,            "en_ZW" },
    { LANG_ENGLISH,        SUBLANG_ENGLISH_PHILIPPINES,         "en_PH" },
    { LANG_SPANISH,        SUBLANG_NEUTRAL,                     "es" },
    { LANG_SPANISH,        SUBLANG_SPANISH,                     "es_ES" },
    { LANG_SPANISH,        SUBLANG_SPANISH_MEXICAN,             "es_MX" },
    { LANG_SPANISH,        SUBLANG_SPANISH_MODERN,              "es_ES_modern" },
    { LANG_SPANISH,        SUBLANG_SPANISH_GUATEMALA,           "es_GT" },
    { LANG_SPANISH,        SUBLANG_SPANISH_COSTA_RICA,          "es_CR" },
    { LANG_SPANISH,        SUBLANG_SPANISH_PANAMA,              "es_PA" },
    { LANG_SPANISH,        SUBLANG_SPANISH_DOMINICAN_REPUBLIC,  "es_DO" },
    { LANG_SPANISH,        SUBLANG_SPANISH_VENEZUELA,           "es_VE" },
    { LANG_SPANISH,        SUBLANG_SPANISH_COLOMBIA,            "es_CO" },
    { LANG_SPANISH,        SUBLANG_SPANISH_PERU,                "es_PE" },
    { LANG_SPANISH,        SUBLANG_SPANISH_ARGENTINA,           "es_AR" },
    { LANG_SPANISH,        SUBLANG_SPANISH_ECUADOR,             "es_EC" },
    { LANG_SPANISH,        SUBLANG_SPANISH_CHILE,               "es_CL" },
    { LANG_SPANISH,        SUBLANG_SPANISH_URUGUAY,             "es_UY" },
    { LANG_SPANISH,        SUBLANG_SPANISH_PARAGUAY,            "es_PY" },
    { LANG_SPANISH,        SUBLANG_SPANISH_BOLIVIA,             "es_BO" },
    { LANG_SPANISH,        SUBLANG_SPANISH_EL_SALVADOR,         "es_SV" },
    { LANG_SPANISH,        SUBLANG_SPANISH_HONDURAS,            "es_HN" },
    { LANG_SPANISH,        SUBLANG_SPANISH_NICARAGUA,           "es_NI" },
    { LANG_SPANISH,        SUBLANG_SPANISH_PUERTO_RICO,         "es_PR" },
    { LANG_FINNISH,        SUBLANG_NEUTRAL,                     "fi" },
    { LANG_FINNISH,        SUBLANG_FINNISH_FINLAND,             "fi_FI" },
    { LANG_FRENCH,         SUBLANG_NEUTRAL,                     "fr" },
    { LANG_FRENCH,         SUBLANG_FRENCH,                      "fr_FR" },
    { LANG_FRENCH,         SUBLANG_FRENCH_BELGIAN,              "fr_BE" },
    { LANG_FRENCH,         SUBLANG_FRENCH_CANADIAN,             "fr_CA" },
    { LANG_FRENCH,         SUBLANG_FRENCH_SWISS,                "fr_CH" },
    { LANG_FRENCH,         SUBLANG_FRENCH_LUXEMBOURG,           "fr_LU" },
    { LANG_FRENCH,         SUBLANG_FRENCH_MONACO,               "fr_MC" },
    { LANG_HEBREW,         SUBLANG_NEUTRAL,                     "he" },
    { LANG_HEBREW,         SUBLANG_HEBREW_ISRAEL,               "he_IL" },
    { LANG_HUNGARIAN,      SUBLANG_NEUTRAL,                     "hu" },
    { LANG_HUNGARIAN,      SUBLANG_HUNGARIAN_HUNGARY,           "hu_HU" },
    { LANG_ICELANDIC,      SUBLANG_NEUTRAL,                     "is" },
    { LANG_ICELANDIC,      SUBLANG_ICELANDIC_ICELAND,           "is_IS" },
    { LANG_ITALIAN,        SUBLANG_NEUTRAL,                     "it" },
    { LANG_ITALIAN,        SUBLANG_ITALIAN,                     "it_IT" },
    { LANG_ITALIAN,        SUBLANG_ITALIAN_SWISS,               "it_CH" },
    { LANG_JAPANESE,       SUBLANG_NEUTRAL,                     "ja" },
    { LANG_JAPANESE,       SUBLANG_JAPANESE_JAPAN,              "ja_JP" },
    { LANG_KOREAN,         SUBLANG_NEUTRAL,                     "ko" },
    { LANG_KOREAN,         SUBLANG_KOREAN,                      "ko_KR" },
    { LANG_DUTCH,          SUBLANG_NEUTRAL,                     "nl" },
    { LANG_DUTCH,          SUBLANG_DUTCH,                       "nl_NL" },
    { LANG_DUTCH,          SUBLANG_DUTCH_BELGIAN,               "nl_BE" },
    { LANG_DUTCH,          SUBLANG_DUTCH_SURINAM,               "nl_SR" },
    { LANG_NORWEGIAN,      SUBLANG_NORWEGIAN_BOKMAL,            "nb_NO" },
    { LANG_NORWEGIAN,      SUBLANG_NORWEGIAN_NYNORSK,           "nn_NO" },
    { LANG_POLISH,         SUBLANG_NEUTRAL,                     "pl" },
    { LANG_POLISH,         SUBLANG_POLISH_POLAND,               "pl_PL" },
    { LANG_PORTUGUESE,     SUBLANG_NEUTRAL,                     "pt" },
    { LANG_PORTUGUESE,     SUBLANG_PORTUGUESE_BRAZILIAN,        "pt_BR" },
    { LANG_PORTUGUESE,     SUBLANG_PORTUGUESE_PORTUGAL,         "pt_PT" },
    { LANG_ROMANSH,        SUBLANG_NEUTRAL,                     "rm" },
    { LANG_ROMANSH,        SUBLANG_ROMANSH_SWITZERLAND,         "rm_CH" },
    { LANG_ROMANIAN,       SUBLANG_NEUTRAL,                     "ro" },
    { LANG_ROMANIAN,       SUBLANG_ROMANIAN_ROMANIA,            "ro_RO" },
    { LANG_RUSSIAN,        SUBLANG_NEUTRAL,                     "ru" },
    { LANG_RUSSIAN,        SUBLANG_RUSSIAN_RUSSIA,              "ru_RU" },
    { LANG_SERBIAN,        SUBLANG_NEUTRAL,                     "hr" },
    { LANG_SERBIAN,        SUBLANG_SERBIAN_CROATIA,             "hr_HR" },
    { LANG_SERBIAN,        SUBLANG_SERBIAN_LATIN,               "sr_RS@latin" },
    { LANG_SERBIAN,        SUBLANG_SERBIAN_CYRILLIC,            "sr_RS@cyrillic" },
    { LANG_SLOVAK,         SUBLANG_NEUTRAL,                     "sk" },
    { LANG_SLOVAK,         SUBLANG_SLOVAK_SLOVAKIA,             "sk_SK" },
    { LANG_ALBANIAN,       SUBLANG_NEUTRAL,                     "sq" },
    { LANG_ALBANIAN,       SUBLANG_ALBANIAN_ALBANIA,            "sq_AL" },
    { LANG_SWEDISH,        SUBLANG_NEUTRAL,                     "sv" },
    { LANG_SWEDISH,        SUBLANG_SWEDISH_SWEDEN,              "sv_SE" },
    { LANG_SWEDISH,        SUBLANG_SWEDISH_FINLAND,             "sv_FI" },
    { LANG_THAI,           SUBLANG_NEUTRAL,                     "th" },
    { LANG_THAI,           SUBLANG_THAI_THAILAND,               "th_TH" },
    { LANG_TURKISH,        SUBLANG_NEUTRAL,                     "tr" },
    { LANG_TURKISH,        SUBLANG_TURKISH_TURKEY,              "tr_TR" },
    { LANG_URDU,           SUBLANG_NEUTRAL,                     "ur" },
    { LANG_URDU,           SUBLANG_URDU_PAKISTAN,               "ur_PK" },
    { LANG_INDONESIAN,     SUBLANG_NEUTRAL,                     "id" },
    { LANG_INDONESIAN,     SUBLANG_INDONESIAN_INDONESIA,        "id_ID" },
    { LANG_UKRAINIAN,      SUBLANG_NEUTRAL,                     "uk" },
    { LANG_UKRAINIAN,      SUBLANG_UKRAINIAN_UKRAINE,           "uk_UA" },
    { LANG_BELARUSIAN,     SUBLANG_NEUTRAL,                     "be" },
    { LANG_BELARUSIAN,     SUBLANG_BELARUSIAN_BELARUS,          "be_BY" },
    { LANG_SLOVENIAN,      SUBLANG_NEUTRAL,                     "sl" },
    { LANG_SLOVENIAN,      SUBLANG_SLOVENIAN_SLOVENIA,          "sl_SI" },
    { LANG_ESTONIAN,       SUBLANG_NEUTRAL,                     "et" },
    { LANG_ESTONIAN,       SUBLANG_ESTONIAN_ESTONIA,            "et_EE" },
    { LANG_LATVIAN,        SUBLANG_NEUTRAL,                     "lv" },
    { LANG_LATVIAN,        SUBLANG_LATVIAN_LATVIA,              "lv_LV" },
    { LANG_LITHUANIAN,     SUBLANG_NEUTRAL,                     "lt" },
    { LANG_LITHUANIAN,     SUBLANG_LITHUANIAN_LITHUANIA,        "lt_LT" },
    { LANG_PERSIAN,        SUBLANG_NEUTRAL,                     "fa" },
    { LANG_PERSIAN,        SUBLANG_PERSIAN_IRAN,                "fa_IR" },
    { LANG_ARMENIAN,       SUBLANG_NEUTRAL,                     "hy" },
    { LANG_ARMENIAN,       SUBLANG_ARMENIAN_ARMENIA,            "hy_AM" },
    { LANG_AZERI,          SUBLANG_NEUTRAL,                     "az" },
    { LANG_AZERI,          SUBLANG_AZERI_LATIN,                 "az_AZ@latin" },
    { LANG_AZERI,          SUBLANG_AZERI_CYRILLIC,              "az_AZ@cyrillic" },
    { LANG_BASQUE,         SUBLANG_NEUTRAL,                     "eu" },
    { LANG_BASQUE,         SUBLANG_BASQUE_BASQUE,               "eu_ES" },
    { LANG_MACEDONIAN,     SUBLANG_NEUTRAL,                     "mk" },
    { LANG_MACEDONIAN,     SUBLANG_MACEDONIAN_MACEDONIA,        "mk_MK" },
    { LANG_AFRIKAANS,      SUBLANG_NEUTRAL,                     "af" },
    { LANG_AFRIKAANS,      SUBLANG_AFRIKAANS_SOUTH_AFRICA,      "af_ZA" },
    { LANG_GEORGIAN,       SUBLANG_NEUTRAL,                     "ka" },
    { LANG_GEORGIAN,       SUBLANG_GEORGIAN_GEORGIA,            "ka_GE" },
    { LANG_FAEROESE,       SUBLANG_NEUTRAL,                     "fo" },
    { LANG_FAEROESE,       SUBLANG_FAEROESE_FAROE_ISLANDS,      "fo_FO" },
    { LANG_HINDI,          SUBLANG_NEUTRAL,                     "hi" },
    { LANG_HINDI,          SUBLANG_HINDI_INDIA,                 "hi_IN" },
    { LANG_MALAY,          SUBLANG_NEUTRAL,                     "ms" },
    { LANG_MALAY,          SUBLANG_MALAY_MALAYSIA,              "ms_MY" },
    { LANG_MALAY,          SUBLANG_MALAY_BRUNEI_DARUSSALAM,     "ms_BN" },
    { LANG_KAZAK,          SUBLANG_NEUTRAL,                     "kk" },
    { LANG_KAZAK,          SUBLANG_KAZAK_KAZAKHSTAN,            "kk_KZ" },
    { LANG_KYRGYZ,         SUBLANG_NEUTRAL,                     "ky" },
    { LANG_KYRGYZ,         SUBLANG_KYRGYZ_KYRGYZSTAN,           "ky_KG" },
    { LANG_SWAHILI,        SUBLANG_NEUTRAL,                     "sw" },
    { LANG_SWAHILI,        SUBLANG_SWAHILI_KENYA,               "sw_KE" },
    { LANG_UZBEK,          SUBLANG_NEUTRAL,                     "uz" },
    { LANG_UZBEK,          SUBLANG_UZBEK_LATIN,                 "uz_UZ@latin" },
    { LANG_UZBEK,          SUBLANG_UZBEK_CYRILLIC,              "uz_UZ@cyrillic" },
    { LANG_TATAR,          SUBLANG_NEUTRAL,                     "tt" },
    { LANG_TATAR,          SUBLANG_TATAR_RUSSIA,                "tt_TA" },
    { LANG_PUNJABI,        SUBLANG_NEUTRAL,                     "pa" },
    { LANG_PUNJABI,        SUBLANG_PUNJABI_INDIA,               "pa_IN" },
    { LANG_GUJARATI,       SUBLANG_NEUTRAL,                     "gu" },
    { LANG_GUJARATI,       SUBLANG_GUJARATI_INDIA,              "gu_IN" },
    { LANG_ORIYA,          SUBLANG_NEUTRAL,                     "or" },
    { LANG_ORIYA,          SUBLANG_ORIYA_INDIA,                 "or_IN" },
    { LANG_TAMIL,          SUBLANG_NEUTRAL,                     "ta" },
    { LANG_TAMIL,          SUBLANG_TAMIL_INDIA,                 "ta_IN" },
    { LANG_TELUGU,         SUBLANG_NEUTRAL,                     "te" },
    { LANG_TELUGU,         SUBLANG_TELUGU_INDIA,                "te_IN" },
    { LANG_KANNADA,        SUBLANG_NEUTRAL,                     "kn" },
    { LANG_KANNADA,        SUBLANG_KANNADA_INDIA,               "kn_IN" },
    { LANG_MALAYALAM,      SUBLANG_NEUTRAL,                     "ml" },
    { LANG_MALAYALAM,      SUBLANG_MALAYALAM_INDIA,             "ml_IN" },
    { LANG_MARATHI,        SUBLANG_NEUTRAL,                     "mr" },
    { LANG_MARATHI,        SUBLANG_MARATHI_INDIA,               "mr_IN" },
    { LANG_SANSKRIT,       SUBLANG_NEUTRAL,                     "sa" },
    { LANG_SANSKRIT,       SUBLANG_SANSKRIT_INDIA,              "sa_IN" },
    { LANG_MONGOLIAN,      SUBLANG_NEUTRAL,                     "mn" },
    { LANG_MONGOLIAN,      SUBLANG_MONGOLIAN_CYRILLIC_MONGOLIA, "mn_MN" },
    { LANG_WELSH,          SUBLANG_NEUTRAL,                     "cy" },
    { LANG_WELSH,          SUBLANG_WELSH_UNITED_KINGDOM,        "cy_GB" },
    { LANG_GALICIAN,       SUBLANG_NEUTRAL,                     "gl" },
    { LANG_GALICIAN,       SUBLANG_GALICIAN_GALICIAN,           "gl_ES" },
    { LANG_KONKANI,        SUBLANG_NEUTRAL,                     "kok" },
    { LANG_KONKANI,        SUBLANG_KONKANI_INDIA,               "kok_IN" },
    { LANG_DIVEHI,         SUBLANG_NEUTRAL,                     "dv" },
    { LANG_DIVEHI,         SUBLANG_DIVEHI_MALDIVES,             "dv_MV" },
    { LANG_BRETON,         SUBLANG_NEUTRAL,                     "br" },
    { LANG_BRETON,         SUBLANG_BRETON_FRANCE,               "br_FR" },

#ifdef LANG_ESPERANTO
    { LANG_ESPERANTO,      SUBLANG_DEFAULT,                     "eo" },
#endif
#ifdef LANG_WALON
    { LANG_WALON,          SUBLANG_NEUTRAL,                     "wa" },
    { LANG_WALON,          SUBLANG_DEFAULT,                     "wa_BE" },
#endif
#ifdef LANG_CORNISH
    { LANG_CORNISH,        SUBLANG_NEUTRAL,                     "kw" },
    { LANG_CORNISH,        SUBLANG_DEFAULT,                     "kw_GB" },
#endif
#ifdef LANG_GAELIC
    { LANG_GAELIC,         SUBLANG_NEUTRAL,                     "ga" },
    { LANG_GAELIC,         SUBLANG_GAELIC,                      "ga_IE" },
    { LANG_GAELIC,         SUBLANG_GAELIC_SCOTTISH,             "gd_GB" },
    { LANG_GAELIC,         SUBLANG_GAELIC_MANX,                 "gv_GB" },
#endif
};

static void po_xerror( int severity, po_message_t message,
                       const char *filename, size_t lineno, size_t column,
                       int multiline_p, const char *message_text )
{
    fprintf( stderr, "%s:%u:%u: %s\n",
             filename, (unsigned int)lineno, (unsigned int)column, message_text );
    if (severity) exit(1);
}

static void po_xerror2( int severity, po_message_t message1,
                        const char *filename1, size_t lineno1, size_t column1,
                        int multiline_p1, const char *message_text1,
                        po_message_t message2,
                        const char *filename2, size_t lineno2, size_t column2,
                        int multiline_p2, const char *message_text2 )
{
    fprintf( stderr, "%s:%u:%u: %s\n",
             filename1, (unsigned int)lineno1, (unsigned int)column1, message_text1 );
    fprintf( stderr, "%s:%u:%u: %s\n",
             filename2, (unsigned int)lineno2, (unsigned int)column2, message_text2 );
    if (severity) exit(1);
}

static const struct po_xerror_handler po_xerror_handler = { po_xerror, po_xerror2 };

static int is_english( int lan )
{
    return lan == MAKELANGID( LANG_ENGLISH, SUBLANG_DEFAULT );
}

static char *convert_string_utf8( const lanmsg_t *msg )
{
    char *buffer = xmalloc( msg->len * 4 + 1 );
    int len = wine_utf8_wcstombs( 0, msg->msg, msg->len, buffer, msg->len * 4 );
    buffer[len] = 0;
    return buffer;
}

static char *convert_msgid_ascii( const lanmsg_t *msg, int error_on_invalid_char )
{
    int i;
    char *buffer = xmalloc( msg->len * 4 + 1 );

    for (i = 0; i < msg->len; i++)
    {
        buffer[i] = msg->msg[i];
        if (!msg->msg[i]) break;
        if (msg->msg[i] >= 32 && msg->msg[i] <= 127) continue;
        if (msg->msg[i] == '\t' || msg->msg[i] == '\n') continue;
        if (error_on_invalid_char)
        {
            fprintf( stderr, "%s:%d: ", msg->file, msg->line );
            error( "Invalid character %04x in source string\n", msg->msg[i] );
        }
        free( buffer );
        return NULL;
    }
    buffer[i] = 0;
    return buffer;
}

static char *get_message_context( char **msgid )
{
    static const char magic[] = "#msgctxt#";
    char *id, *context;

    if (strncmp( *msgid, magic, sizeof(magic) - 1 )) return NULL;
    context = *msgid + sizeof(magic) - 1;
    if (!(id = strchr( context, '#' ))) return NULL;
    *id = 0;
    *msgid = id + 1;
    return context;
}

static po_message_t find_message( po_file_t po, const char *msgid, const char *msgctxt,
                                  po_message_iterator_t *iterator )
{
    po_message_t msg;
    const char *context;

    *iterator = po_message_iterator( po, NULL );
    while ((msg = po_next_message( *iterator )))
    {
        if (strcmp( po_message_msgid( msg ), msgid )) continue;
        if (!msgctxt) break;
        if (!(context = po_message_msgctxt( msg ))) continue;
        if (!strcmp( context, msgctxt )) break;
    }
    return msg;
}

static void add_po_string( po_file_t po, const lanmsg_t *msgid, const lanmsg_t *msgstr )
{
    po_message_t msg;
    po_message_iterator_t iterator;
    char *id, *id_buffer, *context, *str = NULL, *str_buffer = NULL;

    if (msgid->len <= 1) return;

    id_buffer = id = convert_msgid_ascii( msgid, 1 );
    context = get_message_context( &id );

    if (msgstr)
    {
        str_buffer = str = convert_string_utf8( msgstr );
        if (is_english( msgstr->lan )) get_message_context( &str );
    }
    if (!(msg = find_message( po, id, context, &iterator )))
    {
        msg = po_message_create();
        po_message_set_msgid( msg, id );
        po_message_set_msgstr( msg, str ? str : "" );
        if (context) po_message_set_msgctxt( msg, context );
        po_message_insert( iterator, msg );
    }
    if (msgid->file) po_message_add_filepos( msg, msgid->file, msgid->line );
    po_message_iterator_free( iterator );
    free( id_buffer );
    free( str_buffer );
}

static po_file_t create_po_file(void)
{
    po_file_t po;
    po_message_t msg;
    po_message_iterator_t iterator;

    po = po_file_create();
    iterator = po_message_iterator( po, NULL );
    msg = po_message_create();
    po_message_set_msgid( msg, "" );
    po_message_set_msgstr( msg,
                           "Project-Id-Version: Wine\n"
                           "Report-Msgid-Bugs-To: http://bugs.winehq.org\n"
                           "POT-Creation-Date: N/A\n"
                           "PO-Revision-Date: N/A\n"
                           "Last-Translator: Automatically generated\n"
                           "Language-Team: none\n"
                           "MIME-Version: 1.0\n"
                           "Content-Type: text/plain; charset=UTF-8\n"
                           "Content-Transfer-Encoding: 8bit\n" );
    po_message_insert( iterator, msg );
    po_message_iterator_free( iterator );
    return po;
}

void write_pot_file( const char *outname )
{
    int i, j;
    lan_blk_t *lbp;
    po_file_t po = create_po_file();

    for (lbp = lanblockhead; lbp; lbp = lbp->next)
    {
        if (!is_english( lbp->lan )) continue;
        for (i = 0; i < lbp->nblk; i++)
        {
            block_t *blk = &lbp->blks[i];
            for (j = 0; j < blk->nmsg; j++) add_po_string( po, blk->msgs[j], NULL );
        }
    }
    po_file_write( po, outname, &po_xerror_handler );
    po_file_free( po );
}

static lan_blk_t *new_top, *new_tail;

static lanmsg_t *translate_string( po_file_t po, lanmsg_t *str, int lang, int *found )
{
    po_message_t msg;
    po_message_iterator_t iterator;
    lanmsg_t *new;
    const char *transl;
    int res;
    char *buffer, *msgid, *context;

    if (str->len <= 1 || !(buffer = convert_msgid_ascii( str, 0 ))) return str;

    msgid = buffer;
    context = get_message_context( &msgid );
    msg = find_message( po, msgid, context, &iterator );
    po_message_iterator_free( iterator );

    if (msg && !po_message_is_fuzzy( msg ))
    {
        transl = po_message_msgstr( msg );
        if (!transl[0]) transl = msgid;  /* ignore empty strings */
        else (*found)++;
    }
    else transl = msgid;

    new = xmalloc( sizeof(*new) );
    new->lan  = lang;
    new->cp   = 0;  /* FIXME */
    new->file = str->file;
    new->line = str->line;
    new->len  = wine_utf8_mbstowcs( 0, transl, strlen(transl) + 1, NULL, 0 );
    new->msg  = xmalloc( new->len * sizeof(WCHAR) );
    res = wine_utf8_mbstowcs( MB_ERR_INVALID_CHARS, transl, strlen(transl) + 1, new->msg, new->len );
    if (res == -2)
        error( "Invalid utf-8 character in string '%s'\n", transl );
    free( buffer );
    return new;
}

static void translate_block( po_file_t po, block_t *blk, block_t *new, int lang, int *found )
{
    int i;

    new->idlo = blk->idlo;
    new->idhi = blk->idhi;
    new->size = 0;
    new->msgs = xmalloc( blk->nmsg * sizeof(*new->msgs) );
    new->nmsg = blk->nmsg;
    for (i = 0; i < blk->nmsg; i++)
    {
        new->msgs[i] = translate_string( po, blk->msgs[i], lang, found );
        new->size += ((2 * new->msgs[i]->len + 3) & ~3) + 4;
    }
}

static void translate_messages( po_file_t po, int lang )
{
    int i, found;
    lan_blk_t *lbp, *new;

    for (lbp = lanblockhead; lbp; lbp = lbp->next)
    {
        if (!is_english( lbp->lan )) continue;
        found = 0;
        new = xmalloc( sizeof(*new) );
        /* English "translations" take precedence over the original contents */
        new->version = is_english( lang ) ? 1 : -1;
        new->lan = lang;
        new->blks = xmalloc( lbp->nblk * sizeof(*new->blks) );
        new->nblk = lbp->nblk;

        for (i = 0; i < lbp->nblk; i++)
            translate_block( po, &lbp->blks[i], &new->blks[i], lang, &found );
        if (found)
        {
            if (new_tail) new_tail->next = new;
            else new_top = new;
            new->prev = new_tail;
            new_tail = new;
        }
        else
        {
            free( new->blks );
            free( new );
        }
    }
}

void add_translations( const char *po_dir )
{
    lan_blk_t *lbp;
    po_file_t po;
    char buffer[256];
    char *p, *tok, *name;
    unsigned int i;
    FILE *f;

    /* first check if we have English resources to translate */
    for (lbp = lanblockhead; lbp; lbp = lbp->next) if (is_english( lbp->lan )) break;
    if (!lbp) return;

    new_top = new_tail = NULL;

    name = strmake( "%s/LINGUAS", po_dir );
    if (!(f = fopen( name, "r" ))) return;
    free( name );
    while (fgets( buffer, sizeof(buffer), f ))
    {
        if ((p = strchr( buffer, '#' ))) *p = 0;
        for (tok = strtok( buffer, " \t\r\n" ); tok; tok = strtok( NULL, " \t\r\n" ))
        {
            for (i = 0; i < sizeof(languages)/sizeof(languages[0]); i++)
                if (!strcmp( tok, languages[i].name )) break;

            if (i == sizeof(languages)/sizeof(languages[0]))
                error( "unknown language '%s'\n", tok );

            name = strmake( "%s/%s.po", po_dir, tok );
            if (!(po = po_file_read( name, &po_xerror_handler )))
                error( "cannot load po file for language '%s'\n", tok );
            translate_messages( po, MAKELANGID(languages[i].id, languages[i].sub) );
            po_file_free( po );
            free( name );
        }
    }
    fclose( f );

    /* prepend the translated messages to the global list */
    if (new_tail)
    {
        new_tail->next = lanblockhead;
        lanblockhead->prev = new_tail;
        lanblockhead = new_top;
    }
}

#else  /* HAVE_LIBGETTEXTPO */

void write_pot_file( const char *outname )
{
    error( "PO files not supported in this wmc build\n" );
}

void add_translations( const char *po_dir )
{
}

#endif
