#include <WiFi.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

extern Adafruit_ST7789 tft;
extern String glucose_value, min_time, max_time;
extern int glucose_hist[], r_count, min_g, max_g;
String get_time(bool include_seconds);

void update_display() {
    tft.fillScreen(0x0000); tft.drawRect(0, 0, 320, 240, 0x528A); tft.drawRect(1, 1, 318, 238, 0x2104);
    String lt_sec = get_time(true); String lt_min = get_time(false); int bg = glucose_value.toInt();
    if (bg > 0) {
        if (bg < min_g) { min_g = bg; min_time = lt_min; }
        if (bg > max_g) { max_g = bg; max_time = lt_min; }
        if (r_count < 15) glucose_hist[r_count++] = bg;
        else { for (int i=0; i<14; i++) glucose_hist[i] = glucose_hist[i+1]; glucose_hist[14] = bg; }
    }
    String minStr = (min_g < 999) ? String(min_g) : "--"; String maxStr = (max_g > 0) ? String(max_g) : "--";
    int16_t x1, y1; uint16_t w, h_text;
    
    tft.fillRoundRect(8, 36, 76, 68, 6, 0x18C3);
    tft.setTextSize(3); tft.getTextBounds(minStr, 0, 52, &x1, &y1, &w, &h_text); tft.setTextColor(0xFE19); tft.setCursor(46 - (w/2), 52); tft.print(minStr);
    tft.setTextSize(1); tft.setTextColor(0x8410); tft.setCursor(35, 39); tft.print("MIN");
    tft.setTextSize(2); tft.getTextBounds(min_time, 0, 82, &x1, &y1, &w, &h_text); tft.setTextColor(0x07E0); tft.setCursor(46 - (w/2), 82); tft.print(min_time);
    
    tft.setTextSize(7); tft.getTextBounds(glucose_value, 0, 26, &x1, &y1, &w, &h_text);
    int center_x = (320 - w) / 2; tft.setCursor(center_x, 26); tft.setTextColor((bg<70||bg>180)?0xF800:(bg>=140)?0xE7E0:0xF133); tft.print(glucose_value);
    tft.setTextSize(2); tft.setTextColor(0x8410); tft.setCursor(130, 84); tft.print("mg/dL");
    
    tft.fillRoundRect(236, 36, 76, 68, 6, 0x18C3);
    tft.setTextSize(3); tft.getTextBounds(maxStr, 0, 52, &x1, &y1, &w, &h_text); tft.setTextColor(0xF133); tft.setCursor(274 - (w/2), 52); tft.print(maxStr);
    tft.setTextSize(1); tft.setTextColor(0x8410); tft.setCursor(263, 39); tft.print("MAX");
    tft.setTextSize(2); tft.getTextBounds(max_time, 0, 82, &x1, &y1, &w, &h_text); tft.setTextColor(0x07E0); tft.setCursor(274 - (w/2), 82); tft.print(max_time);
    
    tft.drawFastHLine(12, 114, 296, 0x294A); tft.drawFastHLine(12, 196, 296, 0x294A);
    if (r_count > 1) {
        int px=0, py=0;
        for (int i=0; i<r_count; i++) {
            int cx = 25+int(i*(270.0/14.0)), v = glucose_hist[i];
            if (v<40) v=40; if (v>300) v=300; int cy = 195-int((v-40)*(80.0/260.0));
            tft.fillCircle(cx,cy,4,0xF133); if(i>0) tft.drawLine(px,py,cx,cy,0x528A); px=cx; py=cy;
        }
    }
    String ipStr = WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : "AP: APDP";
    tft.setTextSize(1); tft.getTextBounds(ipStr, 0, 220, &x1, &y1, &w, &h_text); tft.setTextColor(0xF800); tft.setCursor(315 - w, 220); tft.print(ipStr);
    tft.setCursor(88, 212); tft.setTextColor(0x07E0); tft.setTextSize(3); tft.print(lt_sec);
}
