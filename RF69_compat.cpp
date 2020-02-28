 #include <JeeLib.h>
 
//#define JEELIB_SPI1 1 //if using SPI1 and RFM69CW, otherwice compiler error of multiple defined hspi or not defined at all 

#if JEELIB_SPI1 
	#if defined (MCU_STM32F103C8) || defined(MCU_STM32F103CB) || defined(MCU_STM32F103CBT6) || defined(MCU_STM32F103RB) || defined(STM32F103xB)
		#include<SPI.h>
	//	SPIClass SPI_3(2);
	#elif defined  (ESP32)
		#include<SPI.h>
	#else
		#include<SPI1.h>
	#endif
#else
	#include<SPI.h>
#endif

#if defined (__LGT8FX8P__)
  #include <util/crc16.h>
  //#include <avr/eeprom.h>
  //#include <avr/pgmspace.h>
  //#include <util/parity.h>
  #include <avr/sleep.h>
  #include <EEPROM.h>
  #define RFM_IRQ 2 // Arduino interrupt pin number used by radio (RFM12b/RFM69CW)
#elif defined(ARDUINO_ARCH_AVR) || defined(ARDUINO_ARCH_MEGAAVR)
	#include <avr/eeprom.h>
	#include <avr/sleep.h>
	#include <util/crc16.h>
	#define RFM_IRQ 2
#elif defined(MCU_STM32F103C8) || defined(MCU_STM32F103CB) || defined(MCU_STM32F103CBT6) || defined(MCU_STM32F103RB) || defined(STM32F103xB)
	#include <EEPROM.h> 
  	#if JEELIB_SPI1  //true if using At328PB and SPI1. False if SPI or SPI0. 
		#define RFM_IRQ PA8 // SPI1 interrupt
	#else
  		#define RFM_IRQ PB0 // SPI interrupt
  	#endif
	extern "C" {uint16_t _crc16_update(uint16_t crc, uint8_t a);}
	#include "crc16.h"
#else
	#if defined (ESP8266) || defined  (ESP8266_GENERIC) || defined  (ESP32) 
  	  #if defined  (ESP32)
  	  	#if JEELIB_SPI1 	
  			#define RFM_IRQ 35
  		#else
  			#define RFM_IRQ 34
  		#endif	
  	  #else	// ESP8266
  	      	#if defined HALLARD
				#define RFM_IRQ 15 // works for my Wemos D1 mini R1
				//https://github.com/hallard/WeMos-RFM69
				//#error Hallards module RF69_awr.h //debug 
    		#else
				#define RFM_IRQ 4 // works for my esp8266
			#endif	
  	  #endif
  	#include <EEPROM.h> 
  	extern "C" {uint16_t _crc16_update(uint16_t crc, uint8_t a);}
	#include "crc16.h"

		// same code as in util/crc16.h
	//	uint16_t _crc16update(uint16_t crc, uint8_t a) //renamed _crc16_update -> _crc16update
	//	{
	//  	int i;
	//  	crc ^= a;
	//  	for (i = 0; i < 8; ++i)  {
	//    	if (crc & 1)
	//      	crc = (crc >> 1) ^ 0xA001;
	//    	else
	//      	crc = (crc >> 1);
	//  	}
	//  	return crc;
	//	}
	//	#define crc_update      _crc16update 
	//	#define crc_update      _crc16_update 
	#endif
#endif


volatile uint16_t rf69_crc;
volatile uint8_t rf69_buf[72];

static byte nodeid; // only used in the easyPoll code

// same as in RF12
#define RETRIES     8               // stop retrying after 8 times
#define RETRY_MS    1000            // resend packet every second until ack'ed

// same as in RF12
static uint8_t ezInterval;          // number of seconds between transmits
static uint8_t ezSendBuf[RF12_MAXDATA]; // data to send
static char ezSendLen;              // number of bytes to send
static uint8_t ezPending;           // remaining number of retries
static long ezNextSend[2];          // when was last retry [0] or data [1] sent

