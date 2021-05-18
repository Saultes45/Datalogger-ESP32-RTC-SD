/* ========================================
*
* Copyright Tonkin and Taylor Ltd, 2021
* All Rights Reserved
* UNPUBLISHED, LICENSED SOFTWARE.
*
* Metadata
* Written by    : Iain Billington, Nathanaël Esnault
* Verified by   : Nathanaël Esnault
* Creation date : 2021-04-07
* Version       : 1.1 (finished on 2021-05-12)
* Modifications :
* Known bugs    :
*
*
* Possible Improvements
*
*
* Notes
*
*
* Ressources (Boards + Libraries Manager)
*
*
* SD card:        https://learn.adafruit.com/adafruit-adalogger-featherwing/using-the-sd-card
* RTC:            https://learn.adafruit.com/adafruit-adalogger-featherwing/adafruit2-rtc-with-arduino
* IMU: lsm6ds33:  https://www.adafruit.com/product/4480 Adafruit Guide: https://learn.adafruit.com/lsm6ds33-6-dof-imu=accelerometer-gyro
*
* Adafruit ESP32 Feather Board (Adafruit HUZZAH32) https://www.adafruit.com/product/3405
* JSON for IDE pref: https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
* Adalogger FeatherWing - RTC + SD Add-on For All Feather Boards
*   -> I2C real time clock (PCF8523)
*   -> microSD socket that connects to the SPI port pins (+ extra pin for CS)
* Adafruit LSM6DS33 - 6 DoF IMU - Accelerometer and Gyroscope - STEMMA QT / Qwiic
*
* IMU datasheet https://www.st.com/resource/en/datasheet/lsm6ds33.pdf
*
* TODO
* Create task to be sure to read @ 10Hz
*
* ========================================
*/

// -------------------------- Includes --------------------------
// Our own library
#include "config.h"   // all the parameters

//ESP32
#include <esp_system.h>
#include <rom/rtc.h> // reset reason

//String Formatting
#include <string.h>
#include <stdio.h>

// IMU accelerometer 6DoF
// Adafruit LSM6DS - Version: Latest 
#include <Adafruit_LSM6DS33.h>

// SD card
#include <SPI.h>
#include <SD.h>

// RTC
#include <RTClib.h> // use the one from Adafruit, not the forks with the same name

// -------------------------- Defines --------------------------
// All moved in "config.h" now

// -------------------------- Structs --------------------------
// All moved in "config.h" now

// -------------------------- Global variables ----------------

//IMU
Adafruit_LSM6DS33 lsm6ds33;

//RTC
RTC_PCF8523 rtc;
DateTime time_loop;             // MUST be global!!!!! or it won't update
DateTime timestampForFileName;  // MUST be global!!!!! or it won't update

//SD card
// 1 file = 1.67min recording @ 10Hz = 1,000 lines per files ~ 85KB
// If we lose a file, or it gets corrupted, we only lose 1.67min worth of data
uint16_t  cntLinesInFile  = 0; // Written at the end of a file for check (36,000 < 65,535)
uint32_t  cntFile         = 0; // Counter that counts the files written in the SD card this session (we don't include prvious files), included in the name of the file, can handle 0d to 99999d (need 17 bits)
File      dataFile;            // Only 1 file can be opened at a certain time, <KEEP GLOBAL>
String    fileName        = "";// Name of the current opened file on the SD card

// Watchdog
void IRAM_ATTR resetModule() {
  ets_printf("Problem! Watchdog trigger: Rebooting...\r\n");
  esp_restart();
}

// -------------------------- Classes -------------------------
// NONE

// -------------------------- Functions declaration --------------------------
void changeCPUFrequency             (void);                 // Change the CPU frequency and report about it over serial
void print_reset_reason             (RESET_REASON reason);
void verbose_print_reset_reason     (RESET_REASON reason);
void displayWakeUpReason            (void);                 // Display why the module went to sleep
void testSDCard                     (void);                 // Test that the SD card can be accessed
void testRTC                        (void);                 // Test that the RTC can be accessed
void createNewFile                  (void);                 // Create a name a new DATA file on the SD card, file variable is global
void closeFile                      (void);                 // Close the file currently being used, on the SD card, file variable is global // <Not used>
void blinkAnError                   (uint8_t errno);        // Use an on-board LED (the red one close to the micro USB connector, left of the enclosure) to signal errors (RTC/SD)
void createNewSeparatorFile         (void);                 // Create a name a new SEPARATOR file on the SD card and close it immediatly (just a beautifier)

