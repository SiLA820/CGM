#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>

extern WebServer server; extern Preferences prefs;
extern String wifi_ssid, wifi_password, wifi_ssid2, wifi_password2, wifi_ssid3, wifi_password3;
extern String llu_email, llu_password, glucose_value, min_time, max_time;
extern int min_g, max_g;
void try_multi_wifi();

void handle_root() {
    int bg = glucose_value.toInt(); String c = (bg<70||bg>180)?"#d32f2f":(bg>=140)?"#fbc02d" : "#2e7d32";
    String h = "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1,maximum-scale=1'><style>*{box-sizing:border-box;}body{font-family:sans-serif;background:#f5f6fa;color:#2f3542;text-align:center;padding:15px;margin:0;} .card{background:#ffffff;padding:25px 20px;border-radius:20px;border:1px solid #e1e4ed;display:inline-block;width:100%;max-width:350px;text-align:left;box-shadow:0 8px 20px rgba(0,0,0,0.06);} h2{color:#2f3542;margin-top:0;} .val{font-size:55px;font-weight:bold;text-align:center;color:"+c+";margin:5px 0;} .unit{font-size:16px;color:#747d8c;display:block;text-align:center;margin-top:-5px;} .box{display:flex;background:#f1f2f6;padding:10px;border-radius:12px;margin:15px 0;text-align:center;border:1px solid #e1e4ed;} .item{flex:1;} h3{font-size:14px;color:#2f3542;text-transform:uppercase;margin:15px 0 8px 0;letter-spacing:0.5px;border-bottom:2px solid #f1f2f6;padding-bottom:3px;} input[type=text],input[type=password]{width:100%;max-width:100%;padding:12px;margin:4px 0 12px 0;border-radius:10px;border:1px solid #ced6e0;background:#ffffff;color:#2f3542;font-size:14px;} input:focus{border-color:#2e7d32;outline:none;} .btn{width:100%;background:#1d1129;color:#ffffff!important;font-weight:bold;font-size:15px;padding:14px;margin-top:10px;border-radius:10px;cursor:pointer;border:none;box-shadow:0 4px 10px rgba(0,0,0,0.15);} .btn-del{width:100%;background:#f44336;color:#fff!important;font-weight:bold;font-size:14px;padding:12px;margin-top:15px;border-radius:10px;cursor:pointer;border:none;opacity:0.9;} .btn-del:hover{opacity:1;}</style></head><body>";
    h += "<h2>CGM APDP</h2><div class='card'><div class='val'>"+glucose_value+"<span class='unit'>mg/dL</span></div><div class='box'><div class='item' style='border-right:1px solid #ced6e0;'><b>MIN</b><br><span style='color:#0288d1'>"+String(min_g<999?min_g:0)+"</span><br><small style='color:#747d8c'>"+min_time+"</small></div><div class='item'><b>MAX</b><br><span style='color:#c2185b'>"+String(max_g)+"</span><br><small style='color:#747d8c'>"+max_time+"</small></div></div>";
    h += "<form action='/save' method='POST'><h3>Wi-Fi Network 1</h3><input type='text' name='ssid' value='"+wifi_ssid+"' placeholder='SSID 1'><input type='password' name='w_pass' value='"+wifi_password+"' placeholder='Password 1'><h3>Wi-Fi Network 2</h3><input type='text' name='ssid2' value='"+wifi_ssid2+"' placeholder='SSID 2'><input type='password' name='w_pass2' value='"+wifi_password2+"' placeholder='Password 2'><h3>Wi-Fi Network 3</h3><input type='text' name='ssid3' value='"+wifi_ssid3+"' placeholder='SSID 3'><input type='password' name='w_pass3' value='"+wifi_password3+"' placeholder='Password 3'><h3>Abbott Login</h3><input type='text' name='email' value='"+llu_email+"' placeholder='Email Address'><input type='password' name='pass' value='"+llu_password+"' placeholder='Account Password'><input type='submit' class='btn' value='Save & Reboot Device'></form>";
    h += "<form action='/clear' method='POST'><input type='submit' class='btn btn-del' value='Reset All Data (Clear Flash)' onclick='return confirm(\"Are you sure you want to wipe all settings?\")'></form></div></body></html>";
    server.send(200, "text/html", h);
}

void handle_save() {
    if (server.hasArg("ssid") && server.hasArg("email")) {
        wifi_ssid = server.arg("ssid"); wifi_password = server.arg("w_pass");
        wifi_ssid2 = server.arg("ssid2"); wifi_password2 = server.arg("w_pass2");
        wifi_ssid3 = server.arg("ssid3"); wifi_password3 = server.arg("w_pass3");
        llu_email = server.arg("email"); llu_password = server.arg("pass");
        prefs.begin("cgm-config", false);
        prefs.putString("w_ssid", wifi_ssid); prefs.putString("w_pass", wifi_password);
        prefs.putString("w_ssid2", wifi_ssid2); prefs.putString("w_pass2", wifi_password2);
        prefs.putString("w_ssid3", wifi_ssid3); prefs.putString("w_pass3", wifi_password3);
        prefs.putString("email", llu_email); prefs.putString("pass", llu_password); prefs.end();
        try_multi_wifi();
        String next_ip = (WiFi.status() == WL_CONNECTED) ? WiFi.localIP().toString() : "192.168.4.1";
        String r = "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta http-equiv='refresh' content='8;url=http://" + next_ip + "'><style>body{font-family:sans-serif;background:#f5f6fa;color:#2f3542;text-align:center;padding-top:15vh;} .loader{color:#2e7d32;font-size:22px;font-weight:bold;}</style></head><body><h2>✔ Settings Saved!</h2><p class='loader'>Connecting... </p></body></html>";
        server.send(200, "text/html", r); delay(3000); ESP.restart();
    }
}

// INSERITO: Implementazione fisica mancante della funzione per pulire la Flash delle preferenze
void handle_clear() { 
    prefs.begin("cgm-config", false); 
    prefs.clear(); 
    prefs.end(); 
    server.send(200, "text/html", "<h2>Memory Wiped! Restarting...</h2>"); 
    delay(2000); 
    ESP.restart(); 
}
