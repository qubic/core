// Implements functions of QPI::Collection in order to:
// 1. keep setMem() and copyMem() unavailable to contracts
// 2. keep QPI file smaller and easier to read for contract devs
// CAUTION: Include this AFTER the contract implementations!

#pragma once

#include "../contracts/qpi.h"
#include "../platform/memory.h"

namespace QPI
{
	template <typename T, uint64 L>
	void Collection<T, L>::_softReset()
	{
		setMem(_povs, sizeof(_povs), 0);
		setMem(_povOccupationFlags, sizeof(_povOccupationFlags), 0);
		_population = 0;
		_markRemovalCounter = 0;
	}

	template <typename T, uint64 L>
	sint64 Collection<T, L>::_povIndex(const id& pov) const
	{
		sint64 povIndex = pov.u64._0 & (L - 1);
		for (sint64 counter = 0; counter < L; counter += 32)
		{
			uint64 flags = _getEncodedPovOccupationFlags(_povOccupationFlags, povIndex);
			for (auto i = 0; i < _nEncodedFlags; i++, flags >>= 2)
			{
				switch (flags & 3ULL)
				{
				case 0:
					return NULL_INDEX;
				case 1:
					if (_povs[povIndex].value == pov)
					{
						return povIndex;
					}
					break;
				}
				povIndex = (povIndex + 1) & (L - 1);
			}
		}
		return NULL_INDEX;
	}

	template <typename T, uint64 L>
	sint64 Collection<T, L>::_headIndex(const sint64 povIndex, const sint64 maxPriority) const
	{
		// with current code path, pov is not empty here
		const auto& pov = _povs[povIndex];

		// quick check head/tail
		if (_elements[pov.headIndex].priority <= maxPriority)
		{
			return pov.headIndex;
		}
		if (_elements[pov.tailIndex].priority > maxPriority)
		{
			return NULL_INDEX;
		}

		// here, head's priority > maxPriority >= tail's priority
		// => always found a valid element

		// search index of parent element
		// - always found parent element because pov is not empty
		sint64 idx = _searchElement(pov.bstRootIndex, maxPriority);
		if (_elements[idx].priority > maxPriority)
		{
			// forward iterating until meet element having priority <= maxPriority
			while (true)
			{
				idx = _nextElementIndex(idx);
				if (_elements[idx].priority <= maxPriority)
				{
					break;
				}
			}
			return idx;
		}

		// backward iterating until meet element having priority > maxPriority
		while (true)
		{
			sint64 prevIdx = _previousElementIndex(idx);
			if (prevIdx == NULL_INDEX || _elements[prevIdx].priority > maxPriority)
			{
				break;
			}
			idx = prevIdx;
		}
		return idx;
	}

	template <typename T, uint64 L>
	sint64 Collection<T, L>::_tailIndex(const sint64 povIndex, const sint64 minPriority) const
	{
		// with current code path, pov is not empty here
		const auto& pov = _povs[povIndex];

		// quick check head/tail
		if (_elements[pov.headIndex].priority < minPriority)
		{
			return NULL_INDEX;
		}
		if (_elements[pov.tailIndex].priority >= minPriority)
		{
			return pov.tailIndex;
		}

		// here, head's priority >= minPriority > tail's priority
		// => always found a valid element

		// search index of parent element
		// - always found parent element because pov is not empty
		sint64 idx = _searchElement(pov.bstRootIndex, minPriority);

		if (_elements[idx].priority >= minPriority)
		{
			// forward iterating until meet element having priority < minPriority
			while (true)
			{
				sint64 nextIdx = _nextElementIndex(idx);
				if (nextIdx == NULL_INDEX || _elements[nextIdx].priority < minPriority)
				{
					break;
				}
				idx = nextIdx;
			}
			return idx;
		}

		// backward iterating to meet element having priority >= minPriority
		while (true)
		{
			idx = _previousElementIndex(idx);
			if (_elements[idx].priority >= minPriority)
			{
				break;
			}
		}
		return idx;
	}

