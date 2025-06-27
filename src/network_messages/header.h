#pragma once

#include <lib/platform_common/qintrin.h>
#include <lib/platform_common/compiler_warnings.h>

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
    inline unsigned int size() const
    {
        return (*((unsigned int*)_size)) & 0xFFFFFF;
    }

    // Set message size with compile-time check
    template<unsigned int size>
    constexpr inline void setSize()
    {
        SUPPRESS_WARNINGS_BEGIN
        IGNORE_CONVERSION_DATALOSS_WARNING
        static_assert(size <= max_size);
        _size[0] = (unsigned char)size;
        _size[1] = (unsigned char)(size >> 8);
        _size[2] = (unsigned char)(size >> 16);
        SUPPRESS_WARNINGS_END
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

    // Check if dejavue is 0. A zero dejavu is used to signal that a message should be distributed to other peers.
    inline bool isDejavuZero() const
    {
        return !_dejavu;
    }

    inline unsigned int dejavu() const
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

    inline unsigned char type() const
    {
        return _type;
    }

    inline void setType(const unsigned char type)
    {
        _type = type;
    }

    // Return pointer to payload, which is stored behind the header.
    // The type() is not checked against the PayloadType!
    template <typename PayloadType>
    inline PayloadType* getPayload()
    {
        return reinterpret_cast<PayloadType*>(this + 1);
    }

    // Check if the payload size is as expected.
    inline bool checkPayloadSize(unsigned int expected_payload_size) const
    {
        return size() == expected_payload_size + sizeof(RequestResponseHeader);
    }

    // Check if the payload size is in the expected range.
    inline bool checkPayloadSizeMinMax(unsigned int min_payload_size, unsigned int max_payload_size) const
    {
        return min_payload_size + sizeof(RequestResponseHeader) <= size() && size() <= max_payload_size + sizeof(RequestResponseHeader);
    }

    // Get size of the payload (without checking validity of overall size).
    inline unsigned int getPayloadSize() const
    {
        return this->size() - sizeof(RequestResponseHeader);
    }
};
