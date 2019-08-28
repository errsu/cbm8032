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
#include "video_memory.h"
#include "nbsp.h"

// 1 tick == 10 ns
// f = 100,000,000 / TICKS_PER_BIT

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
// Normal Speed: faster, still OK, enough room:
// baud rate = 1562500 bit/s
// TICKS_PER_BIT = 64
// init_uart_clock=25000000 (16 * baud rate)
// init_uart_baud=1562500

// Faster data transfer: solution for FT232RL USB-serial adapter
// baud rate = 3000000 bit/s
// TICKS_PER_BIT = 33

on tile[0]: const clock refClk = XS1_CLKBLK_REF;
#define TICKS_PER_BIT 64

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

on tile[0]: out port p_uart_tx = XS1_PORT_1F;  // D13 J7 pin 1
on tile[0]: in  port p_graphic = XS1_PORT_1I;  // D24 J7 pin 20
on tile[0]: in  port p_trigger = XS1_PORT_1H;  // D23 J7 pin 2
on tile[0]: in  port p_address = XS1_PORT_32A; // lower 16 pins only, at J3 and J8
on tile[0]: in  port p_data    = XS1_PORT_8B;  // J7 5/7/9/13/12/14/6/8
on tile[0]: in  port p_frame   = XS1_PORT_1G;  // D22 J7 pin 3

void memory_observer()
{
  unsigned trigger = 0;
  unsigned graphic = 0;
  timer t;

  video_memory_init();

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
            unsigned video_buffer_address = address & 0x07FF;
            if (video_buffer_address < 2000) {
              video_memory_write_data(video_buffer_address, data & 0xFF);
            }
          }
        }
        break;

      case p_graphic when pinsneq(graphic) :> graphic:
        video_memory_write_graphic(graphic);
        break;

    }
  }
}

static unsigned frame_rate = 50; // only 50 or 60 are allowed

static void check_frame_rate(unsigned time, unsigned last)
{
  if (last == 0)
  {
    // no change if first frame or (by seldom chance) wrapping just to zero
    return;
  }

  unsigned delta = (time - last) / 100; // in microseconds

  // fps    frame    missed frame
  // 50     20ms     40ms
  // 60     16.66ms  33.33 ms
  // using a tolerance of 5%, make sure areas don't overlap

  if (frame_rate == 50) // we test only for changes
  {
    if ((15800 < delta && delta < 17500) || (31600 < delta && delta < 35000))
    {
      frame_rate = 60;
    }
  }
  else
  {
    if ((19000 < delta && delta < 21000) || (38000 < delta && delta < 42000))
    {
      frame_rate = 50;
    }
  }
}

void frame_observer(chanend c_observer)
{
  unsigned frame = 0;
  timer t;
  unsigned last_frame_time = 0;
  unsigned buf_num = 0;

  t_nbsp_state observer_state;
  NBSP_INIT_WITH_BUFFER(c_observer, observer_state, 8); // we need only one frame signal, don't we?

  while (1)
  {
#pragma ordered
    select
    {
      case p_frame when pinsneq(frame) :> frame:
        if (frame == 0)
        {
          unsigned time;
          t :> time;
          check_frame_rate(time, last_frame_time);
          last_frame_time = time;

          for (unsigned line = 0; line < 25; line++)
          {
            video_memory_copy_line_to(buf_num, line);
          }
          video_memory_copy_flags_to(buf_num); // TODO: check if we want to copy flags before
          nbsp_send(observer_state, buf_num);

          buf_num += 1;
          if (buf_num == VIDEO_BUF_COPIES)
          {
            buf_num = 0;
          }
        }
        break;

      // Q: is this faster than the frame signal duration?
      //    if not, we would miss a frame (visually hard to see)
      // Q: is the nbsp buffer ever used?
      case NBSP_RECEIVE_MSG(observer_state):
        nbsp_handle_msg(observer_state); // pushing the buffer through
        break;
    }
  }
}

void renderer(chanend c_observer, chanend c_tx)
{
  t_nbsp_state observer_state;
  NBSP_INIT_NO_BUFFER(c_observer, observer_state);

  t_nbsp_state tx_state;
  NBSP_INIT_WITH_BUFFER(c_tx, tx_state, 128); // Q: hold whole frame?

  unsigned pending_tx = 0;
  unsigned buf_num = 0;
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
        buf_num = nbsp_received_data(observer_state);
        if (pending_tx)
        {
          // could not send out frame until next arrived
          // -> skipping frame signal
          break;
        }
        line = 0;    // 0 to 51
        ch_index = 0; // 0 to 40
        while (line < 52)
        {
          unsigned byte_to_send = (ch_index == 40) ? line : video_memory_read_from_copy(buf_num, line * 40 + ch_index);
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
      }
      break;

    case NBSP_RECEIVE_MSG(tx_state):
      if (!nbsp_handle_msg(tx_state))
      {
        // ack received from uart_tx - tx buffer might have more room
        if (pending_tx) {
          unsigned byte_to_send = (ch_index == 40) ? line : video_memory_read_from_copy(buf_num, line * 40 + ch_index);
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

int main()
{
  chan c_observer, c_tx;

  par
  {
    on tile[0]: memory_observer();
    on tile[0]: frame_observer(c_observer);
    on tile[0]: renderer(c_observer, c_tx);
    on tile[0]: uart_tx(p_uart_tx, c_tx);
  }
  return 0;
}
