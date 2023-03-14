#ifndef LINUX_PULSE
#define LINUX_PULSE

extern "C" {
    #include <pulse/pulseaudio.h>
}

void pulse_init();
void pulse_unref();

void pulse_loop_lock();
void pulse_loop_unlock();

bool pulse_context_is_ready();

void pulse_signal(int);

int pulse_get_source_info_list();
int pulse_get_sink_info_list();

void pulse_get_source_info();
void pulse_get_sink_info();

#endif