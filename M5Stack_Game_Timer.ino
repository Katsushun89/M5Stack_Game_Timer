#include <M5Stack.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <time.h>
#include "config.h"
 
#define JST 3600* 9
#define DELAY_CONNECTION 100

static time_t start_time = 0;
static time_t pre_min = 0;

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

void displayTime(time_t t)
{
    struct tm *tm;
    static const char *wd[7] = {"Sun","Mon","Tue","Wed","Thr","Fri","Sat"};

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
    M5.Lcd.println(String(c_hour) + ":"
               + String(c_min) + ":"
               + String(c_sec)
               );
    
}

void setStartTime(time_t t)
{
    if(start_time == 0){
        start_time = t;
        Serial.println("set start_time");
        struct tm *tm;
        tm = localtime(&start_time);
        Serial.printf("start %04d/%02d/%02d %02d:%02d:%02d\n",
            tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday,
            tm->tm_hour, tm->tm_min, tm->tm_sec);
    }
}

uint16_t measureElapsedTime(time_t t)
{
    if(start_time == 0) return 0;
    double elapsed_time = difftime(t, start_time);
    uint32_t elapsed_min = (uint32_t)(elapsed_time / 60);
    //Serial.printf("elapsed sec %f min %d\n", elapsed_time, elapsed_min);
    return elapsed_min;
}

bool isUpdatedMin(time_t t)
{
    bool is_updated_min = false;
    struct tm *tm;

    tm = localtime(&t);
    if(tm->tm_min != pre_min){
        is_updated_min = true;
    }
    pre_min = tm->tm_min;
    return is_updated_min; 
}

uint16_t getRemainTime(uint16_t elapsed_min)
{
    return GAME_TIMER_LIMIT - elapsed_min;
}

bool alertRemainTime(uint32_t remain_min)
{
    Serial.printf("alert remain_min %03d\n", remain_min);
    Serial.printf("alert time %02d / %02d / %02d\n",
                    ALERT_REMAIN_MIN_1,
                    ALERT_REMAIN_MIN_2,
                    ALERT_REMAIN_MIN_3);
    
    if(remain_min == ALERT_REMAIN_MIN_1){
        Serial.println("alert 1");
        M5.Lcd.setTextSize(5);
        M5.Lcd.println("Alert1");
        return true; 
    }

    if(remain_min == ALERT_REMAIN_MIN_2){
        Serial.println("alert 2");
        M5.Lcd.setTextSize(5);
        M5.Lcd.println("Alert2");
        return true; 
    }

    if(remain_min == ALERT_REMAIN_MIN_3){
        Serial.println("alert 3");
        M5.Lcd.setTextSize(5);
        M5.Lcd.println("Alert3");
        return true; 
    }

    if(remain_min == 0){
        Serial.println("time over");

        return true;
    }

    return false;
}

void loop() 
{
    time_t cur_time;
    M5.update();

    cur_time = time(NULL);

    //button
    if (M5.BtnA.wasPressed()){
        setStartTime(cur_time);
    }

    uint16_t elapsed_min = measureElapsedTime(cur_time);
    uint16_t remain_min = getRemainTime(elapsed_min);
    char c_remain_min[5];
    snprintf(c_remain_min, sizeof(c_remain_min), "%03d", remain_min);

    //update LCD
    if(isUpdatedMin(cur_time)){
        M5.Lcd.clear(BLACK);
        M5.Lcd.setCursor(1, 0);

        displayTime(cur_time);
        M5.Lcd.setTextSize(5);
        M5.Lcd.println(String(c_remain_min) + "[min]");
        alertRemainTime(remain_min);
    }

    delay(1);
}
