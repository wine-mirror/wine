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

var tests = [];

sync_test("date_now", function() {
    var now = Date.now();
    var time = (new Date()).getTime();

    ok(time >= now && time-now < 50, "unexpected Date.now() result " + now + " expected " + time);

    Date.now(1, 2, 3);
});

sync_test("toISOString", function() {
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
});

sync_test("indexOf", function() {
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
});

sync_test("forEach", function() {
    ok(Array.prototype.forEach.length === 1, "forEach.length = " + Array.prototype.forEach.length);

    function test(array, expect) {
        var r = Array.prototype.forEach.call(array, function(value, index, arr) {
            ok(arr === array, "unexpected array " + arr);
            ok(index === expect[0][0], "index = " + index + " expected " + expect[0][0]);
            ok(value === expect[0][1], "value = " + value + " expected " + expect[0][1]);
            expect.shift();
        });
        ok(r === undefined, "forEach returned " + r);
        ok(expect.length === 0, "too few forEach() calls, expected " + expect.length + " more");
    }

    test(["a",2,"c"], [[0,"a"],[1,2],[2,"c"]]);
    test({length: 1000, 500: false, c: 30, 3: "x", 999: 1}, [[3,"x"],[500,false],[999,1]]);
    test(new String("abc"), [[0,"a"],[1,"b"],[2,"c"]]);
    test([], []);

    [1,2].forEach(function() {
        ok(this === window, "this != window");
    });
    [1,2].forEach(function() {
        ok(this === window, "this != window");
    }, undefined);

    var o = new Object(), a = [1,2];
    a.forEach(function(value, index, array) {
        ok(array === a, "array != a");
        ok(this === o, "this != o");
    }, o);
});

sync_test("isArray", function() {
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
});

sync_test("array_map", function() {
    var calls, m, arr, ctx;

    /* basic map call with context */
    calls = "";
    arr = [1,2,3];
    ctx = {};
    m = arr.map(function(x, i, a) {
        ok(this === ctx, "this != ctx");
        ok(i === x - 1, "i = " + i);
        ok(a === arr, "a != arr");
        calls += x + ",";
        return x * 2;
    }, ctx);
    ok(calls === "1,2,3,", "calls = " + calls);
    ok(m.join() === "2,4,6", "m = " + m);

    /* non-array object as this argument */
    calls = "";
    arr = { 1: "one", 2: "two", 3: "three", length: 3 };
    m = Array.prototype.map.call(arr, function(x, i) {
        calls += i + ":" + x + ",";
        return x + "!";
    });
    ok(calls === "1:one,2:two,", "calls = " + calls);
    ok(m.join() === ",one!,two!", "m = " + m);
    ok(!("0" in m), "0 is in m");

    /* mutate array in callback */
    calls = "";
    arr = [1,2,3];
    m = Array.prototype.map.call(arr, function(x, i) {
        calls += i + ":" + x + ",";
        for(var j = i; j < arr.length; j++)
            arr[j]++;
        arr.push(i * i);
        return x - 1;
    });
    ok(calls === "0:1,1:3,2:5,", "calls = " + calls);
    ok(m.join() === "0,2,4", "m = " + m);

    [1,2].map(function() {
        ok(this === window, "this != window");
    });
    [1,2].map(function() {
        ok(this === window, "this != window");
    }, undefined);
});

sync_test("array_sort", function() {
    var r;

    r = [3,1,2].sort(function(x,y) { return y-x; }, 1, 2, 3, true, undefined ).join();
    ok(r === "3,2,1", "reverse sorted [3,1,2] = " + r);

    r = [3,1,2].sort(undefined).join();
    ok(r === "1,2,3", "null sorted [3,1,2] = " + r);

    try {
        r = [3,1,2].sort(null);
        ok(false, "expected sort(null) exception");
    }catch(e) {
        ok(e.name === "TypeError", "got exception " + e.name);
    }
});

sync_test("identifier_keywords", function() {
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
});

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

sync_test("getOwnPropertyDescriptor", function() {
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

    (function() {
        test_own_data_prop_desc(arguments, "length", true, false, true);
        test_own_data_prop_desc(arguments, "callee", true, false, true);
    })();

    test_own_data_prop_desc(String, "prototype", false, false, false);
    test_own_data_prop_desc(function(){}, "prototype", true, false, false);
    test_own_data_prop_desc(Function, "prototype", false, false, false);
    test_own_data_prop_desc(String.prototype, "constructor", true, false, true);
});

