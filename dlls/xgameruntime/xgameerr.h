/*
 * Copyright (C) the Wine project
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

#ifndef __WINE_XGAMEERR_H
#define __WINE_XGAMEERR_H

#include <specstrings.h>

#define E_GAME_MISSING_GAME_CONFIG                                  _HRESULT_TYPEDEF_(0x87E5001FL)
#define E_GAMERUNTIME_NOT_INITIALIZED                               _HRESULT_TYPEDEF_(0x89240100L)
#define E_GAMERUNTIME_DLL_NOT_FOUND                                 _HRESULT_TYPEDEF_(0x89240101L)
#define E_GAMERUNTIME_VERSION_MISMATCH                              _HRESULT_TYPEDEF_(0x89240102L)
#define E_GAMERUNTIME_WINDOW_NOT_FOREGROUND                         _HRESULT_TYPEDEF_(0x89240103L)
#define E_GAMERUNTIME_SUSPENDED                                     _HRESULT_TYPEDEF_(0x89240104L)
#define E_GAMERUNTIME_UNINITIALIZE_ACTIVEOBJECTS                    _HRESULT_TYPEDEF_(0x89240105L)
#define E_GAMERUNTIME_MULTIPLAYER_NOT_CONFIGURED                    _HRESULT_TYPEDEF_(0x89240106L)
#define E_GAMERUNTIME_MISSING_DEPENDENCY                            _HRESULT_TYPEDEF_(0x89240107L)
#define E_GAMERUNTIME_SUSPEND_ACTIVEOBJECTS                         _HRESULT_TYPEDEF_(0x89240108L)
#define E_GAMERUNTIME_OPTIONS_MISMATCH                              _HRESULT_TYPEDEF_(0x89240109L)
#define E_GAMERUNTIME_OPTIONS_NOT_SUPPORTED                         _HRESULT_TYPEDEF_(0x8924010AL)
#define E_GAMERUNTIME_GAMECONFIG_BAD_FORMAT                         _HRESULT_TYPEDEF_(0x8924010BL)
#define E_GAMERUNTIME_INVALID_HANDLE                                _HRESULT_TYPEDEF_(0x8924010CL)
#define E_GAMEUSER_NO_AUTH_USER                                     _HRESULT_TYPEDEF_(0x87DD0013L)
#define E_GAMEUSER_USER_NOT_IN_SANDBOX                              _HRESULT_TYPEDEF_(0x8015DC12L)
#define E_GAMEUSER_MAX_USERS_ADDED                                  _HRESULT_TYPEDEF_(0x89245100L)
#define E_GAMEUSER_SIGNED_OUT                                       _HRESULT_TYPEDEF_(0x89245101L)
#define E_GAMEUSER_RESOLVE_USER_ISSUE_REQUIRED                      _HRESULT_TYPEDEF_(0x89245102L)
#define E_GAMEUSER_DEFERRAL_NOT_AVAILABLE                           _HRESULT_TYPEDEF_(0x89245103L)
#define E_GAMEUSER_USER_NOT_FOUND                                   _HRESULT_TYPEDEF_(0x89245104L)
#define E_GAMEUSER_NO_TOKEN_REQUIRED                                _HRESULT_TYPEDEF_(0x89245105L)
#define E_GAMEUSER_NO_DEFAULT_USER                                  _HRESULT_TYPEDEF_(0x89245106L)
#define E_GAMEUSER_FAILED_TO_RESOLVE                                _HRESULT_TYPEDEF_(0x89245107L)
#define E_GAMEUSER_NO_TITLE_ID                                      _HRESULT_TYPEDEF_(0x89245108L)
#define E_GAMEUSER_UNKNOWN_GAME_IDENTITY                            _HRESULT_TYPEDEF_(0x89245109L)
#define E_GAMEUSER_NO_PACKAGE_IDENTITY                              _HRESULT_TYPEDEF_(0x89245110L)
#define E_GAMEUSER_FAILED_TO_GET_TOKEN                              _HRESULT_TYPEDEF_(0x89245111L)
#define E_GAMEUSER_INVALID_APP_CONFIGURATION                        _HRESULT_TYPEDEF_(0x89245112L)
#define E_GAMEUSER_MALFORMED_MSAAPPID                               _HRESULT_TYPEDEF_(0x89245113L)
#define E_GAMEUSER_INCONSISTENT_MSAAPPID_AND_TITLEID                _HRESULT_TYPEDEF_(0x89245114L)
#define E_GAMEUSER_NO_MSAAPPID                                      _HRESULT_TYPEDEF_(0x89245115L)
#define E_GAMEPACKAGE_APP_NOT_PACKAGED                              _HRESULT_TYPEDEF_(0x89245200L)
#define E_GAMEPACKAGE_NO_INSTALLED_LANGUAGES                        _HRESULT_TYPEDEF_(0x89245201L)
#define E_GAMEPACKAGE_NO_STORE_ID                                   _HRESULT_TYPEDEF_(0x89245202L)
#define E_GAMEPACKAGE_INVALID_SELECTOR                              _HRESULT_TYPEDEF_(0x89245203L)
#define E_GAMEPACKAGE_DOWNLOAD_REQUIRED                             _HRESULT_TYPEDEF_(0x89245204L)
#define E_GAMEPACKAGE_NO_TAG_CHANGE                                 _HRESULT_TYPEDEF_(0x89245205L)
#define E_GAMEPACKAGE_DLC_NOT_SUPPORTED                             _HRESULT_TYPEDEF_(0x89245206L)
#define E_GAMEPACKAGE_DUPLICATE_ID_VALUES                           _HRESULT_TYPEDEF_(0x89245207L)
#define E_GAMEPACKAGE_NO_PACKAGE_IDENTIFIER                         _HRESULT_TYPEDEF_(0x89245208L)
#define E_GAMEPACKAGE_CONFIG_NO_ROOT_NODE                           _HRESULT_TYPEDEF_(0x89245209L)
#define E_GAMEPACKAGE_CONFIG_ZERO_VERSION                           _HRESULT_TYPEDEF_(0x8924520AL)
#define E_GAMEPACKAGE_CONFIG_NO_MSAAPPID_NOTITLEID                  _HRESULT_TYPEDEF_(0x8924520BL)
#define E_GAMEPACKAGE_CONFIG_DEPRECATED_PC_ENTRIES                  _HRESULT_TYPEDEF_(0x8924520CL)
#define E_GAMEPACKAGE_CONFIG_SUM_REQUIRES_MSAAPPID                  _HRESULT_TYPEDEF_(0x8924520DL)
#define E_GAMEPACKAGE_CONFIG_NO_CODE_CLOUD_SAVES_REQUIRES_MSAAPPID  _HRESULT_TYPEDEF_(0x8924520EL)
#define E_GAMEPACKAGE_CONFIG_MSAAPPID_OR_TITLEID_IS_DEFAULT         _HRESULT_TYPEDEF_(0x8924520FL)
#define E_GAMEPACKAGE_CONFIG_INVALID_CONTROL_CHARACTERS             _HRESULT_TYPEDEF_(0x89245210L)
#define E_GAMEPACKAGE_CONFIG_PROTOCOL_REQUIRES_EXECUTABLE           _HRESULT_TYPEDEF_(0x89245211L)
#define E_GAMESTORE_LICENSE_ACTION_NOT_APPLICABLE_TO_PRODUCT        _HRESULT_TYPEDEF_(0x89245300L)
#define E_GAMESTORE_NETWORK_ERROR                                   _HRESULT_TYPEDEF_(0x89245301L)
#define E_GAMESTORE_SERVER_ERROR                                    _HRESULT_TYPEDEF_(0x89245302L)
#define E_GAMESTORE_INSUFFICIENT_QUANTITY                           _HRESULT_TYPEDEF_(0x89245303L)
#define E_GAMESTORE_ALREADY_PURCHASED                               _HRESULT_TYPEDEF_(0x89245304L)
#define E_GAMESTORE_LICENSE_ACTION_THROTTLED                        _HRESULT_TYPEDEF_(0x89245305L)
#define E_GAMESTREAMING_NOT_INITIALIZED                             _HRESULT_TYPEDEF_(0x89245400L)
#define E_GAMESTREAMING_CLIENT_NOT_CONNECTED                        _HRESULT_TYPEDEF_(0x89245401L)
#define E_GAMESTREAMING_NO_DATA                                     _HRESULT_TYPEDEF_(0x89245402L)
#define E_GAMESTREAMING_NO_DATACENTER                               _HRESULT_TYPEDEF_(0x89245403L)
#define E_GAMESTREAMING_NOT_STREAMING_CONTROLLER                    _HRESULT_TYPEDEF_(0x89245404L)
#define E_GAMESTREAMING_NO_MATCH                                    _HRESULT_TYPEDEF_(0x89245405L)
#define E_GAMESTREAMING_TOO_MANY_CALLS                              _HRESULT_TYPEDEF_(0x89245406L)
#define E_GAMESTREAMING_CUSTOM_RESOLUTION_NOT_SUPPORTED             _HRESULT_TYPEDEF_(0x89245407L)
#define E_GAMESTREAMING_CUSTOM_RESOLUTION_TOO_SMALL                 _HRESULT_TYPEDEF_(0x89245408L)
#define E_GAMESTREAMING_CUSTOM_RESOLUTION_TOO_LARGE                 _HRESULT_TYPEDEF_(0x89245409L)
#define E_GAMESTREAMING_CUSTOM_RESOLUTION_TOO_MANY_PIXELS           _HRESULT_TYPEDEF_(0x8924540AL)
#define E_GAMESTREAMING_INVALID_CUSTOM_RESOLUTION                   _HRESULT_TYPEDEF_(0x8924540BL)

#endif
