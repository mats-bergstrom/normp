/*                               -*- Mode: C -*- 
 * Copyright (C) 2024, Mats Bergstrom
 * $Id$
 * 
 * File name       : normp.c
 * Description     : norm power
 * 
 * Author          : Mats Bergstrom
 * Created On      : Sun Mar 10 14:18:08 2024
 * 
 * Last Modified By: Mats Bergstrom
 * Last Modified On: Sun Mar 10 17:05:15 2024
 * Update Count    : 32
 */



#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>

#include <mosquitto.h>

#include "cfgf.h"

/* MQTT ID, broker and port */
#define MQTT_ID		"mbrdr"
#define MQTT_BROKER	"127.0.0.1"
#define MQTT_PORT	1883


int opt_v = 0;				/* Verbose printing */
int opt_n = 0;				/* NoActive, do not send mqtt data */

/*****************************************************************************/
/* Config file handling */
/* mqtt   <broker> <port> <id> */

char* mqtt_broker = 0;
int   mqtt_port;
char* mqtt_id = 0;

int
set_mqtt( int argc, const char** argv)
{
    if ( argc != 4 )
	return -1;
    
    if (mqtt_broker)
	free( mqtt_broker );
    mqtt_broker = strdup( argv[1] );
    if ( !mqtt_broker || !*mqtt_broker )
	return -1;

    mqtt_port = atoi(argv[2]);
    if ( (mqtt_port < 1) || (mqtt_port > 65535) )
	return -1;

    if (mqtt_id)
	free( mqtt_id );
    mqtt_id = strdup( argv[3] );
    if ( !mqtt_id || !*mqtt_id )
	return -1;

    if ( opt_v )
	printf("mqtt %s %d %s\n", mqtt_broker, mqtt_port, mqtt_id);
    
    return 0;
}


double calibration_factor = 1.0;

int
set_calibration( int argc, const char** argv )
{
    double X;
    char* p = 0;
    
    if ( argc != 2 )
	return -1;

    errno = 0;
    X = strtod( argv[1], &p );
    if ( errno != 0 )
	return -1;
    if ( *p != '\0' )
	return -1;

    calibration_factor = X;
    printf("calibration factor = %lf\n", calibration_factor);
    return 0;
}


char* topic_P_ut    = 0;		/* Topic for P_ut */
char* topic_P_in    = 0;		/* Topic for P_in */
char* topic_P_sun   = 0;		/* Topic for P_sun */
char* topic_P_norm  = 0;		/* Topic for P_norm */

unsigned topics_missing = 0;		/* if > 0 topic(s) are missing! */

int
set_topics( int argc, const char** argv )
{
    char* s = 0;
    
    if ( argc != 5 )
	return -1;

    s = strdup( argv[1] );
    if ( !s || !*s )
	return -1;
    if ( topic_P_ut )
	free( topic_P_ut );
    topic_P_ut = strdup( argv[1] );
    
    s = strdup( argv[2] );
    if ( !s || !*s )
	return -1;
    if ( topic_P_in )
	free( topic_P_in );
    topic_P_in = strdup( argv[2] );

    s = strdup( argv[3] );
    if ( !s || !*s )
	return -1;
    if ( topic_P_sun )
	free( topic_P_sun );
    topic_P_sun = strdup( argv[3] );

    s = strdup( argv[4] );
    if ( !s || !*s )
	return -1;
    if ( topic_P_norm )
	free( topic_P_norm );
    topic_P_norm = strdup( argv[4] );

    printf("Topic P_ut   \"%s\"\n", topic_P_ut);
    printf("Topic P_in   \"%s\"\n", topic_P_in);
    printf("Topic P_sun  \"%s\"\n", topic_P_sun);
    printf("Topic P_norm \"%s\"\n", topic_P_norm);
    
    return 0;
}



cfgf_tagtab_t tagtab[] = {
			  {"mqtt",		3, set_mqtt },
			  {"calibration",	1, set_calibration },
			  {"topics",		4, set_topics },

			  {0,0,0}
};


/*****************************************************************************/
/* Misc support */

void
my_gettime(struct timespec* ts)
	/* Get current time or bomb */
{
    int i;
    i = clock_gettime(CLOCK_REALTIME, ts);
    if ( i ) {
	perror("clock_gettime: ");
	exit( EXIT_FAILURE );
    }
}

void
add_time_sec(struct timespec* t, const struct timespec* now, unsigned dt)
{
    t->tv_sec  = now->tv_sec + dt;
    t->tv_nsec = now->tv_nsec;
}

