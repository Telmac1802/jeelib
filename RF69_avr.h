// prog_uint8_t appears to be deprecated in avr libc, this resolves it for now
#define __PROG_TYPES_COMPAT__
#define ROM_UINT8       const prog_uint8_t
#define ROM_READ_UINT8  pgm_read_byte
#define ROM_DATA        PROGMEM
//#include<SPI.h>

#if defined (__LGT8FX8P__)
  #include <util/crc16.h>
  #include <avr/pgmspace.h>
  #include <avr/interrupt.h>
 // #include <util/parity.h>
 // #include <avr/sleep.h>
  #include <EEPROM.h>
#elif defined(ARDUINO_ARCH_AVR) || defined(ARDUINO_ARCH_MEGAAVR) 
	#include <avr/interrupt.h>
	#include <util/crc16.h>
	#include <avr/pgmspace.h>
#elif defined (ESP8266) || defined  (ESP8266_GENERIC) || defined  (ESP32) 
  	//#define RFM_IRQ 4 // works for my esp8266
  	#include <EEPROM.h> 
	#include <pgmspace.h>
#elif defined (MCU_STM32F103C8) || defined(MCU_STM32F103CB) || defined(MCU_STM32F103CBT6) || defined(MCU_STM32F103RB) || defined(STM32F103xB)
  	//#define RFM_IRQ 4 // dom't know if works or not
	#include <EEPROM.h> 
	//#include <pgmspace.h>
	//#include "crc16.h"
#else
	//for non AVR non ESP processors
#endif

//#if JEELIB_SPI1 
//	#include<SPI1.h>
//	static uint8_t cs_pin = SS1;     // chip select pin
//#else
	#include<SPI.h>
    #if defined HALLARD
		static uint8_t cs_pin = 16;     // chip select pin Hallards module
		//https://github.com/hallard/WeMos-RFM69
		//#error Hallards module RF69_awr.h //debug 
    #else
		static uint8_t cs_pin = SS;     // chip select pin
	#endif		
//#endif

//RFM12b reliable SPI read speed is 2 MHz, write speed is higher 4-8 Mhz
//RFM69 maybe tolerates more (read) SPI speed - using 8 MHz settings 
SPISettings settings69Read(2000000L, MSBFIRST, SPI_MODE0); //SPI setings for reading
SPISettings settings69Write(F_CPU, MSBFIRST, SPI_MODE0); //SPI setings for writing (not used)

//// prog_uint8_t appears to be deprecated in avr libc, this resolves it for now
//#define __PROG_TYPES_COMPAT__
//#include <avr/pgmspace.h>
//
//#define ROM_UINT8       const prog_uint8_t
//#define ROM_READ_UINT8  pgm_read_byte
//#define ROM_DATA        PROGMEM

//#define  sei() interrupts()
#define IRQ_ENABLE     interrupts() // sei()

//#ifndef JEELIB_SPI1
//	#define JEELIB_SPI1 1  // For using SPI1 on Atmega238PB, needs MiniCore. No effect on other boards. 
//#endif