// -------------------------- Set up --------------------------

void setup() {
	Serial.begin(CONSOLE_BAUD_RATE); //Begin serial communication (USB)
	pinMode(LED_BUILTIN, OUTPUT);   //Set up the on-board LED (RED, close to the uUSB port)

	// Indicate the start of the setup with the red on-board LED
	//------------------------------------------------------------
	digitalWrite(LED_BUILTIN, HIGH);   // Turn the LED ON
	delay(10);                         // Wait a bit

	delay(CONSOLE_DELAY);             // Simple delay so I can open the console

	displayWakeUpReason();
	changeCPUFrequency();
	// The order of the last 3 tests can be changed
	testIMU();
	testSDCard();
	testRTC();

	// Create the first file
	//----------------------
	createNewFile();

	// Indicates the end of the setup with the red on-board LED
	//---------------------------------------------------------
	digitalWrite(LED_BUILTIN, LOW);  // Turn the LED OFF
	delay(10);                       // Wait a bit

  Serial.println("This might be the last transmission to the console if you turned verbose OFF");
  Serial.println("And here, we, go, ...");
  // Do not go gentle into that good night

  timer = timerBegin(0, 80, true);                  //timer 0, div 80
  timerAttachInterrupt(timer, &resetModule, true);  //attach callback
  timerAlarmWrite(timer, wdtTimeout * 1000, false); //set time in us
  timerAlarmEnable(timer);                          //enable interrupt
}

