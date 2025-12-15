#pragma once

enum class SortingOrder
{
    SortAscending,
    SortDescending,
};

// Lomuto's partition scheme for quick sort:
// Uses the last element in the range as pivot. Swaps elements until all elements that should go before the pivot
// (according to the sorting order) are on the left side of the pivot and all others are on the right side of the pivot.
// Returns the index of the pivot in the range after partitioning.
template <typename T>
unsigned int partition(T* range, int first, int last, SortingOrder order)
{
    constexpr auto swap = [](T& a, T& b) { T tmp = b; b = a; a = tmp; };
    
    T pivot = range[last];

    // Next available index to swap to. Elements with indices < nextIndex are certain to go before the pivot. 
    int nextIndex = first;
    for (int i = first; i < last; ++i)
    {
        bool shouldGoBefore = range[i] < pivot; // SortAscending
        if (order == SortingOrder::SortDescending)
            shouldGoBefore = !shouldGoBefore;

        if (shouldGoBefore)
        {
            swap(range[nextIndex], range[i]);
            ++nextIndex;
        }
    }

    // move pivot after all elements that should go before the pivot
    swap(range[nextIndex], range[last]);
    
    return nextIndex;
}

// Sorts the elements from range[first] to range[last] according to the given `order`.
// The sorting happens in place and requires type T to have the comparison operator < defined.
template <typename T>
void quickSort(T* range, int first, int last, SortingOrder order)
{
    if (first >= last)
        return;

    // pivot is the partitioning index, range[pivot] is in correct position 
    unsigned int pivot = partition(range, first, last, order);

    // recursively sort smaller ranges to the left and right of the pivot
    quickSort(range, first, pivot - 1, order);
    quickSort(range, pivot + 1, last, order);

    return;
}