//#if defined(__AVR_ATmega2560__) || defined(__AVR_ATmega1280__)
//
//#define RFM_IRQ     2		//digital 2, Arduino int 0, INT4, PE4
//#define SS_DDR      DDRB
//#define SS_PORT     PORTB
//#define SS_BIT      0
//
//#define SPI_SS      0 //53    // PB0, pin 19
//#define SPI_MOSI    2 //51    // PB2, pin 21
//#define SPI_MISO    3 //50    // PB3, pin 22
//#define SPI_SCK     1 //52    // PB1, pin 20
//
//static void spiConfigPins () {
//    SS_PORT |= _BV(SS_BIT);
//    SS_DDR |= _BV(SS_BIT);
//    PORTB |= _BV(SPI_SS);
//    DDRB |= _BV(SPI_SS) | _BV(SPI_MOSI) | _BV(SPI_SCK);
//}
//
//#elif defined(__AVR_ATmega644P__)
//
//#define RFM_IRQ     10
//#define SS_DDR      DDRB
//#define SS_PORT     PORTB
//#define SS_BIT      4
//
//#define SPI_SS      4
//#define SPI_MOSI    5
//#define SPI_MISO    6
//#define SPI_SCK     7
//
//static void spiConfigPins () {
//    SS_PORT |= _BV(SS_BIT);
//    SS_DDR |= _BV(SS_BIT);
//    PORTB |= _BV(SPI_SS);
//    DDRB |= _BV(SPI_SS) | _BV(SPI_MOSI) | _BV(SPI_SCK);
//}
//
//#elif defined(__AVR_ATtiny84__) || defined(__AVR_ATtiny44__)
//
//#define RFM_IRQ     2
//#define SS_DDR      DDRB
//#define SS_PORT     PORTB
//#define SS_BIT      1
//
//#define SPI_SS      1     // PB1, pin 3
//#define SPI_MISO    6     // PA6, pin 7
//#define SPI_MOSI    5     // PA5, pin 8
//#define SPI_SCK     4     // PA4, pin 9
//
//static void spiConfigPins () {
//    SS_PORT |= _BV(SS_BIT);
//    SS_DDR |= _BV(SS_BIT);
//    PORTB |= _BV(SPI_SS);
//    DDRB |= _BV(SPI_SS);
//    PORTA |= _BV(SPI_SS);
//    DDRA |= _BV(SPI_MOSI) | _BV(SPI_SCK);
//}
//
//#elif defined(__AVR_ATmega32U4__) //Arduino Leonardo
//
//#define RFM_IRQ     0	    // PD0, INT0, Digital3
//#define SS_DDR      DDRB
//#define SS_PORT     PORTB
//#define SS_BIT      6	    // Dig10, PB6
//
//#define SPI_SS      0 //Pin0 10    // PB6, pin 30, Digital10
//#define SPI_MISO    3     // PB3, pin 11, Digital14
//#define SPI_MOSI    2     // PB2, pin 10, Digital16
//#define SPI_SCK     1     // PB1, pin 9, Digital15
//
//static void spiConfigPins () {
//    SS_PORT |= _BV(SS_BIT);
//    SS_DDR |= _BV(SS_BIT);
//    PORTB |= _BV(SPI_SS);
//    DDRB |= _BV(SPI_SS) | _BV(SPI_MOSI) | _BV(SPI_SCK);
//}
//
//#elif defined(__AVR_ATmega328PB__) //Atmega328PB 
//
//// #define RFM_IRQ     2 //Digital pin 2, Arduino int 0, INT0, PD2, 
//
//// You can use either SPI0 or SPI1, not both in the same time
//// Using both in the same time needs more to hack
//#if JEELIB_SPI1
//
//// SPI1 pin settings copied from RF12.cpp and changed to bit numbers
//// look: https://github.com/jeelabs/jeelib/pull/73
//// SPI1 pins are on PORTE and PORTC. Originally PORTB only 
//#define SS_DDR      DDRE   //Port E Data Direction Register
//#define SS_PORT     PORTE
//#define SS_BIT      2     
//
//#define SPI_SS      2 //25    // PE2, pin 19 SS1   32 TQFP package
//#define SPI_MOSI    3 //26    // PE3, pin 22 MOSI1 32 TQFP package
//#define SPI_MISO    0 //14    // PC0, pin 23 MISO1 32 TQFP package
//#define SPI_SCK     1 //15    // PC1, pin 24 SCK1  32 TQFP package

////#ifndef SPCR
//// #define SPCR SPCR1
//// #define SPSR SPSR1
//// #define SPDR SPDR1
////#endif
//
//static void spiConfigPins () {
//	SS_PORT |= _BV(SS_BIT);
//    SS_DDR |= _BV(SS_BIT);
//    PORTE |= _BV(SPI_SS);
//    DDRE |= (_BV(SPI_SS) | _BV(SPI_MOSI)) ;
//    DDRC |= _BV(SPI_SCK); 
//}
//// SPI0 pin settings
//#else
//	#define SS_DDR      DDRB
//	#define SS_PORT     PORTB
//	#define SS_BIT      2     // for PORTB: 2 = d.10, 1 = d.9, 0 = d.8
//
//	#define SPI_SS      2     // PB0 D10
//	#define SPI_MOSI    3     // PB1 D11
//	#define SPI_MISO    4     // PB2 D12
//	#define SPI_SCK     5     // PB3 D13
//
//	// Atmega328PB compability to Atmega328P
//	#ifndef SPCR
//	 #define SPCR SPCR0
//	 #define SPSR SPSR0
//	 #define SPDR SPDR0
//	#endif
//
//
//	static void spiConfigPins () {
//    	SS_PORT |= _BV(SS_BIT);
//    	SS_DDR |= _BV(SS_BIT);
//    	PORTB |= _BV(SPI_SS);
//    	DDRB |= _BV(SPI_SS) | _BV(SPI_MOSI) | _BV(SPI_SCK);
//	}
//
//#endif
//
//
//#else // ATmega168, ATmega328, etc.
//
//// #define RFM_IRQ     2
//#define SS_DDR      DDRB
//#define SS_PORT     PORTB
//#define SS_BIT      2     // for PORTB: 2 = d.10, 1 = d.9, 0 = d.8
//
//#define SPI_SS      2     // PB0
//#define SPI_MOSI    3     // PB1
//#define SPI_MISO    4     // PB2
//#define SPI_SCK     5     // PB3
//
//static void spiConfigPins () {
//    SS_PORT |= _BV(SS_BIT);
//    SS_DDR |= _BV(SS_BIT);
//    PORTB |= _BV(SPI_SS);
//    DDRB |= _BV(SPI_SS) | _BV(SPI_MOSI) | _BV(SPI_SCK);
//}
//
//#endif
//
//#ifndef EIMSK
//#define EIMSK GIMSK // ATtiny
//#endif

