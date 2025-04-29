#include <lib/platform_common/sleep.h>

#include <chrono>
#include <thread>

void sleepMicroseconds(unsigned int microseconds)
{
    std::this_thread::sleep_for(std::chrono::microseconds(microseconds));
}

void sleepMilliseconds(unsigned int milliseconds)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}
