/// @dir RF12demo
/// Configure some values in EEPROM for easy config of the RF12 later on.
// 2009-05-06 <jc@wippler.nl> http://opensource.org/licenses/mit-license.php

// this version adds flash memory support, 2009-11-19
// Adding frequency features, author JohnO, 2013-09-05
// Major EEPROM format change, refactoring, and cleanup for v12, 2014-02-13

// This version uses Arduino 1.8.10 and Arduino boards own SPI-library. Jeelibs builtin SPI is removed
// JeeLib is forked to make it compatible with Arduinos SPI-library and various microcontrollers
// Tryed and found working on boards or microcontrollers: 
// Jeenode, Arduino Uno, Arduino Nano, Arduino Mini Pro Arduino ATmega 2560, ATmega328PB, Arduino Nano Every
// LGT8F328 with core LGT8fx boards by buezas
// ESP8266 (Wemos D1 R1 Mini), ESP32 (DOIT ESP32 DEVKIT V1), 
// STM32F103C8 (Blue Pill), STM32F103CBT6 (Maple Mini) with STMduino.com core (Roger Clarck)  and STM32 (official) core
// Modified files in JeeLib: RF12.cpp, RF69.cpp, RF69_compat.cpp, RF69_avr.h, Ports.h, Ports.cpp, PortsSHT11.cpp
// Added files to JeeLib: RF69_avr1.h, crc16.h
// Original JeeLib support continue to JeeNode and atmega328PA. Due errors in compiling Ports.cpp to other MCU:s there is no 
// or partial support to LuxPlug, ColorPlug, InputPlug, InfraredPlug, Sleep modes and PowerDown, -Saving. 
// Main goal is using radio modules RFM12b and RFM69CW with different microcontrollers.
// Needed two hardware serial ports. Using MiniCore + SPI1 + Atmega328PB made it possible.
// ESP32 and STM32F103X can use two SPI-bus. Used aither SPI or SPI1 succesfylly - see RF12demo_SPI.ino. 
// Using both SPI and SPI1 bus on the same scketc is not supported.
// Used radio modules RFM12b (#define RF69_COMPAT 0) or RFM69CW (#define RF69_COMPAT 1) 
// SPI and SPI1 are supported. SPI id default. No software-SPI implementeed.
// if SPI1 is used, "#define JEELIB_SPI1 1" in RF12.h  is needed 
// if SPI1 and RFM69CW is used, "#define JEELIB_SPI1 1" and "#define RF69_COMPAT 1" is needed in RF12.h and
// "#define JEELIB_SPI1 1" in RF69.cpp and RF69_compat.cpp is needed.



//#define RF69_COMPAT 1 // define this to use the RF69 driver i.s.o. RF12
// #define JEELIB_SPI1 1 = define in a file RF12.h if using SPI1
//#define RF12_COMPAT 1  = define RFM12b in a file  RF12.h to emulate RFM69CW native mode
#include <JeeLib.h>

#if defined (__LGT8FX8P__)
  #include <util/crc16.h>
  //#include <avr/eeprom.h>
  #include <avr/pgmspace.h>
  #include <util/parity.h>
  #include <EEPROM.h>
#elif defined(ARDUINO_ARCH_AVR)  || defined(ARDUINO_ARCH_MEGAAVR)
 // #warning "arduino and every"
  #include <util/crc16.h>
  #include <avr/eeprom.h>
  #include <avr/pgmspace.h>
  #include <util/parity.h>
#elif defined (ESP8266) || defined  (ESP8266_GENERIC) || defined  (ESP32)
  #if defined  (ESP32)
    //ESP32 word is 32 bit and arduino word is 16 bit long
    //https://github.com/espressif/arduino-esp32/issues/1745
    #define word uint16_t 
  #endif
  #include <pgmspace.h>
  // emulates AVR processors EEPROM in FLASH memory
  #include <EEPROM.h>
  // same code as in util/crc16.h
  extern "C" {uint16_t _crc16_update(uint16_t crc, uint8_t a);}
  //  #include <parity.h>  //found parity.h for ESP8266/32 and others:
  //https://stackoverflow.com/questions/21617970/how-to-check-if-value-has-even-parity-of-bits-or-odd/21618038
  int parity_even_bit(uint16_t x) {
  uint16_t count = 0, i, b = 1;

  for (i = 0; i < 8; i++) {
    if ( x & (b << i) ) {
      count++;
    }
  }

  if ( (count % 2) ) {
    return 0;
  }
  return 1;
  }
