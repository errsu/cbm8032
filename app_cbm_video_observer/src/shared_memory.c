#define SIZE 2048

unsigned buffer[SIZE];

void write_shared_memory(unsigned index, unsigned data)
{
  buffer[index % SIZE] = data;
}

unsigned read_shared_memory(unsigned index)
{
  return buffer[index % SIZE];
}
