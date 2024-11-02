
#include "event.h"
#include <sys/time.h>


void init_event(event_t* event)
{
    pthread_mutex_init(&(event->mutex),NULL);
    pthread_cond_init(&(event->cond),NULL);
    event->flag = 0;
};

void uninit_event(event_t* event)
{
    pthread_mutex_destroy(&event->mutex);
    pthread_cond_destroy(&event->cond);
};

void set_event( event_t* event)
{
    pthread_mutex_lock(&event->mutex);
    event->flag = 1;
    pthread_cond_signal(&event->cond);

    pthread_mutex_unlock(&event->mutex);
};

void wait_event(event_t* event)
{
    pthread_mutex_lock(&event->mutex);

    while(!event->flag)
        pthread_cond_wait(&event->cond,&event->mutex);

    event->flag = 0;

    pthread_mutex_unlock(&event->mutex);
};

int time_wait_event(event_t* event,int ms)
{
    struct timespec ts;
    struct timeval tv;
    int    result=0;

    pthread_mutex_lock(&event->mutex);

    gettimeofday(&tv, NULL);

    ts.tv_nsec = (tv.tv_usec + ms*1000)*1000;
    ts.tv_sec = tv.tv_sec;
    if( ts.tv_nsec > 1000000000L ) {
        ts.tv_nsec -= 1000000000L;
        ts.tv_sec++;
    };

    while((!event->flag) && (result == 0))
        result = pthread_cond_timedwait(&event->cond,&event->mutex,&ts);

    if( result == 0 ) event->flag = 0;

    pthread_mutex_unlock(&event->mutex);

    return result;
};
