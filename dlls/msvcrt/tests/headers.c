/*
 * Copyright 2004 Dimitrie O. Paun
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * This file contains tests to ensure the consistency between symbols
 * defined in the regular msvcrt headers, and the corresponding duplicated
 * symbol defined in msvcrt.h (prefixed by MSVCRT_).
 */

#include "dos.h"
#include "math.h"
#include "stdlib.h"
#include "io.h"
#include "errno.h"
#include "fcntl.h"
#include "malloc.h"
#include "limits.h"
#include "mbctype.h"
#include "stdio.h"
#include "wchar.h"
#include "ctype.h"
#include "crtdbg.h"
#include "share.h"
#include "search.h"
#include "wctype.h"
#include "float.h"
#include "stddef.h"
#include "mbstring.h"
#include "sys/locking.h"
#include "sys/utime.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "sys/timeb.h"
#include "direct.h"
#include "conio.h"
#include "process.h"
#include "string.h"
#include "time.h"
#include "locale.h"
#include "setjmp.h"
#include "wine/test.h"

#ifdef __WINE_USE_MSVCRT
/* Wine-specific msvcrt headers */
#define __WINE_MSVCRT_TEST
#include "eh.h"
#include "msvcrt.h"

#ifdef __GNUC__
#define TYPEOF(type) typeof(type)
#else
#define TYPEOF(type) int
#endif
#define MSVCRT(x)    MSVCRT_##x
#define OFFSET(T,F) ((unsigned int)((char *)&((struct T *)0L)->F - (char *)0L))
#define CHECK_SIZE(e) ok(sizeof(e) == sizeof(MSVCRT(e)), "Element has different sizes\n")
#define CHECK_TYPE(t) { TYPEOF(t) a = 0; TYPEOF(MSVCRT(t)) b = 0; a = b; CHECK_SIZE(t); }
#define CHECK_STRUCT(s) ok(sizeof(struct s) == sizeof(struct MSVCRT(s)), "Struct has different sizes\n")
#define CHECK_FIELD(s,e) ok(OFFSET(s,e) == OFFSET(MSVCRT(s),e), "Bad offset\n")
#define CHECK_DEF(n,d1,d2) ok(d1 == d2, "Defines (MSVCRT_)%s are different: '%d' vs. '%d'\n", n, d1, d2)

/************* Checking types ***************/
static void test_types(void)
{
    CHECK_TYPE(wchar_t);
    CHECK_TYPE(wint_t);
    CHECK_TYPE(wctype_t);
    CHECK_TYPE(_ino_t);
    CHECK_TYPE(_fsize_t);
    CHECK_TYPE(size_t);
    CHECK_TYPE(_dev_t);
    CHECK_TYPE(_off_t);
    CHECK_TYPE(clock_t);
    CHECK_TYPE(time_t);
    CHECK_TYPE(fpos_t);
    CHECK_SIZE(FILE);
    CHECK_TYPE(terminate_handler);
    CHECK_TYPE(terminate_function);
    CHECK_TYPE(unexpected_handler);
    CHECK_TYPE(unexpected_function);
    CHECK_TYPE(_se_translator_function);
    CHECK_TYPE(_beginthread_start_routine_t);
    CHECK_TYPE(_onexit_t);
}

