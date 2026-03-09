#pragma once

#include "lib/platform_common/sorting.h"

// Calculates percentile value from array (in-place sort)
// Returns value at position: (count * Numerator) / Denominator
template <typename T, unsigned int Numerator, unsigned int Denominator,
          SortingOrder Order = SortingOrder::SortAscending>
T calculatePercentileValue(T* values, unsigned int count)
{
    static_assert(Denominator > 0, "Denominator must be greater than 0");
    static_assert(Numerator < Denominator, "Numerator must be < Denominator");

    if (count == 0)
    {
        return T(0);
    }

    quickSort(values, 0, count - 1, Order);

    unsigned int percentileIndex = (count * Numerator) / Denominator;

    return values[percentileIndex];
}

// Calculates 2/3 quorum value with ascending sort order
template <typename T>
T calculateAscendingQuorumValue(T* values, unsigned int count)
{
    return calculatePercentileValue<T, 2, 3, SortingOrder::SortAscending>(values, count);
}
