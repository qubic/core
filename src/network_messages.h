#pragma once

#include <intrin.h>

struct RequestResponseHeader
{
private:
    unsigned char _size[3];
    unsigned char _type;
    unsigned int _dejavu;

public:
    // The maximum size that a message may have (encoded in 3 bytes)
    static constexpr unsigned int max_size = 0xFFFFFF;

    // Return the size of the message
    inline unsigned int size()
    {
        return (*((unsigned int*)_size)) & 0xFFFFFF;
    }

    // Set message size with compile-time check
    template<unsigned int size>
    constexpr inline void setSize()
    {
        static_assert(size <= max_size);
        _size[0] = (unsigned char)size;
        _size[1] = (unsigned char)(size >> 8);
        _size[2] = (unsigned char)(size >> 16);
    }

    // Set message size with run-time check of size (returns false if message is too big)
    inline bool checkAndSetSize(unsigned int size)
    {
        if (size > max_size)
            return false;

        _size[0] = (unsigned char)size;
        _size[1] = (unsigned char)(size >> 8);
        _size[2] = (unsigned char)(size >> 16);
        return true;
    }

    inline bool isDejavuZero()
    {
        return !_dejavu;
    }

    inline unsigned int dejavu()
    {
        return _dejavu;
    }

    inline void setDejavu(unsigned int dejavu)
    {
        _dejavu = dejavu;
    }

    inline void randomizeDejavu()
    {
        _rdrand32_step(&_dejavu);
        if (!_dejavu)
        {
            _dejavu = 1;
        }
    }

    inline unsigned char type()
    {
        return _type;
    }

    inline void setType(const unsigned char type)
    {
        _type = type;
    }
};

#define EXCHANGE_PUBLIC_PEERS 0
#define NUMBER_OF_EXCHANGED_PEERS 4

typedef struct
{
    unsigned char peers[NUMBER_OF_EXCHANGED_PEERS][4];
} ExchangePublicPeers;


struct TryAgain // Must be returned if _dejavu is not 0, and the incoming packet cannot be processed (usually when incoming packets queue is full)
{
    static constexpr unsigned char type()
    {
        return 46;
    }
};