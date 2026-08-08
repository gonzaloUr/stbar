static const struct arg args[] = {
    {
        .type = WRITES_TO_FD,
        .data.writes_to_fd = {
            .init = pa_init_component,
            .pass_fd = pa_pass_fd_component,
            .start = pa_start_component,
            .free = pa_free_component
        }
    }
};
