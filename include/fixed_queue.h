#pragma once

#include <vector>
#include <queue>
#include <algorithm>
#include "string_utils.h"

template<class _Ty,
	class _Container = std::vector<_Ty>,
	class _Pr = std::less<typename _Container::value_type> >
class fixed_queue : public std::priority_queue<_Ty, _Container, _Pr>
{

public:
   fixed_queue() {}
   fixed_queue(size_t size)
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


   void pop()
   {
      auto itFirst = _PQ_specialization::c.begin();

      if (itFirst != _PQ_specialization::c.end())
         _PQ_specialization::c.erase(itFirst);
   }

   void clear()
   {
      _PQ_specialization::c.clear();
   }

   template
      <typename Comparator>
   bool erase(std::string const& str_id, Comparator &&comp)
   {
      for (auto it = _PQ_specialization::c.begin(); it != _PQ_specialization::c.end(); ++it)
      {
         if (comp(str_id, it))
         {
            _PQ_specialization::c.erase(it);
            return true;
         }
      }

      return false;
   }

   template 
    <typename Comparator>
   _Ty* find(std::string str_id, Comparator &&comp)
   {
      for (auto it = _PQ_specialization::c.begin(); it != _PQ_specialization::c.end(); ++it)
      {
         if (comp(str_id, it))
         {
            return &*it;
         }
      }

      return nullptr;
   }

   size_t   size() const { return _PQ_specialization::c.size(); }
   bool     is_full() const {      return fixed_size == _PQ_specialization::c.size();    }

   auto     begin() -> decltype(_PQ_specialization::c.begin()) { return  _PQ_specialization::c.begin(); }
   auto     cbegin() -> decltype(_PQ_specialization::c.cbegin()) { return  _PQ_specialization::c.cbegin(); }

   auto     end() -> decltype(_PQ_specialization::c.end()) { return  _PQ_specialization::c.end(); }
   auto     cend() -> decltype(_PQ_specialization::c.cend()) { return  _PQ_specialization::c.cend(); }

private:
	const size_t fixed_size;
};