//    struct PreventInterrupt {
//     PreventInterrupt () { EIMSK &= ~ _BV(INT0); }
//    ~PreventInterrupt () { EIMSK |= _BV(INT0); }
//    };

//	#if defined(__AVR_ATmega2560__) || defined(__AVR_ATmega1280__)
//	struct PreventInterrupt {
//    PreventInterrupt () { EIMSK &= ~ _BV(INT4); }
//   ~PreventInterrupt () { EIMSK |= _BV(INT4);   }
//	};
//    #else
//    struct PreventInterrupt {
//     PreventInterrupt () { EIMSK &= ~ _BV(INT0); }
//    ~PreventInterrupt () { EIMSK |= _BV(INT0);   }
//    };
//    #endif


static void spiInit (void) {
	digitalWrite(cs_pin,HIGH);
    pinMode(cs_pin,OUTPUT);
    
//#if JEELIB_SPI1  //true if using At328PB and SPI1. False if SPI or SPI0. 
//	//#define SPI SPI1
//	SPI1.begin();
//	//static uint8_t cs_pin = SS1;     // chip select pin
//#else	
	SPI.begin();
	//static uint8_t cs_pin = SS;     // chip select pin
//#endif
  
 //   Serial.println("\r\n SPI.begin(); ");
 #if defined (ESP8266) || defined  (ESP8266_GENERIC) || defined  (ESP32)
    EEPROM.begin(512);
 #elif defined(MCU_STM32F103C8) || defined(MCU_STM32F103CB)  || defined(MCU_STM32F103CBT6) || defined(MCU_STM32F103RB) 
     byte Status = EEPROM.init(0x801F000, 0x801F200, 0x200); 
 #elif defined(STM32F103xB)    
 	EEPROM.begin();
 #endif
}
//    spiConfigPins();
//
//#ifdef SPCR
//	#if defined(__AVR_ATmega328PB__) & (JEELIB_SPI1) //Atmega328PB + SPI1
//	    	SPCR1 = _BV(SPE) | _BV(MSTR);
//	    	SPSR1 |= _BV(SPI2X); 
//	#else
//		SPCR = _BV(SPE) | _BV(MSTR);//| _BV(SPR0); //fck/16
//	    SPSR |= _BV(SPI2X); //
//	#endif
//#else
//    USICR = _BV(USIWM0); // ATtiny
//#endif
//     //pinMode(RFM_IRQ, INPUT);
//    // digitalWrite(RFM_IRQ, 1); // pull-up
//}

//static uint8_t spiTransferByte (uint8_t out) {
//#ifdef SPDR
//	#if defined(__AVR_ATmega328PB__) & (JEELIB_SPI1) //Atmega328PB + SPI1
//			SPDR1 = out;
//    		while (!(SPSR1 & _BV(SPIF)))
//        	;
//    		return SPDR1;
//	#else
//    	SPDR = out;
//    	while (!(SPSR & _BV(SPIF)))
//        	;
//    	return SPDR;
//	#endif
//#else
//    USIDR = out; // ATtiny
//    uint8_t v1 = _BV(USIWM0) | _BV(USITC);
//    uint8_t v2 = _BV(USIWM0) | _BV(USITC) | _BV(USICLK);
//    for (uint8_t i = 0; i < 8; ++i) {
//        USICR = v1;
//        USICR = v2;
//    }
//    return USIDR;
//#endif
//}
//
//static uint8_t spiTransfer (uint8_t cmd, uint8_t val) {
//    SS_PORT &= ~ _BV(SS_BIT);
//    spiTransferByte(cmd);
//    uint8_t in = spiTransferByte(val);
//    SS_PORT |= _BV(SS_BIT);
//    return in;
//}

