#include "wifi_debug.h"

#include <Arduino.h>
#include <ArduinoOTA.h>
#include <Update.h>
#include <WebServer.h>
#include <WiFi.h>
#include <esp_system.h>

#include "ble_control.h"
#include "protocol.h"

static WebServer server(80);

static const char *reset_reason_text() {
    switch (esp_reset_reason()) {
    case ESP_RST_POWERON:
        return "power-on";
    case ESP_RST_SW:
        return "software";
    case ESP_RST_PANIC:
        return "panic";
    case ESP_RST_INT_WDT:
    case ESP_RST_TASK_WDT:
    case ESP_RST_WDT:
        return "watchdog";
    case ESP_RST_BROWNOUT:
        return "brownout";
    case ESP_RST_SDIO:
        return "sdio";
    default:
        return "other";
    }
}

static void send_status_page() {
    int8_t left = 0;
    int8_t right = 0;
    ble_get_command(&left, &right);
    const uint32_t last = ble_last_drive_ms();
    const uint32_t age = last == 0 ? 0 : (millis() - last);

    String html;
    html.reserve(1800);
    html += "<!DOCTYPE html><html><head><meta charset='utf-8'/>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1'/>";
    html += "<meta http-equiv='refresh' content='2'/>";
    html += "<title>Dozer debug</title>";
    html += "<style>body{font-family:system-ui,sans-serif;background:#121212;color:#f2f2f2;";
    html += "max-width:40rem;margin:1.5rem auto;padding:0 1rem}a,button{color:#111;";
    html += "background:#f5a623;border:0;border-radius:8px;padding:.6rem 1rem;";
    html += "font-weight:700;text-decoration:none;display:inline-block}";
    html += "dl{display:grid;grid-template-columns:9rem 1fr;gap:.35rem 1rem}";
    html += "dt{color:#9a9a9a}code{color:#f5a623}form{margin-top:1.25rem}</style></head><body>";
    html += "<h1>Dozer</h1><dl>";
    html += "<dt>Uptime</dt><dd>";
    html += String(millis() / 1000);
    html += " s</dd><dt>Reset</dt><dd>";
    html += reset_reason_text();
    html += "</dd><dt>BLE</dt><dd>";
    html += ble_is_connected() ? "connected" : "disconnected";
    html += "</dd><dt>Drive</dt><dd>L ";
    html += String(left);
    html += " &nbsp; R ";
    html += String(right);
    html += "</dd><dt>Last cmd</dt><dd>";
    html += last == 0 ? "none" : (String(age) + " ms ago");
    html += "</dd><dt>Heap</dt><dd>";
    html += String(ESP.getFreeHeap());
    html += " / ";
    html += String(ESP.getHeapSize());
    html += "</dd><dt>PSRAM</dt><dd>";
    html += String(ESP.getFreePsram());
    html += " / ";
    html += String(ESP.getPsramSize());
    html += "</dd><dt>AP</dt><dd>";
    html += WiFi.softAPIP().toString();
    html += " &nbsp; ";
    html += String(WiFi.softAPgetStationNum());
    html += " client(s)</dd></dl>";
    html += "<p>SSID <code>";
    html += WIFI_AP_SSID;
    html += "</code> &nbsp; password <code>";
    html += WIFI_AP_PASS;
    html += "</code></p>";
    html += "<h2>OTA</h2><p>Upload a <code>firmware.bin</code> from PlatformIO "
            "<code>.pio/build/...</code></p>";
    html += "<form method='POST' action='/update' enctype='multipart/form-data'>";
    html += "<input type='file' name='firmware' accept='.bin'/> ";
    html += "<button type='submit'>Update</button></form>";
    html += "<p><a href='/reboot'>Reboot</a></p></body></html>";
    server.send(200, "text/html", html);
}

static void send_json() {
    int8_t left = 0;
    int8_t right = 0;
    ble_get_command(&left, &right);
    const uint32_t last = ble_last_drive_ms();
    String json = "{";
    json += "\"uptime_ms\":" + String(millis());
    json += ",\"ble\":" + String(ble_is_connected() ? "true" : "false");
    json += ",\"left\":" + String(left);
    json += ",\"right\":" + String(right);
    json += ",\"last_drive_ms\":" + String(last);
    json += ",\"heap\":" + String(ESP.getFreeHeap());
    json += ",\"clients\":" + String(WiFi.softAPgetStationNum());
    json += "}";
    server.send(200, "application/json", json);
}

static void handle_update_finish() {
    server.sendHeader("Connection", "close");
    if (Update.hasError()) {
        server.send(500, "text/plain", "OTA failed");
        Serial.println("[ota] web update failed");
    } else {
        server.send(200, "text/plain", "OTA ok, rebooting");
        Serial.println("[ota] web update ok, reboot");
        delay(500);
        ESP.restart();
    }
}

static void handle_update_upload() {
    HTTPUpload &upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
        Serial.printf("[ota] start %s\n", upload.filename.c_str());
        if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
            Update.printError(Serial);
        }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
            Update.printError(Serial);
        }
    } else if (upload.status == UPLOAD_FILE_END) {
        if (Update.end(true)) {
            Serial.printf("[ota] %u bytes written\n", upload.totalSize);
        } else {
            Update.printError(Serial);
        }
    }
}

void wifi_debug_begin() {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASS);
    delay(100);

    ArduinoOTA.setHostname("dozer");
    ArduinoOTA.onStart([]() { Serial.println("[ota] ArduinoOTA start"); });
    ArduinoOTA.onEnd([]() { Serial.println("[ota] ArduinoOTA end"); });
    ArduinoOTA.onError([](ota_error_t err) {
        Serial.printf("[ota] ArduinoOTA error %u\n", err);
    });
    ArduinoOTA.begin();

    server.on("/", HTTP_GET, send_status_page);
    server.on("/status.json", HTTP_GET, send_json);
    server.on(
        "/update", HTTP_POST, handle_update_finish, handle_update_upload);
    server.on("/reboot", HTTP_GET, []() {
        server.send(200, "text/plain", "rebooting");
        delay(200);
        ESP.restart();
    });
    server.begin();

    Serial.printf("[wifi] AP SSID %s  pass %s  http://%s/\n", WIFI_AP_SSID,
                  WIFI_AP_PASS, WiFi.softAPIP().toString().c_str());
}

void wifi_debug_loop() {
    ArduinoOTA.handle();
    server.handleClient();
}
