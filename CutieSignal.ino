// Inspiration: https://www.reddit.com/r/arduino/comments/12uy1r2/never_gunna_give_you_up_wifi_beacon/
//Each line of the lyrics in the array
//is 32 characters (including spaces) or less
//to comply with SSID naming limitations.

//https://github.com/marcelstoer/esp-wifi-rickroller
//https://github.com/ivynya/ESP32-Rick-Roller/tree/master
// Libraries for DNS and AP
#include <WiFi.h>
#include <DNSServer.h>

// Libraries for web server/captive portal site
#include <ArduinoJson.h>            //https://www.arduino.cc/reference/en/libraries/arduinojson/
#include <AsyncTCP.h>               //https://github.com/me-no-dev/AsyncTCP
#include <ESPAsyncWebServer.h>      //https://github.com/me-no-dev/ESPAsyncWebServer
#include <SPIFFS.h>
#include <Wire.h>
#include "SSD1306Wire.h"            //Display text on OLED module SSD1306 with I2C
#include "pins_arduino.h"          
#include <ESP32Time.h>              //Always usefull a Time Lib
#include "esp_system.h"             //ESPSystem Calls. Very cool must explore more.



SSD1306Wire display(0x3c, SDA_OLED, SCL_OLED);  // ADDRESS, SDA, SCL  -  SDA_OLED and SCL_OLED are the pins from pins_arduino.h where the Heltec connects the OLED display


#define uS_TO_S_FACTOR 1000000 /* Conversion factor for micro seconds to seconds */ 
#define TIME_TO_SLEEP 30 /* Time ESP32 will go to sleep (in seconds) */
#define INTERVAL_T1 10000

ESP32Time rtc;
RTC_DATA_ATTR int bootCount     = 0;
RTC_DATA_ATTR int timeFlag      = 0;
RTC_DATA_ATTR int current_line  = 0;
RTC_DATA_ATTR int dayNow        = 0;


int   hourNow       = 0;
int   minutesNow    = 0;
int   numSeconds    = 0;


//Each line of the in the array is 32 characters (including spaces) or less to comply with SSID naming limitations.
String never_gonna[] = {
        "W05A is amazing",
        "W05A lights up the room",
        "W05A is very kind",
        "W05As smile is bright",
        "W05A has a brilliant mind",
        "W05A is a true cutie",
        "W05A you inspire",
        "W05A is so talented",
        "W05A is awesome fun",
        "W05A has positivity",
        "W05A has a heart of gold",
        "W05A you are the best",
        "W05A so pretty",
        "W05A you are a star",
        "W05A makes life better",
        "W05A is wonderful",
        "W05A has an amazing spirit",
        "W05A is the best",
        "W05A is the best cook",
        "W05A brings out the best",
        "W05A is magnetic",
        "W05A has style",
        "W05A is refreshing",
        "W05A is thoughtful",
        "W05A is kindness",
        "W05A has a beautiful soul",
        "W05A you are a treasure",
        "W05A has great humor",
        "W05A is compassionate",
        "W05A has a great smile",
        "W05A your energy is great",
        "W05A makes things better",
        "W05A is delightful",
        "W05As wisdom is deep",
        "W05A has a great spirit",
        "W05A is a good friend",
        "W05A is so thoughtful",
        "W05A has a radiant vibe",
        "W05A has a warm heart",
        "W05A is inspiring",
        "W05A is Drum and Bass",
        "W05A has a sweet heart",
        "W05A is so optimistic",
        "W05A is so cheerful",
        "W05A knows detail",
        "W05A is so insightful",
        "W05A has a big heart",
        "W05A is so empathetic",
        "W05A has a great smile",
        "W05A is genuine",
        "W05A so smart",
        "W05A is so talented",
        "W05A is fun",
        "W05A an adventurous soul",
        "W05A has a sweet nature",
        "W05A is inspiring",
        "W05A has a good attitude",
        "W05A is very thoughtful"
};




const byte DNS_PORT = 53;
IPAddress apIP(192, 168, 4, 1);
DNSServer dnsServer;
AsyncWebServer server(80);

// Non-persistent helper variables
int visitors = 0;
int totalSeconds = 0;
int avgSeconds = 0;
int timesInfoViewed = 0;


void VextON(void) {
  pinMode(Vext, OUTPUT);
  digitalWrite(Vext, LOW);
}

void VextOFF(void) 
{
  pinMode(Vext, OUTPUT);
  digitalWrite(Vext, HIGH);
}

