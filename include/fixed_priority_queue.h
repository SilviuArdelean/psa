#pragma once

#include <vector>
#include <queue>
#include <algorithm>

template<class _Ty,
	class _Container = std::vector<_Ty>,
	class _Pr = std::less<typename _Container::value_type> >
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
		if (fixed_size == std::priority_queue<_Ty, _Container, _Pr>::size())
		{
			auto min = std::min_element(std::priority_queue<_Ty, _Container, _Pr>::c.begin(), 
											std::priority_queue<_Ty, _Container, _Pr>::c.end(), 
											_Pr());
			if (x > *min)
			{
				*min = x;
	
				// Re-make the heap, since we may have just invalidated it.
				std::make_heap(std::priority_queue<_Ty, _Container, _Pr>::c.begin(), 
										std::priority_queue<_Ty, _Container, _Pr>::c.end());
			}
		}
		else
		{
			std::priority_queue<_Ty, _Container, _Pr>::emplace(x);
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