// copied rf12_xfer(unsigned int cmd) from rf12.cpp
// must change unsigned int cmd -> uint8_t cmd, uint8_t val
//unsigned int rf12_xfer(unsigned int cmd) {
//  unsigned int result;   // result to return
//  SPI.beginTransaction(settingsWrite);
//  // take the chip select low to select the device:
//  digitalWrite(cs_pin, LOW);
//result=SPI.transfer16(cmd);  //Send value to record into register
//  // take the chip select high to de-select:
//  digitalWrite(cs_pin, HIGH);
//  SPI.endTransaction();
//  return result;
//}

// 241119
// Prosessorin valinnalla ei merkitystä

//#if defined (ESP8266) || defined  (ESP8266_GENERIC) || defined  (ESP32) 
// #if defined  (ESP32)	
//  	static uint8_t IRAM_ATTR spiTransfer (uint8_t cmd, uint8_t val)
//  #else	
//  	static uint8_t  ICACHE_RAM_ATTR spiTransfer (uint8_t cmd, uint8_t val) 
//  #endif
//#else
//	static uint8_t spiTransfer (uint8_t cmd, uint8_t val)
//#endif 
//{
//  unsigned int result;   // result to return
//  
////  Serial.print("JEELIB_SPI1 ");Serial.println(JEELIB_SPI1);
//  
//#if JEELIB_SPI1  //true if using At328PB and SPI1. False if SPI or SPI0. 
//  //      Serial.println("SPI1.beginTransaction(settings69Read);");
//  SPI1.beginTransaction(settings69Read);
//  // take the chip select low to select the device:
//  digitalWrite(cs_pin, LOW);
//  result=SPI1.transfer(cmd);  
//  result=SPI1.transfer(val);  
////Send value to record into register
//  // take the chip select high to de-select:
//  digitalWrite(cs_pin, HIGH);
//  SPI1.endTransaction();
//#else
////Serial.println("SPI.beginTransaction(settings69Read);");
//  SPI.beginTransaction(settings69Read);
//  // take the chip select low to select the device:
//  digitalWrite(cs_pin, LOW);
//  result=SPI.transfer(cmd);  
//  result=SPI.transfer(val);  
////Send value to record into register
//  // take the chip select high to de-select:
//  digitalWrite(cs_pin, HIGH);
//  SPI.endTransaction();
// #endif 
//  return result;
//}

#if defined (ESP8266) || defined  (ESP8266_GENERIC) || defined  (ESP32) 
 #if defined  (ESP32)	
  	static uint8_t IRAM_ATTR spiTransfer (uint8_t cmd, uint8_t val)
  #else	
  	static uint8_t  ICACHE_RAM_ATTR spiTransfer (uint8_t cmd, uint8_t val) 
  #endif
#else
	static uint8_t spiTransfer (uint8_t cmd, uint8_t val)
#endif 
{
  unsigned int result;   // result to return
  
//  Serial.print("JEELIB_SPI1 ");Serial.println(JEELIB_SPI1);
  
//#if JEELIB_SPI1  //true if using At328PB and SPI1. False if SPI or SPI0. 
  ////      Serial.println("SPI1.beginTransaction(settings69Read);");
  //SPI1.beginTransaction(settings69Read);
  //// take the chip select low to select the device:
  //digitalWrite(cs_pin, LOW);
  //result=SPI1.transfer(cmd);  
  //result=SPI1.transfer(val);  
////Send value to record into register
  //// take the chip select high to de-select:
  //digitalWrite(cs_pin, HIGH);
  //SPI1.endTransaction();
//#else
//Serial.println("SPI kun pitäisi olla SPI1 ");
  SPI.beginTransaction(settings69Read);
  // take the chip select low to select the device:
  digitalWrite(cs_pin, LOW);
  result=SPI.transfer(cmd);  
  result=SPI.transfer(val);  
//Send value to record into register
  // take the chip select high to de-select:
  digitalWrite(cs_pin, HIGH);
  SPI.endTransaction();

 //#endif 
  return result;
}
