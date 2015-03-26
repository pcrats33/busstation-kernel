#ifndef BUSDEPOT_H
#define BUSDEPOT_H
// ** Main data structure for busterminal.c module
// author: Rick Tilley, Alexander Morehouse
#define NUMTERMINALS 5
//const int NUMTERMINALS = 5;
// enums
enum Person_Type { CHILD, ADULT, TIMMY, LUGGAGE } Person_Type;
enum Bus_Action { MOVING, WAITING } Bus_Action;
enum Bus_Status { INSERVICE, OUTOFSERVICE } Bus_Status;
// smaller structs first
struct Person {
  int origin;
  int dest;
  enum Person_Type weight;
} Person;

struct Terminal {
  int waiters;
  int seekers;
  int delivered;
  struct Person * populus;
  int pstart, pend;
  int psize;
} Terminal;

struct Bus_Verb {
  enum Bus_Action action;
  enum Bus_Status status;
  int location;
  long timetillchange;
} Bus_Verb;

struct Bus {
  struct Person passenger[50];
  int pbitmap[50];
  int d;
  int pcnt;
  int seats;
  struct Bus_Verb status1;
  struct Bus_Verb status2;
} Bus;

struct Stations {
  struct Terminal terminal[NUMTERMINALS];
  struct Bus bus;
  int delivered_c, delivered_a, delivered_al;
  int thread;
} Stations;

void swapwaiter( struct Person *p, struct Person *j);
void cloadwaiter( struct Person *p, struct Person *j);
int addwaiter(struct Terminal *t, struct Person *p);
int grow_populus(struct Person **p, int *oldsize, int size, int *pstart, int *pend);
void buscleanall(struct Stations *t);
void businit(struct Stations *t);
void twaitstat(struct Terminal *t, int *al, int *a, int *c, int *total);

#endif