/************* Checking structs ***************/
static void test_structs(void)
{
    CHECK_STRUCT(tm);
    CHECK_FIELD(tm, tm_sec);
    CHECK_FIELD(tm, tm_min);
    CHECK_FIELD(tm, tm_hour);
    CHECK_FIELD(tm, tm_mday);
    CHECK_FIELD(tm, tm_mon);
    CHECK_FIELD(tm, tm_year);
    CHECK_FIELD(tm, tm_wday);
    CHECK_FIELD(tm, tm_yday);
    CHECK_FIELD(tm, tm_isdst);
    CHECK_STRUCT(_timeb);
    CHECK_FIELD(_timeb, time);
    CHECK_FIELD(_timeb, millitm);
    CHECK_FIELD(_timeb, timezone);
    CHECK_FIELD(_timeb, dstflag);
    CHECK_STRUCT(_iobuf);
    CHECK_FIELD(_iobuf, _ptr);
    CHECK_FIELD(_iobuf, _cnt);
    CHECK_FIELD(_iobuf, _base);
    CHECK_FIELD(_iobuf, _flag);
    CHECK_FIELD(_iobuf, _file);
    CHECK_FIELD(_iobuf, _charbuf);
    CHECK_FIELD(_iobuf, _bufsiz);
    CHECK_FIELD(_iobuf, _tmpfname);
    CHECK_STRUCT(lconv);
    CHECK_FIELD(lconv, decimal_point);
    CHECK_FIELD(lconv, thousands_sep);
    CHECK_FIELD(lconv, grouping);
    CHECK_FIELD(lconv, int_curr_symbol);
    CHECK_FIELD(lconv, currency_symbol);
    CHECK_FIELD(lconv, mon_decimal_point);
    CHECK_FIELD(lconv, mon_thousands_sep);
    CHECK_FIELD(lconv, mon_grouping);
    CHECK_FIELD(lconv, positive_sign);
    CHECK_FIELD(lconv, negative_sign);
    CHECK_FIELD(lconv, int_frac_digits);
    CHECK_FIELD(lconv, frac_digits);
    CHECK_FIELD(lconv, p_cs_precedes);
    CHECK_FIELD(lconv, p_sep_by_space);
    CHECK_FIELD(lconv, n_cs_precedes);
    CHECK_FIELD(lconv, n_sep_by_space);
    CHECK_FIELD(lconv, p_sign_posn);
    CHECK_FIELD(lconv, n_sign_posn);
    CHECK_STRUCT(_exception);
    CHECK_FIELD(_exception, type);
    CHECK_FIELD(_exception, name);
    CHECK_FIELD(_exception, arg1);
    CHECK_FIELD(_exception, arg2);
    CHECK_FIELD(_exception, retval);
    CHECK_STRUCT(_complex);
    CHECK_FIELD(_complex, x);
    CHECK_FIELD(_complex, y);
    CHECK_STRUCT(_div_t);
    CHECK_FIELD(_div_t, quot);
    CHECK_FIELD(_div_t, rem);
    CHECK_STRUCT(_ldiv_t);
    CHECK_FIELD(_ldiv_t, quot);
    CHECK_FIELD(_ldiv_t, rem);
    CHECK_STRUCT(_heapinfo);
    CHECK_FIELD(_heapinfo, _pentry);
    CHECK_FIELD(_heapinfo, _size);
    CHECK_FIELD(_heapinfo, _useflag);
#ifdef __i386__
    CHECK_STRUCT(__JUMP_BUFFER);
    CHECK_FIELD(__JUMP_BUFFER, Ebp);
    CHECK_FIELD(__JUMP_BUFFER, Ebx);
    CHECK_FIELD(__JUMP_BUFFER, Edi);
    CHECK_FIELD(__JUMP_BUFFER, Esi);
    CHECK_FIELD(__JUMP_BUFFER, Esp);
    CHECK_FIELD(__JUMP_BUFFER, Eip);
    CHECK_FIELD(__JUMP_BUFFER, Registration);
    CHECK_FIELD(__JUMP_BUFFER, TryLevel);
    CHECK_FIELD(__JUMP_BUFFER, Cookie);
    CHECK_FIELD(__JUMP_BUFFER, UnwindFunc);
    CHECK_FIELD(__JUMP_BUFFER, UnwindData[6]);
#endif
    CHECK_STRUCT(_diskfree_t);
    CHECK_FIELD(_diskfree_t, total_clusters);
    CHECK_FIELD(_diskfree_t, avail_clusters);
    CHECK_FIELD(_diskfree_t, sectors_per_cluster);
    CHECK_FIELD(_diskfree_t, bytes_per_sector);
    CHECK_STRUCT(_finddata_t);
    CHECK_FIELD(_finddata_t, attrib);
    CHECK_FIELD(_finddata_t, time_create);
    CHECK_FIELD(_finddata_t, time_access);
    CHECK_FIELD(_finddata_t, time_write);
    CHECK_FIELD(_finddata_t, size);
    CHECK_FIELD(_finddata_t, name[260]);
    CHECK_STRUCT(_finddatai64_t);
    CHECK_FIELD(_finddatai64_t, attrib);
    CHECK_FIELD(_finddatai64_t, time_create);
    CHECK_FIELD(_finddatai64_t, time_access);
    CHECK_FIELD(_finddatai64_t, time_write);
    CHECK_FIELD(_finddatai64_t, size);
    CHECK_FIELD(_finddatai64_t, name[260]);
    CHECK_STRUCT(_wfinddata_t);
    CHECK_FIELD(_wfinddata_t, attrib);
    CHECK_FIELD(_wfinddata_t, time_create);
    CHECK_FIELD(_wfinddata_t, time_access);
    CHECK_FIELD(_wfinddata_t, time_write);
    CHECK_FIELD(_wfinddata_t, size);
    CHECK_FIELD(_wfinddata_t, name[260]);
    CHECK_STRUCT(_wfinddatai64_t);
    CHECK_FIELD(_wfinddatai64_t, attrib);
    CHECK_FIELD(_wfinddatai64_t, time_create);
    CHECK_FIELD(_wfinddatai64_t, time_access);
    CHECK_FIELD(_wfinddatai64_t, time_write);
    CHECK_FIELD(_wfinddatai64_t, size);
    CHECK_FIELD(_wfinddatai64_t, name[260]);
    CHECK_STRUCT(_utimbuf);
    CHECK_FIELD(_utimbuf, actime);
    CHECK_FIELD(_utimbuf, modtime);
    CHECK_STRUCT(_stat);
    CHECK_FIELD(_stat, st_dev);
    CHECK_FIELD(_stat, st_ino);
    CHECK_FIELD(_stat, st_mode);
    CHECK_FIELD(_stat, st_nlink);
    CHECK_FIELD(_stat, st_uid);
    CHECK_FIELD(_stat, st_gid);
    CHECK_FIELD(_stat, st_rdev);
    CHECK_FIELD(_stat, st_size);
    CHECK_FIELD(_stat, st_atime);
    CHECK_FIELD(_stat, st_mtime);
    CHECK_FIELD(_stat, st_ctime);
    CHECK_FIELD(_stat, st_dev);
    CHECK_FIELD(_stat, st_ino);
    CHECK_FIELD(_stat, st_mode);
    CHECK_FIELD(_stat, st_nlink);
    CHECK_FIELD(_stat, st_uid);
    CHECK_FIELD(_stat, st_gid);
    CHECK_FIELD(_stat, st_rdev);
    CHECK_FIELD(_stat, st_size);
    CHECK_FIELD(_stat, st_atime);
    CHECK_FIELD(_stat, st_mtime);
    CHECK_FIELD(_stat, st_ctime);
    CHECK_FIELD(_stat, st_dev);
    CHECK_FIELD(_stat, st_ino);
    CHECK_FIELD(_stat, st_mode);
    CHECK_FIELD(_stat, st_nlink);
    CHECK_FIELD(_stat, st_uid);
    CHECK_FIELD(_stat, st_gid);
    CHECK_FIELD(_stat, st_rdev);
    CHECK_FIELD(_stat, st_size);
    CHECK_FIELD(_stat, st_atime);
    CHECK_FIELD(_stat, st_mtime);
    CHECK_FIELD(_stat, st_ctime);
    CHECK_STRUCT(stat);
    CHECK_FIELD(stat, st_dev);
    CHECK_FIELD(stat, st_ino);
    CHECK_FIELD(stat, st_mode);
    CHECK_FIELD(stat, st_nlink);
    CHECK_FIELD(stat, st_uid);
    CHECK_FIELD(stat, st_gid);
    CHECK_FIELD(stat, st_rdev);
    CHECK_FIELD(stat, st_size);
    CHECK_FIELD(stat, st_atime);
    CHECK_FIELD(stat, st_mtime);
    CHECK_FIELD(stat, st_ctime);
    CHECK_FIELD(stat, st_dev);
    CHECK_FIELD(stat, st_ino);
    CHECK_FIELD(stat, st_mode);
    CHECK_FIELD(stat, st_nlink);
    CHECK_FIELD(stat, st_uid);
    CHECK_FIELD(stat, st_gid);
    CHECK_FIELD(stat, st_rdev);
    CHECK_FIELD(stat, st_size);
    CHECK_FIELD(stat, st_atime);
    CHECK_FIELD(stat, st_mtime);
    CHECK_FIELD(stat, st_ctime);
    CHECK_FIELD(stat, st_dev);
    CHECK_FIELD(stat, st_ino);
    CHECK_FIELD(stat, st_mode);
    CHECK_FIELD(stat, st_nlink);
    CHECK_FIELD(stat, st_uid);
    CHECK_FIELD(stat, st_gid);
    CHECK_FIELD(stat, st_rdev);
    CHECK_FIELD(stat, st_size);
    CHECK_FIELD(stat, st_atime);
    CHECK_FIELD(stat, st_mtime);
    CHECK_FIELD(stat, st_ctime);
    CHECK_STRUCT(_stati64);
    CHECK_FIELD(_stati64, st_dev);
    CHECK_FIELD(_stati64, st_ino);
    CHECK_FIELD(_stati64, st_mode);
    CHECK_FIELD(_stati64, st_nlink);
    CHECK_FIELD(_stati64, st_uid);
    CHECK_FIELD(_stati64, st_gid);
    CHECK_FIELD(_stati64, st_rdev);
    CHECK_FIELD(_stati64, st_size);
    CHECK_FIELD(_stati64, st_atime);
    CHECK_FIELD(_stati64, st_mtime);
    CHECK_FIELD(_stati64, st_ctime);
}

