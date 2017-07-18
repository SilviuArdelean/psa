#pragma once
#include "general.h"

class smart_handle
{
	HANDLE handle;
public:
	smart_handle(const HANDLE& h) : handle(h) {}

	~smart_handle()
	{
		if (INVALID_HANDLE_VALUE != handle)
			CloseHandle(handle);
	}

	smart_handle(const smart_handle& rhs)
		: handle(rhs.handle)
	{
	}

	smart_handle& operator = (const HANDLE& h)
	{
		if (handle != &h)
		{
			handle = h;
		}
		return *this;
	}

	operator HANDLE () const
	{
		return handle;
	}

	operator bool() const
	{
		return handle != nullptr;
	}
	bool operator! () const
	{
		return handle == nullptr;
	}
};