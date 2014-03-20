/*
 * Copyright 2011 Piotr Caban for CodeWeavers
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

#ifdef PRINTF_WIDE
#define APICHAR MSVCRT_wchar_t
#define CONVCHAR char
#define FUNC_NAME(func) func ## _w
#else
#define APICHAR char
#define CONVCHAR MSVCRT_wchar_t
#define FUNC_NAME(func) func ## _a
#endif

typedef struct FUNC_NAME(pf_flags_t)
{
    APICHAR Sign, LeftAlign, Alternate, PadZero;
    int FieldLength, Precision;
    APICHAR IntegerLength, IntegerDouble;
    APICHAR WideString;
    APICHAR Format;
} FUNC_NAME(pf_flags);

struct FUNC_NAME(_str_ctx) {
    MSVCRT_size_t len;
    APICHAR *buf;
};

static int FUNC_NAME(puts_clbk_str)(void *ctx, int len, const APICHAR *str)
{
    struct FUNC_NAME(_str_ctx) *out = ctx;

    if(!out->buf)
        return len;

    if(out->len < len) {
        memcpy(out->buf, str, out->len*sizeof(APICHAR));
        out->buf += out->len;
        out->len = 0;
        return -1;
    }

    memcpy(out->buf, str, len*sizeof(APICHAR));
    out->buf += len;
    out->len -= len;
    return len;
}

static inline const APICHAR* FUNC_NAME(pf_parse_int)(const APICHAR *fmt, int *val)
{
    *val = 0;

    while(isdigit(*fmt)) {
        *val *= 10;
        *val += *fmt++ - '0';
    }

    return fmt;
}

/* pf_fill: takes care of signs, alignment, zero and field padding */
static inline int FUNC_NAME(pf_fill)(FUNC_NAME(puts_clbk) pf_puts, void *puts_ctx,
        int len, FUNC_NAME(pf_flags) *flags, BOOL left)
{
    int i, r = 0, written;

    if(flags->Sign && !strchr("diaeEfgG", flags->Format))
        flags->Sign = 0;

    if(left && flags->Sign) {
        flags->FieldLength--;
        if(flags->PadZero)
            r = pf_puts(puts_ctx, 1, &flags->Sign);
    }
    written = r;

    if((!left && flags->LeftAlign) || (left && !flags->LeftAlign)) {
        APICHAR ch;

        if(left && flags->PadZero)
            ch = '0';
        else
            ch = ' ';

        for(i=0; i<flags->FieldLength-len && r>=0; i++) {
            r = pf_puts(puts_ctx, 1, &ch);
            written += r;
        }
    }


    if(r>=0 && left && flags->Sign && !flags->PadZero) {
        r = pf_puts(puts_ctx, 1, &flags->Sign);
        written += r;
    }

    return r>=0 ? written : r;
}

static inline int FUNC_NAME(pf_output_wstr)(FUNC_NAME(puts_clbk) pf_puts, void *puts_ctx,
        const MSVCRT_wchar_t *str, int len, MSVCRT_pthreadlocinfo locinfo)
{
#ifdef PRINTF_WIDE
    return pf_puts(puts_ctx, len, str);
#else
    LPSTR out;
    int len_a = WideCharToMultiByte(locinfo->lc_codepage, 0, str, len, NULL, 0, NULL, NULL);

    out = HeapAlloc(GetProcessHeap(), 0, len_a);
    if(!out)
        return -1;

    WideCharToMultiByte(locinfo->lc_codepage, 0, str, len, out, len_a, NULL, NULL);
    len = pf_puts(puts_ctx, len_a, out);
    HeapFree(GetProcessHeap(), 0, out);
    return len;
#endif
}