	template <typename T, uint64 L>
	sint64 Collection<T, L>::_searchElement(const sint64 bstRootIndex,
		const sint64 priority, int* pIterationsCount) const
	{
		sint64 idx = bstRootIndex;
		while (idx != NULL_INDEX)
		{
			if (pIterationsCount)
			{
				*pIterationsCount += 1;
			}
			auto& curElement = _elements[idx];
			if (curElement.priority >= priority)
			{
				if (curElement.bstRightIndex != NULL_INDEX)
				{
					idx = curElement.bstRightIndex;
				}
				else
				{
					return idx;
				}
			}
			else
			{
				if (curElement.bstLeftIndex != NULL_INDEX)
				{
					idx = curElement.bstLeftIndex;
				}
				else
				{
					return idx;
				}
			}
		}
		return NULL_INDEX;
	}

	template <typename T, uint64 L>
	sint64 Collection<T, L>::_addPovElement(const sint64 povIndex, const T value, const sint64 priority)
	{
		const sint64 newElementIdx = _population++;
		auto& newElement = _elements[newElementIdx].init(value, priority, povIndex);
		auto& pov = _povs[povIndex];

		if (pov.population == 0)
		{
			pov.population = 1;
			pov.headIndex = newElementIdx;
			pov.tailIndex = newElementIdx;
			pov.bstRootIndex = newElementIdx;
		}
		else
		{
			int iterations_count = 0;
			sint64 parentIdx = _searchElement(pov.bstRootIndex, priority, &iterations_count);
			if (_elements[parentIdx].priority >= priority)
			{
				_elements[parentIdx].bstRightIndex = newElementIdx;
			}
			else
			{
				_elements[parentIdx].bstLeftIndex = newElementIdx;
			}
			newElement.bstParentIndex = parentIdx;
			pov.population++;


			if (_elements[pov.headIndex].priority < priority)
			{
				pov.headIndex = newElementIdx;
			}
			else if (_elements[pov.tailIndex].priority >= priority)
			{
				pov.tailIndex = newElementIdx;
			}
			if (pov.population > 32 && iterations_count > pov.population / 4)
			{
				// make balanced binary search tree to get better performance
				pov.bstRootIndex = _rebuild(pov.bstRootIndex);
			}
		}
		return newElementIdx;
	}

	template <typename T, uint64 L>
	uint64 Collection<T, L>::_getSortedElements(const sint64 rootIdx, sint64* sortedElementIndices) const
	{
		uint64 count = 0;
		sint64 elementIdx = rootIdx;
		sint64 lastElementIdx = NULL_INDEX;
		while (elementIdx != NULL_INDEX)
		{
			if (lastElementIdx == _elements[elementIdx].bstParentIndex)
			{
				if (_elements[elementIdx].bstLeftIndex != NULL_INDEX)
				{
					lastElementIdx = elementIdx;
					elementIdx = _elements[elementIdx].bstLeftIndex;
					continue;
				}
				lastElementIdx = NULL_INDEX;
			}
			if (lastElementIdx == _elements[elementIdx].bstLeftIndex)
			{
				sortedElementIndices[count++] = elementIdx;

				if (_elements[elementIdx].bstRightIndex != NULL_INDEX)
				{
					lastElementIdx = elementIdx;
					elementIdx = _elements[elementIdx].bstRightIndex;
					continue;
				}
				lastElementIdx = NULL_INDEX;
			}
			if (lastElementIdx == _elements[elementIdx].bstRightIndex)
			{
				lastElementIdx = elementIdx;
				elementIdx = _elements[elementIdx].bstParentIndex;
			}
		}
		return count;
	}

	template <typename T, uint64 L>
	inline void Collection<T, L>::_set(sint64_4& vec, sint64 v0, sint64 v1, sint64 v2, sint64 v3) const
	{
		vec.set(0, v0);
		vec.set(1, v1);
		vec.set(2, v2);
		vec.set(3, v3);
	}

