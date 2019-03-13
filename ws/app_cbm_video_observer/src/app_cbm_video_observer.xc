//----------------------------------------------------------------------------------------
// app_cbm_video_observer.xc
//
// (c) 2016 errsu, all rights reserved
//
//----------------------------------------------------------------------------------------

#include <xs1.h>
#include <platform.h>

#include <stdlib.h>
#include <stdio.h>
#include <safestring.h>
#include "shared_memory.h"
#include "nbsp.h"

// 1 tick == 10 ns
// f = 100,000,000 / TICKS_PER_BIT
// 7   = 14.28 Mbit/s (minimum working for 125MHz XMOS thread)
// 50  = 2Mbit/s
// 60 = 1666666 bit/s
// 100 = 1MBit/s
// 868 = 115200 bit/s
// 10419 = 9600 bit/s

// New version with frame sync and binary data transfer
// ----------------------------------------------------
// Transferred frame structure
// 52 buffers of 41 characters
// Last character of each buffer is the buffer number (0..51)
// First buffer (number zero) is completely zero, so the receiver should
// sync to 41 zeroes, preceeded by a non-zero (a value of 51, actually).
// Following buffers (numbers 1 to 50) contain 80x25 bytes of screen data in characters 0..39.
// Last buffer (number 51) contains graphic state in character 0 and 39 zeroes, followed by 51.
//
// The required bandwidth at 8N1 with 11 bit cycles per byte and 50 Hz
// is 52*41*11*50 = 1172600 bit/s. With 10 bit/cycle it's 1066000 bit/s.
//
// Looking for solutions with correct baud rates on both sides.
//
// Solution A: reliable, possibly tight:
// baud rate = 1250000 bit/s
// TICKS_PER_BIT = 80
// RASPI1/2 using UART0 (ttyAMA0 in Linux)
// Set in options.txt or programmatically in bcm2709
// init_uart_clock=20000000 (or 40MHz, 16x or 32x the baud rate)
// init_uart_baud=1250000
// RASPI3 using UART1 (ttyS0 in Linux)
// then the clock is the core VPU clock (>=250MHz)
// and only init_uart_baud needs to be set
//
// Solution B: faster, still OK, enough room:
// baud rate = 1562500 bit/s
// TICKS_PER_BIT = 64
// init_uart_clock=25000000 (16 * baud rate)
// init_uart_baud=1562500

// Solution C: lots of room, was too fast for RASPI last time:
// baud rate = 2000000 bit/s
// TICKS_PER_BIT = 50
// init_uart_clock=32000000 (16 * baud rate)
// init_uart_baud=2000000

on tile[0]: const clock refClk = XS1_CLKBLK_REF;
#define TICKS_PER_BIT 50

void uart_tx(out port p, chanend c_tx) {

  t_nbsp_state tx_state;
  NBSP_INIT_NO_BUFFER(c_tx, tx_state);

  configure_out_port_no_ready(p, refClk, 1);

  int t;
  unsigned char b;
  while (1) {
    NBSP_RECEIVE_MSG(tx_state);
    if (nbsp_handle_msg(tx_state))
    {
      b = (unsigned char)nbsp_received_data(tx_state);
      p <: 0 @ t; //send start bit and timestamp (grab port timer value)
      t += TICKS_PER_BIT;
  #pragma loop unroll(8)
      for(int i = 0; i < 8; i++) {
        p @ t <: >> b; //timed output with post right shift
        t += TICKS_PER_BIT;
      }
      p @ t <: 1; //send stop bit
      t += TICKS_PER_BIT;
      p @ t <: 1; //wait until end of stop bit
    }
  }
}

on tile[0]: out port p_uart_tx = XS1_PORT_1F;  // J7 pin 1
on tile[0]: in  port p_graphic = XS1_PORT_1I;  // J7 pin 20
on tile[0]: in  port p_trigger = XS1_PORT_1H;  // J7 pin 2
on tile[0]: in  port p_address = XS1_PORT_32A; // lower 16 pins only, at J3 and J8
on tile[0]: in  port p_data    = XS1_PORT_8B;  // J7 5/7/9/13/12/14/6/8
on tile[0]: in  port p_frame   = XS1_PORT_1G;  // J7 pin 3

