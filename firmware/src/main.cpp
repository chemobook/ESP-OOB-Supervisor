#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>

// --- Подключение экрана (скомпилируется только если в platformio.ini указан HAS_OLED) ---
#ifdef HAS_OLED
#include <U8g2lib.h>
#include <Wire.h>
// Инициализация OLED 128x64 без пина Reset (Специально для плат Wemos/Lolin с батареей)
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE, /* clock=*/ OLED_SCL, /* data=*/ OLED_SDA);
#endif

const byte DNS_PORT = 53;
IPAddress apIP(192, 168, 4, 1);
DNSServer dnsServer;
WebServer server(80);

// HTML-страница веб-морды (хранится во флеш-памяти для экономии RAM)
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="ru">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-width=1.0">
    <title>Настройка ESP-OOB</title>
    <style>
        body { font-family: -apple-system, sans-serif; background-color: #f0f2f5; display: flex; justify-content: center; align-items: center; height: 100vh; margin: 0; }
        .container { background: #fff; padding: 25px; border-radius: 12px; box-shadow: 0 4px 12px rgba(0,0,0,0.1); width: 100%; max-width: 320px; }
        h2 { text-align: center; color: #333; margin-top: 0; }
        label { font-size: 14px; color: #666; margin-bottom: 5px; display: block; }
        input { width: 100%; padding: 10px; margin-bottom: 15px; border: 1px solid #ccc; border-radius: 6px; box-sizing: border-box; font-size: 16px; }
        button { width: 100%; padding: 12px; background-color: #007bff; color: white; border: none; border-radius: 6px; font-size: 16px; cursor: pointer; font-weight: bold; }
        button:hover { background-color: #0056b3; }
    </style>
</head>
<body>
    <div class="container">
        <h2>ESP-OOB Setup</h2>
        <form action="/save" method="POST">
            <label>Домашний Wi-Fi (SSID):</label>
            <input type="text" name="ssid" placeholder="MyWiFi" required>
            
            <label>Пароль Wi-Fi:</label>
            <input type="password" name="pass" placeholder="Password">
            
            <label>Ваш Telegram ID:</label>
            <input type="number" name="tid" placeholder="123456789" required>
            
            <label>URL Сервера:</label>
            <input type="url" name="server_url" value="ws://your-vps-ip:8000/ws" required>
            
            <button type="submit">Сохранить и Перезагрузить</button>
        </form>
    </div>
</body>
</html>
)rawliteral";

// Функция для вывода текста на экран (если он есть)
void printToScreen(String line1, String line2, String line3) {
#ifdef HAS_OLED
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_helvB08_tr); // Красивый читаемый шрифт
    u8g2.setCursor(0, 15); u8g2.print(line1);
    u8g2.setCursor(0, 35); u8g2.print(line2);
    u8g2.setCursor(0, 55); u8g2.print(line3);
    u8g2.sendBuffer();
#endif
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    // Запускаем экран, если плата его поддерживает
#ifdef HAS_OLED
    u8g2.begin();
    printToScreen("Starting...", "Please wait", "");
    delay(1000);
#endif

    // Настраиваем точку доступа
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
    WiFi.softAP("ESP-OOB-Setup");
    
    // Выводим данные на экран
    printToScreen("AP: ESP-OOB-Setup", "IP: 192.168.4.1", "Waiting for config...");

    dnsServer.start(DNS_PORT, "*", apIP);

    server.on("/", HTTP_GET, []() {
        server.send(200, "text/html", index_html);
    });

    server.on("/save", HTTP_POST, []() {
        String ssid = server.arg("ssid");
        String pass = server.arg("pass");
        String tid = server.arg("tid");
        String server_url = server.arg("server_url");

        // Показываем на экране, что данные получены
        printToScreen("Config Saved!", "SSID: " + ssid, "Rebooting...");

        Serial.println("\n--- Настройки сохранены ---");
        String response = "<!DOCTYPE html><html><body style='font-family:sans-serif;text-align:center;margin-top:50px;'><h2>Настройки сохранены!</h2><p>Устройство перезагружается...</p></body></html>";
        server.send(200, "text/html", response);
    });

    server.onNotFound([]() {
        server.sendHeader("Location", "/", true);
        server.send(302, "text/plain", "");
    });

    server.begin();
}

void loop() {
    dnsServer.processNextRequest();
    server.handleClient();
}