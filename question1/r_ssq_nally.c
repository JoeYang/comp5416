/* program r_ssq_n.c */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <limits.h>

#define NEW(type) (type *) malloc(sizeof(type))
#define ARRIVAL 1
#define DEPARTURE 2

#define NUM_HOSTS 10         /* Number of hosts connected to the LAN */
#define R_CAPACITY 10        /* gateway processing capacity */
#define MEAN_PKT_LENGTH 1000 /* mean packet length */

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
buffer_size,       /* buffer size to hold packets */		
total_events,      /* number of events to be simulated */
iar;

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
	int i;
	sim_init();
	
	
	while (narr < total_events){
		switch (act()){
			case ARRIVAL:
				/* let each of the hosts arrive independently */	
				for (i = 0; i < NUM_HOSTS; ++i) { 
					arrival();
				}
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
	printf("Minimum buffer size: %d bytes \n", buffer_size);
	
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
	double service_time = (pkt_length*8.0)/(R_CAPACITY*pow(10.0,6.0));
	return (service_time);
}

/**************************************************************************/
void arrival() /* a customer arrives */
{
	struct packet_info *t, *x;
	narr += 1;                /* keep tally of number of arrivals */
	q_sum += q;
	schedule(negexp(iat), ARRIVAL); /* schedule the next arrival */
	
	int new_pkt_len = (int)(-log(drand48()) * MEAN_PKT_LENGTH); 
	double utilisation = (q_len * 8.0)/(iar * pow(10.0, 6.0));
	//printf("Utilisation: %d %f\n", q_len, utilisation);
	if (utilisation <= 0.9) {
		/* If loss probability when packet is dropped is no more than 0.001, drop the packet */
		if ((((float) (nloss + 1)) / narr) <= 0.001) {
			nloss++;
		}
		else {
			q += 1;
			q_len += new_pkt_len;
			if (q_len > buffer_size) {
				buffer_size = q_len;
			}
			t = NEW(PACKET_Q);
			for(x=headPkt ; x->next!=tailPkt ; x=x->next);
			t->pkt_len = new_pkt_len;
			t->next = x->next;
			x->next = t;
			if (q == 1)
				schedule(srv_time(headPkt->next->pkt_len), DEPARTURE);
		}
	}
	else {
		/* link is more than 90% utilised; packet is dropped */
		nloss++;
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
	printf("\nenter the mean packet arrival rate (pkts/sec)\n");
	scanf("%d", &iar);
	//printf("\nenter the gateway capacity (Mbps)\n");
	//scanf("%d", &r_capacity);
	//printf("\nenter the max buffer size (KB)\n");
	//scanf("%d", &buffer_size);
	printf("enter the total number of packets to be simulated\n");
	scanf("%d", &total_events);
	
	/* providing automated seed from system time */
	seed = time(NULL);
	srand48(seed);
	
	head = NEW(EVENTLIST);
	tail = NEW(EVENTLIST);
	head->next = tail;
	tail->next = tail;
	
	headPkt = NEW(PACKET_Q);
	tailPkt = NEW(PACKET_Q);
	headPkt->next = tailPkt;
	tailPkt->next = tailPkt;
	
	gmt = 0.0;
	q = 0;
	narr = 0;
	nloss = 0;
	q_sum = 0;
	q_len = 0;
	//buffer_size *= 1024;     /* converts size from MB to B (i.e. octets) */
	iat = 1.0 / iar;
	schedule(negexp(iat), ARRIVAL); /* schedule the first arrival */
}
/**************************************************************************/
