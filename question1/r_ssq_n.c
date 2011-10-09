/* program r_ssq_n.c */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <limits.h>

#define NEW(type) (type *) malloc(sizeof(type))
#define ARRIVAL 	1
#define DEPARTURE	2
#define NUM_HOSTS	10

/* Event by event simulation of a router queue with finite waiting
   room */


double 
  gmt,    /* absolute time */
  iat;    /* mean interarrival time */

int
  q,       /* number of packets in the system */
  narr,    /* number of arrivals */
  nloss,   /* number of lost packets */
  q_sum,   /* sum of queue lengths at arrival instants */ 
  q_len,   /* queue length n octets */ 
  r_capacity,        /* router processing capacity */
  buffer_size,       /* max buffer size to hold packets */		
  mean_pkt_length,   /* mean packet length */
  total_events;      /* number of events to be simulated */

long
  seed;   /* seed for the random number generator */

typedef struct schedule_info{
  double time;                       /* Time that event occurs */
  int event_type;                    /* Type of event */
  struct schedule_info *next;        /* Pointer to next item in linked list */
  } EVENTLIST;

typedef struct packet_info{
  int pkt_len;                       /* Length of packet */ 			
  struct packet_info *next;          /* Pointer to next packet in linked list */
  } PACKET_Q;

struct schedule_info
  *head,
  *tail;

struct packet_info
  *headPkt,
  *tailPkt;

int  act(void);
double  negexp(double);
double  srv_time(int);
void arrival(void);
void departure(void);
void schedule(double, int);
void sim_init(void);

/**************************************************************************/
main(){

  sim_init();

int i;
  while (narr < total_events){
    switch (act()){
      case ARRIVAL:
        arrival();
        break;
      case DEPARTURE:
        departure();
        break;
      default:
        printf("error in act procedure\n");
        exit(1);
        break;
      } /* end switch */
  }      /* end while */
  printf("The mean queue length seen by arriving customers is: %8.4f\n",
         ((float) q_sum) / narr);
  printf("Probablity a packet is blocked is: %8.4f\n",
         ((float) nloss) / narr);

  return(0);

} /* end main */
/**************************************************************************/
double negexp(double mean) /* returns a negexp rv with mean `mean' */

{
  return (- log(drand48()) * mean);
}

/**************************************************************************/
double srv_time(int pkt_length) /* returns the service time for given length */

{
  double service_time = (pkt_length*8.0)/(r_capacity*pow(10.0,6.0));
  return (service_time);
}

/**************************************************************************/
void arrival() /* a customer arrives */

{
  struct packet_info *t, *x;
  narr += 1;                /* keep tally of number of arrivals */
  q_sum += q;
  schedule(negexp(iat), ARRIVAL); /* schedule the next arrival */

  int new_pkt_len = (int)(-log(drand48()) * mean_pkt_length); 
  double utilisation = (q_len/(iat*1024*128))/10;
  if(utilisation>0.9){
  		nloss += 1;
  	}
  else if ((new_pkt_len+q_len) <= buffer_size) {
  /* still space in buffer */
    q += 1;
    q_len += new_pkt_len;
    t = NEW(PACKET_Q);
    for(x=headPkt ; x->next!=tailPkt ; x=x->next);
    t->pkt_len = new_pkt_len;
    t->next = x->next;
    x->next = t;
    if (q == 1)
      schedule(srv_time(headPkt->next->pkt_len), DEPARTURE);
  }
  else {
  /* buffer is full; packet is dropped */
    nloss += 1;
  }
}

/**************************************************************************/
void departure()  /* a customer departs */

{
  struct packet_info *x;

  q -= 1;
  x = headPkt->next;                 /* Delete event from linked list */
  q_len -= x->pkt_len;
  headPkt->next = headPkt->next->next;
  free(x);
  if (q > 0)
    schedule(srv_time(headPkt->next->pkt_len), DEPARTURE);
}

/**************************************************************************/


/**************************************************************************/
void schedule(double time_interval, int event)  /* Schedules an event of type */
                                             /* 'event' at time 'time_interval' 
                                                 in the future */

{
double 
  event_time;

struct schedule_info
  *x,
  *t;

event_time = gmt + time_interval;
t = NEW(EVENTLIST);
for(x=head ; x->next->time<event_time && x->next!=tail ; x=x->next);
t->time = event_time;
t->event_type = event;
t->next = x->next;
x->next = t;
}
/**************************************************************************/
int act()    /* find the next event  and go to it */
{
int 
  type;
struct schedule_info 
  *x;

gmt = head->next->time; /* step time forward to the next event */
type = head->next->event_type; /*  Record type of this next event */
x = head->next;                 /* Delete event from linked list */
head->next = head->next->next;
free(x);
return type; /* return value is type of the next event */
}
/*************************************************************************/
/**************************************************************************/
void sim_init()
/* initialise the simulation */

{ 
  int iar;

  printf("\nenter the mean packet arrival rate (pkts/sec) and the max buffer size (KB) \n");
  scanf("%d %d", &iar, &buffer_size);
  
  /* providing automated seed from system time */
  seed = time(NULL);
  srand48(seed);
	srand(seed);
  head = NEW(EVENTLIST);
  tail = NEW(EVENTLIST);
  head->next = tail;
  tail->next = tail;

  headPkt = NEW(PACKET_Q);
  tailPkt = NEW(PACKET_Q);
  headPkt->next = tailPkt;
  tailPkt->next = tailPkt;
  
  total_events = rand()%99000 + 1000;
  mean_pkt_length = 1000;
  r_capacity = 10;
  
  gmt = 0.0;
  q = 0;
  narr = 0;
  nloss = 0;
  q_sum = 0;
  q_len = 0;
  buffer_size *= 1024;     /* converts size from MB to B (i.e. octets) */
  iat = 1.0 / iar;
  schedule(negexp(iat), ARRIVAL); /* schedule the first arrival */
}
/**************************************************************************/
