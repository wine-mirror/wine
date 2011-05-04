/*
 * Copyright 2011 Alexandre Julliard
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

#include <stdarg.h>
#include <limits.h>
#include <float.h>

#include "msvcp90.h"
#include "windef.h"
#include "winbase.h"

enum std_float_denorm_style
{
    denorm_indeterminate = -1,
    denorm_absent = 0,
    denorm_present = 1
};

enum std_float_round_style
{
    round_indeterminate = -1,
    round_toward_zero = 0,
    round_to_nearest = 1,
    round_toward_infinity = 2,
    round_toward_neg_infinity = 3
};

/* ?digits10@?$numeric_limits@C@std@@2HB -> public: static int const std::numeric_limits<signed char>::digits10 */
const int std_numeric_limits_signed_char_digits10 = 2;

/* ?digits10@?$numeric_limits@D@std@@2HB -> public: static int const std::numeric_limits<char>::digits10 */
const int std_numeric_limits_char_digits10 = 2;

/* ?digits10@?$numeric_limits@E@std@@2HB -> public: static int const std::numeric_limits<unsigned char>::digits10 */
const int std_numeric_limits_unsigned_char_digits10 = 2;

/* ?digits10@?$numeric_limits@F@std@@2HB -> public: static int const std::numeric_limits<short>::digits10 */
const int std_numeric_limits_short_digits10 = 4;

/* ?digits10@?$numeric_limits@G@std@@2HB -> public: static int const std::numeric_limits<unsigned short>::digits10 */
const int std_numeric_limits_unsigned_short_digits10 = 4;

/* ?digits10@?$numeric_limits@H@std@@2HB -> public: static int const std::numeric_limits<int>::digits10 */
const int std_numeric_limits_int_digits10 = 9;

/* ?digits10@?$numeric_limits@I@std@@2HB -> public: static int const std::numeric_limits<unsigned int>::digits10 */
const int std_numeric_limits_unsigned_int_digits10 = 9;

/* ?digits10@?$numeric_limits@J@std@@2HB -> public: static int const std::numeric_limits<long>::digits10 */
const int std_numeric_limits_long_digits10 = 9;

/* ?digits10@?$numeric_limits@K@std@@2HB -> public: static int const std::numeric_limits<unsigned long>::digits10 */
const int std_numeric_limits_unsigned_long_digits10 = 9;

/* ?digits10@?$numeric_limits@M@std@@2HB -> public: static int const std::numeric_limits<float>::digits10 */
const int std_numeric_limits_float_digits10 = FLT_DIG;

/* ?digits10@?$numeric_limits@N@std@@2HB -> public: static int const std::numeric_limits<double>::digits10 */
const int std_numeric_limits_double_digits10 = DBL_DIG;

/* ?digits10@?$numeric_limits@O@std@@2HB -> public: static int const std::numeric_limits<long double>::digits10 */
const int std_numeric_limits_long_double_digits10 = LDBL_DIG;

/* ?digits10@?$numeric_limits@_J@std@@2HB -> public: static int const std::numeric_limits<__int64>::digits10 */
const int std_numeric_limits_int64_digits10 = 18;

/* ?digits10@?$numeric_limits@_K@std@@2HB -> public: static int const std::numeric_limits<unsigned __int64>::digits10 */
const int std_numeric_limits_unsigned_int64_digits10 = 18;

/* ?digits10@?$numeric_limits@_N@std@@2HB -> public: static int const std::numeric_limits<bool>::digits10 */
const int std_numeric_limits_bool_digits10 = 0;

/* ?digits10@?$numeric_limits@_W@std@@2HB -> public: static int const std::numeric_limits<wchar_t>::digits10 */
const int std_numeric_limits_wchar_t_digits10 = 4;

/* ?digits10@_Num_base@std@@2HB -> public: static int const std::_Num_base::digits10 */
const int std_Num_base_digits10 = 0;

/* ?digits@?$numeric_limits@C@std@@2HB -> public: static int const std::numeric_limits<signed char>::digits */
const int std_numeric_limits_signed_char_digits = 7;

/* ?digits@?$numeric_limits@D@std@@2HB -> public: static int const std::numeric_limits<char>::digits */
const int std_numeric_limits_char_digits = (CHAR_MIN < 0) ? 7 : 8;

