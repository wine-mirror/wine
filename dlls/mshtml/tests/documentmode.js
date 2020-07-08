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
    test_exposed("textContent", v >= 9);
    test_exposed("prefix", v >= 9);
    test_exposed("firstElementChild", v >= 9);
    test_exposed("onsubmit", v >= 9);
    test_exposed("getElementsByClassName", v >= 9);
    test_exposed("removeAttributeNS", v >= 9);
    test_exposed("addEventListener", v >= 9);
    test_exposed("removeEventListener", v >= 9);
    test_exposed("dispatchEvent", v >= 9);

    next_test();
}

function test_doc_props() {
    function test_exposed(prop, expect) {
        if(expect)
            ok(prop in document, prop + " not found in document.");
        else
            ok(!(prop in document), prop + " found in document.");
    }

    var v = document.documentMode;

    test_exposed("textContent", v >= 9);
    test_exposed("prefix", v >= 9);
    test_exposed("defaultView", v >= 9);
    test_exposed("head", v >= 9);
    test_exposed("addEventListener", v >= 9);
    test_exposed("removeEventListener", v >= 9);
    test_exposed("dispatchEvent", v >= 9);
    test_exposed("createEvent", v >= 9);

    test_exposed("parentWindow", true);
    if(v >= 9) ok(document.defaultView === document.parentWindow, "defaultView != parentWindow");

    next_test();
}

function test_docfrag_props() {
    var docfrag = document.createDocumentFragment();

    function test_exposed(prop, expect) {
        if(expect)
            ok(prop in docfrag, prop + " not found in document fragent.");
        else
            ok(!(prop in docfrag), prop + " found in document fragent.");
    }

    var v = document.documentMode;

    test_exposed("compareDocumentPosition", v >= 9);

    next_test();
}

function test_window_props() {
    function test_exposed(prop, expect) {
        if(expect)
            ok(prop in window, prop + " not found in window.");
        else
            ok(!(prop in window), prop + " found in window.");
    }

    var v = document.documentMode;

    test_exposed("postMessage", true);
    test_exposed("sessionStorage", true);
    test_exposed("localStorage", true);
    test_exposed("addEventListener", v >= 9);
    test_exposed("removeEventListener", v >= 9);
    test_exposed("dispatchEvent", v >= 9);
    test_exposed("getSelection", v >= 9);
    test_exposed("onfocusout", v >= 9);
    test_exposed("getComputedStyle", v >= 9);
    if(v >= 9) /* FIXME: native exposes it in all compat modes */
        test_exposed("performance", true);

    next_test();
}

function test_xhr_props() {
    var xhr = new XMLHttpRequest();

    function test_exposed(prop, expect) {
        if(expect)
            ok(prop in xhr, prop + " not found in XMLHttpRequest.");
        else
            ok(!(prop in xhr), prop + " found in XMLHttpRequest.");
    }

    var v = document.documentMode;

    test_exposed("addEventListener", v >= 9);
    test_exposed("removeEventListener", v >= 9);
    test_exposed("dispatchEvent", v >= 9);

    next_test();
}

function test_style_props() {
    var style = document.body.style;

    function test_exposed(prop, expect) {
        if(expect)
            ok(prop in style, prop + " not found in style object.");
        else
            ok(!(prop in style), prop + " found in style object.");
    }

    var v = document.documentMode;

    test_exposed("removeAttribute", true);
    test_exposed("zIndex", true);
    test_exposed("z-index", true);
    test_exposed("filter", true);
    test_exposed("pixelTop", true);
    test_exposed("float", true);
    test_exposed("css-float", false);
    test_exposed("style-float", false);
    test_exposed("setProperty", v >= 9);
    test_exposed("removeProperty", v >= 9);
    test_exposed("background-clip", v >= 9);
    test_exposed("msTransform", v >= 9);
    test_exposed("transform", v >= 10);

    style = document.body.currentStyle;

    test_exposed("zIndex", true);
    test_exposed("z-index", true);
    test_exposed("filter", true);
    test_exposed("pixelTop", false);
    test_exposed("float", true);
    test_exposed("css-float", false);
    test_exposed("style-float", false);
    test_exposed("setProperty", v >= 9);
    test_exposed("removeProperty", v >= 9);
    test_exposed("background-clip", v >= 9);
    test_exposed("transform", v >= 10);

    if(window.getComputedStyle) {
        style = window.getComputedStyle(document.body);

        test_exposed("removeAttribute", false);
        test_exposed("zIndex", true);
        test_exposed("z-index", true);
        test_exposed("pixelTop", false);
        test_exposed("float", true);
        test_exposed("css-float", false);
        test_exposed("style-float", false);
        test_exposed("setProperty", v >= 9);
        test_exposed("removeProperty", v >= 9);
        test_exposed("background-clip", v >= 9);
        test_exposed("transform", v >= 10);
    }

    next_test();
}

