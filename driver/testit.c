#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>

void addpeople();
void snooze();

int main(void)
{ int misc, v;
  srand(time(NULL));
  for (misc = 0; misc < 50; misc++)
  {
    addpeople();
  } 
  v = syscall(351);
  snooze();
  for (misc = 0; misc < 50; misc++)
  {
   for (v = 0; v < 100; v++)
    addpeople();
   snooze();
//    usleep(1000);
  }
  v = syscall(353);
  printf("goodbye!\n");
  return 0;
}

void addpeople()
{
  int start = 0, dest = 0, v;
  char c;
 while (start == dest)
{
  start = rand() % 5;
  dest = rand() % 5;
}
start++; dest++;
  v = rand() % 3;
  switch (v) {
    case 0 : c = 'C'; break;
    case 1 : c = 'A'; break;
    case 2 : c = 'L'; break;
    default: c = 'T'; break;
  }
  v = syscall(352, c, start, dest);
    if (v == 1)
      printf("[!Error adding user]");
  printf(".\nAdding %c start=%d dest=%d\n",c, start, dest);
}

void snooze()
{
int x,y,m;
  x = 2;
 for (m = 1; m < 5000; m++)
 { y = y * x;
 }
 sleep(1);
}