sync_test("defineProperty", function() {
    function test_accessor_prop_desc(obj, prop, orig_desc) {
        var expected_enumerable = "enumerable" in orig_desc && !!orig_desc.enumerable;
        var expected_configurable = "configurable" in orig_desc && !!orig_desc.configurable;

        var desc = Object.getOwnPropertyDescriptor(obj, prop);
        ok(desc.enumerable === expected_enumerable, "desc.enumerable = " + desc.enumerable
           + " expected " + expected_enumerable);
        ok(desc.configurable === expected_configurable, "desc.configurable = " + desc.configurable
           + " expected " + expected_configurable);
        ok(desc.get === orig_desc.get, "desc.get = " + desc.get);
        ok(desc.set === orig_desc.set, "desc.set = " + desc.set);
    }

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

    var obj = new Object(), defined;
    defined = Object.defineProperty(obj, "test", {});
    ok(defined === obj, "defined != obj");
    ok("test" in obj, "test is not in obj");
    test_own_data_prop_desc(obj, "test", false, false, false);
    ok(obj.test === undefined, "obj.test = " + obj.test);

    Object.defineProperty(obj, "all", {writable: true, enumerable: true, configurable: true, value: 1});
    test_own_data_prop_desc(obj, "all", true, true, true);
    ok(obj.all === 1, "obj.test = " + obj.test);

    Object.defineProperty(obj, "all", {writable: false});
    test_own_data_prop_desc(obj, "all", false, true, true);
    ok(obj.all === 1, "obj.test = " + obj.test);

    var getsetprop_value = 1;
    var desc = {
        get: function() {
            return getsetprop_value;
        },
        set: function(v) {
            getsetprop_value = v;
        }
    };
    Object.defineProperty(obj, "getsetprop", desc);
    test_accessor_prop_desc(obj, "getsetprop", desc);

    ok(obj.getsetprop === 1, "getsetprop = " + obj.getsetprop);
    obj.getsetprop = 2;
    ok(getsetprop_value === 2, "getsetprop_value = " + getsetprop_value);
    test_accessor_prop_desc(obj, "getsetprop", desc);
    ok(obj.getsetprop === 2, "getsetprop = " + obj.getsetprop);

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

    desc = {
        get: function() {
            return getsetprop_value;
        },
        set: function(v) {
            getsetprop_value = v;
        },
        configurable: false
    };
    Object.defineProperty(obj, "notConfAcc", desc);
    test_accessor_prop_desc(obj, "notConfAcc", desc);

    expect_exception(function() {
        Object.defineProperty(obj, "notConfAcc", {value: 1});
    }, JS_E_NONCONFIGURABLE_REDEFINED);

    expect_exception(function() {
        Object.defineProperty(obj, "notConfAcc", {get: desc.get, set: function () {}});
    }, JS_E_NONCONFIGURABLE_REDEFINED);

    expect_exception(function() {
        Object.defineProperty(obj, "notConfAcc", {get: undefined, set: desc.set});
    }, JS_E_NONCONFIGURABLE_REDEFINED);

    expect_exception(function() {
        Object.defineProperty(obj, "notConfAcc", {writable: true});
    }, JS_E_NONCONFIGURABLE_REDEFINED);

    Object.defineProperty(obj, "notConfAcc", {get: desc.get});
    test_accessor_prop_desc(obj, "notConfAcc", desc);

    Object.defineProperty(obj, "notConfAcc", {set: desc.set});
    test_accessor_prop_desc(obj, "notConfAcc", desc);

    Object.defineProperty(obj, "notConfAcc", {configurable: false});
    test_accessor_prop_desc(obj, "notConfAcc", desc);

    desc = {
        get: function() {
            return getsetprop_value;
        },
        set: function(v) {
            getsetprop_value = v;
        },
        configurable: true
    };
    Object.defineProperty(obj, "confAcc", desc);
    test_accessor_prop_desc(obj, "confAcc", desc);

    Object.defineProperty(obj, "confAcc", {writable: 1});
    test_own_data_prop_desc(obj, "confAcc", true, false, true);

    Object.defineProperty(obj, "confAcc", desc);
    test_accessor_prop_desc(obj, "confAcc", desc);

    desc.get = function() {};
    desc.set = undefined;
    Object.defineProperty(obj, "confAcc", desc);
    test_accessor_prop_desc(obj, "confAcc", desc);

    expect_exception(function() {
        Object.defineProperty(obj, "invaliddesc", {get: undefined, value: 1});
    }, JS_E_PROP_DESC_MISMATCH);

    expect_exception(function() {
        Object.defineProperty(obj, "invaliddesc", {set: undefined, writable: true});
    }, JS_E_INVALID_WRITABLE_PROP_DESC);

    function child() {}
    desc = {
        get: function() {
            ok(this === obj, "this != obj");
            return getsetprop_value;
        },
        set: function(v) {
            ok(this === obj, "this != obj");
            getsetprop_value = v;
        },
        configurable: true
    };
    Object.defineProperty(child.prototype, "parent_accessor", desc);

    obj = new child();
    getsetprop_value = 6;
    ok(obj.parent_accessor === 6, "parent_accessor = " + obj.parent_accessor);
    obj.parent_accessor = 1;
    ok(getsetprop_value === 1, "getsetprop_value = " + getsetprop_value);
    ok(obj.parent_accessor === 1, "parent_accessor = " + obj.parent_accessor);

    ok(Object.getOwnPropertyDescriptor(obj, "parent_accessor") === undefined,
       "getOwnPropertyDescriptor(parent_accessor) did not return undefined");
    test_accessor_prop_desc(child.prototype, "parent_accessor", desc);

    desc.get = undefined;
    Object.defineProperty(child.prototype, "parent_accessor", desc);
    ok(obj.parent_accessor === undefined, "parent_accessor = " + obj.parent_accessor);

    /* no setter */
    desc = {
        get: function() {
            ok(this === obj, "this != obj");
            return true;
        },
        configurable: true
    };
    Object.defineProperty(obj, "no_setter", desc);
    test_accessor_prop_desc(obj, "no_setter", desc);
    obj.no_setter = false;
    ok(obj.no_setter === true, "no_setter = " + obj.no_setter);

    /* call prop with getter */
    desc = {
        get: function() {
            return function(x) {
                ok(x === 100, "x = " + x);
                return 10;
            };
        }
    };
    Object.defineProperty(obj, "funcprop", desc);
    test_accessor_prop_desc(obj, "funcprop", desc);
    ok(obj.funcprop(100) === 10, "obj.funcprop() = " + obj.funcprop(100));
});

