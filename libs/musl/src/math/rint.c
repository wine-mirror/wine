#include <float.h>
#include <math.h>
#include <stdint.h>
#include "libm.h"

static const float_t tointf = 1 / FLT_EPSILON;
static const double_t toint = 1 / DBL_EPSILON;

double __cdecl rint(double x)
{
#if defined(__GNUC__) && !defined(__clang__)
    return __builtin_rint(x);
#elif defined(__SSE_MATH__)
    union {double f; uint64_t i;} u = {x};
    int e = u.i>>52 & 0x7ff;
    int s = u.i>>63;
    double_t y;

    if (e >= 0x3ff+52)
        return x;
    if (s)
        y = x - toint + toint;
    else
        y = x + toint - toint;
    if (y == 0)
        return s ? -0.0 : 0;
    return y;
#else
    union {double f; uint64_t i;} u = {x};
    int e = (u.i >> 52 & 0x7ff) - 0x3ff;
    int s = u.i>>63;
    double_t y, z;
    float_t f;
    int even;

    if (e >= 52)
        return x;

    even = e <= -1 ? 1 : !((u.i >> (52 - e)) % 2);

    if (s)
    {
        y = ceil(x);
        z = x - y;
        if (z < -0.5) f = -0.75;
        else if (z == -0.5) f = -0.5;
        else if (z != 0.0) f = -0.25;
        else f = 0.0;

        f = (even ? -0.0 : -1.0) + f;
        f = fp_barrierf(f - tointf) + tointf;
        if (f == (even ? 0.0 : -1.0)) z = y;
        else z = floor(x);
    }
    else
    {
        y = floor(x);
        z = x - y;
        if (z > 0.5) f = 0.75;
        else if (z == 0.5) f = 0.5;
        else if (z != 0.0) f = 0.25;
        else f = 0.0;

        f = (even ? 0.0 : 1.0) + f;
        f = fp_barrierf(f + tointf) - tointf;
        if (f == (even ? 0.0 : 1.0)) z = y;
        else z = ceil(x);
    }

    return z;
#endif
}
