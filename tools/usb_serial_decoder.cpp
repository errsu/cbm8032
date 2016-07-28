#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

//----------------------------------------------------------------------------------------

static int open_serial_port()
{
  const char *portname = "/dev/ttyUSB0";

  int fd = open(portname, O_RDONLY | O_NOCTTY);
  if (fd < 0)
  {
    fprintf(stderr, "error %d opening %s: %s\n", errno, portname, strerror (errno));
    exit(1);
  }

  struct termios serialOpt;

  // all other bits are unset
  memset(&serialOpt, 0, sizeof(serialOpt));
  serialOpt.c_cflag = CS8 | CLOCAL | CREAD; // 8N1
  serialOpt.c_cc[VTIME] = 0; // timeout between characters in hundreds of ms
  serialOpt.c_cc[VMIN]  = 1; // block reading until 1 character received
  cfsetispeed (&serialOpt, B2000000);

  tcflush(fd, TCIFLUSH);

  if (tcsetattr(fd, TCSANOW, &serialOpt) != 0)
  {
    fprintf(stderr, "error %d setting tty attributes\n", errno);
    exit(1);
  }
  return fd;
}

//----------------------------------------------------------------------------------------
// base64 decoding
//
// 4 x incoming characters -> 4 x 6-bit values -> 3 x 8-bit decoded data
//
//  +--first octet--+-second octet--+--third octet--+
//  |7 6 5 4 3 2 1 0|7 6 5 4 3 2 1 0|7 6 5 4 3 2 1 0| -> 3 Bytes
//  +-----------+---+-------+-------+---+-----------+
//  |5 4 3 2 1 0|5 4 3 2 1 0|5 4 3 2 1 0|5 4 3 2 1 0| <- 4 Values
//  +--1.index--+--2.index--+--3.index--+--4.index--+
//
//   Value Char  Value Char  Value Char  Value Char
//       0 A        17 R        34 i        51 z
//       1 B        18 S        35 j        52 0
//       2 C        19 T        36 k        53 1
//       3 D        20 U        37 l        54 2
//       4 E        21 V        38 m        55 3
//       5 F        22 W        39 n        56 4
//       6 G        23 X        40 o        57 5
//       7 H        24 Y        41 p        58 6
//       8 I        25 Z        42 q        59 7
//       9 J        26 a        43 r        60 8
//      10 K        27 b        44 s        61 9
//      11 L        28 c        45 t        62 +
//      12 M        29 d        46 u        63 /
//      13 N        30 e        47 v
//      14 O        31 f        48 w       pad =   (unused)
//      15 P        32 g        49 x
//      16 Q        33 h        50 y
//

unsigned char base64_to_binary[256] = {
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63,
  52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1,
  -1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
  15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1,
  -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
  41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
};

static int decode_base64_buffer(unsigned char inbuf[108], unsigned char outbuf[81])
{
  int iin  = 0;
  int iout = 0;

  while (iin < 108)
  {
    unsigned char b0 = base64_to_binary[inbuf[iin++]];
    unsigned char b1 = base64_to_binary[inbuf[iin++]];
    unsigned char b2 = base64_to_binary[inbuf[iin++]];
    unsigned char b3 = base64_to_binary[inbuf[iin++]];

    if (b0 == -1 || b1 == -1 || b2 == -1 || b3 == -1)
    {
      return 0;
    }

    outbuf[iout++] = ( b0         << 2) | (b1 >> 4);
    outbuf[iout++] = ((b1 & 0x0F) << 4) | (b2 >> 2);
    outbuf[iout++] = ((b2 & 0x03) << 6) |  b3;
  }
  return 1;
}

//----------------------------------------------------------------------------------------

static unsigned char display_buffer[25][81];
static unsigned char flags_buffer[80];
static unsigned char next_line = 0;

static void received_frame()
{
  fprintf(stdout, "\033[H");
  for (int line = 0; line < 25; line++)
  {
    fprintf(stdout, "%s\n", display_buffer[line]);
  }
  fprintf(stdout, "%s\n", flags_buffer[0] ? "graphic" : "text   ");
  fflush(stdout);
}

static void received_line(unsigned char inbuf[108])
{
  unsigned char decoded_line[81];

  if (decode_base64_buffer(inbuf, decoded_line))
  {
    unsigned char line = decoded_line[0];
    if (line == next_line)
    {
      if (next_line < 25)
      {
        memcpy(display_buffer[next_line], decoded_line + 1, 80);
      }
      else
      {
        memcpy(flags_buffer, decoded_line + 1, 80);
      }
      next_line++;
      if (next_line == 26)
      {
        received_frame();
        next_line = 0;
      }
    }
    else
    {
      next_line = 0; // skip incomplete frames
    }
  }
}

//----------------------------------------------------------------------------------------

int main(int argc, char*argv[])
{
  int fd = open_serial_port();

  for (int line = 0; line < 25; line++)
  {
    memset(display_buffer[line], ' ', 80);
    display_buffer[line][80] = '\0';
  }
  memset(flags_buffer, 0, 80);

  unsigned char inbuf[108];
  int iin = 0;

  while (1)
  {
    unsigned char byte;
    int n = read(fd, &byte, 1);
    if (n != 1)
    {
      fprintf(stderr, "read failed\n");
      exit(1);
    }
    if (byte == '\n' || byte == '\r') // don't make assumptions on line ending
    {
      if (iin == 108)
      {
        received_line(inbuf);
      }
      iin = 0;
    }
    else
    {
      if (iin < 108)
      {
        inbuf[iin] = byte;
      }
      iin++;
    }
  }
}

// eof
