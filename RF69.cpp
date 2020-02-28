#include <stdint.h>
#include <RF69.h>

//#define JEELIB_SPI1 1 //if using SPI1 and RFM69CW, otherwice compiler error of multiple defined hspi or not defined at all
#define HALLARD 1 	// if using Hallards RFM12B/RFM69CW module. Diffrent IRQ=16  and SS = 16 
					//https://github.com/hallard/WeMos-RFM69
					
#if JEELIB_SPI1  //true if using  SPI1. False if SPI or SPI0. 
	#if defined (MCU_STM32F103C8) || defined(MCU_STM32F103CB) || defined(MCU_STM32F103CBT6) || defined(MCU_STM32F103RB)
		#include<SPI.h>
	#elif defined(STM32F103xB)
		#include<SPI.h>
	#elif defined  (ESP32)
		#include<SPI.h>
	#else
		#include<SPI1.h>
	#endif
	#include <RF69_avr1.h>
#else	//SPI
	#include <RF69_avr.h>
	#include<SPI.h>
//	#error " <RF69_avr.h> liitetty"
//	static uint8_t cs_pin = SS;     // chip select pin
#endif

#if defined (__LGT8FX8P__)
  #define RFM_IRQ 2 // Arduino interrupt pin number used by radio (RFM12b/RFM69CW)
#elif defined(ARDUINO_ARCH_AVR) || defined(ARDUINO_ARCH_MEGAAVR)
	#define RFM_IRQ 2	
#elif defined (ESP8266) || defined  (ESP8266_GENERIC) || defined  (ESP32) 
 #if defined  (ESP32)
	#if JEELIB_SPI1 	
  		#define RFM_IRQ 35
  	#else
		#define RFM_IRQ 34	
  	#endif
  	void IRAM_ATTR RF69::interrupt_compat (); // 
 #else	//ESP8266
	#if defined HALLARD
		#define RFM_IRQ 15 // Hallards module IRQ 
  		//#error Hallards module RF69.cpp //debug
    #else
		#define RFM_IRQ 4 // works for my esp8266
	#endif		
	void ICACHE_RAM_ATTR RF69::interrupt_compat (); // 
 #endif
 #include <EEPROM.h> 
 extern "C" {uint16_t _crc16_update(uint16_t crc, uint8_t a);}
  //#include "crc16.h"
 
// same code as in util/crc16.h
	//compiler complaint: must change the name of the program a bit from uint16_t _crc16_update(uint16_t crc, uint8_t a)
//	uint16_t __crc16_update(uint16_t crc, uint8_t a)
//	{
//	  int i;
//	  crc ^= a;
//	  for (i = 0; i < 8; ++i)  {
//	    if (crc & 1)
//	      crc = (crc >> 1) ^ 0xA001;
//	    else
//	      crc = (crc >> 1);
//	  }
//	  return crc;
//	}
//	#define _crc16_update      __crc16_update 
#elif defined (MCU_STM32F103C8) || defined(MCU_STM32F103CB) || defined(MCU_STM32F103CBT6) || defined(MCU_STM32F103RB) || defined(STM32F103xB)
  	#if JEELIB_SPI1  //true if using MCU_STM32F103X and SPI1. False if SPI or SPI0. 
		#define RFM_IRQ PA8 // SPI1 interrupt
	#else
  		#define RFM_IRQ PB0 // SPI interrupt
  	#endif
	#include <EEPROM.h> 
	extern "C" {uint16_t _crc16_update(uint16_t crc, uint8_t a);}
	//#include "crc16.h"
#else
// other than AVR or ESP or STM32F103C8 controllers
#endif



#define REG_FIFO            0x00
#define REG_OPMODE          0x01
#define REG_FRFMSB          0x07
#define REG_PALEVEL			0x11
#define REG_AFCFEI          0x1E
#define REG_RSSIVALUE       0x24
#define REG_DIOMAPPING1     0x25
#define REG_IRQFLAGS1       0x27
#define REG_IRQFLAGS2       0x28
#define REG_SYNCCONFIG      0x2E
#define REG_SYNCVALUE1      0x2F
#define REG_SYNCVALUE2      0x30
#define REG_SYNCVALUE3      0x31
#define REG_NODEADRS        0x39
#define REG_PACKETCONFIG2   0x3D
#define REG_AESKEY1         0x3E

