#pragma once
#include "general.h"
#include <algorithm>
#include <regex>

class string_utils
{
 public:

	static bool compare_case_sensitive(ustring strFirst, ustring strSecond)
	{
		#ifdef __linux__
			return (0 == strFirst.compare(strSecond));
		#else	
			// Convert both strings to upper case by transfrom() before compare.
			std::transform(strFirst.begin(), strFirst.end(), strFirst.begin(), toupper);
			std::transform(strSecond.begin(), strSecond.end(), strSecond.begin(), toupper);		

			return (strFirst == strSecond);
		#endif
	}

	static bool is_filename(const ustring& filename)
	{
		#ifdef UNICODE
			const std::wregex pattern(_T("^([a-zA-Z0-9s._-]+)$"));
		#else
			const std::regex pattern("^([a-zA-Z0-9s._-]+)$");
		#endif
		
		return std::regex_match(filename.cbegin(), filename.cend(), pattern);
	}

	static bool search_substring(const ustring& str, const ustring& sub_str, bool case_sensitive = true)
	{
		std::size_t pos = 0;

		if (case_sensitive)
		{
			ustring path2seach(str);
			ustring str4seach(sub_str);

		#ifdef _WIN32
			std::transform(path2seach.begin(), path2seach.end(), path2seach.begin(), toupper);
			std::transform(str4seach.begin(), str4seach.end(), str4seach.begin(), toupper);
		#endif

			pos = path2seach.find(str4seach);

			return (pos != std::string::npos);
		}

		pos = str.find(sub_str);

		return (pos != std::string::npos);
	}
};
