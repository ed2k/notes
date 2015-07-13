#define _GNU_SOURCE             /* See feature_test_macros(7) */
#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <sched.h>
#include <inttypes.h>
#include <math.h>

#ifndef rdtscll
#define rdtscll(val) do { \
  unsigned int __a,__d; \
  __asm__ __volatile__("rdtsc" : "=a" (__a), "=d" (__d)); \
  (val) = ((unsigned long)__a) | (((unsigned long)__d)<<32); \
  } while(0)
#endif



#define handle_error_en(en, msg) \
        do { errno = en; perror(msg); exit(1); } while (0)

#define MEAS_PRIME_NUMS 100000
#define BUSY_PRIME_NUMS 100000000
#define TOLERANCE_DEFAULT 1.5f
#define REPEAT_DEFAULT 1

static const char *progname;

static void
usage()
{
    printf("usage: %s {-b CPU} [-r REPEAT | -t tolerance]\n", progname);
}

static int cpu_busy=0;
static float tolerance=TOLERANCE_DEFAULT;
static int repeat=REPEAT_DEFAULT;


/* Thread start function: display address near top of our stack,
   and return upper-cased copy of argv_string */

void
first_x_primes(uint32_t count) {
    uint32_t i,j;
    uint32_t k=0;
    char* mm = malloc(count);
    for (i=0; i<count; i++)
        for (j=0; j<0xff; j++)
        {
            k = (i+j+1);
            k = (i+j+mm[k%count]);
	    mm[k%count] = k;
        }
}


static int
ht_detect(int cpu, long long *meas_max, long long *meas_avg, int *sibling) {
    int rc;
    long long t0, t1;
    long long meas_count=0;
    long long t;

    int num_cpus = sysconf( _SC_NPROCESSORS_ONLN );

    *meas_max=0;
    *meas_avg=0;
    *sibling=-1;
 
    int idx;
    cpu_set_t cpu_set_meas;
    for(idx=0; idx<num_cpus; idx++) {
        CPU_ZERO(&cpu_set_meas);
        CPU_SET(0, &cpu_set_meas);
        rc = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpu_set_meas);
        if (rc != 0) {
            handle_error_en(rc, "meas: pthread_setaffinity_np");
        }
        rdtscll(t0);
        first_x_primes(4096*(idx+1));
        rdtscll(t1);
        t = t1-t0;
        meas_count+=t;
        if(t>*meas_max) {
            *meas_max=t;
            *sibling=idx;
        }
        printf("idx:%d tsc:%llu\n", idx, t/(idx+1));
    }
    /* Calculate the average */
    *meas_avg=(meas_count/(1));
    if(*meas_max < (*meas_avg * tolerance)) {
        return 1;
    }
    return 0;
}

static void *
thread_meas_hdlr(void *arg)
{
    int rc;
    int sibling;
    long long meas_max;
    long long meas_avg;

    int count;
    int prev_sibling=0;
    for(count=0; count<repeat; count++) {
        rc = ht_detect(cpu_busy, &meas_max, &meas_avg, &sibling);
        if(rc) {
            printf("HT not detected for CPU%d. meas_max:%llu meas_avg:%llu\n",
                   cpu_busy, meas_max, meas_avg);
            return NULL;
        }
        if(prev_sibling == 0) {
            prev_sibling = sibling;
        } else if (prev_sibling != sibling) {
            /* Insert another coin */
            printf("HT results are variable! prev_sibling:%d sibling:%d\n",
                   prev_sibling, sibling);
            return NULL;
        }
    }
    printf("HT Sibling of CPU%d is CPU%d. meas_max:%llu meas_avg:%llu\n",
            cpu_busy, sibling, meas_max, meas_avg);
    return NULL;
}

int
main(int argc, char *argv[])
{
    int rc, opt;
    pthread_t thr_busy;
    pthread_t thr_meas;
    pthread_attr_t attr;

    progname = argv[0];

    while ((opt = getopt(argc, argv, "r:b:t:h")) != -1) {
        switch (opt) {
        case 'b':
            cpu_busy = strtol(optarg, (char**)NULL, 0);
            break;
        case 'r':
            repeat = strtol(optarg, (char**)NULL, 0);
            break;
        case 't':
            tolerance = strtof(optarg, (char**)NULL);
            break;
        case 'h':
        default:
            usage();
            exit(1);
        }
    }

    if(cpu_busy == -1) {
        usage();
        exit(1);
    }

    rc = pthread_attr_init(&attr);
    if(rc) {
        handle_error_en(rc, "pthread_attr_init");
    }

    rc = pthread_create(&thr_meas, &attr,
                       &thread_meas_hdlr, NULL);
    if(rc) {
        handle_error_en(rc, "pthread_create");
    }

    /* Destroy the thread attributes object, since it is no
       longer needed */

    rc = pthread_attr_destroy(&attr);
    if(rc) {
        handle_error_en(rc, "pthread_attr_destroy");
    }

    /* Now join with each thread, and display its returned value */

    rc = pthread_join(thr_meas, NULL);
    if(rc) {
        handle_error_en(rc, "pthread_join");
    }

    return 0;
}
