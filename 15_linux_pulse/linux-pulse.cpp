#include <mutex>
#include <fmt/format.h>
#include <logging.h>
#include <fmt/format.h>
#include "defer.h"
#include "linux-pulse.h"

static std::mutex pulse_mtx;
static pa_threaded_mainloop *pulse_loop = nullptr;
static pa_context *pulse_ctx = nullptr;

static void pulse_context_state_callback(pa_context *ctx, void *userdata)
{
    switch (pa_context_get_state(ctx))
    {
    // There are just here for reference
    case PA_CONTEXT_UNCONNECTED:
    case PA_CONTEXT_CONNECTING:
    case PA_CONTEXT_AUTHORIZING:
    case PA_CONTEXT_SETTING_NAME:
    default:
        break;
    case PA_CONTEXT_FAILED:
    case PA_CONTEXT_TERMINATED:
        LOG(INFO) << "PA_CONTEXT_TERMINATED";
        break;
    case PA_CONTEXT_READY:
        LOG(INFO) << "PA_CONTEXT_READY";
        break;
    }

    pulse_signal(0);
}

static const char *pulse_source_state_to_string(int state)
{
    switch (state)
    {
    case PA_SOURCE_INVALID_STATE:
    default:
        return "INVALID";

    case PA_SOURCE_RUNNING:
        return "RUNNING";
    case PA_SOURCE_IDLE:
        return "IDLE";
    case PA_SOURCE_SUSPENDED:
        return "SUSPENDED";
    case PA_SOURCE_INIT:
        return "INIT";
    case PA_SOURCE_UNLINKED:
        return "UNLINKED";
    }
}

static void pulse_source_info_callback(pa_context *ctx, const pa_source_info *i, int eol, void *userdata)
{
    if (eol == 0)
    {
        LOG(INFO) << fmt::format("name: {:64}, desc: {:64}, format: {:6}, rate: {:>5d}, channels: {:>1d}, state: {:>9}",
                                 i->name, i->description,
                                 pa_sample_format_to_string(i->sample_spec.format), i->sample_spec.rate, i->sample_spec.channels,
                                 pulse_source_state_to_string(i->state));
    }

    pulse_signal(0);
}

static void pulse_sink_info_callback(pa_context *ctx, const pa_sink_info *i, int eol, void *userdata)
{
    if (eol == 0)
    {
        LOG(INFO) << fmt::format("name: {:64}, desc: {:64}, format: {:6}, rate: {:>5d}, channels: {:>1d}, state: {:>9}",
                                 i->name, i->description,
                                 pa_sample_format_to_string(i->sample_spec.format), i->sample_spec.rate, i->sample_spec.channels,
                                 pulse_source_state_to_string(i->state));
    }

    pulse_signal(0);
}

void pulse_init()
{
    std::lock_guard lock(pulse_mtx);

    if (pulse_loop == nullptr)
    {
        pulse_loop = pa_threaded_mainloop_new();
        pa_threaded_mainloop_start(pulse_loop);

        pulse_loop_lock();
        defer(pulse_loop_unlock());

        pulse_ctx = pa_context_new(pa_threaded_mainloop_get_api(pulse_loop), "FFMPEG_EXAMPLES_TEST");

        pa_context_set_state_callback(pulse_ctx, pulse_context_state_callback, nullptr);

        // A context must be connected to a server before any operation can be issued.
        // Calling pa_context_connect() will initiate the connection procedure.
        pa_context_connect(pulse_ctx, NULL, PA_CONTEXT_NOAUTOSPAWN, NULL);
    }
}

void pulse_unref()
{
    std::lock_guard lock(pulse_mtx);

    pulse_loop_lock();

    if (pulse_ctx != nullptr)
    {
        pa_context_disconnect(pulse_ctx);
        pa_context_unref(pulse_ctx);

        pulse_ctx = nullptr;
    }

    pulse_loop_unlock();

    if (pulse_loop != nullptr)
    {
        pa_threaded_mainloop_stop(pulse_loop);
        pa_threaded_mainloop_free(pulse_loop);

        pulse_loop = nullptr;
    }
}

void pulse_loop_lock()
{
    pa_threaded_mainloop_lock(pulse_loop);
}

void pulse_loop_unlock()
{
    pa_threaded_mainloop_unlock(pulse_loop);
}

bool pulse_context_is_ready()
{
    pulse_loop_lock();
    defer(pulse_loop_unlock());

    if (!PA_CONTEXT_IS_GOOD(pa_context_get_state(pulse_ctx)))
    {
        return false;
    }

    while (pa_context_get_state(pulse_ctx) != PA_CONTEXT_READY)
    {
        pa_threaded_mainloop_wait(pulse_loop);
    }

    return true;
}

