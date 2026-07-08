#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

extern String llu_email;
extern String llu_password;
extern String jwt_token;
extern String account_id_hash;
extern String glucose_value;
extern String last_reading_time;

void update_display();

// --- Endpoint Base Corretto (Libreview EU) ---
String get_api_url() {
    return "https://api-eu.libreview.io"; 
}

// --- Native C++ SHA256 Encryption Array Engine (Fixed) ---
String sha256(String payload) {
    byte shaResult[32]; // <-- FIX: Changed from 'byte shaResult' to 'byte shaResult[32]'
    mbedtls_md_context_t ctx;
    mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;
    
    mbedtls_md_init(&ctx);
    mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 0);
    mbedtls_md_starts(&ctx);
    mbedtls_md_update(&ctx, (const unsigned char*) payload.c_str(), payload.length());
    mbedtls_md_finish(&ctx, shaResult); // Now passing the correct memory pointer
    mbedtls_md_free(&ctx);
    
    String hashStr = "";
    for(int i=0; i<32; i++) {
        if(shaResult[i]<16) hashStr += "0";
        hashStr += String(shaResult[i], HEX);
    }
    return hashStr;
}

// --- Autenticazione LibreLinkUp ---
bool login_llu() {
    WiFiClientSecure client;
    client.setInsecure(); 
    HTTPClient http;
    
    String url = get_api_url() + "/llu/auth/login";
    http.begin(client, url);
    http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
    
    http.addHeader("Content-Type", "application/json");
    http.addHeader("version", "4.16.0");
    http.addHeader("product", "llu.android");
    http.addHeader("User-Agent", "Mozilla/5.0 (Linux; Android 13; LLU App)");
    http.addHeader("Accept", "application/json, text/plain, */*");

    JsonDocument doc;
    doc["email"] = llu_email;
    doc["password"] = llu_password;
    String requestBody;
    serializeJson(doc, requestBody);

    int httpResponseCode = http.POST(requestBody);
    if (httpResponseCode == 200) {
        String response = http.getString();
        JsonDocument resDoc;
        deserializeJson(resDoc, response);
        
        jwt_token = resDoc["data"]["authTicket"]["token"].as<String>();
        String userId = resDoc["data"]["user"]["id"].as<String>();
        account_id_hash = sha256(userId);
        
        Serial.println("[LLU] Autenticazione riuscita su Libreview!");
        http.end();
        return true;
    }
    Serial.print("[LLU] Errore di Login. Codice HTTP: ");
    Serial.println(httpResponseCode);
    http.end();
    return false;
}

// --- Recupero Dati Glicemia (Senza variabili Trend) ---
void fetch_glucose() {
    if (jwt_token == "" && !login_llu()) {
        glucose_value = "Errore Auth";
        update_display();
        return;
    }

    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;

    String url = get_api_url() + "/llu/connections";
    http.begin(client, url);
    http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
    
    http.addHeader("Authorization", "Bearer " + jwt_token);
    http.addHeader("account-id", account_id_hash); 
    http.addHeader("version", "4.16.0");
    http.addHeader("product", "llu.android");
    http.addHeader("User-Agent", "Mozilla/5.0 (Linux; Android 13; LLU App)");
    http.addHeader("Accept", "application/json, text/plain, */*");

    int httpResponseCode = http.GET();
    if (httpResponseCode == 401) { 
        Serial.println("[LLU] Token scaduto. Tento il rinnovo...");
        jwt_token = ""; 
        login_llu();
        http.end();
        return;
    }

    if (httpResponseCode == 200) {
        String response = http.getString();
        
        if (response.indexOf("<html") != -1 || response.indexOf("<!DOCTYPE html>") != -1) {
            glucose_value = "Blocco HTML";
            update_display();
            http.end();
            return;
        }

        JsonDocument resDoc;
        DeserializationError error = deserializeJson(resDoc, response);
        if (error) {
            glucose_value = "Err JSON";
            update_display();
            http.end();
            return;
        }

        JsonObject connection;
        bool dataFound = false;

        if (resDoc["data"].is<JsonArray>() && resDoc["data"].size() > 0) {
            connection = resDoc["data"][0].as<JsonObject>();
            dataFound = true;
        } else if (resDoc["data"].is<JsonObject>()) {
            connection = resDoc["data"].as<JsonObject>();
            dataFound = true;
        }

        if (dataFound) {
            if (connection.containsKey("glucoseMeasurement")) {
                // Rimossa interamente la lettura ed il controllo dello switch trend per snellire la visualizzazione
                glucose_value = connection["glucoseMeasurement"]["Value"].as<String>();
                last_reading_time = connection["glucoseMeasurement"]["Timestamp"].as<String>();
                Serial.println("[LLU] Successo! Glicemia caricata: " + glucose_value);
                
                // Aggiorna lo schermo ad ogni ciclo andato a buon fine
                update_display();
                
            } else {
                glucose_value = "No Meas";
                update_display();
            }
        } else {
            glucose_value = "No Data";
            update_display();
        }
    } else {
        Serial.print("[LLU] Errore HTTP Connections: ");
        Serial.println(httpResponseCode);
        update_display();
    }
    http.end();
}
