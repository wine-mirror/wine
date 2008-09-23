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


var m;

m = "abcabc".match(/ca/);
ok(typeof(m) === "object", "typeof m is not object");
ok(m.length === 1, "m.length is not 1");
ok(m["0"] === "ca", "m[0] is not \"ca\"");

m = "abcabc".match(/ab/);
ok(typeof(m) === "object", "typeof m is not object");
ok(m.length === 1, "m.length is not 1");
ok(m["0"] === "ab", "m[0] is not \"ab\"");

m = "abcabc".match(/ab/g);
ok(typeof(m) === "object", "typeof m is not object");
ok(m.length === 2, "m.length is not 1");
ok(m["0"] === "ab", "m[0] is not \"ab\"");
ok(m["1"] === "ab", "m[1] is not \"ab\"");

m = "abcabc".match(/Ab/g);
ok(typeof(m) === "object", "typeof m is not object");
ok(m === null, "m is not null");

m = "abcabc".match(/Ab/gi);
ok(typeof(m) === "object", "typeof m is not object");
ok(m.length === 2, "m.length is not 1");
ok(m["0"] === "ab", "m[0] is not \"ab\"");
ok(m["1"] === "ab", "m[1] is not \"ab\"");

m = "aaabcabc".match(/a+b/g);
ok(typeof(m) === "object", "typeof m is not object");
ok(m.length === 2, "m.length is not 1");
ok(m["0"] === "aaab", "m[0] is not \"ab\"");
ok(m["1"] === "ab", "m[1] is not \"ab\"");

m = "abcabc".match(new RegExp("ab"));
ok(typeof(m) === "object", "typeof m is not object");
ok(m.length === 1, "m.length is not 1");
ok(m["0"] === "ab", "m[0] is not \"ab\"");

/*
m = "abcabc".match(new RegExp("ab","g"));
ok(typeof(m) === "object", "typeof m is not object");
ok(m.length === 2, "m.length is not 1");
ok(m["0"] === "ab", "m[0] is not \"ab\"");
ok(m["1"] === "ab", "m[1] is not \"ab\"");

m = "abcabc".match(new RegExp(/ab/g));
ok(typeof(m) === "object", "typeof m is not object");
ok(m.length === 2, "m.length is not 1");
ok(m["0"] === "ab", "m[0] is not \"ab\"");
ok(m["1"] === "ab", "m[1] is not \"ab\"");

m = "abcabc".match(new RegExp("ab","g", "test"));
ok(typeof(m) === "object", "typeof m is not object");
ok(m.length === 2, "m.length is not 1");
ok(m["0"] === "ab", "m[0] is not \"ab\"");
ok(m["1"] === "ab", "m[1] is not \"ab\"");
*/
reportSuccess();
