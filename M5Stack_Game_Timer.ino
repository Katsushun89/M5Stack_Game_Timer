#include <M5Stack.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <time.h>
#include "config.h"
 
#define JST 3600* 9
#define DELAY_CONNECTION 100

void setupWiFi(void)
{
    int ret, i;
    while ((ret = WiFi.status()) != WL_CONNECTED) {
        Serial.printf("> stat: %02x", ret);
        ret = WiFi.begin(ssid, password);  //  Wi-Fi APに接続
        i = 0;
        while ((ret = WiFi.status()) != WL_CONNECTED) {  //  Wi-Fi AP接続待ち
            delay(DELAY_CONNECTION);
            if ((++i % (1000 / DELAY_CONNECTION)) == 0) {
                Serial.printf(" >stat: %02x", ret);
            }
            if (i > 10 * 1000 / DELAY_CONNECTION) { // 10秒経過してもDISCONNECTEDのままなら、再度begin()
                break;
            }
        }
    }
    Serial.print("WiFi connected\r\nIP address: ");
    Serial.println(WiFi.localIP());
}

void setup() {
  M5.begin();
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextColor(GREEN);
  
  Serial.begin(115200);
  delay(100);
  Serial.print("\n\nStart\n");
 
  setupWiFi();
  configTime( JST, 0, "ntp.nict.jp", "ntp.jst.mfeed.ad.jp");
}
 
void loop() {
  time_t t;
  struct tm *tm;
  static const char *wd[7] = {"Sun","Mon","Tue","Wed","Thr","Fri","Sat"};

  M5.Lcd.clear(BLACK);
  M5.Lcd.setCursor(1, 0);
   
  t = time(NULL);
  tm = localtime(&t);
  Serial.printf(" %04d/%02d/%02d(%s) %02d:%02d:%02d\n",
        tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday,
        wd[tm->tm_wday],
        tm->tm_hour, tm->tm_min, tm->tm_sec);
  char c_hour[5];
  snprintf(c_hour, sizeof(c_hour), "%02d", tm->tm_hour);
  char c_min[5];
  snprintf(c_min, sizeof(c_min), "%02d", tm->tm_min);
  char c_sec[5];
  snprintf(c_sec, sizeof(c_sec), "%02d", tm->tm_sec);
  
  M5.Lcd.setTextSize(3);
  M5.Lcd.println(String(tm->tm_year+1900, DEC) + "/"
               + String(tm->tm_mon+1) + "/"
               + String(tm->tm_mday) + "("
               + String(wd[tm->tm_wday]) + ")");

  M5.Lcd.setTextSize(5);
  M5.Lcd.println(String(tm->tm_hour) + ":"
               + String(tm->tm_min) + ":"
               + String(tm->tm_sec)
               );
  delay(1000);

}
