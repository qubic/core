// Implements functions of QPI::LinkedList in order to:
// 1. keep setMem() and copyMem() unavailable to contracts
// 2. keep QPI file smaller and easier to read for contract devs
// CAUTION: Include this AFTER the contract implementations!

#pragma once

#include "../contracts/qpi.h"
#include "../platform/memory.h"

namespace QPI
{
	template <typename T, uint64 L>
	bool LinkedList<T, L>::_isValidAndOccupied(sint64 elementIndex) const
	{
		if (elementIndex < 0 || elementIndex >= (sint64)L)
			return false;
		return (_occupiedFlags[elementIndex >> 6] & (1ULL << (elementIndex & 63))) != 0;
	}

	template <typename T, uint64 L>
	void LinkedList<T, L>::_initIfNeeded()
	{
		// Contract state is zero-initialized (no constructor runs). Detect this case
		// and set sentinel values. Safe to check: _nextUnusedIndex only increases and
		// is never 0 again after the first add (unless reset() is called, which already
		// sets the sentinels correctly).
		if (_population == 0 && _nextUnusedIndex == 0)
		{
			_headIndex = NULL_INDEX;
			_tailIndex = NULL_INDEX;
			_freeHeadIndex = NULL_INDEX;
		}
	}

	template <typename T, uint64 L>
	sint64 LinkedList<T, L>::_allocateNode()
	{
		_initIfNeeded();

		// First try to recycle a freed node
		if (_freeHeadIndex != NULL_INDEX)
		{
			sint64 nodeIndex = _freeHeadIndex;
			_freeHeadIndex = _nodes[nodeIndex].nextIndex;
			return nodeIndex;
		}

		// Then try a never-used node
		if (_nextUnusedIndex < L)
		{
			sint64 nodeIndex = (sint64)_nextUnusedIndex;
			_nextUnusedIndex++;
			return nodeIndex;
		}

		// List is full
		return NULL_INDEX;
	}

	template <typename T, uint64 L>
	void LinkedList<T, L>::_freeNode(sint64 nodeIndex)
	{
		// Clear occupied bit
		_occupiedFlags[nodeIndex >> 6] &= ~(1ULL << (nodeIndex & 63));

		// Zero the value for security (prevents data leaking between contract calls)
		setMem(&_nodes[nodeIndex].value, sizeof(T), 0);

		// Push onto free list (reuse nextIndex as free list pointer)
		_nodes[nodeIndex].nextIndex = _freeHeadIndex;
		_nodes[nodeIndex].prevIndex = NULL_INDEX;
		_freeHeadIndex = nodeIndex;
	}

	template <typename T, uint64 L>
	inline uint64 LinkedList<T, L>::population() const
	{
		return _population;
	}

	template <typename T, uint64 L>
	inline sint64 LinkedList<T, L>::headIndex() const
	{
		return (_population == 0) ? NULL_INDEX : _headIndex;
	}

	template <typename T, uint64 L>
	inline sint64 LinkedList<T, L>::tailIndex() const
	{
		return (_population == 0) ? NULL_INDEX : _tailIndex;
	}

	template <typename T, uint64 L>
	sint64 LinkedList<T, L>::nextElementIndex(sint64 elementIndex) const
	{
		if (!_isValidAndOccupied(elementIndex))
			return NULL_INDEX;
		return _nodes[elementIndex].nextIndex;
	}

	template <typename T, uint64 L>
	sint64 LinkedList<T, L>::prevElementIndex(sint64 elementIndex) const
	{
		if (!_isValidAndOccupied(elementIndex))
			return NULL_INDEX;
		return _nodes[elementIndex].prevIndex;
	}

	template <typename T, uint64 L>
	inline const T& LinkedList<T, L>::element(sint64 elementIndex) const
	{
		return _nodes[elementIndex & (L - 1)].value;
	}

	template <typename T, uint64 L>
	inline bool LinkedList<T, L>::isEmptySlot(sint64 elementIndex) const
	{
		return !_isValidAndOccupied(elementIndex);
	}

	template <typename T, uint64 L>
	sint64 LinkedList<T, L>::addHead(const T& value)
	{
		sint64 nodeIndex = _allocateNode();
		if (nodeIndex == NULL_INDEX)
			return NULL_INDEX;

		// Set occupied bit
		_occupiedFlags[nodeIndex >> 6] |= (1ULL << (nodeIndex & 63));

		// Initialize node
		_nodes[nodeIndex].value = value;
		_nodes[nodeIndex].prevIndex = NULL_INDEX;
		_nodes[nodeIndex].nextIndex = _headIndex;

		// Link into list
		if (_headIndex != NULL_INDEX)
			_nodes[_headIndex].prevIndex = nodeIndex;
		else
			_tailIndex = nodeIndex;  // List was empty, new node is also the tail

		_headIndex = nodeIndex;
		_population++;

		return nodeIndex;
	}

