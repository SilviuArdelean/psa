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

	typedef typename  std::priority_queue<_Ty, _Container, _Pr> _PQ_specialization;

	void push(const _Ty& x)
	{
		if (fixed_size == _PQ_specialization::size())
		{
			auto min = std::min_element(_PQ_specialization::c.begin(),
										_PQ_specialization::c.end(),
											_Pr());
			if (x > *min)
			{
				*min = x;
	
				// Re-make the heap, since we may have just invalidated it.
				std::make_heap(_PQ_specialization::c.begin(),
									_PQ_specialization::c.end());
			}
		}
		else
		{
			_PQ_specialization::emplace(x);
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