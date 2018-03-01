/*
 * Copyright 2018 Jacek Caban for CodeWeavers
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

function test_date_now() {
    var now = Date.now();
    var time = (new Date()).getTime();

    ok(time >= now && time-now < 50, "unexpected Date.now() result " + now + " expected " + time);

    Date.now(1, 2, 3);

    next_test();
}

function test_indexOf() {
    function expect(array, args, exr) {
        var r = Array.prototype.indexOf.apply(array, args);
        ok(r == exr, "indexOf returned " + r + " expected " + exr);
    }

    ok(Array.prototype.indexOf.length == 1, "indexOf.length = " + Array.prototype.indexOf.length);

    expect([1,2,3], [2], 1);
    expect([1,undefined,3], [undefined], 1);
    expect([1,undefined,3], [], 1);
    expect([1,,3], [undefined], -1);
    expect([1,2,3,4,5,6], [2, 2], -1);
    expect([1,2,3,4,5,6], [5, -1], -1);
    expect([1,2,3,4,5,6], [5, -2], 4);
    expect([1,2,3,4,5,6], [5, -20], 4);
    expect([1,2,3,4,5,6], [5, 20], -1);
    expect("abc", ["b"], 1);
    expect(true, [true], -1);
    expect({"4": 4, length: 5}, [4], 4);
    expect({"4": 4, length: 5}, [undefined], -1);
    expect({"4": 4, length: 3}, [4], -1);
    expect({"test": true}, [true], -1);
    expect([1,2,3], [2, 1.9], 1);

    next_test();
}

function test_isArray() {
    function expect_array(a, exr) {
        var r = Array.isArray(a);
        ok(r === exr, "isArray returned " + r + " expected " + exr);
    }

    expect_array([1], true);
    expect_array(Array, false);
    expect_array(new Array(), true);
    expect_array({"1": 1, "2": 2, length: 2}, false);

    function C() {}
    C.prototype = Array.prototype;
    expect_array(new C(), false);

    next_test();
}

var tests = [
    test_date_now,
    test_indexOf,
    test_isArray
];
