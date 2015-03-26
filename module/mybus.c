#ifndef MYBUS_C
#define MYBUS_C

/**** /proc/mybus
** Project 2c - main module for the airport bus system
*  Team : Rick Tilley, Alexander Morehouse
*  Date : 11/5/2013
*
*/
#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/seq_file.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/time.h>
#include <linux/linkage.h>
#include <linux/seqlock.h>
#include <asm/uaccess.h>
#include "busdepot.h"

//MODULE_LICENSE("GPL");
MODULE_LICENSE("Dual BSD/GPL");


static struct Stations airport;  // global repository
static int mybus_open(struct inode *inode, struct file *file);
int mybus_show(struct seq_file *m, void *v);
void simulaterun(void);

extern int (*STUB_start)(void);
extern int (*STUB_request)(char ptype, int start, int dest);
extern int (*STUB_stop)(void);
int yourstartshuttle(void);
int yourissuerequest(char ptype, int start, int dest);
int yourstopshuttle(void);

struct task_struct *task;
int shuttleloop(void *data);
void picknextstate(void);
void busmoves(void);
int timecycle;  // keep track of the timecycle for testing purposes
static struct mutex lock;

static struct proc_dir_entry *proc_entry;

struct file_operations proc_fops = {
.owner = THIS_MODULE,
.open = mybus_open,
.read = seq_read,
.llseek = seq_lseek,
.release = single_release,
};

static int gime_init(void)
{
  int ret = 0; timecycle = 0;
  proc_entry = proc_create("mybus",0, NULL, &proc_fops);
  if (proc_entry == NULL)
  { ret = -ENOMEM;
    printk(KERN_INFO "mybus: Couldn't create proc entry\n");
  }
  else
  { // proc_entry->read_proc = xtime_read;
    printk(KERN_INFO "mybus: Module loaded.\n");
  }
  task = NULL;
  mutex_init(&lock);
  businit(&airport);
/* testing start early
yourstartshuttle();
// [/test] */
  STUB_start = yourstartshuttle;
  STUB_request = yourissuerequest;
  STUB_stop = yourstopshuttle;
  return ret;
}

static void gime_exit(void)
{
  int ret;
  mutex_lock(&lock);
  if (task != NULL)
  {  // critical read
    if (airport.thread == 1)
      ret = kthread_stop(task);
  }
  STUB_start = NULL;
  STUB_request = NULL;
  STUB_stop = NULL;
  remove_proc_entry("mybus", NULL);
// memory free's cause seg faults, why?
  buscleanall(&airport);
  printk(KERN_INFO "Unloaded mybus module /proc/mybus\n");
  mutex_unlock(&lock);
}

module_init(gime_init);
module_exit(gime_exit);

static int mybus_open(struct inode *inode, struct file *file)
{ int ret;
  mutex_lock(&lock);
  ret = single_open(file, mybus_show, NULL);
  mutex_unlock(&lock);
  return ret;
}
 