	template <typename T, uint64 L>
	sint64 Collection<T, L>::_rebuild(sint64 rootIdx)
	{
		auto* sortedElementIndices = reinterpret_cast<sint64*>(::__scratchpad());
		if (sortedElementIndices == NULL)
		{
			return rootIdx;
		}
		sint64 n = _getSortedElements(rootIdx, sortedElementIndices);
		if (!n)
		{
			return rootIdx;
		}
		// initialize root
		sint64 mid = n / 2;
		rootIdx = sortedElementIndices[mid];
		_elements[rootIdx].bstParentIndex = NULL_INDEX;
		_elements[rootIdx].bstLeftIndex = NULL_INDEX;
		_elements[rootIdx].bstRightIndex = NULL_INDEX;
		// initialize queue
		auto* queue = reinterpret_cast<sint64_4*>(sortedElementIndices + ((n + 3) / 4) * 4);
		sint64 dequeueIdx = 0;
		sint64 enqueueIdx = 0;
		sint64 queueSize = 0;
		// push left and right ranges to the queue
		if (mid > 0)
		{
			_set(queue[enqueueIdx], rootIdx, 0, mid - 1, mid);
			enqueueIdx = (enqueueIdx + 1) & (L - 1);
			queueSize++;
		}
		if (mid + 1 < n)
		{
			_set(queue[enqueueIdx], rootIdx, mid + 1, n - 1, mid);
			enqueueIdx = (enqueueIdx + 1) & (L - 1);
			queueSize++;
		}
		while (queueSize > 0)
		{
			// get the front element from the queue
			auto curRange = queue[dequeueIdx];
			dequeueIdx = (dequeueIdx + 1) & (L - 1);
			queueSize--;

			// get the parent node and range
			const auto parentElementIdx = curRange.get(0);
			const auto left = curRange.get(1);
			const auto right = curRange.get(2);

			if (left <= right) // if there are elements to process
			{
				mid = (left + right) / 2;
				const auto elementIdx = sortedElementIndices[mid];
				_elements[elementIdx].bstParentIndex = parentElementIdx;
				_elements[elementIdx].bstLeftIndex = NULL_INDEX;
				_elements[elementIdx].bstRightIndex = NULL_INDEX;

				// set the child node for the parent node
				if (mid < curRange.get(3))
				{
					_elements[parentElementIdx].bstLeftIndex = elementIdx;
				}
				else
				{
					_elements[parentElementIdx].bstRightIndex = elementIdx;
				}

				// push left and right ranges to the queue
				if (mid > left)
				{
					_set(queue[enqueueIdx], elementIdx, left, mid - 1, mid);
					enqueueIdx = (enqueueIdx + 1) & (L - 1);
					queueSize++;
				}
				if (mid < right)
				{
					_set(queue[enqueueIdx], elementIdx, mid + 1, right, mid);
					enqueueIdx = (enqueueIdx + 1) & (L - 1);
					queueSize++;
				}
			}
		}

		return rootIdx;
	}

	template <typename T, uint64 L>
	sint64 Collection<T, L>::_getMostLeft(sint64 elementIdx) const
	{
		while (_elements[elementIdx].bstLeftIndex != NULL_INDEX)
		{
			elementIdx = _elements[elementIdx].bstLeftIndex;
		}
		return elementIdx;
	}

	template <typename T, uint64 L>
	sint64 Collection<T, L>::_getMostRight(sint64 elementIdx) const
	{
		while (_elements[elementIdx].bstRightIndex != NULL_INDEX)
		{
			elementIdx = _elements[elementIdx].bstRightIndex;
		}
		return elementIdx;
	}

	template <typename T, uint64 L>
	sint64 Collection<T, L>::_previousElementIndex(sint64 elementIdx) const
	{
		elementIdx &= (L - 1);
		if (uint64(elementIdx) < _population)
		{
			if (_elements[elementIdx].bstLeftIndex != NULL_INDEX)
			{
				return _getMostRight(_elements[elementIdx].bstLeftIndex);
			}
			else if (_elements[elementIdx].bstParentIndex != NULL_INDEX)
			{
				auto parentIdx = _elements[elementIdx].bstParentIndex;
				if (_elements[parentIdx].bstRightIndex == elementIdx)
				{
					return parentIdx;
				}
				if (_elements[parentIdx].bstLeftIndex == elementIdx)
				{
					while (parentIdx != NULL_INDEX && _elements[parentIdx].bstLeftIndex == elementIdx)
					{
						elementIdx = parentIdx;
						parentIdx = _elements[elementIdx].bstParentIndex;
					}
					return parentIdx;
				}
			}
		}
		return NULL_INDEX;
	}

	template <typename T, uint64 L>
	sint64 Collection<T, L>::_nextElementIndex(sint64 elementIdx) const
	{
		elementIdx &= (L - 1);
		if (uint64(elementIdx) < _population)
		{
			if (_elements[elementIdx].bstRightIndex != NULL_INDEX)
			{
				return _getMostLeft(_elements[elementIdx].bstRightIndex);
			}
			else if (_elements[elementIdx].bstParentIndex != NULL_INDEX)
			{
				auto parentIdx = _elements[elementIdx].bstParentIndex;
				if (_elements[parentIdx].bstLeftIndex == elementIdx)
				{
					return parentIdx;
				}
				if (_elements[parentIdx].bstRightIndex == elementIdx)
				{
					while (parentIdx != NULL_INDEX && _elements[parentIdx].bstRightIndex == elementIdx)
					{
						elementIdx = parentIdx;
						parentIdx = _elements[elementIdx].bstParentIndex;
					}
					return parentIdx;
				}
			}
		}
		return NULL_INDEX;
	}

