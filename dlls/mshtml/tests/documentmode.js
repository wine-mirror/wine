/*
 * Copyright 2016 Jacek Caban for CodeWeavers
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

var compat_version;

function test_elem_props() {
    var elem = document.documentElement;

    function test_exposed(prop, expect) {
        if(expect)
            ok(prop in elem, prop + " not found in element.");
        else
            ok(!(prop in elem), prop + " found in element.");
    }

    var v = document.documentMode;

    test_exposed("doScroll", v < 11);
    test_exposed("querySelectorAll", v >= 8);

    next_test();
}

function test_doc_mode() {
    compat_version = parseInt(document.location.search.substring(1));

    trace("Testing compatibility mode " + compat_version);

    if(compat_version > 6 && compat_version > document.documentMode) {
        win_skip("Document mode not supported (expected " + compat_version + " got " + document.documentMode + ")");
        reportSuccess();
        return;
    }

    ok(Math.max(compat_version, 5) === document.documentMode, "documentMode = " + document.documentMode);

    if(document.documentMode > 5)
        ok(document.compatMode === "CSS1Compat", "document.compatMode = " + document.compatMode);
    else
        ok(document.compatMode === "BackCompat", "document.compatMode = " + document.compatMode);

    next_test();
}

function test_conditional_comments() {
    var div = document.createElement("div");
    document.body.appendChild(div);

    function test_version(v) {
        var version = compat_version ? compat_version : 7;

        /* Uncomment and fix tests below once we support that. */
        if(version >= 9)
            return;

        div.innerHTML = "<!--[if lte IE " + v + "]>true<![endif]-->";
        ok(div.innerText === (version <= v ? "true" : ""),
           "div.innerText = " + div.innerText + " for version (<=) " + v);

        div.innerHTML = "<!--[if lt IE " + v + "]>true<![endif]-->";
        ok(div.innerText === (version < v ? "true" : ""),
           "div.innerText = " + div.innerText + " for version (<) " + v);

        div.innerHTML = "<!--[if gte IE " + v + "]>true<![endif]-->";
        ok(div.innerText === (version >= v ? "true" : ""),
           "div.innerText = " + div.innerText + " for version (>=) " + v);

        div.innerHTML = "<!--[if gt IE " + v + "]>true<![endif]-->";
        ok(div.innerText === (version > v ? "true" : ""),
           "div.innerText = " + div.innerText + " for version (>) " + v);
    }

    test_version(5);
    test_version(6);
    test_version(7);
    test_version(8);

    next_test();
}

var tests = [
    test_doc_mode,
    test_elem_props,
    test_conditional_comments
];
