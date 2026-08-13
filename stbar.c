#define _GNU_SOURCE

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
    const void* config;
    void* (*init)(const void*);
    int (*get_fd)(void*);
    void (*exec)(void*, int);
    void (*free)(void*);
};

struct writes_to_fd_arg {
    const void* config;
    void* (*init)(const void*);
    int (*get_fd)(void*);
    void (*start)(void*);
    void (*free)(void*);
};

struct every_n_msecs_arg {
    const void* config;
    void* (*init)(const void*);
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

#include "config.c"

int main() {
    size_t n = sizeof(args) / sizeof(struct arg);
    void **userdatas = malloc(sizeof(void*) * n);
    int *readfds = malloc(sizeof(int) * n);
    int *writefds = malloc(sizeof(int) * n);

    int epollfd = epoll_create1(0);
    if (epollfd == -1) {
        perror("epoll_create1");
        return 1;
    }

    for (int i = 0; i < n; i++) {
        int rfd;
        int wfd;
        int efd;

        int pipefd[2];
        void *self;

        switch (args[i].type) {
            case WAITS_ON_FD:
                struct waits_on_fd_arg arg_waits = args[i].data.waits_on_fd;

                self = arg_waits.init(arg_waits.config);
                userdatas[i] = self;

                efd = arg_waits.get_fd(self);

                if (pipe(pipefd) == -1) {
                    perror("pipe");
                    return 1;
                }

                rfd = pipefd[0];
                wfd = pipefd[1];

                break;

            case WRITES_TO_FD:
                struct writes_to_fd_arg arg_writes = args[i].data.writes_to_fd;

                self = arg_writes.init(arg_writes.config);
                userdatas[i] = self;

                efd = arg_writes.get_fd(self);
                rfd = efd;
                wfd = 0;

                arg_writes.start(self);

                break;

            case EVERY_N_MSEC:
                struct every_n_msecs_arg arg_every = args[i].data.every_n_msecs;

                self = arg_every.init(arg_every.config);
                userdatas[i] = self;

                efd = timerfd_create(CLOCK_MONOTONIC, 0);

                if (pipe(pipefd) == -1) {
                    perror("pipe");
                    return 1;
                }

                rfd = pipefd[0];
                wfd = pipefd[1];

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

                timerfd_settime(efd, 0, &timer, NULL);

                break;
        }

        readfds[i] = rfd;
        writefds[i] = wfd;

        struct epoll_event ev = { .events = EPOLLIN, .data.fd = efd, .data.u32 = i };
        if (epoll_ctl(epollfd, EPOLL_CTL_ADD, efd, &ev) == -1) {
            perror("epoll_ctl");
            return 1;
        }
    }

    struct epoll_event *events = malloc(sizeof(struct epoll_event) * n);

    while (1) {
        int count = epoll_wait(epollfd, events, n, -1);
        if (count == -1) {
            perror("epoll_wait");
            return 1;
        }

        for (int i = 0; i < count; i++) {
            struct epoll_event event = events[i];
            int j = event.data.u32;

            void *self = userdatas[j];

            switch (args[j].type) {
                case WAITS_ON_FD:
                    struct waits_on_fd_arg arg_waits = args[j].data.waits_on_fd;
                    arg_waits.exec(self, writefds[j]);
                    break;

                case EVERY_N_MSEC:
                    struct every_n_msecs_arg arg_every = args[j].data.every_n_msecs;
                    arg_every.exec(self, writefds[j]);
                    break;

                default:
                    break;
            }

            int readfd = readfds[j];
            int size;

            if (ioctl(readfd, FIONREAD, &size) == -1) {
                perror("ioctl");
                return 1;
            }

            char *buf = malloc(size + 1);
            ssize_t n = read(readfd, buf, size);

            if (n > 0) {
                buf[n] = '\0';
                printf("%s\n", buf);
            }
        }
    }

    return 0;
}
