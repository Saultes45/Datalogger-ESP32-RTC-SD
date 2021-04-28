#include "WifiPass.h"
//#include <ESP8266WiFi.h>
#include <WiFi.h>
#include <NTPClient.h> //NTPClient by Arduino
#include <WiFiUdp.h>
#include <Wire.h>
#include <EEPROM.h>
#include <RTClib.h> //RTClib by Adafruit

//closest NTP Server
#define NTP_SERVER "pool.ntp.org"

//GMT Time Zone with sign
#define GMT_TIME_ZONE +12

//Force RTC update and store on EEPROM
//change this to a random number between 0-255 to force time update
#define FORCE_RTC_UPDATE 2

char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

RTC_PCF8523 rtc;
WiFiUDP ntpUDP;

// You can specify the time server pool and the offset, (in seconds)
// additionaly you can specify the update interval (in milliseconds).
NTPClient timeClient(ntpUDP, NTP_SERVER, GMT_TIME_ZONE * 3600 , 60000);

int timeUpdated = 0;

void setup() {
  Serial.begin(115200);

   Serial.println("Testing the RTC...");

  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    abort();
  }

  if (!rtc.initialized() || rtc.lostPower()) {
    Serial.println("RTC is NOT initialized");
        // When the RTC was stopped and stays connected to the battery, it has
  // to be restarted by clearing the STOP bit. Let's do this to ensure
  // the RTC is running.
    rtc.start();
  }



    Serial.println("Let's see if the RTC is running");
    Serial.println("There should be about 1s drifference");
    
    DateTime now = rtc.now();
    Serial.print(now.year(), DEC);
    Serial.print('/');
    Serial.print(now.month(), DEC);
    Serial.print('/');
    Serial.print(now.day(), DEC);
    Serial.print(' ');
    Serial.print(now.hour(), DEC);
    Serial.print(':');
    Serial.print(now.minute(), DEC);
    Serial.print(':');
    Serial.print(now.second(), DEC);
    Serial.println();

    delay(1000);

    now = rtc.now();
    Serial.print(now.year(), DEC);
    Serial.print('/');
    Serial.print(now.month(), DEC);
    Serial.print('/');
    Serial.print(now.day(), DEC);
    Serial.print(' ');
    Serial.print(now.hour(), DEC);
    Serial.print(':');
    Serial.print(now.minute(), DEC);
    Serial.print(':');
    Serial.print(now.second(), DEC);
    Serial.println();

//  if (!rtc.begin()) {
//    Serial.println("Couldn't find RTC");
//    while (1);
//  }

//  EEPROM.begin(4);
//
//  Serial.println("Waiting to Start....");
//  delay(5000);
//
//  //Read the EEPROM to check if time has been synced
//
//  byte addvalue = EEPROM.read(timeUpdated);
//  Serial.print("EEPROM: ");
//  Serial.print(addvalue);
//  Serial.print(" == ");
//  Serial.print(FORCE_RTC_UPDATE);
//  Serial.println(" ?");
//  if (addvalue != FORCE_RTC_UPDATE) {
//    //if(true == false){
//    //time hasn' it been setup
//    Serial.println("Forcing Time Update");
    syncTime();
    Serial.println("Updating EEPROM..");
//    EEPROM.write(timeUpdated, FORCE_RTC_UPDATE);
//    EEPROM.commit();
//
//  } else {
//    Serial.println("Time has been updated before...EEPROM CHECKED");
//    Serial.print("EEPROM: ");
//    Serial.print(addvalue);
//    Serial.print(" = ");
//    Serial.print(FORCE_RTC_UPDATE);
//    Serial.println("!");
//  }



}

void syncTime(void) {

//connect to WiFi
  Serial.printf("Connecting to %s ", ssid);+
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
  }
  Serial.println(" CONNECTED");

  
  timeClient.begin();
  timeClient.update();

  long actualTime = timeClient.getEpochTime();
  Serial.print("Internet Epoch Time: ");
  Serial.println(actualTime);
  rtc.adjust(DateTime(actualTime));



  //Turn Off WIFI after update
  WiFi.disconnect();
  WiFi.mode(WIFI_OFF);
  //WiFi.forceSleepBegin();

}


void loop () {
  DateTime now = rtc.now();

  Serial.print(now.year(), DEC);
  Serial.print('/');
  Serial.print(now.month(), DEC);
  Serial.print('/');
  Serial.print(now.day(), DEC);
  Serial.print(" (");
  Serial.print(daysOfTheWeek[now.dayOfTheWeek()]);
  Serial.print(") ");
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  Serial.print(now.second(), DEC);
  Serial.println();

//  Serial.print(" since midnight 1/1/1970 = ");
//  Serial.print(now.unixtime());
//  Serial.print("s = ");
//  Serial.print(now.unixtime() / 86400L);
//  Serial.println("d");
//
//  // calculate a date which is 7 days and 30 seconds into the future
//  DateTime future (now + TimeSpan(7, 12, 30, 6));
//
//  Serial.print(" now + 7d + 30s: ");
//  Serial.print(future.year(), DEC);
//  Serial.print('/');
//  Serial.print(future.month(), DEC);
//  Serial.print('/');
//  Serial.print(future.day(), DEC);
//  Serial.print(' ');
//  Serial.print(future.hour(), DEC);
//  Serial.print(':');
//  Serial.print(future.minute(), DEC);
//  Serial.print(':');
//  Serial.print(future.second(), DEC);
//  Serial.println();

  Serial.println();
  delay(1000);
}