#define CMD_WRITE   0x01000000
#define CMD_GRAPHIC 0x02000000
#define CMD_FRAME   0x03000000

void observer(chanend c_observer)
{
  unsigned trigger = 0;
  unsigned graphic = 0;
  unsigned frame = 0;
  timer t;

  t_nbsp_state observer_state;
  NBSP_INIT_WITH_BUFFER(c_observer, observer_state, 2048); // holds a whole frame

  while (1)
  {
#pragma ordered
    select
    {
      case p_trigger when pinsneq(trigger) :> trigger:
        if (trigger == 0)
        {
          unsigned time;
          t :> time;
          t when timerafter(time + 70) :> void;

          unsigned address;
          p_address :> address;
          unsigned data;
          p_data :> data;

          // - we are only interested in range 0x8000..0x87FF
          // - address consists of a low-active 0x8000..0x8FFF range indicator at bit 15
          //   and a 11 bit (0x7FF) address, assuming range 0x8800...0x8FFF is not used,
          // - the range indicator is low-active
          // - the unused bits may have arbitrary values and are to be ignored
          // - data should be 8 bit only, we are assuming higher bits are all zero

          if ((address & 0x8000) == 0) // 0x8000 address indicator is low-active
          {
            nbsp_send(observer_state, CMD_WRITE | ((address & 0x07FF) << 8) | data);
          }
        }
        break;

      case p_graphic when pinsneq(graphic) :> graphic:
        nbsp_send(observer_state, CMD_GRAPHIC | graphic);
        break;

      case p_frame when pinsneq(frame) :> frame:
        if (frame == 0)
        {
          nbsp_send(observer_state, CMD_FRAME);
        }
        break;

      case NBSP_RECEIVE_MSG(observer_state):
        nbsp_handle_msg(observer_state); // pushing the buffer through
        break;
    }
  }
}

void observer_mockup(chanend c_observer)
{
  t_nbsp_state observer_state;
  NBSP_INIT_WITH_BUFFER(c_observer, observer_state, 2048); // holds a whole frame

  unsigned char data = 'A';
  unsigned char graphic = 0;

  unsigned time;
  timer t;
  t :> time;

  time += 100; // 1 usec
  t when timerafter (time) :> void;

  unsigned address = 0;

  while (1)
  {
    // TODO: check for "nbsp buffer full"
#pragma ordered
    select
    {
      case t when timerafter(time) :> void:
        nbsp_send(observer_state, CMD_WRITE | ((address & 0x07FF) << 8) | data);
        address += 1;

        time += 100; // 1 usec

        if (address == 25 * 80)
        {
          nbsp_send(observer_state, CMD_GRAPHIC | graphic);
          nbsp_send(observer_state, CMD_FRAME);

          data += 1;
          if (data > 'Z')
          {
            data = 'A';
          }
          graphic = graphic? 0 : 1;

          address = 0;
          time += 1800000; // 18 ms
        }
        break;

      case NBSP_RECEIVE_MSG(observer_state):
        nbsp_handle_msg(observer_state); // pushing the buffer through
        break;
    }
  }
}

void observer_mockup2(chanend c_observer)
{
  t_nbsp_state observer_state;
  NBSP_INIT_WITH_BUFFER(c_observer, observer_state, 2048); // holds a whole frame

                      //  12345678901234567890123456789012345678901234567890123456789012345678901234567890
  unsigned char tl[81] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz+-:;*!$%&/(){}[]?=";

  unsigned time;
  timer t;
  t :> time;

  while (1)
  {
    time += 2000000; // 50 Hz
    t when timerafter (time) :> void;

    for (unsigned line = 0; line < 25; line++)
    {
      for (unsigned i = 0; i < 80; i++)
      {
        unsigned address = (line * 80) + i;
        unsigned data = tl[i];
        nbsp_send(observer_state, CMD_WRITE | ((address & 0x07FF) << 8) | data);
      }
    }
    nbsp_send(observer_state, CMD_FRAME);
  }
}