// -------------------------- Loop --------------------------
void loop() {

	// Create a string for assembling the data to log:
	String dataString  = "";
	String myTimestamp = "";

	// Add start of message charachter
	//--------------------------------
	dataString += SOM_LOG;

	// Read the RTC time
	//------------------------------------------
	time_loop = rtc.now(); // MUST be global!!!!! or it won't update
	//  DateTime now = DateTime(2000,01,01, 00, 00, 00); // DEBUG

	//source https://github.com/adafruit/RTClib/blob/master/examples/toString/toString.ino
	// HERE -> look for user "cattledog" : https://forum.arduino.cc/t/now-rtc-gets-stuck-if-called-in-setup/632619/3 
	//buffer can be defined using following combinations:
	// hh 	- hour with a leading zero (00 to 23)
	// mm 	- minute with a leading zero (00 to 59)
	// ss 	- whole second with a leading zero where applicable (00 to 59)
	// YYYY - year as four digit number
	// YY 	- year as two digit number (00-99)
	// MM 	- month as number with a leading zero (01-12)
	// MMM 	- abbreviated English month name ('Jan' to 'Dec')
	// DD 	- day as number with a leading zero (01 to 31)
	// DDD 	- abbreviated English day name ('Mon' to 'Sun')

	char timeStamp[sizeof(timeStampFormat_Line)]; // We are obliged to do that horror because the method "toString" input parameter is also the output
	strncpy(timeStamp, timeStampFormat_Line, sizeof(timeStampFormat_Line));
	dataString += time_loop.toString(timeStamp);

	dataString += FORMAT_SEP;

#ifdef SERIAL_DEBUG
	Serial.print("Time from RTC: ");
	Serial.println(time_loop.toString(timeStamp));
#endif

	// Read accelerometer data
	//------------------------

	// Get the IMU data
	//-----------------
	sensors_event_t accel;
	sensors_event_t gyro;
	sensors_event_t temp;
	lsm6ds33.getEvent( & accel, & gyro, & temp);

	// Print the raw IMU to the console if required
	//---------------------------------------------
#ifdef SERIAL_DEBUG
	Serial.print("\t\tTemperature ");
	Serial.print(temp.temperature);
	Serial.println(" deg C");
	Serial.print("\t\tAccel X: ");
	Serial.print(accel.acceleration.x);
	Serial.print(" \tY: ");
	Serial.print(accel.acceleration.y);
	Serial.print(" \tZ: ");
	Serial.print(accel.acceleration.z);
	Serial.println(" m/s^2 ");
	Serial.print("\t\tGyro X: ");
	Serial.print(gyro.gyro.x);
	Serial.print(" \tY: ");
	Serial.print(gyro.gyro.y);
	Serial.print(" \tZ: ");
	Serial.print(gyro.gyro.z);
	Serial.println(" radians/s ");
	Serial.println();
#endif

	// Build the String we are going to write to the SD card with the raw IMU data
	//----------------------------------------------------------------------------
	
  //dataString += String(temp.temperature, FORMAT_TEMP); // add to the data to write
	//dataString += FORMAT_SEP;

	/* Display the results (acceleration is measured in m/s^2) */
	dataString += String(accel.acceleration.x, FORMAT_ACC); // add to the data to write
	dataString += FORMAT_SEP;

	dataString += String(accel.acceleration.y, FORMAT_ACC); // add to the data to write
	dataString += FORMAT_SEP;

	dataString += String(accel.acceleration.z, FORMAT_ACC); // add to the data to write
	dataString += FORMAT_SEP;

	/* Display the results (rotation is measured in rad/s) */
	dataString += String(gyro.gyro.x, FORMAT_GYR); // add to the data to write
	dataString += FORMAT_SEP;

	dataString += String(gyro.gyro.y, FORMAT_GYR); // add to the data to write
	dataString += FORMAT_SEP;

	dataString += String(gyro.gyro.z, FORMAT_GYR); // add to the data to write
	//dataString += FORMAT_END; // Actually not needed since the function to write to the SD card aleady do that

#ifdef SERIAL_DEBUG
	Serial.println("Data going to SD card: ");
	Serial.println(dataString);
#endif

	// Log data in the SD card
	//------------------------

	// Check if the file is available
	if (dataFile) {
    if (cntLinesInFile >= MAX_LINES_PER_FILES - 1) { // Check if we have reached the max. number of lines per file
    // Write to the file w/out the "\r\n"
    dataFile.print(dataString);
    // Close the file
    dataFile.close();
    // Reset the line counter
    cntLinesInFile = 0;
    // Create a new file
    createNewFile();
    #ifdef SERIAL_DEBUG
      Serial.println("Reached the max number of lines per file, starting a new one");
    #endif
  }
  else
  {
    dataFile.println(dataString);
		cntLinesInFile++; // Increment the lines-in-current-file counter

		#ifdef SERIAL_DEBUG
      Serial.println("Data have been written");
      Serial.print("Current number of lines: ");
      Serial.print(cntLinesInFile);
      Serial.print("/");
      Serial.println(MAX_LINES_PER_FILES);
		#endif
  }
		
	}
	// If the file isn't open, pop up an error
	else {
		#ifdef SERIAL_DEBUG
      Serial.print("Error writting to the file: ");
      Serial.println(fileName);
		#endif
		fileName = "";        // Reset the filename
		cntLinesInFile = 0;   // Reset the lines-in-current-file counter
	}

	/*
  * At this place, the line counter should never be 0 except if there has been a problem with opening the file
  * This is why we can use it as a check
  */

  // Reset the timer (i.e. feed the watchdog)
  //------------------------------------------  
  timerWrite(timer, 0);
  delay(3000); // DEBUG: Trigger the watchdog

	// Wait before logging new sensor data
	//------------------------------------
	delay(WAIT_LOOP); //  TODO: use freeRTOS tasks: https://randomnerdtutorials.com/esp32-dual-core-arduino-ide/

}

//******************************************************************************************
void changeCPUFrequency(void) {
	Serial.println("----------------------------------------");
	Serial.print("Current CPU frequency [MHz]: ");
	Serial.println(getCpuFrequencyMhz());
	Serial.println("Changing...");
	setCpuFrequencyMhz(TARGET_CPU_FREQUENCY);
	Serial.print("Current CPU frequency [MHz]: ");
	Serial.println(getCpuFrequencyMhz());
}

