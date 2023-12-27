#pragma once

struct EndResponse
{
    enum {
        type = 35,
    };
};

struct TryAgain // Must be returned if _dejavu is not 0, and the incoming packet cannot be processed (usually when incoming packets queue is full)
{
    enum {
        type = 46,
    };
};