/* ?digits@?$numeric_limits@E@std@@2HB -> public: static int const std::numeric_limits<unsigned char>::digits */
const int std_numeric_limits_unsigned_char_digits = 8;

/* ?digits@?$numeric_limits@F@std@@2HB -> public: static int const std::numeric_limits<short>::digits */
const int std_numeric_limits_short_digits = 15;

/* ?digits@?$numeric_limits@G@std@@2HB -> public: static int const std::numeric_limits<unsigned short>::digits */
const int std_numeric_limits_unsigned_short_digits = 16;

/* ?digits@?$numeric_limits@H@std@@2HB -> public: static int const std::numeric_limits<int>::digits */
const int std_numeric_limits_int_digits = 31;

/* ?digits@?$numeric_limits@I@std@@2HB -> public: static int const std::numeric_limits<unsigned int>::digits */
const int std_numeric_limits_unsigned_int_digits = 32;

/* ?digits@?$numeric_limits@J@std@@2HB -> public: static int const std::numeric_limits<long>::digits */
const int std_numeric_limits_long_digits = 31;

/* ?digits@?$numeric_limits@K@std@@2HB -> public: static int const std::numeric_limits<unsigned long>::digits */
const int std_numeric_limits_unsigned_long_digits = 32;

/* ?digits@?$numeric_limits@M@std@@2HB -> public: static int const std::numeric_limits<float>::digits */
const int std_numeric_limits_float_digits = FLT_MANT_DIG;

/* ?digits@?$numeric_limits@N@std@@2HB -> public: static int const std::numeric_limits<double>::digits */
const int std_numeric_limits_double_digits = DBL_MANT_DIG;

/* ?digits@?$numeric_limits@O@std@@2HB -> public: static int const std::numeric_limits<long double>::digits */
const int std_numeric_limits_long_double_digits = LDBL_MANT_DIG;

/* ?digits@?$numeric_limits@_J@std@@2HB -> public: static int const std::numeric_limits<__int64>::digits */
const int std_numeric_limits_int64_digits = 63;

/* ?digits@?$numeric_limits@_K@std@@2HB -> public: static int const std::numeric_limits<unsigned __int64>::digits */
const int std_numeric_limits_unsigned_int64_digits = 64;

/* ?digits@?$numeric_limits@_N@std@@2HB -> public: static int const std::numeric_limits<bool>::digits */
const int std_numeric_limits_bool_digits = 1;

/* ?digits@?$numeric_limits@_W@std@@2HB -> public: static int const std::numeric_limits<wchar_t>::digits */
const int std_numeric_limits_wchar_t_digits = 16;

/* ?digits@_Num_base@std@@2HB -> public: static int const std::_Num_base::digits */
const int std_Num_base_digits = 0;

/* ?has_denorm@_Num_base@std@@2W4float_denorm_style@2@B -> public: static enum std::float_denorm_style const std::_Num_base::has_denorm */
const enum std_float_denorm_style std_Num_base_has_denorm = denorm_absent;

/* ?has_denorm@_Num_float_base@std@@2W4float_denorm_style@2@B -> public: static enum std::float_denorm_style const std::_Num_float_base::has_denorm */
const enum std_float_denorm_style std_Num_float_base_has_denorm = denorm_present;

/* ?has_denorm_loss@_Num_base@std@@2_NB -> public: static bool const std::_Num_base::has_denorm_loss */
const BOOLEAN std_Num_base_has_denorm_loss = FALSE;

/* ?has_denorm_loss@_Num_float_base@std@@2_NB -> public: static bool const std::_Num_float_base::has_denorm_loss */
const BOOLEAN std_Num_float_base_has_denorm_loss = TRUE;

/* ?has_infinity@_Num_base@std@@2_NB -> public: static bool const std::_Num_base::has_infinity */
const BOOLEAN std_Num_base_has_infinity = FALSE;

/* ?has_infinity@_Num_float_base@std@@2_NB -> public: static bool const std::_Num_float_base::has_infinity */
const BOOLEAN std_Num_float_base_has_infinity = TRUE;