int mybus_show(struct seq_file *m, void *v)
{
  char status[64];
  int seats, seat;
  int a,b,c,t, misc;
  struct Bus* bus = &airport.bus;
  struct Person *ptest;  // testing
  if ( (*bus).status1.status == OUTOFSERVICE )
  { 
    if ((*bus).seats != 50)
      strcpy(status,"Deactivating");
    else
      strcpy(status,"Offline");
  }
  else if ( (*bus).status1.action == MOVING )
    strcpy(status,"Moving");
  else if ( (*bus).status1.action == WAITING )
    strcpy(status,"Parked");
  else
    strcpy(status,"Undefined!");
  seats = 50 - (*bus).seats;
  seat = seats / 2;
    
//[test] print out something from busrepot.h struct
  seq_printf(m, "Status:\t%s\n",status);
  seq_printf(m, "Seats:\t%d", seat);
  if ( (seat*2)+1 == seats )
    seq_printf(m, ".5");
  seats = (*bus).seats;
  seat = seats / 2;
  seq_printf(m, " used, %d", seat);
  if ( (seat*2)+1 == seats )
    seq_printf(m, ".5");
  seq_printf(m," available\n");
//  seq_printf(m, "Passengers:\t%d (%d adult with luggage, %d adult without luggage, %d children)\n",airport);
  seq_printf(m, "Location:\t%d\n",(*bus).status1.location+1);
  seq_printf(m, "Destination:\t%d\n",(*bus).status2.location+1);
  seq_printf(m, "Delivered:\t%d (%d adult with luggage, %d adult without luggage, %d children)\n",airport.delivered_al+airport.delivered_a+airport.delivered_c, airport.delivered_al, airport.delivered_a, airport.delivered_c);
  for (misc = 1; misc <= NUMTERMINALS; misc++)
  {
    twaitstat(&airport.terminal[misc-1],&a,&b,&c,&t);
    seq_printf(m, "Terminal %d:%d adult with luggage, %d adult without luggage, %d children in queue.  %d passengers delivered\n",misc,a,b,c,t);
  } 

//[testing]
//seq_printf(m,"Timecycle: %d\n",timecycle);
/*
  seq_printf(m, "\t");
  for (misc = 0; misc < 5; misc++)
    seq_printf(m,"T - %d\t",misc);
  seq_printf(m, "\n");

  seq_printf(m, "pstart: ");
  for (misc = 0; misc < 5; misc++)
    seq_printf(m,"%d\t",airport.terminal[misc].pstart);
  seq_printf(m, "\n");

  seq_printf(m, "pend--: ");
  for (misc = 0; misc < 5; misc++)
    seq_printf(m,"%d\t",airport.terminal[misc].pend);
  seq_printf(m, "\n");

  seq_printf(m, "waiters ");
  for (misc = 0; misc < 5; misc++)
    seq_printf(m,"%d\t",airport.terminal[misc].waiters);
  seq_printf(m, "\n");

  seq_printf(m, "queue0 :");
  for (misc = 0; misc < 5; misc++)
  {
    if (airport.terminal[misc].populus != NULL)
      seq_printf(m,"%d\t",1 + (int) airport.terminal[misc].populus[airport.terminal[misc].pstart].weight);
    else seq_printf(m,"-\t");
  }
  seq_printf(m, "\n");
  seq_printf(m, "queue1 :");
  for (misc = 0; misc < 5; misc++)
  {
    if (airport.terminal[misc].waiters > 1)
      seq_printf(m,"%d\t",1 + (int) airport.terminal[misc].populus[airport.terminal[misc].pstart+1].weight);
    else seq_printf(m,"-\t");
  }
  seq_printf(m, "\n");

  for (misc = 0; misc < 50; misc++)
  { ptest = &(*bus).passenger[misc];
    seq_printf(m,"[%d:%d,%d,w(%d)]  ",airport.bus.pbitmap[misc],(*ptest).origin, (*ptest).dest, 1+ (int) (*ptest).weight); 
   }
*/
//[/testing]
  return 0;
}