#define MODE_SLEEP          0x00
#define MODE_STANDBY        0x04
#define MODE_RECEIVER       0x10
#define MODE_TRANSMITTER    0x0C

#define IRQ1_MODEREADY      0x80
#define IRQ1_RXREADY        0x40

#define IRQ2_FIFOFULL       0x80
#define IRQ2_FIFONOTEMPTY   0x40
#define IRQ2_FIFOOVERRUN    0x10
#define IRQ2_PACKETSENT     0x08
#define IRQ2_PAYLOADREADY   0x04

#define DMAP1_PACKETSENT    0x00
#define DMAP1_PAYLOADREADY  0x40
#define DMAP1_SYNCADDRESS   0x80

#define AFC_CLEAR           0x02

#define RF_MAX   72

// transceiver states, these determine what to do with each interrupt
enum { TXCRC1, TXCRC2, TXTAIL, TXDONE, TXIDLE, TXRECV };

namespace RF69 {
    uint32_t frf;
    uint8_t  group;
    uint8_t  node;
    uint16_t crc;
    uint8_t  rssi;
}

static volatile uint8_t rxfill;     // number of data bytes in rf12_buf
static volatile int8_t rxstate;     // current transceiver state

static ROM_UINT8 configRegs_compat [] ROM_DATA = {
  0x01, 0x04, // OpMode = standby
  0x02, 0x00, // DataModul = packet mode, fsk
  0x03, 0x02, // BitRateMsb, data rate = 49,261 khz
  0x04, 0x8A, // BitRateLsb, divider = 32 MHz / 650
  0x05, 0x05, // FdevMsb = 90 KHz
  0x06, 0xC3, // FdevLsb = 90 KHz
  // 0x07, 0xD9, // FrfMsb, freq = 868.000 MHz
  // 0x08, 0x00, // FrfMib, divider = 14221312
  // 0x09, 0x00, // FrfLsb, step = 61.03515625
  0x0B, 0x20, // AfcCtrl, afclowbetaon
  0x11, 0x9F,
  0x19, 0x42, // RxBw ...
  0x1E, 0x2C, // FeiStart, AfcAutoclearOn, AfcAutoOn
  0x25, 0x80, // DioMapping1 = SyncAddress (Rx)
  // 0x29, 0xDC, // RssiThresh ...
  0x2E, 0x90, // SyncConfig = sync on, sync size = 3
  //0x2E, 0x8C, // SyncConfig = sync on, sync size = 2
  0x2F, 0xAA, // SyncValue1 = 0xAA
  0x30, 0x2D, // SyncValue2 = 0x2D
  // 0x31, 0x05, // SyncValue3 = 0x05
  0x37, 0x00, // PacketConfig1 = fixed, no crc, filt off
 // 0x37, 0x04, // PacketConfig1 = fixed, no crc, Address field must match NodeAddress or BroadcastAddress
  0x38, 0x00, // PayloadLength = 0, unlimited
  0x3C, 0x8F, // FifoTresh, not empty, level 15
  0x3D, 0x10, // PacketConfig2, interpkt = 1, autorxrestart off
  0x6F, 0x20, // TestDagc ...
  0
};

uint8_t RF69::control(uint8_t cmd, uint8_t val) {
	// PreventInterrupt irq0; 
      return spiTransfer(cmd, val);
}

static void writeReg (uint8_t addr, uint8_t value) {
    //Serial.println("writeReg");
    RF69::control(addr | 0x80, value);
}

static uint8_t readReg (uint8_t addr) {
    return RF69::control(addr, 0);
}

static void flushFifo () {
    while (readReg(REG_IRQFLAGS2) & (IRQ2_FIFONOTEMPTY | IRQ2_FIFOOVERRUN))
        readReg(REG_FIFO);
}

