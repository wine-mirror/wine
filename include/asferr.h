/*
 * Copyright (C) 2020 Zebediah Figura for CodeWeavers
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

#ifndef _ASFERR_H
#define _ASFERR_H

#ifdef RC_INVOKED
#define _ASF_HRESULT_TYPEDEF_(x) x
#else
#define _ASF_HRESULT_TYPEDEF_(x) ((HRESULT)x)
#endif

#define ASF_S_OPAQUEPACKET                  _ASF_HRESULT_TYPEDEF_(0x000d07f0)

#define ASF_E_BUFFEROVERRUN                 _ASF_HRESULT_TYPEDEF_(0xc00d07d0)
#define ASF_E_BUFFERTOOSMALL                _ASF_HRESULT_TYPEDEF_(0xc00d07d1)
#define ASF_E_BADLANGUAGEID                 _ASF_HRESULT_TYPEDEF_(0xc00d07d2)
#define ASF_E_NOPAYLOADLENGTH               _ASF_HRESULT_TYPEDEF_(0xc00d07db)
#define ASF_E_TOOMANYPAYLOADS               _ASF_HRESULT_TYPEDEF_(0xc00d07dc)
#define ASF_E_PACKETCONTENTTOOLARGE         _ASF_HRESULT_TYPEDEF_(0xc00d07de)
#define ASF_E_UNKNOWNPACKETSIZE             _ASF_HRESULT_TYPEDEF_(0xc00d07e0)
#define ASF_E_INVALIDHEADER                 _ASF_HRESULT_TYPEDEF_(0xc00d07e2)
#define ASF_E_NOCLOCKOBJECT                 _ASF_HRESULT_TYPEDEF_(0xc00d07e6)
#define ASF_E_UNKNOWNCLOCKTYPE              _ASF_HRESULT_TYPEDEF_(0xc00d07eb)
#define ASF_E_OPAQUEPACKET                  _ASF_HRESULT_TYPEDEF_(0xc00d07ed)
#define ASF_E_WRONGVERSION                  _ASF_HRESULT_TYPEDEF_(0xc00d07ee)
#define ASF_E_OVERFLOW                      _ASF_HRESULT_TYPEDEF_(0xc00d07ef)
#define ASF_E_NOTFOUND                      _ASF_HRESULT_TYPEDEF_(0xc00d07f0)
#define ASF_E_OBJECTTOOBIG                  _ASF_HRESULT_TYPEDEF_(0xc00d07f3)
#define ASF_E_UNEXPECTEDVALUE               _ASF_HRESULT_TYPEDEF_(0xc00d07f4)
#define ASF_E_INVALIDSTATE                  _ASF_HRESULT_TYPEDEF_(0xc00d07f5)
#define ASF_E_NOLIBRARY                     _ASF_HRESULT_TYPEDEF_(0xc00d07f6)
#define ASF_E_ALREADYINITIALIZED            _ASF_HRESULT_TYPEDEF_(0xc00d07f7)
#define ASF_E_INVALIDINIT                   _ASF_HRESULT_TYPEDEF_(0xc00d07f8)
#define ASF_E_NOHEADEROBJECT                _ASF_HRESULT_TYPEDEF_(0xc00d07f9)
#define ASF_E_NODATAOBJECT                  _ASF_HRESULT_TYPEDEF_(0xc00d07fa)
#define ASF_E_NOINDEXOBJECT                 _ASF_HRESULT_TYPEDEF_(0xc00d07fb)
#define ASF_E_NOSTREAMPROPS                 _ASF_HRESULT_TYPEDEF_(0xc00d07fc)
#define ASF_E_NOFILEPROPS                   _ASF_HRESULT_TYPEDEF_(0xc00d07fd)
#define ASF_E_NOLANGUAGELIST                _ASF_HRESULT_TYPEDEF_(0xc00d07fe)
#define ASF_E_NOINDEXPARAMETERS             _ASF_HRESULT_TYPEDEF_(0xc00d07ff)
#define ASF_E_UNSUPPORTEDERRORCONCEALMENT   _ASF_HRESULT_TYPEDEF_(0xc00d0800)
#define ASF_E_INVALIDFLAGS                  _ASF_HRESULT_TYPEDEF_(0xc00d0801)
#define ASF_E_BADDATADESCRIPTOR             _ASF_HRESULT_TYPEDEF_(0xc00d0802)
#define ASF_E_BADINDEXINTERVAL              _ASF_HRESULT_TYPEDEF_(0xc00d0803)
#define ASF_E_INVALIDTIME                   _ASF_HRESULT_TYPEDEF_(0xc00d0804)
#define ASF_E_INVALIDINDEX                  _ASF_HRESULT_TYPEDEF_(0xc00d0805)
#define ASF_E_STREAMNUMBERINUSE             _ASF_HRESULT_TYPEDEF_(0xc00d0806)
#define ASF_E_BADMEDIATYPE                  _ASF_HRESULT_TYPEDEF_(0xc00d0807)
#define ASF_E_WRITEFAILED                   _ASF_HRESULT_TYPEDEF_(0xc00d0808)
#define ASF_E_NOTENOUGHDESCRIPTORS          _ASF_HRESULT_TYPEDEF_(0xc00d0809)
#define ASF_E_INDEXBLOCKUNLOADED            _ASF_HRESULT_TYPEDEF_(0xc00d080a)
#define ASF_E_NOTENOUGHBANDWIDTH            _ASF_HRESULT_TYPEDEF_(0xc00d080b)
#define ASF_E_EXCEEDEDMAXIMUMOBJECTSIZE     _ASF_HRESULT_TYPEDEF_(0xc00d080c)
#define ASF_E_BADDATAUNIT                   _ASF_HRESULT_TYPEDEF_(0xc00d080d)
#define ASF_E_HEADERSIZE                    _ASF_HRESULT_TYPEDEF_(0xc00d080e)

#endif
