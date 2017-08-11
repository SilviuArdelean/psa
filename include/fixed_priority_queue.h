#pragma once

#include <queue>
#include <algorithm>

template<class _Ty,
	class _Container = vector<_Ty>,
	class _Pr = less<typename _Container::value_type> >
class fixed_priority_queue : public std::priority_queue<_Ty, _Container, _Pr>
{
	fixed_priority_queue() {}

public:
	fixed_priority_queue(unsigned int size) 
		: fixed_size(size) 
	{
	}

	void push(const _Ty& x)
	{
		if (fixed_size == size())
		{
			auto min = std::min_element(c.begin(), c.end(), _Pr());
			if (x > *min)
			{
				*min = x;
	
				// Re-make the heap, since we may have just invalidated it.
				std::make_heap(c.begin(), c.end());
			}
		}
		else
		{
			priority_queue::emplace(x);
		}
	}
	
private:
	const unsigned int fixed_size;

	// Prevent heap allocation
	void * operator new(size_t) = delete;
	void * operator new[](size_t) = delete;
	void   operator delete   (void *) = delete;
	void   operator delete[](void*) = delete;
};