	template <typename T, uint64 L>
	bool Collection<T, L>::_updateParent(const sint64 elementIdx, const sint64 newElementIdx)
	{
		if (elementIdx != NULL_INDEX)
		{
			auto& curElement = _elements[elementIdx];
			if (curElement.bstParentIndex != NULL_INDEX)
			{
				auto& parentElement = _elements[curElement.bstParentIndex];
				if (parentElement.bstRightIndex == elementIdx)
				{
					parentElement.bstRightIndex = newElementIdx;
				}
				else
				{
					parentElement.bstLeftIndex = newElementIdx;
				}
				if (newElementIdx != NULL_INDEX)
				{
					_elements[newElementIdx].bstParentIndex = curElement.bstParentIndex;
				}
				return true;
			}
		}
		return false;
	}

	template <typename T, uint64 L>
	void Collection<T, L>::_moveElement(const sint64 srcIdx, const sint64 dstIdx)
	{
		copyMem(&_elements[dstIdx], &_elements[srcIdx], sizeof(_elements[0]));

		const auto povIndex = _elements[dstIdx].povIndex;
		auto& pov = _povs[povIndex];
		if (pov.bstRootIndex == srcIdx)
		{
			pov.bstRootIndex = dstIdx;
		}
		if (pov.headIndex == srcIdx)
		{
			pov.headIndex = dstIdx;
		}
		if (pov.tailIndex == srcIdx)
		{
			pov.tailIndex = dstIdx;
		}

		auto& element = _elements[dstIdx];
		if (element.bstLeftIndex != NULL_INDEX)
		{
			_elements[element.bstLeftIndex].bstParentIndex = dstIdx;
		}
		if (element.bstRightIndex != NULL_INDEX)
		{
			_elements[element.bstRightIndex].bstParentIndex = dstIdx;
		}
		if (element.bstParentIndex != NULL_INDEX)
		{
			auto& parentElement = _elements[element.bstParentIndex];
			if (parentElement.bstLeftIndex == srcIdx)
			{
				parentElement.bstLeftIndex = dstIdx;
			}
			else
			{
				parentElement.bstRightIndex = dstIdx;
			}
		}
	}

	template <typename T, uint64 L>
	uint64 Collection<T, L>::_getEncodedPovOccupationFlags(const uint64* povOccupationFlags, const sint64 povIndex) const
	{
		const sint64 offset = (povIndex & 31) << 1;
		uint64 flags = povOccupationFlags[povIndex >> 5] >> offset;
		if (offset > 0)
		{
			flags |= povOccupationFlags[((povIndex + 32) & (L - 1)) >> 5] << (2 * _nEncodedFlags - offset);
		}
		return flags;
	}

	template <typename T, uint64 L>
	sint64 Collection<T, L>::add(const id& pov, T element, sint64 priority)
	{
		if (_population < capacity())
		{
			// search in pov hash map
			sint64 markedForRemovalIndexForReuse = NULL_INDEX;
			sint64 povIndex = pov.u64._0 & (L - 1);
			for (sint64 counter = 0; counter < L; counter += 32)
			{
				uint64 flags = _getEncodedPovOccupationFlags(_povOccupationFlags, povIndex);
				for (auto i = 0; i < _nEncodedFlags; i++, flags >>= 2)
				{
					switch (flags & 3ULL)
					{
					case 0:
						// empty hash map entry -> pov isn't in map yet
						// If we have already seen an entry marked for removal, reuse this slot because it is closer to the hash index
						if (markedForRemovalIndexForReuse != NULL_INDEX)
							goto reuse_slot;
						// ... otherwise mark as occupied and init new priority queue with 1 element
						_povOccupationFlags[povIndex >> 5] |= (1ULL << ((povIndex & 31) << 1));
						_povs[povIndex].value = pov;
						return _addPovElement(povIndex, element, priority);
					case 1:
						if (_povs[povIndex].value == pov)
						{
							// found pov entry -> insert element in priority queue of pov
							return _addPovElement(povIndex, element, priority);
						}
						break;
					case 2:
						// marked for removal -> reuse slot (first slot we see) later if we are sure that key isn't in the set
						if (markedForRemovalIndexForReuse == NULL_INDEX)
							markedForRemovalIndexForReuse = povIndex;
						break;
					}
					povIndex = (povIndex + 1) & (L - 1);
				}
			}

			if (markedForRemovalIndexForReuse != NULL_INDEX)
			{
			reuse_slot:
				// Reuse slot marked for removal: put pov key here and set flags from 2 to 1.
				// But don't decrement _markRemovalCounter, because it is used to check if cleanup() is needed.
				// Without cleanup, we don't get new unoccupied slots and at least lookup of povs that aren't contained in the set
				// stays slow.
				povIndex = markedForRemovalIndexForReuse;
				_povOccupationFlags[povIndex >> 5] ^= (3ULL << ((povIndex & 31) << 1));
				_povs[povIndex].value = pov;
				return _addPovElement(povIndex, element, priority);
			}
		}
		return NULL_INDEX;
	}