//******************************************************************************************
void verbose_print_reset_reason(RESET_REASON reason) {
	switch (reason) {
	case 1:
		Serial.println("Vbat power on reset");
		break;
	case 3:
		Serial.println("Software reset digital core");
		break;
	case 4:
		Serial.println("Legacy watch dog reset digital core");
		break;
	case 5:
		Serial.println("Deep Sleep reset digital core");
		break;
	case 6:
		Serial.println("Reset by SLC module, reset digital core");
		break;
	case 7:
		Serial.println("Timer Group0 Watch dog reset digital core");
		break;
	case 8:
		Serial.println("Timer Group1 Watch dog reset digital core");
		break;
	case 9:
		Serial.println("RTC Watch dog Reset digital core");
		break;
	case 10:
		Serial.println("Instrusion tested to reset CPU");
		break;
	case 11:
		Serial.println("Time Group reset CPU");
		break;
	case 12:
		Serial.println("Software reset CPU");
		break;
	case 13:
		Serial.println("RTC Watch dog Reset CPU");
		break;
	case 14:
		Serial.println("for APP CPU, reseted by PRO CPU");
		break;
	case 15:
		Serial.println("Reset when the vdd voltage is not stable");
		break;
	case 16:
		Serial.println("RTC Watch dog reset digital core and rtc module");
		break;
	default:
		Serial.println("NO_MEAN");
	}
}

//******************************************************************************************
void displayWakeUpReason(void) {
	Serial.println("----------------------------------------");
	Serial.print("CPU0 reset reason: ");
	//print_reset_reason(rtc_get_reset_reason(0));
	verbose_print_reset_reason(rtc_get_reset_reason(0));
	Serial.print("CPU1 reset reason: ");
	//print_reset_reason(rtc_get_reset_reason(1));
	verbose_print_reset_reason(rtc_get_reset_reason(1));
}

//******************************************************************************************
void testIMU(void) {
	Serial.println("Adafruit LSM6DS33 test");

	if (!lsm6ds33.begin_I2C()) {
		Serial.println("Failed to find LSM6DS33 chip");
		while (1) {
			delay(10);
		}
	}

	Serial.println("LSM6DS33 has been found!");


	Serial.println("LSM6DS33 sensors ranges");
	Serial.println("------------------------");

	lsm6ds33.setAccelRange(LSM6DS_ACCEL_RANGE_16_G);
	Serial.print("Accelerometer range set to: ");
	switch (lsm6ds33.getAccelRange()) {
	case LSM6DS_ACCEL_RANGE_2_G:
		Serial.println("+-2G");
		break;
	case LSM6DS_ACCEL_RANGE_4_G:
		Serial.println("+-4G");
		break;
	case LSM6DS_ACCEL_RANGE_8_G:
		Serial.println("+-8G");
		break;
	case LSM6DS_ACCEL_RANGE_16_G:
		Serial.println("+-16G");
		break;
	}

	lsm6ds33.setGyroRange(LSM6DS_GYRO_RANGE_125_DPS);
	Serial.print("Gyro range set to: ");
	switch (lsm6ds33.getGyroRange()) {
	case LSM6DS_GYRO_RANGE_125_DPS:
		Serial.println("125 degrees/s");
		break;
	case LSM6DS_GYRO_RANGE_250_DPS:
		Serial.println("250 degrees/s");
		break;
	case LSM6DS_GYRO_RANGE_500_DPS:
		Serial.println("500 degrees/s");
		break;
	case LSM6DS_GYRO_RANGE_1000_DPS:
		Serial.println("1000 degrees/s");
		break;
	case LSM6DS_GYRO_RANGE_2000_DPS:
		Serial.println("2000 degrees/s");
		break;
	case ISM330DHCX_GYRO_RANGE_4000_DPS:
		break; // unsupported range for the DS33
	}


	Serial.println("LSM6DS33 sensors data rate");
	Serial.println("--------------------------");

	lsm6ds33.setAccelDataRate(LSM6DS_RATE_12_5_HZ);
	Serial.print("Accelerometer data rate set to: ");
	switch (lsm6ds33.getAccelDataRate()) {
	case LSM6DS_RATE_SHUTDOWN:
		Serial.println("0 Hz");
		break;
	case LSM6DS_RATE_12_5_HZ:
		Serial.println("12.5 Hz");
		break;
	case LSM6DS_RATE_26_HZ:
		Serial.println("26 Hz");
		break;
	case LSM6DS_RATE_52_HZ:
		Serial.println("52 Hz");
		break;
	case LSM6DS_RATE_104_HZ:
		Serial.println("104 Hz");
		break;
	case LSM6DS_RATE_208_HZ:
		Serial.println("208 Hz");
		break;
	case LSM6DS_RATE_416_HZ:
		Serial.println("416 Hz");
		break;
	case LSM6DS_RATE_833_HZ:
		Serial.println("833 Hz");
		break;
	case LSM6DS_RATE_1_66K_HZ:
		Serial.println("1.66 KHz");
		break;
	case LSM6DS_RATE_3_33K_HZ:
		Serial.println("3.33 KHz");
		break;
	case LSM6DS_RATE_6_66K_HZ:
		Serial.println("6.66 KHz");
		break;
	}

	lsm6ds33.setGyroDataRate(LSM6DS_RATE_12_5_HZ);
	Serial.print("Gyro data rate set to: ");
	switch (lsm6ds33.getGyroDataRate()) {
	case LSM6DS_RATE_SHUTDOWN:
		Serial.println("0 Hz");
		break;
	case LSM6DS_RATE_12_5_HZ:
		Serial.println("12.5 Hz");
		break;
	case LSM6DS_RATE_26_HZ:
		Serial.println("26 Hz");
		break;
	case LSM6DS_RATE_52_HZ:
		Serial.println("52 Hz");
		break;
	case LSM6DS_RATE_104_HZ:
		Serial.println("104 Hz");
		break;
	case LSM6DS_RATE_208_HZ:
		Serial.println("208 Hz");
		break;
	case LSM6DS_RATE_416_HZ:
		Serial.println("416 Hz");
		break;
	case LSM6DS_RATE_833_HZ:
		Serial.println("833 Hz");
		break;
	case LSM6DS_RATE_1_66K_HZ:
		Serial.println("1.66 KHz");
		break;
	case LSM6DS_RATE_3_33K_HZ:
		Serial.println("3.33 KHz");
		break;
	case LSM6DS_RATE_6_66K_HZ:
		Serial.println("6.66 KHz");
		break;
	}

	lsm6ds33.configInt1(false, false, true); // accelerometer DRDY on INT1
	lsm6ds33.configInt2(false, true, false); // gyro DRDY on INT2
}

