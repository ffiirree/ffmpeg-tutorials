#include <fmt/format.h>
#include "logging.h"
#include "linux-v4l2.h"

// v4l2-ctl --all
int main(int argc, char *argv[])
{
    auto devices = v4l2_device_list();

    for (auto& device : devices) {
        LOG(INFO) << fmt::format("{}: {}({})", device.name_, device.card_, device.bus_info_);
    }
    return 0;
}