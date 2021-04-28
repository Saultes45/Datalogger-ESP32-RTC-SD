/* ========================================
*
* Copyright Tonkin and Taylor Ltd, 2021
* All Rights Reserved
* UNPUBLISHED, LICENSED SOFTWARE.
*
* Metadata
* Written by    : Iain Billington, Nathanaël Esnault
* Verified by   : N/A
* Creation date : 2021-04-07
* Version       : 0.1 (finished on ...)
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
* Ressources
*
*
SD card: https://learn.adafruit.com/adafruit-adalogger-featherwing/using-the-sd-card
RTC: https://learn.adafruit.com/adafruit-adalogger-featherwing/adafruit2-rtc-with-arduino
IMU: lsm6ds33 https://www.adafruit.com/product/4480 Adafruit Guide: https://learn.adafruit.com/lsm6ds33-6-dof-imu=accelerometer-gyro

*Adafruit HUZZAH32 – ESP32 Feather Board https://www.adafruit.com/product/3405
*Adalogger FeatherWing - RTC + SD Add-on For All Feather Boards
*   -> I2C real time clock (PCF8523)
*   -> microSD socket that connects to the SPI port pins (+ extra pin for CS)
*Adafruit ISM330DHCX - 6 DoF IMU - Accelerometer and Gyroscope - STEMMA QT / Qwiic
*
* IMU datasheet https://www.st.com/resource/en/datasheet/<continue>
*
* TODO
* Create task to be sure to read @ 10Hz
* set ACC/GYR range and data rate : 
*
* ========================================
*/

// -------------------------- Includes --------------------------
// Our own library
#include "config.h"         // all the parameters

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

//RTC
#include <RTClib.h> // use the one from Adafruit


// -------------------------- Defines --------------------------
// All moved in "config.h" now

// -------------------------- Structs --------------------------
// All moved in "config.h" now

// -------------------------- Global variables ----------------

//TODO: put most of them in "config.h"
//IMU
Adafruit_LSM6DS33 lsm6ds33;

//RTC
RTC_PCF8523 rtc;
DateTime time_loop; // MUST be global!!!!! or it won't update
DateTime timestampForFileName; // MUST be global!!!!! or it won't update
 
//SD card
/* The SDCS pin is the chip select line.
  On ESP8266, the SD CS pin is on GPIO 15
  On ESP32 it's GPIO 33
  On WICED it's GPIO PB5
  On the nRF52832 it's GPIO 11
  On Atmel M0, M4, 328p or 32u4 it's on GPIO 10
  On Teensy 3.x it's on GPIO 10 */
//const int chipSelect = 33;

// 1 file = 1h recording @ 10Hz = 36,000 lines per files ~ 290KB
// if we lose a file we only lose 1h worth of data
uint16_t  cntLinesInFile  = 0; // written at the end of a file for check (36,000 < 65,535)
uint8_t   cntFile         = 0; // counter that counts the files, included in the name of the file
File      dataFile;            // only 1 file can be opened at a certain time, <KEEP GLOBAL>
String    fileName        = ""; // name of the file on the SD card



// -------------------------- Classes -------------------------
// NONE

// -------------------------- Functions declaration --------------------------
void changeCPUFrequency             (void); // Change thef CPU frequency and report about it over serial
void print_reset_reason             (RESET_REASON reason);
void verbose_print_reset_reason     (RESET_REASON reason);
void displayWakeUpReason            (void); // Display why the module went to sleep
void testSDCard                     (void); // Test that the SD card can be accessed
void testRTC                        (void); // Test that the RTC can be accessed
void createNewFile                  (void); // Create a name a new file on the SD card, file variable is global
void closeFile                      (void); // close the file currently being used, on the SD card, file variable is global
void error                          (uint8_t errno);

// -------------------------- Set up --------------------------

void setup() {
  Serial.begin(CONSOLE_BAUD_RATE); //Begin serial communication (USB)
  pinMode(LED_BUILTIN, OUTPUT);   //Set up the on-board LED (RED, close to the uUSB port)

  // Indicates the start of the setup with the red on-board LED
  //------------------------------------------------------------
  digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(10);                       // wait for a second
  
  delay(1000); // wait so I can open the console
  
  displayWakeUpReason();
  changeCPUFrequency();

  // Order of the last 3 tests can be changed
  testIMU();
  testSDCard();
  testRTC();

  // Create the first file
  //-------------------------
  createNewFile();

  // Indicates the end of the setup with the red on-board LED
  //---------------------------------------------------------
  digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
  delay(10);                       // wait for a second

}