//******************************************************************************************
void testSDCard(void) {
	Serial.print("Initializing SD card...");
	String testString = "Test 0123456789";

	// see if the card is present and can be initialized:
	if (!SD.begin(PIN_CS_SD)) {
		Serial.println("Card failed, or not present");
		blinkAnError(2);
		// Don't do anything more: infinite loop just here
		while (1);
	}
	Serial.println("Card initialized");

	if (SD.exists("/00_test.txt")) { // The "00_" prefix is to make sure it is displayed by win 10 explorer at the top
		Serial.println("Looks like a test file already exits on the SD card"); // Just a warning
  }

	// Create and open the test file. Note that only one file can be open at a time,
	// so you have to close this one before opening another.
	File dataFile = SD.open("/00_test.txt", FILE_WRITE);

	// if the file is available, write to it:
	if (dataFile) {
		dataFile.println(testString);
		dataFile.close();
		// print to the serial port too:
		Serial.println(testString);
	}
	// If the file isn't open, pop up an error:
	else {
		Serial.println("Error while opening /00_test.txt");
		blinkAnError(3);
	}

	// Add a separator file (empty but with nice title) so the user knows a new DAQ session started
	createNewSeparatorFile();

}

//******************************************************************************************
void testRTC(void) {

	Serial.println("Testing the RTC...");

	if (!rtc.begin()) {
		Serial.println("Couldn't find RTC");
		Serial.flush();
		abort();
	}

	if (!rtc.initialized() || rtc.lostPower()) {
		Serial.println("RTC is NOT initialized. Use the NTP sketch to set the time!");
    //blinkAnError(6);
	}

	// When the RTC was stopped and stays connected to the battery, it has
	// to be restarted by clearing the STOP bit. Let's do this to ensure
	// the RTC is running.
	rtc.start();

	Serial.println("Let's see if the RTC is running");
	Serial.println("There should a difference of about 1s");
	
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

	delay(1000); // Wait 1s so the user can see if the RTC is running by looking at the console

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

  Serial.println("Please check");

}

