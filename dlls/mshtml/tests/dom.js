/*
 * Copyright 2017 Jacek Caban for CodeWeavers
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

var tests = [];

sync_test("input_selection", function() {
    var input = document.createElement("input");
    input.type = "text";
    input.value = "test";
    document.body.appendChild(input);

    function test_range(start, end) {
        ok(input.selectionStart === start, "input.selectionStart = " + input.selectionStart + " expected " + start);
        ok(input.selectionEnd === end, "input.selectionEnd = " + input.selectionEnd + " expected " + end);
    }

    test_range(0, 0);

    input.selectionStart = 2;
    test_range(2, 2);

    input.selectionStart = -1;
    test_range(0, 2);

    input.selectionStart = 10;
    test_range(4, 4);

    input.selectionEnd = 2;
    test_range(2, 2);

    input.selectionEnd = -1;
    test_range(0, 0);

    input.selectionEnd = 10;
    test_range(0, 4);

    input.setSelectionRange(2, 3);
    test_range(2, 3);

    input.setSelectionRange(-1, 10);
    test_range(0, 4);

    input.setSelectionRange(3, 3);
    test_range(3, 3);
});

sync_test("textContent", function() {
    var text = document.createTextNode("test");
    ok(text.textContent === "test", "text.textContent = " + text.textContent);

    var div = document.createElement("div");
    document.body.appendChild(div);
    div.innerHTML = "abc<script>/* */</script><div>text</div>";
    ok(div.textContent === "abc/* */text", "div.textContent = " + div.textContent);

    div.textContent = "test";
    ok(div.textContent === "test", "div.textContent = " + div.textContent);
    ok(div.childNodes.length === 1, "div.childNodes.length = " + div.childNodes.length);
    ok(div.firstChild.textContent === "test", "div.firstChild.textContent = " + div.firstChild.textContent);

    div.textContent = "";
    ok(div.textContent === "", "div.textContent = " + div.textContent);
    ok(div.childNodes.length === 0, "div.childNodes.length = " + div.childNodes.length);

    div.textContent = null;
    ok(div.textContent === "", "div.textContent = " + div.textContent);
    div.textContent = 11;
    ok(div.textContent === "11", "div.textContent = " + div.textContent);
    div.textContent = 10.5;
    ok(div.textContent === "10.5", "div.textContent = " + div.textContent);

    ok(document.textContent === null, "document.textContent = " + document.textContent);
});

sync_test("ElementTraversal", function() {
    var div = document.createElement("div");
    div.innerHTML = "abc<b>bold</b><script>/* */<script><div>text</div>def";
    ok(div.firstElementChild.outerHTML === "<b>bold</b>",
            "div.firstElementChild.outerHTML = " + div.firstElementChild.outerHTML);

    div.innerHTML = "abc";
    ok(div.firstElementChild === null, "div.firstElementChild = " + div.firstElementChild);

    ok(!("firstElementChild" in document), "firstElementChild found in document");
});

sync_test("head", function() {
    var h = document.head;
    ok(h.tagName === "HEAD", "h.tagName = " + h.tagName);
    ok(h === document.getElementsByTagName("head")[0], "unexpected head element");
});

async_test("iframe", function() {
    document.body.innerHTML = '<iframe src="runscript.html?frame.js"></iframe>'
    var iframe = document.body.firstChild;

    iframe.onload = guard(function() {
        var r = iframe.contentWindow.global_object.get_global_value();
        ok(r === "global value", "get_global_value() returned " + r);

        var f = iframe.contentWindow.global_object.get_global_value;
        ok(f() === "global value", "f() returned " + f());

        next_test();
    });
});

async_test("iframe_location", function() {
    document.body.innerHTML = '<iframe src="emptyfile"></iframe>'
    var iframe = document.body.firstChild;

    iframe.onload = function() {
        ok(iframe.contentWindow.location.pathname === "/emptyfile",
           "path = " + iframe.contentWindow.location.pathname);
        iframe.onload = function () {
            ok(iframe.contentWindow.location.pathname === "/empty/file",
               "path = " + iframe.contentWindow.location.pathname);
            next_test();
        }
        iframe.src = "empty/file";
    }
});

