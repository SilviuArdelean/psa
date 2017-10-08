#pragma once
#include "general.h"

class string_utils
{
 public:
	static bool compare_case_sensitive(ustring strFirst, ustring strSecond)
	{
		// Convert both strings to upper case by transfrom() before compare.
		std::transform(strFirst.begin(), strFirst.end(), strFirst.begin(), toupper);
		std::transform(strSecond.begin(), strSecond.end(), strSecond.begin(),toupper);

		return (strFirst == strSecond);
	}

	static bool is_filename(const ustring& filename)
	{
		// define a regular expression
		const std::wregex pattern(L"^([a-zA-Z0-9s._-]+)$");
		
		return std::regex_match(filename.cbegin(), filename.cend(), pattern);
	}

	static bool search_substring(const ustring& str, const ustring& sub_str, bool case_sensitive = true)
	{
		std::size_t pos = 0;

		if (case_sensitive)
		{
			ustring path2seach(str);
			ustring str4seach(sub_str);

			std::transform(path2seach.begin(), path2seach.end(), path2seach.begin(), toupper);
			std::transform(str4seach.begin(), str4seach.end(), str4seach.begin(), toupper);

			pos = path2seach.find(str4seach);

			return (pos != std::string::npos);
		}

		pos = str.find(sub_str);

		return (pos != std::string::npos);
	}
};
