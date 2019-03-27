#include <M5Stack.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <WiFiClientSecure.h>
#include <time.h>
#include <AquesTalkTTS.h>
#include "config.h"

#define JST 3600* 9
#define DELAY_CONNECTION 100
#define CF_OL32 &Orbitron_Light_32
#define GFXFF 1

// AquesTalk License Key
// NULL or wrong value is just ignored
const char* AQUESTALK_KEY = "XXXX-XXXX-XXXX-XXXX";

static time_t start_time = 0;
static time_t pre_min = 0;
static bool is_set_start = false;

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

void setupVoice(void)
{
  int iret = TTS.createK(AQUESTALK_KEY);
  Serial.print("TTS.createK iret "); Serial.print(iret);
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

    setupVoice(); 

    M5.Lcd.clear();
    M5.Lcd.drawJpgFile(SD, "/game_start.jpg");

    sendLineNotify("reset");
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
        is_set_start = true;
        start_time = t;
        struct tm *tm;
        tm = localtime(&start_time);
        Serial.printf("start timer %04d/%02d/%02d %02d:%02d:%02d\n",
            tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday,
            tm->tm_hour, tm->tm_min, tm->tm_sec);
        TTS.playK(voice_start_timer.c_str(), VOICE_SPEED);
        sendLineNotify("start timer");
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
    uint16_t remain = 0;
    if(elapsed_min > GAME_TIMER_LIMIT){
        remain = 0;
    }else{
        remain = GAME_TIMER_LIMIT - elapsed_min;
    }
    return remain;
}

bool alertRemainTime(uint32_t remain_min)
{
    Serial.printf("alert remain_min %03d\n", remain_min);
    Serial.printf("alert time %02d -> %02d -> %02d[min]\n",
                    ALERT_REMAIN_MIN_1,
                    ALERT_REMAIN_MIN_2,
                    ALERT_REMAIN_MIN_3);
    
    if(remain_min == ALERT_REMAIN_MIN_1){
        Serial.println("alert 1");
        TTS.playK(voice_alert_1.c_str(), VOICE_SPEED);
        return true; 
    }

    if(remain_min == ALERT_REMAIN_MIN_2){
        Serial.println("alert 2");
        TTS.playK(voice_alert_2.c_str(), VOICE_SPEED);
        return true; 
    }

    if(remain_min == ALERT_REMAIN_MIN_3){
        Serial.println("alert 3");
        TTS.playK(voice_alert_3.c_str(), VOICE_SPEED);
        return true; 
    }

    if(remain_min == 0){
        Serial.println("time over");
        TTS.playK(voice_timer_over.c_str(), VOICE_SPEED);
        sendLineNotify("time over");
        return true;
    }

    return false;
}

void sendLineNotify(const char *line_message)
{
    WiFiClientSecure client;
    Serial.println("Try");
    //LineのAPIサーバに接続
    if (!client.connect(line_host, 443)) {
      Serial.println("Connection failed");
      return;
    }
    Serial.println("Connected");
    //リクエストを送信
    String query = String("message=") + String(line_message);
    String request = String("") +
                 "POST /api/notify HTTP/1.1\r\n" +
               "Host: " + line_host + "\r\n" +
               "Authorization: Bearer " + line_token + "\r\n" +
               "Content-Length: " + String(query.length()) +  "\r\n" + 
               "Content-Type: application/x-www-form-urlencoded\r\n\r\n" +
                query + "\r\n";
    client.print(request);
 
     //受信終了まで待つ 
    while (client.connected()) {
        String line = client.readStringUntil('\n');
        if (line == "\r") {
            break;
        }
    }
  
    String line = client.readStringUntil('\n');
    Serial.println(line);
}

void drawCount(uint8_t number)
{
    if(number == 0){
        M5.Lcd.setTextColor(TFT_RED, TFT_BLACK);
    }else{
        M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
    }
    M5.Lcd.setTextPadding(5);
    M5.Lcd.setTextSize(3);
    M5.Lcd.setTextDatum(MC_DATUM);
    M5.Lcd.setFreeFont(CF_OL32);
    M5.Lcd.drawNumber(number, 160, 120, GFXFF);
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
    if(is_set_start && isUpdatedMin(cur_time)){
        /*
        M5.Lcd.clear(BLACK);
        M5.Lcd.setCursor(1, 0);

        displayTime(cur_time);
        M5.Lcd.setTextSize(5);
        M5.Lcd.println(String(c_remain_min) + "[min]");
*/
        //M5.Lcd.clear();
        M5.Lcd.drawJpgFile(SD, "/game_count.jpg");
        drawCount(remain_min);
        alertRemainTime(remain_min);
    }

    delay(1);
}
