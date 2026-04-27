#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <Update.h> // OTA update
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <time.h>

// --- OLED display ---
#ifdef HAS_OLED
#include <U8g2lib.h>
#include <Wire.h>
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(
    U8G2_R0,
    /* reset=*/ U8X8_PIN_NONE,
    /* clock=*/ OLED_SCL,
    /* data=*/ OLED_SDA
);
#endif

const byte DNS_PORT = 53;
IPAddress apIP(192, 168, 4, 1);

DNSServer dnsServer;
WebServer server(80);
Preferences preferences;

String saved_ssid = "";
String saved_pass = "";
String saved_tg_id = "";
String saved_server_url = "https://pilot.myvnc.com";
int saved_tz_offset = 0;
bool isAPMode = false;
bool timeSynced = false;

// Ring buffer for logs
String logBuffer = "";

void sysLog(String msg) {
    Serial.println(msg);
    logBuffer += msg + "\n";

    if (logBuffer.length() > 2500) {
        logBuffer = logBuffer.substring(logBuffer.length() - 2500);
    }
}

// =========================================================================
// AP setup HTML
// =========================================================================

const char ap_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>ESP-OOB Setup</title>
<style>
body {
    font-family: sans-serif;
    background: #f0f2f5;
    display: flex;
    justify-content: center;
    align-items: center;
    min-height: 100vh;
    margin: 0;
}
.c {
    background: #fff;
    padding: 25px;
    border-radius: 12px;
    box-shadow: 0 4px 12px rgba(0,0,0,0.1);
    width: 100%;
    max-width: 340px;
}
input, select, button {
    width: 100%;
    padding: 10px;
    margin-bottom: 15px;
    border: 1px solid #ccc;
    border-radius: 6px;
    box-sizing: border-box;
}
button {
    background: #007bff;
    color: #fff;
    border: none;
    cursor: pointer;
    font-weight: bold;
}
.small {
    font-size: 12px;
    color: #666;
    margin-bottom: 15px;
    line-height: 1.35;
}
</style>
</head>
<body>
<div class="c">
    <h2>ESP-OOB Setup</h2>
    <div class="small">Minimum setup: Wi-Fi, password and Telegram numeric ID.</div>

    <form action="/save" method="POST" onsubmit="document.getElementById('tz').value=new Date().getTimezoneOffset();">
        <label>Wi-Fi:</label>
        <select id="s_sel" name="ssid_sel" onchange="chk()">
            <option value="" disabled selected>Scanning...</option>
        </select>

        <input type="text" id="s_man" name="ssid_man" style="display:none;" placeholder="SSID">

        <label>Password:</label>
        <input type="password" name="pass" placeholder="Wi-Fi password">

        <label>Telegram ID:</label>
        <input type="text" name="tgid" placeholder="Example: 123456789" inputmode="numeric">
        <div class="small">Telegram bot token is stored on VPS. ESP stores only your Telegram ID and connects via pilot.myvnc.com.</div>

        <input type="hidden" name="tz_offset" id="tz">

        <button type="submit">Save & Reboot</button>
    </form>
</div>

<script>
fetch('/scan')
.then(r => r.json())
.then(d => {
    let s = document.getElementById('s_sel');
    s.innerHTML = '<option value="" disabled selected>-- Select Wi-Fi --</option>';

    d.forEach(n => {
        let opt = document.createElement('option');
        opt.value = n;
        opt.textContent = n;
        s.appendChild(opt);
    });

    let manual = document.createElement('option');
    manual.value = 'MANUAL';
    manual.textContent = 'Enter manually...';
    s.appendChild(manual);
})
.catch(() => {
    let s = document.getElementById('s_sel');
    s.innerHTML = '<option value="MANUAL">Enter manually...</option>';
    chk();
});

function chk() {
    let sel = document.getElementById('s_sel');
    let man = document.getElementById('s_man');

    if (sel.value === 'MANUAL') {
        sel.style.display = 'none';
        man.style.display = 'block';
        man.focus();
    }
}
</script>
</body>
</html>
)rawliteral";