void picknextstate(void)
{ int total, need, fatty, misc, unloaded, loaded, myloc, done;
  struct Bus *b = &airport.bus;
  struct Terminal *t;
  busmoves();
  // drop off?
  myloc = (*b).status1.location;
  t = &airport.terminal[myloc];
  total = 0; unloaded = 0; loaded = 0;
  if ((*t).seekers > 0)
  {
    for (misc = 0; misc < 50; misc++)
    {
      if ((*b).pbitmap[misc] != -1)
      {
        if ((*b).passenger[misc].dest == myloc)
        {
          ++unloaded;
          --(*t).seekers;
          ++(*t).delivered;
          switch ( (*b).passenger[misc].weight ) {
            case CHILD: ++airport.delivered_c; ++(*b).seats; break;
            case ADULT: ++airport.delivered_a; (*b).seats += 2; break;
            case LUGGAGE: ++airport.delivered_al; (*b).seats += 4; break;
            case TIMMY: break;
          }
          (*b).pbitmap[misc] = -1; --(*b).pcnt;
          (*b).passenger[misc].weight = CHILD;
          (*b).passenger[misc].origin = 5;
          (*b).passenger[misc].dest = 5;
        }
      }
    }
  }
  // pick up?
  done = 0;
  if ((*t).waiters > 0 && (*b).status1.status == INSERVICE 
       && (*b).seats > 0)
  { 
    while (done == 0 && (*b).seats > 0)
    {  fatty = 1 + (int) (*t).populus[(*t).pstart].weight;
       if ((*b).seats-fatty >= 0)
       {  for (misc = 0; (*b).pbitmap[misc] != -1; misc++)
          {;}
          cloadwaiter(&(*b).passenger[misc],&(*t).populus[(*t).pstart++]);
          (*b).seats -= fatty;  ++(*b).pcnt;
          (*b).pbitmap[misc] = (int) (*b).passenger[misc].weight;
          if ((*t).pstart >= (*t).psize)
            (*t).pstart = 0;
          --(*t).waiters;  ++loaded;
          if ((*t).waiters == 0)
          { // emptied queue
            (*t).pstart = -1;  (*t).pend = -1; 
            done = -1;
          }
       }
       else
       {  // does fatty lose his place in line?
         need = 0;
         misc = (*t).pstart+1;
         if (misc >= (*t).psize && misc != (*t).pend+1) misc = 0;
         for (; misc != (*t).pend+1; misc++)
         {  if (misc >= (*t).psize) misc = 0;
 	    if (fatty > (1+ (int) (*t).populus[misc].weight))
            {  swapwaiter(&(*t).populus[(*t).pstart], &(*t).populus[misc]);
               misc = (*t).pend;  // took off +1 because of loop increment
               need = 8;
            }          
         }
         if (need == 0);
           done = -1;
       }
    }
    // account for time taken pickup/dropoff
    total = loaded + unloaded;
    if (total > 0)
    {
      (*b).status1.timetillchange += 10;
      total = total - 4;
      if (total > 0)
      {  (*b).status1.timetillchange += (3 * total);
      }
      (*b).status1.action = WAITING;
    }
  }

  if ((*b).status1.action == MOVING)
  {
    // turn around?
    need = 0;
    for (misc = (*b).status1.location + (*b).d; (misc >= 0) && (misc < 5) ; misc = misc + (*b).d)
    { need = need + airport.terminal[misc].waiters + airport.terminal[misc].seekers;
    }
    if (need == 0)  // turns around at last sight of humanity
    {
      (*b).d = (*b).d * -1;
      // look to see if everything's empty
      need = 0;
      for (misc = (*b).status1.location + (*b).d; (misc >= 0) && (misc < 5) ; misc = misc + (*b).d)
       need = need + airport.terminal[misc].waiters + airport.terminal[misc].seekers;
      if (need == 0)  // no humanity to be found
      { (*b).status1.action = WAITING;
        (*b).status1.timetillchange = 10;
        (*b).status2.location = (*b).status1.location - (*b).d;
      }
    }
    (*b).status2.location += (*b).d;
    if ((*b).status2.location < 0 || (*b).status2.location > 4)
    { // confused nothing to do, error
      (*b).status2.location -= (*b).d;
      (*b).status1.action = WAITING;
      (*b).status1.timetillchange = 10;
    }
    else 
      (*b).status1.timetillchange = 30;
  }
}


void busmoves(void)
{
  struct Bus *b = &airport.bus;
  (*b).status1.action = (*b).status2.action;
  (*b).status1.status = (*b).status2.status;
  (*b).status1.location = (*b).status2.location;
  (*b).status1.timetillchange = (*b).status2.timetillchange;
  (*b).status2.action = MOVING;
  (*b).status2.timetillchange = 0;
}

int shuttleloop(void *data)
{
  int var;
  var = 50;
//  set_current_state(TASK_INTERRUPTIBLE);
  while (!kthread_should_stop() && !(airport.bus.status1.status == OUTOFSERVICE && airport.bus.seats == 50))
  { 
    ++timecycle;
    mutex_lock(&lock);
    picknextstate();
    mutex_unlock(&lock);
    // sleep till next state
    set_current_state(TASK_INTERRUPTIBLE);
    msleep(10 * airport.bus.status1.timetillchange);
    set_current_state(TASK_RUNNING);
/* [testing] populates waiters in leu of using a system call
   if (var % 3 == 0)
     simulaterun();
// [/testing]  */
  }
  airport.thread = 0;
  airport.bus.status1.action = WAITING;
//  printk(KERN_INFO "Mybus: exiting shuttleloop thread\n");
  return var;
}

