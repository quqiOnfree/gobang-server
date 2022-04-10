#pragma once

#define MAX_LONG_LONG 18446744073709551615

void fast_sort(long long*, size_t);
//long long qpow(long long, long long);

template<typename T>
T qpow(T a, size_t n)
{
	T nas = 1;
	for (size_t i = 0; i < n; i++)
		nas *= a;
	return nas;
}

long long randint(long long, long long);
long double random();
long double uniform(long double, long double);
void pt_l_a(long long*, size_t);
void randrange(long long*, size_t);
