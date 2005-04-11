/*
 * Copyright 2005 Jacek Caban
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

typedef struct {
    IHTMLDocument2Vtbl  *lpHTMLDocument2Vtbl;
    IPersistMonikerVtbl *lpPersistMonikerVtbl;
    IPersistFileVtbl    *lpPersistFileVtbl;
    IMonikerPropVtbl    *lpMonikerPropVtbl;

    ULONG ref;
} HTMLDocument;

#define HTMLDOC(x)       ((IHTMLDocument2*)     &(x)->lpHTMLDocument2Vtbl)
#define PERSIST(x)       ((IPersist*)           &(x)->lpPersistFileVtbl)
#define PERSISTMON(x)    ((IPersistMoniker*)    &(x)->lpPersistMonikerVtbl)
#define PERSISTFILE(x)   ((IPersistFile*)       &(x)->lpPersistFileVtbl)
#define MONPROP(x)       ((IMonikerProp*)       &(x)->lpMonikerPropVtbl)

HRESULT HTMLDocument_Create(IUnknown*,REFIID,void**);

void HTMLDocument_Persist_Init(HTMLDocument*);