void pulse_signal(int wait_for_accept)
{
    pa_threaded_mainloop_signal(pulse_loop, wait_for_accept);
}

int pulse_get_source_info_list()
{
    if (!pulse_context_is_ready())
    {
        return -1;
    }

    pulse_loop_lock();
    defer(pulse_loop_unlock());

    auto op = pa_context_get_source_info_list(
        pulse_ctx,
        pulse_source_info_callback,
        nullptr);

    if (!op)
    {
        return -1;
    }

    while (pa_operation_get_state(op) == PA_OPERATION_RUNNING)
    {
        pa_threaded_mainloop_wait(pulse_loop);
    }
    pa_operation_unref(op);

    return 0;
}

int pulse_get_sink_info_list()
{
    if (!pulse_context_is_ready())
    {
        return -1;
    }

    pulse_loop_lock();
    defer(pulse_loop_unlock());

    auto op = pa_context_get_sink_info_list(
        pulse_ctx,
        pulse_sink_info_callback,
        nullptr);

    if (!op)
    {
        return -1;
    }

    while (pa_operation_get_state(op) == PA_OPERATION_RUNNING)
    {
        pa_threaded_mainloop_wait(pulse_loop);
    }
    pa_operation_unref(op);

    return 0;
}

// // Field list is here: http://0pointer.de/lennart/projects/pulseaudio/doxygen/structpa__sink__info.html
// typedef struct pa_devicelist
// {
//     uint8_t initialized;
//     char name[512];
//     uint32_t index;
//     char description[256];
// } pa_devicelist_t;

// void pa_state_cb(pa_context *c, void *userdata);
// void pa_sinklist_cb(pa_context *c, const pa_sink_info *l, int eol, void *userdata);
// void pa_sourcelist_cb(pa_context *c, const pa_source_info *l, int eol, void *userdata);
// int pa_get_devicelist(pa_devicelist_t *input, pa_devicelist_t *output);

// // This callback gets called when our context changes state.  We really only
// // care about when it's ready or if it has failed
// void pa_state_cb(pa_context *c, void *userdata)
// {
//     pa_context_state_t state;
//     int *pa_ready = (int *)userdata;

//     state = pa_context_get_state(c);
//     switch (state)
//     {
//     // There are just here for reference
//     case PA_CONTEXT_UNCONNECTED:
//     case PA_CONTEXT_CONNECTING:
//     case PA_CONTEXT_AUTHORIZING:
//     case PA_CONTEXT_SETTING_NAME:
//     default:
//         break;
//     case PA_CONTEXT_FAILED:
//     case PA_CONTEXT_TERMINATED:
//         *pa_ready = 2;
//         break;
//     case PA_CONTEXT_READY:
//         *pa_ready = 1;
//         break;
//     }
// }

// // pa_mainloop will call this function when it's ready to tell us about a sink.
// // Since we're not threading, there's no need for mutexes on the devicelist
// // structure
// void pa_sinklist_cb(pa_context *c, const pa_sink_info *l, int eol, void *userdata)
// {
//     pa_devicelist_t *pa_devicelist = (pa_devicelist_t *)userdata;
//     int ctr = 0;

//     // If eol is set to a positive number, you're at the end of the list
//     if (eol > 0)
//     {
//         return;
//     }

//     printf("\n@OUTPUT{\n");
//     printf("\tname = %s\n", l->name);
//     printf("\tindex = %d\n", l->index);
//     printf("\tmute = %d\n", l->mute);
//     printf("\tdescription = %s\n", l->description);
//     printf("\tmonitor_source = %d\n", l->monitor_source);
//     printf("\tmonitor_source_name = %s\n", l->monitor_source_name);
//     printf("}\n");

//     // We know we've allocated 16 slots to hold devices.  Loop through our
//     // structure and find the first one that's "uninitialized."  Copy the
//     // contents into it and we're done.  If we receive more than 16 devices,
//     // they're going to get dropped.  You could make this dynamically allocate
//     // space for the device list, but this is a simple example.
//     for (ctr = 0; ctr < 16; ctr++)
//     {
//         if (!pa_devicelist[ctr].initialized)
//         {
//             strncpy(pa_devicelist[ctr].name, l->name, 511);
//             strncpy(pa_devicelist[ctr].description, l->description, 255);
//             pa_devicelist[ctr].index = l->index;
//             pa_devicelist[ctr].initialized = 1;
//             break;
//         }
//     }
// }

// // See above.  This callback is pretty much identical to the previous
// void pa_sourcelist_cb(pa_context *c, const pa_source_info *l, int eol, void *userdata)
// {
//     pa_devicelist_t *pa_devicelist = (pa_devicelist_t *)userdata;
//     int ctr = 0;

//     if (eol > 0)
//     {
//         return;
//     }