sync_test("defineProperties", function() {
    var o, defined, descs;

    descs = {
        acc_prop: { get: function() { return 1; } },
        prop: { value: 2 },
        e: { enumerable: true }
    };

    o = new Object();
    defined = Object.defineProperties(o, descs);
    ok(defined === o, "defined != o");
    ok(o.acc_prop === 1, "o.acc_prop = " + o.acc_prop);
    ok(o.prop === 2, "o.prop = " + o.prop);
    ok(o.e === undefined, "o.e = " + o.e);
    ok("e" in o, "e is not in o");
    test_own_data_prop_desc(o, "prop", false, false, false);
    test_own_data_prop_desc(o, "e", false, true, false);
    for(var p in o) ok(p === "e", "p = " + p);

    o = new Object();
    Object.defineProperties(o, 1);
    for(var p in o) ok(false, "o has property " + p);

    o = Object.create(null, descs);
    ok(o.acc_prop === 1, "o.acc_prop = " + o.acc_prop);
    ok(o.prop === 2, "o.prop = " + o.prop);
    ok(o.e === undefined, "o.e = " + o.e);
    ok("e" in o, "e is not in o");
    test_own_data_prop_desc(o, "prop", false, false, false);
    test_own_data_prop_desc(o, "e", false, true, false);
    for(var p in o) ok(p === "e", "p = " + p);

    var desc_proto = new Object();
    desc_proto.proto_prop = { value: true, enumerable: true };
    descs = Object.create(desc_proto);
    o = Object.create(null, descs);
    ok(!("proto_prop" in o), "proto_prop is in o");
});

sync_test("property_definitions", function() {
    var obj, val, i, arr;

    function test_accessor_prop_desc(obj, prop, have_getter, have_setter) {
        var desc = Object.getOwnPropertyDescriptor(obj, prop);
        ok(desc.enumerable === true, "desc.enumerable = " + desc.enumerable);
        ok(desc.configurable === true, "desc.configurable = " + desc.configurable);

        if(have_getter) {
            ok(typeof(desc.get) === "function", "desc.get = " + desc.get);
            ok(typeof(desc.get.prototype) === "object", "desc.get.prototype = " + desc.get.prototype);
        }else {
            ok(!("get" in obj), "desc.get = " + desc.get);
        }

        if(have_setter) {
            ok(typeof(desc.set) === "function", "desc.set = " + desc.set);
            ok(typeof(desc.set.prototype) === "object", "desc.set.prototype = " + desc.set.prototype);
        }else {
            ok(!("set" in obj), "desc.get = " + desc.get);
        }
    }

    obj = {
        get prop()  { return val + 1; },
        set prop(v) { val = v; }
    };
    test_accessor_prop_desc(obj, "prop", true, true);
    val = 0;
    ok(obj.prop === 1, "obj.prop = " + obj.prop);
    obj.prop = 3;
    ok(val === 3, "val = " + val);
    ok(obj.prop === 4, "obj.prop = " + obj.prop);

    arr = [];
    for(i in obj)
        arr.push(i);
    ok(arr.join() === "prop", "prop of obj = " + arr.join());

    obj = {
        set prop(v) { val = v; }
    };
    test_accessor_prop_desc(obj, "prop", false, true);
    val = 1;
    ok(obj.prop === undefined, "obj.prop = " + obj.prop);
    obj.prop = 2;
    ok(val === 2, "val = " + val);
    ok(obj.prop === undefined, "obj.prop = " + obj.prop);

    obj = {
        get prop()  { return val + 1; },
        get 0()     { return val + 2; }
    };
    test_accessor_prop_desc(obj, "prop", true, false);
    val = 5;
    ok(obj.prop === 6, "obj.prop = " + obj.prop);
    obj.prop = 10;
    ok(val === 5, "val = " + val);
    ok(obj.prop === 6, "obj.prop = " + obj.prop);
    test_accessor_prop_desc(obj, "0", true, false);
    ok(obj[0] === 7, "obj.prop = " + obj[0]);
});

sync_test("string_trim", function() {
    function test_trim(value, expected) {
        var r = String.prototype.trim.call(value);
        ok(r === expected, "trim(" + value + ") = " + r);
    }

    test_trim("test", "test");
    test_trim(false, "false");
    test_trim("\n \t\rte st\t\t\n", "te st");
    test_trim({ toString: function() { return " test "; } }, "test");
    test_trim("", "");
    test_trim(" \t\n", "");
});

sync_test("global_properties", function() {
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
});