static void setMode (uint8_t mode) {
    writeReg(REG_OPMODE, (readReg(REG_OPMODE) & 0xE3) | mode);
    // while ((readReg(REG_IRQFLAGS1) & IRQ1_MODEREADY) == 0)
    //     ;
}

// from: https://github.com/LowPowerLab/RFM69/blob/master/RFM69.cpp
// set *transmit/TX* output power: 0=min, 31=max
// this results in a "weaker" transmitted signal, and directly results in a lower RSSI at the receiver
// the power configurations are explained in the SX1231H datasheet (Table 10 on p21; RegPaLevel p66): http://www.semtech.com/images/datasheet/sx1231h.pdf
// valid powerLevel parameter values are 0-31 and result in a directly proportional effect on the output/transmission power
// this function implements 2 modes as follows:
//       - for RFM69W the range is from 0-31 [-18dBm to 13dBm] (PA0 only on RFIO pin)
//       - for RFM69HW the range is from 0-31 [5dBm to 20dBm]  (PA1 & PA2 on PA_BOOST pin & high Power PA settings - see section 3.3.7 in datasheet, p22)
//void RFM69::setPowerLevel(uint8_t powerLevel)
void RF69::setPowerLevel(uint8_t powerLevel) // 3.1.2020 programmed for RFM69CW only
{
 uint8_t _powerLevel = (powerLevel > 31 ? 31 : powerLevel);
  //if (_isRFM69HW) _powerLevel /= 2; 
  writeReg(REG_PALEVEL, (readReg(REG_PALEVEL) & 0xE0) | _powerLevel);
}


static void initRadio (ROM_UINT8* init) {
	
    spiInit();
    
    do
        {writeReg(REG_SYNCVALUE1, 0xAA);}
    while (readReg(REG_SYNCVALUE1) != 0xAA);
    
    do
        {writeReg(REG_SYNCVALUE1, 0x55);}
    while (readReg(REG_SYNCVALUE1) != 0x55);
    
    for (;;) {
        uint8_t cmd = ROM_READ_UINT8(init);
        if (cmd == 0) break;
        writeReg(cmd, ROM_READ_UINT8(init+1));
        init += 2;
    }
}

void RF69::setFrequency (uint32_t freq) {
    // Frequency steps are in units of (32,000,000 >> 19) = 61.03515625 Hz
    // use multiples of 64 to avoid multi-precision arithmetic, i.e. 3906.25 Hz
    // due to this, the lower 6 bits of the calculated factor will always be 0
    // this is still 4 ppm, i.e. well below the radio's 32 MHz crystal accuracy
    // 868.0 MHz = 0xD90000, 868.3 MHz = 0xD91300, 915.0 MHz = 0xE4C000
    frf = ((freq << 2) / (32000000L >> 11)) << 6;
}

bool RF69::canSend () {
	// Serial.println("\r\nRF69::canSend ");Serial.flush();//debug
    if (rxstate == TXRECV && rxfill == 0) {
        rxstate = TXIDLE;
        setMode(MODE_STANDBY);
        return true;
    }
    return false;
}

bool RF69::sending () {
    return rxstate < TXIDLE;
}

void RF69::sleep (bool off) {
    setMode(off ? MODE_SLEEP : MODE_STANDBY);
    rxstate = TXIDLE;
}

// References to the RF12 driver above this line will generate compiler errors!
#include <RF69_compat.h>
#include <RF12.h>

void RF69::configure_compat () {
   // Serial.println("initRadio(configRegs_compat);");
    initRadio(configRegs_compat);
    //Serial.println("initRadio(configRegs_compat);");
    // FIXME doesn't seem to work, nothing comes in but noise for group 0
    // writeReg(REG_SYNCCONFIG, group ? 0x88 : 0x80);
    writeReg(REG_SYNCVALUE3, group);

    writeReg(REG_FRFMSB, frf >> 16);
    writeReg(REG_FRFMSB+1, frf >> 8);
    writeReg(REG_FRFMSB+2, frf);

    rxstate = TXIDLE;
    
}

uint8_t* recvBuf;

