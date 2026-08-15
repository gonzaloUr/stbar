#include "./components/pacomponent.h"
#include "./components/udevcomponent.h"
#include "./components/clockcomponent.h"
#include <stdio.h>
#include <math.h>

typedef struct {
    char *default_sink;
    char *default_source;
} pa_component_cb_data;

void on_server_info(const pa_server_info *i, void *userdata) {
    pa_component *comp = (pa_component*) userdata;
    pa_component_cfg *config = (pa_component_cfg*) comp->config;
    pa_component_cb_data *cb_data = (pa_component_cb_data*) config->cb_data;

    if (cb_data->default_sink) free(cb_data->default_sink);
    if (cb_data->default_source) free(cb_data->default_source);

    if (i->default_sink_name) cb_data->default_sink = strdup(i->default_sink_name);
    if (i->default_source_name) cb_data->default_source = strdup(i->default_source_name);
}

void on_sink_info(const pa_sink_info *i, void *userdata) {
    pa_component *comp = (pa_component*) userdata;
    pa_component_cfg *config = (pa_component_cfg*) comp->config;
    pa_component_cb_data *cb_data = (pa_component_cb_data*) config->cb_data;

    if (!cb_data->default_sink)
        return;

    if (strcmp(cb_data->default_sink, i->name))
        return;

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
    pa_component_cfg *config = (pa_component_cfg*) comp->config;
    pa_component_cb_data *cb_data = (pa_component_cb_data*) config->cb_data;

    if (!cb_data->default_source)
        return;

    if (strcmp(cb_data->default_source, i->name))
        return;

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

void on_dev(struct udev_device* dev, void* userdata) {
    udev_component *comp = (udev_component*) userdata;
    int *memfds = comp->memfds;
    pthread_mutex_t *mutex = comp->mutex;

    if (dev == NULL || strcmp(udev_device_get_subsystem(dev), "backlight") == 0) {
        char result[256];

        FILE *fp = popen("light", "r");
        if (!fp) abort();
        fgets(result, sizeof(result), fp);
        pclose(fp);

        double brg_d = atof(result);
        int brg = (int) round(brg_d);

        pthread_mutex_lock(mutex);

        int fd = memfds[2];
        ftruncate(fd, 0);
        lseek(fd, 0, SEEK_SET);
        dprintf(fd, "brg %3d", brg);

        pthread_mutex_unlock(mutex);
    }
}

void on_tick(void* userdata) {
    clock_component *comp = (clock_component*) userdata;
    int *memfds = comp->memfds;
    pthread_mutex_t *mutex = comp->mutex;

    char buf[256];
    time_t now = time(NULL) + 1;
    struct tm tm;

    localtime_r(&now, &tm);
    strftime(buf, sizeof(buf), "%a %d/%m %b %H:%M", &tm);

    pthread_mutex_lock(mutex);

    int fd = memfds[3];
    ftruncate(fd, 0);
    lseek(fd, 0, SEEK_SET);
    dprintf(fd, "%s", buf);

    pthread_mutex_unlock(mutex);
}

static pa_component_cb_data pa_cb_data = {
    .default_sink = NULL,
    .default_source = NULL
};

static const pa_component_cfg pa_cfg = {
    .cb_data = &pa_cb_data,
    .on_sink_info = &on_sink_info,
    .on_source_info = &on_source_info,
    .on_sink_input_info = NULL,
    .on_source_output_info = NULL,
    .on_module_info = NULL,
    .on_client_info = NULL,
    .on_sample_info = NULL,
    .on_server_info = &on_server_info,
    .on_card_info = NULL,
};

static const udev_subsystem_match udev_component_cfg_matches[] = {
    { .subsystem = "backlight", .devtype = NULL },
};

static const udev_component_cfg udev_cfg = {
    .subsystem_matches = udev_component_cfg_matches,
    .subsystem_matches_size = sizeof(udev_component_cfg_matches) / sizeof(udev_subsystem_match),

    .tag_matches = NULL,
    .tag_matches_size = 0,

    .on_dev = &on_dev
};

static const clock_component_cfg clock_cfg = {
    .on_tick = on_tick
};

static const char *modules[] = {
    "vol ???",
    "mic ???",
    "brg ???",
    "??? --/-- ??? ??:??"
};

static const struct arg args[] = {
    {
        .config = &pa_cfg,
        .init = pa_component_init,
        .get_fd = pa_component_get_fd,
        .exec = pa_component_exec,
        .free = pa_component_free
    },
    {
        .config = &udev_cfg,
        .init = udev_component_init,
        .get_fd = udev_component_get_fd,
        .exec = udev_component_exec,
        .free = udev_component_free
    },
    {
        .config = &clock_cfg,
        .init = clock_component_init,
        .get_fd = clock_component_get_fd,
        .exec = clock_component_exec,
        .free = clock_component_free
    }
};