sync_test("string_split", function() {
    var r;

    /* IE9 got this wrong*/
    if("1undefined2".split(undefined).length != 1) {
        win_skip("detected broken String.prototype.split implementation");
        return;
    }

    function test(string, separator, result) {
        var r = string.split(separator);
        ok(r == result, "\"" + string + "\".split(" + separator + ") returned " + r + " expected " + result);
    }

    test("test", /^|\s+/, "test");
    test("test", /$|\s+/, "test");
    test("test", /^|./, "t,,,");
    test("test", /.*/, ",");
    test("test", /x*/, "t,e,s,t");
    test("test", /$|x*/, "t,e,s,t");
    test("test", /^|x*/, "t,e,s,t");
    test("test", /t*/, ",e,s,");
    test("xaabaax", /a*|b*/, "x,b,x");
    test("xaabaax", /a+|b+/, "x,,,x");
    test("xaabaax", /a+|b*/, "x,,,x");
    test("xaaxbaax", /b+|a+/, "x,x,,x");
    test("test", /^|t/, "tes,");
    test("test", /^|t/, "tes,");
    test("a,,b", /,/, "a,,b");
    test("ab", /a*/, ",b");
    test("aab", "a", ",,b");
    test("a", "a", ",");

    function test_length(string, separator, len) {
        var r = string.split(separator);
        ok(r.length === len, "\"" + string + "\".split(" + separator + ").length = "
           + r.length + " expected " + len);
    }

    test_length("", /a*/, 0);
    test_length("", /a+/, 1);
    test_length("", "", 0);
    test_length("", "x", 1);

    r = "1,2,3".split(undefined);
    ok(typeof(r) === "object", "typeof(r) = " + typeof(r));
    ok(r.length === 1, "r.length = " + r.length);
    ok(r[0] === "1,2,3", "r[0] = " + r[0]);

    r = "1,undefined2undefined,3".split(undefined);
    ok(typeof(r) === "object", "typeof(r) = " + typeof(r));
    ok(r.length === 1, "r.length = " + r.length);
    ok(r[0] === "1,undefined2undefined,3", "r[0] = " + r[0]);

    r = "1,undefined2undefined,3".split();
    ok(typeof(r) === "object", "typeof(r) = " + typeof(r));
    ok(r.length === 1, "r.length = " + r.length);
    ok(r[0] === "1,undefined2undefined,3", "r[0] = " + r[0]);

    /* note: spec violation, limit is ignored */
    r = "1,undefined2undefined,3".split(undefined, 0);
    ok(typeof(r) === "object", "typeof(r) = " + typeof(r));
    ok(r.length === 1, "r.length = " + r.length);
    ok(r[0] === "1,undefined2undefined,3", "r[0] = " + r[0]);

    r = "1,undefined2null,3".split(null);
    ok(typeof(r) === "object", "typeof(r) = " + typeof(r));
    ok(r.length === 2, "r.length = " + r.length);
    ok(r[0] === "1,undefined2", "r[0] = " + r[0]);
    ok(r[1] === ",3", "r[1] = " + r[1]);

    r = "".split();
    ok(typeof(r) === "object", "typeof(r) = " + typeof(r));
    ok(r.length === 1, "r.length = " + r.length);
    ok(r[0] === "", "r[0] = " + r[0]);
});

sync_test("getPrototypeOf", function() {
    ok(Object.create.length === 2, "Object.create.length = " + Object.create.length);
    ok(Object.getPrototypeOf.length === 1, "Object.getPrototypeOf.length = " + Object.getPrototypeOf.length);

    ok(Object.getPrototypeOf(new Object()) === Object.prototype,
       "Object.getPrototypeOf(new Object()) !== Object.prototype");

    function Constr() {}
    var obj = new Constr();
    ok(Object.getPrototypeOf(Constr.prototype) === Object.prototype,
       "Object.getPrototypeOf(Constr.prototype) !== Object.prototype");
    ok(Object.getPrototypeOf(obj) === Constr.prototype,
       "Object.getPrototypeOf(obj) !== Constr.prototype");

    var proto = new Object();
    Constr.prototype = proto;
    ok(Object.getPrototypeOf(obj) != proto,
       "Object.getPrototypeOf(obj) == proto");
    obj = new Constr();
    ok(Object.getPrototypeOf(obj) === proto,
       "Object.getPrototypeOf(obj) !== proto");
    ok(Object.getPrototypeOf(obj, 2, 3, 4) === proto,
       "Object.getPrototypeOf(obj) !== proto");

    ok(Object.getPrototypeOf(Object.prototype) === null,
       "Object.getPrototypeOf(Object.prototype) !== null");

    obj = Object.create(proto = { test: 1 });
    ok(Object.getPrototypeOf(obj) === proto,
       "Object.getPrototypeOf(obj) !== proto");
    ok(obj.test === 1, "obj.test = " + obj.test);

    obj = Object.create(null);
    ok(!("toString" in obj), "toString is in obj");
    ok(Object.getPrototypeOf(obj) === null, "Object.getPrototypeOf(obj) = " + Object.getPrototypeOf(obj));
});

sync_test("toString", function() {
    var tmp, obj;

    (function () { tmp = Object.prototype.toString.call(arguments); })();
    todo_wine.
    ok(tmp === "[object Arguments]", "toString.call(arguments) = " + tmp);
    tmp = Object.prototype.toString.call(this);
    todo_wine.
    ok(tmp === "[object Window]", "toString.call(null) = " + tmp);
    tmp = Object.prototype.toString.call(null);
    todo_wine.
    ok(tmp === "[object Null]", "toString.call(null) = " + tmp);
    tmp = Object.prototype.toString.call(undefined);
    todo_wine.
    ok(tmp === "[object Undefined]", "toString.call(undefined) = " + tmp);
    tmp = Object.prototype.toString.call();
    todo_wine.
    ok(tmp === "[object Undefined]", "toString.call() = " + tmp);

    obj = Object.create(null);
    tmp = Object.prototype.toString.call(obj);
    ok(tmp === "[object Object]", "toString.call(Object.create(null)) = " + tmp);
    obj = Object.create(Number.prototype);
    tmp = Object.prototype.toString.call(obj);
    ok(tmp === "[object Object]", "toString.call(Object.create(Number.prototype)) = " + tmp);
});

