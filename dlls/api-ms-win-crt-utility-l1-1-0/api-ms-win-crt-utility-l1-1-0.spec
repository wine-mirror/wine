@ cdecl -ret64 _abs64(int64) ucrtbase._abs64
@ cdecl _byteswap_uint64(int64) ucrtbase._byteswap_uint64
@ cdecl _byteswap_ulong(long) ucrtbase._byteswap_ulong
@ cdecl _byteswap_ushort(long) ucrtbase._byteswap_ushort
@ cdecl _lfind(ptr ptr ptr long ptr) ucrtbase._lfind
@ cdecl _lfind_s(ptr ptr ptr long ptr ptr) ucrtbase._lfind_s
@ cdecl _lrotl(long long) ucrtbase._lrotl
@ cdecl _lrotr(long long) ucrtbase._lrotr
@ cdecl _lsearch(ptr ptr ptr long ptr) ucrtbase._lsearch
@ stub _lsearch_s
@ cdecl _rotl(long long) ucrtbase._rotl
@ cdecl -ret64 _rotl64(int64 long) ucrtbase._rotl64
@ cdecl _rotr(long long) ucrtbase._rotr
@ cdecl -ret64 _rotr64(int64 long) ucrtbase._rotr64
@ cdecl _swab(str str long) ucrtbase._swab
@ cdecl abs(long) ucrtbase.abs
@ cdecl bsearch(ptr ptr long long ptr) ucrtbase.bsearch
@ cdecl bsearch_s(ptr ptr long long ptr ptr) ucrtbase.bsearch_s
@ cdecl -ret64 div(long long) ucrtbase.div
@ cdecl -ret64 imaxabs(int64) ucrtbase.imaxabs
@ stub imaxdiv
@ cdecl labs(long) ucrtbase.labs
@ cdecl -ret64 ldiv(long long) ucrtbase.ldiv
@ cdecl -ret64 llabs(int64) ucrtbase.llabs
@ cdecl lldiv(int64 int64) ucrtbase.lldiv
@ cdecl qsort(ptr long long ptr) ucrtbase.qsort
@ cdecl qsort_s(ptr long long ptr ptr) ucrtbase.qsort_s
@ cdecl rand() ucrtbase.rand
@ cdecl rand_s(ptr) ucrtbase.rand_s
@ cdecl srand(long) ucrtbase.srand