// system call
int yourstartshuttle(void)
{ int data = 20;
// or could we stably go from shutting down to INSERVICE??
  if (airport.thread == 0)
 {
  simulaterun();
  airport.thread = 1;
  airport.bus.status1.status = INSERVICE;
  airport.bus.status2.status = INSERVICE;
  task = kthread_run(&shuttleloop,(void *)data,"busninja");
//  printk(KERN_INFO "Mybus added Kernel Thread : %s\n",(*task).comm); 
 }
  else return 1;
 return 0; 
}

// system call
int yourissuerequest(char ptype, int start, int dest)
{ int ret = 0;
  enum Person_Type p;
  struct Person pson;
  mutex_lock(&lock);
  switch (ptype) {
	case 'L' : p = LUGGAGE; break;
	case 'A' : p = ADULT; break;
	case 'C' : p = CHILD; break;
	default : p = TIMMY; ret = 1;
  }
  // add to waiting queue
  if (start > 0 && start <= 5 && dest > 0 && dest <= 5 && start != dest && ret == 0)
  { --start; --dest;
    pson.origin = start;
    pson.dest = dest;
    pson.weight = p;
//[testing]  printk(KERN_INFO "mybus: wants to insert type:%d, from %d to %d\n", p, start, dest);
    ret = addwaiter(&airport.terminal[start], &pson);
    if (ret == 0)
      ++airport.terminal[dest].seekers;
  }
  else
    ret =  1;
  mutex_unlock(&lock);
  return ret;
}

void simulaterun(void)
{
  yourissuerequest('A',3,2);
  yourissuerequest('C',4,5);
  yourissuerequest('L',5,1);
  yourissuerequest('L',1,5);
  yourissuerequest('A',1,4);
  yourissuerequest('C',1,3);
  yourissuerequest('C',2,5);
  yourissuerequest('A',3,4);
  yourissuerequest('C',5,3);
}

// system call
int yourstopshuttle(void)
{ 
  mutex_lock(&lock);
  airport.bus.status2.status = OUTOFSERVICE;
  mutex_unlock(&lock);
  return 0;
}

/** busdepot.h implementation here **
*****
*****  busdepot.c merged into mybus.c!   ***
***
*/
// lets not use double pointers this time.. 
// struct Person isn't going to overflow memory anyways
// simple static copy
void swapwaiter( struct Person *p, struct Person *j)
{ struct Person t;
  t.origin = (*j).origin;
  t.dest = (*j).dest;
  t.weight = (*j).weight;
  (*j).origin = (*p).origin;
  (*j).dest = (*p).dest;
  (*j).weight = (*p).weight;
  (*p).origin = t.origin;
  (*p).dest = t.dest;
  (*p).weight = t.weight;
}
void cloadwaiter( struct Person *p, struct Person *j)
{
  (*p).origin = (*j).origin;
  (*p).dest = (*j).dest;
  (*p).weight = (*j).weight;
}

int addwaiter(struct Terminal *t, struct Person *p)
{ int ret = 0;
  if ( (*t).populus == NULL)
  {
    ret = grow_populus( &(*t).populus, &(*t).psize, 5000,&(*t).pstart, &(*t).pend);
    if (ret == 0) ++(*t).pend;
  }
  else
 {
  if (++(*t).pend > (*t).psize-1)
    (*t).pend = 0;
  if ((*t).pend == (*t).pstart)
  {  --(*t).pend;  if ((*t).pend < 0) (*t).pend = (*t).psize-1;
// do not allow more than 5000 waiters per terminal
ret = 1;  // memory problems are prohibitive
//     ret = grow_populus( &(*t).populus, &(*t).psize, (*t).psize * 2, &(*t).pstart, &(*t).pend);
  }
 }
  // allocation done, now add it
  if (ret == 0)
  {
    cloadwaiter(&(*t).populus[(*t).pend], p);
    if ((*t).pstart == -1) 
    { (*t).pend = 0;
      (*t).pstart = 0;
    }
    ++(*t).waiters;
  }
  else if (!((*t).pstart == -1 && (*t).pend == -1))
  { // failed to allocate!
    --(*t).pend;
    if (--(*t).pend < 0)
      (*t).pend = (*t).psize-1;
    if ((*t).pstart == -1)
      (*t).pend = -1;
  }
return ret;
}