//******************************************************************************************
void createNewFile(void) {

	cntFile ++; // Increment the counter of files
	
  // To name the file we need to know the date : ask the RTC
	timestampForFileName = rtc.now(); // MUST be global!!!!! or it won't update
	fileName = "";                    // Reset the filename
	fileName += "/";                  // To tell it to put in the root folder, absolutely necessary

	char timeStamp[sizeof(timeStampFormat_FileName)]; // We are obliged to do that horror because the method "toString" input parameter is also the output
	strncpy(timeStamp, timeStampFormat_FileName, sizeof(timeStampFormat_FileName));
	fileName += timestampForFileName.toString(timeStamp);

	fileName += "-"; // Add a separator between datetime and filenumber

	char buffer[5];
	sprintf(buffer, "%05d", cntFile); // Making sure the file number is always printed with 5 digits
	//  Serial.println(buffer);
	fileName += String(buffer); 

	fileName += ".txt"; // Add the file extension


	if (SD.exists(fileName)) {
		Serial.print("Looks like a file already exits on the SD card with that name: ");
		Serial.println(fileName);
	}

	// open the file. note that only one file can be open at a time,
	// so you have to close this one before opening another.
	Serial.print("Creating the following file on the SD card: ");
	Serial.println(fileName);
	dataFile = SD.open(fileName, FILE_WRITE);

}

//******************************************************************************************
void closeFile(void) { // <Not used>
	dataFile.close();
	// TODO:
	// 1) Replace in the code
	// 2) Add more set in this functions
	Serial.println("File closed");
}

//******************************************************************************************
void blinkAnError(uint8_t errno) {  // Use an on-board LED (the red one close to the micro USB connector, left of the enclosure) to signal errors (RTC/SD)
	
  //errno argument tells how many blinks per period to do. Must be  strictly less than 10
  
  while(1) { // Infinite loop: stay here until power cycle
		uint8_t i;

    // This part is executed errno times, quick blink
		for (i=0; i<errno; i++) {
			digitalWrite(LED_BUILTIN, HIGH);
			delay(100);
			digitalWrite(LED_BUILTIN, LOW);
			delay(100);
		}


    // This part is executed (10 - errno) times, led off (waiting to reblink)
		for (i=errno; i<10; i++) {
			delay(200);
		}

    // Total time spent is: errno * (100 + 100) + (10 - errno) * 200 = 2000ms
	}
}

//******************************************************************************************
void createNewSeparatorFile(void) {

	// We do NOT increment the file counter because this file is NOT for data

	// To name the file we need to know the date : ask the RTC
	timestampForFileName = rtc.now(); // MUST be global!!!!! or it won't update
	fileName = "";    // Reset the filename
	fileName += "/";  // To tell it to put in the root folder, absolutely necessary

	char timeStamp[sizeof(timeStampFormat_FileName)]; // We are obliged to do that horror because the method "toString" input parameter is also the output
	strncpy(timeStamp, timeStampFormat_FileName, sizeof(timeStampFormat_FileName));
	fileName += timestampForFileName.toString(timeStamp);

  fileName += "_U";
  fileName += UNIT_NUMBER; // Add the unit number to the file name to identify quickly from which unit the files associated to this session are from
  fileName += "_";

	fileName += SESSION_SEPARATOR_STRING; // Add a separator marker after the date

	fileName += ".txt"; // Add the extension (we do bad things but we do have principles)


	if (SD.exists(fileName)) {
		Serial.print("Looks like a file already exits on the SD card with that name: ");
		Serial.println(fileName);
	}

	// Open the file. Note that only one file can be open at a time,
	// so you have to close this one before opening another.
	Serial.print("Creating this SEPARATOR file on the SD card: ");
	Serial.println(fileName);
	dataFile = SD.open(fileName, FILE_WRITE);

	// Close this file immediately so there are no access problem
	// This file is SUPPOSED to be empty. The Matlab parser code will IGNORE it
	dataFile.close();

}


// END OF FILE
