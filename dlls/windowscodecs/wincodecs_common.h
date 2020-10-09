/*
 * Copyright 2020 Esme Povirk
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

HRESULT CDECL decoder_initialize(struct decoder *decoder, IStream *stream, struct decoder_stat *st)
{
    return decoder->vtable->initialize(decoder, stream, st);
}

HRESULT CDECL decoder_get_frame_info(struct decoder *decoder, UINT frame, struct decoder_frame *info)
{
    return decoder->vtable->get_frame_info(decoder, frame, info);
}

void CDECL decoder_destroy(struct decoder *decoder)
{
    decoder->vtable->destroy(decoder);
}
