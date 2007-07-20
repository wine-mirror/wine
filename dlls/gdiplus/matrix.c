/*
 * Copyright (C) 2007 Google (Evan Stade)
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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"

#include "gdiplus.h"
#include "gdiplus_private.h"

/* Multiplies two matrices of the form
 *
 * idx:0 idx:1     0
 * idx:2 idx:3     0
 * idx:4 idx:5     1
 *
 * and puts the output in out.
 * */
static void matrix_multiply(GDIPCONST REAL * left, GDIPCONST REAL * right, REAL * out)
{
    REAL temp[6];
    int i, odd;

    for(i = 0; i < 6; i++){
        odd = i % 2;
        temp[i] = left[i - odd] * right[odd] + left[i - odd + 1] * right[odd + 2] +
                  (i >= 4 ? right[odd + 4] : 0.0);
    }

    memcpy(out, temp, 6 * sizeof(REAL));
}

GpStatus WINGDIPAPI GdipCreateMatrix2(REAL m11, REAL m12, REAL m21, REAL m22,
    REAL dx, REAL dy, GpMatrix **matrix)
{
    if(!matrix)
        return InvalidParameter;

    *matrix = GdipAlloc(sizeof(GpMatrix));
    if(!*matrix)    return OutOfMemory;

    /* first row */
    (*matrix)->matrix[0] = m11;
    (*matrix)->matrix[1] = m12;
    /* second row */
    (*matrix)->matrix[2] = m21;
    (*matrix)->matrix[3] = m22;
    /* third row */
    (*matrix)->matrix[4] = dx;
    (*matrix)->matrix[5] = dy;

    return Ok;
}

GpStatus WINGDIPAPI GdipDeleteMatrix(GpMatrix *matrix)
{
    if(!matrix)
        return InvalidParameter;

    GdipFree(matrix);

    return Ok;
}

GpStatus WINGDIPAPI GdipMultiplyMatrix(GpMatrix *matrix, GpMatrix* matrix2,
    GpMatrixOrder order)
{
    if(!matrix || !matrix2)
        return InvalidParameter;

    if(order == MatrixOrderAppend)
        matrix_multiply(matrix->matrix, matrix2->matrix, matrix->matrix);
    else
        matrix_multiply(matrix2->matrix, matrix->matrix, matrix->matrix);

    return Ok;
}

GpStatus WINGDIPAPI GdipScaleMatrix(GpMatrix *matrix, REAL scaleX, REAL scaleY,
    GpMatrixOrder order)
{
    REAL scale[6];

    if(!matrix)
        return InvalidParameter;

    scale[0] = scaleX;
    scale[1] = 0.0;
    scale[2] = 0.0;
    scale[3] = scaleY;
    scale[4] = 0.0;
    scale[5] = 0.0;

    if(order == MatrixOrderAppend)
        matrix_multiply(matrix->matrix, scale, matrix->matrix);
    else
        matrix_multiply(scale, matrix->matrix, matrix->matrix);

    return Ok;
}

GpStatus WINGDIPAPI GdipTransformMatrixPoints(GpMatrix *matrix, GpPointF *pts,
                                              INT count)
{
    REAL x, y;
    INT i;

    if(!matrix || !pts)
        return InvalidParameter;

    for(i = 0; i < count; i++)
    {
        x = pts[i].X;
        y = pts[i].Y;

        pts[i].X = x * matrix->matrix[0] + y * matrix->matrix[2] + matrix->matrix[4];
        pts[i].Y = x * matrix->matrix[1] + y * matrix->matrix[3] + matrix->matrix[5];
    }

    return Ok;
}