static inline int FUNC_NAME(pf_output_str)(FUNC_NAME(puts_clbk) pf_puts, void *puts_ctx,
        const char *str, int len, MSVCRT_pthreadlocinfo locinfo)
{
#ifdef PRINTF_WIDE
    LPWSTR out;
    int len_w = MultiByteToWideChar(locinfo->lc_codepage, 0, str, len, NULL, 0);

    out = HeapAlloc(GetProcessHeap(), 0, len_w*sizeof(WCHAR));
    if(!out)
        return -1;

    MultiByteToWideChar(locinfo->lc_codepage, 0, str, len, out, len_w);
    len = pf_puts(puts_ctx, len_w, out);
    HeapFree(GetProcessHeap(), 0, out);
    return len;
#else
    return pf_puts(puts_ctx, len, str);
#endif
}

static inline int FUNC_NAME(pf_output_format_wstr)(FUNC_NAME(puts_clbk) pf_puts, void *puts_ctx,
        const MSVCRT_wchar_t *str, int len, FUNC_NAME(pf_flags) *flags, MSVCRT_pthreadlocinfo locinfo)
{
    int r, ret;

    if(len < 0)
        len = strlenW(str);

    if(flags->Precision>=0 && flags->Precision<len)
        len = flags->Precision;

    r = FUNC_NAME(pf_fill)(pf_puts, puts_ctx, len, flags, TRUE);
    ret = r;
    if(r >= 0) {
        r = FUNC_NAME(pf_output_wstr)(pf_puts, puts_ctx, str, len, locinfo);
        ret += r;
    }
    if(r >= 0) {
        r = FUNC_NAME(pf_fill)(pf_puts, puts_ctx, len, flags, FALSE);
        ret += r;
    }

    return r>=0 ? ret : r;
}

static inline int FUNC_NAME(pf_output_format_str)(FUNC_NAME(puts_clbk) pf_puts, void *puts_ctx,
        const char *str, int len, FUNC_NAME(pf_flags) *flags, MSVCRT_pthreadlocinfo locinfo)
{
    int r, ret;

    if(len < 0)
        len = strlen(str);

    if(flags->Precision>=0 && flags->Precision<len)
        len = flags->Precision;

    r = FUNC_NAME(pf_fill)(pf_puts, puts_ctx, len, flags, TRUE);
    ret = r;
    if(r >= 0) {
        r = FUNC_NAME(pf_output_str)(pf_puts, puts_ctx, str, len, locinfo);
        ret += r;
    }
    if(r >= 0) {
        r = FUNC_NAME(pf_fill)(pf_puts, puts_ctx, len, flags, FALSE);
        ret += r;
    }

    return r>=0 ? ret : r;
}

static inline int FUNC_NAME(pf_handle_string)(FUNC_NAME(puts_clbk) pf_puts, void *puts_ctx,
        const void *str, int len, FUNC_NAME(pf_flags) *flags, MSVCRT_pthreadlocinfo locinfo)
{
#ifdef PRINTF_WIDE
    static const MSVCRT_wchar_t nullW[] = {'(','n','u','l','l',')',0};

    if(!str)
        return FUNC_NAME(pf_output_format_wstr)(pf_puts, puts_ctx, nullW, 6, flags, locinfo);
#else
    if(!str)
        return FUNC_NAME(pf_output_format_str)(pf_puts, puts_ctx, "(null)", 6, flags, locinfo);
#endif

    if(flags->WideString || flags->IntegerLength=='l')
        return FUNC_NAME(pf_output_format_wstr)(pf_puts, puts_ctx, str, len, flags, locinfo);
    if(flags->IntegerLength == 'h')
        return FUNC_NAME(pf_output_format_str)(pf_puts, puts_ctx, str, len, flags, locinfo);

    if((flags->Format=='S' || flags->Format=='C') == (sizeof(APICHAR)==sizeof(MSVCRT_wchar_t)))
        return FUNC_NAME(pf_output_format_str)(pf_puts, puts_ctx, str, len, flags, locinfo);
    else
        return FUNC_NAME(pf_output_format_wstr)(pf_puts, puts_ctx, str, len, flags, locinfo);
}

static inline void FUNC_NAME(pf_rebuild_format_string)(char *p, FUNC_NAME(pf_flags) *flags)
{
    *p++ = '%';
    if(flags->Alternate)
        *p++ = flags->Alternate;
    if(flags->Precision >= 0) {
        sprintf(p, ".%d", flags->Precision);
        p += strlen(p);
    }
    *p++ = flags->Format;
    *p++ = 0;
}

