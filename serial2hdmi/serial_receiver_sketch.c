#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <termios.h>
#include <errno.h>

static int open_uart0()
{
  // At bootup, pins 8 and 10 are already set to UART0_TXD,
  // UART0_RXD (ie the alt0 function) respectively

  // OPEN THE UART
  // The flags (defined in fcntl.h):
  //   Access modes (use 1 of these):
  //     O_RDONLY - Open for reading only.
  //     O_RDWR - Open for reading and writing.
  //     O_WRONLY - Open for writing only.
  //
  //     O_NDELAY / O_NONBLOCK (same function) - Enables nonblocking mode.
  //       When set read requests on the file can return immediately with a
  //       failure status if there is no input immediately available
  //       (instead of blocking). Likewise, write requests can also return
  //       immediately with a failure status if the output can't be written immediately.
  //
  //     O_NOCTTY - When set and path identifies a terminal device, open()
  //       shall not cause the terminal device to become the controlling
  //       terminal for the process.

  // int uart0_filestream = open("/dev/ttyS0", O_RDWR | O_NOCTTY | O_NDELAY); // non blocking read/write mode
  int uart0_filestream = open("/dev/ttyUSB0", O_RDONLY | O_NOCTTY | O_NDELAY); // non blocking read mode

  if (uart0_filestream == -1)
  {
    perror("unable to open /dev/ttyUSB0");
    return -1;
  }

  //CONFIGURE THE UART
  //  The flags (defined in /usr/include/termios.h
  //  (actually in /usr/include/asm-generic/termbits.h [errsu])
  //  see http://pubs.opengroup.org/onlinepubs/007908799/xsh/termios.h.html):
  //  Baud rate: B1200, B2400, B4800, B9600, B19200, B38400, B57600, B115200,
  //             B230400, B460800, B500000, B576000, B921600, B1000000, B1152000,
  //             B1500000, B2000000, B2500000, B3000000, B3500000, B4000000
  //  CSIZE:     CS5, CS6, CS7, CS8
  //  CLOCAL:    Ignore modem status lines
  //  CREAD:     Enable receiver
  //  IGNPAR:    Ignore characters with parity errors
  //  ICRNL:     Map CR to NL on input (auto correct end of line characters - not for binary comms)
  //  PARENB:    Parity enable
  //  PARODD:    Odd parity (else even)

  struct termios options;
  tcgetattr(uart0_filestream, &options);
  options.c_cflag = B3000000 | CS8 | CLOCAL | CREAD;   // Set baud rate
  options.c_iflag = IGNPAR;
  options.c_oflag = 0;
  options.c_lflag = 0;
  tcflush(uart0_filestream, TCIFLUSH);
  tcsetattr(uart0_filestream, TCSANOW, &options);

  return uart0_filestream;
}

// static void close_uart0(int uart0_filestream)
// {
//   if (close(uart0_filestream) == -1) {
//     perror("unable to close /dev/serial0");
//   }
// }

static int read_bytes(int uart0_filestream, unsigned char* pBuffer, unsigned int buflen)
{
  //----- CHECK FOR ANY RX BYTES -----
  if (uart0_filestream != -1)
  {
    // Read up to buflen characters from the port if they are there
    int rx_length = read(uart0_filestream, pBuffer, buflen);
    if (rx_length == -1)
    {
      if (errno == EWOULDBLOCK) {
        return 0;
      }
      perror("unable to read from /dev/serial0");
      return -1;
    }
    return rx_length;
  } else {
    return -1;
  }
}

//----------------------------------------------------------------------------------------

#define STATE_COUNTING_ZEROS 1
#define STATE_IN_SYNC        2

typedef struct {
  unsigned char rx_buffer[256];
  unsigned rx_buffer_index;
  unsigned rx_buffer_count;
  unsigned state;
  unsigned bufnum;
  unsigned count;
  unsigned char buffer[40];
  unsigned char* screen_buffer;
  unsigned char* graphic;
} t_receiver_context;

static void init_receiver_context(t_receiver_context* context)
{
  context->state = STATE_COUNTING_ZEROS;
  context->count = 0;
  context->rx_buffer_index = 0;
  context->rx_buffer_count = 0;
}

static void handle_sync_loss(t_receiver_context* context, unsigned char byte)
{
  printf("out of sync at bufnum %d count %d - received %02x\n", context->bufnum, context->count, byte);
}

static void handle_received_buffer(t_receiver_context* context)
{
  if (context->bufnum > 0)
  {
    if (context->bufnum < 51)
    {
      memcpy(context->screen_buffer + (context->bufnum - 1) * 40, context->buffer, 40);
    }
    else
    {
      *(context->graphic) = context->buffer[0];
    }
  }
}

static unsigned handle_received_byte(t_receiver_context* context, unsigned char byte)
{
  unsigned screen_complete = 0;

  switch(context->state)
  {
  case STATE_COUNTING_ZEROS:
    if (byte == 0)
    {
      context->count += 1;
      if (context->count == 41)
      {
        context->state = STATE_IN_SYNC;
        context->bufnum = 1;
        context->count = 0;
      }
    }
    else
    {
      context->count = 0;
    }
    break;

  case STATE_IN_SYNC:
    if (context->count < 40)
    {
      context->buffer[context->count] = byte;
      context->count += 1;
    }
    else
    {
      if (byte == (unsigned char)context->bufnum)
      {
        handle_received_buffer(context);
        context->bufnum += 1;
        context->count = 0;
        if (context->bufnum == 52)
        {
          context->state = STATE_COUNTING_ZEROS;
          screen_complete = 1;
        }
      }
      else
      {
        handle_sync_loss(context, byte);
        context->state = STATE_COUNTING_ZEROS;
        context->count = 0;
      }
    }
    break;
  }
  return screen_complete;
}

static void receive_screen(int stream, t_receiver_context* context, unsigned char* screen_buffer, unsigned char* graphic)
{
  context->screen_buffer = screen_buffer;
  context->graphic = graphic;

  while (1)
  {
    if (context->rx_buffer_index == context->rx_buffer_count)
    {
       context->rx_buffer_index = 0;
       context->rx_buffer_count = read_bytes(stream, context->rx_buffer, 256);
    }
    else
    {
      unsigned char received_byte = context->rx_buffer[context->rx_buffer_index++];
      if (handle_received_byte(context, received_byte))
      {
        return;
      }
    }
  }
}

//----------------------------------------------------------------------------------------

static void print_screen_buffer(unsigned char* screen_buffer, unsigned char graphic)
{
  unsigned i;

  for (i = 0; i < 2000; i++)
  {
    putchar(screen_buffer[i]);
    if (i % 80 == 79)
    {
      putchar('\n');
    }
  }
  printf("graphic: %s\n", graphic == 0 ? "no" : "yes");
}

int main(int argc, char **argv) {

  int uart0_filestream = open_uart0();

  if (uart0_filestream > 0) {
    printf("usart opened\n");
  }

  t_receiver_context context;
  init_receiver_context(&context);

  unsigned char screen_buffer[2000];
  unsigned char graphic;

  while (1) {
    receive_screen(uart0_filestream, &context, screen_buffer, &graphic);
    print_screen_buffer(screen_buffer, graphic);
  }

  // close_uart0(uart0_filestream);
  // printf("exiting\n");
  // return 0;
}
