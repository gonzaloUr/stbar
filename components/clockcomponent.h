#ifndef CLOCK_COMPONENT_H
#define CLOCK_COMPONENT_H

#include <pthread.h>

typedef struct {
    const void      *config;
    pthread_mutex_t *mutex;
    int             *memfds;
    int             fd;
} clock_component;

typedef struct {
    void (*on_tick)(void*);
} clock_component_cfg;

void* clock_component_init(const void*, int*, pthread_mutex_t*);
int clock_component_get_fd(void *userdata);
void clock_component_exec(void *userdata);
void clock_component_free(void *userdata);

#endif
