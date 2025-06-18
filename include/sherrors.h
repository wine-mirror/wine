/*
 * Copyright 2024 Paul Gofman for CodeWeavers
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

#ifndef _SHERROR_
#define _SHERROR_

#include <winerror.h>

#define COPYENGINE_S_YES                                          _HRESULT_TYPEDEF_(0x270001)
#define COPYENGINE_S_NOT_HANDLED                                  _HRESULT_TYPEDEF_(0x270003)
#define COPYENGINE_S_USER_RETRY                                   _HRESULT_TYPEDEF_(0x270004)
#define COPYENGINE_S_USER_IGNORED                                 _HRESULT_TYPEDEF_(0x270005)
#define COPYENGINE_S_MERGE                                        _HRESULT_TYPEDEF_(0x270006)
#define COPYENGINE_S_DONT_PROCESS_CHILDREN                        _HRESULT_TYPEDEF_(0x270008)
#define COPYENGINE_S_ALREADY_DONE                                 _HRESULT_TYPEDEF_(0x27000a)
#define COPYENGINE_S_PENDING                                      _HRESULT_TYPEDEF_(0x27000b)
#define COPYENGINE_S_KEEP_BOTH                                    _HRESULT_TYPEDEF_(0x27000c)
#define COPYENGINE_S_CLOSE_PROGRAM                                _HRESULT_TYPEDEF_(0x27000d)
#define COPYENGINE_S_COLLISIONRESOLVED                            _HRESULT_TYPEDEF_(0x27000e)
#define COPYENGINE_S_PROGRESS_PAUSE                               _HRESULT_TYPEDEF_(0x27000f)
#define COPYENGINE_S_PENDING_DELETE                               _HRESULT_TYPEDEF_(0x270010)

#define COPYENGINE_E_USER_CANCELLED                               _HRESULT_TYPEDEF_(0x80270000)
#define COPYENGINE_E_CANCELLED                                    _HRESULT_TYPEDEF_(0x80270001)
#define COPYENGINE_E_REQUIRES_ELEVATION                           _HRESULT_TYPEDEF_(0x80270002)
#define COPYENGINE_E_SAME_FILE                                    _HRESULT_TYPEDEF_(0x80270003)
#define COPYENGINE_E_DIFF_DIR                                     _HRESULT_TYPEDEF_(0x80270004)
#define COPYENGINE_E_MANY_SRC_1_DEST                              _HRESULT_TYPEDEF_(0x80270005)
#define COPYENGINE_E_DEST_SUBTREE                                 _HRESULT_TYPEDEF_(0x80270009)
#define COPYENGINE_E_DEST_SAME_TREE                               _HRESULT_TYPEDEF_(0x8027000a)
#define COPYENGINE_E_FLD_IS_FILE_DEST                             _HRESULT_TYPEDEF_(0x8027000b)
#define COPYENGINE_E_FILE_IS_FLD_DEST                             _HRESULT_TYPEDEF_(0x8027000c)
#define COPYENGINE_E_FILE_TOO_LARGE                               _HRESULT_TYPEDEF_(0x8027000d)
#define COPYENGINE_E_REMOVABLE_FULL                               _HRESULT_TYPEDEF_(0x8027000e)
#define COPYENGINE_E_DEST_IS_RO_CD                                _HRESULT_TYPEDEF_(0x8027000f)
#define COPYENGINE_E_DEST_IS_RW_CD                                _HRESULT_TYPEDEF_(0x80270010)
#define COPYENGINE_E_DEST_IS_R_CD                                 _HRESULT_TYPEDEF_(0x80270011)
#define COPYENGINE_E_DEST_IS_RO_DVD                               _HRESULT_TYPEDEF_(0x80270012)
#define COPYENGINE_E_DEST_IS_RW_DVD                               _HRESULT_TYPEDEF_(0x80270013)
#define COPYENGINE_E_DEST_IS_R_DVD                                _HRESULT_TYPEDEF_(0x80270014)
#define COPYENGINE_E_SRC_IS_RO_CD                                 _HRESULT_TYPEDEF_(0x80270015)
#define COPYENGINE_E_SRC_IS_RW_CD                                 _HRESULT_TYPEDEF_(0x80270016)
#define COPYENGINE_E_SRC_IS_R_CD                                  _HRESULT_TYPEDEF_(0x80270017)
#define COPYENGINE_E_SRC_IS_RO_DVD                                _HRESULT_TYPEDEF_(0x80270018)
#define COPYENGINE_E_SRC_IS_RW_DVD                                _HRESULT_TYPEDEF_(0x80270019)
#define COPYENGINE_E_SRC_IS_R_DVD                                 _HRESULT_TYPEDEF_(0x8027001a)
#define COPYENGINE_E_INVALID_FILES_SRC                            _HRESULT_TYPEDEF_(0x8027001b)
#define COPYENGINE_E_INVALID_FILES_DEST                           _HRESULT_TYPEDEF_(0x8027001c)
#define COPYENGINE_E_PATH_TOO_DEEP_SRC                            _HRESULT_TYPEDEF_(0x8027001d)
#define COPYENGINE_E_PATH_TOO_DEEP_DEST                           _HRESULT_TYPEDEF_(0x8027001e)
#define COPYENGINE_E_ROOT_DIR_SRC                                 _HRESULT_TYPEDEF_(0x8027001f)
#define COPYENGINE_E_ROOT_DIR_DEST                                _HRESULT_TYPEDEF_(0x80270020)
#define COPYENGINE_E_ACCESS_DENIED_SRC                            _HRESULT_TYPEDEF_(0x80270021)
#define COPYENGINE_E_ACCESS_DENIED_DEST                           _HRESULT_TYPEDEF_(0x80270022)
#define COPYENGINE_E_PATH_NOT_FOUND_SRC                           _HRESULT_TYPEDEF_(0x80270023)
#define COPYENGINE_E_PATH_NOT_FOUND_DEST                          _HRESULT_TYPEDEF_(0x80270024)
#define COPYENGINE_E_NET_DISCONNECT_SRC                           _HRESULT_TYPEDEF_(0x80270025)
#define COPYENGINE_E_NET_DISCONNECT_DEST                          _HRESULT_TYPEDEF_(0x80270026)
#define COPYENGINE_E_SHARING_VIOLATION_SRC                        _HRESULT_TYPEDEF_(0x80270027)
#define COPYENGINE_E_SHARING_VIOLATION_DEST                       _HRESULT_TYPEDEF_(0x80270028)
#define COPYENGINE_E_ALREADY_EXISTS_NORMAL                        _HRESULT_TYPEDEF_(0x80270029)
#define COPYENGINE_E_ALREADY_EXISTS_READONLY                      _HRESULT_TYPEDEF_(0x8027002a)
#define COPYENGINE_E_ALREADY_EXISTS_SYSTEM                        _HRESULT_TYPEDEF_(0x8027002b)
#define COPYENGINE_E_ALREADY_EXISTS_FOLDER                        _HRESULT_TYPEDEF_(0x8027002c)
#define COPYENGINE_E_STREAM_LOSS                                  _HRESULT_TYPEDEF_(0x8027002d)
#define COPYENGINE_E_EA_LOSS                                      _HRESULT_TYPEDEF_(0x8027002e)
#define COPYENGINE_E_PROPERTY_LOSS                                _HRESULT_TYPEDEF_(0x8027002f)
#define COPYENGINE_E_PROPERTIES_LOSS                              _HRESULT_TYPEDEF_(0x80270030)
#define COPYENGINE_E_ENCRYPTION_LOSS                              _HRESULT_TYPEDEF_(0x80270031)
#define COPYENGINE_E_DISK_FULL                                    _HRESULT_TYPEDEF_(0x80270032)
#define COPYENGINE_E_DISK_FULL_CLEAN                              _HRESULT_TYPEDEF_(0x80270033)
#define COPYENGINE_E_EA_NOT_SUPPORTED                             _HRESULT_TYPEDEF_(0x80270034)
#define COPYENGINE_E_RECYCLE_UNKNOWN_ERROR                        _HRESULT_TYPEDEF_(0x80270035)
#define COPYENGINE_E_RECYCLE_FORCE_NUKE                           _HRESULT_TYPEDEF_(0x80270036)
#define COPYENGINE_E_RECYCLE_SIZE_TOO_BIG                         _HRESULT_TYPEDEF_(0x80270037)
#define COPYENGINE_E_RECYCLE_PATH_TOO_LONG                        _HRESULT_TYPEDEF_(0x80270038)
#define COPYENGINE_E_RECYCLE_BIN_NOT_FOUND                        _HRESULT_TYPEDEF_(0x8027003a)
#define COPYENGINE_E_NEWFILE_NAME_TOO_LONG                        _HRESULT_TYPEDEF_(0x8027003b)
#define COPYENGINE_E_NEWFOLDER_NAME_TOO_LONG                      _HRESULT_TYPEDEF_(0x8027003c)
#define COPYENGINE_E_DIR_NOT_EMPTY                                _HRESULT_TYPEDEF_(0x8027003d)
#define COPYENGINE_E_FAT_MAX_IN_ROOT                              _HRESULT_TYPEDEF_(0x8027003e)
#define COPYENGINE_E_ACCESSDENIED_READONLY                        _HRESULT_TYPEDEF_(0x8027003f)
#define COPYENGINE_E_REDIRECTED_TO_WEBPAGE                        _HRESULT_TYPEDEF_(0x80270040)
#define COPYENGINE_E_SERVER_BAD_FILE_TYPE                         _HRESULT_TYPEDEF_(0x80270041)
#define COPYENGINE_E_INTERNET_ITEM_UNAVAILABLE                    _HRESULT_TYPEDEF_(0x80270042)
#define COPYENGINE_E_CANNOT_MOVE_FROM_RECYCLE_BIN                 _HRESULT_TYPEDEF_(0x80270043)
#define COPYENGINE_E_CANNOT_MOVE_SHARED_FOLDER                    _HRESULT_TYPEDEF_(0x80270044)
#define COPYENGINE_E_INTERNET_ITEM_STORAGE_PROVIDER_ERROR         _HRESULT_TYPEDEF_(0x80270045)
#define COPYENGINE_E_INTERNET_ITEM_STORAGE_PROVIDER_PAUSED        _HRESULT_TYPEDEF_(0x80270046)
#define COPYENGINE_E_REQUIRES_EDP_CONSENT                         _HRESULT_TYPEDEF_(0x80270047)
#define COPYENGINE_E_BLOCKED_BY_EDP_POLICY                        _HRESULT_TYPEDEF_(0x80270048)
#define COPYENGINE_E_REQUIRES_EDP_CONSENT_FOR_REMOVABLE_DRIVE     _HRESULT_TYPEDEF_(0x80270049)
#define COPYENGINE_E_BLOCKED_BY_EDP_FOR_REMOVABLE_DRIVE           _HRESULT_TYPEDEF_(0x8027004a)
#define COPYENGINE_E_RMS_REQUIRES_EDP_CONSENT_FOR_REMOVABLE_DRIVE _HRESULT_TYPEDEF_(0x8027004b)
#define COPYENGINE_E_RMS_BLOCKED_BY_EDP_FOR_REMOVABLE_DRIVE       _HRESULT_TYPEDEF_(0x8027004c)
#define COPYENGINE_E_WARNED_BY_DLP_POLICY                         _HRESULT_TYPEDEF_(0x8027004d)
#define COPYENGINE_E_BLOCKED_BY_DLP_POLICY                        _HRESULT_TYPEDEF_(0x8027004e)
#define COPYENGINE_E_SILENT_FAIL_BY_DLP_POLICY                    _HRESULT_TYPEDEF_(0x8027004f)

#endif
