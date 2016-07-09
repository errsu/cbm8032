#define SIZE 2048

unsigned buffer[SIZE] = {0, };

void shmem_write(unsigned index, unsigned data)
{
  buffer[index % SIZE] = data;
}

unsigned shmem_read(unsigned index)
{
  return buffer[index % SIZE];
}
