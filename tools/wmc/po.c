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

struct mo_file
{
    unsigned int magic;
    unsigned int revision;
    unsigned int count;
    unsigned int msgid_off;
    unsigned int msgstr_off;
    /* ... rest of file data here */
};

static struct lan_blk *new_top, *new_tail;

static const struct
{
    unsigned int id, sub;
    const char *name;
} languages[] =
{
    { LANG_AFRIKAANS,      SUBLANG_NEUTRAL,                     "af" },
    { LANG_AFRIKAANS,      SUBLANG_AFRIKAANS_SOUTH_AFRICA,      "af_ZA" },
    { LANG_ALBANIAN,       SUBLANG_NEUTRAL,                     "sq" },
    { LANG_ALBANIAN,       SUBLANG_ALBANIAN_ALBANIA,            "sq_AL" },
    { LANG_AMHARIC,        SUBLANG_NEUTRAL,                     "am" },
    { LANG_AMHARIC,        SUBLANG_AMHARIC_ETHIOPIA,            "am_ET" },
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
    { LANG_ARMENIAN,       SUBLANG_NEUTRAL,                     "hy" },
    { LANG_ARMENIAN,       SUBLANG_ARMENIAN_ARMENIA,            "hy_AM" },
    { LANG_ASSAMESE,       SUBLANG_NEUTRAL,                     "as" },
    { LANG_ASSAMESE,       SUBLANG_ASSAMESE_INDIA,              "as_IN" },
    { LANG_ASTURIAN,       SUBLANG_NEUTRAL,                     "ast" },
    { LANG_ASTURIAN,       SUBLANG_DEFAULT,                     "ast_ES" },
    { LANG_AZERBAIJANI,    SUBLANG_NEUTRAL,                     "az" },
    { LANG_AZERBAIJANI,    SUBLANG_AZERBAIJANI_AZERBAIJAN_LATIN,"az_AZ@latin" },
    { LANG_AZERBAIJANI,    SUBLANG_AZERBAIJANI_AZERBAIJAN_CYRILLIC, "az_AZ@cyrillic" },
    { LANG_BASQUE,         SUBLANG_NEUTRAL,                     "eu" },
    { LANG_BASQUE,         SUBLANG_BASQUE_BASQUE,               "eu_ES" },
    { LANG_BELARUSIAN,     SUBLANG_NEUTRAL,                     "be" },
    { LANG_BELARUSIAN,     SUBLANG_BELARUSIAN_BELARUS,          "be_BY" },
    { LANG_BENGALI,        SUBLANG_NEUTRAL,                     "bn" },
    { LANG_BENGALI,        SUBLANG_BENGALI_INDIA,               "bn_IN" },
    { LANG_BENGALI,        SUBLANG_BENGALI_BANGLADESH,          "bn_BD" },
    { LANG_BRETON,         SUBLANG_NEUTRAL,                     "br" },
    { LANG_BRETON,         SUBLANG_BRETON_FRANCE,               "br_FR" },
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
    { LANG_DIVEHI,         SUBLANG_NEUTRAL,                     "dv" },
    { LANG_DIVEHI,         SUBLANG_DIVEHI_MALDIVES,             "dv_MV" },
    { LANG_DUTCH,          SUBLANG_NEUTRAL,                     "nl" },
    { LANG_DUTCH,          SUBLANG_DUTCH,                       "nl_NL" },
    { LANG_DUTCH,          SUBLANG_DUTCH_BELGIAN,               "nl_BE" },
    { LANG_DUTCH,          SUBLANG_DUTCH_SURINAM,               "nl_SR" },
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
    { LANG_ENGLISH,        SUBLANG_ENGLISH_INDIA,               "en_IN" },
    { LANG_ENGLISH,        SUBLANG_ENGLISH_MALAYSIA,            "en_MY" },
    { LANG_ENGLISH,        SUBLANG_ENGLISH_SINGAPORE,           "en_SG" },
    { LANG_ESTONIAN,       SUBLANG_NEUTRAL,                     "et" },
    { LANG_ESTONIAN,       SUBLANG_ESTONIAN_ESTONIA,            "et_EE" },
    { LANG_FAEROESE,       SUBLANG_NEUTRAL,                     "fo" },
    { LANG_FAEROESE,       SUBLANG_FAEROESE_FAROE_ISLANDS,      "fo_FO" },
    { LANG_FILIPINO,       SUBLANG_NEUTRAL,                     "fil" },
    { LANG_FILIPINO,       SUBLANG_FILIPINO_PHILIPPINES,        "fil_PH" },
    { LANG_FINNISH,        SUBLANG_NEUTRAL,                     "fi" },
    { LANG_FINNISH,        SUBLANG_FINNISH_FINLAND,             "fi_FI" },
    { LANG_FRENCH,         SUBLANG_NEUTRAL,                     "fr" },
    { LANG_FRENCH,         SUBLANG_FRENCH,                      "fr_FR" },
    { LANG_FRENCH,         SUBLANG_FRENCH_BELGIAN,              "fr_BE" },
    { LANG_FRENCH,         SUBLANG_FRENCH_CANADIAN,             "fr_CA" },
    { LANG_FRENCH,         SUBLANG_FRENCH_SWISS,                "fr_CH" },
    { LANG_FRENCH,         SUBLANG_FRENCH_LUXEMBOURG,           "fr_LU" },
    { LANG_FRENCH,         SUBLANG_FRENCH_MONACO,               "fr_MC" },
    { LANG_GALICIAN,       SUBLANG_NEUTRAL,                     "gl" },
    { LANG_GALICIAN,       SUBLANG_GALICIAN_GALICIAN,           "gl_ES" },
    { LANG_GEORGIAN,       SUBLANG_NEUTRAL,                     "ka" },
    { LANG_GEORGIAN,       SUBLANG_GEORGIAN_GEORGIA,            "ka_GE" },
    { LANG_GERMAN,         SUBLANG_NEUTRAL,                     "de" },
    { LANG_GERMAN,         SUBLANG_GERMAN,                      "de_DE" },
    { LANG_GERMAN,         SUBLANG_GERMAN_SWISS,                "de_CH" },
    { LANG_GERMAN,         SUBLANG_GERMAN_AUSTRIAN,             "de_AT" },
    { LANG_GERMAN,         SUBLANG_GERMAN_LUXEMBOURG,           "de_LU" },
    { LANG_GERMAN,         SUBLANG_GERMAN_LIECHTENSTEIN,        "de_LI" },
    { LANG_GREEK,          SUBLANG_NEUTRAL,                     "el" },
    { LANG_GREEK,          SUBLANG_GREEK_GREECE,                "el_GR" },
    { LANG_GUJARATI,       SUBLANG_NEUTRAL,                     "gu" },
    { LANG_GUJARATI,       SUBLANG_GUJARATI_INDIA,              "gu_IN" },
    { LANG_HAUSA,          SUBLANG_NEUTRAL,                     "ha" },
    { LANG_HAUSA,          SUBLANG_HAUSA_NIGERIA,               "ha_NG" },
    { LANG_HAWAIIAN,       SUBLANG_NEUTRAL,                     "haw" },
    { LANG_HAWAIIAN,       SUBLANG_HAWAIIAN_US,                 "haw_US" },
    { LANG_HEBREW,         SUBLANG_NEUTRAL,                     "he" },
    { LANG_HEBREW,         SUBLANG_HEBREW_ISRAEL,               "he_IL" },
    { LANG_HINDI,          SUBLANG_NEUTRAL,                     "hi" },
    { LANG_HINDI,          SUBLANG_HINDI_INDIA,                 "hi_IN" },
    { LANG_HUNGARIAN,      SUBLANG_NEUTRAL,                     "hu" },
    { LANG_HUNGARIAN,      SUBLANG_HUNGARIAN_HUNGARY,           "hu_HU" },
    { LANG_ICELANDIC,      SUBLANG_NEUTRAL,                     "is" },
    { LANG_ICELANDIC,      SUBLANG_ICELANDIC_ICELAND,           "is_IS" },
    { LANG_IGBO,           SUBLANG_NEUTRAL,                     "ig" },
    { LANG_IGBO,           SUBLANG_IGBO_NIGERIA,                "ig_NG" },
    { LANG_INDONESIAN,     SUBLANG_NEUTRAL,                     "id" },
    { LANG_INDONESIAN,     SUBLANG_INDONESIAN_INDONESIA,        "id_ID" },
    { LANG_INUKTITUT,      SUBLANG_NEUTRAL,                     "iu" },
    { LANG_INUKTITUT,      SUBLANG_INUKTITUT_CANADA,            "iu_CA" },
    { LANG_IRISH,          SUBLANG_NEUTRAL,                     "ga" },
    { LANG_IRISH,          SUBLANG_IRISH_IRELAND,               "ga_IE" },
    { LANG_ITALIAN,        SUBLANG_NEUTRAL,                     "it" },
    { LANG_ITALIAN,        SUBLANG_ITALIAN,                     "it_IT" },
    { LANG_ITALIAN,        SUBLANG_ITALIAN_SWISS,               "it_CH" },
    { LANG_JAPANESE,       SUBLANG_NEUTRAL,                     "ja" },
    { LANG_JAPANESE,       SUBLANG_JAPANESE_JAPAN,              "ja_JP" },
    { LANG_KANNADA,        SUBLANG_NEUTRAL,                     "kn" },
    { LANG_KANNADA,        SUBLANG_KANNADA_INDIA,               "kn_IN" },
    { LANG_KAZAK,          SUBLANG_NEUTRAL,                     "kk" },
    { LANG_KAZAK,          SUBLANG_KAZAK_KAZAKHSTAN,            "kk_KZ" },
    { LANG_KHMER,          SUBLANG_NEUTRAL,                     "km" },
    { LANG_KHMER,          SUBLANG_KHMER_CAMBODIA,              "km_KH" },
    { LANG_KINYARWANDA,    SUBLANG_NEUTRAL,                     "rw" },
    { LANG_KINYARWANDA,    SUBLANG_KINYARWANDA_RWANDA,          "rw_RW" },
    { LANG_KONKANI,        SUBLANG_NEUTRAL,                     "kok" },
    { LANG_KONKANI,        SUBLANG_KONKANI_INDIA,               "kok_IN" },
    { LANG_KOREAN,         SUBLANG_NEUTRAL,                     "ko" },
    { LANG_KOREAN,         SUBLANG_KOREAN,                      "ko_KR" },
    { LANG_KYRGYZ,         SUBLANG_NEUTRAL,                     "ky" },
    { LANG_KYRGYZ,         SUBLANG_KYRGYZ_KYRGYZSTAN,           "ky_KG" },
    { LANG_LAO,            SUBLANG_NEUTRAL,                     "lo" },
    { LANG_LAO,            SUBLANG_LAO_LAO,                     "lo_LA" },
    { LANG_LATVIAN,        SUBLANG_NEUTRAL,                     "lv" },
    { LANG_LATVIAN,        SUBLANG_LATVIAN_LATVIA,              "lv_LV" },
    { LANG_LITHUANIAN,     SUBLANG_NEUTRAL,                     "lt" },
    { LANG_LITHUANIAN,     SUBLANG_LITHUANIAN,                  "lt_LT" },
    { LANG_MACEDONIAN,     SUBLANG_NEUTRAL,                     "mk" },
    { LANG_MACEDONIAN,     SUBLANG_MACEDONIAN_MACEDONIA,        "mk_MK" },
    { LANG_MALAY,          SUBLANG_NEUTRAL,                     "ms" },
    { LANG_MALAY,          SUBLANG_MALAY_MALAYSIA,              "ms_MY" },
    { LANG_MALAY,          SUBLANG_MALAY_BRUNEI_DARUSSALAM,     "ms_BN" },
    { LANG_MALAYALAM,      SUBLANG_NEUTRAL,                     "ml" },
    { LANG_MALAYALAM,      SUBLANG_MALAYALAM_INDIA,             "ml_IN" },
    { LANG_MALTESE,        SUBLANG_NEUTRAL,                     "mt" },
    { LANG_MALTESE,        SUBLANG_MALTESE_MALTA,               "mt_MT" },
    { LANG_MARATHI,        SUBLANG_NEUTRAL,                     "mr" },
    { LANG_MARATHI,        SUBLANG_MARATHI_INDIA,               "mr_IN" },
    { LANG_MONGOLIAN,      SUBLANG_NEUTRAL,                     "mn" },
    { LANG_MONGOLIAN,      SUBLANG_MONGOLIAN_CYRILLIC_MONGOLIA, "mn_MN" },
    { LANG_MONGOLIAN,      SUBLANG_MONGOLIAN_PRC,               "mn_CN" },
    { LANG_NEPALI,         SUBLANG_NEUTRAL,                     "ne" },
    { LANG_NEPALI,         SUBLANG_NEPALI_NEPAL,                "ne_NP" },
    { LANG_NEPALI,         SUBLANG_NEPALI_INDIA,                "ne_IN" },
    { LANG_NORWEGIAN,      SUBLANG_NORWEGIAN_BOKMAL,            "nb_NO" },
    { LANG_NORWEGIAN,      SUBLANG_NORWEGIAN_NYNORSK,           "nn_NO" },
    { LANG_ODIA,           SUBLANG_NEUTRAL,                     "or" },
    { LANG_ODIA,           SUBLANG_ODIA_INDIA,                  "or_IN" },
    { LANG_PASHTO,         SUBLANG_NEUTRAL,                     "ps" },
    { LANG_PASHTO,         SUBLANG_PASHTO_AFGHANISTAN,          "ps_AF" },
    { LANG_PERSIAN,        SUBLANG_NEUTRAL,                     "fa" },
    { LANG_PERSIAN,        SUBLANG_PERSIAN_IRAN,                "fa_IR" },
    { LANG_POLISH,         SUBLANG_NEUTRAL,                     "pl" },
    { LANG_POLISH,         SUBLANG_POLISH_POLAND,               "pl_PL" },
    { LANG_PORTUGUESE,     SUBLANG_NEUTRAL,                     "pt" },
    { LANG_PORTUGUESE,     SUBLANG_PORTUGUESE_BRAZILIAN,        "pt_BR" },
    { LANG_PORTUGUESE,     SUBLANG_PORTUGUESE_PORTUGAL,         "pt_PT" },
    { LANG_PUNJABI,        SUBLANG_NEUTRAL,                     "pa" },
    { LANG_PUNJABI,        SUBLANG_PUNJABI_INDIA,               "pa_IN" },
    { LANG_PUNJABI,        SUBLANG_PUNJABI_PAKISTAN,            "pa_PK" },
    { LANG_ROMANIAN,       SUBLANG_NEUTRAL,                     "ro" },
    { LANG_ROMANIAN,       SUBLANG_ROMANIAN_ROMANIA,            "ro_RO" },
    { LANG_ROMANSH,        SUBLANG_NEUTRAL,                     "rm" },
    { LANG_ROMANSH,        SUBLANG_ROMANSH_SWITZERLAND,         "rm_CH" },
    { LANG_RUSSIAN,        SUBLANG_NEUTRAL,                     "ru" },
    { LANG_RUSSIAN,        SUBLANG_RUSSIAN_RUSSIA,              "ru_RU" },
    { LANG_SAMI,           SUBLANG_NEUTRAL,                     "se" },
    { LANG_SAMI,           SUBLANG_SAMI_NORTHERN_NORWAY,        "se_NO" },
    { LANG_SAMI,           SUBLANG_SAMI_NORTHERN_SWEDEN,        "se_SE" },
    { LANG_SAMI,           SUBLANG_SAMI_NORTHERN_FINLAND,       "se_FI" },
    { LANG_SANSKRIT,       SUBLANG_NEUTRAL,                     "sa" },
    { LANG_SANSKRIT,       SUBLANG_SANSKRIT_INDIA,              "sa_IN" },
    { LANG_SCOTTISH_GAELIC,SUBLANG_NEUTRAL,                     "gd" },
    { LANG_SCOTTISH_GAELIC,SUBLANG_SCOTTISH_GAELIC,             "gd_GB" },
    /* LANG_SERBIAN/LANG_CROATIAN/LANG_BOSNIAN are the same */
    { LANG_SERBIAN,        SUBLANG_NEUTRAL,                     "hr" },
    { LANG_SERBIAN,        SUBLANG_SERBIAN_CROATIA,             "hr_HR" },
    { LANG_SERBIAN,        SUBLANG_SERBIAN_LATIN,               "sr_RS@latin" },
    { LANG_SERBIAN,        SUBLANG_SERBIAN_CYRILLIC,            "sr_RS@cyrillic" },
    { LANG_SERBIAN,        SUBLANG_CROATIAN_BOSNIA_HERZEGOVINA_LATIN,   "hr_BA@latin" },
    { LANG_SERBIAN,        SUBLANG_BOSNIAN_BOSNIA_HERZEGOVINA_LATIN,    "bs_BA@latin" },
    { LANG_SERBIAN,        SUBLANG_SERBIAN_BOSNIA_HERZEGOVINA_LATIN,    "sr_BA@latin" },
    { LANG_SERBIAN,        SUBLANG_SERBIAN_BOSNIA_HERZEGOVINA_CYRILLIC, "sr_BA@cyrillic" },
    { LANG_SERBIAN,        SUBLANG_BOSNIAN_BOSNIA_HERZEGOVINA_CYRILLIC, "bs_BA@cyrillic" },
    { LANG_SERBIAN,        SUBLANG_SERBIAN_SERBIA_LATIN,        "sr_RS@latin" },
    { LANG_SERBIAN,        SUBLANG_SERBIAN_SERBIA_CYRILLIC,     "sr_RS@cyrillic" },
    { LANG_SERBIAN,        SUBLANG_SERBIAN_MONTENEGRO_LATIN,    "sr_ME@latin" },
    { LANG_SERBIAN,        SUBLANG_SERBIAN_MONTENEGRO_CYRILLIC, "sr_ME@cyrillic" },
    { LANG_SINHALESE,      SUBLANG_NEUTRAL,                     "si" },
    { LANG_SINHALESE,      SUBLANG_SINHALESE_SRI_LANKA,         "si_LK" },
    { LANG_SLOVAK,         SUBLANG_NEUTRAL,                     "sk" },
    { LANG_SLOVAK,         SUBLANG_SLOVAK_SLOVAKIA,             "sk_SK" },
    { LANG_SLOVENIAN,      SUBLANG_NEUTRAL,                     "sl" },
    { LANG_SLOVENIAN,      SUBLANG_SLOVENIAN_SLOVENIA,          "sl_SI" },
    { LANG_SOTHO,          SUBLANG_NEUTRAL,                     "nso" },
    { LANG_SOTHO,          SUBLANG_SOTHO_NORTHERN_SOUTH_AFRICA, "nso_ZA" },
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
    { LANG_SPANISH,        SUBLANG_SPANISH_US,                  "es_US" },
    { LANG_SWAHILI,        SUBLANG_NEUTRAL,                     "sw" },
    { LANG_SWAHILI,        SUBLANG_SWAHILI_KENYA,               "sw_KE" },
    { LANG_SWEDISH,        SUBLANG_NEUTRAL,                     "sv" },
    { LANG_SWEDISH,        SUBLANG_SWEDISH_SWEDEN,              "sv_SE" },
    { LANG_SWEDISH,        SUBLANG_SWEDISH_FINLAND,             "sv_FI" },
    { LANG_SYRIAC,         SUBLANG_NEUTRAL,                     "syr" },
    { LANG_SYRIAC,         SUBLANG_SYRIAC_SYRIA,                "syr_SY" },
    { LANG_TAJIK,          SUBLANG_NEUTRAL,                     "tg" },
    { LANG_TAJIK,          SUBLANG_TAJIK_TAJIKISTAN,            "tg_TJ" },
    { LANG_TAMIL,          SUBLANG_NEUTRAL,                     "ta" },
    { LANG_TAMIL,          SUBLANG_TAMIL_INDIA,                 "ta_IN" },
    { LANG_TATAR,          SUBLANG_NEUTRAL,                     "tt" },
    { LANG_TATAR,          SUBLANG_TATAR_RUSSIA,                "tt_TA" },
    { LANG_TELUGU,         SUBLANG_NEUTRAL,                     "te" },
    { LANG_TELUGU,         SUBLANG_TELUGU_INDIA,                "te_IN" },
    { LANG_THAI,           SUBLANG_NEUTRAL,                     "th" },
    { LANG_THAI,           SUBLANG_THAI_THAILAND,               "th_TH" },
    { LANG_TIGRINYA,       SUBLANG_NEUTRAL,                     "ti" },
    { LANG_TIGRINYA,       SUBLANG_TIGRINYA_ETHIOPIA,           "ti_ET" },
    { LANG_TIGRINYA,       SUBLANG_TIGRINYA_ERITREA,            "ti_ER" },
    { LANG_TSWANA,         SUBLANG_NEUTRAL,                     "tn" },
    { LANG_TSWANA,         SUBLANG_TSWANA_SOUTH_AFRICA,         "tn_ZA" },
    { LANG_TURKISH,        SUBLANG_NEUTRAL,                     "tr" },
    { LANG_TURKISH,        SUBLANG_TURKISH_TURKEY,              "tr_TR" },
    { LANG_UIGHUR,         SUBLANG_NEUTRAL,                     "ug" },
    { LANG_UIGHUR,         SUBLANG_UIGHUR_PRC,                  "ug_CN" },
    { LANG_UKRAINIAN,      SUBLANG_NEUTRAL,                     "uk" },
    { LANG_UKRAINIAN,      SUBLANG_UKRAINIAN_UKRAINE,           "uk_UA" },
    { LANG_URDU,           SUBLANG_NEUTRAL,                     "ur" },
    { LANG_URDU,           SUBLANG_URDU_PAKISTAN,               "ur_PK" },
    { LANG_URDU,           SUBLANG_URDU_INDIA,                  "ur_IN" },
    { LANG_UZBEK,          SUBLANG_NEUTRAL,                     "uz" },
    { LANG_UZBEK,          SUBLANG_UZBEK_LATIN,                 "uz_UZ@latin" },
    { LANG_UZBEK,          SUBLANG_UZBEK_CYRILLIC,              "uz_UZ@cyrillic" },
    { LANG_VIETNAMESE,     SUBLANG_NEUTRAL,                     "vi" },
    { LANG_VIETNAMESE,     SUBLANG_VIETNAMESE_VIETNAM,          "vi_VN" },
    { LANG_WELSH,          SUBLANG_NEUTRAL,                     "cy" },
    { LANG_WELSH,          SUBLANG_WELSH_UNITED_KINGDOM,        "cy_GB" },
    { LANG_WOLOF,          SUBLANG_NEUTRAL,                     "wo" },
    { LANG_WOLOF,          SUBLANG_WOLOF_SENEGAL,               "wo_SN" },
    { LANG_XHOSA,          SUBLANG_NEUTRAL,                     "xh" },
    { LANG_XHOSA,          SUBLANG_XHOSA_SOUTH_AFRICA,          "xh_ZA" },
    { LANG_YORUBA,         SUBLANG_NEUTRAL,                     "yo" },
    { LANG_YORUBA,         SUBLANG_YORUBA_NIGERIA,              "yo_NG" },
    { LANG_ZULU,           SUBLANG_NEUTRAL,                     "zu" },
    { LANG_ZULU,           SUBLANG_ZULU_SOUTH_AFRICA,           "zu_ZA" },

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
#ifdef LANG_MANX_GAELIC
    { LANG_MANX_GAELIC,    SUBLANG_MANX_GAELIC,                 "gv_GB" },
#endif
};

