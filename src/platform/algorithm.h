#pragma once

#ifdef NO_UEFI

#include <algorithm>

#else

namespace std
{

    template <class T>
    constexpr const T& max(const T& left, const T& right)
    {
        return left < right ? right : left;
    }

}

#endif
