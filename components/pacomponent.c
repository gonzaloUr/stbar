#include "pacomponent.h"
#include <stdlib.h>

void* pa_component_init() {
    void *self = malloc(sizeof(pa_component));
    return self;
}

void pa_component_pass_fd(void *userdata, int fd) {
    pa_component *self = (pa_component*) userdata;
    self->fd = fd;
}

void pa_component_start(void *userdata) {
    pa_component *self = (pa_component*) userdata;

    // Create mainloop.
    self->mainloop = pa_threaded_mainloop_new();
    if (!self->mainloop) {
        free(self);
        abort();
    }

    // Get mainloop api.
    self->api = pa_threaded_mainloop_get_api(self->mainloop);

    // Create context.
    self->ctx = pa_context_new(self->api, "pacomponent");
    if (!self->ctx) {
        pa_threaded_mainloop_free(self->mainloop);
        free(self);
        abort();
    }

    // Setup context callbacks.
    pa_context_set_state_callback(self->ctx, ctx_state_callback, self);
    pa_context_set_event_callback(self->ctx, ctx_event_callback, self);
    pa_context_set_subscribe_callback(self->ctx, ctx_subscribe_callback, self);

    // Connect context.
    pa_threaded_mainloop_lock(self->mainloop);

    if (pa_context_connect(self->ctx, NULL, PA_CONTEXT_NOFLAGS, NULL) < 0) {
        pa_context_unref(self->ctx);
        pa_threaded_mainloop_free(self->mainloop);
        free(self);
        abort();
    }

    pa_threaded_mainloop_unlock(self->mainloop);
}

void pa_component_free(void *userdata) {
    pa_component *self = (pa_component*) userdata;

	pa_threaded_mainloop_stop(self->mainloop);
	pa_context_disconnect(self->ctx);
	pa_context_unref(self->ctx);
	pa_threaded_mainloop_free(self->mainloop);
    free(self);
}

// Callback related to the mainloop of this pulseaudio client.
void ctx_state_callback(pa_context *ctx, void *userdata) {
    pa_context_state_t state = pa_context_get_state(ctx);

    switch (state) {
        case PA_CONTEXT_UNCONNECTED:
            break;

        case PA_CONTEXT_CONNECTING:
            break;

        case PA_CONTEXT_AUTHORIZING:
            break;

        case PA_CONTEXT_SETTING_NAME:
            break;

        case PA_CONTEXT_READY:
            break;

        case PA_CONTEXT_FAILED:
            break;

        case PA_CONTEXT_TERMINATED:
            break;
    }
}

// Init function that prints the current server info.
void init_ctx_server_info_callback(pa_context *ctx, const pa_server_info *i, void *userdata) {
}

// context callback for events.
void ctx_event_callback(pa_context *ctx, const char *name, pa_proplist *pl, void *userdata) {
}

// context callback for subscriptions, changes in the server pretty much, then dispatchs to different functions.
void ctx_subscribe_callback(pa_context *ctx, pa_subscription_event_type_t t, uint32_t idx, void *userdata) {
    switch (t & PA_SUBSCRIPTION_EVENT_FACILITY_MASK) {
        case PA_SUBSCRIPTION_EVENT_SINK:
            pa_context_get_sink_info_by_index(ctx, idx, ctx_sink_info_callback, userdata);
            break;
        case PA_SUBSCRIPTION_EVENT_SOURCE:
            pa_context_get_source_info_by_index(ctx, idx, ctx_source_info_callback, userdata);
            break;
        case PA_SUBSCRIPTION_EVENT_SINK_INPUT:
            pa_context_get_sink_input_info(ctx, idx, ctx_sink_input_info_callback, userdata);
            break;
        case PA_SUBSCRIPTION_EVENT_SOURCE_OUTPUT:
            pa_context_get_source_output_info(ctx, idx, ctx_source_output_info_callback, userdata);
            break;
        case PA_SUBSCRIPTION_EVENT_MODULE:
            pa_context_get_module_info(ctx, idx, ctx_module_info_callback, userdata);
            break;
        case PA_SUBSCRIPTION_EVENT_CLIENT:
            pa_context_get_client_info(ctx, idx, ctx_client_info_callback, userdata);
            break;
        case PA_SUBSCRIPTION_EVENT_SAMPLE_CACHE:
            pa_context_get_sample_info_by_index(ctx, idx, ctx_sample_info_callback, userdata);
            break;
        case PA_SUBSCRIPTION_EVENT_SERVER:
            pa_context_get_server_info(ctx, ctx_server_info_callback, userdata);
            break;
        case PA_SUBSCRIPTION_EVENT_CARD:
            pa_context_get_card_info_by_index(ctx, idx, ctx_card_info_callback, userdata);
            break;
    }
}

// A subscribe event related to a sink, a sink is an audio output device, like your speakers, headphones, HDMI audio, etc.
void ctx_sink_info_callback(pa_context *ctx, const pa_sink_info *i, int eol, void *userdata) {
    if (eol > 0 || !i) return;
}

// A subscribe event related to a source, which is an audio input device, like a microphone or a virtual capture source.
void ctx_source_info_callback(pa_context *ctx, const pa_source_info *i, int eol, void *userdata) {
    if (eol > 0 || !i) return;
}

// A subscribe event about a sink input, which is an audio stream that’s going into a sink.
void ctx_sink_input_info_callback(pa_context *ctx, const pa_sink_input_info *i, int eol, void *userdata) {
    if (eol > 0 || !i) return;
}

// A subscribe event about a source output, which is a stream being recorded from a source.
void ctx_source_output_info_callback(pa_context *ctx, const pa_source_output_info *i, int eol, void *userdata) {
    if (eol > 0 || !i) return;
}

// A subscribe event about a PulseAudio module. Modules are plug-ins that provide functionality, like Bluetooth support.
void ctx_module_info_callback(pa_context *ctx, const pa_module_info *i, int eol, void *userdata) {
    if (eol > 0 || !i) return;
}

// A subscribe event about a PulseAudio client, which is any process connected to the server.
void ctx_client_info_callback(pa_context *ctx, const pa_client_info *i, int eol, void *userdata) {
    if (eol > 0 || !i) return;
}

// A subscribe event about a sample cache item, which is an audio sample stored for quick playback.
void ctx_sample_info_callback(pa_context *ctx, const pa_sample_info *i, int eol, void *userdata) {
    if (eol > 0 || !i) return;
}

// Indicates a global server state change.
void ctx_server_info_callback(pa_context *ctx, const pa_server_info *i, void *userdata) {
}

// A subscribe event about an audio card, which is a representation of a physical or virtual sound device.
void ctx_card_info_callback(pa_context *ctx, const pa_card_info *i, int eol, void *userdata) {
    if (eol > 0 || !i) return;
}
