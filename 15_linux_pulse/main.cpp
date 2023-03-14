#include "defer.h"
#include "linux-pulse.h"
#include "logging.h"

int main(int argc, char *argv[])
{
    pulse_init();
    defer(pulse_unref());

    LOG(INFO) << "pulse sources:";
    pulse_get_source_info_list();

    LOG(INFO) << "pulse sinks:";
    pulse_get_sink_info_list();

    return 0;
}
