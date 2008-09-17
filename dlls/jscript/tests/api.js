/*
 * Copyright 2008 Jacek Caban for CodeWeavers
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

ok("".length === 0, "\"\".length = " + "".length);
ok(getVT("".length) == "VT_I4", "\"\".length = " + "".length);
ok("abc".length === 3, "\"abc\".length = " + "abc".length);
ok(String.prototype.length === 0, "String.prototype.length = " + String.prototype.length);

var arr = new Array();
ok(typeof(arr) === "object", "arr () is not object");
ok((arr.length === 0), "arr.length is not 0");
ok(arr["0"] === undefined, "arr[0] is not undefined");

var arr = new Array(1, 2, "test");
ok(typeof(arr) === "object", "arr (1,2,test) is not object");
ok((arr.length === 3), "arr.length is not 3");
ok(arr["0"] === 1, "arr[0] is not 1");
ok(arr["1"] === 2, "arr[1] is not 2");
ok(arr["2"] === "test", "arr[2] is not \"test\"");

arr["7"] = true;
ok((arr.length === 8), "arr.length is not 8");

var arr = new Array(6);
ok(typeof(arr) === "object", "arr (6) is not object");
ok((arr.length === 6), "arr.length is not 6");
ok(arr["0"] === undefined, "arr[0] is not undefined");

reportSuccess();
