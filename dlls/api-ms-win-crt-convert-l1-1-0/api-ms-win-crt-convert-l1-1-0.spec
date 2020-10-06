@ cdecl __toascii(long) ucrtbase.__toascii
@ cdecl _atodbl(ptr str) ucrtbase._atodbl
@ cdecl _atodbl_l(ptr str ptr) ucrtbase._atodbl_l
@ cdecl _atof_l(str ptr) ucrtbase._atof_l
@ cdecl _atoflt(ptr str) ucrtbase._atoflt
@ cdecl _atoflt_l(ptr str ptr) ucrtbase._atoflt_l
@ cdecl -ret64 _atoi64(str) ucrtbase._atoi64
@ cdecl -ret64 _atoi64_l(str ptr) ucrtbase._atoi64_l
@ cdecl _atoi_l(str ptr) ucrtbase._atoi_l
@ cdecl _atol_l(str ptr) ucrtbase._atol_l
@ cdecl _atoldbl(ptr str) ucrtbase._atoldbl
@ cdecl _atoldbl_l(ptr str ptr) ucrtbase._atoldbl_l
@ cdecl -ret64 _atoll_l(str ptr) ucrtbase._atoll_l
@ cdecl _ecvt(double long ptr ptr) ucrtbase._ecvt
@ cdecl _ecvt_s(str long double long ptr ptr) ucrtbase._ecvt_s
@ cdecl _fcvt(double long ptr ptr) ucrtbase._fcvt
@ cdecl _fcvt_s(ptr long double long ptr ptr) ucrtbase._fcvt_s
@ cdecl _gcvt(double long str) ucrtbase._gcvt
@ cdecl _gcvt_s(ptr long double long) ucrtbase._gcvt_s
@ cdecl _i64toa(int64 ptr long) ucrtbase._i64toa
@ cdecl _i64toa_s(int64 ptr long long) ucrtbase._i64toa_s
@ cdecl _i64tow(int64 ptr long) ucrtbase._i64tow
@ cdecl _i64tow_s(int64 ptr long long) ucrtbase._i64tow_s
@ cdecl _itoa(long ptr long) ucrtbase._itoa
@ cdecl _itoa_s(long ptr long long) ucrtbase._itoa_s
@ cdecl _itow(long ptr long) ucrtbase._itow
@ cdecl _itow_s(long ptr long long) ucrtbase._itow_s
@ cdecl _ltoa(long ptr long) ucrtbase._ltoa
@ cdecl _ltoa_s(long ptr long long) ucrtbase._ltoa_s
@ cdecl _ltow(long ptr long) ucrtbase._ltow
@ cdecl _ltow_s(long ptr long long) ucrtbase._ltow_s
@ cdecl _strtod_l(str ptr ptr) ucrtbase._strtod_l
@ cdecl _strtof_l(str ptr ptr) ucrtbase._strtof_l
@ cdecl -ret64 _strtoi64(str ptr long) ucrtbase._strtoi64
@ cdecl -ret64 _strtoi64_l(str ptr long ptr) ucrtbase._strtoi64_l
@ cdecl -ret64 _strtoimax_l(str ptr long ptr) ucrtbase._strtoimax_l
@ cdecl _strtol_l(str ptr long ptr) ucrtbase._strtol_l
@ cdecl _strtold_l(str ptr ptr) ucrtbase._strtold_l
@ cdecl -ret64 _strtoll_l(str ptr long ptr) ucrtbase._strtoll_l
@ cdecl -ret64 _strtoui64(str ptr long) ucrtbase._strtoui64
@ cdecl -ret64 _strtoui64_l(str ptr long ptr) ucrtbase._strtoui64_l
@ cdecl _strtoul_l(str ptr long ptr) ucrtbase._strtoul_l
@ cdecl -ret64 _strtoull_l(str ptr long ptr) ucrtbase._strtoull_l
@ cdecl -ret64 _strtoumax_l(str ptr long ptr) ucrtbase._strtoumax_l
@ cdecl _ui64toa(int64 ptr long) ucrtbase._ui64toa
@ cdecl _ui64toa_s(int64 ptr long long) ucrtbase._ui64toa_s
@ cdecl _ui64tow(int64 ptr long) ucrtbase._ui64tow
@ cdecl _ui64tow_s(int64 ptr long long) ucrtbase._ui64tow_s
@ cdecl _ultoa(long ptr long) ucrtbase._ultoa
@ cdecl _ultoa_s(long ptr long long) ucrtbase._ultoa_s
@ cdecl _ultow(long ptr long) ucrtbase._ultow
@ cdecl _ultow_s(long ptr long long) ucrtbase._ultow_s
@ cdecl _wcstod_l(wstr ptr ptr) ucrtbase._wcstod_l
@ cdecl _wcstof_l(wstr ptr ptr) ucrtbase._wcstof_l
@ cdecl -ret64 _wcstoi64(wstr ptr long) ucrtbase._wcstoi64
@ cdecl -ret64 _wcstoi64_l(wstr ptr long ptr) ucrtbase._wcstoi64_l
@ stub _wcstoimax_l
@ cdecl _wcstol_l(wstr ptr long ptr) ucrtbase._wcstol_l
@ cdecl _wcstold_l(wstr ptr ptr) ucrtbase._wcstold_l
@ cdecl -ret64 _wcstoll_l(wstr ptr long ptr) ucrtbase._wcstoll_l
@ cdecl _wcstombs_l(ptr ptr long ptr) ucrtbase._wcstombs_l
@ cdecl _wcstombs_s_l(ptr ptr long wstr long ptr) ucrtbase._wcstombs_s_l
@ cdecl -ret64 _wcstoui64(wstr ptr long) ucrtbase._wcstoui64
@ cdecl -ret64 _wcstoui64_l(wstr ptr long ptr) ucrtbase._wcstoui64_l
@ cdecl _wcstoul_l(wstr ptr long ptr) ucrtbase._wcstoul_l
@ cdecl -ret64 _wcstoull_l(wstr ptr long ptr) ucrtbase._wcstoull_l
@ stub _wcstoumax_l
@ cdecl _wctomb_l(ptr long ptr) ucrtbase._wctomb_l
@ cdecl _wctomb_s_l(ptr ptr long long ptr) ucrtbase._wctomb_s_l
@ cdecl _wtof(wstr) ucrtbase._wtof
@ cdecl _wtof_l(wstr ptr) ucrtbase._wtof_l
@ cdecl _wtoi(wstr) ucrtbase._wtoi
@ cdecl -ret64 _wtoi64(wstr) ucrtbase._wtoi64
@ cdecl -ret64 _wtoi64_l(wstr ptr) ucrtbase._wtoi64_l
@ cdecl _wtoi_l(wstr ptr) ucrtbase._wtoi_l
@ cdecl _wtol(wstr) ucrtbase._wtol
@ cdecl _wtol_l(wstr ptr) ucrtbase._wtol_l
@ cdecl -ret64 _wtoll(wstr) ucrtbase._wtoll
@ cdecl -ret64 _wtoll_l(wstr ptr) ucrtbase._wtoll_l
@ cdecl atof(str) ucrtbase.atof
@ cdecl atoi(str) ucrtbase.atoi
@ cdecl atol(str) ucrtbase.atol
@ cdecl -ret64 atoll(str) ucrtbase.atoll
@ cdecl btowc(long) ucrtbase.btowc
@ stub c16rtomb
@ stub c32rtomb
@ stub mbrtoc16
@ stub mbrtoc32
@ cdecl mbrtowc(ptr str long ptr) ucrtbase.mbrtowc
@ cdecl mbsrtowcs(ptr ptr long ptr) ucrtbase.mbsrtowcs
@ cdecl mbsrtowcs_s(ptr ptr long ptr long ptr) ucrtbase.mbsrtowcs_s
@ cdecl mbstowcs(ptr str long) ucrtbase.mbstowcs
@ cdecl mbstowcs_s(ptr ptr long str long) ucrtbase.mbstowcs_s
@ cdecl mbtowc(ptr str long) ucrtbase.mbtowc
@ cdecl strtod(str ptr) ucrtbase.strtod
@ cdecl strtof(str ptr) ucrtbase.strtof
@ cdecl -ret64 strtoimax(str ptr long) ucrtbase.strtoimax
@ cdecl strtol(str ptr long) ucrtbase.strtol
@ cdecl strtold(str ptr) ucrtbase.strtold
@ cdecl -ret64 strtoll(str ptr long) ucrtbase.strtoll
@ cdecl strtoul(str ptr long) ucrtbase.strtoul
@ cdecl -ret64 strtoull(str ptr long) ucrtbase.strtoull
@ cdecl -ret64 strtoumax(str ptr long) ucrtbase.strtoumax
@ cdecl wcrtomb(ptr long ptr) ucrtbase.wcrtomb
@ cdecl wcrtomb_s(ptr ptr long long ptr) ucrtbase.wcrtomb_s
@ cdecl wcsrtombs(ptr ptr long ptr) ucrtbase.wcsrtombs
@ cdecl wcsrtombs_s(ptr ptr long ptr long ptr) ucrtbase.wcsrtombs_s
@ cdecl wcstod(wstr ptr) ucrtbase.wcstod
@ cdecl wcstof(ptr ptr) ucrtbase.wcstof
@ stub wcstoimax
@ cdecl wcstol(wstr ptr long) ucrtbase.wcstol
@ cdecl wcstold(wstr ptr) ucrtbase.wcstold
@ cdecl -ret64 wcstoll(wstr ptr long) ucrtbase.wcstoll
@ cdecl wcstombs(ptr ptr long) ucrtbase.wcstombs
@ cdecl wcstombs_s(ptr ptr long wstr long) ucrtbase.wcstombs_s
@ cdecl wcstoul(wstr ptr long) ucrtbase.wcstoul
@ cdecl -ret64 wcstoull(wstr ptr long) ucrtbase.wcstoull
@ stub wcstoumax
@ cdecl wctob(long) ucrtbase.wctob
@ cdecl wctomb(ptr long) ucrtbase.wctomb
@ cdecl wctomb_s(ptr ptr long long) ucrtbase.wctomb_s
@ cdecl wctrans(str) ucrtbase.wctrans
