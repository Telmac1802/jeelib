// prog_uint8_t appears to be deprecated in avr libc, this resolves it for now
#define __PROG_TYPES_COMPAT__
#define ROM_UINT8       const prog_uint8_t
#define ROM_READ_UINT8  pgm_read_byte
#define ROM_DATA        PROGMEM
//#include<SPI.h>

#if defined(ARDUINO_ARCH_AVR) || defined(ARDUINO_ARCH_MEGAAVR)
	#include <avr/interrupt.h>
	#include <util/crc16.h>
	#include <avr/pgmspace.h>
	#include<SPI1.h>
	static uint8_t cs_pin = SS1;     // chip select pin	
#elif defined (ESP8266) || defined  (ESP8266_GENERIC) || defined  (ESP32) 
  	//#define RFM_IRQ 4 // works for my esp8266
  	#if defined  (ESP32)
  		#include<SPI.h>
  	//	hspi = new SPIClass(HSPI);
  	extern SPIClass * hspi = NULL;
		static uint8_t cs_pin = 15;     // chip select pin	
	#else
		#include<SPI1.h>
		static uint8_t cs_pin = SS1;     // chip select pin	
	#endif
  	#include <EEPROM.h> 
	#include <pgmspace.h>
#elif defined (MCU_STM32F103C8) || defined(MCU_STM32F103CB) || defined(MCU_STM32F103CBT6) || defined(MCU_STM32F103RB) 
  	//#define RFM_IRQ 4 // dom't know if works or not
	#include <EEPROM.h> 
	#include<SPI.h>
	SPIClass SPI_2(2);
	static uint8_t cs_pin = SS1;     // chip select pin	
	//#include <pgmspace.h>
	//#include "crc16.h"
#elif defined(STM32F103xB) //Maple Mini
  	#include <EEPROM.h> 
	#include<SPI.h>
	SPIClass SPI_M2(28, 29, 30 ); //mosi, miso, sclk  ;
	static uint8_t cs_pin = 31;     // 31 = PB12 = chip select pin	
#else
	//for non AVR non ESP processors
#endif


//RFM12b reliable SPI read speed is 2 MHz, write speed is higher 4-8 Mhz
//RFM69 maybe tolerates more (read) SPI1 speed - using 8 MHz settings 


#if defined  (ESP32) || defined (MCU_STM32F103C8) || defined(MCU_STM32F103CB) || defined(MCU_STM32F103CBT6) || defined(MCU_STM32F103RB) || defined(STM32F103xB)
SPISettings settings69Read(2000000L, MSBFIRST, SPI_MODE0); //SPI1 setings for reading
SPISettings settings69Write(F_CPU, MSBFIRST, SPI_MODE0); //SPI1 setings for writing (not used)
#else
SPI1Settings settings69Read(2000000L, MSBFIRST, SPI_MODE0); //SPI1 setings for reading
SPI1Settings settings69Write(F_CPU, MSBFIRST, SPI_MODE0); //SPI1 setings for writing (not used)
#endif

//#define  sei() interrupts()
#define IRQ_ENABLE     interrupts() // sei()


static void spiInit (void) {
	digitalWrite(cs_pin,HIGH);
    pinMode(cs_pin,OUTPUT);
    

#if defined(MCU_STM32F103C8) || defined(MCU_STM32F103CB) || defined(MCU_STM32F103CBT6) || defined(MCU_STM32F103RB)
	SPI_2.begin();
#elif defined(STM32F103xB)
	SPI_M2.begin(); //mosi, miso, sclk
#elif defined  (ESP32)
	hspi = new SPIClass(HSPI); // first and last definition here
	hspi->begin();
#else	
	SPI1.begin();
#endif	

#if defined (ESP8266) || defined  (ESP8266_GENERIC) || defined  (ESP32)
    EEPROM.begin(512);
#elif defined(MCU_STM32F103C8) || defined(MCU_STM32F103CB) || defined(MCU_STM32F103CBT6) || defined(MCU_STM32F103RB)
    uint16 Status = EEPROM.init(0x801F000, 0x801F200, 0x200); 
#elif defined(STM32F103xB)
 	EEPROM.begin();
#endif
}


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
  
#if defined(MCU_STM32F103C8) || defined(MCU_STM32F103CB) || defined(MCU_STM32F103CBT6) || defined(MCU_STM32F103RB) 
  SPI_2.beginTransaction(settings69Read);
#elif defined(STM32F103xB)
  SPI_M2.beginTransaction(settings69Read);
#elif defined  (ESP32)
  hspi->beginTransaction(settings69Read);
#else
  SPI1.beginTransaction(settings69Read);
#endif
  // take the chip select low to select the device:
  digitalWrite(cs_pin, LOW);
#if defined(MCU_STM32F103C8) || defined(MCU_STM32F103CB) || defined(MCU_STM32F103CBT6) || defined(MCU_STM32F103RB) 
  result=SPI_2.transfer(cmd);  
  result=SPI_2.transfer(val);  
#elif defined(STM32F103xB)
  result=SPI_M2.transfer(cmd);  
  result=SPI_M2.transfer(val);  
#elif defined  (ESP32)
  result=hspi->transfer(cmd);  
  result=hspi->transfer(val);  
#else
  result=SPI1.transfer(cmd);  
  result=SPI1.transfer(val);  
#endif
//Send value to record into register
  // take the chip select high to de-select:
  digitalWrite(cs_pin, HIGH);
#if defined(MCU_STM32F103C8) || defined(MCU_STM32F103CB) || defined(MCU_STM32F103CBT6) || defined(MCU_STM32F103RB) 
  SPI_2.endTransaction();
#elif defined(STM32F103xB)
  SPI_M2.endTransaction();
#elif defined  (ESP32)
  hspi->endTransaction();
#else
  SPI1.endTransaction();
#endif  

  return result;
}