void
my_sleep( const struct timespec* ts )
{
    int i;
    i = clock_nanosleep( CLOCK_REALTIME, TIMER_ABSTIME, ts, 0);
    if ( i ) {
	perror("clock_nanosleep: ");
	exit( EXIT_FAILURE );
    }
}

int
is_past_time( struct timespec* reset_ts, unsigned int dt )
{
    struct timespec now_ts;
    my_gettime( &now_ts );
    /* Have we passed the reset time yet? */
    if ( now_ts.tv_sec < reset_ts->tv_sec ) return 0; /* Nope */

    /* Yes, set the next reset time */
    add_time_sec( reset_ts, &now_ts, dt );
    return 1;
}

/*****************************************************************************/
/* normp loop */

double P_in   = 0;
double P_ut   = 0;
double P_sun  = 0;
double P_norm = 0;

#define HAS_IN  ((unsigned)0x01)
#define HAS_UT  ((unsigned)0x02)
#define HAS_SUN ((unsigned)0x04)
unsigned has_mask = 0;

int
normp_loop()
{
    /* Empty for now */
    for(;;) {
	sleep(10);
    }
}



/*****************************************************************************/
/* Mosquitto handling */

/* Global mosquitto handle. */
struct mosquitto* mqc = 0;


#define MAX_TOPIC_LEN		(80)
char topic_val[MAX_TOPIC_LEN];


void
mq_message_callback(struct mosquitto *mqc, void *obj,
		    const struct mosquitto_message *msg)
{
    
    const char*    topic = msg->topic;
    const char*    pload = msg->payload;

    int calc = 0;

    if ( opt_v )
	printf("data: %s = %s\n", topic, pload);
    
    if ( !strcmp(topic,topic_P_ut) ) {
	double X;
	char* p = 0;
	errno = 0;
	X = strtod( pload, &p );
	if ( !errno && *p != '\0' ) {
	    printf("bad payload, %s=%s\n",topic,pload);
	    return;
	}
	P_ut = X;
	has_mask |= HAS_UT;
    }

    else if ( !strcmp(topic,topic_P_in) ) {
	double X;
	char* p = 0;
	errno = 0;
	X = strtod( pload, &p );
	if ( !errno && *p != '\0' ) {
	    printf("bad payload, %s=%s\n",topic,pload);
	    return;
	}
	P_in = X;
	has_mask |= HAS_IN;
	calc = 1;
    }

    else if ( !strcmp(topic,topic_P_sun) ) {
	double X;
	char* p = 0;
	errno = 0;
	X = strtod( pload, &p );
	if ( !errno && *p != '\0' ) {
	    printf("bad payload, %s=%s\n",topic,pload);
	    return;
	}
	P_sun = X;
	has_mask |= HAS_SUN;
    }

    else {
	static int bad_count = 25;
	if ( bad_count ) {
	    printf("Ignored topic(%d): %s\n",bad_count,topic);
	    --bad_count;
	}
	return;
    }

    /* Only calculate P_norm if we have all data *AND*  */
    /* when we have a P_in because we know that P_ut comes first */
    /* and then P_in immediately after. */
    if ( calc && (has_mask == ((HAS_IN) | (HAS_UT) | (HAS_SUN)) ) ) {
	int status;
	int l;
	P_norm = calibration_factor * P_sun + P_ut - P_in;
	has_mask = HAS_SUN;

	if ( opt_v )
	    printf("    : %.3lf = %.6lf * %.3lf + %.3lf - %.3lf\n",
		   P_norm, calibration_factor, P_sun,P_ut,P_in);
	
	snprintf(topic_val, MAX_TOPIC_LEN, "%.3lf", P_norm);

	if ( opt_v )
	    printf("calc: %s = %s\n", topic_P_norm, topic_val);
	l = strlen( topic_val );
	status = mosquitto_publish(mqc, 0,
				   topic_P_norm,
				   l,
				   topic_val, 1, true );
	if ( status != MOSQ_ERR_SUCCESS) {
	    perror("mosquitto_publish: ");
	    exit( EXIT_FAILURE );
	}
    }
    
}

void
mq_publish()
{
#if 0
    int i;
    for ( i = 0; tab[i].addr; ++i ) {
	size_t l;
	int status;
	l = strnlen( topic_val[i], MAX_TOPIC_LEN );

	if ( opt_v ) {
	    printf(" %s %s\n", tab[i].topic, topic_val[i] );
	}

	if ( !opt_n && i && (l >0) ) {
	    status = mosquitto_publish(mqc, 0,
				       tab[i].topic,
				       l,
				       topic_val[i], 1, true );
	    if ( status != MOSQ_ERR_SUCCESS) {
		perror("mosquitto_publish: ");
		exit( EXIT_FAILURE );
	    }

	    status = mosquitto_loop_write( mqc, 1 );
	}
    }
#endif
}


