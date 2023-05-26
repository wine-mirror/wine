/*
 * Speech API (SAPI) Errors.
 *
 * Copyright (C) 2017 Huw Davies
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

#ifndef SPError_h
#define SPError_h

#include <winerror.h>

#define SP_END_OF_STREAM                          0x00045005
#define SP_INSUFFICIENT_DATA                      0x0004500b
#define SP_AUDIO_PAUSED                           0x00045010
#define SP_AUDIO_CONVERSION_ENABLED               0x00045015
#define SP_NO_HYPOTHESIS_AVAILABLE                0x00045016
#define SP_ALREADY_IN_LEX                         0x00045018
#define SP_LEX_NOTHING_TO_SYNC                    0x0004501a
#define SP_REQUEST_PENDING                        0x00045026
#define SP_NO_PARSE_FOUND                         0x0004502c
#define SP_UNSUPPORTED_ON_STREAM_INPUT            0x00045034
#define SP_WORD_EXISTS_WITHOUT_PRONUNCIATION      0x00045037
#define SP_RECOGNIZER_INACTIVE                    0x0004504e
#define SP_PARTIAL_PARSE_FOUND                    0x00045053
#define SP_NO_RULE_ACTIVE                         0x00045055
#define SP_STREAM_UNINITIALIZED                   0x00045057
#define SP_AUDIO_STOPPED                          0x00045065
#define SP_COMPLETE_BUT_EXTENDABLE                0x00045074
#define SP_NO_RULES_TO_ACTIVATE                   0x0004507b
#define SP_NO_WORDENTRY_NOTIFICATION              0x0004507c
#define S_LIMIT_REACHED                           0x0004507f
#define S_NOTSUPPORTED                            0x00045080

#define SPERR_UNINITIALIZED                       0x80045001
#define SPERR_ALREADY_INITIALIZED                 0x80045002
#define SPERR_UNSUPPORTED_FORMAT                  0x80045003
#define SPERR_INVALID_FLAGS                       0x80045004
#define SPERR_DEVICE_BUSY                         0x80045006
#define SPERR_DEVICE_NOT_SUPPORTED                0x80045007
#define SPERR_DEVICE_NOT_ENABLED                  0x80045008
#define SPERR_NO_DRIVER                           0x80045009
#define SPERR_FILE_MUST_BE_UNICODE                0x8004500a
#define SPERR_INVALID_PHRASE_ID                   0x8004500c
#define SPERR_BUFFER_TOO_SMALL                    0x8004500d
#define SPERR_FORMAT_NOT_SPECIFIED                0x8004500e
#define SPERR_AUDIO_STOPPED                       0x8004500f
#define SPERR_RULE_NOT_FOUND                      0x80045011
#define SPERR_TTS_ENGINE_EXCEPTION                0x80045012
#define SPERR_TTS_NLP_EXCEPTION                   0x80045013
#define SPERR_ENGINE_BUSY                         0x80045014
#define SPERR_CANT_CREATE                         0x80045017
#define SPERR_NOT_IN_LEX                          0x80045019
#define SPERR_LEX_VERY_OUT_OF_SYNC                0x8004501b
#define SPERR_UNDEFINED_FORWARD_RULE_REF          0x8004501c
#define SPERR_EMPTY_RULE                          0x8004501d
#define SPERR_GRAMMAR_COMPILER_INTERNAL_ERROR     0x8004501e
#define SPERR_RULE_NOT_DYNAMIC                    0x8004501f
#define SPERR_DUPLICATE_RULE_NAME                 0x80045020
#define SPERR_DUPLICATE_RESOURCE_NAME             0x80045021
#define SPERR_TOO_MANY_GRAMMARS                   0x80045022
#define SPERR_CIRCULAR_REFERENCE                  0x80045023
#define SPERR_INVALID_IMPORT                      0x80045024
#define SPERR_INVALID_WAV_FILE                    0x80045025
#define SPERR_ALL_WORDS_OPTIONAL                  0x80045027
#define SPERR_INSTANCE_CHANGE_INVALID             0x80045028
#define SPERR_RULE_NAME_ID_CONFLICT               0x80045029
#define SPERR_NO_RULES                            0x8004502a
#define SPERR_CIRCULAR_RULE_REF                   0x8004502b
#define SPERR_INVALID_HANDLE                      0x8004502d
#define SPERR_REMOTE_CALL_TIMED_OUT               0x8004502e
#define SPERR_AUDIO_BUFFER_OVERFLOW               0x8004502f
#define SPERR_NO_AUDIO_DATA                       0x80045030
#define SPERR_DEAD_ALTERNATE                      0x80045031
#define SPERR_HIGH_LOW_CONFIDENCE                 0x80045032
#define SPERR_INVALID_FORMAT_STRING               0x80045033
#define SPERR_APPLEX_READ_ONLY                    0x80045035
#define SPERR_NO_TERMINATING_RULE_PATH            0x80045036
#define SPERR_STREAM_CLOSED                       0x80045038
#define SPERR_NO_MORE_ITEMS                       0x80045039
#define SPERR_NOT_FOUND                           0x8004503a
#define SPERR_INVALID_AUDIO_STATE                 0x8004503b
#define SPERR_GENERIC_MMSYS_ERROR                 0x8004503c
#define SPERR_MARSHALER_EXCEPTION                 0x8004503d
#define SPERR_NOT_DYNAMIC_GRAMMAR                 0x8004503e
#define SPERR_AMBIGUOUS_PROPERTY                  0x8004503f
#define SPERR_INVALID_REGISTRY_KEY                0x80045040
#define SPERR_INVALID_TOKEN_ID                    0x80045041
#define SPERR_XML_BAD_SYNTAX                      0x80045042
#define SPERR_XML_RESOURCE_NOT_FOUND              0x80045043
#define SPERR_TOKEN_IN_USE                        0x80045044
#define SPERR_TOKEN_DELETED                       0x80045045
#define SPERR_MULTI_LINGUAL_NOT_SUPPORTED         0x80045046
#define SPERR_EXPORT_DYNAMIC_RULE                 0x80045047
#define SPERR_STGF_ERROR                          0x80045048
#define SPERR_WORDFORMAT_ERROR                    0x80045049
#define SPERR_STREAM_NOT_ACTIVE                   0x8004504a
#define SPERR_ENGINE_RESPONSE_INVALID             0x8004504b
#define SPERR_SR_ENGINE_EXCEPTION                 0x8004504c
#define SPERR_STREAM_POS_INVALID                  0x8004504d
#define SPERR_REMOTE_CALL_ON_WRONG_THREAD         0x8004504f
#define SPERR_REMOTE_PROCESS_TERMINATED           0x80045050
#define SPERR_REMOTE_PROCESS_ALREADY_RUNNING      0x80045051
#define SPERR_LANGID_MISMATCH                     0x80045052
#define SPERR_NOT_TOPLEVEL_RULE                   0x80045054
#define SPERR_LEX_REQUIRES_COOKIE                 0x80045056
#define SPERR_UNSUPPORTED_LANG                    0x80045059
#define SPERR_VOICE_PAUSED                        0x8004505a
#define SPERR_AUDIO_BUFFER_UNDERFLOW              0x8004505b
#define SPERR_AUDIO_STOPPED_UNEXPECTEDLY          0x8004505c
#define SPERR_NO_WORD_PRONUNCIATION               0x8004505d
#define SPERR_ALTERNATES_WOULD_BE_INCONSISTENT    0x8004505e
#define SPERR_NOT_SUPPORTED_FOR_SHARED_RECOGNIZER 0x8004505f
#define SPERR_TIMEOUT                             0x80045060
#define SPERR_REENTER_SYNCHRONIZE                 0x80045061
#define SPERR_STATE_WITH_NO_ARCS                  0x80045062
#define SPERR_NOT_ACTIVE_SESSION                  0x80045063
#define SPERR_ALREADY_DELETED                     0x80045064
#define SPERR_RECOXML_GENERATION_FAIL             0x80045066
#define SPERR_SML_GENERATION_FAIL                 0x80045067
#define SPERR_NOT_PROMPT_VOICE                    0x80045068
#define SPERR_ROOTRULE_ALREADY_DEFINED            0x80045069
#define SPERR_SCRIPT_DISALLOWED                   0x80045070
#define SPERR_REMOTE_CALL_TIMED_OUT_START         0x80045071
#define SPERR_REMOTE_CALL_TIMED_OUT_CONNECT       0x80045072
#define SPERR_SECMGR_CHANGE_NOT_ALLOWED           0x80045073
#define SPERR_FAILED_TO_DELETE_FILE               0x80045075
#define SPERR_SHARED_ENGINE_DISABLED              0x80045076
#define SPERR_RECOGNIZER_NOT_FOUND                0x80045077
#define SPERR_AUDIO_NOT_FOUND                     0x80045078
#define SPERR_NO_VOWEL                            0x80045079
#define SPERR_UNSUPPORTED_PHONEME                 0x8004507a
#define SPERR_WORD_NEEDS_NORMALIZATION            0x8004507d
#define SPERR_CANNOT_NORMALIZE                    0x8004507e
#define SPERR_TOPIC_NOT_ADAPTABLE                 0x80045081
#define SPERR_PHONEME_CONVERSION                  0x80045082
#define SPERR_NOT_SUPPORTED_FOR_INPROC_RECOGNIZER 0x80045083
#define SPERR_OVERLOAD                            0x80045084
#define SPERR_LEX_INVALID_DATA                    0x80045085
#define SPERR_CFG_INVALID_DATA                    0x80045086
#define SPERR_LEX_UNEXPECTED_FORMAT               0x80045087
#define SPERR_STRING_TOO_LONG                     0x80045088
#define SPERR_STRING_EMPTY                        0x80045089
#define SPERR_NON_WORD_TRANSITION                 0x80045090
#define SPERR_SISR_ATTRIBUTES_NOT_ALLOWED         0x80045091
#define SPERR_SISR_MIXED_NOT_ALLOWED              0x80045092
#define SPERR_VOICE_NOT_FOUND                     0x80045093

#endif /* SPError_h */