// void rf69_set_cs (uint8_t pin) {
// }

 void rf69_spiInit () { //spiInit(); //error: 'spiInit' was not declared in this scope
 // Serial.println(F("rf12_spiInit () call from main program redirected to rf69_spiInit ()"));
#if JEELIB_SPI1  //true if using At328PB and SPI1. False if SPI or SPI0. 
   	#if defined (MCU_STM32F103C8) || defined(MCU_STM32F103CB) || defined(MCU_STM32F103CBT6) || defined(MCU_STM32F103RB) || defined(STM32F103xB)
   		#if defined(STM32F103xB) 
   		pinMode(PIN_SPI_MISO,INPUT_PULLUP);
   		static uint8_t cs_pin = 31;     // chip select pin
   		#else
   		pinMode(BOARD_SPI2_MISO_PIN,INPUT_PULLUP);
   		static uint8_t cs_pin = SS1;     // chip select pin
   		#endif
   		//SPI_3.begin();
   		//	Serial.print(F("\r\nSPI_3.begin();"));
   	#elif defined  (ESP32) 
    	pinMode(12,INPUT_PULLUP);
    	static uint8_t cs_pin = 15;     // chip select pin
   	#else
		pinMode(MISO1,INPUT_PULLUP);
		SPI1.begin();
		static uint8_t cs_pin = SS1;     // chip select pin
	#endif	
	
#else	//SPI
	pinMode(MISO,INPUT_PULLUP);
	SPI.begin();
	//Serial.println(F("SPI.begin();"));
	static uint8_t cs_pin = SS;     // chip select pin
#endif
	
	
 #if defined (ESP8266) || defined  (ESP8266_GENERIC) || defined  (ESP32)
    EEPROM.begin(512);
    #if defined (HALLARD) && !defined(ESP32)
    	cs_pin = 16;     // chip select pin Hallards module
    //	#error Hallards module RF69_compat.cpp //debug	
    #endif
 #elif defined(MCU_STM32F103C8) || defined(MCU_STM32F103CB) || defined(MCU_STM32F103CBT6) || defined(MCU_STM32F103RB)
     //uint16 Status = EEPROM.init(0x801F000, 0x801F200, 0x200); 
     byte Status = EEPROM.init(0x801F000, 0x801F200, 0x200); 
 #elif defined(STM32F103xB)
 	EEPROM.begin();    
 #endif
 pinMode(cs_pin,OUTPUT);
//}
  
 }



// void rf69_set_cs (uint8_t pin) {
// }

 //void rf69_lowbat () {
 //}

uint8_t rf69_initialize (uint8_t id, uint8_t band, uint8_t group, uint16_t off) {
    uint8_t freq = 0;
    switch (band) {
        case RF12_433MHZ: freq = 43; break;
        case RF12_868MHZ: freq = 86; break;
        case RF12_915MHZ: freq = 90; break;
    } 
    RF69::setFrequency(freq * 10000000L + band * 2500L * off);
    RF69::group = group;
    RF69::node = id & RF12_HDR_MASK;
    delay(20); // needed to make RFM69 work properly on power-up
    pinMode(RFM_IRQ,INPUT); //at least ESP32 need this 
    //Serial.print(F("RF69::node "));Serial.println(RF69::node);
  #if defined (ESP8266) || defined  (ESP8266_GENERIC) || defined  (ESP32)
    if (RF69::node != 0)
      {attachInterrupt(digitalPinToInterrupt(RFM_IRQ), RF69::interrupt_compat, RISING);}//LOW did'nt work ESP8266
    else
       {detachInterrupt(digitalPinToInterrupt(RFM_IRQ));}
    // SPI.usingInterrupt(digitalPinToInterrupt(IRQ)); //ESP8266 does not support interrupt handling in SPI
  #elif defined(MCU_STM32F103C8) || defined(MCU_STM32F103CB) || defined(MCU_STM32F103CBT6) || defined(MCU_STM32F103RB)
    if (RF69::node != 0)
      {attachInterrupt(digitalPinToInterrupt(RFM_IRQ), RF69::interrupt_compat, (ExtIntTriggerMode) RISING);}
    else
      {detachInterrupt(digitalPinToInterrupt(RFM_IRQ));}
  #elif defined(STM32F103xB)
    if (RF69::node != 0)
      {attachInterrupt(digitalPinToInterrupt(RFM_IRQ), RF69::interrupt_compat, RISING);
      #if JEELIB_SPI1 
    	SPI.usingInterrupt(digitalPinToInterrupt(RFM_IRQ)); 
      #else
    	SPI.usingInterrupt(digitalPinToInterrupt(RFM_IRQ)); 
      #endif 
      }       
  #else
    if (RF69::node != 0)
      {attachInterrupt(digitalPinToInterrupt(RFM_IRQ), RF69::interrupt_compat, RISING);
      #if JEELIB_SPI1 
    	SPI1.usingInterrupt(digitalPinToInterrupt(RFM_IRQ)); 
      #else
    	SPI.usingInterrupt(digitalPinToInterrupt(RFM_IRQ)); 
      #endif	
      //Serial.print(F("attach RFM_IRQ_PIN "));Serial.print(RFM_IRQ);Serial.print(F(", RFM_IRQ "));Serial.println(digitalPinToInterrupt(RFM_IRQ));
      }
    else
      {detachInterrupt(digitalPinToInterrupt(RFM_IRQ));
       //  SPI.notUsingInterrupt(digitalPinToInterrupt(RFM_IRQ)); 
      //Serial.print(F("detach RFM_IRQ "));Serial.println(digitalPinToInterrupt(RFM_IRQ));
      }  
 #endif
    RF69::configure_compat();
    return nodeid = id;
}

