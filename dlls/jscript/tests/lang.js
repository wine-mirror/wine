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

ok(true, "true is not true?");
ok(!false, "!false is not true");
ok(!undefined, "!undefined is not true");
ok(!null, "!null is not true");
ok(!0, "!0 is not true");
ok(!0.0, "!0.0 is not true");

ok(1 === 1, "1 === 1 is false");
ok(!(1 === 2), "!(1 === 2) is false");
ok(1.0 === 1, "1.0 === 1 is false");
ok("abc" === "abc", "\"abc\" === \"abc\" is false");
ok(true === true, "true === true is false");
ok(null === null, "null === null is false");
ok(undefined === undefined, "undefined === undefined is false");
ok(!(undefined === null), "!(undefined === null) is false");

ok(1 !== 2, "1 !== 2 is false");
ok(null !== undefined, "null !== undefined is false");

var trueVar = true;
ok(trueVar, "trueVar is not true");

reportSuccess();
