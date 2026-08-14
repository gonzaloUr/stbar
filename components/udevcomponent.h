#ifndef UDEV_COMPONENT_H
#define UDEV_COMPONENT_H

#include <libudev.h>
#include <pthread.h>

typedef struct {
    const void           *config;
    pthread_mutex_t      *mutex;
    int                  *memfds;
    int                  fd;
    struct udev          *udev;
    struct udev_monitor  *mon;
    int                  fd_mon;
} udev_component;

typedef struct {
    const char *subsystem;
    const char *devtype;
} udev_subsystem_match;

typedef struct {
    const udev_subsystem_match *subsystem_matches;
    size_t subsystem_matches_size;

    const char **tag_matches;
    size_t tag_matches_size;

    void (*on_dev)(struct udev_device*, void*);
} udev_component_cfg;

void* udev_component_init(const void*, int*, pthread_mutex_t*);
int udev_component_get_fd(void *userdata);
void udev_component_exec(void *userdata);
void udev_component_free(void *userdata);

#endif
