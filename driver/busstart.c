#include <unistd.h>
#include <stdio.h>

int main(void)
{
  printf("Calling system call start_shuttle ");
  syscall(351);
  printf(".\nBus system is going, check /proc/mybus\n");
}