	template <typename T, uint64 L>
	void Collection<T, L>::cleanupIfNeeded(uint64 removalThresholdPercent)
	{
		if (_markRemovalCounter > (removalThresholdPercent * L / 100))
		{
			cleanup();
		}
	}

	template <typename T, uint64 L>
	void Collection<T, L>::cleanup()
	{
		// _povs gets occupied over time with entries of type 3 which means they are marked for cleanup.
		// Once cleanup is called it's necessary to remove all these type 3 entries by reconstructing a fresh Collection residing in scratchpad buffer.
		// The _elements array is not reorganized by the cleanup (only references to _povs are updated).
		// Cleanup() called for a Collection having only type 3 entries in _povs must give the result equal to reset() memory content wise.

		// Quick check to cleanup
		if (!_markRemovalCounter)
		{
			return;
		}

		// Speedup case of empty Collection but existed marked for removal povs
		if (!population())
		{
			_softReset();
			return;
		}

		// Init buffers
		auto* _povsBuffer = reinterpret_cast<PoV*>(::__scratchpad());
		auto* _povOccupationFlagsBuffer = reinterpret_cast<uint64*>(_povsBuffer + L);
		auto* _stackBuffer = reinterpret_cast<sint64*>(
			_povOccupationFlagsBuffer + sizeof(_povOccupationFlags) / sizeof(_povOccupationFlags[0]));
		setMem(::__scratchpad(), sizeof(_povs) + sizeof(_povOccupationFlags), 0);
		uint64 newPopulation = 0;

		// Go through pov hash map. For each pov that is occupied but not marked for removal, insert pov in new Collection's pov buffers and
		// update povIndex in elements belonging to pov.
		constexpr uint64 oldPovIndexGroupCount = (L >> 5) ? (L >> 5) : 1;
		for (sint64 oldPovIndexGroup = 0; oldPovIndexGroup < oldPovIndexGroupCount; oldPovIndexGroup++)
		{
			const uint64 flags = _povOccupationFlags[oldPovIndexGroup];
			uint64 maskBits = (0xAAAAAAAAAAAAAAAA & (flags << 1));
			maskBits &= maskBits ^ (flags & 0xAAAAAAAAAAAAAAAA);
			sint64 oldPovIndexOffset = _tzcnt_u64(maskBits) & 0xFE;
			const sint64 oldPovIndexOffsetEnd = 64 - (_lzcnt_u64(maskBits) & 0xFE);
			for (maskBits >>= oldPovIndexOffset;
				oldPovIndexOffset < oldPovIndexOffsetEnd; oldPovIndexOffset += 2, maskBits >>= 2)
			{
				// Only add pov to new Collection that are occupied and not marked for removal
				if (maskBits & 3ULL)
				{
					// find empty position in new pov hash map
					const sint64 oldPovIndex = (oldPovIndexGroup << 5) + (oldPovIndexOffset >> 1);
					sint64 newPovIndex = _povs[oldPovIndex].value.u64._0 & (L - 1);
					for (sint64 counter = 0; counter < L; counter += 32)
					{
						QPI::uint64 newFlags = _getEncodedPovOccupationFlags(_povOccupationFlagsBuffer, newPovIndex);
						for (sint64 i = 0; i < _nEncodedFlags; i++, newFlags >>= 2)
						{
							if ((newFlags & 3ULL) == 0)
							{
								newPovIndex = (newPovIndex + i) & (L - 1);
								goto foundEmptyPosition;
							}
						}
						newPovIndex = (newPovIndex + _nEncodedFlags) & (L - 1);
					}
#ifdef NO_UEFI
					// should never be reached, because old and new map have same capacity (there should always be an empty slot)
					goto cleanupBug;
#endif

				foundEmptyPosition:
					// occupy empty pov hash map entry
					_povOccupationFlagsBuffer[newPovIndex >> 5] |= (1ULL << ((newPovIndex & 31) << 1));
					copyMem(&_povsBuffer[newPovIndex], &_povs[oldPovIndex], sizeof(PoV));

					// update newPovIndex for elements
					if (newPovIndex != oldPovIndex)
					{
						sint64 stackSize = 0;
						_stackBuffer[stackSize++] = _povsBuffer[newPovIndex].bstRootIndex;
						while (stackSize > 0)
						{
							auto& element = _elements[_stackBuffer[--stackSize]];
							element.povIndex = newPovIndex;
							if (element.bstLeftIndex != NULL_INDEX)
							{
								_stackBuffer[stackSize++] = element.bstLeftIndex;
							}
							if (element.bstRightIndex != NULL_INDEX)
							{
								_stackBuffer[stackSize++] = element.bstRightIndex;
							}
						}
					}

					// check if we are done
					newPopulation += _povs[oldPovIndex].population;
					if (newPopulation == _population)
					{
						// povs of all elements have been transferred -> overwrite old pov arrays with new pov arrays
						copyMem(_povs, _povsBuffer, sizeof(_povs));
						copyMem(_povOccupationFlags, _povOccupationFlagsBuffer, sizeof(_povOccupationFlags));
						_markRemovalCounter = 0;
						return;
					}
				}
			}
		}

#ifdef NO_UEFI
		cleanupBug :
		// don't expect here, certainly got error!!!
		printf("ERROR: Something went wrong at cleanup!\n");
#endif
	}