/* pf_integer_conv:  prints x to buf, including alternate formats and
   additional precision digits, but not field characters or the sign */
static inline void FUNC_NAME(pf_integer_conv)(APICHAR *buf, int buf_len,
        FUNC_NAME(pf_flags) *flags, LONGLONG x)
{
    unsigned int base;
    const char *digits;
    int i, j, k;

    if(flags->Format == 'o')
        base = 8;
    else if(flags->Format=='x' || flags->Format=='X')
        base = 16;
    else
        base = 10;

    if(flags->Format == 'X')
        digits = "0123456789ABCDEFX";
    else
        digits = "0123456789abcdefx";

    if(x<0 && (flags->Format=='d' || flags->Format=='i')) {
        x = -x;
        flags->Sign = '-';
    }

    i = 0;
    if(x == 0) {
        flags->Alternate = 0;
        if(flags->Precision)
            buf[i++] = '0';
    } else {
        while(x != 0) {
            j = (ULONGLONG)x%base;
            x = (ULONGLONG)x/base;
            buf[i++] = digits[j];
        }
    }
    k = flags->Precision-i;
    while(k-- > 0)
        buf[i++] = '0';
    if(flags->Alternate) {
        if(base == 16) {
            buf[i++] = digits[16];
            buf[i++] = '0';
        } else if(base==8 && buf[i-1]!='0')
            buf[i++] = '0';
    }

    /* Adjust precision so pf_fill won't truncate the number later */
    flags->Precision = i;

    buf[i] = '\0';
    j = 0;
    while(--i > j) {
        APICHAR tmp = buf[j];
        buf[j] = buf[i];
        buf[i] = tmp;
        j++;
    }
}

static inline void FUNC_NAME(pf_fixup_exponent)(char *buf)
{
    char* tmp = buf;

    while(tmp[0] && toupper(tmp[0])!='E')
        tmp++;

    if(tmp[0] && (tmp[1]=='+' || tmp[1]=='-') &&
            isdigit(tmp[2]) && isdigit(tmp[3])) {
        BOOL two_digit_exp = (_get_output_format() == MSVCRT__TWO_DIGIT_EXPONENT);

        tmp += 2;
        if(isdigit(tmp[2])) {
            if(two_digit_exp && tmp[0]=='0') {
                tmp[0] = tmp[1];
                tmp[1] = tmp[2];
                tmp[2] = tmp[3];
            }

            return; /* Exponent already 3 digits */
        }else if(two_digit_exp) {
            return;
        }

        tmp[3] = tmp[2];
        tmp[2] = tmp[1];
        tmp[1] = tmp[0];
        tmp[0] = '0';
    }
}