#elif defined (MCU_STM32F103C8) || defined(MCU_STM32F103CB) || defined(MCU_STM32F103CBT6) || defined(MCU_STM32F103RB) || defined(STM32F103xB)
  #define word uint16_t //MCU_STM32 word is 32 bit long, Arduino word 16 bit long 
  #if defined(STM_NUCLEO_F103RB)
    #undef LED_PIN
  #elif defined(MCU_STM32F103CB) //Maple mini
    #define LED_PIN 33
  #else
    #define LED_PIN PC13
  #endif
  #include <avr/pgmspace.h>
  #include <EEPROM.h>
  extern "C" {uint16_t _crc16_update(uint16_t crc, uint8_t a);}

  //  #include <parity.h>  //found no ready parity.h in STM32F103 library, using code:
  //https://stackoverflow.com/questions/21617970/how-to-check-if-value-has-even-parity-of-bits-or-odd/21618038
  int parity_even_bit(uint16_t x) {
  uint16_t count = 0, i, b = 1;
  for (i = 0; i < 8; i++) {
    if ( x & (b << i) ) {
      count++;
    }
   }
    if ( (count % 2) ) {
    return 0;
     }
    return 1;
  }

#else
  //#define word uint16_t 
#endif

#define MAJOR_VERSION RF12_EEPROM_VERSION // bump when EEPROM layout changes
#define MINOR_VERSION 2                   // bump on other non-trivial changes
#define VERSION "[RF12demo.12]"           // keep in sync with the above

#if defined(__AVR_ATtiny84__) || defined(__AVR_ATtiny44__)
#define TINY        1
#define SERIAL_BAUD 38400   // can only be 9600 or 38400
#define DATAFLASH   0       // do not change
#undef  LED_PIN             // do not change
#define rf12_configDump()   // disabled
#else
#define TINY        0

#define SERIAL_BAUD 57600   // adjust as needed
//#define SERIAL_BAUD 9600   // adjust as needed
#define DATAFLASH   0       // set to 0 for non-JeeLinks, else 4/8/16 (Mbit)
//#define LED_PIN     9       // activity LED, comment out to disable
#endif

/// Save a few bytes of flash by declaring const if used more than once.
const char INVALID1[] PROGMEM = "\rInvalid\n";
const char INITFAIL[] PROGMEM = "config save failed\n";

#if TINY
// Serial support (output only) for Tiny supported by TinyDebugSerial
// http://www.ernstc.dk/arduino/tinycom.html
// 9600, 38400, or 115200
// hardware\jeelabs\avr\cores\tiny\TinyDebugSerial.h Modified to
// moveTinyDebugSerial from PB0 to PA3 to match the Jeenode Micro V3 PCB layout
// Connect Tiny84 PA3 to USB-BUB RXD for serial output from sketch.
// Jeenode AIO2
//
// With thanks for the inspiration by 2006 David A. Mellis and his AFSoftSerial
// code. All right reserved.
// Connect Tiny84 PA2 to USB-BUB TXD for serial input to sketch.
// Jeenode DIO2
// 9600 or 38400 at present.

#if SERIAL_BAUD == 9600
#define BITDELAY 54          // 9k6 @ 8MHz, 19k2 @16MHz
#endif
#if SERIAL_BAUD == 38400
#define BITDELAY 11         // 38k4 @ 8MHz, 76k8 @16MHz
#endif

#define _receivePin 8
static int _bitDelay;
static char _receive_buffer;
static byte _receive_buffer_index;

static void showString (PGM_P s); // forward declaration

