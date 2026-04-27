{\rtf1\ansi\ansicpg1251\cocoartf2869
\cocoatextscaling0\cocoaplatform0{\fonttbl\f0\fswiss\fcharset0 Helvetica;}
{\colortbl;\red255\green255\blue255;}
{\*\expandedcolortbl;;}
\paperw11900\paperh16840\margl1440\margr1440\vieww11520\viewh8400\viewkind0
\pard\tx720\tx1440\tx2160\tx2880\tx3600\tx4320\tx5040\tx5760\tx6480\tx7200\tx7920\tx8640\pardirnatural\partightenfactor0

\f0\fs24 \cf0 #include <Arduino.h>\
#include <WiFi.h>\
#include <WebServer.h>\
#include <DNSServer.h>\
\
const byte DNS_PORT = 53;\
IPAddress apIP(192, 168, 4, 1);\
DNSServer dnsServer;\
WebServer server(80);\
\
// HTML-\uc0\u1089 \u1090 \u1088 \u1072 \u1085 \u1080 \u1094 \u1072  \u1074 \u1077 \u1073 -\u1084 \u1086 \u1088 \u1076 \u1099  (\u1093 \u1088 \u1072 \u1085 \u1080 \u1090 \u1089 \u1103  \u1074 \u1086  \u1092 \u1083 \u1077 \u1096 -\u1087 \u1072 \u1084 \u1103 \u1090 \u1080  \u1076 \u1083 \u1103  \u1101 \u1082 \u1086 \u1085 \u1086 \u1084 \u1080 \u1080  RAM)\
const char index_html[] PROGMEM = R"rawliteral(\
<!DOCTYPE html>\
<html lang="ru">\
<head>\
    <meta charset="UTF-8">\
    <meta name="viewport" content="width=device-width, initial-width=1.0">\
    <title>\uc0\u1053 \u1072 \u1089 \u1090 \u1088 \u1086 \u1081 \u1082 \u1072  ESP-OOB</title>\
    <style>\
        body \{ font-family: -apple-system, sans-serif; background-color: #f0f2f5; display: flex; justify-content: center; align-items: center; height: 100vh; margin: 0; \}\
        .container \{ background: #fff; padding: 25px; border-radius: 12px; box-shadow: 0 4px 12px rgba(0,0,0,0.1); width: 100%; max-width: 320px; \}\
        h2 \{ text-align: center; color: #333; margin-top: 0; \}\
        label \{ font-size: 14px; color: #666; margin-bottom: 5px; display: block; \}\
        input \{ width: 100%; padding: 10px; margin-bottom: 15px; border: 1px solid #ccc; border-radius: 6px; box-sizing: border-box; font-size: 16px; \}\
        button \{ width: 100%; padding: 12px; background-color: #007bff; color: white; border: none; border-radius: 6px; font-size: 16px; cursor: pointer; font-weight: bold; \}\
        button:hover \{ background-color: #0056b3; \}\
    </style>\
</head>\
<body>\
    <div class="container">\
        <h2>ESP-OOB Setup</h2>\
        <form action="/save" method="POST">\
            <label>\uc0\u1044 \u1086 \u1084 \u1072 \u1096 \u1085 \u1080 \u1081  Wi-Fi (SSID):</label>\
            <input type="text" name="ssid" placeholder="MyWiFi" required>\
            \
            <label>\uc0\u1055 \u1072 \u1088 \u1086 \u1083 \u1100  Wi-Fi:</label>\
            <input type="password" name="pass" placeholder="Password">\
            \
            <label>\uc0\u1042 \u1072 \u1096  Telegram ID:</label>\
            <input type="number" name="tid" placeholder="123456789" required>\
            \
            <label>URL \uc0\u1057 \u1077 \u1088 \u1074 \u1077 \u1088 \u1072 :</label>\
            <input type="url" name="server_url" value="ws://your-vps-ip:8000/ws" required>\
            \
            <button type="submit">\uc0\u1057 \u1086 \u1093 \u1088 \u1072 \u1085 \u1080 \u1090 \u1100  \u1080  \u1055 \u1077 \u1088 \u1077 \u1079 \u1072 \u1075 \u1088 \u1091 \u1079 \u1080 \u1090 \u1100 </button>\
        </form>\
    </div>\
</body>\
</html>\
)rawliteral";\
\
void setup() \{\
    Serial.begin(115200);\
    delay(1000);\
    Serial.println("\\n--- \uc0\u1047 \u1072 \u1087 \u1091 \u1089 \u1082  ESP-OOB Supervisor ---");\
\
    // \uc0\u1047 \u1072 \u1087 \u1091 \u1089 \u1082 \u1072 \u1077 \u1084  \u1090 \u1086 \u1095 \u1082 \u1091  \u1076 \u1086 \u1089 \u1090 \u1091 \u1087 \u1072 \
    WiFi.mode(WIFI_AP);\
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));\
    WiFi.softAP("ESP-OOB-Setup"); // \uc0\u1048 \u1084 \u1103  \u1089 \u1077 \u1090 \u1080 , \u1082 \u1086 \u1090 \u1086 \u1088 \u1086 \u1077  \u1074 \u1099  \u1091 \u1074 \u1080 \u1076 \u1080 \u1090 \u1077  \u1074  \u1090 \u1077 \u1083 \u1077 \u1092 \u1086 \u1085 \u1077 \
    \
    Serial.println("Wi-Fi \uc0\u1058 \u1086 \u1095 \u1082 \u1072  \u1076 \u1086 \u1089 \u1090 \u1091 \u1087 \u1072  \u1089 \u1086 \u1079 \u1076 \u1072 \u1085 \u1072 : ESP-OOB-Setup");\
    Serial.print("IP \uc0\u1072 \u1076 \u1088 \u1077 \u1089 : ");\
    Serial.println(WiFi.softAPIP());\
\
    // \uc0\u1047 \u1072 \u1087 \u1091 \u1089 \u1082 \u1072 \u1077 \u1084  DNS-\u1089 \u1077 \u1088 \u1074 \u1077 \u1088 , \u1082 \u1086 \u1090 \u1086 \u1088 \u1099 \u1081  \u1087 \u1077 \u1088 \u1077 \u1085 \u1072 \u1087 \u1088 \u1072 \u1074 \u1083 \u1103 \u1077 \u1090  \u1042 \u1057 \u1045  \u1076 \u1086 \u1084 \u1077 \u1085 \u1099  \u1085 \u1072  \u1085 \u1072 \u1096 \u1091  ESP\
    dnsServer.start(DNS_PORT, "*", apIP);\
\
    // \uc0\u1054 \u1073 \u1088 \u1072 \u1073 \u1086 \u1090 \u1082 \u1072  \u1075 \u1083 \u1072 \u1074 \u1085 \u1086 \u1081  \u1089 \u1090 \u1088 \u1072 \u1085 \u1080 \u1094 \u1099 \
    server.on("/", HTTP_GET, []() \{\
        server.send(200, "text/html", index_html);\
    \});\
\
    // \uc0\u1054 \u1073 \u1088 \u1072 \u1073 \u1086 \u1090 \u1082 \u1072  \u1089 \u1086 \u1093 \u1088 \u1072 \u1085 \u1077 \u1085 \u1080 \u1103  \u1085 \u1072 \u1089 \u1090 \u1088 \u1086 \u1077 \u1082 \
    server.on("/save", HTTP_POST, []() \{\
        String ssid = server.arg("ssid");\
        String pass = server.arg("pass");\
        String tid = server.arg("tid");\
        String server_url = server.arg("server_url");\
\
        // \uc0\u1055 \u1086 \u1082 \u1072  \u1087 \u1088 \u1086 \u1089 \u1090 \u1086  \u1074 \u1099 \u1074 \u1086 \u1076 \u1080 \u1084  \u1074  \u1082 \u1086 \u1085 \u1089 \u1086 \u1083 \u1100 . \u1047 \u1072 \u1074 \u1090 \u1088 \u1072  \u1076 \u1086 \u1073 \u1072 \u1074 \u1080 \u1084  \u1089 \u1086 \u1093 \u1088 \u1072 \u1085 \u1077 \u1085 \u1080 \u1077  \u1074  EEPROM\
        Serial.println("\\n--- \uc0\u1055 \u1086 \u1083 \u1091 \u1095 \u1077 \u1085 \u1099  \u1085 \u1086 \u1074 \u1099 \u1077  \u1085 \u1072 \u1089 \u1090 \u1088 \u1086 \u1081 \u1082 \u1080 ! ---");\
        Serial.println("SSID: " + ssid);\
        Serial.println("PASS: " + pass);\
        Serial.println("Telegram ID: " + tid);\
        Serial.println("Server URL: " + server_url);\
        Serial.println("---------------------------------");\
\
        String response = "<!DOCTYPE html><html><body style='font-family:sans-serif;text-align:center;margin-top:50px;'><h2>\uc0\u1053 \u1072 \u1089 \u1090 \u1088 \u1086 \u1081 \u1082 \u1080  \u1089 \u1086 \u1093 \u1088 \u1072 \u1085 \u1077 \u1085 \u1099 !</h2><p>\u1059 \u1089 \u1090 \u1088 \u1086 \u1081 \u1089 \u1090 \u1074 \u1086  \u1087 \u1077 \u1088 \u1077 \u1079 \u1072 \u1075 \u1088 \u1091 \u1078 \u1072 \u1077 \u1090 \u1089 \u1103 ...</p></body></html>";\
        server.send(200, "text/html", response);\
        \
        // \uc0\u1058 \u1091 \u1090  \u1087 \u1086 \u1090 \u1086 \u1084  \u1073 \u1091 \u1076 \u1077 \u1090  \u1083 \u1086 \u1075 \u1080 \u1082 \u1072  \u1087 \u1077 \u1088 \u1077 \u1079 \u1072 \u1075 \u1088 \u1091 \u1079 \u1082 \u1080 : ESP.restart();\
    \});\
\
    // \uc0\u1045 \u1089 \u1083 \u1080  \u1090 \u1077 \u1083 \u1077 \u1092 \u1086 \u1085  \u1079 \u1072 \u1087 \u1088 \u1072 \u1096 \u1080 \u1074 \u1072 \u1077 \u1090  \u1083 \u1102 \u1073 \u1091 \u1102  \u1076 \u1088 \u1091 \u1075 \u1091 \u1102  \u1089 \u1090 \u1088 \u1072 \u1085 \u1080 \u1094 \u1091  - \u1087 \u1077 \u1088 \u1077 \u1082 \u1080 \u1076 \u1099 \u1074 \u1072 \u1077 \u1084  \u1085 \u1072  \u1075 \u1083 \u1072 \u1074 \u1085 \u1091 \u1102  (\u1076 \u1083 \u1103  \u1088 \u1072 \u1073 \u1086 \u1090 \u1099  Captive Portal)\
    server.onNotFound([]() \{\
        server.sendHeader("Location", "/", true);\
        server.send(302, "text/plain", "");\
    \});\
\
    server.begin();\
    Serial.println("Web-\uc0\u1089 \u1077 \u1088 \u1074 \u1077 \u1088  \u1079 \u1072 \u1087 \u1091 \u1097 \u1077 \u1085 !");\
\}\
\
void loop() \{\
    dnsServer.processNextRequest();\
    server.handleClient();\
\}}