void displayReset(void) {
  // Send a reset
  pinMode(RST_OLED, OUTPUT);
  digitalWrite(RST_OLED, HIGH);
  delay(1);
  digitalWrite(RST_OLED, LOW);
  delay(1);
  digitalWrite(RST_OLED, HIGH);
  delay(1);
}

// Gets called on not found
void onRequest(AsyncWebServerRequest *request) {
  request->send(SPIFFS, "/index.html", "text/html");
}

// Adds a visitor, recalculates average time spent
void incrementVisitorCount(int secondsSpent, bool scrolledDown) {
  visitors += 1;
  totalSeconds += secondsSpent;
  avgSeconds = totalSeconds / visitors;
  if (scrolledDown) {
    timesInfoViewed += 1;
  }

  savePersistentData();
  saveIndividualData(secondsSpent, scrolledDown);
}

void savePersistentData() {
  File file = SPIFFS.open("/persistent.txt", FILE_WRITE);
  if (!file) { return; }
  file.print("{\"v\":");
  file.print(visitors);
  file.print(",\"s\":");
  file.print(avgSeconds);
  file.print(",\"i\":");
  file.print(timesInfoViewed);
  file.print("}");
  file.close();
}

void saveIndividualData(int seconds, bool scrolledDown) {
  File file = SPIFFS.open("/stats.txt", FILE_APPEND);
  if (!file) { return; }
  file.print("v: ");
  file.print(seconds);
  file.print(", ");
  file.println(scrolledDown);
  file.close();
}


void showText(String text) {
  display.clear(); 
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawStringMaxWidth(0, 0, 128, text);
  display.display();
}


void setup() {
  Serial.begin(9600);
  delay(3000);

  Serial.println("Time on bootup   "+rtc.getTime("%A, %B %d %Y %H:%M:%S"));
  delay(5000);
  Serial.println("timeFlag   "+String(timeFlag) +" if 100 do nothing. if 0 or 255 set time");
  if (timeFlag == 255 || timeFlag == 0)  { 
                //ss  mm  hh  dd mm yyyy
      rtc.setTime(01, 37, 17, 11, 7, 2024);  //---------------------------------------------------------------------------------------------------------------------------------------Set Date & Time-----------------------------    
      timeFlag = 100;    
  }
  delay(2000);
  Serial.println("Time that matters: >> "+rtc.getTime("%A, %B %d %Y %H:%M:%S"));
  Serial.println("Free HEAP Size: "+String(esp_get_free_heap_size()));                                  //esp_get_free_heap_size(); Get the size of available internal heap
  Serial.println("Minimum free HEAP seen: "+String(esp_get_minimum_free_heap_size()));                  //esp_get_minimum_free_heap_size(); //Get the minimum heap that has ever been available
  delay(5000);
  
  esp_reset_reason_t reason = esp_reset_reason();
  getResetReasonFunc(reason);

  delay(2000);  
  ++bootCount;
  Serial.println("Boot number: " + String(bootCount));  
  Serial.println();


//*****************************************************************************************
//*************** Start OLED Setup*********************************************************
//*****************************************************************************************  
  //This turns on and resets the OLED on the Heltec boards
  VextON();
  displayReset();  
  display.init();
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);

//*****************************************************************************************
//*************** END OLED Setup***********************************************************
//*****************************************************************************************



//*****************************************************************************************
//*************** Start SSID Setup*********************************************************
//*****************************************************************************************


 if (current_line == 0 || current_line == 255) {
    if (current_line >= sizeof(never_gonna)) {     current_line = 0;      } //trying to avoid a miss here need to check this.
    Serial.println("SSID on boot[0/255] is: "+never_gonna[current_line]+" with index value "+ String(current_line));
  }else{    
    Serial.println("SSID on boot[RTC_DATA_ATTR] is: "+never_gonna[current_line]+" with index value "+ String(current_line));
  }

//*****************************************************************************************
//*************** END SSID Setup***********************************************************
//*****************************************************************************************

delay(2000);

//*****************************************************************************************
//*************** Start RTC_DATA_ATTR & Time Setup*****************************************
//*****************************************************************************************
   //Largest number it can take is 30 Minutes so ... 
  //The long type on ESP32 has a maximum value of 2147483647 which is as you said, "about half an hour" (not quite 36 minutes) worth of microseconds.
  //https://arduino.stackexchange.com/questions/91670/esp32-can-not-deep-sleep-longer-than-35-minutes
  // 30 Min in seconds is 1800 seconds or     1,800,000,000 microseconds  or 30 minutes.
  //https://github.com/fbiego/ESP32Time
  
  //dayNow = rtc.getDay();             //(int)17 (1-31)
  hourNow = rtc.getHour(true);       //(int)15 (0-23)
  minutesNow = rtc.getMinute();      //(int)24 (0-59)

 
  if (dayNow == 0 || dayNow == 255) {      
      Serial.println("dayNow is 0 or 255 so setting day from rtc to "+String(rtc.getDay()));          
      dayNow = rtc.getDay();      
  }else{        
      Serial.println("Day in RTC is "+String(rtc.getDay())+"     dayNow variable is "+dayNow);      
  }
