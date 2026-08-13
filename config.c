#include "./components/pacomponent.h"
#include <stdio.h>

void on_sink_info(const pa_sink_info *i, void *userdata) {
    pa_component *comp = (pa_component*) userdata;
    pa_cvolume volumes = i->volume;
    pa_volume_t volumes_avg = pa_cvolume_avg(&volumes);
    double percent = volumes_avg * 100.0 / PA_VOLUME_NORM;

    sprintf(comp->msg, "vol %f\n", percent);
}

static const pa_component_cfg pa_cfg = {
    .on_sink_info = &on_sink_info
};

static const struct arg args[] = {
    {
        .initial_text = "???",
        .config = &pa_cfg,
        .init = pa_component_init,
        .get_fd = pa_component_get_fd,
        .exec = pa_component_exec,
        .free = pa_component_free
    }
};
