/* ========================================
*
* Copyright Tonkin and Taylor Ltd, 2021
* All Rights Reserved
* UNPUBLISHED, LICENSED SOFTWARE.
*
* Metadata
* Written by    : Iain Billington, Nathanaël Esnault
* Verified by   : N/A
* Creation date : 2021-05-19
* Version       : 1.2 (finished on ...)
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


#define UNIT_NUMBER                   2       // Number of the unit to identify the files (should be 1, 2, 3, or 4)

// CPU frequency
#define MAX_CPU_FREQUENCY             240
#define TARGET_CPU_FREQUENCY          40//80

// FreeRTOS
#if CONFIG_FREERTOS_UNICORE
#define ARDUINO_RUNNING_CORE 0
#else
#define ARDUINO_RUNNING_CORE 1
#endif

// Global
#define CONSOLE_BAUD_RATE             115200  // Baudrate in [bauds] for serial communication to the console (used for debug only) TX0(GPIO_1) RX0(GPIO_3)
#define WAIT_LOOP_MS                  100     // For the delay to wait, in [ms] at the end of the loop
#define CONSOLE_DELAY				          1000
#define US_TO_MS_CONVERSION           1000    // For the ISR task
//#define SERIAL_DEBUG        				        // If defined then the ESP32 will verbose to the console


// IMU (if the communication SPI pins are soldered, we chose I2C)
//#define LSM_CS 	  10
//#define LSM_SCK 	13
//#define LSM_MISO 	12
//#define LSM_MOSI 	11

// LOG
#define USE_SD             		  	// If defined then the ESP32 will log data to SD card (if not it will just read IMU) // <Not coded yet>
#define PIN_CS_SD   		33     		// Chip Select (ie CS/SS) for SPI for SD card
#define SOM_LOG     		'$'   		// Start of message indicator, mostly used for heath check (no checksum)
#define FORMAT_TEMP 		1     		// Numbers significative digits for the TEMPERATURE
#define FORMAT_ACC  		6     		// Numbers significative digits for the ACCELEROMETERS
#define FORMAT_GYR  		6     		// Numbers significative digits for the GYROSCOPES
#define FORMAT_SEP  		','    		// Separator between the different files so that the data can be read/parsed by softwares
#define FORMAT_END  		"\r\n"  	// End of line for 1 aquisition, to be printed in the SD card // <Not used>
#define MAX_LINES_PER_FILES 18000 // Maximum number of lines that we want stored in 1 SD card file. It should be about 1h worth
#define SESSION_SEPARATOR_STRING "----------------------------------"

// -------------------------- Global variables ----------------

// LOG
char timeStampFormat_Line[]     = "YYYY_MM_DD__hh_mm_ss"; // naming convention for EACH LINE OF THE FILE logged to the SD card
char timeStampFormat_FileName[] = "YYYY_MM_DD__hh_mm_ss"; // naming convention for EACH FILE NAME created on the SD card

// Watchdog
hw_timer_t *timer     = NULL;
const int wdtTimeout  = 3000;    // Time in ms to trigger the watchdog


//SD card
// 1 file = 1.67min recording @ 10Hz = 1,000 lines per files ~ 79KB
// If we lose a file, or it gets corrupted, we only lose 1.67min worth of data
uint16_t  cntLinesInFile  = 0; // Written at the end of a file for check (36,000 < 65,535)
uint32_t  cntFile         = 0; // Counter that counts the files written in the SD card this session (we don't include prvious files), included in the name of the file, can handle 0d to 99999d (need 17 bits)
String    fileName        = "";// Name of the current opened file on the SD card



// -------------------------- Structs --------------------------
//NONE

// -------------------------- Classes --------------------------
//NONE


// END OF FILE
