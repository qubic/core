#define NO_UEFI

#include "gtest/gtest.h"

#include "../src/smart_contracts/qpi.h"
#include "../src/smart_contracts/useful_functions.h"

#include <vector>
#include <random>

using namespace QPI;

const int gseed = 12345;

template<typename T, typename TrandomOp>
std::vector<T> generate_vec(TrandomOp& randomOp, int n, bool checkDuplication)
{
	std::vector<T> vec;
	vec.reserve(n);
	for (int i = 0; i < n; ++i)
	{
		while (true)
		{
			auto v = static_cast<T>(randomOp());
			bool is_duplicated = false;
			if (checkDuplication)
			{
				is_duplicated = std::find(vec.begin(), vec.begin() + i, v) != vec.end();
			}
			if (!is_duplicated)
			{
				vec.push_back(v);
				break;
			}
		}
	}
	return vec;
}

template<typename Tarray, typename Telement>
void set_array_data(Tarray& arr, const std::vector<Telement>& elements)
{
	for (uint32_t i = 0; i < elements.size(); ++i)
	{
		arr.set(i, elements.at(i));
	}
}

TEST(TestCoreQPI, UsefulFunctionSearch)
{
	// generate array
	const int seed = gseed;
	std::mt19937_64 gen64(seed);

	QPI::uint64_8 arr;
	const int n = 8;
	auto tmp_vec = generate_vec<QPI::uint64, std::mt19937_64>(gen64, n + 1, true);

	//auto tmp_vec = generate_vec(n + 1, seed);
	const auto not_existed_v = tmp_vec.at(n);
	tmp_vec.resize(n);
	set_array_data(arr, tmp_vec);
	int gt_idx = 3;
	auto gt_v = arr.get(gt_idx);

	// 1a. test: found
	EXPECT_EQ(gt_idx, QPI::search(arr, gt_v, n, false));

	// 1b. test: not found
	EXPECT_EQ(-1, QPI::search(arr, not_existed_v, n, false));

	// 1c. test: found at the first occurence
	for (int j = 0, idx = gt_idx + 1; j < 3 && idx < n; ++j, ++idx)
	{
		arr.set(idx, gt_v);
	}
	EXPECT_EQ(gt_idx, QPI::search(arr, gt_v, n, false));

	// sort vec
	std::sort(tmp_vec.begin(), tmp_vec.end());
	QPI::uint64_8 sorted_arr;
	set_array_data(sorted_arr, tmp_vec);
	gt_idx = 3;
	gt_v = sorted_arr.get(gt_idx);

	// 2a. test sorted array: found
	EXPECT_EQ(gt_idx, QPI::search(sorted_arr, gt_v, n, true));

	// 2b. test sorted array: not found
	EXPECT_EQ(-1, QPI::search(sorted_arr, not_existed_v, n, true));

	// 2c. test sorted array: found at the first occurence
	for (int j = 0, idx = gt_idx + 1; j < 3 && idx < n; ++j, ++idx)
	{
		sorted_arr.set(idx, gt_v);
	}
	EXPECT_EQ(gt_idx, QPI::search(sorted_arr, gt_v, n, true));
}

TEST(TestCoreQPI, UsefulFunctionSum)
{
	// generate array
	const int seed = gseed + 1;
	std::mt19937_64 gen64(seed);
	QPI::sint64_512 arr;
	const int n = 512;
	auto tmp_vec = generate_vec<QPI::sint64, std::mt19937_64>(gen64, n, false);
	set_array_data(arr, tmp_vec);

	QPI::sint64 total = 0;
	for (size_t i = 0; i < tmp_vec.size(); ++i)
	{
		total += tmp_vec.at(i);
	}
	auto sum_arr = QPI::sum<QPI::sint64_512, QPI::sint64>(arr, n);
	EXPECT_EQ(total, sum_arr);
}

