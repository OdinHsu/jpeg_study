#ifndef EVENT_H
#define EVENT_H

#include <pthread.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct{
    pthread_mutex_t mutex;
    pthread_cond_t  cond;
    unsigned int    flag;
} event_t;

#define SetEvent( event ) set_event(&(event))
#define WaitEvent( event ) wait_event(&(event))

void init_event(event_t* event);
void uninit_event(event_t* event);
void set_event( event_t* event);
void wait_event(event_t* event);
int  time_wait_event(event_t* event,int ms);

#ifdef __cplusplus
};
#endif

#endif