sync_test("bind", function() {
    var f, r;
    var o = new Object(), o2 = new Object();

    f = (function() {
        ok(this === o, "this != o");
        ok(arguments.length === 0, "arguments.length = " + arguments.length);
        return 1;
    }).bind(o);
    ok(f.length === 0, "f.length = " + f.length);
    r = f.call(o2);
    ok(r === 1, "r = " + r);

    f = (function() {
        ok(this === o, "this != o");
        ok(arguments.length === 1, "arguments.length = " + arguments.length);
        ok(arguments[0] === 1, "arguments[0] = " + arguments[0]);
        return 1;
    }).bind(o, 1);
    ok(f.length === 0, "f.length = " + f.length);
    r = f.call(o2);
    ok(r === 1, "r = " + r);

    f = (function() {
        ok(this === o, "this != o");
        ok(arguments.length === 2, "arguments.length = " + arguments.length);
        ok(arguments[0] === 1, "arguments[0] = " + arguments[0]);
        ok(arguments[1] === 2, "arguments[1] = " + arguments[0]);
        return 1;
    }).bind(o, 1);
    r = f.call(o2, 2);
    ok(r === 1, "r = " + r);

    o2.f = f;
    r = o2.f(2);
    ok(r === 1, "r = " + r);

    f = (function test(x, y, z) {
        ok(this === o, "this != o");
        ok(arguments.length === 2, "arguments.length = " + arguments.length);
        ok(x === 1, "x = " + x);
        ok(y === 2, "y = " + y);
        ok(z === undefined, "z = " + z);
        return 1;
    }).bind(o, 1);
    ok(f.length === 2, "f.length = " + f.length);
    r = f.call(o2, 2);
    ok(r === 1, "r = " + r);
    ok(f.toString() === "\nfunction() {\n    [native code]\n}\n", "f.toString() = " + f.toString());
    ok(!("prototype" in f), "bound function has prototype");

    var a = [];
    f = Array.prototype.push.bind(a, 1);
    f();
    ok(a.length === 1, "a.length = " + a.length);
    f(2);
    ok(a.length === 3, "a.length = " + a.length);
    ok(f.length === 0, "f.length = " + f.length);
    ok(f.toString() === "\nfunction() {\n    [native code]\n}\n", "f.toString() = " + f.toString());
    ok(a.toString() === "1,1,2", "a = " + a);
    f.call([], 3);
    ok(a.toString() === "1,1,2,1,3", "a = " + a);

    f = (function() { return this; }).bind(a);
    ok(f() === a, "f() != a");

    f = (function() { return this; }).bind(null);
    ok(f() === window, "f() = " + f());

    var t;
    f = (function() { return t = this; }).bind(a);
    ok(new f() === t, "new f() != a");
    ok(typeof(t) === "object", "typeof(t) = " + typeof(t));
    ok(t != a, "t == a");

    ok(Function.prototype.bind.length === 1, "Function.prototype.bind.length = " + Function.prototype.bind.length);
});

sync_test("keys", function() {
    var o = { a: 1, b: 2, c: 3 };
    var keys = Object.keys(o).sort().join();
    ok(keys === "a,b,c", "keys = " + keys);

    o = Object.create(o);
    keys = Object.keys(o).sort().join();
    ok(keys === "", "keys = " + keys);

    o.test = 1;
    keys = Object.keys(o).sort().join();
    ok(keys === "test", "keys = " + keys);

    Object.defineProperty(o, "defined", { value: 3, enumerable: false });
    keys = Object.keys(o).sort().join();
    ok(keys === "test", "keys = " + keys);

    keys = Object.keys([]).sort().join();
    ok(keys === "", "keys([]) = " + keys);

    ok(Object.keys.length === 1, "Object.keys.length = " + Object.keys.length);
});

sync_test("getOwnPropertyNames", function() {
    var o = { a: 1, b: 2, c: 3 };
    var names = Object.getOwnPropertyNames(o).sort().join();
    ok(names === "a,b,c", "names = " + names);

    o = Object.create(o);
    names = Object.getOwnPropertyNames(o).sort().join();
    ok(names === "", "names = " + names);

    o.test = 1;
    names = Object.getOwnPropertyNames(o).sort().join();
    ok(names === "test", "names = " + names);

    Object.defineProperty(o, "defined", { value: 3, enumerable: false });
    names = Object.getOwnPropertyNames(o).sort().join();
    ok(names === "defined,test", "names = " + names);

    names = Object.getOwnPropertyNames([]).sort().join();
    todo_wine.
    ok(names === "length", "names = " + names);

    ok(Object.getOwnPropertyNames.length === 1, "Object.getOwnPropertyNames.length = " + Object.getOwnPropertyNames.length);
});