sync_test("anchor", function() {
    var iframe = document.body.firstChild;
    var anchor = document.createElement("a");

    var anchor_tests = [
        { href: "http://www.winehq.org:123/about", protocol: "http:", host: "www.winehq.org:123" },
        { href: "https://www.winehq.org:123/about", protocol: "https:", host: "www.winehq.org:123" },
        { href: "about:blank", protocol: "about:", host: "" },
        { href: "file:///c:/dir/file.html", protocol: "file:", host: "" },
        { href: "http://www.winehq.org/about", protocol: "http:", host: "www.winehq.org:80", todo_host: true },
        { href: "https://www.winehq.org/about", protocol: "https:", host: "www.winehq.org:443", todo_host: true },
    ];

    for(var i in anchor_tests) {
        var t = anchor_tests[i];
        anchor.href = t.href;
        ok(anchor.protocol === t.protocol, "anchor(" + t.href + ").protocol = " + anchor.protocol);
        todo_wine_if("todo_host" in t).
        ok(anchor.host === t.host, "anchor(" + t.href + ").host = " + anchor.host);
    }
});

sync_test("getElementsByClassName", function() {
    var elems;

    document.body.innerHTML = '<div class="class1">'
        + '<div class="class1"></div>'
        + '<a id="class1" class="class2"></a>'
        + '</div>'
        + '<script class="class1"></script>';

    elems = document.getElementsByClassName("class1");
    ok(elems.length === 3, "elems.length = " + elems.length);
    ok(elems[0].tagName === "DIV", "elems[0].tagName = " + elems[0].tagName);
    ok(elems[1].tagName === "DIV", "elems[1].tagName = " + elems[1].tagName);
    ok(elems[2].tagName === "SCRIPT", "elems[2].tagName = " + elems[2].tagName);

    elems = document.getElementsByClassName("class2");
    ok(elems.length === 1, "elems.length = " + elems.length);
    ok(elems[0].tagName === "A", "elems[0].tagName = " + elems[0].tagName);

    elems = document.getElementsByClassName("classnotfound");
    ok(elems.length == 0, "elems.length = " + elems.length);
});

sync_test("createElementNS", function() {
    var svg_ns = "http://www.w3.org/2000/svg";
    var elem;

    elem = document.createElementNS(null, "test");
    ok(elem.tagName === "test", "elem.tagName = " + elem.tagName);
    ok(elem.namespaceURI === null, "elem.namespaceURI = " + elem.namespaceURI);

    elem = document.createElementNS(svg_ns, "test");
    ok(elem.tagName === "test", "elem.tagName = " + elem.tagName);
    ok(elem.namespaceURI === svg_ns, "elem.namespaceURI = " + elem.namespaceURI);

    elem = document.createElementNS(svg_ns, "svg");
    ok(elem.tagName === "svg", "elem.tagName = " + elem.tagName);
    ok(elem.namespaceURI === svg_ns, "elem.namespaceURI = " + elem.namespaceURI);

    elem = document.createElementNS("test", "svg");
    ok(elem.tagName === "svg", "elem.tagName = " + elem.tagName);
    ok(elem.namespaceURI === "test", "elem.namespaceURI = " + elem.namespaceURI);
});

sync_test("query_selector", function() {
    document.body.innerHTML = '<div class="class1">'
        + '<div class="class1"></div>'
        + '<a id="class1" class="class2"></a>'
        + '</div>'
        + '<script class="class1"></script>';

    var e = document.querySelector("nomatch");
    ok(e === null, "e = " + e);
    e = document.body.querySelector("nomatch");
    ok(e === null, "e = " + e);

    e = document.querySelector(".class1");
    ok(e.tagName === "DIV", "e.tagName = " + e.tagName);
    e = document.body.querySelector(".class1");
    ok(e.tagName === "DIV", "e.tagName = " + e.tagName);
    ok(e.msMatchesSelector(".class1") === true, "msMatchesSelector returned " + e.msMatchesSelector(".class1"));
    ok(e.msMatchesSelector(".class2") === false, "msMatchesSelector returned " + e.msMatchesSelector(".class2"));

    e = document.querySelector("a");
    ok(e.tagName === "A", "e.tagName = " + e.tagName);
    e = document.body.querySelector("a");
    ok(e.tagName === "A", "e.tagName = " + e.tagName);
});

