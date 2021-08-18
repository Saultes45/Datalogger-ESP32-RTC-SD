
/* ========================================
*
* Copyright ??
* All Rights Reserved
* UNPUBLISHED, LICENSED SOFTWARE.
*
* Metadata
* Written by    : ??
* Verified by   : Nathanael Esnault
* Creation date : ??
* Version       : 1.2 (finished on ...)
* Modifications :
* Known bugs    :
*
*
* Possible Improvements
*
* Notes
*
* Ressources (Boards + Libraries Manager)
*
* TODO
*
*
*/


// -------------------------- Includes [7] --------------------------
#include <WiFi.h>
#include <NTPClient.h> // NTPClient by Arduino
#include <WiFiUdp.h>
#include <Wire.h>
#include <EEPROM.h>
#include <RTClib.h>   // RTClib by Adafruit

#include "WifiPass.h" // <--- Where you put your WiFi credentials


// -------------------------- Defines and Const [3] --------------------------

//closest NTP Server
#define NTP_SERVER "pool.ntp.org"

//GMT Time Zone with sign
#define GMT_TIME_ZONE +0

//Force RTC update and store on EEPROM
//change this to a random number between 0-255 to force time update
#define FORCE_RTC_UPDATE 2

const uint8_t   LOG_PWR_PIN_1               = 25;  // To turn the adalogger Feather ON and OFF
const uint8_t   LOG_PWR_PIN_2               = 26;  // To turn the adalogger Feather ON and OFF
const uint8_t   LOG_PWR_PIN_3               = 12;  // To turn the adalogger Feather ON and OFF
const uint8_t   LOG_PWR_PIN_4               = 27;  // To turn the adalogger Feather ON and OFF


// -------------------------- Global Variables [5] --------------------------

char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

RTC_PCF8523 rtc;
WiFiUDP ntpUDP;

// You can specify the time server pool and the offset, (in seconds)
// additionaly you can specify the update interval (in milliseconds).
NTPClient timeClient(ntpUDP, NTP_SERVER, GMT_TIME_ZONE * 3600 , 60000);

int timeUpdated = 0;

// -------------------------- Functions declaration [1] --------------------------

//******************************************************************************************
void syncTime(void) 
{

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



// -------------------------- Set up --------------------------

void setup() 
{

  pinMode (LOG_PWR_PIN_1    , OUTPUT);
  pinMode (LOG_PWR_PIN_2    , OUTPUT);
  pinMode (LOG_PWR_PIN_3    , OUTPUT);
  pinMode (LOG_PWR_PIN_4    , OUTPUT);

  digitalWrite(LOG_PWR_PIN_1, HIGH);
  digitalWrite(LOG_PWR_PIN_2, HIGH);
  digitalWrite(LOG_PWR_PIN_3, HIGH);
  digitalWrite(LOG_PWR_PIN_4, HIGH);

  
	Serial.begin(115200);

	Serial.println("Testing the RTC...");

	if (!rtc.begin()) 
	{
		Serial.println("Couldn't find RTC");
		Serial.flush();
		abort();
	}

	if (!rtc.initialized() || rtc.lostPower()) 
	{
		Serial.println("RTC is NOT initialized");
		// When the RTC was stopped and stays connected to the battery, it has
		// to be restarted by clearing the STOP bit. Let's do this to ensure
		// the RTC is running.
		rtc.start();
	}



	Serial.println("Let's see if the RTC is running");
	Serial.println("There should be about 1s difference");
	
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

	syncTime();
	Serial.println("Updating EEPROM..");

}




// -------------------------- Loop --------------------------
void loop () 
{
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


	Serial.println();
	delay(1000);
}


// END OF THE FILE
