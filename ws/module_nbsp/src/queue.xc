/**
 * Copyright: 2018, Ableton AG, Berlin. All rights reserved.
 */

#include "queue.h"
#include "util.h"

void queue_init(t_queue& queue, unsigned * movable buffer, unsigned buffer_size)
{
  // buffer size must be a power of 2 greater than 1
  queue.read_index  = 0;
  queue.write_index = 0;
  queue.buffer = move(buffer);
  queue.mask = buffer_size - 1;
}

void queue_drop(t_queue& queue)
{
  queue.read_index  = 0;
  queue.write_index = 0;
}

unsigned queue_in(t_queue& queue, unsigned data)
{
  unsigned next_write_index = (queue.write_index + 1) & queue.mask;

  if (next_write_index != queue.read_index)
  {
    queue.buffer[queue.write_index] = data;
    queue.write_index = next_write_index;
    return 1;
  }
  else
  {
    return 0;
  }
}

unsigned queue_out(t_queue& queue)
{
  if (queue.read_index != queue.write_index)
  {
    unsigned data = queue.buffer[queue.read_index];
    queue.read_index = (queue.read_index + 1) & queue.mask;
    return data;
  }
  else
  {
    return NONE;
  }
}

unsigned queue_out_or_default(t_queue& queue, unsigned default_data)
{
  unsigned data = queue_out(queue);
  return data == NONE ? default_data : data;
}

unsigned queue_count(t_queue& queue)
{
  return (queue.write_index + queue.mask + 1 - queue.read_index) & queue.mask;
}

unsigned queue_room(t_queue& queue)
{
  return (queue.read_index - queue.write_index - 1) & queue.mask;
}

unsigned queue_empty(t_queue& queue)
{
  return queue.write_index == queue.read_index;
}

unsigned queue_full(t_queue& queue)
{
  return ((queue.write_index + 1) & queue.mask) == queue.read_index;
}
