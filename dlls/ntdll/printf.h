/*
 * ntdll printf functions
 *
 * Copyright 1999, 2009 Alexandre Julliard
 * Copyright 2000 Jon Griffiths
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
#define APICHAR wchar_t
#define APISTR(str) L"" str
#define FUNC_NAME(func) func ## _w
#else
#define APICHAR char
#define APISTR(str) str
#define FUNC_NAME(func) func ## _a
#endif

typedef struct
{
    APICHAR *buf;
    SIZE_T   len;
    SIZE_T   used;
} FUNC_NAME(pf_output);

/*
 * writes a string of characters to the output
 * returns -1 if the string doesn't fit in the output buffer
 * return the length of the string if all characters were written
 */
static int FUNC_NAME(pf_output_wstr)( FUNC_NAME(pf_output) *out, const WCHAR *str, int len )
{
    SIZE_T space = out->len - out->used;
    APICHAR *p = out->buf + out->used;
    ULONG n;

    if (len < 0) len = wcslen( str );

#ifdef PRINTF_WIDE
    n = len;
    if (out->buf) memcpy( p, str, min( n, space ) * sizeof(WCHAR) );
#else
    RtlUnicodeToMultiByteSize( &n, str, len * sizeof(WCHAR) );
    if (out->buf) RtlUnicodeToMultiByteN( p, min( n, space ), NULL, str, len * sizeof(WCHAR) );
#endif
    if (out->buf && space < n)  /* overflow */
    {
        out->used = out->len;
        return -1;
    }
    out->used += n;
    return len;
}

static int FUNC_NAME(pf_output_str)( FUNC_NAME(pf_output) *out, const char *str, int len )
{
    SIZE_T space = out->len - out->used;
    APICHAR *p = out->buf + out->used;
    ULONG n;

    if (len < 0) len = strlen( str );

#ifdef PRINTF_WIDE
    RtlMultiByteToUnicodeSize( &n, str, len );
    n /= sizeof(WCHAR);
    if (out->buf) RtlMultiByteToUnicodeN( p, min( n, space ) * sizeof(WCHAR), NULL, str, len );
#else
    n = len;
    if (out->buf) memcpy( p, str, min( n, space ));
#endif
    if (out->buf && space < n)  /* overflow */
    {
        out->used = out->len;
        return -1;
    }
    out->used += n;
    return len;
}

static int FUNC_NAME(pf_output_string)( FUNC_NAME(pf_output) *out, const APICHAR *str, int len )
{
#ifdef PRINTF_WIDE
    return FUNC_NAME(pf_output_wstr)( out, str, len );
#else
    return FUNC_NAME(pf_output_str)( out, str, len );
#endif
}

/* pf_fill: takes care of signs, alignment, zero and field padding */
static int FUNC_NAME(pf_fill_left)( FUNC_NAME(pf_output) *out, int len, pf_flags *flags )
{
    int i, r = 0;

    if (flags->Sign && !(flags->Format == 'd' || flags->Format == 'i')) flags->Sign = 0;

    if (flags->Sign)
    {
        APICHAR ch = flags->Sign;
        flags->FieldLength--;
        if (flags->PadZero) r = FUNC_NAME(pf_output_string)( out, &ch, 1 );
    }
    if (!flags->LeftAlign)
    {
        APICHAR ch = flags->PadZero ? '0' : ' ';
        for (i = 0; i < flags->FieldLength - len && r >= 0; i++)
            r = FUNC_NAME(pf_output_string)( out, &ch, 1 );
    }
    if (flags->Sign && !flags->PadZero && r >= 0)
    {
        APICHAR ch = flags->Sign;
        r = FUNC_NAME(pf_output_string)( out, &ch, 1 );
    }
    return r;
}

