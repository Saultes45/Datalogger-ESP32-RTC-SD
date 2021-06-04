# Datalogger-ESP32-RTC-SD
This repository is made for people who want to use any ESP32 datalogging capabilities.

-------------------------

Hardware:

It has been tested with Adafruit gear: Feather Huzzah32 (ESP32 WROOM),  the feather datalogging shield (SD+RTC), and the 6-DoF IMU LSM6DS33

https://www.adafruit.com/product/3405

https://www.adafruit.com/product/2922

https://www.adafruit.com/product/4480


------------------------

Code:

The code in this repo has been developped and tested in Arduino 1.8.13 with the latest arduino libraries (as for 2021/04/28). Win 10 64bit.

This repository is made so that you can do 2 things: set the RTC clock to the correct date and time + use the datalogging capabilities.

The setting of the RTC code is heavily based on an existing code from "radames" for ESP8266 called "NTP_RTC_Sync" (https://github.com/radames/NTP_RTC_Sync).
It has been slighty modified to be ported for ESP32.

PS: I might add the Matlab code here to parse the data.


I hope this is going to help you.

Enjoy!


--------------------

EDIT: Added Matlab processing scripts

Just create a folder named "00_PUT_SD_CARD_DATA_HERE" ins the same folder as the matlab files. Put all the .txt that have been logged in there.

You just need to run this one: A_CalculateOrientationFromIMU.m

