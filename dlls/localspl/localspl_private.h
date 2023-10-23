/*
 * Implementation of the Local Printmonitor: internal include file
 *
 * Copyright 2006 Detlef Riekenberg
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

#ifndef __WINE_LOCALSPL_PRIVATE__
#define __WINE_LOCALSPL_PRIVATE__

#include <windef.h>
#include "winternl.h"
#include "wine/unixlib.h"

extern HINSTANCE localspl_instance;

/* ## Resource-ID ## */
#define IDS_FORM_LETTER 200
#define IDS_FORM_LETTER_SMALL 201
#define IDS_FORM_TABLOID 202
#define IDS_FORM_LEDGER 203
#define IDS_FORM_LEGAL 204
#define IDS_FORM_STATEMENT 205
#define IDS_FORM_EXECUTIVE 206
#define IDS_FORM_A3 207
#define IDS_FORM_A4 208
#define IDS_FORM_A4_SMALL 209
#define IDS_FORM_A5 210
#define IDS_FORM_B4_JIS 211
#define IDS_FORM_B5_JIS 212
#define IDS_FORM_FOLIO 213
#define IDS_FORM_QUARTO 214
#define IDS_FORM_10x14 215
#define IDS_FORM_11x17 216
#define IDS_FORM_NOTE 217
#define IDS_FORM_ENVELOPE_9 218
#define IDS_FORM_ENVELOPE_10 219
#define IDS_FORM_ENVELOPE_11 220
#define IDS_FORM_ENVELOPE_12 221
#define IDS_FORM_ENVELOPE_14 222
#define IDS_FORM_C_SIZE_SHEET 223
#define IDS_FORM_D_SIZE_SHEET 224
#define IDS_FORM_E_SIZE_SHEET 225
#define IDS_FORM_ENVELOPE_DL 226
#define IDS_FORM_ENVELOPE_C5 227
#define IDS_FORM_ENVELOPE_C3 228
#define IDS_FORM_ENVELOPE_C4 229
#define IDS_FORM_ENVELOPE_C6 230
#define IDS_FORM_ENVELOPE_C65 231
#define IDS_FORM_ENVELOPE_B4 232
#define IDS_FORM_ENVELOPE_B5 233
#define IDS_FORM_ENVELOPE_B6 234
#define IDS_FORM_ENVELOPE 235
#define IDS_FORM_ENVELOPE_MONARCH 236
#define IDS_FORM_6_34_ENVELOPE 237
#define IDS_FORM_US_STD_FANFOLD 238
#define IDS_FORM_GERMAN_STD_FANFOLD 239
#define IDS_FORM_GERMAN_LEGAL_FANFOLD 240
#define IDS_FORM_B4_ISO 241
#define IDS_FORM_JAPANESE_POSTCARD 242
#define IDS_FORM_9x11 243
#define IDS_FORM_10x11 244
#define IDS_FORM_15x11 245
#define IDS_FORM_ENVELOPE_INVITE 246
#define IDS_FORM_LETTER_EXTRA 247
#define IDS_FORM_LEGAL_EXTRA 248
#define IDS_FORM_TABLOID_EXTRA 249
#define IDS_FORM_A4_EXTRA 250
#define IDS_FORM_LETTER_TRANSVERSE 251
#define IDS_FORM_A4_TRANSVERSE 252
#define IDS_FORM_LETTER_EXTRA_TRANSVERSE 253
#define IDS_FORM_SUPER_A 254
#define IDS_FORM_SUPER_B 255
#define IDS_FORM_LETTER_PLUS 256
#define IDS_FORM_A4_PLUS 257
#define IDS_FORM_A5_TRANSVERSE 258
#define IDS_FORM_B5_JIS_TRANSVERSE 259
#define IDS_FORM_A3_EXTRA 260
#define IDS_FORM_A5_EXTRA 261
#define IDS_FORM_B5_ISO_EXTRA 262
#define IDS_FORM_A2 263
#define IDS_FORM_A3_TRANSVERSE 264
#define IDS_FORM_A3_EXTRA_TRANSVERSE 265
#define IDS_FORM_JAPANESE_DOUBLE_POSTCARD 266
#define IDS_FORM_A6 267
#define IDS_FORM_JAPANESE_ENVELOPE_KAKU_2 268
#define IDS_FORM_JAPANESE_ENVELOPE_KAKU_3 269
#define IDS_FORM_JAPANESE_ENVELOPE_CHOU_3 270
#define IDS_FORM_JAPANESE_ENVELOPE_CHOU_4 271
#define IDS_FORM_LETTER_ROTATED 272
#define IDS_FORM_A3_ROTATED 273
#define IDS_FORM_A4_ROTATED 274
#define IDS_FORM_A5_ROTATED 275
#define IDS_FORM_B4_JIS_ROTATED 276
#define IDS_FORM_B5_JIS_ROTATED 277
#define IDS_FORM_JAPANESE_POSTCARD_ROTATED 278
#define IDS_FORM_DOUBLE_JAPAN_POSTCARD_ROTATED 279
#define IDS_FORM_A6_ROTATED 280
#define IDS_FORM_JAPAN_ENVELOPE_KAKU_2_ROTATED 281
#define IDS_FORM_JAPAN_ENVELOPE_KAKU_3_ROTATED 282
#define IDS_FORM_JAPAN_ENVELOPE_CHOU_3_ROTATED 283
#define IDS_FORM_JAPAN_ENVELOPE_CHOU_4_ROTATED 284
#define IDS_FORM_B6_JIS 285
#define IDS_FORM_B6_JIS_ROTATED 286
#define IDS_FORM_12x11 287
#define IDS_FORM_JAPAN_ENVELOPE_YOU_4 288
#define IDS_FORM_JAPAN_ENVELOPE_YOU_4_ROTATED 289
#define IDS_FORM_PRC_16K 290
#define IDS_FORM_PRC_32K 291
#define IDS_FORM_PRC_32K_BIG 292
#define IDS_FORM_PRC_ENVELOPE_1 293
#define IDS_FORM_PRC_ENVELOPE_2 294
#define IDS_FORM_PRC_ENVELOPE_3 295
#define IDS_FORM_PRC_ENVELOPE_4 296
#define IDS_FORM_PRC_ENVELOPE_5 297
#define IDS_FORM_PRC_ENVELOPE_6 298
#define IDS_FORM_PRC_ENVELOPE_7 299
#define IDS_FORM_PRC_ENVELOPE_8 300
#define IDS_FORM_PRC_ENVELOPE_9 301
#define IDS_FORM_PRC_ENVELOPE_10 302
#define IDS_FORM_PRC_16K_ROTATED 303
#define IDS_FORM_PRC_32K_ROTATED 304
#define IDS_FORM_PRC_32K_BIG_ROTATED 305
#define IDS_FORM_PRC_ENVELOPE_1_ROTATED 306
#define IDS_FORM_PRC_ENVELOPE_2_ROTATED 307
#define IDS_FORM_PRC_ENVELOPE_3_ROTATED 308
#define IDS_FORM_PRC_ENVELOPE_4_ROTATED 309
#define IDS_FORM_PRC_ENVELOPE_5_ROTATED 310
#define IDS_FORM_PRC_ENVELOPE_6_ROTATED 311
#define IDS_FORM_PRC_ENVELOPE_7_ROTATED 312
#define IDS_FORM_PRC_ENVELOPE_8_ROTATED 313
#define IDS_FORM_PRC_ENVELOPE_9_ROTATED 314
#define IDS_FORM_PRC_ENVELOPE_10_ROTATED 315

