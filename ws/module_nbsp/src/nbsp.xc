/**
 * Copyright: 2014-2017, Ableton AG, Berlin. All rights reserved.
 *
 */

#include "nbsp.h"

// NBSP - non-blocking bidirectional small package protocol
//
// - the protocol is symmetric, both sides send and receive simultaneously
// - data is sent in 32 bit pieces because they are easy to handle by the HW
// - the sender sends a DATA token, followed by an unsigned int and a PAUSE token
// - the receiver replies with an END token
// - therefore, each player waits for either a DATA token or an END token:
//   (a) DATA token: data word follows, nbsp_handle_msg returns 1
//   (b) END token: acknowledgement was received, nbsp_handle_msg returns 0
// - the PAUSE token closes the connection to prevent congestion
// - the sender waits for an acknowledgement before sending more data,
//   so the data to be sent is buffered on the sender side
// - since no player sends more than one data packet (6 bytes) and
//   one ack packet (1 byte), there are no more than 7 bytes in the
//   queue for each player, and since the FIFO size is not less than 8 bytes,
//   no player is ever blocked waiting for a peer

void nbsp_init(
  chanend c,
  t_nbsp_state& state,
  unsigned * movable buffer,
  unsigned buffer_size)
{
#if CHECK_FOR_PROGRAMMING_ERRORS
  if ((buffer_size & (buffer_size - 1)) != 0 || buffer_size == 1)
  {
    printf("nbsp error: buffer size must be zero or a power of 2 greater than 1\n");
  }
#endif
  unsafe
  {
    state.c = c;
  }
  state.words_to_be_acknowledged = 0;
  queue_init(state.queue, move(buffer), buffer_size);
}

static void send_data(unsafe chanend c, unsigned data)
{
  unsafe_outct(c, NBSP_CT_DATA);
  unsafe_outuint(c, data);
}

static void send_ack(unsafe chanend c)
{
  unsafe_outct(c, XS1_CT_END);
}

unsigned nbsp_handle_msg(t_nbsp_state& state)
{
  if (state.msg_is_ack)
  {
#if CHECK_FOR_PROGRAMMING_ERRORS
    if (isnull(buffer))
    {
      printf("nbsp error: sending side must provide buffer to nbsp_handle_msg\n");
      return 0;
    }
    if (state.words_to_be_acknowledged == 0)
    {
      printf("nbsp error: unexpected ack\n");
    }
#endif

    unsafe_outct(state.c, XS1_CT_PAUSE);

    if (!queue_empty(state.queue))
    {
      // there is more data to send
      unsigned data = queue_out(state.queue);
      send_data(state.c, data);
    }
    else
    {
      state.words_to_be_acknowledged = 0;
    }
    return 0; // no data received
  }
  else
  {
    send_ack(state.c);
    return 1; // did receive data
  }
}

unsigned nbsp_send(t_nbsp_state& state, unsigned data)
{
  if (state.words_to_be_acknowledged == 0)
  {
    // buffer must be empty, we can immediately send, no need for buffering
    send_data(state.c, data);
    state.words_to_be_acknowledged = 1;
    return 1;
  }
  else
  {
    // busy sending, must buffer the data
    return queue_in(state.queue, data);
  }
}

unsigned nbsp_pending_words_to_send(t_nbsp_state& state)
{
  return queue_count(state.queue) + state.words_to_be_acknowledged;
}

unsigned nbsp_sending_capacity(t_nbsp_state& state)
{
  // this is incorrect for uddw
  return queue_room(state.queue) + 1 - state.words_to_be_acknowledged;
}

void nbsp_drop(t_nbsp_state& state)
{
  state.words_to_be_acknowledged = 0;
  queue_drop(state.queue);
}

void nbsp_flush(t_nbsp_state& state)
{
  while(state.words_to_be_acknowledged)
  {
    select
    {
      case NBSP_RECEIVE_MSG(state):
      {
        if (nbsp_handle_msg(state))
        {
#if CHECK_FOR_PROGRAMMING_ERRORS
          printf("nbsp error: unexpected data while flushing sender\n");
#endif
        }
        break;
      }
    }
  }
}

void nbsp_uddw_flush(t_nbsp_state& state)
{
  while(state.words_to_be_acknowledged)
  {
    select
    {
      case NBSP_UDDW_HANDLE_ACK(state):
      {
        break;
      }
    }
  }
}

void nbsp_handle_outgoing_traffic(t_nbsp_state& state, unsigned available_tens_of_ns)
{
  // timed idle function for pure data senders

  // all timings in tens of ns at 62.5 MIPS
  #define TIME_TO_START  44 // call the function and setup the timer
  #define TIME_TO_LOOP   23 // enter the loop and select
  #define TIME_TO_FINISH 95 // receive and handle a message, get the timeout and return

  #define MIN_AVAILABLE (TIME_TO_START + TIME_TO_LOOP + TIME_TO_FINISH)

#if CHECK_FOR_PROGRAMMING_ERRORS
  if (available_tens_of_ns < MIN_AVAILABLE)
  {
    printf("nbsp error: handle_outgoing_traffic needs at least %d tens of nanoseconds\n",
      MIN_AVAILABLE);
  }
#endif

  timer t;
  unsigned time;

  t :> time;
  time += (available_tens_of_ns - (TIME_TO_START + TIME_TO_FINISH));

  while(state.words_to_be_acknowledged)
  {
    select
    {
      case NBSP_RECEIVE_MSG(state):
      {
        if (nbsp_handle_msg(state))
        {
#if CHECK_FOR_PROGRAMMING_ERRORS
          printf("nbsp error: unexpected data in pure sender\n");
#endif
        }
        break;
      }
      case t when timerafter(time) :> void:
      {
        return;
      }
    }
  }
}
