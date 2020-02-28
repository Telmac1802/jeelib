#pragma once

#include <stdint.h>

// same code as in util/crc16.h
uint16_t _crc16_update(uint16_t crc, uint8_t a)
{
	  int i;
	  crc ^= a;
	  for (i = 0; i < 8; ++i)  {
	    if (crc & 1)
	      crc = (crc >> 1) ^ 0xA001;
	    else
	      crc = (crc >> 1);
	  }
	  return crc;
}


/** \ingroup util_crc
    Optimized CRC-XMODEM calculation.

    Polynomial: x^16 + x^12 + x^5 + 1 (0x1021)<br>
    Initial value: 0x0

    This is the CRC used by the Xmodem-CRC protocol.

    The following is the equivalent functionality written in C.
*/
  
    uint16_t _crc_xmodem_update (uint16_t crc, uint8_t data)
    {
        int i;

        crc = crc ^ ((uint16_t)data << 8);
        for (i=0; i<8; i++)
        {
            if (crc & 0x8000)
                crc = (crc << 1) ^ 0x1021;
            else
                crc <<= 1;
        }

        return crc;
    }
  