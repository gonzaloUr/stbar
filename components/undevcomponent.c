#include "udevcomponent.h"
#include <stdlib.h>
#include <stdio.h>

void* udev_component_init(const void *cfg, int *memfds, pthread_mutex_t *mutex) {
    udev_component_cfg *c = (udev_component_cfg*) cfg;
    udev_component *u = malloc(sizeof(udev_component));
    u->config = cfg;
    u->memfds = memfds;
    u->mutex = mutex;

    u->udev = udev_new();
    if (!u->udev) {
        perror("udev_new");
        abort();
    }

    u->mon = udev_monitor_new_from_netlink(u->udev, "udev");
    if (!u->mon) {
        perror("udev_monitor_new_from_netlink");
        udev_unref(u->udev);
        abort();
    }

    if (c->subsystem_matches) {
        for (size_t i = 0; i < c->subsystem_matches_size; i++) {
            const udev_subsystem_match sm = c->subsystem_matches[i];
            udev_monitor_filter_add_match_subsystem_devtype(u->mon, sm.subsystem, sm.devtype);
        }
    }

    if (c->tag_matches) {
        for (size_t i = 0; i < c->tag_matches_size; i++) {
            const char *t = c->tag_matches[i];
            udev_monitor_filter_add_match_tag(u->mon, t);
        }
    }

    return u;
}

int udev_component_get_fd(void *userdata) {
    udev_component *u = (udev_component*) userdata;

    if (udev_monitor_enable_receiving(u->mon) < 0) {
        perror("udev_monitor_enable_receiving");
        udev_monitor_unref(u->mon);
        udev_unref(u->udev);
        abort();
    }

    u->fd = udev_monitor_get_fd(u->mon);
    return u->fd;
}

void udev_component_exec(void *userdata) {
    udev_component *u = (udev_component*) userdata;
    udev_component_cfg *c = (udev_component_cfg*) u->config;

    struct udev_device *dev = udev_monitor_receive_device(u->mon);
    if (!dev) return;

    if (c->on_dev) c->on_dev(dev, u);
    udev_device_unref(dev);
}

void udev_component_free(void *userdata) {
    udev_component *u = (udev_component*) userdata;

    udev_monitor_unref(u->mon);
    udev_unref(u->udev);
    free(u);
}