static int FUNC_NAME(pf_fill_right)( FUNC_NAME(pf_output) *out, int len, pf_flags *flags )
{
    int i, r = 0;
    APICHAR ch = ' ';

    if (!flags->LeftAlign) return 0;

    for (i = 0; i < flags->FieldLength - len && r >= 0; i++)
        r = FUNC_NAME(pf_output_string)( out, &ch, 1 );
    return r;
}

static int FUNC_NAME(pf_output_format_wstr)( FUNC_NAME(pf_output) *out, const wchar_t *str, int len,
                                             pf_flags *flags )
{
    int r = 0;

    if (len < 0)
    {
        /* Do not search past the length specified by the precision. */
        if (flags->Precision >= 0)
        {
            for (len = 0; len < flags->Precision; len++) if (!str[len]) break;
        }
        else len = wcslen( str );
    }

    if (flags->Precision >= 0 && flags->Precision < len) len = flags->Precision;

    r = FUNC_NAME(pf_fill_left)( out, len, flags );
    if (r >= 0) r = FUNC_NAME(pf_output_wstr)( out, str, len );
    if (r >= 0) r = FUNC_NAME(pf_fill_right)( out, len, flags );
    return r;
}

static int FUNC_NAME(pf_output_format_str)( FUNC_NAME(pf_output) *out, const char *str, int len,
                                            pf_flags *flags )
{
    int r = 0;

    if (len < 0)
    {
        /* Do not search past the length specified by the precision. */
        if (flags->Precision >= 0)
        {
            for (len = 0; len < flags->Precision; len++) if (!str[len]) break;
        }
        else len = strlen(str);
    }

    if (flags->Precision >= 0 && flags->Precision < len) len = flags->Precision;

    r = FUNC_NAME(pf_fill_left)( out, len, flags );
    if (r >= 0) r = FUNC_NAME(pf_output_str)( out, str, len );
    if (r >= 0) r = FUNC_NAME(pf_fill_right)( out, len, flags );
    return r;
}

static int FUNC_NAME(pf_output_format)( FUNC_NAME(pf_output) *out, const APICHAR *str, int len,
                                        pf_flags *flags )
{
#ifdef PRINTF_WIDE
    return FUNC_NAME(pf_output_format_wstr)( out, str, len, flags );
#else
    return FUNC_NAME(pf_output_format_str)( out, str, len, flags );
#endif
}

static int FUNC_NAME(pf_handle_string_format)( FUNC_NAME(pf_output) *out, const void* str, int len,
                                               pf_flags *flags, BOOL inverted )
{
     if (str == NULL)  /* catch NULL pointer */
         return FUNC_NAME(pf_output_format)( out, APISTR("(null)"), -1, flags );

     /* prefixes take priority over %c,%s vs. %C,%S, so we handle them first */
    if (flags->WideString || flags->IntegerLength == LEN_LONG)
        return FUNC_NAME(pf_output_format_wstr)( out, str, len, flags );
    if (flags->IntegerLength == LEN_SHORT)
        return FUNC_NAME(pf_output_format_str)( out, str, len, flags );

    /* %s,%c ->  chars in ansi functions & wchars in unicode
     * %S,%C -> wchars in ansi functions &  chars in unicode */
    if (!inverted) return FUNC_NAME(pf_output_format)( out, str, len, flags);
#ifdef PRINTF_WIDE
    return FUNC_NAME(pf_output_format_str)( out, str, len, flags );
#else
    return FUNC_NAME(pf_output_format_wstr)( out, str, len, flags );
#endif
}

/* pf_integer_conv:  prints x to buf, including alternate formats and
   additional precision digits, but not field characters or the sign */
