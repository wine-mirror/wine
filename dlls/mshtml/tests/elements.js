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

function test_input_selection() {
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

    next_test();
}

function test_textContent() {
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

    ok(document.textContent === null, "document.textContent = " + document.textContent);

    next_test();
}

function test_ElementTraversal() {
    var div = document.createElement("div");
    div.innerHTML = "abc<b>bold</b><script>/* */<script><div>text</div>def";
    ok(div.firstElementChild.outerHTML === "<b>bold</b>",
            "div.firstElementChild.outerHTML = " + div.firstElementChild.outerHTML);

    div.innerHTML = "abc";
    ok(div.firstElementChild === null, "div.firstElementChild = " + div.firstElementChild);

    ok(!("firstElementChild" in document), "firstElementChild found in document");

    next_test();
}

function test_head() {
    var h = document.head;
    ok(h.tagName === "HEAD", "h.tagName = " + h.tagName);
    ok(h === document.getElementsByTagName("head")[0], "unexpected head element");

    next_test();
}

function test_iframe() {
    document.body.innerHTML = '<iframe src="runscript.html?frame.js"></iframe>'
    var iframe = document.body.firstChild;

    iframe.onload = guard(function() {
        var r = iframe.contentWindow.global_object.get_global_value();
        ok(r === "global value", "get_global_value() returned " + r);

        var f = iframe.contentWindow.global_object.get_global_value;
        ok(f() === "global value", "f() returned " + f());

        next_test();
    });
}

function test_getElementsByClassName() {
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

    next_test();
}

function test_query_selector() {
    document.body.innerHTML = '<div class="class1">'
        + '<div class="class1"></div>'
        + '<a id="class1" class="class2"></a>'
        + '</div>'
        + '<script class="class1"></script>';

    var e = document.querySelector("nomatch");
    ok(e === null, "e = " + e);

    e = document.querySelector(".class1");
    ok(e.tagName === "DIV", "e.tagName = " + e.tagName);

    e = document.querySelector("a");
    ok(e.tagName === "A", "e.tagName = " + e.tagName);

    next_test();
}

function test_compare_position() {
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

    next_test();
}

function test_document_owner() {
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

    next_test();
}

var tests = [
    test_input_selection,
    test_textContent,
    test_ElementTraversal,
    test_getElementsByClassName,
    test_head,
    test_iframe,
    test_query_selector,
    test_compare_position,
    test_document_owner
];
