#pragma once

struct EndResponse
{
    static constexpr unsigned char type()
    {
        return NetworkMessageType::END_RESPONSE;
    }
};

// Must be returned if _dejavu is not 0, and the incoming packet 
// cannot be processed (usually when incoming packets queue is full)
struct TryAgain
{
    static constexpr unsigned char type()
    {
        return NetworkMessageType::TRY_AGAIN;
    }
};