#define IDS_LOCALPORT       500
#define IDS_LOCALMONITOR    507

/* ## Reserved memorysize for the strings (in WCHAR) ## */
#define IDS_LOCALMONITOR_MAXLEN 64
#define IDS_LOCALPORT_MAXLEN 32

/* ## Type of Ports ## */
/* windows types */
#define PORT_IS_UNKNOWN  0
#define PORT_IS_LPT      1
#define PORT_IS_COM      2
#define PORT_IS_FILE     3
#define PORT_IS_FILENAME 4

/* wine extensions */
#define PORT_IS_WINE     5
#define PORT_IS_UNIXNAME 5
#define PORT_IS_PIPE     6
#define PORT_IS_CUPS     7
#define PORT_IS_LPR      8

struct start_doc_params
{
    unsigned int type;
    const WCHAR *port;
    const WCHAR *document_title;
    INT64 *doc;
};

struct write_doc_params
{
    INT64 doc;
    const BYTE *buf;
    unsigned int size;
};

struct end_doc_params
{
    INT64 doc;
};

#define UNIX_CALL(func, params) WINE_UNIX_CALL(unix_ ## func, params)

enum cups_funcs
{
    unix_process_attach,
    unix_start_doc,
    unix_write_doc,
    unix_end_doc,
    unix_funcs_count
};

#endif /* __WINE_LOCALSPL_PRIVATE__ */