/* ?has_quiet_NaN@_Num_base@std@@2_NB -> public: static bool const std::_Num_base::has_quiet_NaN */
const BOOLEAN std_Num_base_has_quiet_NaN = FALSE;

/* ?has_quiet_NaN@_Num_float_base@std@@2_NB -> public: static bool const std::_Num_float_base::has_quiet_NaN */
const BOOLEAN std_Num_float_base_has_quiet_NaN = TRUE;

/* ?has_signaling_NaN@_Num_base@std@@2_NB -> public: static bool const std::_Num_base::has_signaling_NaN */
const BOOLEAN std_Num_base_has_signaling_NaN = FALSE;

/* ?has_signaling_NaN@_Num_float_base@std@@2_NB -> public: static bool const std::_Num_float_base::has_signaling_NaN */
const BOOLEAN std_Num_float_base_has_signaling_NaN = TRUE;

/* ?is_bounded@_Num_base@std@@2_NB -> public: static bool const std::_Num_base::is_bounded */
const BOOLEAN std_Num_base_is_bounded = FALSE;

/* ?is_bounded@_Num_float_base@std@@2_NB -> public: static bool const std::_Num_float_base::is_bounded */
const BOOLEAN std_Num_float_base_is_bounded = TRUE;

/* ?is_bounded@_Num_int_base@std@@2_NB -> public: static bool const std::_Num_int_base::is_bounded */
const BOOLEAN std_Num_int_base_is_bounded = TRUE;

/* ?is_exact@_Num_base@std@@2_NB -> public: static bool const std::_Num_base::is_exact */
const BOOLEAN std_Num_base_is_exact = FALSE;

/* ?is_exact@_Num_float_base@std@@2_NB -> public: static bool const std::_Num_float_base::is_exact */
const BOOLEAN std_Num_float_base_is_exact = FALSE;

/* ?is_exact@_Num_int_base@std@@2_NB -> public: static bool const std::_Num_int_base::is_exact */
const BOOLEAN std_Num_int_base_is_exact = TRUE;

/* ?is_iec559@_Num_base@std@@2_NB -> public: static bool const std::_Num_base::is_iec559 */
const BOOLEAN std_Num_base_is_iec559 = FALSE;

/* ?is_iec559@_Num_float_base@std@@2_NB -> public: static bool const std::_Num_float_base::is_iec559 */
const BOOLEAN std_Num_float_base_is_iec559 = TRUE;

/* ?is_integer@_Num_base@std@@2_NB -> public: static bool const std::_Num_base::is_integer */
const BOOLEAN std_Num_base_is_integer = FALSE;

/* ?is_integer@_Num_float_base@std@@2_NB -> public: static bool const std::_Num_float_base::is_integer */
const BOOLEAN std_Num_float_base_is_integer = FALSE;

/* ?is_integer@_Num_int_base@std@@2_NB -> public: static bool const std::_Num_int_base::is_integer */
const BOOLEAN std_Num_int_base_is_integer = TRUE;

/* ?is_modulo@?$numeric_limits@_N@std@@2_NB -> public: static bool const std::numeric_limits<bool>::is_modulo */
const BOOLEAN std_numeric_limits_bool_is_modulo = FALSE;

/* ?is_modulo@_Num_base@std@@2_NB -> public: static bool const std::_Num_base::is_modulo */
const BOOLEAN std_Num_base_is_modulo = FALSE;

/* ?is_modulo@_Num_float_base@std@@2_NB -> public: static bool const std::_Num_float_base::is_modulo */
const BOOLEAN std_Num_float_base_is_modulo = FALSE;

/* ?is_modulo@_Num_int_base@std@@2_NB -> public: static bool const std::_Num_int_base::is_modulo */
const BOOLEAN std_Num_int_base_is_modulo = TRUE;

/* ?is_signed@?$numeric_limits@C@std@@2_NB -> public: static bool const std::numeric_limits<signed char>::is_signed */
const BOOLEAN std_numeric_limits_signed_char_is_signed = TRUE;

/* ?is_signed@?$numeric_limits@D@std@@2_NB -> public: static bool const std::numeric_limits<char>::is_signed */
const BOOLEAN std_numeric_limits_char_is_signed = (CHAR_MIN < 0);