sync_test("reduce", function() {
    var r, array;

    r = [1,2,3].reduce(function(a, value) { return a + value + 10; });
    ok(r === 26, "reduce() returned " + r);

    r = [1,2,3].reduce(function(a, value) { return a + value + 10; }, 1);
    ok(r === 37, "reduce() returned " + r);

    r = [1,2,3].reduce(function(a, value) { return a + value; }, "str");
    ok(r === "str123", "reduce() returned " + r);

    array = [1,2,3];
    r = array.reduce(function(a, value, index, src) {
        ok(src === array, "src != array");
        return a + "(" + index + "," + value + ")";
    }, "str");
    ok(r === "str(0,1)(1,2)(2,3)", "reduce() returned " + r);

    r = [1,2,3].reduce(function(a, value, index, src) {
        src[0] = false;
        delete src[1];
        src[2] = "test";
        return a + value;
    }, "");
    ok(r === "1test", "reduce() returned " + r);

    r = [1].reduce(function(a) { return 0; });
    ok(r === 1, "[1].reduce() returned " + r);

    r = [1].reduce(function(a) { return 0; }, 2);
    ok(r === 0, "[1].reduce(2) returned " + r);

    r = [].reduce(function(a) { return 0; }, 2);
    ok(r === 2, "[].reduce(2) returned " + r);

    r = [].reduce(function(a) { return 0; }, undefined);
    ok(r === undefined, "[].reduce(undefined) returned " + r);

    try {
        [].reduce(function(a) { return 0; });
        ok(false, "expected exception");
    }catch(e) {}

    ok(Array.prototype.reduce.length === 1, "Array.prototype.reduce.length = " + Array.prototype.reduce.length);
});

sync_test("preventExtensions", function() {
    var o = {};
    var r = Object.preventExtensions(o);
    ok(r === o, "r != o");
    o.x = 1;
    ok(!("x" in o), "x property added to o");
    try {
        Object.defineProperty(o, "y", { value: true });
        ok(false, "expected exception");
    }catch(e) {
        ok(e.name === "TypeError", "got " + e.name + " exception");
    }

    r = Object.preventExtensions(o);
    ok(r === o, "r != o");
    r = Object.isExtensible(o);
    ok(r === false, "isExtensible(o) returned " + r);

    function Constr() {}
    o = Object.preventExtensions(new Constr());
    Constr.prototype.prop = 1;
    ok(o.prop === 1, "o.prop = " + o.prop);
    o.prop = 2;
    ok(o.prop === 1, "o.prop = " + o.prop);
    r = Object.isExtensible(o);
    ok(r === false, "isExtensible(o) returned " + r);

    r = Object.isExtensible({});
    ok(r === true, "isExtensible(o) returned " + r);

    try {
        Object.isExtensible(1);
        ok(false, "exception expected");
    }catch(e) {
        ok(e.name === "TypeError", "got " + e.name + " exception");
    }

    o = [];
    Object.preventExtensions(o);
    try {
        o.push(1);
        ok(false, "exception expected on o.push");
    }catch(e) {
        ok(e.name === "TypeError", "got " + e.name + " exception");
    }
    ok(!("0" in o), "0 is in o");
    ok(o.length === 0, "o.length = " + o.length);

    o = [1];
    Object.preventExtensions(o);
    o.pop();
    ok(!("0" in o), "0 is in o");
    ok(o.length === 0, "o.length = " + o.length);

    ok(Object.preventExtensions.length === 1, "Object.preventExtensions.length = " + Object.preventExtensions.length);
    ok(Object.isExtensible.length === 1, "Object.isExtensible.length = " + Object.isExtensible.length);
});

sync_test("freeze", function() {
    ok(Object.freeze.length === 1, "Object.freeze.length = " + Object.freeze.length);
    try {
        Object.freeze(1);
        ok(false, "exception expected");
    }catch(e) {
        ok(e.name === "TypeError", "got " + e.name + " exception");
    }

    function f() {}

    var o = {}, r;
    o.prop = 1;
    o.func = f;
    Object.defineProperty(o, "accprop", {
        get: function() {
            return r;
        },
        set: function(v) {
            r = v;
        }
    });

    test_own_data_prop_desc(o, "prop", true, true, true);
    r = Object.freeze(o);
    ok(r === o, "r != o");
    test_own_data_prop_desc(o, "prop", false, true, false);
    test_own_data_prop_desc(o, "func", false, true, false);
    ok(!Object.isExtensible(o), "o is still extensible");
    o.prop = false;
    o.func = false;
    ok(o.prop === 1, "o.prop = " + o.prop);
    ok(o.func === f, "o.func = " + o.func);

    r = 1;
    o.accprop = 2;
    ok(r === 2, "r = " + r);
    r = 3;
    ok(o.accprop === 3, "o.accprop = " + o.accprop);

    o = [1];
    Object.freeze(o);
    try {
        o.pop();
        ok(false, "exception expected on o.pop");
    }catch(e) {
        ok(e.name === "TypeError", "got " + e.name + " exception");
    }
    ok(o[0] === 1, "o[0] = " + o[0]);
    ok(o.length === 1, "o.length = " + o.length);
});

sync_test("seal", function() {
    ok(Object.seal.length === 1, "Object.seal.length = " + Object.seal.length);
    try {
        Object.seal(1);
        ok(false, "exception expected");
    }catch(e) {
        ok(e.name === "TypeError", "got " + e.name + " exception");
    }

    function f() {}

    var o = {}, r;
    o.prop = 1;
    o.func = f;
    Object.defineProperty(o, "accprop", {
        get: function() {
            return r;
        },
        set: function(v) {
            r = v;
        }
    });

    test_own_data_prop_desc(o, "prop", true, true, true);
    r = Object.seal(o);
    ok(r === o, "r != o");
    test_own_data_prop_desc(o, "prop", true, true, false);
    test_own_data_prop_desc(o, "func", true, true, false);
    ok(!Object.isExtensible(o), "o is still extensible");
    o.prop = false;
    o.func = false;
    ok(o.prop === false, "o.prop = " + o.prop);
    ok(o.func === false, "o.func = " + o.func);

    r = 1;
    o.accprop = 2;
    ok(r === 2, "r = " + r);
    r = 3;
    ok(o.accprop === 3, "o.accprop = " + o.accprop);

    o = [1];
    Object.seal(o);
    try {
        o.pop();
       ok(false, "exception expected on o.pop");
    }catch(e) {
        ok(e.name === "TypeError", "got " + e.name + " exception");
    }
    ok(o[0] === 1, "o[0] = " + o[0]);
    ok(o.length === 1, "o.length = " + o.length);
});