//     printf("\n@INPUT{\n");
//     printf("\tname = %s\n", l->name);
//     printf("\tindex = %d\n", l->index);
//     printf("\tmute = %d\n", l->mute);
//     printf("\tdescription = %s\n", l->description);
//     printf("\tmonitor_of_sink = %d\n", l->monitor_of_sink);
//     printf("\tmonitor_of_sink_name = %s\n", l->monitor_of_sink_name);
//     printf("}\n");

//     for (ctr = 0; ctr < 16; ctr++)
//     {
//         if (!pa_devicelist[ctr].initialized)
//         {
//             strncpy(pa_devicelist[ctr].name, l->name, 511);
//             strncpy(pa_devicelist[ctr].description, l->description, 255);
//             pa_devicelist[ctr].index = l->index;
//             pa_devicelist[ctr].initialized = 1;
//             break;
//         }
//     }
// }

// int pa_get_devicelist(pa_devicelist_t *input, pa_devicelist_t *output)
// {
//     // Define our pulse audio loop and connection variables
//     pa_mainloop *pa_ml;
//     pa_mainloop_api *pa_mlapi;
//     pa_operation *pa_op;
//     pa_context *pa_ctx;

//     // We'll need these state variables to keep track of our requests
//     int state = 0;
//     int pa_ready = 0;

//     // Initialize our device lists
//     memset(input, 0, sizeof(pa_devicelist_t) * 16);
//     memset(output, 0, sizeof(pa_devicelist_t) * 16);

//     // Create a mainloop API and connection to the default server
//     pa_ml = pa_mainloop_new();
//     pa_mlapi = pa_mainloop_get_api(pa_ml);
//     pa_ctx = pa_context_new(pa_mlapi, "test");

//     // This function connects to the pulse server
//     pa_context_connect(pa_ctx, NULL, PA_CONTEXT_NOFLAGS, NULL);

//     // This function defines a callback so the server will tell us it's state.
//     // Our callback will wait for the state to be ready.  The callback will
//     // modify the variable to 1 so we know when we have a connection and it's
//     // ready.
//     // If there's an error, the callback will set pa_ready to 2
//     pa_context_set_state_callback(pa_ctx, pa_state_cb, &pa_ready);

//     // Now we'll enter into an infinite loop until we get the data we receive
//     // or if there's an error
//     for (;;)
//     {
//         // We can't do anything until PA is ready, so just iterate the mainloop
//         // and continue
//         if (pa_ready == 0)
//         {
//             pa_mainloop_iterate(pa_ml, 1, NULL);
//             continue;
//         }
//         // We couldn't get a connection to the server, so exit out
//         if (pa_ready == 2)
//         {
//             pa_context_disconnect(pa_ctx);
//             pa_context_unref(pa_ctx);
//             pa_mainloop_free(pa_ml);
//             return -1;
//         }
//         // At this point, we're connected to the server and ready to make
//         // requests
//         switch (state)
//         {
//         // State 0: we haven't done anything yet
//         case 0:
//             // This sends an operation to the server.  pa_sinklist_info is
//             // our callback function and a pointer to our devicelist will
//             // be passed to the callback The operation ID is stored in the
//             // pa_op variable
//             pa_op = pa_context_get_sink_info_list(pa_ctx,
//                                                   pa_sinklist_cb,
//                                                   output);

//             // Update state for next iteration through the loop
//             state++;
//             break;
//         case 1:
//             // Now we wait for our operation to complete.  When it's
//             // complete our pa_output_devicelist is filled out, and we move
//             // along to the next state
//             if (pa_operation_get_state(pa_op) == PA_OPERATION_DONE)
//             {
//                 pa_operation_unref(pa_op);

//                 // Now we perform another operation to get the source
//                 // (input device) list just like before.  This time we pass
//                 // a pointer to our input structure
//                 pa_op = pa_context_get_source_info_list(pa_ctx,
//                                                         pa_sourcelist_cb,
//                                                         input);
//                 // Update the state so we know what to do next
//                 state++;
//             }
//             break;
//         case 2:
//             if (pa_operation_get_state(pa_op) == PA_OPERATION_DONE)
//             {
//                 // Now we're done, clean up and disconnect and return
//                 pa_operation_unref(pa_op);
//                 pa_context_disconnect(pa_ctx);
//                 pa_context_unref(pa_ctx);
//                 pa_mainloop_free(pa_ml);
//                 return 0;
//             }
//             break;
//         default:
//             // We should never see this state
//             fprintf(stderr, "in state %d\n", state);
//             return -1;
//         }
//         // Iterate the main loop and go again.  The second argument is whether
//         // or not the iteration should block until something is ready to be
//         // done.  Set it to zero for non-blocking.
//         pa_mainloop_iterate(pa_ml, 1, NULL);
//     }
// }
