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

#ifndef __WINE_XSPEECHSYNTHESIZER_H
#define __WINE_XSPEECHSYNTHESIZER_H

#ifdef __cplusplus
extern "C" {

enum class XSpeechSynthesizerVoiceGender : UINT32
{
    Female,
    Male,
};

#elif defined(__WINESRC__)

typedef enum XSpeechSynthesizerVoiceGender
{
    XSpeechSynthesizerVoiceGender_Female,
    XSpeechSynthesizerVoiceGender_Male,
} XSpeechSynthesizerVoiceGender;

#endif

typedef struct XSpeechSynthesizer *XSpeechSynthesizerHandle;
typedef struct XSpeechSynthesizerStream *XSpeechSynthesizerStreamHandle;

typedef struct XSpeechSynthesizerVoiceInformation XSpeechSynthesizerVoiceInformation;

typedef BOOLEAN __stdcall XSpeechSynthesizerInstalledVoicesCallback( const XSpeechSynthesizerVoiceInformation *information, void *context );

struct XSpeechSynthesizerVoiceInformation
{
    const char *Description;
    const char *DisplayName;
    XSpeechSynthesizerVoiceGender Gender;
    const char *VoiceId;
    const char *Language;
};

#ifdef __cplusplus
}
#endif

#endif