/* ?is_signed@?$numeric_limits@E@std@@2_NB -> public: static bool const std::numeric_limits<unsigned char>::is_signed */
const BOOLEAN std_numeric_limits_unsigned_char_is_signed = FALSE;

/* ?is_signed@?$numeric_limits@F@std@@2_NB -> public: static bool const std::numeric_limits<short>::is_signed */
const BOOLEAN std_numeric_limits_short_is_signed = TRUE;

/* ?is_signed@?$numeric_limits@G@std@@2_NB -> public: static bool const std::numeric_limits<unsigned short>::is_signed */
const BOOLEAN std_numeric_limits_unsigned_short_is_signed = FALSE;

/* ?is_signed@?$numeric_limits@H@std@@2_NB -> public: static bool const std::numeric_limits<int>::is_signed */
const BOOLEAN std_numeric_limits_int_is_signed = TRUE;

/* ?is_signed@?$numeric_limits@I@std@@2_NB -> public: static bool const std::numeric_limits<unsigned int>::is_signed */
const BOOLEAN std_numeric_limits_unsigned_int_is_signed = FALSE;

/* ?is_signed@?$numeric_limits@J@std@@2_NB -> public: static bool const std::numeric_limits<long>::is_signed */
const BOOLEAN std_numeric_limits_long_is_signed = TRUE;

/* ?is_signed@?$numeric_limits@K@std@@2_NB -> public: static bool const std::numeric_limits<unsigned long>::is_signed */
const BOOLEAN std_numeric_limits_unsigned_long_is_signed = FALSE;

/* ?is_signed@?$numeric_limits@_J@std@@2_NB -> public: static bool const std::numeric_limits<__int64>::is_signed */
const BOOLEAN std_numeric_limits_int64_is_signed = TRUE;

/* ?is_signed@?$numeric_limits@_K@std@@2_NB -> public: static bool const std::numeric_limits<unsigned __int64>::is_signed */
const BOOLEAN std_numeric_limits_unsigned_int64_is_signed = FALSE;

/* ?is_signed@?$numeric_limits@_N@std@@2_NB -> public: static bool const std::numeric_limits<bool>::is_signed */
const BOOLEAN std_numeric_limits_bool_is_signed = FALSE;

/* ?is_signed@?$numeric_limits@_W@std@@2_NB -> public: static bool const std::numeric_limits<wchar_t>::is_signed */
const BOOLEAN std_numeric_limits_wchar_t_is_signed = FALSE;

/* ?is_signed@_Num_base@std@@2_NB -> public: static bool const std::_Num_base::is_signed */
const BOOLEAN std_Num_base_is_signed = FALSE;

/* ?is_signed@_Num_float_base@std@@2_NB -> public: static bool const std::_Num_float_base::is_signed */
const BOOLEAN std_Num_float_base_is_signed = TRUE;

/* ?is_specialized@_Num_base@std@@2_NB -> public: static bool const std::_Num_base::is_specialized */
const BOOLEAN std_Num_base_is_specialized = FALSE;

/* ?is_specialized@_Num_float_base@std@@2_NB -> public: static bool const std::_Num_float_base::is_specialized */
const BOOLEAN std_Num_float_base_is_specialized = TRUE;

/* ?is_specialized@_Num_int_base@std@@2_NB -> public: static bool const std::_Num_int_base::is_specialized */
const BOOLEAN std_Num_int_base_is_specialized = TRUE;

/* ?max_exponent10@?$numeric_limits@M@std@@2HB -> public: static int const std::numeric_limits<float>::max_exponent10 */
const int std_numeric_limits_float_max_exponent10 = FLT_MAX_10_EXP;

/* ?max_exponent10@?$numeric_limits@N@std@@2HB -> public: static int const std::numeric_limits<double>::max_exponent10 */
const int std_numeric_limits_double_max_exponent10 = DBL_MAX_10_EXP;

/* ?max_exponent10@?$numeric_limits@O@std@@2HB -> public: static int const std::numeric_limits<long double>::max_exponent10 */
const int std_numeric_limits_long_double_max_exponent10 = LDBL_MAX_10_EXP;

/* ?max_exponent10@_Num_base@std@@2HB -> public: static int const std::_Num_base::max_exponent10 */
const int std_Num_base_max_exponent10 = 0;

