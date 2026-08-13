#define _GNU_SOURCE

#include <sys/epoll.h>
#include <stdlib.h>
#include <stdio.h>

struct arg {
    const char *initial_text;
    const void *config;
    void* (*init)(const void*);
    int (*get_fd)(void*);
    char* (*exec)(void*);
    void (*free)(void*);
};

#include "config.c"

struct modules {
    int n;
    char **current_text;
    void **userdatas;
    int epollfd;
};

struct modules* modules_new(int n) {
    struct modules *m = malloc(sizeof(struct modules));

    m->n = n;
    m->current_text = malloc(sizeof(char*) * n);
    m->userdatas = malloc(sizeof(void*) * n);

    return m;
}

void modules_print(struct modules *m) {
    int n = m->n;

    for (int i = 0; i < n; i++) {
        void *self = m->userdatas[i];
        struct arg arg = args[i];

        printf("%s", arg.exec(self));
    }
}

void modules_free(struct modules *m) {
    free(m->current_text);
    free(m->userdatas);
    free(m);
}

int main() {
    size_t n = sizeof(args) / sizeof(struct arg);
    struct modules *m = modules_new(n);

    int epollfd = epoll_create1(0);
    if (epollfd == -1) {
        modules_free(m);
        perror("epoll_create1");
        return 1;
    }

    for (int i = 0; i < n; i++) {
        struct arg arg = args[i];
        void *self = arg.init(arg.config);
        int fd = arg.get_fd(self);

        m->current_text[i] = (char*) arg.initial_text;
        m->userdatas[i] = self;

        struct epoll_event ev = { .events = EPOLLIN, .data.fd = fd, .data.u32 = i };
        if (epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev) == -1) {
            modules_free(m);
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

            struct arg arg = args[j];
            void *self = m->userdatas[j];
            m->current_text[j] = arg.exec(self);
        }

        modules_print(m);
    }

    return 0;
}