void
mq_connect_callback(struct mosquitto *mqc, void *obj, int result)
{
    printf("MQ Connected: %d\n", result);
    if ( result != 0 ) {
	/* Something is wrong.  Wait before retry */
	sleep(5);
    }
}


void
mq_disconnect_callback(struct mosquitto *mqc, void *obj, int result)
{
    printf("MQ Disonnected: %d\n", result);
}



void
mq_init()
{
    int i;
    if ( opt_v )
	printf("mq_init()\n");
    i = mosquitto_lib_init();
    if ( i != MOSQ_ERR_SUCCESS) {
	perror("mosquitto_lib_init: ");
	exit( EXIT_FAILURE );
    }
    
    mqc = mosquitto_new(mqtt_id, true, 0);
    if ( !mqc ) {
	perror("mosquitto_new: ");
	exit( EXIT_FAILURE );
    }

    mosquitto_connect_callback_set(mqc, mq_connect_callback);
    mosquitto_disconnect_callback_set(mqc, mq_disconnect_callback);
    mosquitto_message_callback_set(mqc, mq_message_callback);

    i = mosquitto_connect(mqc, mqtt_broker, mqtt_port, 60);
    if ( i != MOSQ_ERR_SUCCESS) {
	perror("mosquitto_connect: ");
	exit( EXIT_FAILURE );
    }


}


void
mq_sub(const char* s, const char* t)
{
    if ( !t || !*t ) {
	++topics_missing;
	printf("topic %s missing!\n",s);
    }
    
    mosquitto_subscribe(mqc, NULL, t, 0);
}

void
mq_subscribe()
{
    mq_sub("P_ut",  topic_P_ut );
    mq_sub("P_in",  topic_P_in );
    mq_sub("P_sun", topic_P_sun );
    if ( !topic_P_norm ) {
	++topics_missing;
    }
}

void
mq_fini()
{
    int i;
    if ( opt_v )
	printf("mq_fini()\n");

    if ( mqc ) {
	mosquitto_destroy(mqc);
	mqc = 0;
    }

    i = mosquitto_lib_cleanup();
    if ( i != MOSQ_ERR_SUCCESS) {
	perror("mosquitto_lib_cleanup: ");
	exit( EXIT_FAILURE );
    }
}




/*****************************************************************************/

void
print_usage()
{
    fprintf(stderr,"Usage: mbrdr [-v] [-n] config-file\n");
    exit( EXIT_FAILURE );
}


int
main( int argc, const char** argv )
{
    int i;

    setbuf( stdout, 0 );		/* No buffering */

    chdir("/");

    /* Set default values for the MQTT server. */
    mqtt_id = strdup( MQTT_ID );
    mqtt_broker = strdup( MQTT_BROKER );
    mqtt_port = MQTT_PORT;

    
    --argc; ++argv;			/* Jump over first arg */
    while( argc && *argv && **argv ) {
	if ( !strcmp("-v", *argv) ) {
	    opt_v = 1;
	    if ( opt_v )
		printf("Verbose mode.\n");
	}
	else if ( !strcmp("-n", *argv) ) {
	    opt_n = 1;
	    if ( opt_v )
		printf("No-Active mode.\n");
	}
	else if ( **argv == '-' ) {
	    printf("Illegal argument.");
	    print_usage();
	}
	else if ( argc != 1 ) {
	    printf("Illegal argument.");
	    print_usage();
	    break;
	}
	else {
	    /* Read config file */
	    int status = cfgf_read_file( *argv, tagtab );
	    if ( status ) {
		fprintf(stderr,"Errors in config file.\n");
		exit( EXIT_FAILURE );
	    }
	}
	--argc; ++argv;
    }

    printf("Starting.\n");
	 
    mq_init();
    mq_subscribe();

    if ( topics_missing ) {
	printf("Topic(s) are missing. Inavcive mode!");
	opt_n = 1;
    }

    /* Run the network loop in a background thread, call returns quickly. */
    i = mosquitto_loop_start(mqc);
    if(i != MOSQ_ERR_SUCCESS){
	mosquitto_destroy(mqc);
	fprintf(stderr, "Error: %s\n", mosquitto_strerror(i));
	return 1;
    }

    normp_loop();
    
    mq_fini();

        
    printf("Ending.\n");

    /* Should not come here */
    return EXIT_FAILURE;
}

