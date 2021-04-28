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
* TODO
*
*
* ========================================
*/




// -------------------------- Defines --------------------------

// CPU frequency
#define TARGET_CPU_FREQUENCY          80

// Global
#define CONSOLE_BAUD_RATE             115200  // Baudrate in [bauds] for serial communication to the console (used for debug only) TX0(GPIO_1) RX0(GPIO_3)
#define WAIT_LOOP                     100     // for the delay to wait at the end of the loop
//#define SERIAL_DEBUG        //if defined then the ESP32 will verbose to the console


// IMU
//#define LSM_CS 10 // For SPI mode, we need a CS pin
//// For software-SPI mode we need SCK/MOSI/MISO pins
//#define LSM_SCK 13
//#define LSM_MISO 12
//#define LSM_MOSI 11

// SD

#define USE_SD             // if defined then the ESP32 will log data to SD card (if not it will just read IMU) // <Not coded yet>
#define PIN_CS_SD   33     // Chip select for SPI for SD card
#define SOM_LOG     '$'   // Start of message
#define FORMAT_TEMP 1     // how many significative numbers for the temperature
#define FORMAT_ACC  6     // how many significative numbers for the accelerometers
#define FORMAT_GYR  6     // how many significative numbers for the gyroscopes
#define FORMAT_SEP  ','   // separator between the different filed so that the data can be read
#define FORMAT_END  "\r\n"   // End of line for 1 aquisition, to be printed in the SD card // <Not used>
#define MAX_LINES_PER_FILES 1000//36000   // maximum number of lines that we want stored in 1 SD card file. It should be about 1h worth

char timeStampFormat_Line[]     = "YYYY_MM_DD__hh_mm_ss"; // naming convention for each line of the file logged to the SD card
char timeStampFormat_FileName[] = "YYYY_MM_DD__hh_mm_ss"; // naming convention for each file name created on the SD card


// -------------------------- Structs --------------------------
//NONE


// END OF FILE
