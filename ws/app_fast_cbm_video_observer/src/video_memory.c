#include <safestring.h>
#include "video_memory.h"

unsigned char video_buffer[40*51];  // 2000 bytes of video data and 40 bytes of flags (with graphic)
unsigned char copied_buffer[VIDEO_BUF_COPIES][40*52]; // 40 bytes of zeroes, followed by video_buffer

void video_memory_init()
{
  safememset(video_buffer, 0, sizeof(video_buffer));
  for (unsigned buf_num = 0; buf_num < VIDEO_BUF_COPIES; buf_num++)
  {
    safememset(copied_buffer[buf_num], 0, 40); // first 40 bytes must be zero
  }
}

void video_memory_write(unsigned index, unsigned data)
{
  video_buffer[index] = (unsigned char)data;
}

void video_memory_copy_to(unsigned buf_num)
{
  // first 40 bytes are left alone
  safememcpy(copied_buffer[buf_num]+40, video_buffer, sizeof(video_buffer));
}

unsigned video_memory_read_from_copy(unsigned buf_num, unsigned index)
{
  return copied_buffer[buf_num][index];
}