	template <typename T, uint64 L>
	inline T Collection<T, L>::element(sint64 elementIndex) const
	{
		return _elements[elementIndex & (L - 1)].value;
	}

	template <typename T, uint64 L>
	sint64 Collection<T, L>::headIndex(const id& pov) const
	{
		const sint64 povIndex = _povIndex(pov);

		return povIndex < 0 ? NULL_INDEX : _povs[povIndex].headIndex;
	}

	template <typename T, uint64 L>
	sint64 Collection<T, L>::headIndex(const id& pov, sint64 maxPriority) const
	{
		const sint64 povIndex = _povIndex(pov);
		if (povIndex < 0)
		{
			return NULL_INDEX;
		}

		return _headIndex(povIndex, maxPriority);
	}

	template <typename T, uint64 L>
	sint64 Collection<T, L>::nextElementIndex(sint64 elementIndex) const
	{
		return _nextElementIndex(elementIndex);
	}

	template <typename T, uint64 L>
	inline uint64 Collection<T, L>::population() const
	{
		return _population;
	}

	template <typename T, uint64 L>
	uint64 Collection<T, L>::population(const id& pov) const
	{
		const sint64 povIndex = _povIndex(pov);

		return povIndex < 0 ? 0 : _povs[povIndex].population;
	}

	template <typename T, uint64 L>
	id Collection<T, L>::pov(sint64 elementIndex) const
	{
		return _povs[_elements[elementIndex & (L - 1)].povIndex].value;
	}

	template <typename T, uint64 L>
	sint64 Collection<T, L>::prevElementIndex(sint64 elementIndex) const
	{
		return _previousElementIndex(elementIndex);
	}

	template <typename T, uint64 L>
	sint64 Collection<T, L>::priority(sint64 elementIndex) const
	{
		return _elements[elementIndex & (L - 1)].priority;
	}