int FUNC_NAME(pf_printf)(FUNC_NAME(puts_clbk) pf_puts, void *puts_ctx, const APICHAR *fmt,
        MSVCRT__locale_t locale, BOOL positional_params, BOOL invoke_invalid_param_handler,
        args_clbk pf_args, void *args_ctx, __ms_va_list *valist)
{
    MSVCRT_pthreadlocinfo locinfo;
    const APICHAR *q, *p = fmt;
    APICHAR buf[32];
    int written = 0, pos, i;
    FUNC_NAME(pf_flags) flags;

    TRACE("Format is: %s\n", FUNC_NAME(debugstr)(fmt));

    if(!locale)
        locinfo = get_locinfo();
    else
        locinfo = locale->locinfo;

    while(*p) {
        /* output characters before '%' */
        for(q=p; *q && *q!='%'; q++);
        if(p != q) {
            i = pf_puts(puts_ctx, q-p, p);
            if(i < 0)
                return i;

            written += i;
            p = q;
            continue;
        }

        /* *p == '%' here */
        p++;

        /* output a single '%' character */
        if(*p == '%') {
            i = pf_puts(puts_ctx, 1, p++);
            if(i < 0)
                return i;

            written += i;
            continue;
        }

        /* check parameter position */
        if(positional_params && (q = FUNC_NAME(pf_parse_int)(p, &pos)) && *q=='$')
            p = q+1;
        else
            pos = -1;

        /* parse the flags */
        memset(&flags, 0, sizeof(flags));
        while(*p) {
            if(*p=='+' || *p==' ') {
                if(flags.Sign != '+')
                    flags.Sign = *p;
            } else if(*p == '-')
                flags.LeftAlign = *p;
            else if(*p == '0')
                flags.PadZero = *p;
            else if(*p == '#')
                flags.Alternate = *p;
            else
                break;

            p++;
        }

        /* parse the widh */
        if(*p == '*') {
            p++;
            if(positional_params && (q = FUNC_NAME(pf_parse_int)(p, &i)) && *q=='$')
                p = q+1;
            else
                i = -1;

            flags.FieldLength = pf_args(args_ctx, i, VT_INT, valist).get_int;
            if(flags.FieldLength < 0) {
                flags.LeftAlign = '-';
                flags.FieldLength = -flags.FieldLength;
            }
        } else while(isdigit(*p)) {
            flags.FieldLength *= 10;
            flags.FieldLength += *p++ - '0';
        }

        /* parse the precision */
        flags.Precision = -1;
        if(*p == '.') {
            flags.Precision = 0;
            p++;
            if(*p == '*') {
                p++;
                if(positional_params && (q = FUNC_NAME(pf_parse_int)(p, &i)) && *q=='$')
                    p = q+1;
                else
                    i = -1;

                flags.Precision = pf_args(args_ctx, i, VT_INT, valist).get_int;
            } else while(isdigit(*p)) {
                flags.Precision *= 10;
                flags.Precision += *p++ - '0';
            }
        }

        /* parse argument size modifier */
        while(*p) {
            if(*p=='l' && *(p+1)=='l') {
                flags.IntegerDouble++;
                p += 2;
            } else if(*p=='h' || *p=='l' || *p=='L') {
                flags.IntegerLength = *p;
                p++;
            } else if(*p == 'I') {
                if(*(p+1)=='6' && *(p+2)=='4') {
                    flags.IntegerDouble++;
                    p += 3;
                } else if(*(p+1)=='3' && *(p+2)=='2')
                    p += 3;
                else if(isdigit(*(p+1)) || !*(p+1))
                    break;
                else
                    p++;
            } else if(*p == 'w')
                flags.WideString = *p++;
            else if(*p == 'F')
                p++; /* ignore */
            else
                break;
        }

        flags.Format = *p;

        if(flags.Format == 's' || flags.Format == 'S') {
            i = FUNC_NAME(pf_handle_string)(pf_puts, puts_ctx,
                    pf_args(args_ctx, pos, VT_PTR, valist).get_ptr,
                    -1,  &flags, locinfo);
        } else if(flags.Format == 'c' || flags.Format == 'C') {
            int ch = pf_args(args_ctx, pos, VT_INT, valist).get_int;

            if((ch&0xff) != ch)
                FIXME("multibyte characters printing not supported\n");

            i = FUNC_NAME(pf_handle_string)(pf_puts, puts_ctx, &ch, 1, &flags, locinfo);
        } else if(flags.Format == 'p') {
            flags.Format = 'X';
            flags.PadZero = '0';
            i = flags.Precision;
            flags.Precision = 2*sizeof(void*);
            FUNC_NAME(pf_integer_conv)(buf, sizeof(buf)/sizeof(APICHAR), &flags,
                                       (ULONG_PTR)pf_args(args_ctx, pos, VT_PTR, valist).get_ptr);
            flags.PadZero = 0;
            flags.Precision = i;

#ifdef PRINTF_WIDE
            i = FUNC_NAME(pf_output_format_wstr)(pf_puts, puts_ctx, buf, -1, &flags, locinfo);
#else
            i = FUNC_NAME(pf_output_format_str)(pf_puts, puts_ctx, buf, -1, &flags, locinfo);
#endif
        } else if(flags.Format == 'n') {
            int *used;

            if(!n_format_enabled) {
                MSVCRT_INVALID_PMT("\'n\' format specifier disabled", MSVCRT_EINVAL);
                return -1;
            }

            used = pf_args(args_ctx, pos, VT_PTR, valist).get_ptr;
            *used = written;
            i = 0;
        } else if(flags.Format && strchr("diouxX", flags.Format)) {
            APICHAR *tmp = buf;
            int max_len;

            /* 0 padding is added after '0x' if Alternate flag is in use */
            if((flags.Format=='x' || flags.Format=='X') && flags.PadZero && flags.Alternate
                    && !flags.LeftAlign && flags.Precision<flags.FieldLength-2)
                flags.Precision = flags.FieldLength - 2;

            max_len = (flags.FieldLength>flags.Precision ? flags.FieldLength : flags.Precision) + 10;
            if(max_len > sizeof(buf)/sizeof(APICHAR))
                tmp = HeapAlloc(GetProcessHeap(), 0, max_len);
            if(!tmp)
                return -1;

            if(flags.IntegerDouble)
                FUNC_NAME(pf_integer_conv)(tmp, max_len, &flags, pf_args(args_ctx, pos,
                            VT_I8, valist).get_longlong);
            else if(flags.Format=='d' || flags.Format=='i')
                FUNC_NAME(pf_integer_conv)(tmp, max_len, &flags, flags.IntegerLength!='h' ?
                        pf_args(args_ctx, pos, VT_INT, valist).get_int :
                        (short)pf_args(args_ctx, pos, VT_INT, valist).get_int);
            else
                FUNC_NAME(pf_integer_conv)(tmp, max_len, &flags, flags.IntegerLength!='h' ?
                        (unsigned)pf_args(args_ctx, pos, VT_INT, valist).get_int :
                        (unsigned short)pf_args(args_ctx, pos, VT_INT, valist).get_int);

#ifdef PRINTF_WIDE
            i = FUNC_NAME(pf_output_format_wstr)(pf_puts, puts_ctx, tmp, -1, &flags, locinfo);
#else
            i = FUNC_NAME(pf_output_format_str)(pf_puts, puts_ctx, tmp, -1, &flags, locinfo);
#endif
            if(tmp != buf)
                HeapFree(GetProcessHeap(), 0, tmp);
        } else if(flags.Format && strchr("aeEfgG", flags.Format)) {
            char float_fmt[20], buf_a[32], *tmp = buf_a, *decimal_point;
            int len = flags.Precision + 10;
            double val = pf_args(args_ctx, pos, VT_R8, valist).get_double;
            int r;
            BOOL inf = FALSE, nan = FALSE;

            if(isinf(val))
                inf = TRUE;
            else if(isnan(val))
                nan = TRUE; /* FIXME: NaN value may be displayed as #IND or #QNAN */

            if(inf || nan) {
                if(nan || val<0)
                    flags.Sign = '-';

                if(flags.Format=='g' || flags.Format=='G')
                    val = 1.1234; /* fraction will be overwritten with #INF or #IND string */
                else
                    val = 1;

                if(flags.Format=='a') {
                    if(flags.Precision==-1)
                        flags.Precision = 6; /* strlen("#INF00") */
                }
            }

            if(flags.Format=='f') {
                if(val>-10.0 && val<10.0)
                    i = 1;
                else
                    i = 1 + log10(val<0 ? -val : val);
                /* Default precision is 6, additional space for sign, separator and nullbyte is required */
                i += (flags.Precision==-1 ? 6 : flags.Precision) + 3;

                if(i > len)
                    len = i;
            }

            if(len > sizeof(buf_a))
                tmp = HeapAlloc(GetProcessHeap(), 0, len);
            if(!tmp)
                return -1;

            FUNC_NAME(pf_rebuild_format_string)(float_fmt, &flags);
            if(val < 0) {
                flags.Sign = '-';
                val = -val;
            }

            sprintf(tmp, float_fmt, val);
            if(toupper(flags.Format)=='E' || toupper(flags.Format)=='G')
                FUNC_NAME(pf_fixup_exponent)(tmp);

            decimal_point = strchr(tmp, '.');
            if(decimal_point) {
                *decimal_point = *locinfo->lconv->decimal_point;

                if(inf || nan) {
                    static const char inf_str[] = "#INF";
                    static const char ind_str[] = "#IND";

                    for(i=1; i<(inf ? sizeof(inf_str) : sizeof(ind_str)); i++) {
                        if(decimal_point[i]<'0' || decimal_point[i]>'9')
                            break;

                        decimal_point[i] = inf ? inf_str[i-1] : ind_str[i-1];
                    }

                    if(i!=sizeof(inf_str) && i!=1)
                        decimal_point[i-1]++;
                }
            }

            len = strlen(tmp);
            i = FUNC_NAME(pf_fill)(pf_puts, puts_ctx, len, &flags, TRUE);
            if(i < 0)
                return i;
            r = FUNC_NAME(pf_output_str)(pf_puts, puts_ctx, tmp, len, locinfo);
            if(r < 0)
                return r;
            i += r;
            if(tmp != buf_a)
                HeapFree(GetProcessHeap(), 0, tmp);
            r = FUNC_NAME(pf_fill)(pf_puts, puts_ctx, len, &flags, FALSE);
            if(r < 0)
                return r;
            i += r;
        } else {
            if(invoke_invalid_param_handler) {
                MSVCRT__invalid_parameter(NULL, NULL, NULL, 0, 0);
                *MSVCRT__errno() = MSVCRT_EINVAL;
                return -1;
            }

            continue;
        }

        if(i < 0)
            return i;
        written += i;
        p++;
    }

    return written;
}