//*****************************************************************************************
//*************** END EEP & Time Setup*****************************************************
//*****************************************************************************************


delay(2000);

//*****************************************************************************************
//*************** Sleep Timer Bit on Startup **********************************************
//*****************************************************************************************
  hourNow = rtc.getHour(true);
  minutesNow = rtc.getMinute();    
  numSeconds = secondsToSleep(hourNow, minutesNow);

//if need to sleep after waking up do it until sleep = 0;
if (numSeconds > 0) {        
        Serial.println("Time to sleep for "+String(numSeconds)+" seconds!");        
        showText("Time to sleep for "+String(numSeconds)+" seconds!  "+rtc.getTime("%A, %B %d %Y %H:%M:%S"));        

        //If sleep is less than 30 min use that other sleep in 30 min chunks due to INT limits.
        if (numSeconds > 1800 ) {  
          esp_sleep_enable_timer_wakeup(1800 * 1000000); //Sleep for 30 min (1800 sec) and repeat until less than 1800 remains. Then sleep what remains.
          esp_deep_sleep_start(); 
        }else{
          esp_sleep_enable_timer_wakeup(numSeconds * 1000000); //
          esp_deep_sleep_start(); 
        }                                                
 }else{
      
      Serial.println("Continue to boot numSeconds should be 0 :>>  " + String(numSeconds));       
 }
//*****************************************************************************************
//*************** END Sleep Timer Bit on Startup ******************************************
//*****************************************************************************************

  delay(2000);
  Serial.println();
  Serial.println("Cute Signal Starting ...from Setup");
  showText("Cute Signal Starting...");

  // Initialize SPIFFS
  if(!SPIFFS.begin(true)){
    Serial.println("An error has occurred while mounting SPIFFS");
    return;
  }


  // Start AP with localIP, dnsIP, and gatewayIP
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  
  Serial.println("Setting SSID to '" + never_gonna[0] + "'.");
  WiFi.softAP(never_gonna[0].c_str());

  // Start DNS server with * (will redirect all)
  if(!dnsServer.start(DNS_PORT, "*", apIP)) {
    Serial.println("An error has occurred while starting DNS");
    return;
  }




// Load saved persistent data
  File file = SPIFFS.open("/persistent.txt");
  if (!file) {
    Serial.println("An error has occured when loading persistent data");
    return;
  }
  String json;
  while (file.available()) {
    json += char(file.read());
  }
  // Log the data loaded
  Serial.println(json);

  // Parse saved persistent data
  StaticJsonDocument<54> doc;
  DeserializationError error = deserializeJson(doc, json);
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.c_str());
    return;
  }
  visitors = doc["v"];
  avgSeconds = doc["s"];
  totalSeconds = visitors * avgSeconds;
  timesInfoViewed = doc["i"];
  file.close();

  // Redirected DNS queries will get sent here
  server.onNotFound(onRequest);
  // Redirected DNS queries may also be sent here
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/index.html","text/html");
  });

  // Routes to load site content
  server.on("/css/site.css", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/css/site.css", "text/css");
  });
  server.on("/js/site.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/js/site.js", "text/js");
  });

  // Routes to load media content
  server.on("/media/pippa.webp", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/media/pippa.webp", "image/webp");
  });

  // Routes to get/process data
  server.on("/persistent.txt", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/persistent.txt", "text/plain");
  });
  server.on("/stats.txt", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/stats.txt", "text/plain");
  });
  server.on("/addvisit", HTTP_ANY, [](AsyncWebServerRequest *request) {
    int s = atoi(request->getParam(0)->value().c_str());
    bool v = (atoi(request->getParam(1)->value().c_str()) == 1) ? true : false;
    incrementVisitorCount(s, v);
    request->send(200);
  });

  
  server.begin();
  
  showText("Cutie Signal Start         "+rtc.getTime("%A, %B %d %Y %H:%M:%S")+ " Min HEAP: "+String(esp_get_minimum_free_heap_size()));
  Serial.println("Cutie Signal Start   "+rtc.getTime("%A, %B %d %Y %H:%M:%S")+ " Min HEAP: "+String(esp_get_minimum_free_heap_size()));
    
} 

