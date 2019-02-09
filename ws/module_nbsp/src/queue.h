/**
 * Copyright: 2018, Ableton AG, Berlin. All rights reserved.
 */

#ifndef __queue_h__
#define __queue_h__

typedef struct
{
  unsigned read_index;
  unsigned write_index;
  unsigned mask;
  unsigned * movable buffer;
} t_queue;

#define QUEUE_INIT_WITH_BUFFER(queue, size) \
  unsigned queue ## _buffer[size]; \
  unsigned * movable p_ ## queue ## _buffer = &queue ## _buffer[0]; \
  queue_init(queue, move(p_ ## queue ## _buffer), size)

extern void queue_init(t_queue& queue, unsigned * movable buffer, unsigned buffer_size);
extern unsigned queue_in(t_queue& queue, unsigned data);
extern unsigned queue_out(t_queue& queue);
extern unsigned queue_out_or_default(t_queue& queue, unsigned default_data);
extern unsigned queue_count(t_queue& queue);
extern void queue_drop(t_queue& queue);
extern unsigned queue_room(t_queue& queue);
extern unsigned queue_empty(t_queue& queue);
extern unsigned queue_full(t_queue& queue);

#endif
