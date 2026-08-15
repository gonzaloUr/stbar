#define _GNU_SOURCE

#include "clockcomponent.h"
#include <sys/timerfd.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>

void* clock_component_init(const void* config, int* memfds, pthread_mutex_t* mutex) {
    clock_component *comp = malloc(sizeof(clock_component));
    comp->config = config;
    comp->mutex = mutex;
    comp->memfds = memfds;

    return comp;
}

int clock_component_get_fd(void *userdata) {
    clock_component *comp = (clock_component*) userdata;
    clock_component_cfg *config = (clock_component_cfg*) comp->config;

    comp->fd = timerfd_create(CLOCK_REALTIME, TFD_CLOEXEC);
    if (comp->fd < 0) {
        free(comp);
        abort();
    }

    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);
    time_t next = now.tv_sec - (now.tv_sec % 60) + 60;

    struct itimerspec timer = {
        .it_value = {
            .tv_sec = next,
            .tv_nsec = 0
        },
        .it_interval = {
            .tv_sec = 60,
            .tv_nsec = 0
        }
    };

    if (timerfd_settime(comp->fd, TFD_TIMER_ABSTIME, &timer, NULL) < 0) {
        close(comp->fd);
        free(comp);
        abort();
    }

    if (config->on_tick) config->on_tick(comp);
    return comp->fd;
}

void clock_component_exec(void *userdata) {
    clock_component *comp = (clock_component*) userdata;
    clock_component_cfg *config = (clock_component_cfg*) comp->config;

    uint64_t expirations;
    read(comp->fd, &expirations, sizeof(expirations));

    if (config->on_tick) config->on_tick(comp);
}

void clock_component_free(void *userdata) {
    clock_component *comp = userdata;

    close(comp->fd);
    free(comp);
}