TEST(TestCoreQPI, UsefulFunctionMin)
{
	// generate array
	const int seed = gseed + 2;
	std::mt19937_64 gen64(seed);
	QPI::sint64_1024 arr;
	const int n = 1024;
	auto tmp_vec = generate_vec<QPI::sint64, std::mt19937_64>(gen64, n, false);
	set_array_data(arr, tmp_vec);

	auto iter = std::min_element(tmp_vec.begin(), tmp_vec.end());
	auto gt_val = *iter;
	auto gt_idx = std::distance(tmp_vec.begin(), iter);

	// 1. test: min
	auto val = QPI::min< QPI::sint64_1024, QPI::sint64>(arr, n, false);
	EXPECT_EQ(gt_val, val);

	// 2. test: min index
	auto idx = QPI::minIndex(arr, n, false);
	EXPECT_EQ(gt_idx, idx);

	// sort array
	std::sort(tmp_vec.begin(), tmp_vec.end());
	set_array_data(arr, tmp_vec);
	iter = std::min_element(tmp_vec.begin(), tmp_vec.end());
	gt_val = *iter;
	gt_idx = std::distance(tmp_vec.begin(), iter);
	EXPECT_EQ(gt_idx, 0);
	EXPECT_EQ(gt_val, *tmp_vec.begin());

	// 3. test sorted array: min
	val = QPI::min< QPI::sint64_1024, QPI::sint64>(arr, n, true);
	EXPECT_EQ(gt_val, val);

	// 4. test sorted array: min index
	idx = QPI::minIndex(arr, n, true);
	EXPECT_EQ(gt_idx, idx);
}

TEST(TestCoreQPI, UsefulFunctionMax)
{
	// generate array
	const int seed = gseed + 3;
	std::mt19937_64 gen64(seed);
	QPI::sint64_1024 arr;
	const int n = 1024;
	auto tmp_vec = generate_vec<QPI::sint64, std::mt19937_64>(gen64, n, false);
	set_array_data(arr, tmp_vec);

	auto iter = std::max_element(tmp_vec.begin(), tmp_vec.end());
	auto gt_val = *iter;
	auto gt_idx = std::distance(tmp_vec.begin(), iter);

	// 1. test: max
	auto val = QPI::max< QPI::sint64_1024, QPI::sint64>(arr, n, false);
	EXPECT_EQ(gt_val, val);

	// 2. test: max index
	auto idx = QPI::maxIndex(arr, n, false);
	EXPECT_EQ(gt_idx, idx);

	// sort array
	std::sort(tmp_vec.begin(), tmp_vec.end());
	set_array_data(arr, tmp_vec);
	iter = std::max_element(tmp_vec.begin(), tmp_vec.end());
	gt_val = *iter;
	gt_idx = std::distance(tmp_vec.begin(), iter);
	EXPECT_EQ(gt_idx, tmp_vec.size() - 1);
	EXPECT_EQ(gt_val, *(tmp_vec.end() - 1));

	// 3. test sorted array: max
	val = QPI::max< QPI::sint64_1024, QPI::sint64>(arr, n, true);
	EXPECT_EQ(gt_val, val);

	// 4. test sorted array: max index
	idx = QPI::maxIndex(arr, n, true);
	EXPECT_EQ(gt_idx, idx);
}

template<typename Tarray, typename Telement, typename TrandomOp>
void test_useful_function_unique(
	const int n, TrandomOp& randomOp, bool randomNonDuplicationCheck)
{
	Tarray arr;
	auto tmp_vec = generate_vec<Telement, TrandomOp>(randomOp, n, randomNonDuplicationCheck);
	std::sort(tmp_vec.begin(), tmp_vec.end());
	set_array_data(arr, tmp_vec);

	// gt
	auto last = std::unique(tmp_vec.begin(), tmp_vec.end());
	tmp_vec.erase(last, tmp_vec.end());
	// qpi
	int new_n = n;
	QPI::unique(arr, new_n);

	// test new size
	EXPECT_GT(new_n, 0);
	EXPECT_EQ(new_n, tmp_vec.size());
	if (randomNonDuplicationCheck)
	{
		EXPECT_EQ(new_n, n);
	}
	else
	{
		EXPECT_LT(new_n, n);
	}
	// test elements matched
	for (int i = 0; i < new_n; i++)
	{
		if (tmp_vec.at(i) != arr.get(i))
		{
			EXPECT_EQ(tmp_vec.at(i), arr.get(i));
			break;
		}
	}
}