sync_test("compare_position", function() {
    document.body.innerHTML = '<div><div></div><div></div></div>';

    var parent = document.body.firstChild;
    var child1 = parent.firstChild;
    var child2 = child1.nextSibling;
    var elem = document.createElement("div");

    function compare_position(node1, node2, expected_result, ignore_mask) {
        var cmp = node1.compareDocumentPosition(node2);
        ok((cmp & ~ignore_mask) == expected_result,
           "compareDocumentPosition returned " + cmp + " expected " + expected_result);
    }

    compare_position(child1, child2, 4);
    compare_position(child2, child1, 2);
    compare_position(parent, child1, 0x14);
    compare_position(parent, child2, 0x14);
    compare_position(parent, elem, 0x21, 6);
    compare_position(elem, parent, 0x21, 6);
});

sync_test("rects", function() {
    document.body.innerHTML = '<div>test</div>';
    var elem = document.body.firstChild;
    var rects = elem.getClientRects();
    var rect = elem.getBoundingClientRect();

    ok(rects.length === 1, "rect.length = " + rects.length);
    ok(rects[0].top === rect.top, "rects[0].top = " + rects[0].top + " rect.top = " + rect.top);
    ok(rects[0].bottom === rect.bottom, "rects[0].bottom = " + rects[0].bottom + " rect.bottom = " + rect.bottom);

    elem = document.createElement("style");
    rects = elem.getClientRects();
    ok(rects.length === 0, "rect.length = " + rects.length);
});

sync_test("document_owner", function() {
    var node;

    ok(document.ownerDocument === null, "ownerDocument = " + document.ownerDocument);
    ok(document.body.ownerDocument === document,
       "body.ownerDocument = " + document.body.ownerDocument);
    ok(document.documentElement.ownerDocument === document,
       "documentElement.ownerDocument = " + document.documentElement.ownerDocument);

    node = document.createElement("test");
    ok(node.ownerDocument === document, "element.ownerDocument = " + node.ownerDocument);

    node = document.createDocumentFragment();
    ok(node.ownerDocument === document, "fragment.ownerDocument = " + node.ownerDocument);

    node = document.createTextNode("test");
    ok(node.ownerDocument === document, "text.ownerDocument = " + node.ownerDocument);
});

