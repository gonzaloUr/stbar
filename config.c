#include "./components/pacomponent.h"
#include <stdio.h>

void on_sink_info(const pa_sink_info *i, void *userdata) {
    printf("name: %s\n", i->name);
    printf("mute: %d\n", i->mute);

    pa_cvolume volumes = i->volume;
    for (int i = 0; i < volumes.channels; i++) {
        printf("volume %d: %u\n", i, volumes.values[i]);
    }
}

static const pa_component_cfg pa_cfg = {
    .on_sink_info = &on_sink_info
};

static const struct arg args[] = {
    {
        .type = WRITES_TO_FD,
        .data.writes_to_fd = {
            .config = &pa_cfg,
            .init = pa_component_init,
            .get_fd = pa_component_get_fd,
            .start = pa_component_start,
            .free = pa_component_free
        }
    }
};