static BOOL is_english( int lan )
{
    return lan == MAKELANGID( LANG_ENGLISH, SUBLANG_DEFAULT );
}

static char *convert_msgid_ascii( const struct lanmsg *msg, int error_on_invalid_char )
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

#ifdef HAVE_LIBGETTEXTPO

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

static void add_po_string( po_file_t po, const struct lanmsg *msgid, const struct lanmsg *msgstr )
{
    po_message_t msg;
    po_message_iterator_t iterator;
    char *id, *id_buffer, *context, *str = NULL, *str_buffer = NULL;

    if (msgid->len <= 1) return;

    id_buffer = id = convert_msgid_ascii( msgid, 1 );
    context = get_message_context( &id );

    if (msgstr)
    {
        int len;
        str_buffer = str = unicode_to_utf8( msgstr->msg, msgstr->len, &len );
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
                           "Report-Msgid-Bugs-To: https://bugs.winehq.org\n"
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
    struct lan_blk *lbp;
    po_file_t po = create_po_file();

    for (lbp = lanblockhead; lbp; lbp = lbp->next)
    {
        if (!is_english( lbp->lan )) continue;
        for (i = 0; i < lbp->nblk; i++)
        {
            struct block *blk = &lbp->blks[i];
            for (j = 0; j < blk->nmsg; j++) add_po_string( po, blk->msgs[j], NULL );
        }
    }
    po_file_write( po, outname, &po_xerror_handler );
    po_file_free( po );
}


#else  /* HAVE_LIBGETTEXTPO */

void write_pot_file( const char *outname )
{
    error( "PO files not supported in this wmc build\n" );
}

#endif

static struct mo_file *mo_file;

static void byteswap( unsigned int *data, unsigned int count )
{
    unsigned int i;

    for (i = 0; i < count; i++)
        data[i] = data[i] >> 24 | (data[i] >> 8 & 0xff00) | (data[i] << 8 & 0xff0000) | data[i] << 24;
}

static void load_mo_file( const char *name )
{
    size_t size;

    if (!(mo_file = read_file( name, &size ))) fatal_perror( "Failed to read %s", name );

    /* sanity checks */

    if (size < sizeof(*mo_file))
        error( "%s is not a valid .mo file\n", name );
    if (mo_file->magic == 0xde120495)
        byteswap( &mo_file->revision, 4 );
    else if (mo_file->magic != 0x950412de)
        error( "%s is not a valid .mo file\n", name );
    if ((mo_file->revision >> 16) > 1)
        error( "%s: unsupported file version %x\n", name, mo_file->revision );
    if (mo_file->msgid_off >= size ||
        mo_file->msgstr_off >= size ||
        size < sizeof(*mo_file) + 2 * 8 * mo_file->count)
        error( "%s: corrupted file\n", name );

    if (mo_file->magic == 0xde120495)
    {
        byteswap( (unsigned int *)((char *)mo_file + mo_file->msgid_off), 2 * mo_file->count );
        byteswap( (unsigned int *)((char *)mo_file + mo_file->msgstr_off), 2 * mo_file->count );
    }
}

static void free_mo_file(void)
{
    free( mo_file );
    mo_file = NULL;
}

static inline const char *get_mo_msgid( int index )
{
    const char *base = (const char *)mo_file;
    const unsigned int *offsets = (const unsigned int *)(base + mo_file->msgid_off);
    return base + offsets[2 * index + 1];
}

static inline const char *get_mo_msgstr( int index )
{
    const char *base = (const char *)mo_file;
    const unsigned int *offsets = (const unsigned int *)(base + mo_file->msgstr_off);
    return base + offsets[2 * index + 1];
}

static const char *get_msgstr( const char *msgid, const char *context, int *found )
{
    int pos, res, min, max;
    const char *ret = msgid;
    char *id = NULL;

    if (!mo_file)  /* strings containing a context still need to be transformed */
    {
        if (context) (*found)++;
        return ret;
    }

    if (context) id = strmake( "%s%c%s", context, 4, msgid );
    min = 0;
    max = mo_file->count - 1;
    while (min <= max)
    {
        pos = (min + max) / 2;
        res = strcmp( get_mo_msgid(pos), id ? id : msgid );
        if (!res)
        {
            const char *str = get_mo_msgstr( pos );
            if (str[0])  /* ignore empty strings */
            {
                ret = str;
                (*found)++;
            }
            break;
        }
        if (res > 0) max = pos - 1;
        else min = pos + 1;
    }
    free( id );
    return ret;
}

static struct lanmsg *translate_string( struct lanmsg *str, int lang, int *found )
{
    struct lanmsg *new;
    const char *transl;
    char *buffer, *msgid, *context;

    if (str->len <= 1 || !(buffer = convert_msgid_ascii( str, 0 ))) return str;

    msgid = buffer;
    context = get_message_context( &msgid );
    transl = get_msgstr( msgid, context, found );

    new = xmalloc( sizeof(*new) );
    new->lan  = lang;
    new->cp   = 0;  /* FIXME */
    new->file = str->file;
    new->line = str->line;
    new->msg  = utf8_to_unicode( transl, strlen(transl) + 1, &new->len );
    free( buffer );
    return new;
}

static void translate_block( struct block *blk, struct block *new, int lang, int *found )
{
    int i;

    new->idlo = blk->idlo;
    new->idhi = blk->idhi;
    new->size = 0;
    new->msgs = xmalloc( blk->nmsg * sizeof(*new->msgs) );
    new->nmsg = blk->nmsg;
    for (i = 0; i < blk->nmsg; i++)
    {
        new->msgs[i] = translate_string( blk->msgs[i], lang, found );
        new->size += ((2 * new->msgs[i]->len + 3) & ~3) + 4;
    }
}

static void translate_messages( int lang )
{
    int i, found;
    struct lan_blk *lbp, *new;

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
            translate_block( &lbp->blks[i], &new->blks[i], lang, &found );
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
    struct lan_blk *lbp;
    char buffer[256];
    char *p, *tok, *name;
    unsigned int i;
    FILE *f;

    /* first check if we have English resources to translate */
    for (lbp = lanblockhead; lbp; lbp = lbp->next) if (is_english( lbp->lan )) break;
    if (!lbp) return;

    if (!po_dir)  /* run through the translation process to remove msg contexts */
    {
        translate_messages( MAKELANGID( LANG_ENGLISH, SUBLANG_DEFAULT ));
        goto done;
    }

    new_top = new_tail = NULL;

    name = strmake( "%s/LINGUAS", po_dir );
    if (!(f = fopen( name, "r" ))) return;
    free( name );
    while (fgets( buffer, sizeof(buffer), f ))
    {
        if ((p = strchr( buffer, '#' ))) *p = 0;
        for (tok = strtok( buffer, " \t\r\n" ); tok; tok = strtok( NULL, " \t\r\n" ))
        {
            for (i = 0; i < ARRAY_SIZE(languages); i++)
                if (!strcmp( tok, languages[i].name )) break;

            if (i == ARRAY_SIZE(languages))
                error( "unknown language '%s'\n", tok );

            name = strmake( "%s/%s.mo", po_dir, tok );
            load_mo_file( name );
            translate_messages( MAKELANGID(languages[i].id, languages[i].sub) );
            free_mo_file();
            free( name );
        }
    }
    fclose( f );

done:
    /* prepend the translated messages to the global list */
    if (new_tail)
    {
        new_tail->next = lanblockhead;
        lanblockhead->prev = new_tail;
        lanblockhead = new_top;
    }
}