void renderer(chanend c_observer, chanend c_tx)
{
  t_nbsp_state observer_state;
  NBSP_INIT_NO_BUFFER(c_observer, observer_state);

  t_nbsp_state tx_state;
  NBSP_INIT_WITH_BUFFER(c_tx, tx_state, 128); // Q: hold whole frame?

  unsigned char rx_buffer[40*51];
  unsigned char tx_buffer[40*52]; // first line with all zeroes

  safememset(rx_buffer, 0, sizeof(rx_buffer));
  safememset(tx_buffer, 0, sizeof(tx_buffer));

  unsigned pending_tx = 0;
  unsigned line;
  int ch_index;

  while (1)
  {
#pragma ordered
    select
    {
    case NBSP_RECEIVE_MSG(observer_state):
      if (nbsp_handle_msg(observer_state))
      {
        // incoming data from observer
        unsigned message = nbsp_received_data(observer_state);
        unsigned command = message & 0xFF000000;
        switch(command)
        {
        case CMD_WRITE:
          unsigned address = (message >> 8 ) & 0x07FF;
          unsigned char data = (message & 0xFF);
          rx_buffer[address] = data;
          break;

        case CMD_GRAPHIC:
          unsigned char graphic = (message & 0xFF);
          rx_buffer[40*50] = graphic;
          break;

        case CMD_FRAME:
          if (pending_tx)
          {
            // could not send out frame until next arrived
            // -> skipping frame signal
            break;
          }
          safememcpy(tx_buffer + 40, rx_buffer, sizeof(rx_buffer));
          line = 0;    // 0 to 51
          ch_index = 0; // 0 to 40
          while (line < 52)
          {
            unsigned byte_to_send = (ch_index == 40) ? line : tx_buffer[line * 40 + ch_index];
            if (nbsp_send(tx_state, byte_to_send))
            {
              ch_index += 1;
              if (ch_index == 41)
              {
                line += 1;
                ch_index = 0;
              }
            }
            else
            {
              pending_tx = 1;
              break;
            }
          }
          break;
        }
      }
      break;

    case NBSP_RECEIVE_MSG(tx_state):
      if (!nbsp_handle_msg(tx_state))
      {
        // ack received - buffer might have more room
        if (pending_tx) {
          unsigned byte_to_send = (ch_index == 40) ? line : tx_buffer[line * 40 + ch_index];
          nbsp_send(tx_state, byte_to_send); // should succeed
          ch_index += 1;
          if (ch_index == 41)
          {
            line += 1;
            ch_index = 0;
            if (line == 52)
            {
              pending_tx = 0;
            }
          }
        }
      }
      break;
    }
  }
}

#define TEST_RECEIVER 0
#if TEST_RECEIVER
// Test receiver ------------------------------------------------------------

on tile[0]: in port p_uart_rx = XS1_PORT_1E;  // X0D13 J7 pin 4

static void uart_rx(streaming chanend c_rx)
{
  configure_in_port_no_ready(p_uart_rx, refClk);
  clearbuf(p_uart_rx);

  int t;
  unsigned dummy1;
  while (1) {
    p_uart_rx when pinseq(0) :> dummy1 @ t; // wait until falling edge of start bit
    t += (TICKS_PER_BIT * 3)>>1; //one and a half bit times
#pragma loop unroll(8)
    unsigned data;
    for(int i = 0; i < 8; i++) {
      p_uart_rx @ t :> >> data; //sample value when port timer = t
                  //inlcudes post right shift
        t += TICKS_PER_BIT;
    }
    c_rx <: (unsigned char)(data >> 24); //send to client
    p_uart_rx @ t :> void;
  }
}

static void collector(streaming chanend c_rx, chanend c_collector)
{
  t_nbsp_state collector_state;
  NBSP_INIT_WITH_BUFFER(c_collector, collector_state, 2048); // holds a whole frame

  unsigned char data;

  while (1) {
#pragma ordered
    select
    {
      case c_rx :> data:
        if (!nbsp_send(collector_state, (unsigned)data))
        {
          printf("send failed\n");
        }
        break;

      case NBSP_RECEIVE_MSG(collector_state):
        nbsp_handle_msg(collector_state); // pushing the buffer through
        break;
    }
  }
}

//----------------------------------------------------------------------------------------

