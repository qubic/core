// Qubic's useful functions

#pragma once

#include "../platform/memory.h"

namespace QPI
{
	/// <summary>
	/// search the exact value
	/// </summary>
	/// <typeparam name="T">data type of array</typeparam>
	/// <typeparam name="Telement">data type of element</typeparam>
	/// <param name="arr">input array</param>
	/// <param name="val">value to search</param>
	/// <param name="n">elements count</param>
	/// <param name="isSorted">a boolean value, specifies the input array sorted or not</param>
	/// <returns>the index. -1 if it doesn't exist</returns>
	template <typename T, typename Telement>
	static int search(T& arr, const Telement val, const int n, bool isSorted)
	{
		if (isSorted)
		{
			for (int left = 0, right = n - 1; left <= right;)
			{
				const int mid = (left + right) / 2;
				const auto mid_val = arr.get(mid);
				if (mid_val == val)
				{
					if (mid == left)
					{
						return mid;
					}
					right = mid;
				}
				else if (mid_val < val)
				{
					left = mid + 1;
				}
				else
				{
					right = mid - 1;
				}
			}
		}
		else
		{
			for (int i = 0; i < n; ++i)
			{
				if (arr.get(i) == val)
				{
					return i;
				}
			}
		}
		return -1;
	}

	/// <summary>
	/// sum of an array
	/// </summary>
	/// <typeparam name="T">data type of array</typeparam>
	/// <typeparam name="Tout">data type of output</typeparam>
	/// <param name="arr">input array</param>
	/// <param name="n">elements count</param>
	/// <returns>sum of an array</returns>
	template <typename T, typename Tout>
	static Tout sum(T& arr, const int n)
	{
		Tout ret;
		setMem(&ret, sizeof(Tout), 0);
		for (int i = 0; i < n; ++i)
		{
			ret += static_cast<Tout>(arr.get(i));
		}
		return ret;
	}

	/// <summary>
	/// find index of the min element
	/// </summary>
	/// <typeparam name="T">data type of array</typeparam>
	/// <param name="arr">input array</param>
	/// <param name="n">elements count</param>
	/// <param name="isSorted">a boolean value, specifies the input array sorted or not</param>
	/// <returns>index of the min element</returns>
	template <typename T>
	static int minIndex(T& arr, const int n, const bool isSorted)
	{
		if (isSorted)
		{
			return 0;
		}
		int min_idx = 0;
		auto min_val = arr.get(min_idx);
		for (int i = min_idx + 1; i < n; ++i)
		{
			const auto val = arr.get(i);
			if (val < min_val)
			{
				min_val = val;
				min_idx = i;
			}
		}
		return min_idx;
	}

	/// <summary>
	/// find index of the max element
	/// </summary>
	/// <typeparam name="T">data type of array</typeparam>
	/// <param name="arr">input array</param>
	/// <param name="n">elements count</param>
	/// <param name="isSorted">a boolean value, specifies the input array sorted or not</param>
	/// <returns>index of the max element</returns>
	template <typename T>
	static int maxIndex(T& arr, const int n, const bool isSorted)
	{
		if (isSorted)
		{
			return (n - 1);
		}
		int max_idx = 0;
		auto max_val = arr.get(max_idx);
		for (int i = max_idx + 1; i < n; ++i)
		{
			const auto val = arr.get(i);
			if (val > max_val)
			{
				max_val = val;
				max_idx = i;
			}
		}
		return max_idx;
	}

	/// <summary>
	/// find the min element of an array
	/// </summary>
	/// <typeparam name="T">data type of array</typeparam>
	/// <param name="arr">input array</param>
	/// <param name="n">elements count</param>
	/// <param name="isSorted">a boolean value, specifies the input array sorted or not</param>
	/// <returns>the min element</returns>
	template <typename T, typename Telement>
	static Telement min(T& arr, const int n, const bool isSorted)
	{
		const int min_idx = minIndex<T>(arr, n, isSorted);
		return static_cast<Telement>(arr.get(min_idx));
	}

	/// <summary>
	/// find the max element of an array
	/// </summary>
	/// <typeparam name="T">data type of array</typeparam>
	/// <param name="arr">input array</param>
	/// <param name="n">elements count</param>
	/// <param name="isSorted">a boolean value, specifies the input array sorted or not</param>
	/// <returns>the max element</returns>
	template <typename T, typename Telement>
	static Telement max(T& arr, const int n, const bool isSorted)
	{
		const int max_idx = maxIndex<T>(arr, n, isSorted);
		return static_cast<Telement>(arr.get(max_idx));
	}

	/// <summary>
	/// remove duplicates of any element in a sorted array
	/// </summary>
	/// <typeparam name="T">data type of array</typeparam>
	/// <param name="sortedArray">input sorted array</param>
	/// <param name="n">elements count</param>
	template <typename T>
	static void unique(T& sortedArray, int& n)
	{
		if (n > 1)
		{
			int new_n = 1;
			auto val = sortedArray.get(0);
			for (int i = 1; i < n; ++i)
			{
				const auto val_i = sortedArray.get(i);
				if (val_i != val)
				{
					val = val_i;
					if (i != new_n)
					{
						sortedArray.set(new_n, val);
					}
					new_n++;
				}
			}
			n = new_n;
		}
	}