sync_test("style_properties", function() {
    document.body.innerHTML = '<div>test</div><svg></svg>';
    var elem = document.body.firstChild;
    var style = elem.style;
    var current_style = elem.currentStyle;
    var computed_style = window.getComputedStyle(elem);
    var val;

    style.cssFloat = "left";
    ok(style.cssFloat === "left", "cssFloat = " + style.cssFloat);
    ok(style.getPropertyValue("float") === "left",
       'style.getPropertyValue("float") = ' + style.getPropertyValue("float"));
    ok(style.getPropertyValue("cssFloat") === "",
       'style.getPropertyValue("cssFloat") = ' + style.getPropertyValue("cssFloat"));

    val = style.removeProperty("float");
    ok(val === "left", "removeProperty() returned " + val);
    ok(style.cssFloat === "", "cssFloat = " + style.cssFloat);

    style.cssFloat = "left";
    val = style.removeProperty("FloaT");
    ok(val === "left", "removeProperty() returned " + val);
    ok(style.cssFloat === "", "cssFloat = " + style.cssFloat);

    style.cssFloat = "left";
    val = style.removeProperty("cssFloat");
    ok(val === "", "removeProperty() returned " + val);
    ok(style.cssFloat === "left", "cssFloat = " + style.cssFloat);
    ok(style["float"] === "left", "float = " + style["float"]);

    style.testVal = "test";
    val = style.removeProperty("testVal");
    ok(val === "", "removeProperty() returned " + val);
    ok(style.testVal === "test", "testVal = " + style.testVal);

    style["z-index"] = 1;
    ok(style.zIndex === 1, "zIndex = " + style.zIndex);
    ok(style["z-index"] === 1, "z-index = " + style["z-index"]);
    ok(style.getPropertyValue("z-index") === "1",
       'style.getPropertyValue("x-index") = ' + style.getPropertyValue("z-index"));
    ok(style.getPropertyValue("zIndex") === "",
       'style.getPropertyValue("xIndex") = ' + style.getPropertyValue("zIndex"));

    style.setProperty("border-width", "5px");
    ok(style.borderWidth === "5px", "style.borderWidth = " + style.borderWidth);

    try {
        style.setProperty("border-width", 6);
        ok(style.borderWidth === "5px", "style.borderWidth = " + style.borderWidth);
    }catch(e) {
        win_skip("skipping setProperty tests on too old IE version");
        return;
    }

    style.setProperty("border-width", "7px", "test");
    ok(style.borderWidth === "5px", "style.borderWidth = " + style.borderWidth);

    style.setProperty("border-width", "6px", "");
    ok(style.borderWidth === "6px", "style.borderWidth = " + style.borderWidth);

    style.setProperty("border-width", "7px", "important");
    ok(style.borderWidth === "7px", "style.borderWidth = " + style.borderWidth);

    style.setProperty("border-width", "8px", undefined);
    ok(style.borderWidth === "7px", "style.borderWidth = " + style.borderWidth);

    style.clip = "rect(1px 1px 10px 10px)";
    ok(style.clip === "rect(1px, 1px, 10px, 10px)", "style.clip = " + style.clip);
    ok(current_style.clip === "rect(1px, 1px, 10px, 10px)",
       "current_style.clip = " + current_style.clip);
    ok(computed_style.clip === "rect(1px, 1px, 10px, 10px)",
       "computed_style.clip = " + current_style.clip);

    style.zIndex = 2;
    ok(current_style.zIndex === 2, "current_style.zIndex = " + current_style.zIndex);
    ok(computed_style.zIndex === 2, "computed_style.zIndex = " + computed_style.zIndex);

    try {
        current_style.zIndex = 1;
        ok(false, "expected exception");
    }catch(e) {}

    try {
        computed_style.zIndex = 1;
        ok(false, "expected exception");
    }catch(e) {}

    elem = elem.nextSibling;
    computed_style = window.getComputedStyle(elem);

    elem.style.zIndex = 4;
    ok(computed_style.zIndex === 4, "computed_style.zIndex = " + computed_style.zIndex);

    window.getComputedStyle(elem, null);
});

sync_test("stylesheets", function() {
    document.body.innerHTML = '<style>.div { margin-right: 1px; }</style>';

    ok(document.styleSheets.length === 1, "document.styleSheets.length = " + document.styleSheets.length);

    var stylesheet = document.styleSheets.item(0);
    ok(stylesheet.rules.length === 1, "stylesheet.rules.length = " + stylesheet.rules.length);
    ok(typeof(stylesheet.rules.item(0)) === "object",
       "typeof(stylesheet.rules.item(0)) = " + typeof(stylesheet.rules.item(0)));

    try {
        stylesheet.rules.item(1);
        ok(false, "expected exception");
    }catch(e) {}
});

sync_test("storage", function() {
    ok(typeof(window.sessionStorage) === "object",
       "typeof(window.sessionStorage) = " + typeof(window.sessionStorage));
    ok(typeof(window.localStorage) === "object" || typeof(window.localStorage) === "unknown",
       "typeof(window.localStorage) = " + typeof(window.localStorage));
});

async_test("animation", function() {
    document.body.innerHTML =
        "<style>" +
        "  @keyframes testAnimation {0% { opacity: 0; } 100% { opacity: 1; }}" +
        "  .testAnimation { animation-name: testAnimation; animation-duration: 0.01s; }" +
        "</style>";
    var div = document.createElement("div");
    div.addEventListener("animationstart", function() {
        div.addEventListener("animationend", next_test);
    });
    document.body.appendChild(div);
    div.className = "testAnimation";
});

sync_test("navigator", function() {
    ok(typeof(window.navigator) === "object",
       "typeof(window.navigator) = " + typeof(window.navigator));

    var v = window.navigator;
    ok(v === window.navigator, "v != window.navigator");
    v.testProp = true;
    ok(window.navigator.testProp, "window.navigator.testProp = " + window.navigator.testProp);
});
