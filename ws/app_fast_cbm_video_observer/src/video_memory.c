#include <safestring.h>

#define COPIES 4
unsigned char video_buffer[40*51];  // 2000 bytes of video data and 40 bytes of flags (with graphic)
unsigned char copied_buffer[COPIES][40*52]; // 40 bytes of zeroes, followed by video_buffer

void video_memory_init()
{
  safememset(video_buffer, 0, sizeof(video_buffer));
  for (unsigned bufnum = 0; bufnum < COPIES; bufnum++)
  {
    safememset(copied_buffer[bufnum], 0, 40); // first 40 bytes must be zero
  }
}

void video_memory_write(unsigned index, unsigned data)
{
  video_buffer[index] = (unsigned char)data;
}

void video_memory_copy_to(unsigned bufnum)
{
  // first 40 bytes are left alone
  safememcpy(copied_buffer[bufnum]+40, video_buffer, sizeof(video_buffer));
}

unsigned video_memory_read(unsigned bufnum, unsigned index)
{
  return copied_buffer[bufnum][index];
}
