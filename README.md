**JeeLib** is an Arduino IDE library for [JeeNodes][1] (made by [JeeLabs][2])  
and for compatible devices, with drivers for its wireless radio module,  
its "JeePort" interfaces, and a range of add-on "JeePlug" interfaces.

The home page for this library is at <https://jeelabs.net/projects/jeelib/wiki>.

[1]: https://jeelabs.net/projects/hardware/wiki
[2]: https://jeelabs.org/


This jeelib library is forked to use Arduino SPI library with radios RFM12b and RFM69CW. Arduino version 1.8.10 is used.

Included example RF12demo_SPI.ino (a modified RF12demo.ino) is working on boards or microcontrollers: 
  Jeenode, Arduino Uno, Arduino Nano, Arduino Mini Pro, Arduino ATmega 2560,
  
  ATmega328PB with MiniCore: https://mcudude.github.io/MiniCore/package_MCUdude_MiniCore_index.json ,
  
  Arduino Nano Every; http://downloads.arduino.cc/packages/package_avr_7.3.0_index.json
  
  LGT8F328 with core LGT8fx boards by buezas: https://raw.githubusercontent.com/dbuezas/lgt8fx/master/package_lgt8fx_index.json
  
  ESP8266 (Wemos D1 R1 Mini): http://arduino.esp8266.com/stable/package_esp8266com_index.json, 
  
  ESP32 (DOIT ESP32 DEVKIT V1): https://dl.espressif.com/dl/package_esp32_index.json, 
  
  STM32F103C8 (Blue Pill), STM32F103CBT6 (Maple Mini) with 
  
  STMduino.com core (Roger Clarck) http://dan.drown.org/stm32duino/package_STM32duino_index.json and 
  
  STM32 (official) core https://github.com/stm32duino/BoardManagerFiles/raw/master/STM32/package_stm_index.json.

Other MCUs than AVR dont have dedicated EEPROM and dont restore radio settings when programin with a different sketch.

Modified files in JeeLib: RF12.cpp, RF69.cpp, RF69_compat.cpp, RF69_avr.h, Ports.h, Ports.cpp, PortsSHT11.cpp. 
Added files to JeeLib: RF69_avr1.h and crc16.h.

Original JeeLib support continue to JeeNode and atmega328PA. Due errors in compiling Ports.cpp to other MCU:s there is no
or partial support to LuxPlug, ColorPlug, InputPlug, InfraredPlug, Sleep modes and PowerDown, -Saving. Not tryed  
Jeelabs module working with other MCU than ATMefa328PA.

Main goal to use radio modules RFM12b and RFM69CW with different microcontrollers.
Needed two hardware serial ports. Using MiniCore + SPI1 + Atmega328PB made it possible.

ESP32 and STM32F103X can use two SPI-bus. Used aither SPI or SPI1 succesfylly - see RF12demo_SPI.ino. 
Using both SPI and SPI1 bus on the same sketch is not supported.

Used radio modules RFM12b (#define RF69_COMPAT 0) or RFM69CW (#define RF69_COMPAT 1). 

SPI and SPI1 are supported. SPI is default. No software-SPI implementeed.

if SPI1 is used, "#define JEELIB_SPI1 1" in RF12.h  is needed. 

if SPI1 and RFM69CW is used, "#define JEELIB_SPI1 1" and "#define RF69_COMPAT 1" is needed in RF12.h and
"#define JEELIB_SPI1 1" in RF69.cpp and RF69_compat.cpp is needed.

This library must locate in username/Arduino/libraries/jeelib subfolder. The original jeelib subfolder must be renamed something like "jeelib_original" as long you use this fork. Arduino linker finds two jeelib but uses the one with subfolder name "jeelib". When not using this library, rename the subfolfer "jeelib" to  "jeelib_SPI" and "jeelib_original" to "jeelib" and everything is like in good old jeelib days.
I hope this fork is useful to someone.