/************* Checking defines ***************/
void test_defines(void)
{
    CHECK_DEF("WEOF", WEOF, MSVCRT_WEOF);
    CHECK_DEF("EOF", EOF, MSVCRT_EOF);
    CHECK_DEF("TMP_MAX", TMP_MAX, MSVCRT_TMP_MAX);
    CHECK_DEF("BUFSIZ", BUFSIZ, MSVCRT_BUFSIZ);
    CHECK_DEF("STDIN_FILENO", STDIN_FILENO, MSVCRT_STDIN_FILENO);
    CHECK_DEF("STDOUT_FILENO", STDOUT_FILENO, MSVCRT_STDOUT_FILENO);
    CHECK_DEF("STDERR_FILENO", STDERR_FILENO, MSVCRT_STDERR_FILENO);
    CHECK_DEF("_IOFBF", _IOFBF, MSVCRT__IOFBF);
    CHECK_DEF("_IONBF", _IONBF, MSVCRT__IONBF);
    CHECK_DEF("_IOLBF", _IOLBF, MSVCRT__IOLBF);
    CHECK_DEF("FILENAME_MAX", FILENAME_MAX, MSVCRT_FILENAME_MAX);
    CHECK_DEF("_P_WAIT", _P_WAIT, MSVCRT__P_WAIT);
    CHECK_DEF("_P_NOWAIT", _P_NOWAIT, MSVCRT__P_NOWAIT);
    CHECK_DEF("_P_OVERLAY", _P_OVERLAY, MSVCRT__P_OVERLAY);
    CHECK_DEF("_P_NOWAITO", _P_NOWAITO, MSVCRT__P_NOWAITO);
    CHECK_DEF("_P_DETACH", _P_DETACH, MSVCRT__P_DETACH);
    CHECK_DEF("EPERM", EPERM, MSVCRT_EPERM);
    CHECK_DEF("ENOENT", ENOENT, MSVCRT_ENOENT);
    CHECK_DEF("ESRCH", ESRCH, MSVCRT_ESRCH);
    CHECK_DEF("EINTR", EINTR, MSVCRT_EINTR);
    CHECK_DEF("EIO", EIO, MSVCRT_EIO);
    CHECK_DEF("ENXIO", ENXIO, MSVCRT_ENXIO);
    CHECK_DEF("E2BIG", E2BIG, MSVCRT_E2BIG);
    CHECK_DEF("ENOEXEC", ENOEXEC, MSVCRT_ENOEXEC);
    CHECK_DEF("EBADF", EBADF, MSVCRT_EBADF);
    CHECK_DEF("ECHILD", ECHILD, MSVCRT_ECHILD);
    CHECK_DEF("EAGAIN", EAGAIN, MSVCRT_EAGAIN);
    CHECK_DEF("ENOMEM", ENOMEM, MSVCRT_ENOMEM);
    CHECK_DEF("EACCES", EACCES, MSVCRT_EACCES);
    CHECK_DEF("EFAULT", EFAULT, MSVCRT_EFAULT);
    CHECK_DEF("EBUSY", EBUSY, MSVCRT_EBUSY);
    CHECK_DEF("EEXIST", EEXIST, MSVCRT_EEXIST);
    CHECK_DEF("EXDEV", EXDEV, MSVCRT_EXDEV);
    CHECK_DEF("ENODEV", ENODEV, MSVCRT_ENODEV);
    CHECK_DEF("ENOTDIR", ENOTDIR, MSVCRT_ENOTDIR);
    CHECK_DEF("EISDIR", EISDIR, MSVCRT_EISDIR);
    CHECK_DEF("EINVAL", EINVAL, MSVCRT_EINVAL);
    CHECK_DEF("ENFILE", ENFILE, MSVCRT_ENFILE);
    CHECK_DEF("EMFILE", EMFILE, MSVCRT_EMFILE);
    CHECK_DEF("ENOTTY", ENOTTY, MSVCRT_ENOTTY);
    CHECK_DEF("EFBIG", EFBIG, MSVCRT_EFBIG);
    CHECK_DEF("ENOSPC", ENOSPC, MSVCRT_ENOSPC);
    CHECK_DEF("ESPIPE", ESPIPE, MSVCRT_ESPIPE);
    CHECK_DEF("EROFS", EROFS, MSVCRT_EROFS);
    CHECK_DEF("EMLINK", EMLINK, MSVCRT_EMLINK);
    CHECK_DEF("EPIPE", EPIPE, MSVCRT_EPIPE);
    CHECK_DEF("EDOM", EDOM, MSVCRT_EDOM);
    CHECK_DEF("ERANGE", ERANGE, MSVCRT_ERANGE);
    CHECK_DEF("EDEADLK", EDEADLK, MSVCRT_EDEADLK);
    CHECK_DEF("EDEADLOCK", EDEADLOCK, MSVCRT_EDEADLOCK);
    CHECK_DEF("ENAMETOOLONG", ENAMETOOLONG, MSVCRT_ENAMETOOLONG);
    CHECK_DEF("ENOLCK", ENOLCK, MSVCRT_ENOLCK);
    CHECK_DEF("ENOSYS", ENOSYS, MSVCRT_ENOSYS);
    CHECK_DEF("ENOTEMPTY", ENOTEMPTY, MSVCRT_ENOTEMPTY);
    CHECK_DEF("LC_ALL", LC_ALL, MSVCRT_LC_ALL);
    CHECK_DEF("LC_COLLATE", LC_COLLATE, MSVCRT_LC_COLLATE);
    CHECK_DEF("LC_CTYPE", LC_CTYPE, MSVCRT_LC_CTYPE);
    CHECK_DEF("LC_MONETARY", LC_MONETARY, MSVCRT_LC_MONETARY);
    CHECK_DEF("LC_NUMERIC", LC_NUMERIC, MSVCRT_LC_NUMERIC);
    CHECK_DEF("LC_TIME", LC_TIME, MSVCRT_LC_TIME);
    CHECK_DEF("LC_MIN", LC_MIN, MSVCRT_LC_MIN);
    CHECK_DEF("LC_MAX", LC_MAX, MSVCRT_LC_MAX);
    CHECK_DEF("CLOCKS_PER_SEC", CLOCKS_PER_SEC, MSVCRT_CLOCKS_PER_SEC);
    CHECK_DEF("_HEAPEMPTY", _HEAPEMPTY, MSVCRT__HEAPEMPTY);
    CHECK_DEF("_HEAPOK", _HEAPOK, MSVCRT__HEAPOK);
    CHECK_DEF("_HEAPBADBEGIN", _HEAPBADBEGIN, MSVCRT__HEAPBADBEGIN);
    CHECK_DEF("_HEAPBADNODE", _HEAPBADNODE, MSVCRT__HEAPBADNODE);
    CHECK_DEF("_HEAPEND", _HEAPEND, MSVCRT__HEAPEND);
    CHECK_DEF("_HEAPBADPTR", _HEAPBADPTR, MSVCRT__HEAPBADPTR);
    CHECK_DEF("_FREEENTRY", _FREEENTRY, MSVCRT__FREEENTRY);
    CHECK_DEF("_USEDENTRY", _USEDENTRY, MSVCRT__USEDENTRY);
    CHECK_DEF("_OUT_TO_DEFAULT", _OUT_TO_DEFAULT, MSVCRT__OUT_TO_DEFAULT);
    CHECK_DEF("_REPORT_ERRMODE", _REPORT_ERRMODE, MSVCRT__REPORT_ERRMODE);
    CHECK_DEF("_UPPER", _UPPER, MSVCRT__UPPER);
    CHECK_DEF("_LOWER", _LOWER, MSVCRT__LOWER);
    CHECK_DEF("_DIGIT", _DIGIT, MSVCRT__DIGIT);
    CHECK_DEF("_SPACE", _SPACE, MSVCRT__SPACE);
    CHECK_DEF("_PUNCT", _PUNCT, MSVCRT__PUNCT);
    CHECK_DEF("_CONTROL", _CONTROL, MSVCRT__CONTROL);
    CHECK_DEF("_BLANK", _BLANK, MSVCRT__BLANK);
    CHECK_DEF("_HEX", _HEX, MSVCRT__HEX);
    CHECK_DEF("_LEADBYTE", _LEADBYTE, MSVCRT__LEADBYTE);
    CHECK_DEF("_ALPHA", _ALPHA, MSVCRT__ALPHA);
    CHECK_DEF("_IOREAD", _IOREAD, MSVCRT__IOREAD);
    CHECK_DEF("_IOWRT", _IOWRT, MSVCRT__IOWRT);
    CHECK_DEF("_IOMYBUF", _IOMYBUF, MSVCRT__IOMYBUF);
    CHECK_DEF("_IOEOF", _IOEOF, MSVCRT__IOEOF);
    CHECK_DEF("_IOERR", _IOERR, MSVCRT__IOERR);
    CHECK_DEF("_IOSTRG", _IOSTRG, MSVCRT__IOSTRG);
    CHECK_DEF("_IORW", _IORW, MSVCRT__IORW);
    CHECK_DEF("_S_IEXEC", _S_IEXEC, MSVCRT__S_IEXEC);
    CHECK_DEF("_S_IWRITE", _S_IWRITE, MSVCRT__S_IWRITE);
    CHECK_DEF("_S_IREAD", _S_IREAD, MSVCRT__S_IREAD);
    CHECK_DEF("_S_IFIFO", _S_IFIFO, MSVCRT__S_IFIFO);
    CHECK_DEF("_S_IFCHR", _S_IFCHR, MSVCRT__S_IFCHR);
    CHECK_DEF("_S_IFDIR", _S_IFDIR, MSVCRT__S_IFDIR);
    CHECK_DEF("_S_IFREG", _S_IFREG, MSVCRT__S_IFREG);
    CHECK_DEF("_S_IFMT", _S_IFMT, MSVCRT__S_IFMT);
    CHECK_DEF("_LK_UNLCK", _LK_UNLCK, MSVCRT__LK_UNLCK);
    CHECK_DEF("_LK_LOCK", _LK_LOCK, MSVCRT__LK_LOCK);
    CHECK_DEF("_LK_NBLCK", _LK_NBLCK, MSVCRT__LK_NBLCK);
    CHECK_DEF("_LK_RLCK", _LK_RLCK, MSVCRT__LK_RLCK);
    CHECK_DEF("_LK_NBRLCK", _LK_NBRLCK, MSVCRT__LK_NBRLCK);
    CHECK_DEF("_O_RDONLY", _O_RDONLY, MSVCRT__O_RDONLY);
    CHECK_DEF("_O_WRONLY", _O_WRONLY, MSVCRT__O_WRONLY);
    CHECK_DEF("_O_RDWR", _O_RDWR, MSVCRT__O_RDWR);
    CHECK_DEF("_O_ACCMODE", _O_ACCMODE, MSVCRT__O_ACCMODE);
    CHECK_DEF("_O_APPEND", _O_APPEND, MSVCRT__O_APPEND);
    CHECK_DEF("_O_RANDOM", _O_RANDOM, MSVCRT__O_RANDOM);
    CHECK_DEF("_O_SEQUENTIAL", _O_SEQUENTIAL, MSVCRT__O_SEQUENTIAL);
    CHECK_DEF("_O_TEMPORARY", _O_TEMPORARY, MSVCRT__O_TEMPORARY);
    CHECK_DEF("_O_NOINHERIT", _O_NOINHERIT, MSVCRT__O_NOINHERIT);
    CHECK_DEF("_O_CREAT", _O_CREAT, MSVCRT__O_CREAT);
    CHECK_DEF("_O_TRUNC", _O_TRUNC, MSVCRT__O_TRUNC);
    CHECK_DEF("_O_EXCL", _O_EXCL, MSVCRT__O_EXCL);
    CHECK_DEF("_O_SHORT_LIVED", _O_SHORT_LIVED, MSVCRT__O_SHORT_LIVED);
    CHECK_DEF("_O_TEXT", _O_TEXT, MSVCRT__O_TEXT);
    CHECK_DEF("_O_BINARY", _O_BINARY, MSVCRT__O_BINARY);
    CHECK_DEF("_O_RAW", _O_RAW, MSVCRT__O_RAW);
    CHECK_DEF("_SW_INEXACT", _SW_INEXACT, MSVCRT__SW_INEXACT);
    CHECK_DEF("_SW_UNDERFLOW", _SW_UNDERFLOW, MSVCRT__SW_UNDERFLOW);
    CHECK_DEF("_SW_OVERFLOW", _SW_OVERFLOW, MSVCRT__SW_OVERFLOW);
    CHECK_DEF("_SW_ZERODIVIDE", _SW_ZERODIVIDE, MSVCRT__SW_ZERODIVIDE);
    CHECK_DEF("_SW_INVALID", _SW_INVALID, MSVCRT__SW_INVALID);
    CHECK_DEF("_SW_UNEMULATED", _SW_UNEMULATED, MSVCRT__SW_UNEMULATED);
    CHECK_DEF("_SW_SQRTNEG", _SW_SQRTNEG, MSVCRT__SW_SQRTNEG);
    CHECK_DEF("_SW_STACKOVERFLOW", _SW_STACKOVERFLOW, MSVCRT__SW_STACKOVERFLOW);
    CHECK_DEF("_SW_STACKUNDERFLOW", _SW_STACKUNDERFLOW, MSVCRT__SW_STACKUNDERFLOW);
    CHECK_DEF("_SW_DENORMAL", _SW_DENORMAL, MSVCRT__SW_DENORMAL);
    CHECK_DEF("_FPCLASS_SNAN", _FPCLASS_SNAN, MSVCRT__FPCLASS_SNAN);
    CHECK_DEF("_FPCLASS_QNAN", _FPCLASS_QNAN, MSVCRT__FPCLASS_QNAN);
    CHECK_DEF("_FPCLASS_NINF", _FPCLASS_NINF, MSVCRT__FPCLASS_NINF);
    CHECK_DEF("_FPCLASS_NN", _FPCLASS_NN, MSVCRT__FPCLASS_NN);
    CHECK_DEF("_FPCLASS_ND", _FPCLASS_ND, MSVCRT__FPCLASS_ND);
    CHECK_DEF("_FPCLASS_NZ", _FPCLASS_NZ, MSVCRT__FPCLASS_NZ);
    CHECK_DEF("_FPCLASS_PZ", _FPCLASS_PZ, MSVCRT__FPCLASS_PZ);
    CHECK_DEF("_FPCLASS_PD", _FPCLASS_PD, MSVCRT__FPCLASS_PD);
    CHECK_DEF("_FPCLASS_PN", _FPCLASS_PN, MSVCRT__FPCLASS_PN);
    CHECK_DEF("_FPCLASS_PINF", _FPCLASS_PINF, MSVCRT__FPCLASS_PINF);
#ifdef __i386__
    CHECK_DEF("_EM_INVALID", _EM_INVALID, MSVCRT__EM_INVALID);
    CHECK_DEF("_EM_DENORMAL", _EM_DENORMAL, MSVCRT__EM_DENORMAL);
    CHECK_DEF("_EM_ZERODIVIDE", _EM_ZERODIVIDE, MSVCRT__EM_ZERODIVIDE);
    CHECK_DEF("_EM_OVERFLOW", _EM_OVERFLOW, MSVCRT__EM_OVERFLOW);
    CHECK_DEF("_EM_UNDERFLOW", _EM_UNDERFLOW, MSVCRT__EM_UNDERFLOW);
    CHECK_DEF("_EM_INEXACT", _EM_INEXACT, MSVCRT__EM_INEXACT);
    CHECK_DEF("_IC_AFFINE", _IC_AFFINE, MSVCRT__IC_AFFINE);
    CHECK_DEF("_IC_PROJECTIVE", _IC_PROJECTIVE, MSVCRT__IC_PROJECTIVE);
    CHECK_DEF("_RC_CHOP", _RC_CHOP, MSVCRT__RC_CHOP);
    CHECK_DEF("_RC_UP", _RC_UP, MSVCRT__RC_UP);
    CHECK_DEF("_RC_DOWN", _RC_DOWN, MSVCRT__RC_DOWN);
    CHECK_DEF("_RC_NEAR", _RC_NEAR, MSVCRT__RC_NEAR);
    CHECK_DEF("_PC_24", _PC_24, MSVCRT__PC_24);
    CHECK_DEF("_PC_53", _PC_53, MSVCRT__PC_53);
    CHECK_DEF("_PC_64", _PC_64, MSVCRT__PC_64);
#endif
}

#endif /* __WINE_USE_MSVCRT */

START_TEST(headers)
{
#ifdef __WINE_USE_MSVCRT
    test_types();
    test_structs();
    test_defines();
#endif
}
