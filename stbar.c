#define _GNU_SOURCE

#include <sys/epoll.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <errno.h>
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
            if (userdatas[i]) {
                struct arg arg = args[i];
                arg.free(userdatas[i]);
            }

        free(userdatas);
    }

    if (memfds) {
        for (int i = 0; i < modules_size; i++)
            if (memfds[i] > 0)
                close(memfds[i]);

        free(memfds);
    }

    if (mutex) {
        pthread_mutex_destroy(mutex);
        free(mutex);
    }

    if (events)
        free(events);

    if (epollfd > 0)
        close(epollfd);

    if (stbarfd > 0)
        close(epollfd);
}

char* read_fd(int fd) {
    struct stat st;
    if (fstat(fd, &st) == -1) {
        perror("fstat");
        return NULL;
    }

    size_t size = st.st_size;
    char *buf = malloc(size + 1);
    ssize_t n = pread(fd, buf, size, 0);

    if (n == -1) {
        perror("pread");
        free(buf);
        return NULL;
    }

    buf[n] = '\0';
    return buf;
}

int print_memfd(int i) {
    int fd = memfds[i];
    char *buf = read_fd(fd);
    if (!buf) return 1;

    dprintf(stbarfd, "%s", buf);
    free(buf);

    return 0;
}

int print_memfds() {
    ftruncate(stbarfd, 0);
    lseek(stbarfd, 0, SEEK_SET);

    dprintf(stbarfd, left_padding);
    if (print_memfd(0)) return 1;

    for (int i = 1; i < modules_size; i++) {
        dprintf(stbarfd, separator);
        if (print_memfd(i)) return 1;
    }
    dprintf(stbarfd, right_padding);

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
        if (print_memfds()) {
            free_state();
            return 1;
        }
        pthread_mutex_unlock(mutex);

        char *prog = "xsetroot -name";
        char *buf = read_fd(stbarfd);
        int size_cmd = sizeof(char) * (strlen(buf) + strlen(prog) + 4);
        char *cmd = malloc(size_cmd);

        snprintf(cmd, size_cmd, "%s '%s'", prog, buf);
        system(cmd);
        free(cmd);
        free(buf);

        int count;
        while (1) {
            count = epoll_wait(epollfd, events, args_size, -1);

            if (count == -1) {
                if (errno == EINTR) {
                    continue;
                }

                perror("epoll_wait");
                free_state();
                return 1;
            }

            break;
        }

        for (int i = 0; i < count; i++) {
            struct epoll_event event = events[i];
            int j = event.data.u32;

            struct arg arg = args[j];
            void *self = userdatas[j];
            arg.exec(self);
        }
    } while (1);

    return 0;
}
