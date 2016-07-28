#define SIZE 26*80

unsigned buffer[SIZE] = {0, };

void shmem_write(unsigned index, unsigned data)
{
  if (index < SIZE)
  {
    buffer[index] = data;
  }
}

unsigned shmem_read(unsigned index)
{
  if (index < SIZE)
  {
    return buffer[index];
  }
  else
  {
    return 0;
  }
}
