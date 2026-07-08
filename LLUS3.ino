#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include <Adafruit_GFX.h>    
#include <Adafruit_ST7789.h> 
#include <time.h>

void init_display_hard(); 
void handle_clear(); 
void try_multi_wifi();
void update_display();

Adafruit_ST7789 tft = Adafruit_ST7789(&SPI, 3, 4, 5); 
String wifi_ssid="", wifi_password="", wifi_ssid2="", wifi_password2="", wifi_ssid3="", wifi_password3="";
String llu_email="", llu_password="", glucose_value="--";
String jwt_token="", account_id_hash="", last_reading_time="Never", min_time="--:--", max_time="--:--";
WebServer server(80); DNSServer dnsServer; Preferences prefs;
unsigned long last_fetch = 0; const int MAX_R = 15;
int glucose_hist[MAX_R]={0}, r_count=0, min_g=999, max_g=0;

void fetch_glucose();
String get_time(bool include_seconds) { struct tm ti; if(!getLocalTime(&ti)) return include_seconds ? "--:--:--" : "--:--"; char b[12]; strftime(b, sizeof(b), include_seconds ? "%H:%M:%S" : "%H:%M", &ti); return String(b); }

void init_display_hard() { SPI.begin(7, -1, 6, 3); tft.init(240, 320, SPI_MODE3); tft.setRotation(1); tft.invertDisplay(true); tft.fillScreen(0x0000); }

void try_multi_wifi() {
    String ssids[3] = {wifi_ssid, wifi_ssid2, wifi_ssid3};
    String passes[3] = {wifi_password, wifi_password2, wifi_password3};
    int n = WiFi.scanNetworks();
    for (int i = 0; i < n; i++) {
        String available_ssid = WiFi.SSID(i);
        for (int j = 0; j < 3; j++) {
            if (ssids[j] != "" && available_ssid == ssids[j]) {
                WiFi.begin(ssids[j].c_str(), passes[j].c_str());
                int c = 0; while (WiFi.status() != WL_CONNECTED && c++ < 20) delay(250);
                if (WiFi.status() == WL_CONNECTED) return;
            }
        }
    }
    for (int j = 0; j < 3; j++) {
        if (ssids[j] != "") {
            WiFi.begin(ssids[j].c_str(), passes[j].c_str());
            int c = 0; while (WiFi.status() != WL_CONNECTED && c++ < 16) delay(250);
            if (WiFi.status() == WL_CONNECTED) return;
        }
    }
}

void setup() {
    Serial.begin(115200); setCpuFrequencyMhz(80); init_display_hard();
    prefs.begin("cgm-config", false);
    wifi_ssid = prefs.getString("w_ssid", ""); wifi_password = prefs.getString("w_pass", "");
    wifi_ssid2 = prefs.getString("w_ssid2", ""); wifi_password2 = prefs.getString("w_pass2", "");
    wifi_ssid3 = prefs.getString("w_ssid3", ""); wifi_password3 = prefs.getString("w_pass3", "");
    llu_email = prefs.getString("email", ""); llu_password = prefs.getString("pass", ""); prefs.end();
    WiFi.softAP("CGM", "u/SiLA820", 1, 0, 4, WIFI_AUTH_WPA2_PSK); dnsServer.start(53, "*", IPAddress(192, 168, 4, 1)); 
    tft.setCursor(30, 110); tft.setTextColor(0xFFDF); tft.setTextSize(2);
    if (wifi_ssid != "" || wifi_ssid2 != "" || wifi_ssid3 != "") { tft.print("WiFi..."); try_multi_wifi(); }
    init_display_hard();
    if (WiFi.status() == WL_CONNECTED) { configTime(0, 3600, "pool.ntp.org", "://google.com"); fetch_glucose(); } else { update_display(); }
    server.on("/", handle_root); server.on("/save", handle_save); server.on("/clear", handle_clear); server.onNotFound(handle_root); 
    server.begin(); last_fetch = millis();
}

void loop() {
    dnsServer.processNextRequest(); server.handleClient();
    if ((wifi_ssid != "" || wifi_ssid2 != "" || wifi_ssid3 != "") && WiFi.status() != WL_CONNECTED) {
        WiFi.disconnect(); try_multi_wifi();
        unsigned long start_t = millis(); while (WiFi.status() != WL_CONNECTED && millis() - start_t < 10000) delay(100);
    }
    static unsigned long last_second_millis = 0;
    if (millis() - last_second_millis >= 1000) {
        last_second_millis = millis(); tft.setTextSize(3); tft.setTextColor(0x07E0);
        tft.fillRect(88, 212, 150, 24, 0x0000); tft.setCursor(88, 212); tft.print(get_time(true)); 
    }
    if (WiFi.status() == WL_CONNECTED && (millis() - last_fetch > 60000)) { fetch_glucose(); last_fetch = millis(); }
    delay(10);
}
