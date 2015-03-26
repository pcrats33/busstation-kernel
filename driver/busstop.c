#include <unistd.h>
#include <stdio.h>

int main(void)
{
  printf("Calling system call stop_shuttle ");
  syscall(353);
  printf(".\nSet the bus to shut down\n");
}
