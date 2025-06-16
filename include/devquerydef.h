/*
 * Copyright 2025 Vibhav Pant
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

#ifndef __DEVQUERYDEF_H__
#define __DEVQUERYDEF_H__

typedef enum _DEV_OBJECT_TYPE
{
    DevObjectTypeUnknown,
    DevObjectTypeDeviceInterface,
    DevObjectTypeDeviceContainer,
    DevObjectTypeDevice,
    DevObjectTypeDeviceInterfaceClass,
    DevObjectTypeAEP,
    DevObjectTypeAEPContainer,
    DevObjectTypeDeviceInstallerClass,
    DevObjectTypeDeviceInterfaceDisplay,
    DevObjectTypeDeviceContainerDisplay,
    DevObjectTypeAEPService,
    DevObjectTypeDevicePanel,
    DevObjectTypeAEPProtocol,
} DEV_OBJECT_TYPE, *PDEV_OBJECT_TYPE;

typedef enum _DEV_QUERY_FLAGS
{
    DevQueryFlagNone = 0x0,
    DevQueryFlagUpdateResults = 0x1,
    DevQueryFlagAllProperties = 0x2,
    DevQueryFlagLocalize = 0x4,
    DevQueryFlagAsyncClose = 0x8,
} DEV_QUERY_FLAGS, *PDEV_QUERY_FLAGS;

typedef enum _DEV_QUERY_STATE
{
    DevQueryStateInitialized,
    DevQueryStateEnumCompleted,
    DevQueryStateAborted,
    DevQueryStateClosed,
} DEV_QUERY_STATE, *PDEV_QUERY_STATE;

typedef enum _DEV_QUERY_RESULT_ACTION
{
    DevQueryResultStateChange,
    DevQueryResultAdd,
    DevQueryResultUpdate,
    DevQueryResultRemove,
} DEV_QUERY_RESULT_ACTION, *PDEV_QUERY_RESULT_ACTION;

typedef struct _DEV_OBJECT
{
    DEV_OBJECT_TYPE ObjectType;
    PCWSTR pszObjectId;
    ULONG cPropertyCount;
    const DEVPROPERTY *pProperties;
} DEV_OBJECT, *PDEV_OBJECT;

typedef struct _DEV_QUERY_RESULT_ACTION_DATA
{
    DEV_QUERY_RESULT_ACTION Action;
    union
    {
        DEV_QUERY_STATE State;
        DEV_OBJECT DeviceObject;
    } Data;
} DEV_QUERY_RESULT_ACTION_DATA, *PDEV_QUERY_RESULT_ACTION_DATA;

typedef struct _DEV_QUERY_PARAMETER
{
    DEVPROPKEY Key;
    DEVPROPTYPE Type;
    ULONG BufferSize;
    void *Buffer;
} DEV_QUERY_PARAMETER, *PDEV_QUERY_PARAMETER;

#endif /* __DEVQUERYDEF_H__ */
