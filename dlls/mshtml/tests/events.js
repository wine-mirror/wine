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

var test_content_loaded = (function() {
    var calls = "";

    function unexpected_call() {
        ok(false, "unexpected call");
    }

    function record_call(msg) {
        return function() { calls += msg + "," };
    }

    window.addEventListener("DOMContentLoaded", record_call("window.capture"), true);
    window.addEventListener("DOMContentLoaded", record_call("window.bubble"), false);
    document.addEventListener("DOMContentLoaded", record_call("document.capture"), true);
    document.addEventListener("DOMContentLoaded", record_call("document.bubble"), false);

    document.attachEvent("onDOMContentLoaded", unexpected_call);
    document.attachEvent("DOMContentLoaded", unexpected_call);

    return function() {
        ok(calls === "window.capture,document.capture,document.bubble,window.bubble,",
           "calls = " + calls);
        next_test();
    };
})();

function test_listener_order() {
    document.body.innerHTML = '<div></div>';
    var div = document.body.firstChild;

    var calls;
    function record_call(msg) {
        return function() { calls += msg + "," };
    }

    window.addEventListener("click", record_call("window.click(capture)"), true);
    window.addEventListener("click", record_call("window.click(bubble)"), false);

    document.attachEvent("onclick", record_call("document.click(attached)"));
    document.addEventListener("click", record_call("document.click(capture)"), true);
    document.addEventListener("click", record_call("document.click(bubble)"), false);
    document.onclick = record_call("document.onclick");

    document.body.onclick = record_call("body.onclick");
    document.body.addEventListener("click", record_call("body.click(capture)"), true);
    document.body.addEventListener("click", record_call("body.click(bubble)"), false);
    document.body.addEventListener("click", record_call("body.click(bubble2)"));
    document.body.attachEvent("onclick", record_call("body.click(attached)"));

    div.attachEvent("onclick", record_call("div.click(attached)"));
    div.addEventListener("click", record_call("div.click(bubble)"), false);
    div.onclick = record_call("div.onclick");
    div.addEventListener("click", record_call("div.click(capture1)"), true);
    div.addEventListener("click", record_call("div.click(capture2)"), true);

    calls = "";
    div.click();
    ok(calls === "window.click(capture),document.click(capture),body.click(capture),"
       + "div.click(attached),div.click(bubble),div.onclick,div.click(capture1),"
       + "div.click(capture2),body.onclick,body.click(bubble),body.click(bubble2),"
       + "body.click(attached),document.click(attached),document.click(bubble),"
       + "document.onclick,window.click(bubble),", "calls = " + calls);

    div.onclick = record_call("new div.onclick");

    calls = "";
    div.click();
    ok(calls === "window.click(capture),document.click(capture),body.click(capture),"
       + "div.click(attached),div.click(bubble),new div.onclick,div.click(capture1),"
       + "div.click(capture2),body.onclick,body.click(bubble),body.click(bubble2),"
       + "body.click(attached),document.click(attached),document.click(bubble),"
       + "document.onclick,window.click(bubble),", "calls = " + calls);

    next_test();
}

function test_add_listener_in_listener() {
    var calls;

    document.body.innerHTML = '<div><div></div></div>';
    var div1 = document.body.firstChild;
    var div2 = div1.firstChild;

    function record_call(msg) {
        return function() { calls += msg + "," };
    }

    div1.addEventListener("click", function() {
        div2.addEventListener("click", function() {
            calls += "div2.click";
            /* if we add more listeners here, whey won't be invoked */
            div2.onclick = function() { calls += "click2,"; };
            div2.addEventListener("click", function() { calls += "click3,"; }, false);
            div2.attachEvent("onclick", function() { calls += "click4,"; });
        }, true);
    }, true);

    calls = "";
    div2.click();
    ok(calls === "div2.click", "calls = " + calls);

    next_test();
}

function test_remove_listener_in_listener() {
    var calls;

    document.body.innerHTML = '<div></div>';
    var div = document.body.firstChild;

    function record_call(msg) {
        return function() { calls += msg + "," };
    }

    var capture = record_call("capture"), bubble = record_call("bubble");

    div.addEventListener("click", function() {
        div.removeEventListener("click", capture, true);
        div.removeEventListener("click", bubble, false);
        calls += "remove,";
    }, true);

    div.addEventListener("click", capture, true);
    div.addEventListener("click", bubble, false);
    div.onclick = record_call("onclick");

    calls = "";
    div.click();
    ok(calls === "remove,capture,bubble,onclick,", "calls = " + calls);

    next_test();
}