sync_test("isFrozen", function() {
    ok(Object.isFrozen.length === 1, "Object.isFrozen.length = " + Object.isFrozen.length);
    ok(Object.isSealed.length === 1, "Object.isSealed.length = " + Object.isSealed.length);

    var o = Object.freeze({});
    ok(Object.isFrozen(o) === true, "o is not frozen");
    ok(Object.isSealed(o) === true, "o is not sealed");
    ok(Object.isExtensible(o) === false, "o is extensible");

    ok(Object.isFrozen({}) === false, "{} is frozen");
    ok(Object.isSealed({}) === false, "{} is sealed");

    o = Object.preventExtensions({});
    ok(Object.isFrozen(o) === true, "o is not frozen");
    ok(Object.isSealed(o) === true, "o is not sealed");
    ok(Object.isExtensible(o) === false, "o is extensible");

    o = Object.preventExtensions({ prop: 1 });
    ok(Object.isFrozen(o) === false, "o is frozen");
    ok(Object.isSealed(o) === false, "o is sealed");
    ok(Object.isExtensible(o) === false, "o is extensible");

    o = Object.freeze({ prop: 1 });
    ok(Object.isFrozen(o) === true, "o is not frozen");
    ok(Object.isSealed(o) === true, "o is not sealed");
    ok(Object.isExtensible(o) === false, "o is extensible");

    o = Object.seal({ prop: 1 });
    ok(Object.isFrozen(o) === false, "o is frozen");
    ok(Object.isSealed(o) === true, "o is not sealed");
    ok(Object.isExtensible(o) === false, "o is extensible");

    o = {};
    Object.defineProperty(o, "prop", { value: 1 });
    Object.preventExtensions(o);
    ok(Object.isFrozen(o) === true, "o is not frozen");
    ok(Object.isSealed(o) === true, "o is not sealed");
    ok(Object.isExtensible(o) === false, "o is extensible");

    o = {};
    Object.defineProperty(o, "prop", { value: 1, writable: true });
    Object.preventExtensions(o);
    ok(Object.isFrozen(o) === false, "o is frozen");
    ok(Object.isSealed(o) === true, "o is not sealed");
    ok(Object.isExtensible(o) === false, "o is extensible");

    o = {};
    Object.defineProperty(o, "prop", { value: 1, writable: true, configurable: true });
    Object.preventExtensions(o);
    ok(Object.isFrozen(o) === false, "o is frozen");
    ok(Object.isSealed(o) === false, "o is sealed");
    ok(Object.isExtensible(o) === false, "o is extensible");

    o = {};
    Object.defineProperty(o, "prop", { value: 1, configurable: true });
    Object.preventExtensions(o);
    ok(Object.isFrozen(o) === false, "o is frozen");
    ok(Object.isSealed(o) === false, "o is sealed");
    ok(Object.isExtensible(o) === false, "o is extensible");

    o = {};
    Object.defineProperty(o, "prop", { get: function() {}, set: function() {} });
    Object.preventExtensions(o);
    ok(Object.isFrozen(o) === true, "o is not frozen");
    ok(Object.isSealed(o) === true, "o is not sealed");
    ok(Object.isExtensible(o) === false, "o is extensible");
});

sync_test("head_setter", function() {
    document.head = "";
    ok(typeof(document.head) === "object", "typeof(document.head) = " + typeof(document.head));
});