/* ?max_exponent@?$numeric_limits@M@std@@2HB -> public: static int const std::numeric_limits<float>::max_exponent */
const int std_numeric_limits_float_max_exponent = FLT_MAX_EXP;

/* ?max_exponent@?$numeric_limits@N@std@@2HB -> public: static int const std::numeric_limits<double>::max_exponent */
const int std_numeric_limits_double_max_exponent = DBL_MAX_EXP;

/* ?max_exponent@?$numeric_limits@O@std@@2HB -> public: static int const std::numeric_limits<long double>::max_exponent */
const int std_numeric_limits_long_double_max_exponent = LDBL_MAX_EXP;

/* ?max_exponent@_Num_base@std@@2HB -> public: static int const std::_Num_base::max_exponent */
const int std_Num_base_max_exponent = 0;

/* ?min_exponent10@?$numeric_limits@M@std@@2HB -> public: static int const std::numeric_limits<float>::min_exponent10 */
const int std_numeric_limits_float_min_exponent10 = FLT_MIN_10_EXP;

/* ?min_exponent10@?$numeric_limits@N@std@@2HB -> public: static int const std::numeric_limits<double>::min_exponent10 */
const int std_numeric_limits_double_min_exponent10 = DBL_MIN_10_EXP;

/* ?min_exponent10@?$numeric_limits@O@std@@2HB -> public: static int const std::numeric_limits<long double>::min_exponent10 */
const int std_numeric_limits_long_double_min_exponent10 = LDBL_MIN_10_EXP;

/* ?min_exponent10@_Num_base@std@@2HB -> public: static int const std::_Num_base::min_exponent10 */
const int std_Num_base_min_exponent10 = 0;

/* ?min_exponent@?$numeric_limits@M@std@@2HB -> public: static int const std::numeric_limits<float>::min_exponent */
const int std_numeric_limits_float_min_exponent = FLT_MIN_EXP;

/* ?min_exponent@?$numeric_limits@N@std@@2HB -> public: static int const std::numeric_limits<double>::min_exponent */
const int std_numeric_limits_double_min_exponent = DBL_MIN_EXP;

/* ?min_exponent@?$numeric_limits@O@std@@2HB -> public: static int const std::numeric_limits<long double>::min_exponent */
const int std_numeric_limits_long_double_min_exponent = LDBL_MIN_EXP;

/* ?min_exponent@_Num_base@std@@2HB -> public: static int const std::_Num_base::min_exponent */
const int std_Num_base_min_exponent = 0;

/* ?radix@_Num_base@std@@2HB -> public: static int const std::_Num_base::radix */
const int std_Num_base_radix = 0;

/* ?radix@_Num_float_base@std@@2HB -> public: static int const std::_Num_float_base::radix */
const int std_Num_float_base_radix = FLT_RADIX;

/* ?radix@_Num_int_base@std@@2HB -> public: static int const std::_Num_int_base::radix */
const int std_Num_int_base_radix = 2;

/* ?round_style@_Num_base@std@@2W4float_round_style@2@B -> public: static enum std::float_round_style const std::_Num_base::round_style */
const enum std_float_round_style std_Num_base_round_style = round_toward_zero;

/* ?round_style@_Num_float_base@std@@2W4float_round_style@2@B -> public: static enum std::float_round_style const std::_Num_float_base::round_style */
const enum std_float_round_style std_Num_float_base_round_style = round_to_nearest;

/* ?tinyness_before@_Num_base@std@@2_NB -> public: static bool const  std::_Num_base::tinyness_before */
const BOOLEAN std_Num_base_tinyness_before = FALSE;

/* ?tinyness_before@_Num_float_base@std@@2_NB -> public: static bool const  std::_Num_float_base::tinyness_before */
const BOOLEAN std_Num_float_base_tinyness_before = TRUE;

/* ?traps@_Num_base@std@@2_NB -> public: static bool const  std::_Num_base::traps */
const BOOLEAN std_Num_base_traps = FALSE;

/* ?traps@_Num_float_base@std@@2_NB -> public: static bool const  std::_Num_float_base::traps */
const BOOLEAN std_Num_float_base_traps = TRUE;
