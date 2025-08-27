#ifndef AP_HTML_CONTENT_H
#define AP_HTML_CONTENT_H

const char AP_HTML_CONTENT[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>ESP WiFi Setup</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { font-family: sans-serif; text-align: center; padding: 2rem; }
        form { max-width: 300px; margin: auto; }
        input { width: 100%; padding: 10px; margin-top: 10px; box-sizing: border-box; }
        button { width: 100%; padding: 10px; margin-top: 20px; background-color: #007bff; color: white; border: none; }
    </style>
</head>
<body>
    <h1>WiFi Setup</h1>
    <form action="/connect" method="POST">
        <input type="text" name="ssid" placeholder="SSID" required>
        <input type="password" name="password" placeholder="Password">
        <button type="submit">Connect</button>
    </form>
</body>
</html>
)rawliteral";

#endif // AP_HTML_CONTENT_H