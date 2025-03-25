#include <lib/platform_common/sleep.h>

#include <chrono>
#include <thread>

void sleepMilliseconds(unsigned int milliseconds)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}
