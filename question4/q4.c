#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <limits.h>

#define NEW(type) (type *) malloc(sizeof(type))
#define ARRIVAL 1
#define DEPARTURE 2

#define G_CAPACITY 10        /* gateway processing capacity */
#define MEAN_PKT_LENGTH 1000 /* mean packet length */
#define BAR 10               /* batch arrival rate in batches/second per host */
#define BATCH_HOSTS 1        /* number of hosts that have batch arrivals */
#define NUM_HOSTS 10         /* number of hosts */
#define BUFFER_SIZE 41984    /* max buffer size to hold packets */
#define TOTAL_EVENTS 10000   /* number of events to be simulated */
#define R (BUFFER_SIZE * 0.75) /* threshold parameter which triggers discard of packet */
#define Z 0.95

double 
gmt,        /* absolute time */
iat,        /* mean interarrival time */
W,          /* weight ratio for each host */
batch_time, /* time that batched packets spent in the system */
packet_time; /* time that individual packets spent in the system */


int
q,             /* number of packets in the system */
narr,          /* number of arrivals */
nloss,         /* number of lost single arrival packets */
batch_nloss,   /* number of lost batch arrival packets */
q_sum,         /* sum of queue lengths at arrival instants */ 
q_len,         /* queue length n octets */ 
iar,           /* mean packet arrival rate */
batch_size,    /* number of packets in each batch arrival */     
num_packets,   /* the number of packets to add to the buffer in an arrrival */
batch_qlen,    /* sum of the batch packet lengths in q_len */
total_packets, /* total number of packets that passed through the system */
batch_packets, /* number of batch packets that passed through the system */
batch_arrival, /* keeps track of whether an arrival is a batch or single arrival */
batch_interval;/* keeps track of when the next batch process should be scheduled */

long
seed;   /* seed for the random number generator */

typedef struct schedule_info{
	double time;                       /* Time that event occurs */
	int event_type;                    /* Type of event */
	struct schedule_info *next;        /* Pointer to next item in linked list */
} EVENTLIST;

typedef struct packet_info{
	int pkt_len;                       /* Length of packet */ 
	int batch;						   /* keeps track of whether a particular packet is part of a batch arrival */
	double arrival_time;			   /* time that the packet arrived in the system */
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
	
	
	while (narr < TOTAL_EVENTS){
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
	
			//printf("Probablity a packet is blocked - batch arrival: %8.4f single packet arrival: %8.4f\n",
	 //((float) batch_nloss) / total_packets, ((float) nloss) / total_packets, total_time / total_packets);
	//printf("Mean packet delay in the gateway was: %f\n", total_time / total_packets);
	
	printf("%d, %d, %d %d\n", batch_nloss, nloss, total_packets, batch_packets);
	printf("%f,%f,%f,%f\n", ((float) batch_nloss) / batch_packets, ((float) nloss) / (total_packets - batch_packets), ((float)batch_time / batch_packets), ((float) packet_time) / (total_packets - batch_packets));
	
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
	double service_time = (pkt_length*8.0)/(G_CAPACITY*pow(10.0,6.0));
	return (service_time);
}

/**************************************************************************/
void arrival() /* a customer arrives */

{
	int i;
	struct packet_info *t, *x;
	narr += 1;                /* keep tally of number of arrivals */
	q_sum += q;
	schedule(negexp(iat), ARRIVAL); /* schedule the next arrival */
	
	/* check whether to schedule a batch or individual packet arrival */
	if (batch_interval == 1 || narr % batch_interval == 0) {
		num_packets = batch_size;
		batch_arrival = 1;
		batch_packets += num_packets;
	}
	else {
		num_packets = 1;
		batch_arrival = 0;
	}
	
	/* Create the appropriate number of packets for the type of arrival */
	for (i = 0; i < num_packets; ++i) { 
		total_packets += 1;
		int new_pkt_len = (int)(-log(drand48()) * MEAN_PKT_LENGTH); 
		
		if (q_len == 0) {
			W = 0;
		}
		else {
			if (batch_arrival) {
				W = (batch_qlen * NUM_HOSTS)/ ((float) q_len);
			}
			else {
				W = (((q_len - batch_qlen) * NUM_HOSTS) / ((float) q_len)) / (((float) NUM_HOSTS) - BATCH_HOSTS);
			}
		}
		
		if (!(q_len > R && W > (Z * ((BUFFER_SIZE - R)/(q_len - R))))) {
			/* still space in buffer */
			q += 1;
			q_len += new_pkt_len;
			t = NEW(PACKET_Q);
			for(x=headPkt ; x->next!=tailPkt ; x=x->next);
			t->pkt_len = new_pkt_len;
			t->arrival_time = gmt;
			t->next = x->next;
			x->next = t;
			
			if (batch_arrival) {
				batch_qlen += new_pkt_len;
				t->batch = 1;
			}
			
			if (q == 1)
				schedule(srv_time(headPkt->next->pkt_len), DEPARTURE);
		}
		else {
			/* buffer is full; packet is dropped */
			if (batch_arrival) {
				batch_nloss += 1;
			}
			else {
				nloss += 1;
			}
		}
	}
}

/**************************************************************************/
void departure()  /* a customer departs */

{
	struct packet_info *x;
	q -= 1;
	x = headPkt->next;                 /* Delete event from linked list */
	q_len -= x->pkt_len;
	
	if (x->batch) {
		batch_qlen -= x->pkt_len;
		batch_time += gmt - x->arrival_time;
	}
	else {
		packet_time += gmt - x->arrival_time;
	}
	
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
	
    //printf("\nenter the mean packet arrival rate (pkts/sec)\n");
	scanf("%d", &iar);
	
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
	batch_nloss = 0;
	q_sum = 0;
	q_len = 0;
	W = 0;
	batch_qlen = 0;
	batch_time = 0;
	packet_time = 0;
	batch_packets = 0;
	total_packets = 0;
	batch_size = iar/BAR; 
	iar = (BATCH_HOSTS * BAR) + ((NUM_HOSTS - BATCH_HOSTS) * iar);
	batch_interval = iar / (BAR * BATCH_HOSTS);
	iat = 1.0 / iar;
	schedule(negexp(iat), ARRIVAL); /* schedule the first arrival */
}
/**************************************************************************/