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

var tests = [
    test_input_selection
];