ISR (PCINT0_vect) {
  char i, d = 0;
  if (digitalRead(_receivePin))       // PA2 = Jeenode DIO2
    return;             // not ready!
  whackDelay(_bitDelay - 8);
  for (i = 0; i < 8; i++) {
    whackDelay(_bitDelay * 2 - 6);  // digitalread takes some time
    if (digitalRead(_receivePin)) // PA2 = Jeenode DIO2
      d |= (1 << i);
  }
  whackDelay(_bitDelay * 2);
  if (_receive_buffer_index)
    return;
  _receive_buffer = d;                // save data
  _receive_buffer_index = 1;  // got a byte
}

// TODO: replace with code from the std avr libc library:
//  http://www.nongnu.org/avr-libc/user-manual/group__util__delay__basic.html
void whackDelay (word delay) {
  byte tmp = 0;

  asm volatile("sbiw      %0, 0x01 \n\t"
               "ldi %1, 0xFF \n\t"
               "cpi %A0, 0xFF \n\t"
               "cpc %B0, %1 \n\t"
               "brne .-10 \n\t"
               : "+r" (delay), "+a" (tmp)
               : "0" (delay)
              );
}

static byte inChar () {
  byte d;
  if (! _receive_buffer_index)
    return -1;
  d = _receive_buffer; // grab first and only byte
  _receive_buffer_index = 0;
  return d;
}

#endif

static unsigned long now () {
  // FIXME 49-day overflow
  return millis() / 1000;
}

static void activityLed (byte on) {
#ifndef MCU_STM32F103RB
#ifdef LED_PIN
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, !on);
#endif
#endif
}

static void printOneChar (char c) {
  Serial.print(c);
}

static void showString (PGM_P s) {
  for (;;) {
    char c = pgm_read_byte(s++);
    if (c == 0)
      break;
    if (c == '\n')
      printOneChar('\r');
    printOneChar(c);
  }
}

static void displayVersion () {
  showString(PSTR(VERSION));
#if TINY
  showString(PSTR(" Tiny"));
#endif
}

/// @details
/// For the EEPROM layout, see http://jeelabs.net/projects/jeelib/wiki/RF12demo
// RF12 configuration area
//typedef struct {
//    byte nodeId;            // used by rf12_config, offset 0
//    byte group;             // used by rf12_config, offset 1
//    byte format;            // used by rf12_config, offset 2
//    byte hex_output   :2;   // 0 = dec, 1 = hex, 2 = hex+ascii
//    byte collect_mode :1;   // 0 = ack, 1 = don't send acks
//    byte quiet_mode   :1;   // 0 = show all, 1 = show only valid packets
//    byte spare_flags  :4;
//    word frequency_offset;  // used by rf12_config, offset 4
//    byte pad[RF12_EEPROM_SIZE-8];
//    word crc;
//} RF12Config;

typedef struct {
  byte nodeId;            // used by rf12_config, offset 0
  byte group;             // used by rf12_config, offset 1
  byte format;            // used by rf12_config, offset 2
  byte hex_output   : 2;  // 0 = dec, 1 = hex, 2 = hex+ascii
  byte collect_mode : 1;  // 0 = ack, 1 = don't send acks
  byte quiet_mode   : 1;  // 0 = show all, 1 = show only valid packets
  byte spare_flags  : 4;
  uint16_t frequency_offset;  // used by rf12_config, offset 4
  byte pad[RF12_EEPROM_SIZE - 8]; // offset 6-13  
  uint16_t crc;
} RF12Config;

static RF12Config config;
static char cmd;
//static word value;
static uint16_t pvalue;
static byte stack[RF12_MAXDATA + 4], top, sendLen, dest;
static byte testCounter;

static void showNibble (byte nibble) {
  char c = '0' + (nibble & 0x0F);
  if (c > '9')
    c += 7;
  Serial.print(c);
}

static void showByte (byte pvalue) {
  if (config.hex_output) {
    showNibble(pvalue >> 4);
    showNibble(pvalue);
  } else
    Serial.print((word) pvalue);
  //  Serial.print(" showByte ()");//debug
}