// pass your pointers by reference
//      to this function's double pointer
int grow_populus(struct Person **p, int *oldsize, int size, int *pstart, int *pend)
{ struct Person * temp;
  int misc, waiters;
  if (size < 1)
    *p = NULL;
  else
  {  if (*p != NULL)
     { temp = (struct Person*) kmalloc(sizeof(struct Person*)*size, GFP_KERNEL);
      if (temp != NULL)
      {  
       if (*pend >= *pstart)
         waiters = *pend - *pstart + 1;
       else
         waiters = (*oldsize)-(*pstart)+(*pend)+1;
       if (size > *oldsize)
       {
         for (misc = 0; misc < waiters; misc++)
         { 
           cloadwaiter(&temp[misc], &((*p)[(*pstart)++]) );
	   if (*pstart > *oldsize - 1)
             *pstart = 0;
         }
         *pstart = 0; *pend = waiters-1;
  //       kfree(p);  // check for segfaults
         *p = temp;
       }
//       else
  //       kfree(temp);  // we don't do shrinks
      }
      else 
        return 1;
    }
     else
     {
       *p = (struct Person*) kmalloc(sizeof(struct Person*)*size, GFP_KERNEL);
       if (p == NULL)
       {  // oh damn
         return 1;
       }
     }
  }
  *oldsize = size;
  return 0;
}

// pass by reference
// call using cleanall(&myterm);
void buscleanall(struct Stations *t)
{ int misc;
  // free up your memory
  for (misc = 0; misc < 5; misc++)
  {
//    if ((*t).terminal[misc].populus != NULL)
//      kfree((*t).terminal[misc].populus);
  }
}

void businit(struct Stations *t)
{ int misc;
  struct Terminal * tfocus;
  (*t).thread = 0;
  (*t).delivered_c = 0;
  (*t).delivered_a = 0;
  (*t).delivered_al = 0;
  // init terminals
  for (misc = 0; misc < NUMTERMINALS; misc++)
  {
    tfocus = &(*t).terminal[misc];
    grow_populus( &(*tfocus).populus,&(*tfocus).psize,0,&(*tfocus).pstart, &(*tfocus).pend);
 //   (*tfocus).psize = 0; 
    (*tfocus).pstart = -1;
    (*tfocus).pend = -1;
    (*tfocus).waiters = 0;
    (*tfocus).seekers = 0;
    (*tfocus).delivered = 0;
  }
  (*t).bus.d = 1;
  (*t).bus.pcnt = 0;
  (*t).bus.seats = 50;
  (*t).bus.status1.location = 2;
  (*t).bus.status1.action = WAITING;
  (*t).bus.status1.status = OUTOFSERVICE;
  (*t).bus.status1.timetillchange = 0;
  (*t).bus.status2.location = 2;
  (*t).bus.status2.action = WAITING;
  (*t).bus.status2.status = OUTOFSERVICE;
  (*t).bus.status2.timetillchange = 0;
  
  for (misc = 0; misc < 50; misc++)
    (*t).bus.pbitmap[misc] = -1;
}

void incstat(enum Person_Type typ, int *c, int *a, int *al)
{
       switch (typ) {
       case CHILD: ++(*c); break;
       case ADULT: ++(*a); break;
       case LUGGAGE: ++(*al); break;
       case TIMMY: break;
       }

}

void twaitstat(struct Terminal *t, int *al, int *a, int *c, int *total)
{ int misc;  
  int pend, pstart;
  *c = 0; *a = 0; *al = 0; *total = 0;
  
if ( (*t).populus != NULL && (*t).pstart != -1 && (*t).pend != -1)
{
  pstart = (*t).pstart; pend = (*t).pend;
  if (pend >= pstart)
  {
     for (misc = pstart; misc <= pend; misc++)
     {
       incstat( (*t).populus[misc].weight, c, a, al);
     }     
  }
  else
  {
    for (misc = pstart; misc < (*t).psize; misc++)
    { 
       incstat( (*t).populus[misc].weight, c, a, al);
    }
    for (misc = 0; misc <= pend; misc++)
    {  
       incstat( (*t).populus[misc].weight, c, a, al);
    }
  }
}
 *total = (*t).delivered;
}

/** end busdepot.h implementation *****
**/

#endif
