#include <unistd.h>
#include <stdio.h>

int main(int argc, char *argv[])
{ int start, dest, v;
  if (argc == 4)
  {
    start = atoi(argv[2]);
    dest = atoi(argv[3]);
    printf("Calling system call issue_request ");
    v = syscall(352, argv[1][0], start, dest);
    if (v == 1)
      printf("[!Error adding user]");
    printf(".\nAdding %c start=%d dest=%d\n",argv[1][0], start, dest);
  } else
  {
    printf("invalid parameters\nUse waiter 'T' start dest\n");
    printf("Where 'T' = {'L' = Passenger with Luggage, 'A' = Adult, 'C' = Child\n");
    printf("And start and dest are integers between 1 and 5.\n");
    printf("Example:  waiter 'A' 2 5\n\n");
  }
}