sync_test("declaration_let", function() {
    ok(typeof(func) === "undefined", "typeof(func)  = " + typeof(func));
    with(new Object()) {
        var x = false && function func() {};
    }
    ok(typeof(func) === "undefined", "typeof(func)  = " + typeof(func));

    function expect_exception(func, todo) {
        try {
            func();
        }catch(e) {
            return;
        }
        if (typeof todo === 'undefined' || !todo)
            ok(false, "expected exception");
        else
            todo_wine.ok(false, "expected exception");
    }

    function call_func(f, expected_a)
    {
        f(2, expected_a);
    }

    ok(a === undefined, "a is not undefined");
    var a = 3;

    {
        let a = 2;
        let b

        ok(typeof b === 'undefined', "b is defined");
        ok(b === undefined, "b !== undefined");

        ok(a == 2, "a != 2");

        a = 4;
        ok(a == 4, "a != 4");

        eval('ok(a == 4, "eval: a != 4"); b = a; a = 5;')
        ok(b == 4, "b != 4");
        ok(a == 5, "a != 5");

        function func1()
        {
            ok(typeof b === 'undefined', "func1: b is defined");
            ok(b === undefined, "func1: should produce exception");
            let b = 1;
        }
        expect_exception(func1, true);

        function func2()
        {
            let b = 1;
            ok(b == 1, "func2: b != 1");
        }
        func2();

        var w = 8;
        with({w: 9})
        {
            {
                let c = 5

                function func3(b, expected)
                {
                    var b = 2

                    ok(typeof d === 'undefined', "d is defined");

                    ok(c == expected, "func3: c != expected");
                    ok(w == 9, "w != 9")
                    ok(b == 2, "func3: b != 2");
                    b = 3;
                    ok(b == 3, "func3: b != 3");
                    ok(a == expected, "func3: a != expected");
                    a = 6;
                    c = 6;
                }

                let f3 = func3
                let f4 = function()
                    {
                        ok(a == 6, "f4: a != 6");
                    }

                ok(a == 5, "tmp 2 a != 5");
                ok(c == 5, "c != 5");
                func3(1, 5)
                ok(c == 6, "c != 6");
                call_func(func3, 6);
                f3(1, 6)
                ok(a == 6, "a != 6");
                ok(b == 4, "b != 4");
                ok(c == 6, "c != 6");

                call_func(f4);
                f4();
            }
        }
        {
            let c = 4;
            let d = 1;

            func3(1, 6);
        }
    }

    ok(a == 3, "a != 3");

    var except = false

    try
    {
        eval('if (true) let a = 3');
    }
    catch (e)
    {
        except = true;
    }
    ok(except, "with({w:9}) let a = 3: expected exception.");

    let for_count = 0;
    for (let for_i1 = 0, for_i2 = 1; for_i1 < 3; ++for_i1, ++for_i2, ++for_count)
    {
        let for_i2 = 10;

        ok(for_i2 == 10, "for_i2 != 10");
    }

    ok(typeof for_i1 == 'undefined', "for_i1 is defined");
    ok(typeof for_i2 == 'undefined', "for_i2 is defined");
    ok(for_count == 3, "for_count != 3");
});

sync_test("let scope instances", function() {
    var a = [], i;
    for(i = 0; i < 3; i++) {
        a[i] = function() { return v; };
        let v = i;
    }
    for(i = 0; i < 3; i++)
        ok(a[i]() == i, "a[" + i + "]() = " + a[i]());

    ok(typeof f == 'undefined', "f is defined");

    for(i = 0; i < 3; i++) {
        function f() { return v; }
        a[i] = f;
        let v = i;
    }
    for(i = 0; i < 3; i++)
        ok(a[i]() == i, "a[" + i + "]() = " + a[i]());

    ok(f() == 2, "f() = " + f());
});

sync_test("functions scope", function() {
    function f(){ return 1; }
    function f(){ return 2; }

    var f0 = f, f1, f2, f3, i, o, a = [];
    ok(f0() === 2, "f0() = " + f0());

    {
        f1 = f;
        function f() { return 3; }
        ok(f1 === f, "f1 != f");
        ok(f0 != f1, "f0 == f1");
    }
    ok(f === f1, "f != f1");

    for(i = 0; i < 3; i++) {
        a[i] = f;
        function f() {}
        ok(f === a[i], "f != a[i]");
    }
    ok(a[0] != a[1], "a[0] == a[1]");
    ok(a[1] != a[2], "a[1] == a[2]");
    ok(f === a[2], "f != a[2]");

    {
        f2 = f;
        ok(f() === 4, "f() = " + f());
        function f() { return 4; }

        {
            f3 = f;
            ok(f() === 5, "f() = " + f());
            function f() { return 5;}
        }
        ok(f() === 4, "f() = " + f());
        ok(f === f2, "f != f2");
    }
    ok(f === f3, "f != f3");

    with(o = {f: 1}) {
        ok(f === 1, "f != 1");
        {
            ok(f() === 6, "f() = " + f());
            function f() { return 6; }
        }
        ok(f === 1, "f != 1");
        delete o.f;
        ok(f() === 6, "f() = " + f());
    }

    if(false) {
        function f() { throw "unexpected call"; }
    }
    ok(f() === 6, "f() = " + f());

    /* 'with' has no effect for function defined in a single statement context. */
    let w = 8;
    with({w:10, value:11})
        function with_function()
        {
            var except

            ok(w == 8, "w != 8");
            except = false;
            try
            {
                ok(value === undefined, "value is defined");
            }
            catch(e)
            {
                except = true;
            }
            ok(except, "with_function: expected exception");

            let ret = w;
            w = 9;
            return ret;
        }
    val = with_function();
    ok(val == 8, "val != 8");
    ok(w == 9, "w != 9");
});

sync_test("console", function() {
    var except

    window.console.log('1', '2');
    console.info('1', '2', '3');
    console.info();
    console.log();
    console.trace();
    console.warn();
    console.debug();
    console.error();

    console.assert(false, '1');
    console.assert(true, '1');
    console.assert('1');

    console.clear();
    console.count('1');
    console.count(1);


    except = false;
    try
    {
        console.countReset('1');
    }
    catch(e)
    {
        except = true;
    }
    ok(except, "console.countReset: expected exception");
    console.dir(document);
    console.dir();
    console.dirxml(document);
    console.group('1');
    console.groupCollapsed('1');
    console.groupEnd();

    except = false;
    try
    {
        console.table(['1', '2']);
    }
    catch(e)
    {
        except = true;
    }
    ok(except, "console.table: expected exception");

    console.time('1');
    console.timeEnd('1');

    except = false;
    try
    {
        console.timeLog('1');
    }
    catch(e)
    {
        except = true;
    }
    ok(except, "console.timeLog: expected exception");
});
