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
    const IHTMLDocument2Vtbl          *lpHTMLDocument2Vtbl;
    const IPersistMonikerVtbl         *lpPersistMonikerVtbl;
    const IPersistFileVtbl            *lpPersistFileVtbl;
    const IMonikerPropVtbl            *lpMonikerPropVtbl;
    const IOleObjectVtbl              *lpOleObjectVtbl;
    const IOleDocumentVtbl            *lpOleDocumentVtbl;
    const IOleDocumentViewVtbl        *lpOleDocumentViewVtbl;
    const IOleInPlaceActiveObjectVtbl *lpOleInPlaceActiveObjectVtbl;

    ULONG ref;

    IOleClientSite *client;
    IOleInPlaceSite *ipsite;
    IOleInPlaceFrame *frame;

    HWND hwnd;
} HTMLDocument;

#define HTMLDOC(x)       ((IHTMLDocument2*)          &(x)->lpHTMLDocument2Vtbl)
#define PERSIST(x)       ((IPersist*)                &(x)->lpPersistFileVtbl)
#define PERSISTMON(x)    ((IPersistMoniker*)         &(x)->lpPersistMonikerVtbl)
#define PERSISTFILE(x)   ((IPersistFile*)            &(x)->lpPersistFileVtbl)
#define MONPROP(x)       ((IMonikerProp*)            &(x)->lpMonikerPropVtbl)
#define OLEOBJ(x)        ((IOleObject*)              &(x)->lpOleObjectVtbl)
#define OLEDOC(x)        ((IOleDocument*)            &(x)->lpOleDocumentVtbl)
#define DOCVIEW(x)       ((IOleDocumentView*)        &(x)->lpOleDocumentViewVtbl)
#define ACTOBJ(x)        ((IOleInPlaceActiveObject*)  &(x)->lpOleInPlaceActiveObjectVtbl)

HRESULT HTMLDocument_Create(IUnknown*,REFIID,void**);

void HTMLDocument_Persist_Init(HTMLDocument*);
void HTMLDocument_OleObj_Init(HTMLDocument*);
void HTMLDocument_View_Init(HTMLDocument*);

extern HINSTANCE hInst;