TEST(TestCoreQPI, UsefulFunctionUnique)
{
	// generate array
	const int seed = gseed + 4;
	std::mt19937_64 gen64(seed);

	// 1. test: non duplication
	test_useful_function_unique<QPI::uint8_16, QPI::uint8, std::mt19937_64>(16, gen64, true);

	// 2. test: duplication
	test_useful_function_unique<QPI::uint8_1024, QPI::uint8, std::mt19937_64>(1024, gen64, false);
}

template<typename Tarray, typename Telement>
bool test_elements_matched(Tarray& arr, const int n, const std::vector<Telement>& vec)
{
	for (int i = 0; i < n; i++)
	{
		if (vec.at(i) != arr.get(i))
		{
			EXPECT_EQ(vec.at(i), arr.get(i));
			return false;
		}
	}
	return true;
}

template<typename Tarray, typename Telement, typename TrandomOp>
void test_useful_function_sort(
	const int n, TrandomOp& randomOp, bool randomNonDuplicationCheck)
{
	Tarray arr;
	auto tmp_vec = generate_vec<Telement, TrandomOp>(randomOp, n, randomNonDuplicationCheck);
	std::sort(tmp_vec.begin(), tmp_vec.end());
	set_array_data(arr, tmp_vec);

	// gt
	auto last = std::unique(tmp_vec.begin(), tmp_vec.end());
	tmp_vec.erase(last, tmp_vec.end());
	// qpi
	int new_n = n;
	QPI::unique(arr, new_n);

	// test new size
	EXPECT_GT(new_n, 0);
	EXPECT_EQ(new_n, tmp_vec.size());
	if (randomNonDuplicationCheck)
	{
		EXPECT_EQ(new_n, n);
	}
	else
	{
		EXPECT_LT(new_n, n);
	}
	// test elements matched
	test_elements_matched(arr, new_n, tmp_vec);
}

TEST(TestCoreQPI, UsefulFunctionSort)
{
	// generate array
	const int seed = gseed + 5;
	std::mt19937_64 gen64(seed);
	QPI::sint64_1024 arr;
	const int n = 1024;
	auto tmp_vec = generate_vec<QPI::sint64, std::mt19937_64>(gen64, n, false);
	set_array_data(arr, tmp_vec);

	std::sort(tmp_vec.begin(), tmp_vec.end());

	QPI::sort(arr, n);

	test_elements_matched(arr, n, tmp_vec);
}

TEST(TestCoreQPI, UsefulFunctionSortByKey)
{
	// generate array
	const int seed = gseed + 6;
	std::mt19937_64 gen64(seed);
	QPI::sint32_1024 keys;
	QPI::sint64_1024 values;
	const int n = 1024;

	std::vector<bool> dup_modes({ true, false });
	for (auto dup_mode : dup_modes)
	{
		auto tmp_keys = generate_vec<QPI::sint32, std::mt19937_64>(gen64, n, !dup_mode);
		auto tmp_values = generate_vec<QPI::sint64, std::mt19937_64>(gen64, n, false);
		set_array_data(keys, tmp_keys);
		set_array_data(values, tmp_values);

		// gt: std::sort
		std::vector<std::pair<QPI::sint32, QPI::sint64>> tmp_vec(n);
		for (int i = 0; i < n; i++)
		{
			tmp_vec[i] = std::make_pair(tmp_keys.at(i), tmp_values.at(i));
		}
		std::sort(
			tmp_vec.begin(), tmp_vec.end(),
			[](const std::pair<QPI::sint32, QPI::sint64>& lhs,
				const std::pair<QPI::sint32, QPI::sint64>& rhs) {
					return lhs.first <= rhs.first;
			});
		for (int i = 0; i < n; i++)
		{
			tmp_keys.at(i) = tmp_vec.at(i).first;
			tmp_values.at(i) = tmp_vec.at(i).second;
		}

		// qpi's sort
		QPI::sort(keys, values, n);

		// test keys matched
		test_elements_matched(keys, n, tmp_keys);

		// test values matched
		if (!dup_mode)
		{
			// no-duplication mode requires values index-by-index matched completely
			test_elements_matched(values, n, tmp_values);
		}
	}
}
