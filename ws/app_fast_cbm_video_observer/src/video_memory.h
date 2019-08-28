// beware: only 8 bits of the data are written/read

#define VIDEO_BUF_COPIES 4
extern void video_memory_init();
extern void video_memory_write_data(unsigned index, unsigned data);
extern void video_memory_write_graphic(unsigned graphic);
extern void video_memory_copy_line_to(unsigned buf_num, unsigned line);
extern void video_memory_copy_flags_to(unsigned buf_num);
extern unsigned video_memory_read_from_copy(unsigned buf_num, unsigned index);
