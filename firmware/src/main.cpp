#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <Update.h> // OTA update

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
int saved_tz_offset = 0;
bool isAPMode = false;

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
    max-width: 320px;
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
}
</style>
</head>
<body>
<div class="c">
    <h2>ESP-OOB Setup</h2>
    <div class="small">Connect ESP-OOB to your local Wi-Fi network.</div>

    <form action="/save" method="POST" onsubmit="document.getElementById('tz').value=new Date().getTimezoneOffset();">
        <label>Wi-Fi:</label>
        <select id="s_sel" name="ssid_sel" onchange="chk()">
            <option value="" disabled selected>Scanning...</option>
        </select>

        <input type="text" id="s_man" name="ssid_man" style="display:none;" placeholder="SSID">

        <label>Password:</label>
        <input type="password" name="pass" placeholder="Wi-Fi password">

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
<title>ESP-OOB Terminal</title>
<style>
body {
    background-color: #121212;
    color: #00ff00;
    font-family: 'Courier New', monospace;
    margin: 0;
    padding: 10px;
    display: flex;
    flex-direction: column;
    min-height: 100vh;
    box-sizing: border-box;
}
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
}
.input-box {
    display: flex;
    margin-top: 10px;
}
.prompt {
    padding: 10px;
    white-space: nowrap;
}
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
    background: #222;
    padding: 5px 10px;
    border: 1px dashed #555;
    border-radius: 4px;
}
input[type=file] {
    color: #aaa;
    font-size: 12px;
    max-width: 220px;
}
button {
    background: #444;
    color: #fff;
    border: none;
    padding: 5px 10px;
    cursor: pointer;
    font-family: inherit;
}
button:hover {
    background: #666;
}
a {
    color: #00ff00;
}
</style>
</head>
<body>
    <div>OOB-Supervisor // Type 'help' for commands</div>

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

        <div>
            Auto-refresh: ON |
            <a href="/status" target="_blank">status</a>
        </div>
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

function sendCmd() {
    let i = document.getElementById('cmd');

    if (!i.value) return;

    fetch('/api/cmd', {
        method: 'POST',
        headers: {'Content-Type': 'application/x-www-form-urlencoded'},
        body: new URLSearchParams({'c': i.value})
    })
    .then(() => refreshLogs())
    .catch(() => {});

    i.value = '';
}

function uploadOTA() {
    let file = document.getElementById('file').files[0];

    if (!file) {
        alert('Select .bin file!');
        return;
    }

    let stat = document.getElementById('otaStatus');
    let fd = new FormData();

    fd.append("update", file);

    let xhr = new XMLHttpRequest();

    xhr.upload.addEventListener("progress", (e) => {
        if (e.lengthComputable) {
            stat.innerHTML = " " + Math.round((e.loaded / e.total) * 100) + "%";
        }
    });

    xhr.addEventListener("load", () => {
        stat.innerHTML = xhr.responseText === "OK" ? " SUCCESS! Rebooting..." : " FAILED!";
    });

    xhr.addEventListener("error", () => {
        stat.innerHTML = " FAILED!";
    });

    xhr.open("POST", "/update");
    xhr.send(fd);
}
</script>
</body>
</html>
)rawliteral";

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
        json += "\"saved_ssid\":\"" + saved_ssid + "\",";
        json += "\"ap_ip\":\"" + WiFi.softAPIP().toString() + "\",";
        json += "\"sta_ip\":\"" + WiFi.localIP().toString() + "\",";
        json += "\"wifi_status\":" + String(WiFi.status()) + ",";
        json += "\"rssi\":" + String(WiFi.RSSI());
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
            sysLog("Commands: help, clear, ip, status, reboot, resetwifi");
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
            preferences.clear();
            preferences.end();

            server.send(200, "text/plain", "OK");
            delay(1000);
            ESP.restart();
            return;
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
            int new_tz = server.arg("tz_offset").toInt();

            new_ssid.trim();

            if (new_ssid == "" || new_ssid == "MANUAL") {
                server.send(400, "text/html", "<meta charset='UTF-8'><h2>Error: SSID is empty</h2><a href='/'>Back</a>");
                return;
            }

            preferences.begin("oob_config", false);
            preferences.putString("ssid", new_ssid);
            preferences.putString("pass", new_pass);
            preferences.putInt("tz", new_tz);
            preferences.end();

            sysLog("New WiFi config saved.");
            sysLog("SSID: " + new_ssid);
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

            printToScreen("WiFi Connected", "IP: " + WiFi.localIP().toString(), "Ready");

            setupWebServer();
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