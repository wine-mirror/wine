@ cdecl -arch=i386 _CIacos() ucrtbase._CIacos
@ cdecl -arch=i386 _CIasin() ucrtbase._CIasin
@ cdecl -arch=i386 _CIatan() ucrtbase._CIatan
@ cdecl -arch=i386 _CIatan2() ucrtbase._CIatan2
@ cdecl -arch=i386 _CIcos() ucrtbase._CIcos
@ cdecl -arch=i386 _CIcosh() ucrtbase._CIcosh
@ cdecl -arch=i386 _CIexp() ucrtbase._CIexp
@ cdecl -arch=i386 _CIfmod() ucrtbase._CIfmod
@ cdecl -arch=i386 _CIlog() ucrtbase._CIlog
@ cdecl -arch=i386 _CIlog10() ucrtbase._CIlog10
@ cdecl -arch=i386 _CIpow() ucrtbase._CIpow
@ cdecl -arch=i386 _CIsin() ucrtbase._CIsin
@ cdecl -arch=i386 _CIsinh() ucrtbase._CIsinh
@ cdecl -arch=i386 _CIsqrt() ucrtbase._CIsqrt
@ cdecl -arch=i386 _CItan() ucrtbase._CItan
@ cdecl -arch=i386 _CItanh() ucrtbase._CItanh
@ cdecl _Cbuild(ptr double double) ucrtbase._Cbuild
@ stub _Cmulcc
@ stub _Cmulcr
@ stub _FCbuild
@ stub _FCmulcc
@ stub _FCmulcr
@ stub _LCbuild
@ stub _LCmulcc
@ stub _LCmulcr
@ cdecl -arch=i386 -norelay __libm_sse2_acos() ucrtbase.__libm_sse2_acos
@ cdecl -arch=i386 -norelay __libm_sse2_acosf() ucrtbase.__libm_sse2_acosf
@ cdecl -arch=i386 -norelay __libm_sse2_asin() ucrtbase.__libm_sse2_asin
@ cdecl -arch=i386 -norelay __libm_sse2_asinf() ucrtbase.__libm_sse2_asinf
@ cdecl -arch=i386 -norelay __libm_sse2_atan() ucrtbase.__libm_sse2_atan
@ cdecl -arch=i386 -norelay __libm_sse2_atan2() ucrtbase.__libm_sse2_atan2
@ cdecl -arch=i386 -norelay __libm_sse2_atanf() ucrtbase.__libm_sse2_atanf
@ cdecl -arch=i386 -norelay __libm_sse2_cos() ucrtbase.__libm_sse2_cos
@ cdecl -arch=i386 -norelay __libm_sse2_cosf() ucrtbase.__libm_sse2_cosf
@ cdecl -arch=i386 -norelay __libm_sse2_exp() ucrtbase.__libm_sse2_exp
@ cdecl -arch=i386 -norelay __libm_sse2_expf() ucrtbase.__libm_sse2_expf
@ cdecl -arch=i386 -norelay __libm_sse2_log() ucrtbase.__libm_sse2_log
@ cdecl -arch=i386 -norelay __libm_sse2_log10() ucrtbase.__libm_sse2_log10
@ cdecl -arch=i386 -norelay __libm_sse2_log10f() ucrtbase.__libm_sse2_log10f
@ cdecl -arch=i386 -norelay __libm_sse2_logf() ucrtbase.__libm_sse2_logf
@ cdecl -arch=i386 -norelay __libm_sse2_pow() ucrtbase.__libm_sse2_pow
@ cdecl -arch=i386 -norelay __libm_sse2_powf() ucrtbase.__libm_sse2_powf
@ cdecl -arch=i386 -norelay __libm_sse2_sin() ucrtbase.__libm_sse2_sin
@ cdecl -arch=i386 -norelay __libm_sse2_sinf() ucrtbase.__libm_sse2_sinf
@ cdecl -arch=i386 -norelay __libm_sse2_tan() ucrtbase.__libm_sse2_tan
@ cdecl -arch=i386 -norelay __libm_sse2_tanf() ucrtbase.__libm_sse2_tanf
@ cdecl __setusermatherr(ptr) ucrtbase.__setusermatherr
@ cdecl _cabs(long) ucrtbase._cabs
@ cdecl _chgsign(double) ucrtbase._chgsign
@ cdecl _chgsignf(float) ucrtbase._chgsignf
@ cdecl _copysign(double double) ucrtbase._copysign
@ cdecl _copysignf(float float) ucrtbase._copysignf
@ stub _d_int
@ cdecl _dclass(double) ucrtbase._dclass
@ stub _dexp
@ stub _dlog
@ stub _dnorm
@ cdecl _dpcomp(double double) ucrtbase._dpcomp
@ stub _dpoly
@ stub _dscale
@ cdecl _dsign(double) ucrtbase._dsign
@ stub _dsin
@ cdecl _dtest(ptr) ucrtbase._dtest
@ stub _dunscale
@ cdecl _except1(long long double double long ptr) ucrtbase._except1
@ stub _fd_int
@ cdecl _fdclass(float) ucrtbase._fdclass
@ stub _fdexp
@ stub _fdlog
@ stub _fdnorm
@ cdecl _fdopen(long str) ucrtbase._fdopen
@ cdecl _fdpcomp(float float) ucrtbase._fdpcomp
@ stub _fdpoly
@ stub _fdscale
@ cdecl _fdsign(float) ucrtbase._fdsign
@ stub _fdsin
@ cdecl _fdtest(ptr) ucrtbase._fdtest
@ stub _fdunscale
@ cdecl _finite(double) ucrtbase._finite
@ cdecl -arch=arm,x86_64,arm64 _finitef(float) ucrtbase._finitef
@ cdecl _fpclass(double) ucrtbase._fpclass
@ stub _fpclassf
@ cdecl -arch=i386 -ret64 _ftol() ucrtbase._ftol
@ stub _get_FMA3_enable
@ cdecl _hypot(double double) ucrtbase._hypot
@ cdecl _hypotf(float float) ucrtbase._hypotf
@ cdecl _isnan(double) ucrtbase._isnan
@ cdecl -arch=x86_64 _isnanf(float) ucrtbase._isnanf
@ cdecl _j0(double) ucrtbase._j0
@ cdecl _j1(double) ucrtbase._j1
@ cdecl _jn(long double) ucrtbase._jn
@ stub _ld_int
@ cdecl _ldclass(double) ucrtbase._ldclass
@ stub _ldexp
@ stub _ldlog
@ cdecl _ldpcomp(double double) ucrtbase._ldpcomp
@ stub _ldpoly
@ stub _ldscale
@ cdecl _ldsign(double) ucrtbase._ldsign
@ stub _ldsin
@ cdecl _ldtest(ptr) ucrtbase._ldtest
@ stub _ldunscale
@ cdecl -arch=i386 -norelay _libm_sse2_acos_precise() ucrtbase._libm_sse2_acos_precise
@ cdecl -arch=i386 -norelay _libm_sse2_asin_precise() ucrtbase._libm_sse2_asin_precise
@ cdecl -arch=i386 -norelay _libm_sse2_atan_precise() ucrtbase._libm_sse2_atan_precise
@ cdecl -arch=i386 -norelay _libm_sse2_cos_precise() ucrtbase._libm_sse2_cos_precise
@ cdecl -arch=i386 -norelay _libm_sse2_exp_precise() ucrtbase._libm_sse2_exp_precise
@ cdecl -arch=i386 -norelay _libm_sse2_log10_precise() ucrtbase._libm_sse2_log10_precise
@ cdecl -arch=i386 -norelay _libm_sse2_log_precise() ucrtbase._libm_sse2_log_precise
@ cdecl -arch=i386 -norelay _libm_sse2_pow_precise() ucrtbase._libm_sse2_pow_precise
@ cdecl -arch=i386 -norelay _libm_sse2_sin_precise() ucrtbase._libm_sse2_sin_precise
@ cdecl -arch=i386 -norelay _libm_sse2_sqrt_precise() ucrtbase._libm_sse2_sqrt_precise
@ cdecl -arch=i386 -norelay _libm_sse2_tan_precise() ucrtbase._libm_sse2_tan_precise
@ cdecl _logb(double) ucrtbase._logb
@ cdecl -arch=arm,x86_64,arm64 _logbf(float) ucrtbase._logbf
@ cdecl _nextafter(double double) ucrtbase._nextafter
@ cdecl -arch=x86_64 _nextafterf(float float) ucrtbase._nextafterf
@ cdecl _scalb(double long) ucrtbase._scalb
@ cdecl -arch=x86_64 _scalbf(float long) ucrtbase._scalbf
@ cdecl -arch=win64 _set_FMA3_enable(long) ucrtbase._set_FMA3_enable
@ cdecl -arch=i386 _set_SSE2_enable(long) ucrtbase._set_SSE2_enable
@ cdecl _y0(double) ucrtbase._y0
@ cdecl _y1(double) ucrtbase._y1
@ cdecl _yn(long double) ucrtbase._yn
@ cdecl acos(double) ucrtbase.acos
@ cdecl -arch=arm,x86_64,arm64 acosf(float) ucrtbase.acosf
@ cdecl acosh(double) ucrtbase.acosh
@ cdecl acoshf(float) ucrtbase.acoshf
@ cdecl acoshl(double) ucrtbase.acoshl
@ cdecl asin(double) ucrtbase.asin
@ cdecl -arch=arm,x86_64,arm64 asinf(float) ucrtbase.asinf
@ cdecl asinh(double) ucrtbase.asinh
@ cdecl asinhf(float) ucrtbase.asinhf
@ cdecl asinhl(double) ucrtbase.asinhl
@ cdecl atan(double) ucrtbase.atan
@ cdecl atan2(double double) ucrtbase.atan2
@ cdecl -arch=arm,x86_64,arm64 atan2f(float float) ucrtbase.atan2f
@ cdecl -arch=arm,x86_64,arm64 atanf(float) ucrtbase.atanf
@ cdecl atanh(double) ucrtbase.atanh
@ cdecl atanhf(float) ucrtbase.atanhf
@ cdecl atanhl(double) ucrtbase.atanhl
@ stub cabs
@ stub cabsf
@ stub cabsl
@ stub cacos
@ stub cacosf
@ stub cacosh
@ stub cacoshf
@ stub cacoshl
@ stub cacosl
@ stub carg
@ stub cargf
@ stub cargl
@ stub casin
@ stub casinf
@ stub casinh
@ stub casinhf
@ stub casinhl
@ stub casinl
@ stub catan
@ stub catanf
@ stub catanh
@ stub catanhf
@ stub catanhl
@ stub catanl
@ cdecl cbrt(double) ucrtbase.cbrt
@ cdecl cbrtf(float) ucrtbase.cbrtf
@ cdecl cbrtl(double) ucrtbase.cbrtl
@ stub ccos
@ stub ccosf
@ stub ccosh
@ stub ccoshf
@ stub ccoshl
@ stub ccosl
@ cdecl ceil(double) ucrtbase.ceil
@ cdecl -arch=arm,x86_64,arm64 ceilf(float) ucrtbase.ceilf
@ stub cexp
@ stub cexpf
@ stub cexpl
@ stub cimag
@ stub cimagf
@ stub cimagl
@ stub clog
@ stub clog10
@ stub clog10f
@ stub clog10l
@ stub clogf
@ stub clogl
@ stub conj
@ stub conjf
@ stub conjl
@ cdecl copysign(double double) ucrtbase.copysign
@ cdecl copysignf(float float) ucrtbase.copysignf
@ cdecl copysignl(double double) ucrtbase.copysignl
@ cdecl cos(double) ucrtbase.cos
@ cdecl -arch=arm,x86_64,arm64 cosf(float) ucrtbase.cosf
@ cdecl cosh(double) ucrtbase.cosh
@ cdecl -arch=arm,x86_64,arm64 coshf(float) ucrtbase.coshf
@ stub cpow
@ stub cpowf
@ stub cpowl
@ stub cproj
@ stub cprojf
@ stub cprojl
@ cdecl creal(int128) ucrtbase.creal
@ stub crealf
@ stub creall
@ stub csin
@ stub csinf
@ stub csinh
@ stub csinhf
@ stub csinhl
@ stub csinl
@ stub csqrt
@ stub csqrtf
@ stub csqrtl
@ stub ctan
@ stub ctanf
@ stub ctanh
@ stub ctanhf
@ stub ctanhl
@ stub ctanl
@ cdecl erf(double) ucrtbase.erf
@ cdecl erfc(double) ucrtbase.erfc
@ cdecl erfcf(float) ucrtbase.erfcf
@ cdecl erfcl(double) ucrtbase.erfcl
@ cdecl erff(float) ucrtbase.erff
@ cdecl erfl(double) ucrtbase.erfl
@ cdecl exp(double) ucrtbase.exp
@ cdecl exp2(double) ucrtbase.exp2
@ cdecl exp2f(float) ucrtbase.exp2f
@ cdecl exp2l(double) ucrtbase.exp2l
@ cdecl -arch=arm,x86_64,arm64 expf(float) ucrtbase.expf
@ cdecl expm1(double) ucrtbase.expm1
@ cdecl expm1f(float) ucrtbase.expm1f
@ cdecl expm1l(double) ucrtbase.expm1l
@ cdecl fabs(double) ucrtbase.fabs
@ cdecl -arch=arm,arm64 fabsf(float) ucrtbase.fabsf
@ cdecl fdim(double double) ucrtbase.fdim
@ cdecl fdimf(float float) ucrtbase.fdimf
@ cdecl fdiml(double double) ucrtbase.fdiml
@ cdecl floor(double) ucrtbase.floor
@ cdecl -arch=arm,x86_64,arm64 floorf(float) ucrtbase.floorf
@ cdecl fma(double double double) ucrtbase.fma
@ cdecl fmaf(float float float) ucrtbase.fmaf
@ cdecl fmal(double double double) ucrtbase.fmal
@ cdecl fmax(double double) ucrtbase.fmax
@ cdecl fmaxf(float float) ucrtbase.fmaxf
@ cdecl fmaxl(double double) ucrtbase.fmaxl
@ cdecl fmin(double double) ucrtbase.fmin
@ cdecl fminf(float float) ucrtbase.fminf
@ cdecl fminl(double double) ucrtbase.fminl
@ cdecl fmod(double double) ucrtbase.fmod
@ cdecl -arch=arm,x86_64,arm64 fmodf(float float) ucrtbase.fmodf
@ cdecl frexp(double ptr) ucrtbase.frexp
@ cdecl hypot(double double) ucrtbase.hypot
@ cdecl ilogb(double) ucrtbase.ilogb
@ cdecl ilogbf(float) ucrtbase.ilogbf
@ cdecl ilogbl(double) ucrtbase.ilogbl
@ cdecl ldexp(double long) ucrtbase.ldexp
@ cdecl lgamma(double) ucrtbase.lgamma
@ cdecl lgammaf(float) ucrtbase.lgammaf
@ cdecl lgammal(double) ucrtbase.lgammal
@ cdecl -ret64 llrint(double) ucrtbase.llrint
@ cdecl -ret64 llrintf(float) ucrtbase.llrintf
@ cdecl -ret64 llrintl(double) ucrtbase.llrintl
@ cdecl -ret64 llround(double) ucrtbase.llround
@ cdecl -ret64 llroundf(float) ucrtbase.llroundf
@ cdecl -ret64 llroundl(double) ucrtbase.llroundl
@ cdecl log(double) ucrtbase.log
@ cdecl log10(double) ucrtbase.log10
@ cdecl -arch=arm,x86_64,arm64 log10f(float) ucrtbase.log10f
@ cdecl log1p(double) ucrtbase.log1p
@ cdecl log1pf(float) ucrtbase.log1pf
@ cdecl log1pl(double) ucrtbase.log1pl
@ cdecl log2(double) ucrtbase.log2
@ cdecl log2f(float) ucrtbase.log2f
@ cdecl log2l(double) ucrtbase.log2l
@ cdecl logb(double) ucrtbase.logb
@ cdecl logbf(float) ucrtbase.logbf
@ cdecl logbl(double) ucrtbase.logbl
@ cdecl -arch=arm,x86_64,arm64 logf(float) ucrtbase.logf
@ cdecl lrint(double) ucrtbase.lrint
@ cdecl lrintf(float) ucrtbase.lrintf
@ cdecl lrintl(double) ucrtbase.lrintl
@ cdecl lround(double) ucrtbase.lround
@ cdecl lroundf(float) ucrtbase.lroundf
@ cdecl lroundl(double) ucrtbase.lroundl
@ cdecl modf(double ptr) ucrtbase.modf
@ cdecl -arch=arm,x86_64,arm64 modff(float ptr) ucrtbase.modff
@ cdecl nan(str) ucrtbase.nan
@ cdecl nanf(str) ucrtbase.nanf
@ cdecl nanl(str) ucrtbase.nanl
@ cdecl nearbyint(double) ucrtbase.nearbyint
@ cdecl nearbyintf(float) ucrtbase.nearbyintf
@ cdecl nearbyintl(double) ucrtbase.nearbyintl
@ cdecl nextafter(double double) ucrtbase.nextafter
@ cdecl nextafterf(float float) ucrtbase.nextafterf
@ cdecl nextafterl(double double) ucrtbase.nextafterl
@ cdecl nexttoward(double double) ucrtbase.nexttoward
@ cdecl nexttowardf(float double) ucrtbase.nexttowardf
@ cdecl nexttowardl(double double) ucrtbase.nexttowardl
@ stub norm
@ stub normf
@ stub norml
@ cdecl pow(double double) ucrtbase.pow
@ cdecl -arch=arm,x86_64,arm64 powf(float float) ucrtbase.powf
@ cdecl remainder(double double) ucrtbase.remainder
@ cdecl remainderf(float float) ucrtbase.remainderf
@ cdecl remainderl(double double) ucrtbase.remainderl
@ cdecl remquo(double double ptr) ucrtbase.remquo
@ cdecl remquof(float float ptr) ucrtbase.remquof
@ cdecl remquol(double double ptr) ucrtbase.remquol
@ cdecl rint(double) ucrtbase.rint
@ cdecl rintf(float) ucrtbase.rintf
@ cdecl rintl(double) ucrtbase.rintl
@ cdecl round(double) ucrtbase.round
@ cdecl roundf(float) ucrtbase.roundf
@ cdecl roundl(double) ucrtbase.roundl
@ cdecl scalbln(double long) ucrtbase.scalbln
@ cdecl scalblnf(float long) ucrtbase.scalblnf
@ cdecl scalblnl(double long) ucrtbase.scalblnl
@ cdecl scalbn(double long) ucrtbase.scalbn
@ cdecl scalbnf(float long) ucrtbase.scalbnf
@ cdecl scalbnl(double long) ucrtbase.scalbnl
@ cdecl sin(double) ucrtbase.sin
@ cdecl -arch=arm,x86_64,arm64 sinf(float) ucrtbase.sinf
@ cdecl sinh(double) ucrtbase.sinh
@ cdecl -arch=arm,x86_64,arm64 sinhf(float) ucrtbase.sinhf
@ cdecl sqrt(double) ucrtbase.sqrt
@ cdecl -arch=arm,x86_64,arm64 sqrtf(float) ucrtbase.sqrtf
@ cdecl tan(double) ucrtbase.tan
@ cdecl -arch=arm,x86_64,arm64 tanf(float) ucrtbase.tanf
@ cdecl tanh(double) ucrtbase.tanh
@ cdecl -arch=arm,x86_64,arm64 tanhf(float) ucrtbase.tanhf
@ cdecl tgamma(double) ucrtbase.tgamma
@ cdecl tgammaf(float) ucrtbase.tgammaf
@ cdecl tgammal(double) ucrtbase.tgammal
@ cdecl trunc(double) ucrtbase.trunc
@ cdecl truncf(float) ucrtbase.truncf
@ cdecl truncl(double) ucrtbase.truncl