// same code as rf12_config(Silent), just calling rf69_initialize() instead
//uint8_t rf69_configSilent () {
//    uint16_t crc = ~0;
//    for (uint8_t i = 0; i < RF12_EEPROM_SIZE; ++i) {
//        byte e = eeprom_read_byte(RF12_EEPROM_ADDR + i);
//        crc = _crc16_update(crc, e);
//    }
//    if (crc || eeprom_read_byte(RF12_EEPROM_ADDR + 2) != RF12_EEPROM_VERSION)
//        return 0;
//        
//    uint8_t nodeId = 0, group = 0;   
//    uint16_t frequency = 0;  
//     
//    nodeId = eeprom_read_byte(RF12_EEPROM_ADDR + 0);
//    group  = eeprom_read_byte(RF12_EEPROM_ADDR + 1);
//    frequency = eeprom_read_word((uint16_t*) (RF12_EEPROM_ADDR + 4));
//    
//    rf69_initialize(nodeId, nodeId >> 6, group, frequency);
//    return nodeId & RF12_HDR_MASK;
//}
uint8_t rf69_configSilent () {
    uint16_t crc = ~0;
    for (uint8_t i = 0; i < RF12_EEPROM_SIZE; ++i) {
#if defined (__LGT8FX8P__)
		byte e = EEPROM.read((int)RF12_EEPROM_ADDR + i);
#elif defined(ARDUINO_ARCH_AVR) || defined(ARDUINO_ARCH_MEGAAVR)
        byte e = eeprom_read_byte(RF12_EEPROM_ADDR + i);  
#elif defined (ESP8266) || defined  (ESP8266_GENERIC) || defined  (ESP32)    	
 		byte e = EEPROM.read((int)RF12_EEPROM_ADDR + i);
#elif defined (MCU_STM32F103C8) || defined(MCU_STM32F103CB) || defined(MCU_STM32F103CBT6) || defined(MCU_STM32F103RB) || defined(STM32F103xB)
 		 byte e = EEPROM.read(i); 		
#else
		Serial.println(F("\r\nESP rf12_configSilent no processor"));
#endif        
        //crc = crc_update(crc, e);
        crc = _crc16_update(crc, e);
    }
#if defined (__LGT8FX8P__)
    if (crc || EEPROM.read((int)RF12_EEPROM_ADDR + 2) != RF12_EEPROM_VERSION)
        return 0;    
#elif defined (ESP8266) || defined  (ESP8266_GENERIC) || defined  (ESP32)   
    if (crc || EEPROM.read((int)RF12_EEPROM_ADDR + 2) != RF12_EEPROM_VERSION)
        return 0;
#elif defined (MCU_STM32F103C8) || defined(MCU_STM32F103CB) || defined(MCU_STM32F103CBT6) || defined(MCU_STM32F103RB) || defined(STM32F103xB)
    if (crc || EEPROM.read(2) != RF12_EEPROM_VERSION)
             {
        return 0;}
#else    
    if (crc || eeprom_read_byte(RF12_EEPROM_ADDR + 2) != RF12_EEPROM_VERSION)
        return 0;
#endif
    uint8_t nodeId = 0, group = 0;
    uint16_t frequency = 0;
#if defined (__LGT8FX8P__)
    nodeId = EEPROM.read((int)RF12_EEPROM_ADDR + 0);
    group  = EEPROM.read((int)RF12_EEPROM_ADDR + 1);
    frequency = EEPROM.read((int)RF12_EEPROM_ADDR + 5)<<8; 	//MSB
    frequency |= EEPROM.read((int)RF12_EEPROM_ADDR + 4);	//LSB    
#elif defined (ESP8266) || defined  (ESP8266_GENERIC) || defined  (ESP32)
    nodeId = EEPROM.read((int)RF12_EEPROM_ADDR + 0);
    group  = EEPROM.read((int)RF12_EEPROM_ADDR + 1);
    frequency = EEPROM.read((int)RF12_EEPROM_ADDR + 5)<<8; 	//MSB
    frequency |= EEPROM.read((int)RF12_EEPROM_ADDR + 4);	//LSB
#elif defined (MCU_STM32F103C8) || defined(MCU_STM32F103CB) || defined(MCU_STM32F103CBT6) || defined(MCU_STM32F103RB) || defined(STM32F103xB)
    nodeId 	= EEPROM.read( 0);
    group	= EEPROM.read( 1);
    frequency  = EEPROM.read(5)<<8; //MSB
    frequency |= EEPROM.read(4);				//LSB   	
#else
    nodeId = eeprom_read_byte(RF12_EEPROM_ADDR + 0);
    group  = eeprom_read_byte(RF12_EEPROM_ADDR + 1);
    frequency = eeprom_read_word((uint16_t*) (RF12_EEPROM_ADDR + 4));
#endif    
    rf69_initialize(nodeId, nodeId >> 6, group, frequency);
    return nodeId & RF12_HDR_MASK;
}

