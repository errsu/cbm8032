// beware: only 8 bits of the data are written/read

extern void video_memory_init();
extern void video_memory_write(unsigned index, unsigned data);
extern void video_memory_copy_to(unsigned bufnum);
extern unsigned video_memory_read(unsigned bufnum, unsigned index);
