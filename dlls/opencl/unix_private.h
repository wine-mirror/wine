/*
 * Copyright 2021 Zebediah Figura
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

#ifndef __WINE_UNIX_PRIVATE_H
#define __WINE_UNIX_PRIVATE_H

#include <stdarg.h>
#include <stdint.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "wine/unixlib.h"
#include "wine/debug.h"

#define CL_SILENCE_DEPRECATION
#if defined(HAVE_CL_CL_H)
#define CL_USE_DEPRECATED_OPENCL_1_0_APIS
#define CL_USE_DEPRECATED_OPENCL_1_1_APIS
#define CL_USE_DEPRECATED_OPENCL_1_2_APIS
#define CL_USE_DEPRECATED_OPENCL_2_0_APIS
#define CL_TARGET_OPENCL_VERSION 220
#include <CL/cl.h>
#elif defined(HAVE_OPENCL_OPENCL_H)
#include <OpenCL/opencl.h>
#endif

#include "unixlib.h"

NTSTATUS wrap_clBuildProgram( void *args );
NTSTATUS wrap_clCreateContext( void *args );
NTSTATUS wrap_clCreateContextFromType( void *args );
NTSTATUS wrap_clEnqueueNativeKernel( void *args );
NTSTATUS wrap_clSetEventCallback( void *args );
NTSTATUS wrap_clSetMemObjectDestructorCallback( void *args );
NTSTATUS wrap_clCompileProgram( void *args );
NTSTATUS wrap_clLinkProgram( void *args );

#endif