uint16_t RF69::recvDone_compat (uint8_t* buf) {
    switch (rxstate) {
    case TXIDLE:
        //rxfill = rf12_len = 0;
    //    Serial.println("TXIDLE:");
        rxfill = rf12_buf[2] = 0;
        crc = _crc16_update(~0, group);
        recvBuf = buf;
        rxstate = TXRECV;
        flushFifo();
        writeReg(REG_DIOMAPPING1, DMAP1_SYNCADDRESS);    // Interrupt trigger
        setMode(MODE_RECEIVER);
        writeReg(REG_AFCFEI, AFC_CLEAR);
        break;
    case TXRECV:
    //Serial.println("TXRECV:");
        if (rxfill >= rf12_len + 5 || rxfill >= RF_MAX) {
            rxstate = TXIDLE;
            setMode(MODE_STANDBY);
            if (crc != 0 | rf12_len > RF12_MAXDATA)
                return 1; // force bad crc for invalid packet
            if (!(rf12_hdr & RF12_HDR_DST) || node == 31 ||
                    (rf12_hdr & RF12_HDR_MASK) == node)
                return 0; // it's for us, good packet received
        }
        break;
    }
      //Serial.println("return ~0;");
    return ~0; // keep going, not done yet
}

void RF69::sendStart_compat (uint8_t hdr, const void* ptr, uint8_t len) {
//	Serial.println("\r\nRF69::sendStart_compat ");Serial.flush();//debug
    rf12_buf[2] = len;
    for (int i = 0; i < len; ++i)
        rf12_data[i] = ((const uint8_t*) ptr)[i];
    rf12_hdr = hdr & RF12_HDR_DST ? hdr : (hdr & ~RF12_HDR_MASK) + node;
    crc = _crc16_update(~0, group);
    rxstate = - (2 + rf12_len); // preamble and SYN1/SYN2 are sent by hardware
    flushFifo();
    setMode(MODE_TRANSMITTER);
    writeReg(REG_DIOMAPPING1, 0x00); // PacketSent

    // use busy polling until the last byte fits into the buffer
    // this makes sure it all happens on time, and that sendWait can sleep
    while (rxstate < TXDONE)
        if ((readReg(REG_IRQFLAGS2) & IRQ2_FIFOFULL) == 0) {
            uint8_t out = 0xAA;
            if (rxstate < 0) {
                out = recvBuf[3 + rf12_len + rxstate];
                crc = _crc16_update(crc, out);
            } else {
                switch (rxstate) {
                    case TXCRC1: out = crc; break;
                    case TXCRC2: out = crc >> 8; break;
                }
            }
            writeReg(REG_FIFO, out);
            ++rxstate;
        }
}

void RF69::interrupt_compat () {
    if (rxstate == TXRECV) {
        // The following line attempts to stop further interrupts
        writeReg(REG_DIOMAPPING1, 0x40);  // Interrupt on PayloadReady
        rssi = readReg(REG_RSSIVALUE);
        IRQ_ENABLE; // allow nested interrupts from here on
        //interrupts();
        for (;;) { // busy loop, to get each data byte as soon as it comes in
            if (readReg(REG_IRQFLAGS2) & (IRQ2_FIFONOTEMPTY|IRQ2_FIFOOVERRUN)) {
                if (rxfill == 0)
                    recvBuf[rxfill++] = group;
                uint8_t in = readReg(REG_FIFO);
                recvBuf[rxfill++] = in;
                crc = _crc16_update(crc, in);
                if (rf12_len > RF12_MAXDATA)
                    rxfill = RF_MAX; // bail out now, the length is invalid
                if (rxfill >= rf12_len + 5 || rxfill >= RF_MAX)
                    break;
            }
        }
    } else if (readReg(REG_IRQFLAGS2) & IRQ2_PACKETSENT) {
        // rxstate will be TXDONE at this point
        rxstate = TXIDLE;
        setMode(MODE_STANDBY);
        writeReg(REG_DIOMAPPING1, 0x80); // SyncAddress // Interrupt trigger
    }
}

