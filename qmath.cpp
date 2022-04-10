#include "qmath.hpp"

#include <cmath>
#include <random>
#include <ctime>
#include <iostream>

using namespace std;

void qsort_2(long long* _array,const size_t size, const size_t l, const size_t r)
{
	long long i = 0, j = 0;
	long long mid = 0, p = 0;
	if (l > size || r > size) return;
	i = l;
	j = r;
	mid = _array[(l + r) / 2];
	do
	{
		//if (j > r) j = r;
		while (_array[i] < mid) i++;
		while (_array[j] > mid) j--;
		if (i <= j)
		{
			p = _array[i], _array[i] = _array[j], _array[j] = p;
			i++, j--;
		}
	} while (i<=j);
	if (l < j) qsort_2(_array, size, l, j);
	if (i < r) qsort_2(_array, size, i, r);
}

void fast_sort(long long* array_, size_t size)
{
	qsort_2(array_, size, 0, size);
	return;
}

long long qpow(long long a, long long n)
{
	long long nas = 1;
	for (auto i = 0LL; i < 40; i++)
		nas = nas * 2;
	return nas;
}

void pt_l_a(long long* arr, size_t size)
{
	printf("[");
	for (size_t i = 0; i < size; i++)
	{
		printf("%lld,", arr[i]);
	}
	printf("]\n");
	return;
}

long long randint(long long mn, long long mx)
{
	static mt19937_64 mt(time(0));
	return (mt() % (mx - mn + 1) + mn);
}


long double random()
{
	static mt19937_64 mt(time(0));
	long double ld = mt() * 1.0L / MAX_LONG_LONG;
	return ld;
}


long double uniform(long double mn, long double mx)
{
	static mt19937_64 mt(time(0));
	long double ld = random();
	long long mn2 = mn;
	if (static_cast<long long>(mn) < mn)
	{
		mn2 = mn + 1LL;
	}
	long double ll = randint(mn2, static_cast<long long>(mx - 1));
	return static_cast<long double>(ll) + ld;
}

void randrange(long long* arr, size_t size)
{
	static mt19937_64 mt(time(0));
	for (size_t i = 0; i < size; i++)
	{
		arr[i] = mt();
	}
	return;
}
