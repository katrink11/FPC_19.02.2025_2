#include <algorithm>
#include <cassert>
#include <iostream>
#include <random>
#include <vector>
#include <chrono>
#include <omp.h>

#define THREADS 4

#define PERF_START(id) \
	auto start_##id = std::chrono::high_resolution_clock::now();

#define PERF_END(id) \
	auto end_##id = std::chrono::high_resolution_clock::now();

#define PERF_RESULT(id) \
	std::chrono::duration_cast<std::chrono::milliseconds>(end_##id - start_##id).count()

void sort(std::vector<int> &array)
{
	bool sorted = false;
	const int size = array.size();
	while (!sorted)
	{
		sorted = true;
		for (int i = 1; i < size - 1; i += 2)
		{
			if (array[i] < array[i + 1])
			{
				std::swap(array[i], array[i + 1]);
				sorted = false;
			}
		}
		for (int i = 0; i < size - 1; i += 2)
		{
			if (array[i] < array[i + 1])
			{
				std::swap(array[i], array[i + 1]);
				sorted = false;
			}
		}
	}
}

void sort_parallel(std::vector<int> &array)
{
	const int size = array.size();
	bool sorted = false;
	while (!sorted)
	{
		bool swapped_any = false;

		// Нечетная фаза
		bool swapped_odd = false;
#pragma omp parallel for reduction(|| : swapped_odd) schedule(static)
		for (int i = 1; i < size - 1; i += 2)
		{
			if (array[i] < array[i + 1])
			{
				std::swap(array[i], array[i + 1]);
				swapped_odd = true;
			}
		}

		// Четная фаза
		bool swapped_even = false;
#pragma omp parallel for reduction(|| : swapped_even) schedule(static)
		for (int i = 0; i < size - 1; i += 2)
		{
			if (array[i] < array[i + 1])
			{
				std::swap(array[i], array[i + 1]);
				swapped_even = true;
			}
		}

		sorted = !(swapped_odd || swapped_even);
	}
}

std::vector<int> generate(int n)
{
	std::vector<int> result;
	for (int i = 0; i < n; i++)
	{
		result.push_back(rand() % 200 - 100);
	}
	return result;
}

bool check(const std::vector<int> &a)
{
	bool sorted = true;
#pragma omp parallel for reduction(&& : sorted)
	for (int i = 0; i < a.size() - 1; i++)
	{
		if (a[i] < a[i + 1])
		{
			sorted = false;
		}
	}
	return sorted;
}

int main()
{
	omp_set_num_threads(THREADS);

	std::vector<int> vec = generate(100000);
	std::vector<int> copy = vec;

	PERF_START(serial)
	sort(vec);
	PERF_END(serial)

	PERF_START(parallel)
	sort_parallel(copy);
	PERF_END(parallel)

	assert(check(vec));
	assert(check(copy));

	std::cout << "SERIAL: " << PERF_RESULT(serial) << "ms" << std::endl;
	std::cout << "PARALLEL: " << PERF_RESULT(parallel) << "ms" << std::endl;

	return 0;
}
