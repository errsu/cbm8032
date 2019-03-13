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
  int uart0_filestream = open("/dev/serial0", O_RDONLY | O_NOCTTY | O_NDELAY); // non blocking read mode

  if (uart0_filestream == -1)
  {
    perror("unable to open /dev/serial0");
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
  options.c_cflag = B2000000 | CS8 | CLOCAL | CREAD;   // Set baud rate
  options.c_iflag = IGNPAR;
  options.c_oflag = 0;
  options.c_lflag = 0;
  tcflush(uart0_filestream, TCIFLUSH);
  tcsetattr(uart0_filestream, TCSANOW, &options);

  return uart0_filestream;
}

static void close_uart0(int uart0_filestream)
{
  if (close(uart0_filestream) == -1) {
    perror("unable to close /dev/serial0");
  }
}

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

// wait for a specific character
void waituntil(int endchar) {
    int key;
    for (;;) {
        key = getchar();
        if (key == endchar || key == '\n') {
            break;
        }
    }
}

int main(int argc, char **argv) {
  int uart0_filestream = open_uart0();

  if (uart0_filestream > 0) {
    printf("usart opened\n");
  }

  for (unsigned repeat = 0; repeat < 10; repeat++) {
    unsigned char buffer[256];
    int count = read_bytes(uart0_filestream, buffer, 256);
    if (count >= 0) {
      printf("%d bytes read\n", count);
      for (unsigned i = 0; i < count; i++) {
        printf("%02x ", buffer[i]);
        if (i % 16 == 15) {
          printf("\n");
        }
      }
    }
  }

  // printf("press return to terminate\n");
  // waituntil(0x1b);

  close_uart0(uart0_filestream);
  printf("exiting\n");
  return 0;
}