function test_javascript() {
    var g = window;

    function test_exposed(func, obj, expect) {
        if(expect)
            ok(func in obj, func + " not found in " + obj);
        else
            ok(!(func in obj), func + " found in " + obj);
    }

    function test_parses(code, expect) {
        var success;
        try {
            eval(code);
            success = true;
        }catch(e) {
            success = false;
        }
        if(expect)
            ok(success === true, code + " did not parse");
        else
            ok(success === false, code + " parsed");
    }

    var v = document.documentMode;

    test_exposed("ScriptEngineMajorVersion", g, true);

    test_exposed("JSON", g, v >= 8);
    test_exposed("now", Date, true);
    test_exposed("toISOString", Date.prototype, v >= 9);
    test_exposed("isArray", Array, v >= 9);
    test_exposed("forEach", Array.prototype, v >= 9);
    test_exposed("indexOf", Array.prototype, v >= 9);
    test_exposed("trim", String.prototype, v >= 9);
    test_exposed("map", Array.prototype, v >= 9);

    /* FIXME: IE8 implements weird semi-functional property descriptors. */
    if(v != 8) {
        test_exposed("getOwnPropertyDescriptor", Object, v >= 8);
        test_exposed("defineProperty", Object, v >= 8);
        test_exposed("defineProperties", Object, v >= 8);
    }

    test_exposed("getPrototypeOf", Object, v >= 9);

    test_parses("if(false) { o.default; }", v >= 9);
    test_parses("if(false) { o.with; }", v >= 9);
    test_parses("if(false) { o.if; }", v >= 9);

    next_test();
}

function test_elem_by_id() {
    document.body.innerHTML = '<form id="testid" name="testname"></form>';

    var id_elem = document.getElementById("testid");
    ok(id_elem.tagName === "FORM", "id_elem.tagName = " + id_elem.tagName);

    var name_elem = document.getElementById("testname");
    if(document.documentMode < 8)
        ok(id_elem === name_elem, "id_elem != id_elem");
    else
        ok(name_elem === null, "name_elem != null");

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

function test_iframe_doc_mode() {
    var iframe = document.createElement("iframe");

    iframe.onload = function() {
        var iframe_mode = iframe.contentWindow.document.documentMode;
        if(document.documentMode < 9)
            ok(iframe_mode === 5, "iframe_mode = " + iframe_mode);
        else
            ok(iframe_mode === document.documentMode, "iframe_mode = " + iframe_mode);
        next_test();
    }

    iframe.src = "about:blank";
    document.body.appendChild(iframe);
}

function test_conditional_comments() {
    var div = document.createElement("div");
    document.body.appendChild(div);

    function test_version(v) {
        var version = compat_version ? compat_version : 7;

        div.innerHTML = "<!--[if lte IE " + v + "]>true<![endif]-->";
        ok(div.innerText === (version <= v ? "true" : ""),
           "div.innerText = " + div.innerText + " for version (<=) " + v);

        div.innerHTML = "<!--[if lt IE " + v + "]>true<![endif]-->";
        ok(div.innerText === (version < v ? "true" : ""),
           "div.innerText = " + div.innerText + " for version (<) " + v);

        div.innerHTML = "<!--[if gte IE " + v + "]>true<![endif]-->";
        ok(div.innerText === (version >= v && version < 10 ? "true" : ""),
           "div.innerText = " + div.innerText + " for version (>=) " + v);

        div.innerHTML = "<!--[if gt IE " + v + "]>true<![endif]-->";
        ok(div.innerText === (version > v && version < 10 ? "true" : ""),
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
    test_iframe_doc_mode,
    test_elem_props,
    test_doc_props,
    test_docfrag_props,
    test_window_props,
    test_javascript,
    test_xhr_props,
    test_style_props,
    test_elem_by_id,
    test_conditional_comments
];
