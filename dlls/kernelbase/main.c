/*
 * Copyright 2016 Michael Müller
 * Copyright 2017 Andrey Gusev
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

#include "windows.h"
#include "appmodel.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(kernelbase);

/***********************************************************************
 *          AppPolicyGetProcessTerminationMethod (KERNELBASE.@)
 */
LONG WINAPI AppPolicyGetProcessTerminationMethod(HANDLE token, AppPolicyProcessTerminationMethod *policy)
{
    FIXME("%p, %p\n", token, policy);

    if(policy)
        *policy = AppPolicyProcessTerminationMethod_ExitProcess;

    return ERROR_SUCCESS;
}

/***********************************************************************
 *          AppPolicyGetThreadInitializationType (KERNELBASE.@)
 */
LONG WINAPI AppPolicyGetThreadInitializationType(HANDLE token, AppPolicyThreadInitializationType *policy)
{
    FIXME("%p, %p\n", token, policy);

    if(policy)
        *policy = AppPolicyThreadInitializationType_None;

    return ERROR_SUCCESS;
}

/***********************************************************************
 *          AppPolicyGetShowDeveloperDiagnostic (KERNELBASE.@)
 */
LONG WINAPI AppPolicyGetShowDeveloperDiagnostic(HANDLE token, AppPolicyShowDeveloperDiagnostic *policy)
{
    FIXME("%p, %p\n", token, policy);

    if(policy)
        *policy = AppPolicyShowDeveloperDiagnostic_ShowUI;

    return ERROR_SUCCESS;
}

/***********************************************************************
 *          AppPolicyGetWindowingModel (KERNELBASE.@)
 */
LONG WINAPI AppPolicyGetWindowingModel(HANDLE token, AppPolicyWindowingModel *policy)
{
    FIXME("%p, %p\n", token, policy);

    if(policy)
        *policy = AppPolicyWindowingModel_ClassicDesktop;

    return ERROR_SUCCESS;
}

/***********************************************************************
 *           QuirkIsEnabled   (KERNELBASE.@)
 */
BOOL WINAPI QuirkIsEnabled(void *arg)
{
    FIXME("(%p): stub\n", arg);
    return FALSE;
}

/***********************************************************************
 *          QuirkIsEnabled3 (KERNELBASE.@)
 */
BOOL WINAPI QuirkIsEnabled3(void *unk1, void *unk2)
{
    static int once;

    if (!once++)
        FIXME("(%p, %p) stub!\n", unk1, unk2);

    return FALSE;
}