static word calcCrc (const void* ptr, byte len) {
  // word crc = ~0;
  uint16_t crc = ~0;
  for (byte i = 0; i < len; ++i)
  {
    crc = _crc16_update(crc, ((const byte*) ptr)[i]);
  }
  // Serial.print("calcCrc ");Serial.println(crc);
  return crc;

}

static void loadConfig () {
  // this uses 166 bytes less flash than eeprom_read_block(), no idea why
#if defined (__LGT8FX8P__)
 for (byte i = 0; i < sizeof config; ++ i)
  { ((byte*)&config)[i] = EEPROM.read((int)RF12_EEPROM_ADDR + i);
    // Serial.print(i);Serial.print(" ");Serial.println(EEPROM.read((int)RF12_EEPROM_ADDR + i));
  }  
#elif defined(ARDUINO_ARCH_AVR) || defined(ARDUINO_ARCH_MEGAAVR)
  for (byte i = 0; i < sizeof config; ++ i)
    ((byte*) &config)[i] = eeprom_read_byte(RF12_EEPROM_ADDR + i);
#elif defined (ESP8266) || defined  (ESP8266_GENERIC) || defined  (ESP32)
  for (byte i = 0; i < sizeof config; ++ i)
  { ((byte*)&config)[i] = EEPROM.read((int)RF12_EEPROM_ADDR + i);
    // Serial.print(i);Serial.print(" ");Serial.println(EEPROM.read((int)RF12_EEPROM_ADDR + i));
  }
#elif defined(MCU_STM32F103C8) || defined(MCU_STM32F103CB) || defined(MCU_STM32F103CBT6) || defined(MCU_STM32F103RB) || defined(STM32F103xB)
//  for (uint16 i = 0; i < sizeof config; ++i)  //
    for (byte i = 0; i < sizeof config; ++i)
  {
    ((byte*)&config)[i] = EEPROM.read(i);
  }

#else

#endif
}

static void saveConfig () {
  config.format = MAJOR_VERSION;
  config.crc = calcCrc(&config, sizeof config - 2);
  //    Serial.print("calcCrc2 ");Serial.println(config.crc);
  //    Serial.print("nodeId ");Serial.println(config.nodeId & RF12_HDR_MASK);
  //    Serial.print("group ");Serial.println(config.group);
  //    Serial.print("frequency_offset ");Serial.println(config.frequency_offset);
  // eeprom_write_block(&config, RF12_EEPROM_ADDR, sizeof config);
  // this uses 170 bytes less flash than eeprom_write_block(), no idea why
#if defined (__LGT8FX8P__)
  EEPROM.write((int)RF12_EEPROM_ADDR, ((byte*) &config)[0]);
  //
  for (byte i = 0; i < sizeof config; ++ i)
  { EEPROM.write((int)RF12_EEPROM_ADDR + i, ((byte*) &config)[i]);
    //  Serial.print(i);Serial.print(" ");Serial.println(EEPROM.read((int)RF12_EEPROM_ADDR + i));
  } 
#elif defined(ARDUINO_ARCH_AVR) || defined(ARDUINO_ARCH_MEGAAVR)
  eeprom_write_byte(RF12_EEPROM_ADDR, ((byte*) &config)[0]);
  for (byte i = 0; i < sizeof config; ++ i)
    eeprom_write_byte(RF12_EEPROM_ADDR + i, ((byte*) &config)[i]);
#elif defined (ESP8266) || defined  (ESP8266_GENERIC) || defined  (ESP32)
  EEPROM.write((int)RF12_EEPROM_ADDR, ((byte*) &config)[0]);
  //
  for (byte i = 0; i < sizeof config; ++ i)
  { EEPROM.write((int)RF12_EEPROM_ADDR + i, ((byte*) &config)[i]);
    //  Serial.print(i);Serial.print(" ");Serial.println(EEPROM.read((int)RF12_EEPROM_ADDR + i));
  }
  if (EEPROM.commit()) {
#if defined  (ESP32)
    Serial.print(F("\r\nESP32 EEPROM.commit. "));
#else
    Serial.print(F("\r\nESP8266 EEPROM.commit. "));
#endif
  }
#elif defined(MCU_STM32F103C8) || defined(MCU_STM32F103CB) || defined(MCU_STM32F103CBT6) || defined(MCU_STM32F103RB) || defined(STM32F103xB)
  EEPROM.write( 0, ((byte*) &config)[0]);
  //for (uint16 i = 0; i < sizeof config; ++i )
  for (byte i = 0; i < sizeof config; ++i )
  { EEPROM.write( i, ((byte*) &config)[i]);
    //   Serial.print("config ");Serial.print(i);Serial.print(": ");Serial.println(EEPROM.read(i));
  } 
#else

#endif
  if (rf12_configSilent())
    rf12_configDump();
  else
    showString(INITFAIL);
}