static unsigned needs_frame_sync = 1;

static void handle_received_buffer(unsigned bufnum, unsigned char buffer[40])
{
  static unsigned char frame = 0;
  static unsigned framecount = 0;
  static unsigned errorcount = 0;

  unsigned char expected = (bufnum == 0) ? 0 : (bufnum == 51) ? 0 : (0x41 + frame);
  unsigned char graphic = 0xFF;

  for (unsigned i = 0; i < 40; i++)
  {
    unsigned char bb = buffer[i];
    if (bufnum == 51 && i == 0)
    {
      graphic = bb;
    }
    else if (bb != expected)
    {
      if (needs_frame_sync) {
        frame = bb - 0x41;
        expected = bb;
        needs_frame_sync = 0;
      } else {
        errorcount++;
      }
    }
  }
  if (bufnum == 51)
  {
    frame += 1;
    if (frame == 26) {
      frame = 0;
    }
    if (errorcount) {
      printf("errors in frame %d: %d\n", framecount, errorcount);
      errorcount = 0;
      needs_frame_sync = 1;
    }
    framecount++;
    if (framecount == 60) {
      printf(".\n");
      needs_frame_sync = 1; // spending too much time in receiver -> need resync afterwards
      framecount = 0;
    }
  }
}

#define STATE_OUT_OF_SYNC    0
#define STATE_COUNTING_ZEROS 1
#define STATE_IN_SYNC        2

typedef struct {
  unsigned state;
  unsigned bufnum;
  unsigned count;
  unsigned char buffer[40];
} t_receiver_context;

static void init_receiver_context(t_receiver_context& context)
{
  context.state = STATE_OUT_OF_SYNC;
}

static void handle_sync_loss(t_receiver_context& context, unsigned char byte)
{
  printf("out of sync at bufnum %d count %d - received %02x\n", context.bufnum, context.count, byte);
  needs_frame_sync = 1;
}

static void handle_received_byte(unsigned char byte, t_receiver_context& context)
{
  switch(context.state)
  {
  case STATE_OUT_OF_SYNC:
    if (byte != 0)
    {
      context.state = STATE_COUNTING_ZEROS;
      context.count = 0;
    }
    break;
  case STATE_COUNTING_ZEROS:
    if (byte == 0)
    {
      context.count += 1;
      if (context.count == 41)
      {
        context.state = STATE_IN_SYNC;
        context.bufnum = 1;
        context.count = 0;
      }
      else
      {
      }
    }
    else
    {
      context.count = 0;
    }
    break;

  case STATE_IN_SYNC:
    if (context.count < 40)
    {
      context.buffer[context.count] = byte;
      context.count += 1;
    }
    else
    {
      if (byte == (unsigned char)context.bufnum)
      {
        handle_received_buffer(context.bufnum, context.buffer);
        context.bufnum += 1;
        context.count = 0;
        if (context.bufnum == 52)
        {
          context.state = STATE_COUNTING_ZEROS;
        }
      }
      else
      {
        handle_sync_loss(context, byte);
        context.state = STATE_OUT_OF_SYNC;
      }
    }
    break;
  }
}

static void test_receiver(chanend c_collector)
{
  t_nbsp_state receiver_state;
  NBSP_INIT_NO_BUFFER(c_collector, receiver_state);

  t_receiver_context context;
  init_receiver_context(context);

  while (1) {
    NBSP_RECEIVE_MSG(receiver_state);
    if (nbsp_handle_msg(receiver_state))
    {
      unsigned char byte = (unsigned char)nbsp_received_data(receiver_state);
      handle_received_byte(byte, context);
    }
  }
}
#endif

int main()
{
  chan c_observer, c_tx;
#if TEST_RECEIVER
  chan c_collector;
  streaming chan c_rx;
#endif

  par
  {
    on tile[0]: observer_mockup(c_observer);
    on tile[0]: renderer(c_observer, c_tx);
    on tile[0]: uart_tx(p_uart_tx, c_tx);

#if TEST_RECEIVER
    on tile[0]: uart_rx(c_rx);
    on tile[0]: collector(c_rx, c_collector);
    on tile[0]: test_receiver(c_collector);
#endif
  }
  return 0;
}
