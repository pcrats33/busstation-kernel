#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/module.h>
#include <linux/export.h>
#include <linux/linkage.h>

//*******************************************************
//*	system call implementations for bus program	*
//*******************************************************
//*	these are all just wrappers tho, so the kernel	*
//*	doesn't need to be compiled every time a change	*
//*	is made						*
//*******************************************************

int (*STUB_start)(void) = NULL;
int (*STUB_request)(char ptype, int iterm, int dterm) = NULL;
int (*STUB_stop)(void) = NULL;
EXPORT_SYMBOL(STUB_start);
EXPORT_SYMBOL(STUB_request);
EXPORT_SYMBOL(STUB_stop);

// starts shuttle service
// returns 0 if successful
// returns 1 if service has already been started
// returns -ENOSYS if module was not loaded
asmlinkage int sys_start_shuttle(void)
{
  if (STUB_start)
    return STUB_start();

  printk(KERN_ALERT "Called start_shuttle and the module was not loaded.\n");
  return -ENOSYS;	// returns ENOSYS if module was not loaded
}

// adds passenger to waiting queue at corresponding terminal
// returns 0 if successful
// returns 1 if request is invalid
// returns -ENOSYS if module was not loaded
asmlinkage int sys_issue_request(char passenger_type, int initial_terminal, int destination_terminal)
{
  if (STUB_request)
    return STUB_request(passenger_type, initial_terminal, destination_terminal);

  printk(KERN_ALERT "Called issue_request and the module was not loaded.\n");
  return -ENOSYS;	// returns ENOSYS if module was not loaded
  
}

// stops shuttle service
// returns 0 if successful
// returns 1 if shuttle is deactivated OR is in process of unloading passengers
// returns -ENOSYS if module was not loaded
asmlinkage int sys_stop_shuttle(void)
{
  if (STUB_stop)
    return STUB_stop();

  printk(KERN_ALERT "Called stop_shuttle and the module was not loaded.\n");
  return -ENOSYS;	// returns ENOSYS if module was not loaded
}
