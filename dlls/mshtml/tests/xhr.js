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
    var complete_cnt = 0;

    xhr.onreadystatechange = function() {
        if(xhr.readyState != 4)
            return;

        ok(xhr.responseText === "Testing...", "unexpected responseText " + xhr.responseText);
        if(complete_cnt++)
            next_test();
    }
    xhr.ontimeout = function() { ok(false, "ontimeout called"); }
    var onload_func = xhr.onload = function() {
        ok(xhr.statusText === "OK", "statusText = " + xhr.statusText);
        if(complete_cnt++)
            next_test();
    };
    ok(xhr.onload === onload_func, "xhr.onload != onload_func");

    xhr.open("POST", "echo.php", true);
    xhr.setRequestHeader("X-Test", "True");
    xhr.send("Testing...");
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
    test_timeout
];