static void FUNC_NAME(pf_integer_conv)( APICHAR *buf, pf_flags *flags, LONGLONG x )
{
    unsigned int base;
    const APICHAR *digits;
    int i, j, k;

    if( flags->Format == 'o' )
        base = 8;
    else if( flags->Format == 'x' || flags->Format == 'X' )
        base = 16;
    else
        base = 10;

    if( flags->Format == 'X' )
        digits = APISTR("0123456789ABCDEFX");
    else
        digits = APISTR("0123456789abcdefx");

    if( x < 0 && ( flags->Format == 'd' || flags->Format == 'i' ) )
    {
        x = -x;
        flags->Sign = '-';
    }

    i = 0;
    if( x == 0 )
    {
        flags->Alternate = FALSE;
        if( flags->Precision )
            buf[i++] = '0';
    }
    else
        while( x != 0 )
        {
            j = (ULONGLONG) x % base;
            x = (ULONGLONG) x / base;
            buf[i++] = digits[j];
        }
    k = flags->Precision - i;
    while( k-- > 0 )
        buf[i++] = '0';
    if( flags->Alternate )
    {
        if( base == 16 )
        {
            buf[i++] = digits[16];
            buf[i++] = '0';
        }
        else if( base == 8 && buf[i-1] != '0' )
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

static int FUNC_NAME(pf_vsnprintf)( FUNC_NAME(pf_output) *out, const APICHAR *format, va_list valist )
{
    int r;
    const APICHAR *q, *p = format;
    pf_flags flags;

    while (*p)
    {
        for (q = p; *q; q++) if (*q == '%') break;

        if (q != p)
        {
            r = FUNC_NAME(pf_output_string)(out, p, q - p);
            if (r < 0) return r;
            p = q;
            if (!*p) break;
        }
        p++;

        /* output a single % character */
        if( *p == '%' )
        {
            r = FUNC_NAME(pf_output_string)(out, p++, 1);
            if( r<0 )
                return r;
            continue;
        }

        /* parse the flags */
        memset( &flags, 0, sizeof flags );
        while (*p)
        {
            if( *p == '+' || *p == ' ' )
            {
                if ( flags.Sign != '+' )
                    flags.Sign = *p;
            }
            else if( *p == '-' )
                flags.LeftAlign = TRUE;
            else if( *p == '0' )
                flags.PadZero = TRUE;
            else if( *p == '#' )
                flags.Alternate = TRUE;
            else
                break;
            p++;
        }

        /* deal with the field width specifier */
        flags.FieldLength = 0;
        if( *p == '*' )
        {
            flags.FieldLength = va_arg( valist, int );
            if (flags.FieldLength < 0)
            {
                flags.LeftAlign = TRUE;
                flags.FieldLength = -flags.FieldLength;
            }
            p++;
        }
        else while (*p >= '0' && *p <= '9')
        {
            flags.FieldLength *= 10;
            flags.FieldLength += *p++ - '0';
        }

        /* deal with precision */
        flags.Precision = -1;
        if( *p == '.' )
        {
            flags.Precision = 0;
            p++;
            if( *p == '*' )
            {
                flags.Precision = va_arg( valist, int );
                p++;
            }
            else while (*p >= '0' && *p <= '9')
            {
                flags.Precision *= 10;
                flags.Precision += *p++ - '0';
            }
        }

        /* deal with integer width modifier */
        while( *p )
        {
            if (*p == 'l' && *(p+1) == 'l')
            {
                flags.IntegerDouble = TRUE;
                p += 2;
            }
            else if( *p == 'l' )
            {
                flags.IntegerLength = LEN_LONG;
                p++;
            }
            else if( *p == 'L' )
            {
                p++;
            }
            else if( *p == 'h')
            {
                flags.IntegerLength = LEN_SHORT;
                p++;
            }
            else if( *p == 'I' )
            {
                if( *(p+1) == '6' && *(p+2) == '4' )
                {
                    flags.IntegerDouble = TRUE;
                    p += 3;
                }
                else if( *(p+1) == '3' && *(p+2) == '2' )
                    p += 3;
                else if( p[1] && strchr( "diouxX", p[1] ) )
                {
                    if( sizeof(void *) == 8 )
                        flags.IntegerDouble = TRUE;
                    p++;
                }
                else if ((p[1] >= '0' && p[1] <= '9') || !p[1])
                    break;
                else
                    p++;
            }
            else if( *p == 'w' )
            {
                flags.WideString = TRUE;
                p++;
            }
            else if ((*p == 'z' || *p == 't') && p[1] && strchr("diouxX", p[1]))
            {
                flags.IntegerNative = TRUE;
                p++;
            }
            else if (*p == 'j')
            {
                flags.IntegerDouble = TRUE;
                p++;
            }
            else if( *p == 'F' )
                p++; /* ignore */
            else
                break;
        }

        flags.Format = *p;
        r = 0;

        if (flags.Format == '$')
        {
            FIXME("Positional parameters are not supported (%s)\n", FUNC_NAME(wine_dbgstr)(format));
            return -1;
        }
        /* output a string */
        if(  flags.Format == 's' || flags.Format == 'S' )
            r = FUNC_NAME(pf_handle_string_format)( out, va_arg(valist, const void*), -1,
                                                    &flags, (flags.Format == 'S') );

        /* output a single character */
        else if( flags.Format == 'c' || flags.Format == 'C' )
        {
            APICHAR ch = (APICHAR)va_arg( valist, int );
            r = FUNC_NAME(pf_handle_string_format)( out, &ch, 1, &flags, (flags.Format == 'C') );
        }

        /* output a pointer */
        else if( flags.Format == 'p' )
        {
            APICHAR pointer[32];
            void *ptr = va_arg( valist, void * );
            int prec = flags.Precision;
            flags.Format = 'X';
            flags.PadZero = TRUE;
            flags.Precision = 2*sizeof(void*);
            FUNC_NAME(pf_integer_conv)( pointer, &flags, (ULONG_PTR)ptr );
            flags.PadZero = FALSE;
            flags.Precision = prec;
            r = FUNC_NAME(pf_output_format)( out, pointer, -1, &flags );
        }

        /* deal with %n */
        else if( flags.Format == 'n' )
        {
            int *x = va_arg(valist, int *);
            *x = out->used;
        }
        else if( flags.Format && strchr("diouxX", flags.Format ))
        {
            APICHAR number[40], *x = number;
            int max_len;

            /* 0 padding is added after '0x' if Alternate flag is in use */
            if((flags.Format=='x' || flags.Format=='X') && flags.PadZero && flags.Alternate
                    && !flags.LeftAlign && flags.Precision<flags.FieldLength-2)
                flags.Precision = flags.FieldLength - 2;

            max_len = (flags.FieldLength>flags.Precision ? flags.FieldLength : flags.Precision) + 10;
            if(max_len > ARRAY_SIZE(number))
                if (!(x = RtlAllocateHeap( GetProcessHeap(), 0, max_len * sizeof(*x) ))) return -1;

            if(flags.IntegerDouble || (flags.IntegerNative && sizeof(void*) == 8))
                FUNC_NAME(pf_integer_conv)( x, &flags, va_arg(valist, LONGLONG) );
            else if(flags.Format=='d' || flags.Format=='i')
                FUNC_NAME(pf_integer_conv)( x, &flags, flags.IntegerLength != LEN_SHORT ?
                                            va_arg(valist, int) : (short)va_arg(valist, int) );
            else
                FUNC_NAME(pf_integer_conv)( x, &flags, flags.IntegerLength != LEN_SHORT ?
                                 (unsigned int)va_arg(valist, int) : (unsigned short)va_arg(valist, int) );

            r = FUNC_NAME(pf_output_format)( out, x, -1, &flags );
            if( x != number )
                RtlFreeHeap( GetProcessHeap(), 0, x );
        }
        else
            continue;

        if( r<0 )
            return r;
        p++;
    }
    return out->used;
}

#undef APICHAR
#undef APISTR
#undef FUNC_NAME
