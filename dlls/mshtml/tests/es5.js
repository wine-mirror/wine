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

var JS_E_PROP_DESC_MISMATCH = 0x800a01bd;
var JS_E_INVALID_WRITABLE_PROP_DESC = 0x800a13ac;
var JS_E_NONCONFIGURABLE_REDEFINED = 0x800a13d6;
var JS_E_NONWRITABLE_MODIFIED = 0x800a13d7;

function test_date_now() {
    var now = Date.now();
    var time = (new Date()).getTime();

    ok(time >= now && time-now < 50, "unexpected Date.now() result " + now + " expected " + time);

    Date.now(1, 2, 3);

    next_test();
}

function test_toISOString() {
    function expect(date, expected) {
        var s = date.toISOString();
        ok(s === expected, "toISOString returned " + s + " expected " + expected);
    }

    function expect_exception(func) {
        try {
            func();
        }catch(e) {
            return;
        }
        ok(false, "expected exception");
    }

    expect(new Date(0), "1970-01-01T00:00:00.000Z");
    expect(new Date(0xdeadbeef), "1970-02-13T05:45:28.559Z");
    expect(new Date(10928309128301), "2316-04-22T01:25:28.301Z");
    expect(new Date(-1), "1969-12-31T23:59:59.999Z");
    expect(new Date(-62167219200000), "0000-01-01T00:00:00.000Z");
    expect(new Date(-62167219200001), "-000001-12-31T23:59:59.999Z");
    expect(new Date(-6216721920000100), "-195031-12-03T23:59:59.900Z");
    expect(new Date(1092830912830100), "+036600-06-07T22:27:10.100Z");

    expect_exception(function() { new Date(NaN).toISOString(); });
    expect_exception(function() { new Date(31494784780800001).toISOString(); });

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

function test_identifier_keywords() {
    var o = {
        if: 1,
        default: 2,
        function: 3,
        break: true,
        case: true,
        catch: true,
        continue: true,
        delete: true,
        do: true,
        else: true,
        finally: true,
        for: true,
        in: true,
        instanceof: true,
        new: true,
        return: true,
        switch: true,
        throw: true,
        try: true,
        typeof: true,
        var: true,
        void: true,
        while: true,
        with: true,
        true: true,
        false: true,
        null: true,
        this: true
    };
    function ro() { return o; };

    ok(o.if === 1, "o.if = " + o.if);
    ok(ro().default === 2, "ro().default = " + ro().default);
    ok(o.false === true, "o.false = " + o.false);

    next_test();
}

function test_own_data_prop_desc(obj, prop, expected_writable, expected_enumerable,
                            expected_configurable) {
    var desc = Object.getOwnPropertyDescriptor(obj, prop);
    ok("value" in desc, "value is not in desc");
    ok(desc.value === obj[prop], "desc.value = " + desc.value + " expected " + obj[prop]);
    ok(desc.writable === expected_writable, "desc(" + prop + ").writable = " + desc.writable
       + " expected " + expected_writable);
    ok(desc.enumerable === expected_enumerable, "desc.enumerable = " + desc.enumerable
       + " expected " + expected_enumerable);
    ok(desc.configurable === expected_configurable, "desc.configurable = " + desc.configurable
       + " expected " + expected_configurable);
}

function test_getOwnPropertyDescriptor() {
    var obj;

    obj = { test: 1 };
    test_own_data_prop_desc(obj, "test", true, true, true);

    test_own_data_prop_desc(Object, "getOwnPropertyDescriptor", true, false, true);
    test_own_data_prop_desc(Math, "PI", false, false, false);

    var obj = new String();
    test_own_data_prop_desc(obj, "length", false, false, false);
    ok(Object.getOwnPropertyDescriptor(obj, "slice") === undefined,
       "getOwnPropertyDescriptor(slice) did not return undefined");
    test_own_data_prop_desc(String.prototype, "slice", true, false, true);

    obj = new Array();
    test_own_data_prop_desc(obj, "length", true, false, false);

    obj = new Function();
    test_own_data_prop_desc(obj, "length", false, false, false);
    test_own_data_prop_desc(obj, "arguments", false, false, false);

    obj = /test/;
    test_own_data_prop_desc(obj, "global", false, false, false);
    test_own_data_prop_desc(obj, "ignoreCase", false, false, false);
    test_own_data_prop_desc(obj, "lastIndex", true, false, false);
    test_own_data_prop_desc(obj, "multiline", false, false, false);
    test_own_data_prop_desc(obj, "source", false, false, false);

    next_test();
}

function test_defineProperty() {
    function expect_exception(func, expected_number) {
        try {
            func();
        }catch(e) {
            var n = e.number + 0x100000000; /* make it unsigned like HRESULT */
            todo_wine_if(expected_number == JS_E_PROP_DESC_MISMATCH).
            ok(n === expected_number, "got exception " + n.toString(16)
               + " expected " + expected_number.toString(16));
            ok(e.name === "TypeError", "e.name = " + e.name);
            return;
        }
        ok(false, "expected exception");
    }

    var obj = new Object();
    Object.defineProperty(obj, "test", {});
    ok("test" in obj, "test is not in obj");
    test_own_data_prop_desc(obj, "test", false, false, false);
    ok(obj.test === undefined, "obj.test = " + obj.test);

    Object.defineProperty(obj, "all", {writable: true, enumerable: true, configurable: true, value: 1});
    test_own_data_prop_desc(obj, "all", true, true, true);
    ok(obj.all === 1, "obj.test = " + obj.test);

    Object.defineProperty(obj, "all", {writable: false});
    test_own_data_prop_desc(obj, "all", false, true, true);
    ok(obj.all === 1, "obj.test = " + obj.test);

    Object.defineProperty(obj, "notConf", {writable: true, enumerable: true, configurable: false, value: 1});
    test_own_data_prop_desc(obj, "notConf", true, true, false);

    expect_exception(function() {
        Object.defineProperty(obj, "notConf",
                              {writable: true, enumerable: true, configurable: true, value: 1});
    }, JS_E_NONCONFIGURABLE_REDEFINED);

    expect_exception(function() {
        Object.defineProperty(obj, "notConf",
                              {writable: true, enumerable: false, configurable: false, value: 1});
    }, JS_E_NONCONFIGURABLE_REDEFINED);

    Object.defineProperty(obj, "notConf",
                          {writable: true, enumerable: true, configurable: false, value: 2});
    test_own_data_prop_desc(obj, "notConf", true, true, false);

    Object.defineProperty(obj, "notConf", {writable: true, value: 2});
    test_own_data_prop_desc(obj, "notConf", true, true, false);

    Object.defineProperty(obj, "notConf2",
                          {writable: false, enumerable: false, configurable: false, value: 1});
    test_own_data_prop_desc(obj, "notConf2", false, false, false);
    obj.notConf2 = 2;
    ok(obj.notConf2 === 1, "obj.notConf2 = " + obj.notConf2)

    expect_exception(function() {
        Object.defineProperty(obj, "notConf2",
                              {writable: false, enumerable: false, configurable: true, value: 1});
    }, JS_E_NONCONFIGURABLE_REDEFINED);

    expect_exception(function() {
        Object.defineProperty(obj, "notConf2",
                              {writable: false, enumerable: true, configurable: false, value: 1});
    }, JS_E_NONCONFIGURABLE_REDEFINED);

    expect_exception(function() {
        Object.defineProperty(obj, "notConf2", {writable: true, value: 1});
    }, JS_E_NONWRITABLE_MODIFIED);

    expect_exception(function() {
        Object.defineProperty(obj, "notConf2", {value: 2});
    }, JS_E_NONWRITABLE_MODIFIED);

    Object.defineProperty(obj, "notConf2",
                          {writable: false, enumerable: false, configurable: false, value: 1});
    test_own_data_prop_desc(obj, "notConf2", false, false, false);

    Object.defineProperty(obj, "notConf2", {writable: false, value: 1});
    test_own_data_prop_desc(obj, "notConf2", false, false, false);

    expect_exception(function() {
        Object.defineProperty(obj, "invaliddesc", {get: undefined, value: 1});
    }, JS_E_PROP_DESC_MISMATCH);

    expect_exception(function() {
        Object.defineProperty(obj, "invaliddesc", {set: undefined, writable: true});
    }, JS_E_INVALID_WRITABLE_PROP_DESC);

    next_test();
}

function test_global_properties() {
    var o;

    /* Make sure that global properties are not writable. */
    o = NaN;
    NaN = 1;
    ok(isNaN(NaN), "NaN = " + NaN);
    o = undefined;
    undefined = 1;
    ok(undefined === o, "NaN = " + NaN);
    o = Infinity;
    Infinity = 1;
    ok(Infinity === o, "Infinity = " + NaN);

    next_test();
}

var tests = [
    test_date_now,
    test_toISOString,
    test_indexOf,
    test_isArray,
    test_identifier_keywords,
    test_getOwnPropertyDescriptor,
    test_defineProperty,
    test_global_properties
];
