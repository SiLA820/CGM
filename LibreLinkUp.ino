#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Adafruit_ST7789.h>

extern Adafruit_ST7789 tft;
extern String wifi_ssid, wifi_password, llu_email, llu_password, glucose_value;
extern String jwt_token, account_id_hash, last_reading_time, min_time, max_time;
extern int glucose_hist[], r_count, min_g, max_g;
void update_display(); 

String get_api_url() { return "https://api-eu.libreview.io"; }

String sha256(String payload) {
    byte shaResult[32]; mbedtls_md_context_t ctx; mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;
    mbedtls_md_init(&ctx); mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 0); mbedtls_md_starts(&ctx);
    mbedtls_md_update(&ctx, (const unsigned char*) payload.c_str(), payload.length()); mbedtls_md_finish(&ctx, shaResult); mbedtls_md_free(&ctx);
    String hashStr = "";
    for(int i=0; i<32; i++) { if(shaResult[i]<16) hashStr += "0"; hashStr += String(shaResult[i], HEX); }
    return hashStr;
}

bool login_llu() {
    if (llu_email == "" || llu_password == "") return false;
    WiFiClientSecure client; client.setInsecure(); HTTPClient http;
    http.begin(client, get_api_url() + "/llu/auth/login");
    http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
    http.addHeader("Content-Type", "application/json"); http.addHeader("version", "4.16.0");
    http.addHeader("product", "llu.android"); http.addHeader("User-Agent", "Mozilla/5.0 (Linux; Android 13; LLU App)");
    http.addHeader("Accept", "application/json, text/plain, */*");
    JsonDocument doc; doc["email"] = llu_email; doc["password"] = llu_password;
    String requestBody; serializeJson(doc, requestBody);
    int httpResponseCode = http.POST(requestBody);
    if (httpResponseCode == 200) {
        String response = http.getString(); JsonDocument resDoc; deserializeJson(resDoc, response);
        jwt_token = resDoc["data"]["authTicket"]["token"].as<String>();
        account_id_hash = sha256(resDoc["data"]["user"]["id"].as<String>());
        Serial.println("[LLU] Autenticazione riuscita su Libreview!"); http.end(); return true;
    }
    Serial.printf("[LLU] Errore Login: %d\n", httpResponseCode); jwt_token = ""; http.end(); return false;
}

void fetch_glucose() {
    if (WiFi.status() != WL_CONNECTED) return;
    if (jwt_token == "" && !login_llu()) { glucose_value = "Errore Auth"; update_display(); return; }
    WiFiClientSecure client; client.setInsecure(); HTTPClient http;
    http.begin(client, get_api_url() + "/llu/connections");
    http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
    http.addHeader("Authorization", "Bearer " + jwt_token); http.addHeader("account-id", account_id_hash); 
    http.addHeader("version", "4.16.0"); http.addHeader("product", "llu.android");
    http.addHeader("User-Agent", "Mozilla/5.0 (Linux; Android 13; LLU App)"); http.addHeader("Accept", "application/json, text/plain, */*");
    int httpResponseCode = http.GET();
    
    // MODIFICATO: Se il token scade o il server dà errore, svuota la cache per rigenerarla al giro dopo
    if (httpResponseCode == 401 || httpResponseCode == 403 || httpResponseCode < 0) { 
        Serial.println("[LLU] Token scaduto o errore. Reset session cache...");
        jwt_token = ""; http.end(); return;
    }
    if (httpResponseCode == 200) {
        String response = http.getString();
        if (response.indexOf("<html") != -1 || response.indexOf("<!DOCTYPE html>") != -1) { glucose_value = "Blocco HTML"; update_display(); http.end(); return; }
        JsonDocument resDoc; DeserializationError error = deserializeJson(resDoc, response);
        if (error) { glucose_value = "Err JSON"; update_display(); http.end(); return; }
        JsonObject connection; bool dataFound = false;
        if (resDoc["data"].is<JsonArray>() && resDoc["data"].size() > 0) { connection = resDoc["data"][0].as<JsonObject>(); dataFound = true; } 
        else if (resDoc["data"].is<JsonObject>()) { connection = resDoc["data"].as<JsonObject>(); dataFound = true; }
        if (dataFound) {
            if (connection.containsKey("glucoseMeasurement")) {
                glucose_value = connection["glucoseMeasurement"]["Value"].as<String>();
                last_reading_time = connection["glucoseMeasurement"]["Timestamp"].as<String>();
                Serial.println("[LLU] Successo! Glicemia caricata: " + glucose_value);
                update_display();
            } else { glucose_value = "No Meas"; update_display(); }
        } else { glucose_value = "No Data"; update_display(); }
    } else { Serial.printf("[LLU] Errore Connections: %d\n", httpResponseCode); update_display(); }
    http.end();
}
