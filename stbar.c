#define _GNU_SOURCE

#include "stbar.h"
#include <sys/eventfd.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

enum type_arg {
    WAITS_ON_FD,
    WRITES_TO_FD,
    EVERY_N_MSEC
};

struct waits_on_fd_arg {
    void* (*init)();
    int (*get_fd)(void*);
    void (*start)(void*);
    void (*exec)(void*, int);
    void (*free)(void*);
};

struct writes_to_fd_arg {
    void* (*init)();
    void (*pass_fd)(void*, int);
    void (*start)(void*);
    void (*free)(void*);
};

struct every_n_msecs_arg {
    void* (*init)();
    int n_ms;
    void (*exec)(void*, int);
    void (*free)(void*);
};

struct arg {
    enum type_arg type;
    union {
        struct waits_on_fd_arg waits_on_fd;
        struct writes_to_fd_arg writes_to_fd;
        struct every_n_msecs_arg every_n_msecs;
    } data;
};

#include "config.h"

int main() {
    size_t n = sizeof(args) / sizeof(struct arg);
    int (*pipefds)[2] = malloc(sizeof(int) * 2 * n);

    for (int i = 0; i < n; i++) {
        if (pipe(pipefds[i]) == -1) {
            perror("pipe failed");
            return 1;
        }
    }

    void **userdatas = malloc(sizeof(void*) * n);

    int epollfd = epoll_create1(0);
    if (epollfd == -1) {
        perror("epoll_create1 failed");
        return 1;
    }

    for (int i = 0; i < n; i++) {
        int fd;
        void *self;

        switch (args[i].type) {
            case WAITS_ON_FD:
                struct waits_on_fd_arg arg_waits = args[i].data.waits_on_fd;

                self = arg_waits.init();
                userdatas[i] = self;

                fd = arg_waits.get_fd(self);
                arg_waits.start(self);

                break;

            case WRITES_TO_FD:
                struct writes_to_fd_arg arg_writes = args[i].data.writes_to_fd;

                self = arg_writes.init();
                userdatas[i] = self;

                arg_writes.pass_fd(self, pipefds[i][1]);
                fd = pipefds[i][0];
                arg_writes.start(self);

                break;

            case EVERY_N_MSEC:
                struct every_n_msecs_arg arg_every = args[i].data.every_n_msecs;

                self = arg_every.init();
                userdatas[i] = self;

                fd = timerfd_create(CLOCK_MONOTONIC, 0);
                int ms = arg_every.n_ms;

                struct itimerspec timer = {
                    .it_value = {
                        .tv_sec = ms / 1000,
                        .tv_nsec = (ms % 1000) * 1000000
                    },
                    .it_interval = {
                        .tv_sec = ms / 1000,
                        .tv_nsec = (ms % 1000) * 1000000
                    }
                };

                timerfd_settime(fd, 0, &timer, NULL);

                break;
        }

        struct epoll_event ev = { .events = EPOLLIN, .data.fd = fd };
        if (epoll_ctl(epollfd, EPOLL_CTL_ADD, pipefds[i][0], &ev) == -1) {
            perror("epoll_ctl failed");
            return 1;
        }
    }

    struct epoll_event *events = malloc(sizeof(struct epoll_event) * n);

    while (1) {
        int count = epoll_wait(epollfd, events, n, -1);
        if (count == -1) {
            perror("epoll_wait failed");
            return 1;
        }
    }

    return 0;
}