function test_add_remove_listener() {
    var calls;

    document.body.innerHTML = '<div></div>';
    var div = document.body.firstChild;

    function listener() {
        calls += "listener,";
    }

    /* if the same listener is added twice, second one won't be called */
    div.addEventListener("click", listener, false);
    div.addEventListener("click", listener, false);

    calls = "";
    div.click();
    ok(calls === "listener,", "calls = " + calls);

    /* remove capture listener, it won't do anything */
    div.removeEventListener("click", listener, true);

    calls = "";
    div.click();
    ok(calls === "listener,", "calls = " + calls);

    /* remove listener once, it won't called anymore */
    div.removeEventListener("click", listener, false);

    calls = "";
    div.click();
    ok(calls === "", "calls = " + calls);

    div.removeEventListener("click", listener, false);

    /* test implicit capture removeEventListener argument */
    div.addEventListener("click", listener, false);
    div.removeEventListener("click", listener);

    calls = "";
    div.click();
    ok(calls === "", "calls = " + calls);

    next_test();
}

function test_event_phase() {
    document.body.innerHTML = '<div><div></div></div>';
    var div1 = document.body.firstChild;
    var div2 = div1.firstChild;
    var last_event;

    function check_phase(expected_phase) {
        return function(e) {
            if(last_event)
                ok(last_event === e, "last_event != e");
            else
                last_event = e;
            ok(e.eventPhase === expected_phase,
               "eventPhase = " + e.eventPhase + " expedted " + expected_phase);
        };
    }

    div1.addEventListener("click", check_phase(1), true);
    div1.addEventListener("click", check_phase(3), false);
    div1.onclick = check_phase(3);
    div2.addEventListener("click", check_phase(2), true);
    div2.addEventListener("click", check_phase(2), false);
    div2.onclick = check_phase(2);

    div2.click();
    ok(last_event.eventPhase === 3, "last_event.eventPhase = " + last_event.eventPhase);

    next_test();
}

function test_stop_propagation() {
    document.body.innerHTML = '<div><div></div></div>';
    var div1 = document.body.firstChild;
    var div2 = div1.firstChild;

    var calls;
    function record_call(msg) {
        return function() { calls += msg + "," };
    }

    function stop_propagation(e) {
        calls += "stop,";
        e.stopPropagation();
    }

    div1.addEventListener("click", stop_propagation, true);
    div1.addEventListener("click", record_call("div1.click(capture)"), true);

    div2.addEventListener("click", stop_propagation, true);
    div2.addEventListener("click", record_call("div2.click(capture)"), true);

    div1.addEventListener("click", stop_propagation, false);
    div1.addEventListener("click", record_call("div1.click(bubble)"), false);

    div2.addEventListener("click", stop_propagation, false);
    div2.addEventListener("click", record_call("div2.click(bubble)"), false);

    calls = "";
    div2.click();
    ok(calls === "stop,div1.click(capture),", "calls = " + calls);

    div1.removeEventListener("click", stop_propagation, true);
    calls = "";
    div2.click();
    ok(calls === "div1.click(capture),stop,div2.click(capture),stop,div2.click(bubble),",
       "calls = " + calls);

    div2.removeEventListener("click", stop_propagation, true);
    calls = "";
    div2.click();
    ok(calls === "div1.click(capture),div2.click(capture),stop,div2.click(bubble),",
       "calls = " + calls);

    div2.removeEventListener("click", stop_propagation, false);
    calls = "";
    div2.click();
    ok(calls === "div1.click(capture),div2.click(capture),div2.click(bubble),stop,div1.click(bubble),",
       "calls = " + calls);

    next_test();
}

function test_prevent_default() {
    document.body.innerHTML = '<div><a href="about:blank"></a></div>';
    var div = document.body.firstChild;
    var a = div.firstChild;
    var calls;

    div.addEventListener("click", function(e) {
        calls += "div,";
        ok(e.defaultPrevented === false, "e.defaultPrevented = " + e.defaultPrevented);
        e.preventDefault();
        ok(e.defaultPrevented === true, "e.defaultPrevented = " + e.defaultPrevented);
    }, true);

    a.addEventListener("click", function(e) {
        calls += "a,";
        ok(e.defaultPrevented === true, "e.defaultPrevented = " + e.defaultPrevented);
    }, true);

    calls = "";
    a.click();
    ok(calls === "div,a,", "calls = " + calls);

    next_test();
}

var tests = [
    test_content_loaded,
    test_add_remove_listener,
    test_add_listener_in_listener,
    test_remove_listener_in_listener,
    test_event_phase,
    test_stop_propagation,
    test_prevent_default,
    test_listener_order
];
