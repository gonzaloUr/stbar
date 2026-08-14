#include "./components/pacomponent.h"
#include <pthread.h>
#include <stdio.h>
#include <math.h>

void on_sink_info(const pa_sink_info *i, void *userdata) {
    pa_component *comp = (pa_component*) userdata;
    int *memfds = comp->memfds;
    pthread_mutex_t *mutex = comp->mutex;

    pa_cvolume volumes = i->volume;
    pa_volume_t volumes_avg = pa_cvolume_avg(&volumes);
    double percent = volumes_avg * 100.0 / PA_VOLUME_NORM;

    pthread_mutex_lock(mutex);

    int fd = memfds[0];
    ftruncate(fd, 0);
    lseek(fd, 0, SEEK_SET);
    dprintf(fd, "vol %3d", (int) round(percent));

    pthread_mutex_unlock(mutex);
}

void on_source_info(const pa_source_info *i, void *userdata) {
    pa_component *comp = (pa_component*) userdata;
    int *memfds = comp->memfds;
    pthread_mutex_t *mutex = comp->mutex;

    pa_cvolume volumes = i->volume;
    pa_volume_t volumes_avg = pa_cvolume_avg(&volumes);
    double percent = volumes_avg * 100.0 / PA_VOLUME_NORM;

    pthread_mutex_lock(mutex);

    int fd = memfds[1];
    ftruncate(fd, 0);
    lseek(fd, 0, SEEK_SET);
    dprintf(fd, "mic %3d", (int) round(percent));

    pthread_mutex_unlock(mutex);
}

static const pa_component_cfg pa_cfg = {
    .on_sink_info = &on_sink_info,
    .on_source_info = &on_source_info,
    .on_sink_input_info = NULL,
    .on_source_output_info = NULL,
    .on_module_info = NULL,
    .on_client_info = NULL,
    .on_sample_info = NULL,
    .on_server_info = NULL,
    .on_card_info = NULL,
};

static const char *modules[] = {
    "vol ???",
    "mic ???"
};

static const struct arg args[] = {
    {
        .config = &pa_cfg,
        .init = pa_component_init,
        .get_fd = pa_component_get_fd,
        .exec = pa_component_exec,
        .free = pa_component_free
    }
};
