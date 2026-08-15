#define _GNU_SOURCE

#include <sys/epoll.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct arg {
    const void *config;
    void* (*init)(const void*, int*, pthread_mutex_t*);
    int (*get_fd)(void*);
    void (*exec)(void*);
    void (*free)(void*);
};

#include "config.c"

static size_t args_size = sizeof(args) / sizeof(struct arg);
static size_t modules_size = sizeof(modules) / sizeof(char*);
static void **userdatas = NULL;
static int *memfds = NULL;
static pthread_mutex_t *mutex = NULL;
static struct epoll_event *events = NULL;
static int epollfd = -1;
static int stbarfd = -1;

void free_state() {
    if (userdatas) {
        for (int i = 0; i < args_size; i++)
            if (userdatas[i]) free(userdatas[i]);

        free(userdatas);
    }

    if (memfds) {
        for (int i = 0; i < modules_size; i++)
            if (memfds[i]) close(memfds[i]);

        free(memfds);
    }

    if (mutex) {
        pthread_mutex_destroy(mutex);
        free(mutex);
    }

    if (events)
        free(events);
}

int print_memfd(int i) {
    int fd = memfds[i];
    struct stat st;
    if (fstat(fd, &st) == -1) {
        perror("fstat");
        return 1;
    }

    size_t size = st.st_size;
    char *buf = malloc(size + 1);
    ssize_t n = pread(fd, buf, size, 0);

    if (n == -1) {
        perror("pread");
        return 1;
    }

    buf[n] = '\0';

    dprintf(stbarfd, "%s", buf);
    free(buf);

    fflush(stdout);

    return 0;
}

int print_memfds() {
    ftruncate(stbarfd, 0);
    lseek(stbarfd, 0, SEEK_SET);

    if (print_memfd(0)) return 1;

    for (int i = 1; i < modules_size; i++) {
        dprintf(stbarfd, " | ");
        if (print_memfd(i)) return 1;
    }

    fflush(stdout);

    return 0;
}

int main() {
    userdatas = malloc(sizeof(void*) * args_size);
    memfds = malloc(sizeof(int) * modules_size);
    mutex = malloc(sizeof(pthread_mutex_t));
    stbarfd = memfd_create("", MFD_CLOEXEC);
    pthread_mutex_init(mutex, NULL);

    epollfd = epoll_create1(0);
    if (epollfd == -1) {
        perror("epoll_create1");
        free_state();
        return 1;
    }

    for (int i = 0; i < modules_size; i++) {
        memfds[i] = memfd_create("", MFD_CLOEXEC);
        write(memfds[i], modules[i], strlen(modules[i]));
    }

    for (int i = 0; i < args_size; i++) {
        struct arg arg = args[i];
        void *self = arg.init(arg.config, memfds, mutex);
        userdatas[i] = self;

        int fd = arg.get_fd(self);
        struct epoll_event ev = { .events = EPOLLIN, .data.fd = fd, .data.u32 = i };
        if (epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev) == -1) {
            perror("epoll_ctl");
            free_state();
            return 1;
        }
    }

    events = malloc(sizeof(struct epoll_event) * args_size);

    do {
        pthread_mutex_lock(mutex);
        if (print_memfds()) return 1;
        pthread_mutex_unlock(mutex);

        struct stat st;
        if (fstat(stbarfd, &st) < 0) {
            free_state();
            return 1;
        }

        char *buf = malloc(st.st_size + 1);
        lseek(stbarfd, 0, SEEK_SET);
        ssize_t n = read(stbarfd, buf, st.st_size);

        if (n < 0) {
            perror("read");
            free_state();
            return 1;
        }

        buf[n] = '\0';

        char cmd[1024];
        snprintf(cmd, sizeof(cmd), "xsetroot -name '%s'", buf);
        system(cmd);
        free(buf);

        int count = epoll_wait(epollfd, events, args_size, -1);
        if (count == -1) {
            perror("epoll_wait");
            free_state();
            return 1;
        }

        for (int i = 0; i < count; i++) {
            struct epoll_event event = events[i];
            int j = event.data.u32;

            struct arg arg = args[j];
            void *self = userdatas[j];
            arg.exec(self);
        }
    } while(1);

    return 0;
}