	template <typename T, uint64 L>
	sint64 Collection<T, L>::remove(sint64 elementIdx)
	{
		sint64 nextElementIdxOfRemoved = NULL_INDEX;
		elementIdx &= (L - 1);
		if (uint64(elementIdx) < _population)
		{
			auto deleteElementIdx = elementIdx;
			const auto povIndex = _elements[elementIdx].povIndex;
			auto& pov = _povs[povIndex];
			if (pov.population > 1)
			{
				auto& rootIdx = pov.bstRootIndex;
				auto& curElement = _elements[elementIdx];

				nextElementIdxOfRemoved = _nextElementIndex(elementIdx);

				if (curElement.bstRightIndex != NULL_INDEX &&
					curElement.bstLeftIndex != NULL_INDEX)
				{
					// it contains both left and right child
					// -> move next element in priority queue to curElement, delete next element
					const auto tmpIdx = nextElementIdxOfRemoved;
					if (tmpIdx == pov.tailIndex)
					{
						pov.tailIndex = _previousElementIndex(tmpIdx);
					}
					const auto rightTmpIndex = _elements[tmpIdx].bstRightIndex;
					if (tmpIdx == curElement.bstRightIndex)
					{
						curElement.bstRightIndex = rightTmpIndex;
						if (rightTmpIndex != NULL_INDEX)
						{
							_elements[rightTmpIndex].bstParentIndex = elementIdx;
						}
					}
					else
					{
						_elements[_elements[tmpIdx].bstParentIndex].bstLeftIndex = rightTmpIndex;
						if (rightTmpIndex != NULL_INDEX)
						{
							_elements[rightTmpIndex].bstParentIndex = _elements[tmpIdx].bstParentIndex;
						}
					}
					copyMem(&curElement.value, &_elements[tmpIdx].value, sizeof(T));
					curElement.priority = _elements[tmpIdx].priority;
					nextElementIdxOfRemoved = elementIdx;

					deleteElementIdx = tmpIdx;
				}
				else if (curElement.bstRightIndex != NULL_INDEX)
				{
					// contains only right child
					if (elementIdx == pov.headIndex)
					{
						pov.headIndex = nextElementIdxOfRemoved;
					}
					if (!_updateParent(elementIdx, curElement.bstRightIndex))
					{
						rootIdx = curElement.bstRightIndex;
						_elements[rootIdx].bstParentIndex = NULL_INDEX;
					}
				}
				else if (curElement.bstLeftIndex != NULL_INDEX)
				{
					// contains only left child
					if (elementIdx == pov.tailIndex)
					{
						pov.tailIndex = _previousElementIndex(elementIdx);
					}
					if (!_updateParent(elementIdx, curElement.bstLeftIndex))
					{
						rootIdx = curElement.bstLeftIndex;
						_elements[rootIdx].bstParentIndex = NULL_INDEX;
					}
				}
				else // it's a leaf node
				{
					if (elementIdx == pov.headIndex)
					{
						pov.headIndex = nextElementIdxOfRemoved;
					}
					else if (elementIdx == pov.tailIndex)
					{
						pov.tailIndex = _previousElementIndex(elementIdx);
					}
					_updateParent(elementIdx, NULL_INDEX);
				}
				--pov.population;
			}
			else
			{
				pov.population = 0;
				_markRemovalCounter++;
				_povOccupationFlags[povIndex >> 5] ^= (3ULL << ((povIndex & 31) << 1));
			}

			if (--_population && deleteElementIdx != _population)
			{
				// Move last element to fill new gap in array
				_moveElement(_population, deleteElementIdx);
				if (nextElementIdxOfRemoved == _population)
					nextElementIdxOfRemoved = deleteElementIdx;
			}

			const bool CLEAR_UNUSED_ELEMENT = true;
			if (CLEAR_UNUSED_ELEMENT)
			{
				setMem(&_elements[_population], sizeof(Element), 0);
			}
		}

		return nextElementIdxOfRemoved;
	}

	template <typename T, uint64 L>
	void Collection<T, L>::replace(sint64 oldElementIndex, const T& newElement)
	{
		if (uint64(oldElementIndex) < _population)
		{
			_elements[oldElementIndex].value = newElement;
		}
	}

	template <typename T, uint64 L>
	void Collection<T, L>::reset()
	{
		setMem(this, sizeof(*this), 0);
	}

	template <typename T, uint64 L>
	sint64 Collection<T, L>::tailIndex(const id& pov) const
	{
		const sint64 povIndex = _povIndex(pov);

		return povIndex < 0 ? NULL_INDEX : _povs[povIndex].tailIndex;
	}

	template <typename T, uint64 L>
	sint64 Collection<T, L>::tailIndex(const id& pov, sint64 minPriority) const
	{
		const sint64 povIndex = _povIndex(pov);
		if (povIndex < 0)
		{
			return NULL_INDEX;
		}

		return _tailIndex(povIndex, minPriority);
	}
}