	template <typename T, uint64 L>
	sint64 LinkedList<T, L>::addTail(const T& value)
	{
		sint64 nodeIndex = _allocateNode();
		if (nodeIndex == NULL_INDEX)
			return NULL_INDEX;

		// Set occupied bit
		_occupiedFlags[nodeIndex >> 6] |= (1ULL << (nodeIndex & 63));

		// Initialize node
		_nodes[nodeIndex].value = value;
		_nodes[nodeIndex].prevIndex = _tailIndex;
		_nodes[nodeIndex].nextIndex = NULL_INDEX;

		// Link into list
		if (_tailIndex != NULL_INDEX)
			_nodes[_tailIndex].nextIndex = nodeIndex;
		else
			_headIndex = nodeIndex;  // List was empty, new node is also the head

		_tailIndex = nodeIndex;
		_population++;

		return nodeIndex;
	}

	template <typename T, uint64 L>
	sint64 LinkedList<T, L>::insertAfter(sint64 elementIndex, const T& value)
	{
		if (!_isValidAndOccupied(elementIndex))
			return NULL_INDEX;

		// If inserting after the tail, delegate to addTail
		if (elementIndex == _tailIndex)
			return addTail(value);

		sint64 nodeIndex = _allocateNode();
		if (nodeIndex == NULL_INDEX)
			return NULL_INDEX;

		// Set occupied bit
		_occupiedFlags[nodeIndex >> 6] |= (1ULL << (nodeIndex & 63));

		// Get the next node after the target
		sint64 nextIdx = _nodes[elementIndex].nextIndex;

		// Initialize new node
		_nodes[nodeIndex].value = value;
		_nodes[nodeIndex].prevIndex = elementIndex;
		_nodes[nodeIndex].nextIndex = nextIdx;

		// Re-link neighbors
		_nodes[elementIndex].nextIndex = nodeIndex;
		if (nextIdx != NULL_INDEX)
			_nodes[nextIdx].prevIndex = nodeIndex;

		_population++;

		return nodeIndex;
	}

	template <typename T, uint64 L>
	sint64 LinkedList<T, L>::insertBefore(sint64 elementIndex, const T& value)
	{
		if (!_isValidAndOccupied(elementIndex))
			return NULL_INDEX;

		// If inserting before the head, delegate to addHead
		if (elementIndex == _headIndex)
			return addHead(value);

		sint64 nodeIndex = _allocateNode();
		if (nodeIndex == NULL_INDEX)
			return NULL_INDEX;

		// Set occupied bit
		_occupiedFlags[nodeIndex >> 6] |= (1ULL << (nodeIndex & 63));

		// Get the previous node before the target
		sint64 prevIdx = _nodes[elementIndex].prevIndex;

		// Initialize new node
		_nodes[nodeIndex].value = value;
		_nodes[nodeIndex].prevIndex = prevIdx;
		_nodes[nodeIndex].nextIndex = elementIndex;

		// Re-link neighbors
		_nodes[elementIndex].prevIndex = nodeIndex;
		if (prevIdx != NULL_INDEX)
			_nodes[prevIdx].nextIndex = nodeIndex;

		_population++;

		return nodeIndex;
	}

	template <typename T, uint64 L>
	void LinkedList<T, L>::remove(sint64 elementIndex)
	{
		if (!_isValidAndOccupied(elementIndex))
			return;

		sint64 prevIdx = _nodes[elementIndex].prevIndex;
		sint64 nextIdx = _nodes[elementIndex].nextIndex;

		// Unlink from doubly-linked list
		if (prevIdx != NULL_INDEX)
			_nodes[prevIdx].nextIndex = nextIdx;
		else
			_headIndex = nextIdx;  // Removing head

		if (nextIdx != NULL_INDEX)
			_nodes[nextIdx].prevIndex = prevIdx;
		else
			_tailIndex = prevIdx;  // Removing tail

		_population--;

		// Return node to free list
		_freeNode(elementIndex);
	}

	template <typename T, uint64 L>
	bool LinkedList<T, L>::replace(sint64 elementIndex, const T& newValue)
	{
		if (!_isValidAndOccupied(elementIndex))
			return false;

		_nodes[elementIndex].value = newValue;
		return true;
	}

	template <typename T, uint64 L>
	void LinkedList<T, L>::reset()
	{
		setMem(this, sizeof(*this), 0);
		_headIndex = NULL_INDEX;
		_tailIndex = NULL_INDEX;
		_freeHeadIndex = NULL_INDEX;
	}
}