// -------------------------- Loop --------------------------
void loop() {

  // make a string for assembling the data to log:
  String dataString = "";
  String myTimestamp = "";

  // Add start of message charachter
  //------------------------------------------
  dataString += SOM_LOG;

  // Read the RTC time
  //------------------------------------------
  time_loop = rtc.now(); // MUST be global!!!!! or it won't update
//  DateTime now = DateTime(2000,01,01, 00, 00, 00); // DEBUG

  //source https://github.com/adafruit/RTClib/blob/master/examples/toString/toString.ino
  // HERE -> look for user "cattledog" : https://forum.arduino.cc/t/now-rtc-gets-stuck-if-called-in-setup/632619/3 
  //buffer can be defined using following combinations:
  //hh - the hour with a leading zero (00 to 23)
  //mm - the minute with a leading zero (00 to 59)
  //ss - the whole second with a leading zero where applicable (00 to 59)
  //YYYY - the year as four digit number
  //YY - the year as two digit number (00-99)
  //MM - the month as number with a leading zero (01-12)
  //MMM - the abbreviated English month name ('Jan' to 'Dec')
  //DD - the day as number with a leading zero (01 to 31)
  //DDD - the abbreviated English day name ('Mon' to 'Sun')

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
  //  /* Get a new normalized sensor event */
  sensors_event_t accel;
  sensors_event_t gyro;
  sensors_event_t temp;
  lsm6ds33.getEvent( & accel, & gyro, & temp);

// The following is 
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
  
  
  dataString += String(temp.temperature, FORMAT_TEMP); // add to the data to write
  dataString += FORMAT_SEP;

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
  //dataString += FORMAT_END;
  
  #ifdef SERIAL_DEBUG
    Serial.println("Data going to SD card: ");
    Serial.println(dataString);
  #endif

  // Log data in the SD card
  //------------------------------------------

  // if the file is available, write to it:
  if (dataFile) {
    dataFile.println(dataString);
    //dataFile.close();
    cntLinesInFile++; // increment the counter

    #ifdef SERIAL_DEBUG
      Serial.println("Data have been written");
      Serial.print("Current number of lines: ");
      Serial.print(cntLinesInFile);
      Serial.print("/");
      Serial.println(MAX_LINES_PER_FILES);
    #endif
    
  }
  // if the file isn't open, pop up an error:
  else {
    #ifdef SERIAL_DEBUG
      Serial.print("Error writting to the file: ");
      Serial.println(fileName);
    #endif
    fileName = ""; // Reset the filename
    cntLinesInFile = 0; //Reset the line counter
  }

  /*
   * At this place, the line counter should never be 0 except if there has been a problem with opening the file
   * This is why we can use it as a check
   */
  if ((cntLinesInFile >= MAX_LINES_PER_FILES) | (cntLinesInFile == 0)) {
    // close the file
    dataFile.close();
    cntLinesInFile = 0;
    // create a new file
    createNewFile();
    #ifdef SERIAL_DEBUG
      Serial.println("Reached the max number of lines per file, starting a new one");
    #endif
  }

  // Wait before logging new sensor data
  //------------------------------------------
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

  Serial.println("LSM6DS33 Found!");


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
  Serial.println("------------------------");

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
  String testString = "test0123456789";

  // see if the card is present and can be initialized:
  if (!SD.begin(PIN_CS_SD)) {
    Serial.println("Card failed, or not present");
    error(2);
    // don't do anything more:
    while (1);
  }
  Serial.println("Card initialized");

      if (SD.exists("/test.txt")) {
      Serial.println("Looks like a test file already exits on the SD card");
    }

  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  File dataFile = SD.open("/test.txt", FILE_WRITE);

  // if the file is available, write to it:
  if (dataFile) {
    dataFile.println(testString);
    dataFile.close();
    // print to the serial port too:
    Serial.println(testString);
  }
  // if the file isn't open, pop up an error:
  else {
    Serial.println("Error while opening /test.txt");
    error(3);
  }

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
    Serial.println("RTC is NOT initialized, let's set the time!");
  }

    // When the RTC was stopped and stays connected to the battery, it has
  // to be restarted by clearing the STOP bit. Let's do this to ensure
  // the RTC is running.
    rtc.start();

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

  
}

//******************************************************************************************
void createNewFile(void) {

    cntFile ++; //increment the counter of file
  // To name the file we need to know the date : ask the RTC
  timestampForFileName = rtc.now(); // MUST be global!!!!! or it won't update
  fileName = ""; // Reset the filename
  fileName += "/"; // To tell it to put in the root folder, absolutely necessary

  char timeStamp[sizeof(timeStampFormat_FileName)]; // We are obliged to do that horror because the method "toString" input parameter is also the output
  strncpy(timeStamp, timeStampFormat_FileName, sizeof(timeStampFormat_FileName));
  fileName += timestampForFileName.toString(timeStamp);

  fileName += "-"; //add a separator between datetime and filenumber

  char buffer[5];
  sprintf(buffer, "%05d", cntFile); //Making sure the file number is always printed with 5 digits
//  Serial.println(buffer);
  fileName += String(buffer); 
  
  fileName += ".txt"; //add the extension


   if (SD.exists(fileName)) {
      Serial.print("Looks like a file already exits on the SD card with that name: ");
      Serial.println(fileName);
    }

  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  Serial.print("Creating this file on the SD card: ");
  Serial.println(fileName);
  dataFile = SD.open(fileName, FILE_WRITE);

}

//******************************************************************************************
void closeFile(void) {
  dataFile.close();
  // TODO:
  // 1) replace in the code
  // 2) add more set in this functions
  Serial.println("File closed");
}

//******************************************************************************************
void error(uint8_t errno) { // blink out an error code
  while(1) {
    uint8_t i;
    for (i=0; i<errno; i++) {
      digitalWrite(13, HIGH);
      delay(100);
      digitalWrite(13, LOW);
      delay(100);
    }
    for (i=errno; i<10; i++) {
      delay(200);
    }
  }
}


// END OF FILE