#ifndef PRINTF_WIDE
enum types_clbk_flags {
    TYPE_CLBK_VA_LIST = 1,
    TYPE_CLBK_POSITIONAL = 2,
    TYPE_CLBK_ERROR_POS = 4,
    TYPE_CLBK_ERROR_TYPE = 8
};

/* This functions stores types of arguments. It uses args[0] internally */
static printf_arg arg_clbk_type(void *ctx, int pos, int type, __ms_va_list *valist)
{
    static const printf_arg ret;
    printf_arg *args = ctx;

    if(pos == -1) {
        args[0].get_int |= TYPE_CLBK_VA_LIST;
        return ret;
    } else
        args[0].get_int |= TYPE_CLBK_POSITIONAL;

    if(pos<1 || pos>MSVCRT__ARGMAX)
        args[0].get_int |= TYPE_CLBK_ERROR_POS;
    else if(args[pos].get_int && args[pos].get_int!=type)
        args[0].get_int |= TYPE_CLBK_ERROR_TYPE;
    else
        args[pos].get_int = type;

    return ret;
}
#endif

static int FUNC_NAME(create_positional_ctx)(void *args_ctx, const APICHAR *format, __ms_va_list valist)
{
    struct FUNC_NAME(_str_ctx) puts_ctx = {INT_MAX, NULL};
    printf_arg *args = args_ctx;
    int i, j;

    i = FUNC_NAME(pf_printf)(FUNC_NAME(puts_clbk_str), &puts_ctx, format, NULL, TRUE, FALSE,
            arg_clbk_type, args_ctx, NULL);
    if(i < 0)
        return i;

    if(args[0].get_int==0 || args[0].get_int==TYPE_CLBK_VA_LIST)
        return 0;
    if(args[0].get_int != TYPE_CLBK_POSITIONAL)
        return -1;

    for(i=MSVCRT__ARGMAX; i>0; i--)
        if(args[i].get_int)
            break;

    for(j=1; j<=i; j++) {
        switch(args[j].get_int) {
        case VT_I8:
            args[j].get_longlong = va_arg(valist, LONGLONG);
            break;
        case VT_INT:
            args[j].get_int = va_arg(valist, int);
            break;
        case VT_R8:
            args[j].get_double = va_arg(valist, double);
            break;
        case VT_PTR:
            args[j].get_ptr = va_arg(valist, void*);
            break;
        default:
            return -1;
        }
    }

    return j;
}

#undef APICHAR
#undef CONVCHAR
#undef FUNC_NAME
