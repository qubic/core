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

template <typename T>
struct CompareLess
{
    constexpr bool operator()(const T& lhs, const T& rhs) const
    {
        return lhs < rhs;
    }
};

template <typename T>
struct CompareGreater
{
    constexpr bool operator()(const T& lhs, const T& rhs) const
    {
        return lhs > rhs;
    }
};

// Data structure that provides efficient access to the min element (useful for priority queues).
template <typename T, unsigned int Capacity, class Compare = CompareLess<T>>
class MinHeap
{
public:
    void init(Compare compare = Compare())
    {
        _size = 0;
        _compare = compare;
    }

    // Insert new element. Returns true on success.
    bool insert(const T& newElement)
    {
        if (_size >= capacity())
            return false;
        const unsigned int newIndex = _size++;
        _data[newIndex] = newElement;
        upHeap(newIndex);
        return true;
    }

    // Extract minimum element (copy to output reference and remove it from the heap). Returns true on success.
    inline bool extract(T& minElement)
    {
        return peek(minElement) && drop();
    }

    // Remove minimum element. Returns true on success.
    bool drop()
    {
        if (_size == 0)
            return false;
        _data[0] = _data[--_size];
        downHeap(0);
        return true;
    }

    // Extract minimum element and add a new element in one operation (more efficient than extract followed by insert).
    // Returns true on success.
    bool replace(T& minElement, const T& newElement)
    {
        if (_size == 0)
            return false;
        minElement = _data[0];
        _data[0] = newElement;
        downHeap(0);
        return true;
    }

    // Remove first elementToRemove that is found. Requires T::operator==(). Least efficient operation of MinHeap with O(N).
    bool removeFirstMatch(const T& elementToRemove)
    {
        // find element
        unsigned int i = 0;
        for (; i < _size; ++i)
        {
            if (_data[i] == elementToRemove)
                break;
        }
        if (i == _size)
            return false;

        // replace it by the last element and heapify
        --_size;
        if (i < _size)
        {
            _data[i] = _data[_size];
            i = upHeap(i);
            downHeap(i);
        }
        return true;
    }

    // Get minimum element (copy to output reference and WITHOUT removing it from the heap). Returns true on success.
    bool peek(T& minElement) const
    {
        if (_size == 0)
            return false;
        minElement = _data[0];
        return true;
    }

    // Return current number of elements in heap.
    unsigned int size() const
    {
        return _size;
    }

    // Return pointer to elements in heap.
    const T* data() const
    {
        return _data;
    }

    // Return maximum number of elements that can be stored in the heap.
    constexpr unsigned int capacity() const
    {
        return Capacity;
    }

protected:
    void swap(unsigned int idx1, unsigned int idx2)
    {
        const T tmp = _data[idx1];
        _data[idx1] = _data[idx2];
        _data[idx2] = tmp;
    }

    unsigned int upHeap(unsigned int idx)
    {
        while (idx != 0)
        {
            const unsigned int parentIdx = (idx - 1) / 2;
            if (_compare(_data[idx], _data[parentIdx]))
            {
                swap(idx, parentIdx);
                idx = parentIdx;
            }
            else
            {
                break;
            }
        }
        return idx;
    }

    void downHeap(unsigned int idx)
    {
        while (1)
        {
            const unsigned int childIdx1 = idx * 2 + 1;
            const unsigned int childIdx2 = childIdx1 + 1;
            if (childIdx1 >= _size)
                return;
            unsigned int minChildIdx = childIdx1;
            if (childIdx2 < _size && _compare(_data[childIdx2], _data[childIdx1]))
                minChildIdx = childIdx2;
            if (_compare(_data[minChildIdx], _data[idx]))
            {
                swap(idx, minChildIdx);
                idx = minChildIdx;
            }
            else
            {
                return;
            }
        }
    }

    T _data[Capacity];
    unsigned int _size;
    Compare _compare;
};
