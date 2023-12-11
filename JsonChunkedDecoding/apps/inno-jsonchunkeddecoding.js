
/// <reference path="../../web1/lib1/innovaphone.lib1.js" />
/// <reference path="../../web1/appwebsocket/innovaphone.appwebsocket.Connection.js" />
/// <reference path="../../web1/ui1.lib/innovaphone.ui1.lib.js" />

var inno = inno || {};
inno.JsonChunkedDecoding = inno.JsonChunkedDecoding || function (start, args) {

    this.createNode("body");
    var that = this;

    var colorSchemes = {
        dark: {
            "--bg": "#191919",
            "--button": "#303030",
            "--text-standard": "#f2f5f6",
        },
        light: {
            "--bg": "white",
            "--button": "#e0e0e0",
            "--text-standard": "#4a4a49",
        }
    };

    var schemes = new innovaphone.ui1.CssVariables(colorSchemes, start.scheme);
    start.onschemechanged.attach(function () { schemes.activate(start.scheme) });

    var texts = new innovaphone.lib1.Languages(inno.JsonChunkedDecodingTexts, start.lang);
    start.onlangchanged.attach(function () { texts.activate(start.lang) });

    var app = new innovaphone.appwebsocket.Connection(start.url, start.name);
    app.checkBuild = true;
    app.onconnected = app_connected;

    function app_connected(domain, user, dn, appdomain) {
        sendDiv = that.add(new innovaphone.ui1.Div("align-content:center; display:flex; justify-content:center;"));
        inputField = sendDiv.add(new innovaphone.ui1.Input("width:150px; height:25px; font-size:15px; margin-top:90px;", "", "Data", "", "text", "inputStyle"));
        sendButton = sendDiv.add(new innovaphone.ui1.Div("height:25px; padding-top:5px; padding-bottom:25px; margin-top:120px;", "Send", "button").addEvent("click", () => {
            // Make the PUT request
            fetch(start.originalUrl + '/json', {
                method: 'PUT',
                headers: {
                    'Content-Type': 'application/json',
                },
                body: inputField.getValue()
            })
                .catch(error => {
                    console.error('Error:', error);
                });
        }));
    }

}

inno.JsonChunkedDecoding.prototype = innovaphone.ui1.nodePrototype;