//End of setup();


int loops = 0;

unsigned long time_1 = 0;
void loop() {
  
  dnsServer.processNextRequest();

  hourNow = rtc.getHour(true);
  minutesNow = rtc.getMinute();    
  int numSeconds = secondsToSleep(hourNow, minutesNow);
  
  //When day changes update SSID, Day and Write RTC MEM
  if (dayNow != rtc.getDay()) {
    int indexSSID = switchSSID();
    dayNow = rtc.getDay();    
  }


  
  if (numSeconds == 0) {

        //This just updates the OLED display every 10 seconds
        if( millis() >= time_1 + INTERVAL_T1) {    
              time_1 +=INTERVAL_T1;                       
                    showText("Cutie Signal. SSID: '" + never_gonna[current_line] + "'.  "+rtc.getTime("%A, %B %d %Y %H:%M:%S")+ " Min HEAP: "+String(esp_get_minimum_free_heap_size()));
        }
                
  }else{
         server.end(); //Close down the Weberver gracefull if possible.                             
         delay(2000);
        //If sleep is less than 30 min use that other sleep in 30 min chunks due to INT limits.
        if (numSeconds > 1800 ) {                  
                showText("Sleep for "+String(numSeconds)+" seconds 30 min chunks:: "+rtc.getTime("%A, %B %d %Y %H:%M:%S")+ " Min HEAP: "+String(esp_get_minimum_free_heap_size()));
                delay(1000);
                esp_sleep_enable_timer_wakeup(1800 * 1000000); //Sleep for 30 min (1800 sec) and repeat until less than 1800 remains. Then sleep what remains.
                esp_deep_sleep_start(); 
        }else{                
                showText("Sleep for "+String(numSeconds)+" seconds::  "+rtc.getTime("%A, %B %d %Y %H:%M:%S")+ " Min HEAP: "+String(esp_get_minimum_free_heap_size()));
                delay(1000);
                esp_sleep_enable_timer_wakeup(numSeconds * 1000000); //
                esp_deep_sleep_start(); 
        }                        
  }
    
}









int secondsToSleep(int currentHour, int currentMinute) {
  
        if (currentHour == 16) {
            //return only minutes
            int minutes = 60 - currentMinute;
            return minutes * 60; //returns seconds
        }

        if (currentHour >= 22 ) {
            if (currentHour == 22) {
                int minutes = 60 - currentMinute;
                return (minutes * 60) + 3600 + 61200; //returns seconds
                //3600 is 1 hr, 61200 is 17 hrs
            }

            if (currentHour == 23) {
                int minutes = 60 - currentMinute;
                return (minutes * 60) + 61200; //returns seconds
            }
        }

        if (currentHour >= 0 && currentHour < 17 ){
            int hours = 16 - currentHour;
            int minutes = 60 - currentMinute;
            return (hours * 60 * 60) + (minutes * 60);
        }
        return 0; //0 = should be active
}



void getResetReasonFunc(esp_reset_reason_t reason) {
        switch (reason) 
        {      
          case ESP_RST_POWERON:Serial.println("Power ON");break;
          case ESP_RST_SW: Serial.println("Reset ESPrestart()");break;
          case ESP_RST_PANIC:Serial.println("Reset by exception/panic");break;
          case ESP_RST_UNKNOWN:Serial.println("Reset UNKNOW");break;
          case ESP_RST_EXT:Serial.println("Reset by external pin (not applicable for ESP32)");break;
          case ESP_RST_INT_WDT:Serial.println("Reset (software or hardware) for Inturrupt WATCHDOG");break;
          case ESP_RST_TASK_WDT:Serial.println("Reset WATCHDOG");break;
          case ESP_RST_WDT:Serial.println("Reset others WATCHDOG´s");break;                                
          case ESP_RST_DEEPSLEEP:Serial.println("Reset DEEP SLEEP MODE");break;
          case ESP_RST_BROWNOUT:Serial.println("Brownout reset (software or hardware)");break;
          case ESP_RST_SDIO:Serial.println("Reset over SDIO");break;        
        default: Serial.println("Reset over by Default"); break;
        }
   }




int switchSSID() {    
  current_line = (current_line + 1) % (sizeof(never_gonna) / sizeof(String));
  Serial.println("Setting SSID to '" + never_gonna[current_line] + "'.  "+ rtc.getTime("%A, %B %d %Y %H:%M:%S"));  
  WiFi.softAP(never_gonna[current_line].c_str());  
  return current_line;

}