// =========================================================================
// STA terminal HTML
// =========================================================================

const char cli_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>ESP-OOB Supervisor</title>
<style>
body {
    background-color: #101010;
    color: #00ff00;
    font-family: 'Courier New', monospace;
    margin: 0;
    padding: 10px;
    display: flex;
    flex-direction: column;
    min-height: 100vh;
    box-sizing: border-box;
}
.header {
    display: flex;
    justify-content: space-between;
    gap: 10px;
    align-items: center;
    flex-wrap: wrap;
    margin-bottom: 10px;
    color: #b8ffb8;
}
.title { font-weight: bold; }
.server { color: #888; font-size: 12px; }
.panel {
    background: #1b1b1b;
    border: 1px solid #333;
    border-radius: 8px;
    padding: 10px;
    margin-bottom: 10px;
}
.btn-grid {
    display: grid;
    grid-template-columns: repeat(auto-fit, minmax(135px, 1fr));
    gap: 8px;
}
button {
    background: #303030;
    color: #fff;
    border: 1px solid #555;
    border-radius: 6px;
    padding: 9px 10px;
    cursor: pointer;
    font-family: inherit;
    font-size: 13px;
}
button:hover { background: #444; }
button.danger { background: #4a1515; border-color: #803030; }
button.danger:hover { background: #6a2020; }
button.good { background: #174a22; border-color: #307a3c; }
button.good:hover { background: #20662e; }
#term {
    flex-grow: 1;
    background: #000;
    color: #00ff00;
    border: 1px solid #333;
    padding: 10px;
    overflow-y: auto;
    font-size: 14px;
    resize: none;
    width: 100%;
    box-sizing: border-box;
    min-height: 260px;
}
.input-box { display: flex; margin-top: 10px; }
.prompt { padding: 10px; white-space: nowrap; color: #b8ffb8; }
#cmd {
    flex-grow: 1;
    background: #000;
    color: #00ff00;
    border: 1px solid #333;
    padding: 10px;
    font-family: inherit;
    font-size: 16px;
    outline: none;
    min-width: 0;
}
.tools {
    display: flex;
    justify-content: space-between;
    gap: 10px;
    margin-top: 10px;
    font-size: 12px;
    color: #888;
    flex-wrap: wrap;
}
.ota-box {
    background: #1b1b1b;
    padding: 8px 10px;
    border: 1px dashed #555;
    border-radius: 6px;
}
input[type=file] { color: #aaa; font-size: 12px; max-width: 220px; }
a { color: #00ff00; text-decoration: none; }
a:hover { text-decoration: underline; }
.statusline { color: #888; font-size: 12px; margin-top: 6px; }
</style>
</head>
<body>
    <div class="header">
        <div class="title">OOB-Supervisor Terminal</div>
        <div class="server">VPS: pilot.myvnc.com | <a href="/status" target="_blank">status json</a></div>
    </div>

    <div class="panel">
        <div class="btn-grid">
            <button onclick="runCmd('status')">Status</button>
            <button onclick="runCmd('sync_time')">Sync Time</button>
            <button class="good" onclick="runCmd('tgtest')">Telegram Test</button>
            <button onclick="changeTgId()">Change Telegram ID</button>
            <button onclick="changeServer()">Change Server</button>
            <button onclick="runCmd('clear')">Clear Log</button>
            <button onclick="confirmCmd('reboot', 'Reboot ESP-OOB now?')">Reboot</button>
            <button class="danger" onclick="confirmCmd('resetwifi', 'Erase Wi-Fi settings and reboot to setup mode?')">Reset Wi-Fi</button>
            <button class="danger" onclick="confirmCmd('resetall', 'FULL RESET: erase all settings and reboot?')">Reset All</button>
        </div>
        <div class="statusline" id="actionStatus">Ready.</div>
    </div>

    <textarea id="term" readonly></textarea>

    <div class="input-box">
        <span class="prompt">root@esp:~#</span>
        <input type="text" id="cmd" autofocus autocomplete="off" onkeypress="if(event.key==='Enter') sendCmd()">
    </div>

    <div class="tools">
        <div class="ota-box">
            <span>OTA Update:</span>
            <form method="POST" action="/update" enctype="multipart/form-data" style="display:inline;" id="otaForm">
                <input type="file" name="update" id="file" required>
                <button type="button" onclick="uploadOTA()">Flash .bin</button>
            </form>
            <span id="otaStatus"></span>
        </div>
        <div>Auto-refresh: ON</div>
    </div>

<script>
function refreshLogs() {
    fetch('/api/logs')
    .then(r => r.text())
    .then(t => {
        let b = document.getElementById('term');
        if (b.value !== t) {
            b.value = t;
            b.scrollTop = b.scrollHeight;
        }
    })
    .catch(() => {});
}
setInterval(refreshLogs, 1500);
refreshLogs();
function setActionStatus(text) { document.getElementById('actionStatus').textContent = text; }
function postCommand(command) {
    return fetch('/api/cmd', {
        method: 'POST',
        headers: {'Content-Type': 'application/x-www-form-urlencoded'},
        body: new URLSearchParams({'c': command})
    });
}
function runCmd(command) {
    setActionStatus('Running: ' + command);
    postCommand(command)
    .then(() => { setActionStatus('Done: ' + command); refreshLogs(); })
    .catch(() => { setActionStatus('Failed: ' + command); });
}
function confirmCmd(command, message) {
    if (confirm(message)) runCmd(command);
}
function sendCmd() {
    let i = document.getElementById('cmd');
    let command = i.value.trim();
    if (!command) return;
    runCmd(command);
    i.value = '';
}
function changeTgId() {
    let id = prompt('Enter new Telegram ID:');
    if (id === null) return;
    id = id.trim();
    if (!id) { alert('Telegram ID is empty.'); return; }
    if (!/^-?[0-9]+$/.test(id)) { alert('Telegram ID must be numeric.'); return; }
    runCmd('settg ' + id);
}
function changeServer() {
    let url = prompt('Enter server URL:', 'https://pilot.myvnc.com');
    if (url === null) return;
    url = url.trim();
    if (!url) { alert('Server URL is empty.'); return; }
    if (!url.startsWith('http://') && !url.startsWith('https://')) { alert('URL must start with http:// or https://'); return; }
    runCmd('setserver ' + url);
}
function uploadOTA() {
    let file = document.getElementById('file').files[0];
    if (!file) { alert('Select .bin file!'); return; }
    if (!confirm('Flash selected firmware file?')) return;
    let stat = document.getElementById('otaStatus');
    let fd = new FormData();
    fd.append("update", file);
    let xhr = new XMLHttpRequest();
    xhr.upload.addEventListener("progress", (e) => {
        if (e.lengthComputable) stat.innerHTML = " " + Math.round((e.loaded / e.total) * 100) + "%";
    });
    xhr.addEventListener("load", () => {
        stat.innerHTML = xhr.responseText === "OK" ? " SUCCESS! Rebooting..." : " FAILED!";
        refreshLogs();
    });
    xhr.addEventListener("error", () => { stat.innerHTML = " FAILED!"; });
    xhr.open("POST", "/update");
    xhr.send(fd);
}
</script>
</body>
</html>
)rawliteral";
// =========================================================================
// JSON & Time & Server helpers
// =========================================================================

String jsonEscape(String value) {
    value.replace("\\", "\\\\");
    value.replace("\"", "\\\"");
    value.replace("\n", "\\n");
    value.replace("\r", "\\r");
    return value;
}

String currentTimeString() {
    struct tm timeinfo;

    if (!getLocalTime(&timeinfo, 1000)) {
        return "not synced";
    }

    char buf[32];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &timeinfo);
    return String(buf);
}

bool syncTime() {
    if (WiFi.status() != WL_CONNECTED) {
        sysLog("Time sync failed: WiFi not connected.");
        timeSynced = false;
        return false;
    }

    long gmtOffsetSec = -saved_tz_offset * 60;
    int daylightOffsetSec = 0;

    sysLog("NTP sync started...");
    configTime(gmtOffsetSec, daylightOffsetSec, "pool.ntp.org", "time.nist.gov");

    struct tm timeinfo;
    if (!getLocalTime(&timeinfo, 10000)) {
        sysLog("NTP time sync failed.");
        timeSynced = false;
        return false;
    }

    char buf[32];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &timeinfo);
    sysLog("Time synced: " + String(buf));
    timeSynced = true;
    return true;
}

bool postToServer(String endpoint, String jsonBody) {
    if (WiFi.status() != WL_CONNECTED) {
        sysLog("Server POST failed: WiFi not connected.");
        return false;
    }

    String baseUrl = saved_server_url;
    baseUrl.trim();

    if (baseUrl.endsWith("/")) {
        baseUrl.remove(baseUrl.length() - 1);
    }

    String url = baseUrl + endpoint;
    HTTPClient http;
    int code = -1;
    String payload = "";

    sysLog("POST: " + url);

    if (url.startsWith("https://")) {
        WiFiClientSecure client;
        client.setInsecure();

        if (!http.begin(client, url)) {
            sysLog("HTTPS begin failed.");
            return false;
        }

        http.addHeader("Content-Type", "application/json");
        code = http.POST(jsonBody);
        payload = http.getString();
        http.end();
    } else {
        WiFiClient client;

        if (!http.begin(client, url)) {
            sysLog("HTTP begin failed.");
            return false;
        }

        http.addHeader("Content-Type", "application/json");
        code = http.POST(jsonBody);
        payload = http.getString();
        http.end();
    }

    sysLog("Server HTTP code: " + String(code));
    if (payload.length() > 0) {
        sysLog("Server reply: " + payload);
    }

    return code >= 200 && code < 300;
}

bool sendServerTelegramTest() {
    if (saved_tg_id == "") {
        sysLog("Telegram test failed: Telegram ID is empty.");
        return false;
    }

    String json = "{";
    json += "\"telegram_id\":\"" + jsonEscape(saved_tg_id) + "\",";
    json += "\"device\":\"ESP-OOB\",";
    json += "\"ip\":\"" + WiFi.localIP().toString() + "\",";
    json += "\"rssi\":" + String(WiFi.RSSI()) + ",";
    json += "\"free_heap\":" + String(ESP.getFreeHeap()) + ",";
    json += "\"uptime_ms\":" + String(millis()) + ",";
    json += "\"time\":\"" + jsonEscape(currentTimeString()) + "\",";
    json += "\"message\":\"ESP-OOB test message\"";
    json += "}";

    sysLog("Sending Telegram test via VPS...");
    return postToServer("/api/esp/telegram/test", json);
}

// =========================================================================
// OLED helper
// =========================================================================

void printToScreen(String line1, String line2, String line3) {
#ifdef HAS_OLED
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_helvB08_tr);

    u8g2.setCursor(0, 15);
    u8g2.print(line1);

    u8g2.setCursor(0, 35);
    u8g2.print(line2);

    u8g2.setCursor(0, 55);
    u8g2.print(line3);

    u8g2.sendBuffer();
#else
    (void)line1;
    (void)line2;
    (void)line3;
#endif
}

// =========================================================================
// Start AP mode
// =========================================================================

void startAPMode() {
    isAPMode = true;

    WiFi.disconnect(true);
    delay(500);

    WiFi.mode(WIFI_AP_STA);
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
    WiFi.softAP("ESP-OOB-Setup");

    delay(300);

    dnsServer.start(DNS_PORT, "*", apIP);

    sysLog("AP Mode started.");
    sysLog("SSID: ESP-OOB-Setup");
    sysLog("AP IP: " + WiFi.softAPIP().toString());

    printToScreen("AP: ESP-OOB-Setup", "IP: 192.168.4.1", "Waiting config...");
}

// =========================================================================
// Web server routes
// =========================================================================

void setupWebServer() {
    // Main page
    server.on("/", HTTP_GET, []() {
        if (isAPMode) {
            server.send(200, "text/html", ap_html);
        } else {
            server.send(200, "text/html", cli_html);
        }
    });

    // Status JSON
    server.on("/status", HTTP_GET, []() {
        String json = "{";
        json += "\"mode\":\"" + String(isAPMode ? "AP" : "STA") + "\",";
        json += "\"saved_ssid\":\"" + jsonEscape(saved_ssid) + "\",";
        json += "\"telegram_id\":\"" + jsonEscape(saved_tg_id) + "\",";
        json += "\"server_url\":\"" + jsonEscape(saved_server_url) + "\",";
        json += "\"ap_ip\":\"" + WiFi.softAPIP().toString() + "\",";
        json += "\"sta_ip\":\"" + WiFi.localIP().toString() + "\",";
        json += "\"wifi_status\":" + String(WiFi.status()) + ",";
        json += "\"rssi\":" + String(WiFi.RSSI()) + ",";
        json += "\"time_synced\":" + String(timeSynced ? "true" : "false") + ",";
        json += "\"time\":\"" + jsonEscape(currentTimeString()) + "\",";
        json += "\"free_heap\":" + String(ESP.getFreeHeap()) + ",";
        json += "\"uptime_ms\":" + String(millis());
        json += "}";

        server.send(200, "application/json", json);
    });

    // Logs API
    server.on("/api/logs", HTTP_GET, []() {
        server.send(200, "text/plain", logBuffer);
    });

    // Command API
    server.on("/api/cmd", HTTP_POST, []() {
        String cmd = server.arg("c");
        cmd.trim();

        sysLog("root@esp:~# " + cmd);

        if (cmd == "help") {
            sysLog("Commands: help, clear, ip, status, time, sync_time, tgtest, tgid, settg, setserver, reboot, resetwifi, resetall, nano, mc");
        }
        else if (cmd == "clear") {
            logBuffer = "";
        }
        else if (cmd == "ip") {
            if (isAPMode) {
                sysLog("AP IP: " + WiFi.softAPIP().toString());
            } else {
                sysLog("STA IP: " + WiFi.localIP().toString());
            }
        }
        else if (cmd == "status") {
            sysLog("Mode: " + String(isAPMode ? "AP" : "STA"));
            sysLog("Saved SSID: " + saved_ssid);
            sysLog("AP IP: " + WiFi.softAPIP().toString());
            sysLog("STA IP: " + WiFi.localIP().toString());
            sysLog("WiFi status: " + String(WiFi.status()));
            sysLog("RSSI: " + String(WiFi.RSSI()));
            sysLog("Telegram ID: " + saved_tg_id);
            sysLog("Server URL: " + saved_server_url);
            sysLog("Time synced: " + String(timeSynced ? "yes" : "no"));
            sysLog("Time: " + currentTimeString());
            sysLog("Free heap: " + String(ESP.getFreeHeap()));
            sysLog("Uptime ms: " + String(millis()));
        }
        else if (cmd == "time") {
            sysLog("Time: " + currentTimeString());
        }
        else if (cmd == "sync_time") {
            if (syncTime()) {
                sysLog("Time sync OK.");
            } else {
                sysLog("Time sync failed.");
            }
        }
        else if (cmd == "tgid") {
            sysLog("Telegram ID: " + saved_tg_id);
        }
        else if (cmd.startsWith("settg ")) {
            String id = cmd.substring(6);
            id.trim();

            if (id == "") {
                sysLog("Usage: settg 123456789");
            } else {
                saved_tg_id = id;

                preferences.begin("oob_config", false);
                preferences.putString("tgid", saved_tg_id);
                preferences.end();

                sysLog("Telegram ID saved: " + saved_tg_id);
            }
        }
        else if (cmd.startsWith("setserver ")) {
            String url = cmd.substring(10);
            url.trim();

            if (url == "") {
                sysLog("Usage: setserver https://pilot.myvnc.com");
            }
            else if (!url.startsWith("http://") && !url.startsWith("https://")) {
                sysLog("Invalid server URL. Use http:// or https://");
            }
            else {
                saved_server_url = url;

                preferences.begin("oob_config", false);
                preferences.putString("server", saved_server_url);
                preferences.end();

                sysLog("Server URL saved: " + saved_server_url);
            }
        }
        else if (cmd == "tgtest") {
            if (sendServerTelegramTest()) {
                sysLog("Telegram test request sent successfully.");
            } else {
                sysLog("Telegram test request failed.");
            }
        }
        else if (cmd == "reboot") {
            sysLog("Rebooting...");
            server.send(200, "text/plain", "OK");
            delay(1000);
            ESP.restart();
            return;
        }
        else if (cmd == "resetwifi") {
            sysLog("Erasing WiFi config...");

            preferences.begin("oob_config", false);
            preferences.remove("ssid");
            preferences.remove("pass");
            preferences.end();

            server.send(200, "text/plain", "OK");
            delay(1000);
            ESP.restart();
            return;
        }
        else if (cmd == "resetall") {
            sysLog("Erasing all config...");

            preferences.begin("oob_config", false);
            preferences.clear();
            preferences.end();

            server.send(200, "text/plain", "OK");
            delay(1000);
            ESP.restart();
            return;
        }
        else if (cmd == "nano" || cmd.startsWith("nano ")) {
            sysLog("nano is not available on ESP32. Use web controls instead.");
        }
        else if (cmd == "mc") {
            sysLog("mc is not available on ESP32. File manager can be added later via LittleFS.");
        }
        else {
            sysLog("Error: command not found");
        }

        server.send(200, "text/plain", "OK");
    });

    // OTA update
    server.on("/update", HTTP_POST, []() {
        bool ok = !Update.hasError();

        server.send(200, "text/plain", ok ? "OK" : "FAIL");

        if (ok) {
            delay(1000);
            ESP.restart();
        }
    }, []() {
        HTTPUpload& upload = server.upload();

        if (upload.status == UPLOAD_FILE_START) {
            sysLog("OTA Update started: " + upload.filename);

            if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
                Update.printError(Serial);
                sysLog("OTA Update begin failed.");
            }
        }
        else if (upload.status == UPLOAD_FILE_WRITE) {
            if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
                Update.printError(Serial);
                sysLog("OTA Write error.");
            }
        }
        else if (upload.status == UPLOAD_FILE_END) {
            if (Update.end(true)) {
                sysLog("OTA Update Success. Size: " + String(upload.totalSize));
            } else {
                Update.printError(Serial);
                sysLog("OTA Update failed.");
            }
        }
    });

    // AP mode specific routes
    if (isAPMode) {
        server.on("/scan", HTTP_GET, []() {
            sysLog("WiFi scan requested.");

            int n = WiFi.scanNetworks();
            String json = "[";

            for (int i = 0; i < n; ++i) {
                if (i > 0) json += ",";

                String ssid = WiFi.SSID(i);

                // Basic JSON escaping for quotes and backslashes
                ssid.replace("\\", "\\\\");
                ssid.replace("\"", "\\\"");

                json += "\"" + ssid + "\"";
            }

            json += "]";

            server.send(200, "application/json", json);
        });

        server.on("/save", HTTP_POST, []() {
            String ssid_sel = server.arg("ssid_sel");
            String ssid_man = server.arg("ssid_man");
            String new_ssid = ssid_man != "" ? ssid_man : ssid_sel;
            String new_pass = server.arg("pass");
            String new_tg_id = server.arg("tgid");
            int new_tz = server.arg("tz_offset").toInt();

            new_tg_id.trim();
            new_ssid.trim();

            if (new_ssid == "" || new_ssid == "MANUAL") {
                server.send(400, "text/html", "<meta charset='UTF-8'><h2>Error: SSID is empty</h2><a href='/'>Back</a>");
                return;
            }

            preferences.begin("oob_config", false);
            preferences.putString("ssid", new_ssid);
            preferences.putString("pass", new_pass);
            preferences.putString("tgid", new_tg_id);
            preferences.putString("server", saved_server_url);
            preferences.putInt("tz", new_tz);
            preferences.end();

            sysLog("New WiFi config saved.");
            sysLog("SSID: " + new_ssid);
            sysLog("Telegram ID: " + new_tg_id);
            sysLog("Rebooting...");

            server.send(200, "text/html", "<meta charset='UTF-8'><h2>OK! Rebooting...</h2>");
            delay(2000);
            ESP.restart();
        });
    }

    // Captive portal / fallback
    server.onNotFound([]() {
        if (isAPMode) {
            server.sendHeader("Location", "http://192.168.4.1/", true);
            server.send(302, "text/plain", "");
        } else {
            server.send(404, "text/plain", "Not found");
        }
    });

    server.begin();
    sysLog("HTTP server started on port 80.");
}

// =========================================================================
// Setup
// =========================================================================

void setup() {
    Serial.begin(115200);
    delay(1000);

    sysLog("");
    sysLog("=== OOB Supervisor Started ===");

#ifdef HAS_OLED
    u8g2.begin();
#endif

    preferences.begin("oob_config", true);
    saved_ssid = preferences.getString("ssid", "");
    saved_pass = preferences.getString("pass", "");
    saved_tg_id = preferences.getString("tgid", "");
    saved_server_url = preferences.getString("server", "https://pilot.myvnc.com");
    saved_tz_offset = preferences.getInt("tz", 0);
    preferences.end();

    if (saved_ssid == "") {
        sysLog("No WiFi config found.");
        startAPMode();
        setupWebServer();
    } else {
        isAPMode = false;

        WiFi.mode(WIFI_STA);
        WiFi.begin(saved_ssid.c_str(), saved_pass.c_str());

        sysLog("Connecting to WiFi...");
        sysLog("SSID: " + saved_ssid);

        printToScreen("Connecting WiFi", saved_ssid, "Please wait...");

        int attempts = 0;

        while (WiFi.status() != WL_CONNECTED && attempts < 30) {
            delay(500);
            Serial.print(".");
            attempts++;
        }

        Serial.println();

        if (WiFi.status() == WL_CONNECTED) {
            sysLog("WiFi Connected.");
            sysLog("STA IP: " + WiFi.localIP().toString());
            sysLog("Gateway: " + WiFi.gatewayIP().toString());
            sysLog("RSSI: " + String(WiFi.RSSI()));

            syncTime();

            printToScreen("WiFi Connected", "IP: " + WiFi.localIP().toString(), saved_tg_id == "" ? "TG: not set" : "TG: configured");

            setupWebServer();

            if (saved_tg_id != "") {
                sendServerTelegramTest();
            }
        } else {
            sysLog("WiFi connection failed.");
            sysLog("Erasing config and starting AP mode.");

            printToScreen("WiFi Failed", "Starting AP", "192.168.4.1");

            preferences.begin("oob_config", false);
            preferences.clear();
            preferences.end();

            saved_ssid = "";
            saved_pass = "";
            saved_tz_offset = 0;

            startAPMode();
            setupWebServer();
        }
    }
}

// =========================================================================
// Main loop
// =========================================================================

void loop() {
    if (isAPMode) {
        dnsServer.processNextRequest();
    }

    server.handleClient();
}