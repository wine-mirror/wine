/*
 * Copyright 2016 Jacek Caban for CodeWeavers
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

function test_xhr() {
    var xhr = new XMLHttpRequest();
    var complete_cnt = 0, loadstart = false;

    xhr.onreadystatechange = function() {
        if(xhr.readyState != 4)
            return;

        ok(xhr.responseText === "Testing...", "unexpected responseText " + xhr.responseText);
        if(complete_cnt++ && !("onloadend" in xhr))
            next_test();
    }
    xhr.ontimeout = function() { ok(false, "ontimeout called"); }
    var onload_func = xhr.onload = function() {
        ok(xhr.statusText === "OK", "statusText = " + xhr.statusText);
        if("onloadstart" in xhr)
            ok(loadstart, "onloadstart not fired");
        if(complete_cnt++ && !("onloadend" in xhr))
            next_test();
    };
    ok(xhr.onload === onload_func, "xhr.onload != onload_func");
    if("onloadstart" in xhr) {
        xhr.onloadstart = function(e) {
            ok(complete_cnt == 0, "onloadstart fired after onload");
            var props = [ "initProgressEvent", "lengthComputable", "loaded", "total" ];
            for(var i = 0; i < props.length; i++)
                ok(props[i] in e, props[i] + " not available in loadstart");
            loadstart = true;
        };
        xhr.onloadend = function(e) {
            ok(complete_cnt == 2, "onloadend not fired after onload and onreadystatechange");
            ok(loadstart, "onloadstart not fired before onloadend");
            var props = [ "initProgressEvent", "lengthComputable", "loaded", "total" ];
            for(var i = 0; i < props.length; i++)
                ok(props[i] in e, props[i] + " not available in loadstart");
            next_test();
        };
    }

    xhr.open("POST", "echo.php", true);
    xhr.setRequestHeader("X-Test", "True");
    if("withCredentials" in xhr) {
        ok(xhr.withCredentials === false, "default withCredentials = " + xhr.withCredentials);
        xhr.withCredentials = true;
        ok(xhr.withCredentials === true, "withCredentials = " + xhr.withCredentials);
        xhr.withCredentials = false;
    }
    xhr.send("Testing...");
}

function test_abort() {
    var xhr = new XMLHttpRequest();
    if(!("onabort" in xhr)) { next_test(); return; }

    xhr.onreadystatechange = function() {
        if(xhr.readyState != 4)
            return;
        todo_wine_if(v < 10).
        ok(v >= 10, "onreadystatechange called");
    }
    xhr.onload = function() { ok(false, "onload called"); }
    xhr.onabort = function(e) { next_test(); }

    xhr.open("POST", "echo.php?delay", true);
    xhr.setRequestHeader("X-Test", "True");
    xhr.send("Abort Test");
    xhr.abort();
}

function test_timeout() {
    var xhr = new XMLHttpRequest();
    var v = document.documentMode;

    xhr.onreadystatechange = function() {
        if(xhr.readyState != 4)
            return;
        todo_wine_if(v < 10).
        ok(v >= 10, "onreadystatechange called");
    }
    xhr.onload = function() { ok(false, "onload called"); }
    xhr.ontimeout = function(e) {
        var r = Object.prototype.toString.call(e);
        todo_wine.
        ok(r === ("[object " + (v < 10 ? "Event" : "ProgressEvent") + "]"), "Object.toString = " + r);
        var props = [ "initProgressEvent", "lengthComputable", "loaded", "total" ];
        for(r = 0; r < props.length; r++) {
            if(v < 10)
                ok(!(props[r] in e), props[r] + " is available");
            else
                ok(props[r] in e, props[r] + " not available");
        }
        next_test();
    }

    xhr.open("POST", "echo.php?delay", true);
    xhr.setRequestHeader("X-Test", "True");
    xhr.timeout = 10;
    xhr.send("Timeout Test");
}

var tests = [
    test_xhr,
    test_abort,
    test_timeout
];
