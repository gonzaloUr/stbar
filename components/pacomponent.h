#ifndef PULSE_COMPONENT_H
#define PULSE_COMPONENT_H

#include <pulse/pulseaudio.h>

typedef struct {
    const void           *config;
    int                  pipefd[2];
    pa_threaded_mainloop *mainloop;
    pa_mainloop_api      *api;
    pa_context           *ctx;
} pa_component;

typedef struct {
    void (*on_sink_info)(const pa_sink_info*, void*);
} pa_component_cfg;

void* pa_component_init(const void*);
int pa_component_get_fd(void *userdata);
void pa_component_start(void *userdata);
void pa_component_free(void *userdata);

void ctx_state_callback(pa_context *ctx, void *userdata);
void ctx_event_callback(pa_context *ctx, const char *name, pa_proplist *pl, void *userdata);
void ctx_subscribe_callback(pa_context *ctx, pa_subscription_event_type_t t, uint32_t idx, void *userdata);

void ctx_sink_info_callback(pa_context *ctx, const pa_sink_info *i, int eol, void *userdata);
void ctx_source_info_callback(pa_context *ctx, const pa_source_info *i, int eol, void *userdata);
void ctx_sink_input_info_callback(pa_context *ctx, const pa_sink_input_info *i, int eol, void *userdata);
void ctx_source_output_info_callback(pa_context *ctx, const pa_source_output_info *i, int eol, void *userdata);
void ctx_module_info_callback(pa_context *ctx, const pa_module_info *i, int eol, void *userdata);
void ctx_client_info_callback(pa_context *ctx, const pa_client_info *i, int eol, void *userdata);
void ctx_sample_info_callback(pa_context *ctx, const pa_sample_info *i, int eol, void *userdata);
void ctx_server_info_callback(pa_context *ctx, const pa_server_info *i, void *userdata);
void ctx_card_info_callback(pa_context *ctx, const pa_card_info *i, int eol, void *userdata);

#endif
