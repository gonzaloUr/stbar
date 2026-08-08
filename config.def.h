static const struct arg args[] = {
    {
        .type = WRITES_TO_FD,
        .data.writes_to_fd = {
            .init = pa_component_init,
            .pass_fd = pa_component_pass_fd,
            .start = pa_component_start,
            .free = pa_component_free
        }
    }
};