/// @deprecated Please switch over to rf12_configSilent() and rf12_configDump().
uint8_t rf69_config (uint8_t show) {
    uint8_t id = rf69_configSilent();
    if (show)
        rf12_configDump();
    return id;
}

uint8_t rf69_recvDone () {
    rf69_crc = RF69::recvDone_compat((uint8_t*) rf69_buf);
    return rf69_crc != ~0;
}

uint8_t rf69_canSend () {
    return RF69::canSend();
}

 void rf69_sendStart (uint8_t hdr) {
 }

void rf69_sendStart (uint8_t hdr, const void* ptr, uint8_t len) {
    RF69::sendStart_compat(hdr, ptr, len);
}

 void rf69_sendStart (uint8_t hdr, const void* ptr, uint8_t len, uint8_t sync) {
 }

void rf69_sendNow (uint8_t hdr, const void* ptr, uint8_t len) {
    while (!rf69_canSend())
        rf69_recvDone();
    rf69_sendStart(hdr, ptr, len);
}

void rf69_sendWait (uint8_t mode) {
    while (RF69::sending()){
        #if defined (ESP8266) || defined  (ESP8266_GENERIC)  || defined  (ESP32)|| defined(MCU_STM32F103C8) || defined(MCU_STM32F103CBT6) || defined(MCU_STM32F103CB) || defined(MCU_STM32F103RB) || defined(STM32F103xB)
 		//do nothing
		#else 
        if (mode) {
            set_sleep_mode(mode == 3 ? SLEEP_MODE_PWR_DOWN :
#ifdef SLEEP_MODE_STANDBY
                           mode == 2 ? SLEEP_MODE_STANDBY :
#endif
                                       SLEEP_MODE_IDLE);
            sleep_mode();
        }
        #endif
	}
}

void rf69_onOff (uint8_t value) {
    // TODO: not yet implemented
}

void rf69_sleep (char n) {
    RF69::sleep(n == RF12_SLEEP);
}

 char rf69_lowbat () {
 }

// same as in RF12
void rf69_easyInit (uint8_t secs) {
    ezInterval = secs;
}

// same as in RF12, but with rf69_* calls i.s.o. rf12_*
char rf69_easyPoll () {
    if (rf69_recvDone() && rf12_crc == 0) {
        byte myAddr = nodeid & RF12_HDR_MASK;
        if (rf12_hdr == (RF12_HDR_CTL | RF12_HDR_DST | myAddr)) {
            ezPending = 0;
            ezNextSend[0] = 0; // flags succesful packet send
            if (rf12_len > 0)
                return 1;
        }
    }
    if (ezPending > 0) {
        byte newData = ezPending == RETRIES;
        long now = millis();
        if (now >= ezNextSend[newData] && rf69_canSend()) {
            ezNextSend[0] = now + RETRY_MS;
            if (newData)
                ezNextSend[1] = now +
                    (ezInterval > 0 ? 1000L * ezInterval
                                    : (nodeid >> 6) == RF12_868MHZ ?
                                            13 * (ezSendLen + 10) : 100);
            rf69_sendStart(RF12_HDR_ACK, ezSendBuf, ezSendLen);
            --ezPending;
        }
    }
    return ezPending ? -1 : 0;
}

// same as in RF12
char rf69_easySend (const void* data, uint8_t size) {
    if (data != 0 && size != 0) {
        if (ezNextSend[0] == 0 && size == ezSendLen &&
                                    memcmp(ezSendBuf, data, size) == 0)
            return 0;
        memcpy(ezSendBuf, data, size);
        ezSendLen = size;
    }
    ezPending = RETRIES;
    return 1;
}

void rf69_encrypt (const uint8_t*) {
    // TODO: not yet implemented
}

uint16_t rf69_control (uint16_t cmd) {
    // the RF69's API is different: use top 8 bits as reg + w/r flag, and
    // bottom 8 bits as the value to store, result is only 8 bits, not 16
    return RF69::control(cmd >> 8, cmd);
}
