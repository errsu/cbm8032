//----------------------------------------------------------------------------------------
// app_cbm_video_observer.xc
//
// (c) 2016 errsu, all rights reserved
//
//----------------------------------------------------------------------------------------

#include <xs1.h>
#include <platform.h>

#include <stdlib.h>

void uart_tx_streaming(out port p, streaming chanend c, int clocks) {
    int t;
    unsigned char b;
    while (1) {
        c :> b;
        p <: 0 @ t; //send start bit and timestamp (grab port timer value)
        t += clocks;
#pragma loop unroll(8)
        for(int i = 0; i < 8; i++) {
            p @ t <: >> b; //timed output with post right shift
            t += clocks;
        }
        p @ t <: 1; //send stop bit
        t += clocks;
        p @ t <: 1; //wait until end of stop bit
    }
}

on tile[0]: out port p_uart_tx = XS1_PORT_1F;  // J7 pin 1
on tile[0]: in  port p_trigger = XS1_PORT_1H;  // J7 pin 2
on tile[0]: in  port p_address = XS1_PORT_32A; // lower 16 pins only, at J3 and J8
on tile[0]: in  port p_data    = XS1_PORT_8B;  // J7 5/7/9/13/12/14/6/8

// 1 tick == 10 ns
// f = 100,000,000 / TICKS_PER_BIT
// 7   = 14.28 MBit/sec (minimum working for 125MHz XMOS thread)
// 50  = 2Mbit/sec
// 100 = 1MBit/sec
// 868 = 115200 bit/sec
// 10419 = 9600 bit/sec

// 1665000 bit/sec needed for 50Hz * 25 rows * 111 characters per row * 12 bits per character
// 60 = 1666666 bit/sec = 50,05 Hz

#define TICKS_PER_BIT 60

static unsigned char base64[64] = { 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
                                    'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
                                    'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
                                    'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
                                    'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
                                    'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
                                    'w', 'x', 'y', 'z', '0', '1', '2', '3',
                                    '4', '5', '6', '7', '8', '9', '+', '/' };

void observer(streaming chanend c_tx)
{
  unsigned char buffer[2048]; // 25 lines * (1 line number + 80 data bytes)

  unsigned index = 0;
  for (unsigned row = 0; row < 25; row++)
  {
    buffer[index++] = row;
    for (unsigned col = 0; col < 80; col++)
    {
      buffer[index++] = 0;
    }
  }

  unsigned trigger = 0;
  unsigned chcount = 0; // 108 encoded bytes, \r, \n, idle
  index = 0;

  unsigned time;
  timer t;

  t :> time;
  while (1)
  {
    select
    {
      case p_trigger when pinsneq(trigger) :> trigger:
        if (trigger == 0)
        {
          unsigned address;
          p_address :> address;
          address &= 0xFFFF;

          unsigned data;
          p_data :> data;

          if (address >= 0x8000 && address < 0x87D0)
          {
            unsigned index = address - 0x8000;            // < 2000
            unsigned row = index % 80;                    // < 25
            unsigned col = index - (row * 80);            // < 80
            buffer[row * 81 + col] = (unsigned char)data; // respect line index
          }
        }
        break;

      case t when timerafter (time + (TICKS_PER_BIT * 12)) :> time:
      {
        // theoretically the c_tx channel has room for 8 bytes,
        // but we won't rely on this, therefore we always send
        // single bytes only, otherwise the output operation would
        // block and we would miss bus writes

        if (chcount < 108)
        {
          // base64: encoding 3 bytes in 4 characters (6 bits each)
          switch (chcount % 4)
          {
          case 0: c_tx <: base64[                             (buffer[index    ] >> 2)]; break;
          case 1: c_tx <: base64[(buffer[index    ] & 0x03) | (buffer[index + 1] >> 4)]; break;
          case 2: c_tx <: base64[(buffer[index + 1] & 0x0F) | (buffer[index + 2] >> 6)]; break;
          case 3: c_tx <: base64[(buffer[index + 2] & 0x3F)]; index += 3;                break;
          }
          chcount += 1;
        }
        else if (chcount == 108)
        {
          c_tx <: (unsigned char)'\r';
          chcount += 1;
        }
        else if (chcount == 109)
        {
          c_tx <: (unsigned char)'\n';
          chcount += 1;
        }
        else
        {
          // idle
          chcount = 0;
          if (index == (81*25))
          {
            index = 0;
          }
        }
      }
      break;
    }
  }
}

int main()
{
  streaming chan c_tx;
  par
  {
    on tile[0]: uart_tx_streaming(p_uart_tx, c_tx, TICKS_PER_BIT);
    on tile[0]: observer(c_tx);
  }
  return 0;
}
