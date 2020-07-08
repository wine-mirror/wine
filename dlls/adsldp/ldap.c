/*
 * Copyright 2020 Dmitry Timoshkov
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
#include "winerror.h"
#include "winldap.h"

DWORD map_ldap_error(DWORD err)
{
    switch (err)
    {
    case LDAP_SUCCESS:                  return ERROR_SUCCESS;
    case LDAP_OPERATIONS_ERROR:         return ERROR_DS_OPERATIONS_ERROR;
    case LDAP_PROTOCOL_ERROR:           return ERROR_DS_PROTOCOL_ERROR;
    case LDAP_TIMELIMIT_EXCEEDED:       return ERROR_DS_TIMELIMIT_EXCEEDED;
    case LDAP_SIZELIMIT_EXCEEDED:       return ERROR_DS_SIZELIMIT_EXCEEDED;
    case LDAP_COMPARE_FALSE:            return ERROR_DS_COMPARE_FALSE;
    case LDAP_COMPARE_TRUE:             return ERROR_DS_COMPARE_TRUE;
    case LDAP_AUTH_METHOD_NOT_SUPPORTED: return ERROR_DS_AUTH_METHOD_NOT_SUPPORTED;
    case LDAP_STRONG_AUTH_REQUIRED:     return ERROR_DS_STRONG_AUTH_REQUIRED;
    case LDAP_REFERRAL_V2:              return ERROR_DS_REFERRAL;
    case LDAP_REFERRAL:                 return ERROR_DS_REFERRAL;
    case LDAP_ADMIN_LIMIT_EXCEEDED:     return ERROR_DS_ADMIN_LIMIT_EXCEEDED;
    case LDAP_UNAVAILABLE_CRIT_EXTENSION: return ERROR_DS_UNAVAILABLE_CRIT_EXTENSION;
    case LDAP_CONFIDENTIALITY_REQUIRED: return ERROR_DS_CONFIDENTIALITY_REQUIRED;
    case LDAP_NO_SUCH_ATTRIBUTE:        return ERROR_DS_NO_ATTRIBUTE_OR_VALUE;
    case LDAP_UNDEFINED_TYPE:           return ERROR_DS_ATTRIBUTE_TYPE_UNDEFINED;
    case LDAP_INAPPROPRIATE_MATCHING:   return ERROR_DS_INAPPROPRIATE_MATCHING;
    case LDAP_CONSTRAINT_VIOLATION:     return ERROR_DS_CONSTRAINT_VIOLATION;
    case LDAP_ATTRIBUTE_OR_VALUE_EXISTS: return ERROR_DS_ATTRIBUTE_OR_VALUE_EXISTS;
    case LDAP_INVALID_SYNTAX:           return ERROR_DS_INVALID_ATTRIBUTE_SYNTAX;
    case LDAP_NO_SUCH_OBJECT:           return ERROR_DS_NO_SUCH_OBJECT;
    case LDAP_ALIAS_PROBLEM:            return ERROR_DS_ALIAS_PROBLEM;
    case LDAP_INVALID_DN_SYNTAX:        return ERROR_DS_INVALID_DN_SYNTAX;
    case LDAP_IS_LEAF:                  return ERROR_DS_IS_LEAF;
    case LDAP_ALIAS_DEREF_PROBLEM:      return ERROR_DS_ALIAS_DEREF_PROBLEM;
    case LDAP_INAPPROPRIATE_AUTH:       return ERROR_DS_INAPPROPRIATE_AUTH;
    case LDAP_INVALID_CREDENTIALS:      return ERROR_DS_SEC_DESC_INVALID;
    case LDAP_INSUFFICIENT_RIGHTS:      return ERROR_DS_INSUFF_ACCESS_RIGHTS;
    case LDAP_BUSY:                     return ERROR_DS_BUSY;
    case LDAP_UNAVAILABLE:              return ERROR_DS_UNAVAILABLE;
    case LDAP_UNWILLING_TO_PERFORM:     return ERROR_DS_UNWILLING_TO_PERFORM;
    case LDAP_LOOP_DETECT:              return ERROR_DS_LOOP_DETECT;
    case LDAP_SORT_CONTROL_MISSING:     return ERROR_DS_SORT_CONTROL_MISSING;
    case LDAP_OFFSET_RANGE_ERROR:       return ERROR_DS_OFFSET_RANGE_ERROR;
    case LDAP_NAMING_VIOLATION:         return ERROR_DS_NAMING_VIOLATION;
    case LDAP_OBJECT_CLASS_VIOLATION:   return ERROR_DS_OBJ_CLASS_VIOLATION;
    case LDAP_NOT_ALLOWED_ON_NONLEAF:   return ERROR_DS_CANT_ON_NON_LEAF;
    case LDAP_NOT_ALLOWED_ON_RDN:       return ERROR_DS_CANT_ON_RDN;
    case LDAP_ALREADY_EXISTS:           return ERROR_ALREADY_EXISTS;
    case LDAP_NO_OBJECT_CLASS_MODS:     return ERROR_DS_CANT_MOD_OBJ_CLASS;
    case LDAP_RESULTS_TOO_LARGE:        return ERROR_DS_OBJECT_RESULTS_TOO_LARGE;
    case LDAP_AFFECTS_MULTIPLE_DSAS:    return ERROR_DS_AFFECTS_MULTIPLE_DSAS;
    case LDAP_SERVER_DOWN:              return ERROR_DS_SERVER_DOWN;
    case LDAP_LOCAL_ERROR:              return ERROR_DS_LOCAL_ERROR;
    case LDAP_ENCODING_ERROR:           return ERROR_DS_ENCODING_ERROR;
    case LDAP_DECODING_ERROR:           return ERROR_DS_DECODING_ERROR;
    case LDAP_TIMEOUT:                  return ERROR_TIMEOUT;
    case LDAP_AUTH_UNKNOWN:             return ERROR_DS_AUTH_UNKNOWN;
    case LDAP_FILTER_ERROR:             return ERROR_DS_FILTER_UNKNOWN;
    case LDAP_USER_CANCELLED:           return ERROR_CANCELLED;
    case LDAP_PARAM_ERROR:              return ERROR_DS_PARAM_ERROR;
    case LDAP_NO_MEMORY:                return ERROR_NOT_ENOUGH_MEMORY;
    case LDAP_CONNECT_ERROR:            return ERROR_CONNECTION_UNAVAIL;
    case LDAP_NOT_SUPPORTED:            return ERROR_DS_NOT_SUPPORTED;
    case LDAP_CONTROL_NOT_FOUND:        return ERROR_DS_CONTROL_NOT_FOUND;
    case LDAP_NO_RESULTS_RETURNED:      return ERROR_DS_NO_RESULTS_RETURNED;
    case LDAP_MORE_RESULTS_TO_RETURN:   return ERROR_MORE_DATA;
    case LDAP_CLIENT_LOOP:              return ERROR_DS_CLIENT_LOOP;
    case LDAP_REFERRAL_LIMIT_EXCEEDED:  return ERROR_DS_REFERRAL_LIMIT_EXCEEDED;
    default: return err;
    }
}