	/// <summary>
	/// sort an array
	/// </summary>
	/// <typeparam name="T">data type of array</typeparam>
	/// <param name="arr">input array</param>
	/// <param name="n">elements count</param>
	template <typename T>
	static void sort(T& arr, const int n)
	{
		const int RUN = 32;
		T buffer; // not initialize

		// sort individual subarrays of size RUN
		for (int i = 0; i < n; i += RUN)
		{
			timsort::insertionSort(arr, i, timsort::min((i + RUN - 1), (n - 1)));
		}

		// start merging from size RUN (or 32), then 64, 128 and so on....
		for (int size = RUN; size < n; size = 2 * size)
		{
			for (int left = 0; left < n; left += 2 * size)
			{
				const int mid = left + size - 1;
				const int right = timsort::min((left + 2 * size - 1), (n - 1));
				if (mid < right)
				{
					timsort::mergeSort(arr, left, mid, right, buffer);
				}
			}
		}
	}

	/// <summary>
	/// performs a key-value sort
	/// </summary>
	/// <typeparam name="Tkey">data type of key array</typeparam>
	/// <typeparam name="Tval">data type of value array</typeparam>
	/// <param name="keys">key array</param>
	/// <param name="values">value array</param>
	/// <param name="n">elements count</param>
	template <typename Tkey, typename Tval>
	static void sort(Tkey& keys, Tval& values, const int n)
	{
		const int RUN = 32;
		Tkey keyBuffer; // not initialize
		Tval valBuffer; // not initialize

		// sort individual subarrays of size RUN
		for (int i = 0; i < n; i += RUN)
		{
			timsort::insertionSort(keys, values, i, timsort::min((i + RUN - 1), (n - 1)));
		}

		// start merging from size RUN (or 32), then 64, 128 and so on....
		for (int size = RUN; size < n; size = 2 * size)
		{
			for (int left = 0; left < n; left += 2 * size)
			{
				const int mid = left + size - 1;
				const int right = timsort::min((left + 2 * size - 1), (n - 1));
				if (mid < right)
				{
					timsort::mergeSort(keys, values, left, mid, right, keyBuffer, valBuffer);
				}
			}
		}
	}

	namespace timsort
	{
		template<typename T>
		static void insertionSort(T& arr, const int left, const int right)
		{
			for (int i = left + 1; i <= right; i++)
			{
				const auto val_i = arr.get(i);
				int j = i - 1;
				while (j >= left)
				{
					const auto val_j = arr.get(j);
					if (val_j <= val_i)
					{
						break;
					}
					arr.set(j + 1, val_j);
					j--;
				}
				if (++j != i)
				{
					arr.set(j, val_i);
				}
			}
		}

		template<typename Tkey, typename Tval>
		static void insertionSort(Tkey& keys, Tval& values, const int left, const int right)
		{
			for (int i = left + 1; i <= right; i++)
			{
				const auto keyi = keys.get(i);
				const auto vali = values.get(i);
				int j = i - 1;
				while (j >= left)
				{
					const auto keyj = keys.get(j);
					if (keyj <= keyi)
					{
						break;
					}
					keys.set(j + 1, keyj);
					values.set(j + 1, values.get(j));
					j--;
				}
				if (++j != i)
				{
					keys.set(j, keyi);
					values.set(j, vali);
				}
			}
		}

		template<typename T>
		static void mergeSort(T& arr, const int l, const int m, const int r, T& buffer)
		{
			for (int i = l; i <= r; ++i)
			{
				buffer.set(i, arr.get(i));
			}

			int i = l;
			int j = m + 1;
			int k = l;
			for (; i <= m && j <= r;)
			{
				const auto vali = buffer.get(i);
				const auto valj = buffer.get(j);
				if (vali <= valj)
				{
					arr.set(k++, vali);
					i++;
				}
				else
				{
					arr.set(k++, valj);
					j++;
				}
			}

			while (i <= m)
			{
				arr.set(k++, buffer.get(i++));
			}

			while (j <= r)
			{
				arr.set(k++, buffer.get(j++));
			}
		}

		template<typename Tkey, typename Tval>
		static void mergeSort(
			Tkey& keys, Tval& values, const int l, const int m, const int r, Tkey& keyBuffer, Tval& valBuffer)
		{
			for (int i = l; i <= r; ++i)
			{
				keyBuffer.set(i, keys.get(i));
				valBuffer.set(i, values.get(i));
			}

			int i = l;
			int j = m + 1;
			int k = l;
			for (; i <= m && j <= r; k++)
			{
				const auto keyi = keyBuffer.get(i);
				const auto keyj = keyBuffer.get(j);
				if (keyi <= keyj)
				{
					keys.set(k, keyi);
					values.set(k, valBuffer.get(i++));
				}
				else
				{
					keys.set(k, keyj);
					values.set(k, valBuffer.get(j++));
				}
			}

			for (; i <= m; ++i, ++k)
			{
				keys.set(k, keyBuffer.get(i));
				values.set(k, valBuffer.get(i));
			}

			for (; j <= r; ++j, ++k)
			{
				keys.set(k, keyBuffer.get(j));
				values.set(k, valBuffer.get(j));
			}
		}

		template<typename T>
		inline T min(T a, T b)
		{
			return (a < b ? a : b);
		}
	}
}