static byte bandToFreq (byte band) {
  return band == 4 ? RF12_433MHZ : band == 8 ? RF12_868MHZ : band == 9 ? RF12_915MHZ : 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// OOK transmit code

#if RF69_COMPAT // not implemented in RF69 compatibility mode
static void fs20cmd(word house, byte addr, byte cmd) {}
static void kakuSend(char addr, byte device, byte on) {}
#else

// Turn transmitter on or off, but also apply asymmetric correction and account
// for 25 us SPI overhead to end up with the proper on-the-air pulse widths.
// With thanks to JGJ Veken for his help in getting these values right.
static void ookPulse(int on, int off) {
  rf12_onOff(1);
  delayMicroseconds(on + 150);
  rf12_onOff(0);
  delayMicroseconds(off - 200);
}

static void fs20sendBits(word data, byte bits) {
  if (bits == 8) {
    ++bits;
    data = (data << 1) | parity_even_bit(data);
  }
  for (word mask = bit(bits - 1); mask != 0; mask >>= 1) {
    int width = data & mask ? 600 : 400;
    ookPulse(width, width);
  }
}

static void fs20cmd(word house, byte addr, byte cmd) {
  byte sum = 6 + (house >> 8) + house + addr + cmd;
  for (byte i = 0; i < 3; ++i) {
    fs20sendBits(1, 13);
    fs20sendBits(house >> 8, 8);
    fs20sendBits(house, 8);
    fs20sendBits(addr, 8);
    fs20sendBits(cmd, 8);
    fs20sendBits(sum, 8);
    fs20sendBits(0, 1);
    delay(10);
  }
}

static void kakuSend(char addr, byte device, byte on) {
  int cmd = 0x600 | ((device - 1) << 4) | ((addr - 1) & 0xF);
  if (on)
    cmd |= 0x800;
  for (byte i = 0; i < 4; ++i) {
    for (byte bit = 0; bit < 12; ++bit) {
      ookPulse(375, 1125);
      int on = bitRead(cmd, bit) ? 1125 : 375;
      ookPulse(on, 1500 - on);
    }
    ookPulse(375, 375);
    delay(11); // approximate
  }
}

#endif

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// DataFlash code

#if DATAFLASH
#include "dataflash.h"
#else // DATAFLASH

#define df_present() 0
#define df_initialize()
#define df_dump()
#define df_replay(x,y)
#define df_erase(x)
#define df_wipe()
#define df_append(x,y)

#endif

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

const char helpText1[] PROGMEM =
  "\n"
  "Available commands:\n"
  "  <nn> i     - set node ID (standard node ids are 1..30)\n"
  "  <n> b      - set MHz band (4 = 433, 8 = 868, 9 = 915)\n"
  "  <nnnn> o   - change frequency offset within the band (default 1600)\n"
  "               96..3903 is the range supported by the RFM12B\n"
  "  <nnn> g    - set network group (RFM12 only allows 212, 0 = any)\n"
  "  <n> c      - set collect mode (advanced, normally 0)\n"
  "  t          - broadcast max-size test packet, request ack\n"
  "  ...,<nn> a - send data packet to node <nn>, request ack\n"
  "  ...,<nn> s - send data packet to node <nn>, no ack\n"
  "  <n> q      - set quiet mode (1 = don't report bad packets)\n"
  "  <n> x      - set reporting format (0: decimal, 1: hex, 2: hex+ascii)\n"
  "  123 z      - total power down, needs a reset to start up again\n"
  "Remote control commands:\n"
  "  <hchi>,<hclo>,<addr>,<cmd> f     - FS20 command (868 MHz)\n"
  "  <addr>,<dev>,<on> k              - KAKU command (433 MHz)\n"
  ;

const char helpText2[] PROGMEM =
  "Flash storage (JeeLink only):\n"
  "    d                                  - dump all log markers\n"
  "    <sh>,<sl>,<t3>,<t2>,<t1>,<t0> r    - replay from specified marker\n"
  "    123,<bhi>,<blo> e                  - erase 4K block\n"
  "    12,34 w                            - wipe entire flash memory\n"
  ;

static void showHelp () {
#if TINY
  showString(PSTR("?\n"));
#else
  showString(helpText1);
  if (df_present())
    showString(helpText2);
  showString(PSTR("Current configuration:\n"));
  rf12_configDump();
#endif
}

static void handleInput (char c) {
  if ('0' <= c && c <= '9') {
    pvalue = 10 * pvalue + c - '0';
    return;
  }

  if (c == ',') {
    if (top < sizeof stack)
      stack[top++] = pvalue; // truncated to 8 bits
    pvalue = 0;
    return;
  }

  if ('a' <= c && c <= 'z') {
    showString(PSTR("> "));
    for (byte i = 0; i < top; ++i) {
      Serial.print((word) stack[i]);
      printOneChar(',');
    }
    Serial.print(pvalue);
    Serial.println(c);
  }

  // keeping this out of the switch reduces code size (smaller branch table)
  if (c == '>') {
    // special case, send to specific band and group, and don't echo cmd
    // input: band,group,node,header,data...
    stack[top++] = pvalue;
    // TODO: frequency offset is taken from global config, is that ok?
    Serial.println(" c == '>' ");
    rf12_initialize(stack[2], bandToFreq(stack[0]), stack[1],
                    config.frequency_offset);
    rf12_sendNow(stack[3], stack + 4, top - 4);
    rf12_sendWait(2);
    rf12_configSilent();
  } else if (c > ' ') {
    switch (c) {

      case 'i': // set node id
        config.nodeId = (config.nodeId & 0xE0) + (pvalue & 0x1F);
        saveConfig();
        break;

      case 'b': // set band: 4 = 433, 8 = 868, 9 = 915
        pvalue = bandToFreq(pvalue);
        if (pvalue) {
          config.nodeId = (pvalue << 6) + (config.nodeId & 0x3F);
          config.frequency_offset = 1600;
          saveConfig();
        }
        break;

      case 'o': { // Increment frequency within band
          // Stay within your country's ISM spectrum management guidelines, i.e.
          // allowable frequencies and their use when selecting operating frequencies.
          if ((pvalue > 95) && (pvalue < 3904)) { // supported by RFM12B
            config.frequency_offset = pvalue;
            saveConfig();
          }
#if !TINY
          // this code adds about 400 bytes to flash memory use
          // display the exact frequency associated with this setting
          byte freq = 0, band = config.nodeId >> 6;
          switch (band) {
            case RF12_433MHZ: freq = 43; break;
            case RF12_868MHZ: freq = 86; break;
            case RF12_915MHZ: freq = 90; break;
          }
          uint32_t f1 = freq * 100000L + band * 25L * config.frequency_offset;
          Serial.print((word) (f1 / 10000));
          printOneChar('.');
          word f2 = f1 % 10000;
          // tedious, but this avoids introducing floating point
          printOneChar('0' + f2 / 1000);
          printOneChar('0' + (f2 / 100) % 10);
          printOneChar('0' + (f2 / 10) % 10);
          printOneChar('0' + f2 % 10);
          Serial.println(" MHz");
#endif
          break;
        }

      case 'g': // set network group
        config.group = pvalue;
        saveConfig();
        break;

      case 'c': // set collect mode (off = 0, on = 1)
        config.collect_mode = pvalue;
        saveConfig();
        break;

      case 't': // broadcast a maximum size test packet, request an ack
        cmd = 'a';
        sendLen = RF12_MAXDATA;
        dest = 0;
        for (byte i = 0; i < RF12_MAXDATA; ++i)
        { stack[i] = i + testCounter;
          //  Serial.print("stack[i]= ");Serial.println(stack[i]);
        }
        showString(PSTR("test "));
        showByte(testCounter); // first byte in test buffer
        ++testCounter;
        // Serial.print(" Case 't' on 'handleInput' cmd= ");Serial.println(cmd);
        break;

      case 'a': // send packet to node ID N, request an ack
      case 's': // send packet to node ID N, no ack
        cmd = c;
        sendLen = top;
        dest = pvalue;
        break;

      case 'f': // send FS20 command: <hchi>,<hclo>,<addr>,<cmd>f
        rf12_initialize(0, RF12_868MHZ, 0);
        activityLed(1);
        fs20cmd(256 * stack[0] + stack[1], stack[2], pvalue);
        activityLed(0);
        rf12_configSilent();
        break;

      case 'k': // send KAKU command: <addr>,<dev>,<on>k
        rf12_initialize(0, RF12_433MHZ, 0);
        activityLed(1);
        kakuSend(stack[0], stack[1], pvalue);
        activityLed(0);
        rf12_configSilent();
        break;

      case 'z': // put the ATmega in ultra-low power mode (reset needed)
        if (pvalue == 123) {
          showString(PSTR(" Zzz...\n"));
          Serial.flush();
          rf12_sleep(RF12_SLEEP);
#if defined(ARDUINO_ARCH_AVR)
          cli();
          Sleepy::powerDown();
#endif
        }
        break;

      case 'q': // turn quiet mode on or off (don't report bad packets)
        config.quiet_mode = pvalue;
        saveConfig();
        break;

      case 'x': // set reporting mode to decimal (0), hex (1), hex+ascii (2)
        config.hex_output = pvalue;
        saveConfig();
        break;

      case 'v': //display the interpreter version and configuration
        displayVersion();
        rf12_configDump();
#if TINY
        Serial.println();
#endif
        break;

      // the following commands all get optimised away when TINY is set

      case 'l': // turn activity LED on or off
        activityLed(pvalue);
        break;

      case 'd': // dump all log markers
        if (df_present())
          df_dump();
        break;

      case 'r': // replay from specified seqnum/time marker
        if (df_present()) {
          word seqnum = (stack[0] << 8) | stack[1];
          long asof = (stack[2] << 8) | stack[3];
          asof = (asof << 16) | ((stack[4] << 8) | pvalue);
          df_replay(seqnum, asof);
        }
        break;

      case 'e': // erase specified 4Kb block
        if (df_present() && stack[0] == 123) {
          word block = (stack[1] << 8) | pvalue;
          df_erase(block);
        }
        break;

      case 'w': // wipe entire flash memory
        if (df_present() && stack[0] == 12 && pvalue == 34) {
          df_wipe();
          showString(PSTR("erased\n"));
        }
        break;

      default:
        showHelp();
    }
  }
  pvalue = top = 0;
}

static void displayASCII (const byte* data, byte count) {
  for (byte i = 0; i < count; ++i) {
    printOneChar(' ');
    char c = (char) data[i];
    printOneChar(c < ' ' || c > '~' ? '.' : c);
  }
  Serial.println();
}

void setup () {
  delay(100); // shortened for now. Handy with JeeNode Micro V1 where ISP
  // interaction can be upset by RF12B startup process.

#if TINY
  PCMSK0 |= (1 << PCINT2); // tell pin change mask to listen to PA2
  GIMSK |= (1 << PCIE0);  // enable PCINT interrupt in general interrupt mask
  // FIXME: _bitDelay has not yet been initialised here !?
  whackDelay(_bitDelay * 2); // if we were low this establishes the end
  pinMode(_receivePin, INPUT);        // PA2
  digitalWrite(_receivePin, HIGH);    // pullup!
  _bitDelay = BITDELAY;
#endif
  Serial.begin(SERIAL_BAUD);
  //  displayVersion(); //Moved further down
#if defined (ESP8266) || defined  (ESP8266_GENERIC) || defined  (ESP32)
  rf12_spiInit (); //EEPROM.begin ...
#elif defined(MCU_STM32F103C8) || defined(MCU_STM32F103CB) || defined(MCU_STM32F103CBT6) || defined(MCU_STM32F103RB) || defined(STM32F103xB)
  rf12_spiInit(); //EEPROM.init ...
#endif

  if (rf12_configSilent()) {
    loadConfig();
  } else {
    memset(&config, 0, sizeof config);
    config.nodeId = 0x81;       // 868 MHz, node 1
    config.group = 0xD4;        // default group 212
    config.frequency_offset = 1600;
    config.quiet_mode = true;   // Default flags, quiet on
    saveConfig();
    rf12_configSilent();
  }
#if !(defined(MCU_STM32F103CB) || defined(STM32F103xB))//other MCU than Maple Minis STM32F103CB
//#if !defined(MCU_STM32F103C8) || !defined(MCU_STM32F103CB) || !defined(MCU_STM32F103CBT6) || !defined(MCU_STM32F103RB) || defined(STM32F103xB)
  displayVersion(); rf12_configDump(); df_initialize();
  #if !TINY
    showHelp();
  #endif
#else 
      // Maple mini DFU/COM port change time takes time and 
      // no serial printing of program headers comes visible without delays
   delay(6000);displayVersion(); // delay needs to be 6 seconds or more
               rf12_configDump();
   delay(10);  showHelp();       // Even this tiny 10 ms delay is necessary
#endif
}

void loop () {
#if TINY
  if (_receive_buffer_index)
    handleInput(inChar());
#else
  if (Serial.available())
    handleInput(Serial.read());
#endif
  if (rf12_recvDone()) {
    byte n = rf12_len;
    //Serial.println(" n = ");                                                                                                                                                                                                  Serial.println(n);
    if (rf12_crc == 0)
      showString(PSTR("OK"));
    else {
      if (config.quiet_mode)
        return;
      showString(PSTR(" ?"));
      if (n > 20) // print at most 20 bytes if crc is wrong
        n = 20;
    }
    if (config.hex_output)
      printOneChar('X');
    if (config.group == 0) {
      showString(PSTR(" G"));
      showByte(rf12_grp);
    }
    printOneChar(' ');
    showByte(rf12_hdr);
    for (byte i = 0; i < n; ++i) {
      if (!config.hex_output)
        printOneChar(' ');
      showByte(rf12_data[i]);
    }
#if RF69_COMPAT
    // display RSSI value after packet data
    showString(PSTR(" ("));
    if (config.hex_output)
      showByte(RF69::rssi);
    else
      Serial.print(-(RF69::rssi >> 1));
    showString(PSTR(") "));
#endif
    Serial.println();

    if (config.hex_output > 1) { // also print a line as ascii
      showString(PSTR("ASC "));
      if (config.group == 0) {
        showString(PSTR(" II "));
      }
      printOneChar(rf12_hdr & RF12_HDR_DST ? '>' : '<');
      printOneChar('@' + (rf12_hdr & RF12_HDR_MASK));
      displayASCII((const byte*) rf12_data, n);
    }

    if (rf12_crc == 0) {
      activityLed(1);

      if (df_present())
        df_append((const char*) rf12_data - 2, rf12_len + 2);

      if (RF12_WANTS_ACK && (config.collect_mode) == 0) {
        showString(PSTR(" -> ack\n"));
        rf12_sendStart(RF12_ACK_REPLY, 0, 0);
      }
      activityLed(0);
    }
  }

  if (cmd && rf12_canSend()) {
    activityLed(1);
    //Serial.print("cmd && rf12_canSend() ");//debug
    showString(PSTR(" -> "));
    Serial.print((word) sendLen);
    showString(PSTR(" b\n"));
    byte header = cmd == 'a' ? RF12_HDR_ACK : 0;
    if (dest)
      header |= RF12_HDR_DST | dest;
    rf12_sendStart(header, stack, sendLen);
    cmd = 0;

    activityLed(0);
  }
}
